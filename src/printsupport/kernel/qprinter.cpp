// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qprinter.h"
#include "qprinter_p.h"

#ifndef QT_NO_PRINTER

#include <qpa/qplatformprintplugin.h>
#include <qpa/qplatformprintersupport.h>

#include "qprintengine.h"
#include "qlist.h"
#include <qcoreapplication.h>
#include <qfileinfo.h>

#include <private/qpagedpaintdevice_p.h>

#include "qprintengine_pdf_p.h"

#include <qpicture.h>
#if QT_CONFIG(printpreviewwidget)
#include <private/qpaintengine_preview_p.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#define ABORT_IF_ACTIVE(location) \
    if (d->printEngine->printerState() == QPrinter::Active) { \
        qWarning("%s: Cannot be changed while printer is active", location); \
        return; \
    }

#define ABORT_IF_ACTIVE_RETURN(location, retValue) \
    if (d->printEngine->printerState() == QPrinter::Active) { \
        qWarning("%s: Cannot be changed while printer is active", location); \
        return retValue; \
    }

Q_GUI_EXPORT extern qreal qt_pixelMultiplier(int resolution);
extern QMarginsF qt_convertMargins(const QMarginsF &margins, QPageLayout::Unit fromUnits, QPageLayout::Unit toUnits);

QPrinterInfo QPrinterPrivate::findValidPrinter(const QPrinterInfo &printer)
{
    // Try find a valid printer to use, either the one given, the default or the first available
    QPrinterInfo printerToUse = printer;
    if (printerToUse.isNull()) {
        printerToUse = QPrinterInfo::defaultPrinter();
        if (printerToUse.isNull()) {
            QStringList availablePrinterNames = QPrinterInfo::availablePrinterNames();
            if (!availablePrinterNames.isEmpty())
                printerToUse = QPrinterInfo::printerInfo(availablePrinterNames.at(0));
        }
    }
    return printerToUse;
}

void QPrinterPrivate::initEngines(QPrinter::OutputFormat format, const QPrinterInfo &printer)
{
    // Default to PdfFormat
    outputFormat = QPrinter::PdfFormat;
    QPlatformPrinterSupport *ps = nullptr;
    QString printerName;

    // Only set NativeFormat if we have a valid plugin and printer to use
    if (format == QPrinter::NativeFormat) {
        ps = QPlatformPrinterSupportPlugin::get();
        QPrinterInfo printerToUse = findValidPrinter(printer);
        if (ps && !printerToUse.isNull()) {
            outputFormat = QPrinter::NativeFormat;
            printerName = printerToUse.printerName();
        }
    }

    if (outputFormat == QPrinter::NativeFormat) {
        printEngine = ps->createNativePrintEngine(printerMode, printerName);
        paintEngine = ps->createPaintEngine(printEngine, printerMode);
    } else {
        static const QHash<QPrinter::PdfVersion, QPdfEngine::PdfVersion> engineMapping {
            {QPrinter::PdfVersion_1_4, QPdfEngine::Version_1_4},
            {QPrinter::PdfVersion_A1b, QPdfEngine::Version_A1b},
            {QPrinter::PdfVersion_1_6, QPdfEngine::Version_1_6}
        };
        const auto pdfEngineVersion = engineMapping.value(pdfVersion, QPdfEngine::Version_1_4);
        QPdfPrintEngine *pdfEngine = new QPdfPrintEngine(printerMode, pdfEngineVersion);
        paintEngine = pdfEngine;
        printEngine = pdfEngine;
    }

    use_default_engine = true;
    had_default_engines = true;
    validPrinter = true;
}

void QPrinterPrivate::changeEngines(QPrinter::OutputFormat format, const QPrinterInfo &printer)
{
    QPrintEngine *oldPrintEngine = printEngine;
    const bool def_engine = use_default_engine;

    initEngines(format, printer);

    if (oldPrintEngine) {
        const auto properties = m_properties; // take a copy: setProperty() below modifies m_properties
        for (const auto &key : properties) {
            QVariant prop;
            // PPK_NumberOfCopies need special treatmeant since it in most cases
            // will return 1, disregarding the actual value that was set
            // PPK_PrinterName also needs special treatment as initEngines has set it already
            if (key == QPrintEngine::PPK_NumberOfCopies)
                prop = QVariant(q_ptr->copyCount());
            else if (key != QPrintEngine::PPK_PrinterName)
                prop = oldPrintEngine->property(key);
            if (prop.isValid())
                setProperty(key, prop);
        }
    }

    if (def_engine)
        delete oldPrintEngine;
}

#if QT_CONFIG(printpreviewwidget)
QList<const QPicture *> QPrinterPrivate::previewPages() const
{
    if (previewEngine)
        return previewEngine->pages();
    return QList<const QPicture *>();
}

bool QPrinterPrivate::previewMode() const
{
    return (previewEngine != nullptr) && (previewEngine == printEngine);
}

void QPrinterPrivate::setPreviewMode(bool enable)
{
    Q_Q(QPrinter);
    if (enable) {
        if (!previewEngine)
            previewEngine = new QPreviewPaintEngine;
        had_default_engines = use_default_engine;
        use_default_engine = false;
        realPrintEngine = printEngine;
        realPaintEngine = paintEngine;
        q->setEngines(previewEngine, previewEngine);
        previewEngine->setProxyEngines(realPrintEngine, realPaintEngine);
    } else {
        q->setEngines(realPrintEngine, realPaintEngine);
        use_default_engine = had_default_engines;
    }
}
#endif // QT_CONFIG(printpreviewwidget)

void QPrinterPrivate::setProperty(QPrintEngine::PrintEnginePropertyKey key, const QVariant &value)
{
    printEngine->setProperty(key, value);
    m_properties.insert(key);
}


class QPrinterPagedPaintDevicePrivate : public QPagedPaintDevicePrivate
{
public:
    QPrinterPagedPaintDevicePrivate(QPrinter *p)
        : QPagedPaintDevicePrivate(), m_printer(p)
    {}

    virtual ~QPrinterPagedPaintDevicePrivate()
    {}

    bool setPageLayout(const QPageLayout &newPageLayout) override
    {
        QPrinterPrivate *pd = QPrinterPrivate::get(m_printer);

        if (pd->paintEngine->type() != QPaintEngine::Pdf
            && pd->printEngine->printerState() == QPrinter::Active) {
            qWarning("QPrinter::setPageLayout: Cannot be changed while printer is active");
            return false;
        }

        // Try to set the print engine page layout
        pd->setProperty(QPrintEngine::PPK_QPageLayout, QVariant::fromValue(newPageLayout));

        return pageLayout().isEquivalentTo(newPageLayout);
    }

    bool setPageSize(const QPageSize &pageSize) override
    {
        QPrinterPrivate *pd = QPrinterPrivate::get(m_printer);

        if (pd->paintEngine->type() != QPaintEngine::Pdf
            && pd->printEngine->printerState() == QPrinter::Active) {
            qWarning("QPrinter::setPageLayout: Cannot be changed while printer is active");
            return false;
        }


        // Try to set the print engine page size
        pd->setProperty(QPrintEngine::PPK_QPageSize, QVariant::fromValue(pageSize));

        return pageLayout().pageSize().isEquivalentTo(pageSize);
    }

    bool setPageOrientation(QPageLayout::Orientation orientation) override
    {
        QPrinterPrivate *pd = QPrinterPrivate::get(m_printer);

        // Set the print engine value
        pd->setProperty(QPrintEngine::PPK_Orientation, orientation);

        return pageLayout().orientation() == orientation;
    }

    bool setPageMargins(const QMarginsF &margins, QPageLayout::Unit units) override
    {
        QPrinterPrivate *pd = QPrinterPrivate::get(m_printer);

        // Try to set print engine margins
        std::pair<QMarginsF, QPageLayout::Unit> pair(margins, units);
        pd->setProperty(QPrintEngine::PPK_QPageMargins, QVariant::fromValue(pair));

        return pageLayout().margins() == margins && pageLayout().units() == units;
    }

    QPageLayout pageLayout() const override
    {
        QPrinterPrivate *pd = QPrinterPrivate::get(m_printer);

        return qvariant_cast<QPageLayout>(pd->printEngine->property(QPrintEngine::PPK_QPageLayout));
    }

    QPrinter *m_printer;
};


/*!
  \class QPrinter
  \reentrant

  \brief The QPrinter class is a paint device that paints on a printer.

  \ingroup printing
  \inmodule QtPrintSupport


  This device represents a series of pages of printed output, and is
  used in almost exactly the same way as other paint devices such as
  QWidget and QPixmap.
  A set of additional functions are provided to manage device-specific
  features, such as orientation and resolution, and to step through
  the pages in a document as it is generated.

  When printing directly to a printer on Windows or \macos, QPrinter uses
  the built-in printer drivers. On X11, QPrinter uses the
  \l{Common Unix Printing System (CUPS)}
  to send PDF output to the printer. As an alternative,
  the printProgram() function can be used to specify the command or utility
  to use instead of the system default.

  Note that setting parameters like paper size and resolution on an
  invalid printer is undefined. You can use QPrinter::isValid() to
  verify this before changing any parameters.

  QPrinter supports a number of parameters, most of which can be
  changed by the end user through a \l{QPrintDialog}{print dialog}. In
  general, QPrinter passes these functions onto the underlying QPrintEngine.

  The most important parameters are:
  \list
  \li setPageLayout() tells QPrinter which page orientation to use, and
  what size to expect from the printer.
  \li setResolution() tells QPrinter what resolution you wish the
  printer to provide, in dots per inch (DPI).
  \li setFullPage() tells QPrinter whether you want to deal with the
  full page or just with the part the printer can draw on.
  \li setCopyCount() tells QPrinter how many copies of the document
  it should print.
  \endlist

  Many of these functions can only be called before the actual printing
  begins (i.e., before QPainter::begin() is called). This usually makes
  sense because, for example, it's not possible to change the number of
  copies when you are halfway through printing. There are also some
  settings that the user sets (through the printer dialog) and that
  applications are expected to obey. See QAbstractPrintDialog's
  documentation for more details.

  When QPainter::begin() is called, the QPrinter it operates on is prepared for
  a new page, enabling the QPainter to be used immediately to paint the first
  page in a document. Once the first page has been painted, newPage() can be
  called to request a new blank page to paint on, or QPainter::end() can be
  called to finish printing. The second page and all following pages are
  prepared using a call to newPage() before they are painted.

  The first page in a document does not need to be preceded by a call to
  newPage(). You only need to calling newPage() after QPainter::begin() if you
  need to insert a blank page at the beginning of a printed document.
  Similarly, calling newPage() after the last page in a document is painted will
  result in a trailing blank page appended to the end of the printed document.

  If you want to abort the print job, abort() will try its best to
  stop printing. It may cancel the entire job or just part of it.

  Since QPrinter can print to any QPrintEngine subclass, it is possible to
  extend printing support to cover new types of printing subsystem by
  subclassing QPrintEngine and reimplementing its interface.

  \sa QPrintDialog, {Qt Print Support}
*/

/*!
    \enum QPrinter::PrinterState

    \value Idle
    \value Active
    \value Aborted
    \value Error
*/

/*!
    \enum QPrinter::PrinterMode

    This enum describes the mode the printer should work in. It
    basically presets a certain resolution and working mode.

    \value ScreenResolution Sets the resolution of the print device to
    the screen resolution. This has the big advantage that the results
    obtained when painting on the printer will match more or less
    exactly the visible output on the screen. It is the easiest to
    use, as font metrics on the screen and on the printer are the
    same. This is the default value. ScreenResolution will produce a
    lower quality output than HighResolution and should only be used
    for drafts.

    \value PrinterResolution This value is deprecated. It is
    equivalent to ScreenResolution on Unix and HighResolution on
    Windows and Mac. Due to the difference between ScreenResolution
    and HighResolution, use of this value may lead to non-portable
    printer code.

    \value HighResolution On Windows, sets the printer resolution to that
    defined for the printer in use. For PDF printing, sets the
    resolution of the PDF driver to 1200 dpi.

    \note When rendering text on a QPrinter device, it is important
    to realize that the size of text, when specified in points, is
    independent of the resolution specified for the device itself.
    Therefore, it may be useful to specify the font size in pixels
    when combining text with graphics to ensure that their relative
    sizes are what you expect.
*/

/*!
    \enum QPrinter::PrintRange

    Used to specify the print range selection option.

    \value AllPages All pages should be printed.
    \value Selection Only the selection should be printed.
    \value PageRange The specified page range should be printed.
    \value CurrentPage Only the current page should be printed.

    \sa setPrintRange(), printRange(), QAbstractPrintDialog::PrintRange
*/

/*!
  \enum QPrinter::PageOrder

  This enum type is used by QPrinter to tell the application program
  how to print.

  \value FirstPageFirst  the lowest-numbered page should be printed
  first.

  \value LastPageFirst  the highest-numbered page should be printed
  first.
*/

/*!
  \enum QPrinter::ColorMode

  This enum type is used to indicate whether QPrinter should print
  in color or not.

  \value Color  print in color if available, otherwise in grayscale.

  \value GrayScale  print in grayscale, even on color printers.
*/

/*!
  \enum QPrinter::PaperSource

  This enum type specifies what paper source QPrinter is to use.
  QPrinter does not check that the paper source is available; it
  just uses this information to try and set the paper source.
  Whether it will set the paper source depends on whether the
  printer has that particular source.

  \warning This is currently only implemented for Windows.

  \value Auto
  \value Cassette
  \value Envelope
  \value EnvelopeManual
  \value FormSource
  \value LargeCapacity
  \value LargeFormat
  \value Lower
  \value MaxPageSource Deprecated, use LastPaperSource instead
  \value Middle
  \value Manual
  \value OnlyOne
  \value Tractor
  \value SmallFormat
  \value Upper
  \value CustomSource A PaperSource defined by the printer that is unknown to Qt
  \value LastPaperSource The highest valid PaperSource value, currently CustomSource
*/

/*!
  \enum QPrinter::Unit
  \since 4.4

  This enum type is used to specify the measurement unit for page and
  paper sizes.

  \value Millimeter
  \value Point
  \value Inch
  \value Pica
  \value Didot
  \value Cicero
  \value DevicePixel

  Note the difference between Point and DevicePixel. The Point unit is
  defined to be 1/72th of an inch, while the DevicePixel unit is
  resolution dependent and is based on the actual pixels, or dots, on
  the printer.
*/

/*!
    Creates a new printer object with the given \a mode.
*/
QPrinter::QPrinter(PrinterMode mode)
    : QPagedPaintDevice(new QPrinterPagedPaintDevicePrivate(this)),
      d_ptr(new QPrinterPrivate(this))
{
    d_ptr->init(QPrinterInfo(), mode);
}

/*!
    \since 4.4

    Creates a new printer object with the given \a printer and \a mode.
*/
QPrinter::QPrinter(const QPrinterInfo& printer, PrinterMode mode)
    : QPagedPaintDevice(new QPrinterPagedPaintDevicePrivate(this)),
      d_ptr(new QPrinterPrivate(this))
{
    d_ptr->init(printer, mode);
}

void QPrinterPrivate::init(const QPrinterInfo &printer, QPrinter::PrinterMode mode)
{
    if (Q_UNLIKELY(!QCoreApplication::instance())) {
        qFatal("QPrinter: Must construct a QCoreApplication before a QPrinter");
        return;
    }

    printerMode = mode;

    initEngines(QPrinter::NativeFormat, printer);
}

/*!
    This function is used by subclasses of QPrinter to specify custom
    print and paint engines (\a printEngine and \a paintEngine,
    respectively).

    QPrinter does not take ownership of the engines, so you need to
    manage these engine instances yourself.

    Note that changing the engines will reset the printer state and
    all its properties.

    \sa printEngine(), paintEngine(), setOutputFormat()

    \since 4.1
*/
void QPrinter::setEngines(QPrintEngine *printEngine, QPaintEngine *paintEngine)
{
    Q_D(QPrinter);

    if (d->use_default_engine)
        delete d->printEngine;

    d->printEngine = printEngine;
    d->paintEngine = paintEngine;
    d->use_default_engine = false;
}

/*!
    Destroys the printer object and frees any allocated resources. If
    the printer is destroyed while a print job is in progress this may
    or may not affect the print job.
*/
QPrinter::~QPrinter()
{
    Q_D(QPrinter);
    if (d->use_default_engine)
        delete d->printEngine;
#if QT_CONFIG(printpreviewwidget)
    delete d->previewEngine;
#endif
}

/*!
    \enum QPrinter::OutputFormat

    The OutputFormat enum is used to describe the format QPrinter should
    use for printing.

    \value NativeFormat QPrinter will print output using a method defined
    by the platform it is running on. This mode is the default when printing
    directly to a printer.

    \value PdfFormat QPrinter will generate its output as a searchable PDF file.
    This mode is the default when printing to a file.

    \sa outputFormat(), setOutputFormat(), setOutputFileName()
*/

/*!
    \since 4.1

    Sets the output format for this printer to \a format.

    If \a format is the same value as currently set then no change will be made.

    If \a format is NativeFormat then the printerName will be set to the default
    printer.  If there are no valid printers configured then no change will be made.
    If you want to set NativeFormat with a specific printerName then use
    setPrinterName().

    \sa setPrinterName()
*/
void QPrinter::setOutputFormat(OutputFormat format)
{
    Q_D(QPrinter);

    if (d->outputFormat == format)
        return;

    if (format == QPrinter::NativeFormat) {
        QPrinterInfo printerToUse = d->findValidPrinter();
        if (!printerToUse.isNull())
            d->changeEngines(format, printerToUse);
    } else {
        d->changeEngines(format, QPrinterInfo());
    }
}

/*!
    \since 4.1

    Returns the output format for this printer.
*/
QPrinter::OutputFormat QPrinter::outputFormat() const
{
    Q_D(const QPrinter);
    return d->outputFormat;
}

/*!
    \since 5.10

    Sets the PDF version for this printer to \a version.

    If \a version is the same value as currently set then no change will be made.
*/
void QPrinter::setPdfVersion(PdfVersion version)
{
    Q_D(QPrinter);

    if (d->pdfVersion == version)
        return;

    d->pdfVersion = version;

    if (d->outputFormat == QPrinter::PdfFormat) {
        d->changeEngines(d->outputFormat, QPrinterInfo());
    }
}

/*!
    \since 5.10

    Returns the PDF version for this printer. The default is \c PdfVersion_1_4.
*/
QPrinter::PdfVersion QPrinter::pdfVersion() const
{
    Q_D(const QPrinter);
    return d->pdfVersion;
}

/*! \internal
*/
int QPrinter::devType() const
{
    return QInternal::Printer;
}

/*!
    Returns the printer name. This value is initially set to the name
    of the default printer.

    \sa setPrinterName()
*/
QString QPrinter::printerName() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_PrinterName).toString();
}

/*!
    Sets the printer name to \a name.

    If the \a name is empty then the output format will be set to PdfFormat.

    If the \a name is not a valid printer then no change will be made.

    If the \a name is a valid printer then the output format will be set to NativeFormat.

    \sa printerName(), isValid(), setOutputFormat()
*/
void QPrinter::setPrinterName(const QString &name)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setPrinterName");

    if (printerName() == name)
        return;

    if (name.isEmpty()) {
        setOutputFormat(QPrinter::PdfFormat);
        return;
    }

    QPrinterInfo printerToUse = QPrinterInfo::printerInfo(name);
    if (printerToUse.isNull())
        return;

    if (outputFormat() == QPrinter::PdfFormat) {
        d->changeEngines(QPrinter::NativeFormat, printerToUse);
    } else {
        d->setProperty(QPrintEngine::PPK_PrinterName, name);
    }
}

/*!
  \since 4.4

  Returns \c true if the printer currently selected is a valid printer
  in the system, or a pure PDF printer; otherwise returns \c false.

  To detect other failures check the output of QPainter::begin() or QPrinter::newPage().

  \snippet printing-qprinter/errors.cpp 0

  \sa setPrinterName()
*/
bool QPrinter::isValid() const
{
    Q_D(const QPrinter);
    if (!qApp)
        return false;
    return d->validPrinter;
}

/*!
  \fn QString QPrinter::outputFileName() const

  Returns the name of the output file. By default, this is an empty string
  (indicating that the printer shouldn't print to file).

  \sa QPrintEngine::PrintEnginePropertyKey

*/

QString QPrinter::outputFileName() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_OutputFileName).toString();
}

/*!
    Sets the name of the output file to \a fileName.

    Setting a null or empty name (0 or "") disables printing to a file.
    Setting a non-empty name enables printing to a file.

    This can change the value of outputFormat().
    If the file name has the ".pdf" suffix PDF is generated. If the file name
    has a suffix other than ".pdf", the output format used is the
    one set with setOutputFormat().

    QPrinter uses Qt's cross-platform PDF print engines
    respectively. If you can produce this format natively, for example
    \macos can generate PDF's from its print engine, set the output format
    back to NativeFormat.

    \sa outputFileName(), setOutputFormat()
*/

void QPrinter::setOutputFileName(const QString &fileName)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setOutputFileName");

    QFileInfo fi(fileName);
    if (!fi.suffix().compare("pdf"_L1, Qt::CaseInsensitive))
        setOutputFormat(QPrinter::PdfFormat);
    else if (fileName.isEmpty())
        setOutputFormat(QPrinter::NativeFormat);

    d->setProperty(QPrintEngine::PPK_OutputFileName, fileName);
}


/*!
  Returns the name of the program that sends the print output to the
  printer.

  The default is to return an empty string; meaning that QPrinter will try to
  be smart in a system-dependent way. On X11 only, you can set it to something
  different to use a specific print program. On the other platforms, this
  returns an empty string.

  \sa setPrintProgram(), setPrinterSelectionOption()
*/
QString QPrinter::printProgram() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_PrinterProgram).toString();
}


/*!
  Sets the name of the program that should do the print job to \a
  printProg.

  On X11, this function sets the program to call with the PDF
  output. On other platforms, it has no effect.

  \sa printProgram()
*/
void QPrinter::setPrintProgram(const QString &printProg)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setPrintProgram");
    d->setProperty(QPrintEngine::PPK_PrinterProgram, printProg);
}


/*!
  Returns the document name.

  \sa setDocName(), QPrintEngine::PrintEnginePropertyKey
*/
QString QPrinter::docName() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_DocumentName).toString();
}


/*!
  Sets the document name to \a name.

  On X11, the document name is for example used as the default
  output filename in QPrintDialog. Note that the document name does
  not affect the file name if the printer is printing to a file.
  Use the setOutputFile() function for this.

  \sa docName(), QPrintEngine::PrintEnginePropertyKey
*/
void QPrinter::setDocName(const QString &name)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setDocName");
    d->setProperty(QPrintEngine::PPK_DocumentName, name);
}


/*!
  Returns the name of the application that created the document.

  \sa setCreator()
*/
QString QPrinter::creator() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_Creator).toString();
}


/*!
  Sets the name of the application that created the document to \a
  creator.

  This function is only applicable to the X11 version of Qt. If no
  creator name is specified, the creator will be set to "Qt"
  followed by some version number.

  \sa creator()
*/
void QPrinter::setCreator(const QString &creator)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setCreator");
    d->setProperty(QPrintEngine::PPK_Creator, creator);
}

/*!
    Sets the page order to \a pageOrder.

    The page order can be QPrinter::FirstPageFirst or
    QPrinter::LastPageFirst. The application is responsible for
    reading the page order and printing accordingly.

    This function is mostly useful for setting a default value that
    the user can override in the print dialog.

    This function is only supported under X11.
*/

void QPrinter::setPageOrder(PageOrder pageOrder)
{
    d->pageOrderAscending = (pageOrder == FirstPageFirst);

    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setPageOrder");
    d->setProperty(QPrintEngine::PPK_PageOrder, pageOrder);
}


/*!
  Returns the current page order.

  The default page order is \c FirstPageFirst.
*/

QPrinter::PageOrder QPrinter::pageOrder() const
{
    Q_D(const QPrinter);
    return QPrinter::PageOrder(d->printEngine->property(QPrintEngine::PPK_PageOrder).toInt());
}


/*!
  Sets the printer's color mode to \a newColorMode, which can be
  either \c Color or \c GrayScale.

  \sa colorMode()
*/

void QPrinter::setColorMode(ColorMode newColorMode)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setColorMode");
    d->setProperty(QPrintEngine::PPK_ColorMode, newColorMode);
}


/*!
  Returns the current color mode.

  \sa setColorMode()
*/
QPrinter::ColorMode QPrinter::colorMode() const
{
    Q_D(const QPrinter);
    return QPrinter::ColorMode(d->printEngine->property(QPrintEngine::PPK_ColorMode).toInt());
}

/*!
    \since 4.7

    Sets the number of copies to be printed to \a count.

    The printer driver reads this setting and prints the specified number of
    copies.

    \sa copyCount(), supportsMultipleCopies()
*/

void QPrinter::setCopyCount(int count)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setCopyCount;");
    d->setProperty(QPrintEngine::PPK_CopyCount, count);
}

/*!
    \since 4.7

    Returns the number of copies that will be printed. The default value is 1.

    \sa setCopyCount(), supportsMultipleCopies()
*/

int QPrinter::copyCount() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_CopyCount).toInt();
}

/*!
    \since 4.7

    Returns \c true if the printer supports printing multiple copies of the same
    document in one job; otherwise false is returned.

    On most systems this function will return true. However, on X11 systems
    that do not support CUPS, this function will return false. That means the
    application has to handle the number of copies by printing the same
    document the required number of times.

    \sa setCopyCount(), copyCount()
*/

bool QPrinter::supportsMultipleCopies() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_SupportsMultipleCopies).toBool();
}

/*!
    \since 4.1

    Returns \c true if collation is turned on when multiple copies is selected.
    Returns \c false if it is turned off when multiple copies is selected.
    When collating is turned off the printing of each individual page will be repeated
    the numCopies() amount before the next page is started. With collating turned on
    all pages are printed before the next copy of those pages is started.

    \sa setCollateCopies()
*/
bool QPrinter::collateCopies() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_CollateCopies).toBool();
}


/*!
    \since 4.1

    Sets the default value for collation checkbox when the print
    dialog appears.  If \a collate is true, it will enable
    setCollateCopiesEnabled().  The default value is false. This value
    will be changed by what the user presses in the print dialog.

    \sa collateCopies()
*/
void QPrinter::setCollateCopies(bool collate)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setCollateCopies");
    d->setProperty(QPrintEngine::PPK_CollateCopies, collate);
}



/*!
  If \a fp is true, enables support for painting over the entire page;
  otherwise restricts painting to the printable area reported by the
  device.

  By default, full page printing is disabled. In this case, the origin
  of the QPrinter's coordinate system coincides with the top-left
  corner of the printable area.

  If full page printing is enabled, the origin of the QPrinter's
  coordinate system coincides with the top-left corner of the paper
  itself. In this case, the
  \l{QPaintDevice::PaintDeviceMetric}{device metrics} will report
  the exact same dimensions as indicated by \{QPageSize}. It may not
  be possible to print on the entire physical page because of the
  printer's margins, so the application must account for the margins
  itself.

  \sa fullPage(), QPagedPaintDevice::pageLayout(), QPagedPaintDevice::setPageSize()
*/

void QPrinter::setFullPage(bool fp)
{
    Q_D(QPrinter);
    // Set the print engine
    d->setProperty(QPrintEngine::PPK_FullPage, fp);
}


/*!
  Returns \c true if the origin of the printer's coordinate system is
  at the corner of the page and false if it is at the edge of the
  printable area.

  See setFullPage() for details and caveats.

  \sa setFullPage(), QPagedPaintDevice::pageLayout()
*/

bool QPrinter::fullPage() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_FullPage).toBool();
}


/*!
  Requests that the printer prints at \a dpi or as near to \a dpi as
  possible.

  This setting affects the coordinate system as returned by, for
  example QPainter::viewport().

  This function must be called before QPainter::begin() to have an effect on
  all platforms.

  \sa resolution(), QPagedPaintDevice::setPageSize()
*/

void QPrinter::setResolution(int dpi)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setResolution");
    d->setProperty(QPrintEngine::PPK_Resolution, dpi);
}


/*!
  Returns the current assumed resolution of the printer, as set by
  setResolution() or by the printer driver.

  \sa setResolution()
*/

int QPrinter::resolution() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_Resolution).toInt();
}

/*!
  Sets the paper source setting to \a source.

  Windows only: This option can be changed while printing and will
  take effect from the next call to newPage()

  \sa paperSource()
*/

void QPrinter::setPaperSource(PaperSource source)
{
    Q_D(QPrinter);
    d->setProperty(QPrintEngine::PPK_PaperSource, source);
}

/*!
    Returns the printer's paper source. This is \c Manual or a printer
    tray or paper cassette.
*/
QPrinter::PaperSource QPrinter::paperSource() const
{
    Q_D(const QPrinter);
    return QPrinter::PaperSource(d->printEngine->property(QPrintEngine::PPK_PaperSource).toInt());
}


/*!
  \since 4.1

  Enabled or disables font embedding depending on \a enable.

  \sa fontEmbeddingEnabled()
*/
void QPrinter::setFontEmbeddingEnabled(bool enable)
{
    Q_D(QPrinter);
    d->setProperty(QPrintEngine::PPK_FontEmbedding, enable);
}

/*!
  \since 4.1

  Returns \c true if font embedding is enabled.

  \sa setFontEmbeddingEnabled()
*/
bool QPrinter::fontEmbeddingEnabled() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_FontEmbedding).toBool();
}

/*!
    \enum QPrinter::DuplexMode
    \since 4.4

    This enum is used to indicate whether printing will occur on one or both sides
    of each sheet of paper (simplex or duplex printing).

    \value DuplexNone       Single sided (simplex) printing only.
    \value DuplexAuto       The printer's default setting is used to determine whether
                            duplex printing is used.
    \value DuplexLongSide   Both sides of each sheet of paper are used for printing.
                            The paper is turned over its longest edge before the second
                            side is printed
    \value DuplexShortSide  Both sides of each sheet of paper are used for printing.
                            The paper is turned over its shortest edge before the second
                            side is printed
*/

/*!
  \since 4.4

  Enables double sided printing based on the \a duplex mode.

  \sa duplex()
*/
void QPrinter::setDuplex(DuplexMode duplex)
{
    Q_D(QPrinter);
    d->setProperty(QPrintEngine::PPK_Duplex, duplex);
}

/*!
  \since 4.4

  Returns the current duplex mode.

  \sa setDuplex()
*/
QPrinter::DuplexMode QPrinter::duplex() const
{
    Q_D(const QPrinter);
    return static_cast <DuplexMode> (d->printEngine->property(QPrintEngine::PPK_Duplex).toInt());
}

/*!
    \since 4.4

    Returns the page's rectangle in \a unit; this is usually smaller
    than the paperRect() since the page normally has margins between
    its borders and the paper.

    \sa QPagedPaintDevice::pageLayout()
*/
QRectF QPrinter::pageRect(Unit unit) const
{
    if (unit == QPrinter::DevicePixel)
        return pageLayout().paintRectPixels(resolution());
    else
        return pageLayout().paintRect(QPageLayout::Unit(unit));
}


/*!
    \since 4.4

    Returns the paper's rectangle in \a unit; this is usually larger
    than the pageRect().

   \sa pageRect()
*/
QRectF QPrinter::paperRect(Unit unit) const
{
    if (unit == QPrinter::DevicePixel)
        return pageLayout().fullRectPixels(resolution());
    else
        return pageLayout().fullRect(QPageLayout::Unit(unit));
}

/*!
    \internal

    Returns the metric for the given \a id.
*/
int QPrinter::metric(PaintDeviceMetric id) const
{
    Q_D(const QPrinter);
    return d->printEngine->metric(id);
}

/*!
    Returns the paint engine used by the printer.
*/
QPaintEngine *QPrinter::paintEngine() const
{
    Q_D(const QPrinter);
    return d->paintEngine;
}

/*!
    \since 4.1

    Returns the print engine used by the printer.
*/
QPrintEngine *QPrinter::printEngine() const
{
    Q_D(const QPrinter);
    return d->printEngine;
}

/*!
    Returns a list of the resolutions (a list of dots-per-inch
    integers) that the printer says it supports.

    For X11 where all printing is directly to PDF, this
    function will always return a one item list containing only the
    PDF resolution, i.e., 72 (72 dpi -- but see PrinterMode).
*/
QList<int> QPrinter::supportedResolutions() const
{
    Q_D(const QPrinter);
    const QList<QVariant> varlist
        = d->printEngine->property(QPrintEngine::PPK_SupportedResolutions).toList();
    QList<int> intlist;
    intlist.reserve(varlist.size());
    for (const auto &var : varlist)
        intlist << var.toInt();
    return intlist;
}

/*!
    Tells the printer to eject the current page and to continue
    printing on a new page. Returns \c true if this was successful;
    otherwise returns \c false.

    Calling newPage() on an inactive QPrinter object will always
    fail.
*/
bool QPrinter::newPage()
{
    Q_D(QPrinter);
    if (d->printEngine->printerState() != QPrinter::Active)
        return false;
    return d->printEngine->newPage();
}

/*!
    Aborts the current print run. Returns \c true if the print run was
    successfully aborted and printerState() will return QPrinter::Aborted; otherwise
    returns \c false.

    It is not always possible to abort a print job. For example,
    all the data has gone to the printer but the printer cannot or
    will not cancel the job when asked to.
*/
bool QPrinter::abort()
{
    Q_D(QPrinter);
    return d->printEngine->abort();
}

/*!
    Returns the current state of the printer. This may not always be
    accurate (for example if the printer doesn't have the capability
    of reporting its state to the operating system).
*/
QPrinter::PrinterState QPrinter::printerState() const
{
    Q_D(const QPrinter);
    return d->printEngine->printerState();
}

#if defined(Q_OS_WIN) || defined(Q_QDOC)
/*!
    Returns the supported paper sizes for this printer.

    The values will be either a value that matches an entry in the
    QPrinter::PaperSource enum or a driver spesific value. The driver
    spesific values are greater than the constant DMBIN_USER declared
    in wingdi.h.

    \warning This function is only available in windows.
*/

QList<QPrinter::PaperSource> QPrinter::supportedPaperSources() const
{
    Q_D(const QPrinter);
    QVariant v = d->printEngine->property(QPrintEngine::PPK_PaperSources);

    const QList<QVariant> variant_list = v.toList();
    QList<QPrinter::PaperSource> int_list;
    int_list.reserve(variant_list.size());
    for (const auto &variant : variant_list)
        int_list << QPrinter::PaperSource(variant.toInt());

    return int_list;
}

#endif // Q_OS_WIN

/*!
    \fn QString QPrinter::printerSelectionOption() const

    Returns the printer options selection string. This is useful only
    if the print command has been explicitly set.

    The default value (an empty string) implies that the printer should
    be selected in a system-dependent manner.

    Any other value implies that the given value should be used.

    This function always returns an empty string on Windows and Mac.

    \sa setPrinterSelectionOption(), setPrintProgram()
*/

QString QPrinter::printerSelectionOption() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_SelectionOption).toString();
}

/*!
    \fn void QPrinter::setPrinterSelectionOption(const QString &option)

    Sets the printer to use \a option to select the printer. \a option
    is null by default (which implies that Qt should be smart enough
    to guess correctly), but it can be set to other values to use a
    specific printer selection option.

    If the printer selection option is changed while the printer is
    active, the current print job may or may not be affected.

    This function has no effect on Windows or Mac.

    \sa printerSelectionOption(), setPrintProgram()
*/

void QPrinter::setPrinterSelectionOption(const QString &option)
{
    Q_D(QPrinter);
    d->setProperty(QPrintEngine::PPK_SelectionOption, option);
}

/*!
    \since 4.1
    \fn int QPrinter::fromPage() const

    Returns the number of the first page in a range of pages to be printed
    (the "from page" setting). Pages in a document are numbered according to
    the convention that the first page is page 1.

    By default, this function returns a special value of 0, meaning that
    the "from page" setting is unset.

    \note If fromPage() and toPage() both return 0, this indicates that
    \e{the whole document will be printed}.

    \sa setFromTo(), toPage(), pageRanges()
*/

int QPrinter::fromPage() const
{
    return d->pageRanges.firstPage();
}

/*!
    \since 4.1

    Returns the number of the last page in a range of pages to be printed
    (the "to page" setting). Pages in a document are numbered according to
    the convention that the first page is page 1.

    By default, this function returns a special value of 0, meaning that
    the "to page" setting is unset.

    \note If fromPage() and toPage() both return 0, this indicates that
    \e{the whole document will be printed}.

    The programmer is responsible for reading this setting and
    printing accordingly.

    \sa setFromTo(), fromPage(), pageRanges()
*/

int QPrinter::toPage() const
{
    return d->pageRanges.lastPage();
}

/*!
    \since 4.1

    Sets the range of pages to be printed to cover the pages with numbers
    specified by \a from and \a to, where \a from corresponds to the first
    page in the range and \a to corresponds to the last.

    \note Pages in a document are numbered according to the convention that
    the first page is page 1. However, if \a from and \a to are both set to 0,
    the \e{whole document will be printed}.

    This function is mostly used to set a default value that the user can
    override in the print dialog when you call setup().

    \sa fromPage(), toPage(), pageRanges()
*/

void QPrinter::setFromTo(int from, int to)
{
    d->pageRanges.clear();
    if (from && to)
        d->pageRanges.addRange(from, to);
}

/*!
    \since 4.1

    Sets the print range option in to be \a range.
*/
void QPrinter::setPrintRange( PrintRange range )
{
    d->printSelectionOnly = (range == Selection);

    Q_D(QPrinter);
    d->printRange = range;
}

/*!
    \since 4.1

    Returns the page range of the QPrinter. After the print setup
    dialog has been opened, this function returns the value selected
    by the user.

    \sa setPrintRange()
*/
QPrinter::PrintRange QPrinter::printRange() const
{
    Q_D(const QPrinter);
    return d->printRange;
}


/*!
    \class QPrintEngine
    \reentrant

    \ingroup printing
    \inmodule QtPrintSupport

    \brief The QPrintEngine class defines an interface for how QPrinter
    interacts with a given printing subsystem.

    The common case when creating your own print engine is to derive from both
    QPaintEngine and QPrintEngine. Various properties of a print engine are
    given with property() and set with setProperty().

    \sa QPaintEngine
*/

/*!
    \enum QPrintEngine::PrintEnginePropertyKey

    This enum is used to communicate properties between the print
    engine and QPrinter. A property may or may not be supported by a
    given print engine.

    \value PPK_CollateCopies A boolean value indicating whether the
    printout should be collated or not.

    \value PPK_ColorMode Refers to QPrinter::ColorMode, either color or
    monochrome.

    \value PPK_Creator A string describing the document's creator.

    \value PPK_Duplex A boolean value indicating whether both sides of
    the printer paper should be used for the printout.

    \value PPK_DocumentName A string describing the document name in
    the spooler.

    \value PPK_FontEmbedding A boolean value indicating whether data for
    the document's fonts should be embedded in the data sent to the
    printer.

    \value PPK_FullPage A boolean describing if the printer should be
    full page or not.

    \value PPK_NumberOfCopies Obsolete. An integer specifying the number of
    copies. Use PPK_CopyCount instead.

    \value PPK_Orientation Specifies a QPageLayout::Orientation value.

    \value PPK_OutputFileName The output file name as a string. An
    empty file name indicates that the printer should not print to a file.

    \value PPK_PageOrder Specifies a QPrinter::PageOrder value.

    \value PPK_PageRect A QRect specifying the page rectangle

    \value PPK_PageSize Obsolete. Use PPK_PaperSize instead.

    \value PPK_PaperRect A QRect specifying the paper rectangle.

    \value PPK_PaperSource Specifies a QPrinter::PaperSource value.

    \value PPK_PaperSources Specifies more than one QPrinter::PaperSource value.

    \value PPK_PaperName A string specifying the name of the paper.

    \value PPK_PaperSize Specifies a QPrinter::PaperSize value.

    \value PPK_PrinterName A string specifying the name of the printer.

    \value PPK_PrinterProgram A string specifying the name of the
    printer program used for printing,

    \value PPK_Resolution An integer describing the dots per inch for
    this printer.

    \value PPK_SelectionOption

    \value PPK_SupportedResolutions A list of integer QVariants
    describing the set of supported resolutions that the printer has.

    \value PPK_WindowsPageSize An integer specifying a DM_PAPER entry
    on Windows.

    \value PPK_CustomPaperSize A QSizeF specifying a custom paper size
    in the QPrinter::Point unit.

    \value PPK_PageMargins A QList<QVariant> containing the left, top,
    right and bottom margin values in the QPrinter::Point unit.

    \value PPK_CopyCount An integer specifying the number of copies to print.

    \value PPK_SupportsMultipleCopies A boolean value indicating whether or not
    the printer supports printing multiple copies in one job.

    \value PPK_QPageSize Set the page size using a QPageSize object.

    \value PPK_QPageMargins Set the page margins using a std::pair of QMarginsF and QPageLayout::Unit.

    \value PPK_QPageLayout Set the page layout using a QPageLayout object.

    \value PPK_CustomBase Basis for extension.
*/

/*!
    \fn QPrintEngine::~QPrintEngine()

    Destroys the print engine.
*/

/*!
    \fn void QPrintEngine::setProperty(PrintEnginePropertyKey key, const QVariant &value)

    Sets the print engine's property specified by \a key to the given \a value.

    \sa property()
*/

/*!
    \fn void QPrintEngine::property(PrintEnginePropertyKey key) const

    Returns the print engine's property specified by \a key.

    \sa setProperty()
*/

/*!
    \fn bool QPrintEngine::newPage()

    Instructs the print engine to start a new page. Returns \c true if
    the printer was able to create the new page; otherwise returns \c false.
*/

/*!
    \fn bool QPrintEngine::abort()

    Instructs the print engine to abort the printing process. Returns
    true if successful; otherwise returns \c false.
*/

/*!
    \fn int QPrintEngine::metric(QPaintDevice::PaintDeviceMetric id) const

    Returns the metric for the given \a id.
*/

/*!
    \fn QPrinter::PrinterState QPrintEngine::printerState() const

    Returns the current state of the printer being used by the print engine.
*/

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
