// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <qprinter.h>
#include <qpagesetupdialog.h>
#include <qpainter.h>
#include <qprintdialog.h>
#include <qprintpreviewdialog.h>
#include <qprintpreviewwidget.h>
#include <qprinterinfo.h>
#include <qvariant.h>
#include <qpainter.h>
#include <qprintengine.h>
#include <qpagelayout.h>
#include <qsharedpointer.h>
#include <qtemporarydir.h>

#include <math.h>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

#if QT_CONFIG(printer)
typedef QSharedPointer<QPrinter> PrinterPtr;

Q_DECLARE_METATYPE(PrinterPtr)
#endif // printer

static int fileNumber = 0;

class tst_QPrinter : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
#if QT_CONFIG(printer)
    void testPageRectAndPaperRect();
    void testPageRectAndPaperRect_data();
    void testMargins_data();
    void testMargins();
    void testPageSetupDialog();
    void testPrintPreviewDialog();
    void testMultipleSets_data();
    void testMultipleSets();
    void testPageMargins_data();
    void testPageMargins();
    void outputFormatFromSuffix();
    void errorReporting();
    void testCustomPageSizes();
    void customPaperSizeAndMargins_data();
    void customPaperSizeAndMargins();
    void customPaperNameSettingBySize();
#if QT_CONFIG(completer) && QT_CONFIG(filedialog)
    void printDialogCompleter();
#endif
    void testCurrentPage();
    void taskQTBUG4497_reusePrinterOnDifferentFiles();
    void testPdfTitle();

    // Test QPrintEngine keys and their QPrinter setters/getters
    void testMultipleKeys();
    void collateCopies();
    void colorMode();
    void copyCount();
    void creator();
    void docName();
    void duplex();
    void fontEmbedding();
    void fullPage();
    void orientation();
    void outputFileName();
    void pageOrder();
    void pageSize();
    void paperSource();
    void printerName();
    void printerSelectionOption();
    void printProgram();
    void printRange();
    void resolution();
    void supportedPaperSources();
    void supportedResolutions();

    // Test QPrinter setters/getters for non-QPrintEngine options
    void outputFormat();
    void fromToPage();

    void testPageMetrics_data();
    void testPageMetrics();
    void reusePageMetrics();
#endif
private:
    QString testFileName(const QString &prefix, const QString &suffix);
    QString testPdfFileName(const QString &prefix) { return testFileName(prefix, QStringLiteral("pdf")); }

    QTemporaryDir m_tempDir;
};

void tst_QPrinter::initTestCase()
{
#if !QT_CONFIG(printer)
    QSKIP("This test requires printing support");
#endif
    QVERIFY2(m_tempDir.isValid(), qPrintable(m_tempDir.errorString()));
}

#if QT_CONFIG(printer)

void tst_QPrinter::testPageSetupDialog()
{
    // Make sure this doesn't crash at least
    {
        QPrinter printer;
        QPageSetupDialog dialog(&printer);
    }
}

// A preview dialog showing 4 pages for testPrintPreviewDialog().

class MyPreviewDialog : public QPrintPreviewDialog {
    Q_OBJECT
public:
    MyPreviewDialog(QPrinter *p) : QPrintPreviewDialog(p)
    {
        connect(this, SIGNAL(paintRequested(QPrinter*)), this, SLOT(slotPaintRequested(QPrinter*)));
    }

public slots:
    void slotPaintRequested(QPrinter *p);
};

void MyPreviewDialog::slotPaintRequested(QPrinter *p)
{
    enum { pageCount = 4 };
    QPainter painter;
    painter.begin(p);
    for (int i = 0; i < pageCount; ++i) {
        const QRect f = p->pageRect(QPrinter::DevicePixel).toRect();
        painter.fillRect(f, Qt::white);
        painter.drawText(f.center(), QString::fromLatin1("Page %1").arg(i + 1));
        if (i != pageCount - 1)
            p->newPage();
    }
    painter.end();
}

void tst_QPrinter::testPrintPreviewDialog()
{
    // QTBUG-14517: Showing the dialog with Qt::WindowMaximized caused it to switch to
    // page 2 due to the scrollbar logic (besides testing for crashes).
    QPrinter printer;
    MyPreviewDialog dialog(&printer);
    dialog.setWindowState(Qt::WindowMaximized);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QPrintPreviewWidget *widget = dialog.findChild<QPrintPreviewWidget *>();
    QVERIFY(widget);
    QCOMPARE(widget->currentPage(), 1);
}

void tst_QPrinter::testPageRectAndPaperRect_data()
{
    QTest::addColumn<PrinterPtr>("printer");
    QTest::addColumn<QPageLayout::Orientation>("orientation");
    QTest::addColumn<bool>("withPainter");
    QTest::addColumn<int>("resolution");
    QTest::addColumn<bool>("doPaperRect");

    const PrinterPtr printer(new QPrinter(QPrinter::HighResolution));
    // paperrect
    QTest::newRow("paperRect0") << printer << QPageLayout::Portrait << true << 300 << true;
    QTest::newRow("paperRect1") << printer << QPageLayout::Portrait << false << 300 << true;
    QTest::newRow("paperRect2") << printer << QPageLayout::Landscape << true << 300 << true;
    QTest::newRow("paperRect3") << printer << QPageLayout::Landscape << false << 300 << true;
    QTest::newRow("paperRect4") << printer << QPageLayout::Portrait << true << 600 << true;
    QTest::newRow("paperRect5") << printer << QPageLayout::Portrait << false << 600 << true;
    QTest::newRow("paperRect6") << printer << QPageLayout::Landscape << true << 600 << true;
    QTest::newRow("paperRect7") << printer << QPageLayout::Landscape << false << 600 << true;
    QTest::newRow("paperRect8") << printer << QPageLayout::Portrait << true << 1200 << true;
    QTest::newRow("paperRect9") << printer << QPageLayout::Portrait << false << 1200 << true;
    QTest::newRow("paperRect10") << printer << QPageLayout::Landscape << true << 1200 << true;
    QTest::newRow("paperRect11") << printer << QPageLayout::Landscape << false << 1200 << true;

    // page rect
    QTest::newRow("pageRect0") << printer << QPageLayout::Portrait << true << 300 << false;
    QTest::newRow("pageRect1") << printer << QPageLayout::Portrait << false << 300 << false;
    QTest::newRow("pageRect2") << printer << QPageLayout::Landscape << true << 300 << false;
    QTest::newRow("pageRect3") << printer << QPageLayout::Landscape << false << 300 << false;
    QTest::newRow("pageRect4") << printer << QPageLayout::Portrait << true << 600 << false;
    QTest::newRow("pageRect5") << printer << QPageLayout::Portrait << false << 600 << false;
    QTest::newRow("pageRect6") << printer << QPageLayout::Landscape << true << 600 << false;
    QTest::newRow("pageRect7") << printer << QPageLayout::Landscape << false << 600 << false;
    QTest::newRow("pageRect8") << printer << QPageLayout::Portrait << true << 1200 << false;
    QTest::newRow("pageRect9") << printer << QPageLayout::Portrait << false << 1200 << false;
    QTest::newRow("pageRect10") << printer << QPageLayout::Landscape << true << 1200 << false;
    QTest::newRow("pageRect11") << printer << QPageLayout::Landscape << false << 1200 << false;
}

void tst_QPrinter::testPageRectAndPaperRect()
{
    QFETCH(PrinterPtr, printer);
    QFETCH(bool,  withPainter);
    QFETCH(QPageLayout::Orientation, orientation);
    QFETCH(int, resolution);
    QFETCH(bool, doPaperRect);

    QPainter *painter = nullptr;
    printer->setPageOrientation(orientation);
    printer->setOutputFileName(testFileName(QLatin1String("silly"), QString()));

    QRect pageRect = (doPaperRect ? printer->paperRect(QPrinter::DevicePixel) : printer->pageRect(QPrinter::DevicePixel)).toRect();
    float inchesX = float(pageRect.width()) / float(printer->resolution());
    float inchesY = float(pageRect.height()) / float(printer->resolution());
    printer->setResolution(resolution);
    if (withPainter)
        painter = new QPainter(printer.data());

    QRect otherRect = (doPaperRect ? printer->paperRect(QPrinter::DevicePixel) : printer->pageRect(QPrinter::DevicePixel)).toRect();
    float otherInchesX = float(otherRect.width()) / float(printer->resolution());
    float otherInchesY = float(otherRect.height()) / float(printer->resolution());
    if (painter != nullptr)
        delete painter;

    QVERIFY(qAbs(otherInchesX - inchesX) < 0.01);
    QVERIFY(qAbs(otherInchesY - inchesY) < 0.01);

    QVERIFY(printer->pageLayout().orientation() == QPageLayout::Portrait || pageRect.width() > pageRect.height());
    QVERIFY(printer->pageLayout().orientation() != QPageLayout::Portrait || pageRect.width() < pageRect.height());
}

void tst_QPrinter::testMargins_data()
{
    QTest::addColumn<PrinterPtr>("printer");
    QTest::addColumn<QPageLayout::Orientation>("orientation");
    QTest::addColumn<bool>("fullpage");
    QTest::addColumn<QPageSize::PageSizeId>("pagesize");
    QTest::addColumn<bool>("withPainter");

    const PrinterPtr printer(new QPrinter);
    QTest::newRow("data0") << printer << QPageLayout::Portrait << true << QPageSize::A4 << false;
    QTest::newRow("data1") << printer << QPageLayout::Landscape << true << QPageSize::A4 << false;
    QTest::newRow("data2") << printer << QPageLayout::Landscape << false << QPageSize::A4 << false;
    QTest::newRow("data3") << printer << QPageLayout::Portrait << false << QPageSize::A4 << false;
    QTest::newRow("data4") << printer << QPageLayout::Portrait << true << QPageSize::A4 << true;
    QTest::newRow("data5") << printer << QPageLayout::Landscape << true << QPageSize::A4 << true;
    QTest::newRow("data6") << printer << QPageLayout::Landscape << false << QPageSize::A4 << true;
    QTest::newRow("data7") << printer << QPageLayout::Portrait << false << QPageSize::A4 << true;
}

void tst_QPrinter::testMargins()
{
    QFETCH(PrinterPtr, printer);
    QFETCH(bool,  withPainter);
    QFETCH(QPageLayout::Orientation, orientation);
    QFETCH(QPageSize::PageSizeId, pagesize);
    QFETCH(bool, fullpage);
    QPainter *painter = nullptr;
    printer->setOutputFileName(testFileName(QLatin1String("silly"), QString()));
    printer->setPageOrientation(orientation);
    printer->setFullPage(fullpage);
    printer->setPageSize(QPageSize(pagesize));
    if (withPainter)
        painter = new QPainter(printer.data());

    if (painter)
        delete painter;
}

void tst_QPrinter::testMultipleSets_data()
{
    QTest::addColumn<int>("resolution");
    QTest::addColumn<QPageSize::PageSizeId>("pageSize");
    QTest::addColumn<int>("widthMMAfter");
    QTest::addColumn<int>("heightMMAfter");


    QTest::newRow("lowRes") << int(QPrinter::ScreenResolution) << QPageSize::A4 << 210 << 297;
    QTest::newRow("lowResLetter") << int(QPrinter::ScreenResolution) << QPageSize::Letter << 216 << 279;
    QTest::newRow("lowResA5") << int(QPrinter::ScreenResolution) << QPageSize::A5 << 148 << 210;
    QTest::newRow("midRes") << int(QPrinter::PrinterResolution) << QPageSize::A4 << 210 << 297;
    QTest::newRow("midResLetter") << int(QPrinter::PrinterResolution) << QPageSize::Letter << 216 << 279;
    QTest::newRow("midResA5") << int(QPrinter::PrinterResolution) << QPageSize::A5 << 148 << 210;
    QTest::newRow("highRes") << int(QPrinter::HighResolution) << QPageSize::A4 << 210 << 297;
    QTest::newRow("highResLetter") << int(QPrinter::HighResolution) << QPageSize::Letter << 216 << 279;
    QTest::newRow("highResA5") << int(QPrinter::HighResolution) << QPageSize::A5 << 148 << 210;
}

void tst_QPrinter::testMultipleSets()
{
    // A very simple test, but Mac needs to have its format "validated" if the format is changed
    // This takes care of that.
    QFETCH(int, resolution);
    QFETCH(QPageSize::PageSizeId, pageSize);
    QFETCH(int, widthMMAfter);
    QFETCH(int, heightMMAfter);


    QPrinter::PrinterMode mode = QPrinter::PrinterMode(resolution);
    QPrinter printer(mode);
    printer.setFullPage(true);

    int paperWidth, paperHeight;
    //const int Tolerance = 2;

    const auto computePageValue = [&printer](int &retWidth, int &retHeight)
    {
        const double Inch2MM = 25.4;
        double width = double(printer.paperRect(QPrinter::DevicePixel).width()) / printer.logicalDpiX() * Inch2MM;
        double height = double(printer.paperRect(QPrinter::DevicePixel).height()) / printer.logicalDpiY() * Inch2MM;
        retWidth = qRound(width);
        retHeight = qRound(height);
    };

    computePageValue(paperWidth, paperHeight);
    printer.setPageSize(QPageSize(pageSize));

    if (printer.pageLayout().pageSize().id() != pageSize) {
        QSKIP("Current page size is not supported on this printer");
        return;
    }

    QVERIFY(qAbs(printer.widthMM() - widthMMAfter) <= 2);
    QVERIFY(qAbs(printer.heightMM() - heightMMAfter) <= 2);

    computePageValue(paperWidth, paperHeight);

    QVERIFY(qAbs(paperWidth - widthMMAfter) <= 2);
    QVERIFY(qAbs(paperHeight - heightMMAfter) <= 2);

    // Set it again and see if it still works.
    printer.setPageSize(QPageSize(pageSize));
    QVERIFY(qAbs(printer.widthMM() - widthMMAfter) <= 2);
    QVERIFY(qAbs(printer.heightMM() - heightMMAfter) <= 2);

    printer.setPageOrientation(QPageLayout::Landscape);
    computePageValue(paperWidth, paperHeight);
    QVERIFY(qAbs(paperWidth - heightMMAfter) <= 2);
    QVERIFY(qAbs(paperHeight - widthMMAfter) <= 2);
}

void tst_QPrinter::outputFormatFromSuffix()
{
    if (QPrinterInfo::availablePrinters().size() == 0)
        QSKIP("No printers available.");
    QPrinter p;
    QCOMPARE(p.outputFormat(), QPrinter::NativeFormat);
    p.setOutputFileName(testPdfFileName(QLatin1String("test")));
    QCOMPARE(p.outputFormat(), QPrinter::PdfFormat);
    p.setOutputFileName(QString());
    QCOMPARE(p.outputFormat(), QPrinter::NativeFormat);
}

void tst_QPrinter::testPageMargins_data()
{
    QTest::addColumn<qreal>("left");
    QTest::addColumn<qreal>("top");
    QTest::addColumn<qreal>("right");
    QTest::addColumn<qreal>("bottom");
    QTest::addColumn<QPageLayout::Unit>("unit");

    // Use custom margins that will exceed most printers minimum allowed
    QTest::newRow("data0") << qreal(25.5) << qreal(26.5) << qreal(27.5) << qreal(28.5) << QPageLayout::Millimeter;
    QTest::newRow("data1") << qreal(55.5) << qreal(56.5) << qreal(57.5) << qreal(58.5) << QPageLayout::Point;
    QTest::newRow("data2") << qreal(5.5) << qreal(6.5) << qreal(7.5) << qreal(8.5) << QPageLayout::Inch;
    QTest::newRow("data3") << qreal(5.5) << qreal(6.5) << qreal(7.5) << qreal(8.5) << QPageLayout::Pica;
    QTest::newRow("data4") << qreal(55.5) << qreal(56.5) << qreal(57.5) << qreal(58.5) << QPageLayout::Didot;
    QTest::newRow("data5") << qreal(5.5) << qreal(6.5) << qreal(7.5) << qreal(8.5) << QPageLayout::Cicero;
}

void tst_QPrinter::testPageMargins()
{
    QPrinter obj1;

    QFETCH(qreal, left);
    QFETCH(qreal, top);
    QFETCH(qreal, right);
    QFETCH(qreal, bottom);
    QFETCH(QPageLayout::Unit, unit);

    QPageLayout layout = QPageLayout(QPageSize(QPageSize::A0), QPageLayout::Portrait,
                                     QMarginsF(left, top, right, bottom), unit);

    const QMarginsF margins(left, top, right, bottom);
    obj1.setPageMargins(margins, unit);

    for (const auto compareUnit : { QPageLayout::Millimeter,
                                    QPageLayout::Point,
                                    QPageLayout::Inch,
                                    QPageLayout::Pica,
                                    QPageLayout::Didot,
                                    QPageLayout::Cicero}) {
        QMarginsF actualMargins = obj1.pageLayout().margins(compareUnit);
        QCOMPARE(actualMargins, layout.margins(compareUnit));
    }
}

void tst_QPrinter::errorReporting()
{
    QPrinter p;
    p.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(p.isValid(), true);
    QPainter painter;
#ifndef Q_OS_WIN
    // not sure how to choose a never-writable file on windows.  But its QFile behavior anyway, so lets rely on it failing elsewhere
    p.setOutputFileName("/foobar/nonwritable.pdf");
    QCOMPARE(painter.begin(&p), false); // it should check the output file is writable
#endif
    p.setOutputFileName(testPdfFileName(QLatin1String("test")));
    QCOMPARE(painter.begin(&p), true); // it should check the output
    QCOMPARE(p.isValid(), true);
    painter.end();
}

void tst_QPrinter::testCustomPageSizes()
{
    QPrinter p;

    const QPageSize customSize(QSizeF(7.0, 11.0), QPageSize::Inch);
    p.setPageSize(customSize);

    QSizeF paperSize = p.pageLayout().pageSize().size(QPageSize::Inch);
    QCOMPARE(paperSize.width(), customSize.size(QPageSize::Inch).width());
    QCOMPARE(paperSize.height(), customSize.size(QPageSize::Inch).height());

    QPrinter p2(QPrinter::HighResolution);
    p2.setPageSize(customSize);
    paperSize = p.pageLayout().pageSize().size(QPageSize::Inch);
    QCOMPARE(paperSize.width(), customSize.size(QPageSize::Inch).width());
    QCOMPARE(paperSize.height(), customSize.size(QPageSize::Inch).height());

    const QPageSize pageSize = customSize;
    const QSizeF sizeInPixels = p.paperRect(QPrinter::DevicePixel).size();

    QPrinter p3;
    p3.setPageSize(QPageSize(sizeInPixels / p.resolution(), QPageSize::Inch));
    paperSize = p3.pageLayout().pageSize().size(QPageSize::Inch);
    QCOMPARE(paperSize.width(), customSize.size(QPageSize::Inch).width());
    QCOMPARE(paperSize.height(), customSize.size(QPageSize::Inch).height());
    QCOMPARE(p3.pageLayout().pageSize().key(), QString("Custom.7x11in"));
    QCOMPARE(p3.pageLayout().pageSize().name(), QString("Custom (7in x 11in)"));
}

void tst_QPrinter::customPaperSizeAndMargins_data()
{
    QTest::addColumn<bool>("pdf");
    QTest::addColumn<bool>("before");
    QTest::addColumn<qreal>("left");
    QTest::addColumn<qreal>("top");
    QTest::addColumn<qreal>("right");
    QTest::addColumn<qreal>("bottom");

    // Use custom margins that will exceed most printers minimum allowed
    QTest::newRow("beforeNoPDF")   << false << true  << qreal(30) << qreal(30) << qreal(30) << qreal(30);
    QTest::newRow("beforePDF")     << true  << true  << qreal(30) << qreal(30) << qreal(30) << qreal(30);
    QTest::newRow("afterNoPDF")    << false << false << qreal(30) << qreal(30) << qreal(30) << qreal(30);
    QTest::newRow("afterAfterPDF") << true  << false << qreal(30) << qreal(30) << qreal(30) << qreal(30);
}

void tst_QPrinter::customPaperSizeAndMargins()
{
    QFETCH(bool, pdf);
    QFETCH(bool, before);
    QFETCH(qreal, left);
    QFETCH(qreal, top);
    QFETCH(qreal, right);
    QFETCH(qreal, bottom);

    qreal tolerance = 0.05;
    // Use a custom page size that most printers should support, A4 is 210x297
    // TODO Use print device api when available
    QSizeF customSize(200.0, 300.0);

    const QMarginsF margins(left, top, right, bottom);
    QPrinter p;
    if (pdf)
        p.setOutputFormat(QPrinter::PdfFormat);
    if (before)
        p.setPageMargins(margins, QPageLayout::Millimeter);
    QPageSize customPageSize(customSize, QPageSize::Millimeter);
    p.setPageSize(customPageSize);
    QMarginsF actual = p.pageLayout().margins(QPageLayout::Millimeter);
    if (!before) {
        p.setPageMargins(margins, QPageLayout::Millimeter);
        actual = p.pageLayout().margins(QPageLayout::Millimeter);
    }
    QVERIFY(fabs(left - actual.left()) < tolerance);
    QVERIFY(fabs(top - actual.top()) < tolerance);
    QVERIFY(fabs(right - actual.right()) < tolerance);
    QVERIFY(fabs(bottom - actual.bottom()) < tolerance);
}

#if QT_CONFIG(completer) && QT_CONFIG(filedialog)
void tst_QPrinter::printDialogCompleter()
{
    QPrintDialog dialog;
    dialog.printer()->setOutputFileName(testPdfFileName(QLatin1String("file")));
#if defined(Q_OS_WIN) || defined(Q_OS_DARWIN)
    if (dialog.printer()->outputFormat() != QPrinter::NativeFormat)
        QSKIP("Dialog cannot be used with non-native formats");
#endif
    dialog.setOption(QAbstractPrintDialog::PrintToFile);
    dialog.show();

    QVERIFY(QTest::qWaitForWindowActive(&dialog));

    QTest::keyClick(&dialog, Qt::Key_Tab);
    QTest::keyClick(&dialog, 'P');
    // The test passes if it doesn't crash.
}
#endif

static void printPage(QPainter *painter)
{
    painter->setPen(QPen(Qt::black, 4));
    painter->drawRect(50, 60, 70, 80);
}

void tst_QPrinter::taskQTBUG4497_reusePrinterOnDifferentFiles()
{
    const QString fileName1 = testPdfFileName(QLatin1String("out1_"));
    const QString fileName2 = testPdfFileName(QLatin1String("out2_"));

    QPrinter printer;
    {

        printer.setOutputFileName(fileName1);
        QPainter painter(&printer);
        printPage(&painter);

    }
    {

        printer.setOutputFileName(fileName2);
        QPainter painter(&printer);
        printPage(&painter);

    }
    QFile file1(fileName1);
    QVERIFY(file1.open(QIODevice::ReadOnly));

    QFile file2(fileName2);
    QVERIFY(file2.open(QIODevice::ReadOnly));

    while (!file1.atEnd() && !file2.atEnd()) {
        QByteArray file1Line = file1.readLine();
        QByteArray file2Line = file2.readLine();

        if (!file1Line.startsWith("/CreationDate ") &&
            !file1Line.startsWith("/ModDate ") &&
            !file1Line.startsWith("/ID "))
        {
            QCOMPARE(file1Line, file2Line);
        }
    }

    QVERIFY(file1.atEnd());
    QVERIFY(file2.atEnd());
}

void tst_QPrinter::testCurrentPage()
{
    QPrinter printer;
    printer.setFromTo(1, 10);

    // Test set print range
    printer.setPrintRange(QPrinter::CurrentPage);
    QCOMPARE(printer.printRange(), QPrinter::CurrentPage);
    QCOMPARE(printer.fromPage(), 1);
    QCOMPARE(printer.toPage(), 10);

    QPrintDialog dialog(&printer);

    // Test default Current Page option to off
    QVERIFY(!dialog.testOption(QPrintDialog::PrintCurrentPage));

    // Test enable Current Page option
    dialog.setOption(QPrintDialog::PrintCurrentPage);
    QVERIFY(dialog.testOption(QPrintDialog::PrintCurrentPage));

}

void tst_QPrinter::testPdfTitle()
{
    const QString fileName = testPdfFileName(QLatin1String("file"));

    // Check the document name is represented correctly in produced pdf
    {
        QPainter painter;
        QPrinter printer;
        // This string is just the UTF-8 encoding of the string: \()f &oslash; hiragana o
        const unsigned char titleBuf[]={0x5c, 0x28, 0x29, 0x66, 0xc3, 0xb8, 0xe3, 0x81, 0x8a, 0x00};
        const char *title = reinterpret_cast<const char*>(titleBuf);
        printer.setOutputFileName(fileName);
        printer.setDocName(QString::fromUtf8(title));
        painter.begin(&printer);
        painter.end();
    }
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly));
    // The we expect the title to appear in the PDF as:
    // ASCII('\title (') UTF16(\\\(\)f &oslash; hiragana o) ASCII(')').
    // which has the following binary representation
    const unsigned char expectedBuf[] = {
        0x2f, 0x54, 0x69, 0x74, 0x6c, 0x65, 0x20, 0x28, 0xfe,
        0xff, 0x00, 0x5c, 0x5c, 0x00, 0x5c, 0x28, 0x00, 0x5c,
        0x29, 0x00, 0x66, 0x00, 0xf8, 0x30, 0x4a, 0x29};
    const char *expected = reinterpret_cast<const char*>(expectedBuf);
    QVERIFY(file.readAll().contains(QByteArray(expected, 26)));
}

void tst_QPrinter::customPaperNameSettingBySize()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrinterInfo info(printer);
    const QList<QPageSize> sizes = info.supportedPageSizes();
    if (sizes.size() == 0)
        QSKIP("No printers installed on this machine");
    for (const auto &pageSize : sizes) {
        printer.setPageSize(pageSize);
        QCOMPARE(pageSize.size(QPageSize::Millimeter), printer.pageLayout().pageSize().size(QPageSize::Millimeter));
        // Some printers have the same size under different names which can cause a problem for the test
        // So we look at all the other sizes to see if one also matches as we don't know which order they are in
        const QSizeF paperSize = pageSize.size(QPageSize::Millimeter);
        const QString paperName = printer.pageLayout().pageSize().name();
        bool paperNameFound = (pageSize.name() == paperName);
        if (!paperNameFound) {
            for (const auto &pageSize2 : sizes) {
                if (pageSize != pageSize2
                    && pageSize2.size(QPageSize::Millimeter) == paperSize
                    && pageSize2.name() == paperName) {
                    paperNameFound = true;
                    break;
                }
            }
        }
        // Fail with the original values
        if (!paperNameFound) {
            qDebug() << "supportedPageSizes() = " << sizes;
            QEXPECT_FAIL("", "Paper Name mismatch: please report this failure at bugreports.qt.io", Continue);
            QCOMPARE(pageSize.name(), printer.pageLayout().pageSize().name());
        }
    }

    // Check setting a custom size after setting a standard one works
    const QSizeF customSize(200, 300);
    printer.setPageSize(QPageSize(customSize, QPageSize::Millimeter));
    QCOMPARE(printer.pageLayout().pageSize().size(QPageSize::Millimeter), customSize);
    QCOMPARE(printer.pageLayout().pageSize().id(), QPageSize::Custom);

    // Finally check setting a standard size after a custom one works
    const QPageSize standardPageSize = sizes.first();
    printer.setPageSize(standardPageSize);
    QCOMPARE(printer.pageLayout().pageSize().name(), standardPageSize.name());
    QCOMPARE(printer.pageLayout().pageSize().size(QPageSize::Millimeter), standardPageSize.size(QPageSize::Millimeter));
}

// Test QPrintEngine keys and their QPrinter setters/getters

void tst_QPrinter::testMultipleKeys()
{
    // Tests multiple keys preservation, note are only ones that are consistent across all engines

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Check default values
        QCOMPARE(native.fullPage(), false);
        QCOMPARE(native.pageLayout().orientation(), QPageLayout::Portrait);
        QCOMPARE(native.copyCount(), 1);
        QCOMPARE(native.collateCopies(), true);
        QCOMPARE(native.printRange(), QPrinter::AllPages);

        // Change values
        native.setFullPage(true);
        native.setPageOrientation(QPageLayout::Landscape);
        native.setCopyCount(9);
        native.setCollateCopies(false);
        native.setPrintRange(QPrinter::CurrentPage);

        // Check changed values
        QCOMPARE(native.fullPage(), true);
        QCOMPARE(native.pageLayout().orientation(), QPageLayout::Landscape);
        QCOMPARE(native.copyCount(), 9);
        QCOMPARE(native.collateCopies(), false);
        QCOMPARE(native.printRange(), QPrinter::CurrentPage);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.fullPage(), true);
        QCOMPARE(native.pageLayout().orientation(), QPageLayout::Landscape);
        QCOMPARE(native.copyCount(), 9);
        QCOMPARE(native.collateCopies(), false);
        QCOMPARE(native.printRange(), QPrinter::CurrentPage);

        // Change values
        native.setFullPage(false);
        native.setPageOrientation(QPageLayout::Portrait);
        native.setCopyCount(5);
        native.setCollateCopies(true);
        native.setPrintRange(QPrinter::PageRange);

        // Check changed values
        QCOMPARE(native.fullPage(), false);
        QCOMPARE(native.pageLayout().orientation(), QPageLayout::Portrait);
        QCOMPARE(native.copyCount(), 5);
        QCOMPARE(native.collateCopies(), true);
        QCOMPARE(native.printRange(), QPrinter::PageRange);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::collateCopies()
{
    // collateCopies() / setCollateCopies() / PPK_ColorMode
    // PdfFormat: Supported, default true
    // NativeFormat, Cups: Supported, default true
    // NativeFormat, Win: Supported, default true
    // NativeFormat, Mac: Supported, default true

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.collateCopies(), true);
    pdf.setCollateCopies(false);
    QCOMPARE(pdf.collateCopies(), false);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QCOMPARE(native.collateCopies(), true);

        // Test set/get
        bool expected = false;
        native.setCollateCopies(expected);
        QCOMPARE(native.collateCopies(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.collateCopies(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.collateCopies(), expected);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::colorMode()
{
    // colorMode() / setColorMode() / PPK_ColorMode
    // PdfFormat: Supported, default QPrinter::Color
    // NativeFormat, Cups: Supported, default QPrinter::Color
    // NativeFormat, Win: Supported if valid DevMode, otherwise QPrinter::Color
    // NativeFormat, Mac: Unsupported, always QPrinter::Color

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.colorMode(), QPrinter::Color);
    pdf.setColorMode(QPrinter::GrayScale);
    QCOMPARE(pdf.colorMode(), QPrinter::GrayScale);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        // TODO Printer specific, need QPrinterInfo::colorMode()
        //QCOMPARE(native.colorMode(), QPrinter::Color);

        // Test set/get
        QPrinter::ColorMode expected = QPrinter::GrayScale;
        native.setColorMode(expected);
#ifdef Q_OS_MAC
        expected = QPrinter::Color;
#endif // Q_OS_MAC
        QCOMPARE(native.colorMode(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.colorMode(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.colorMode(), expected);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::copyCount()
{
    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.supportsMultipleCopies(), false);
    QCOMPARE(pdf.copyCount(), 1);
    pdf.setCopyCount(9);
    QCOMPARE(pdf.copyCount(), 9);
    pdf.setCopyCount(7);
    QCOMPARE(pdf.copyCount(), 7);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QCOMPARE(native.supportsMultipleCopies(), true);
        QCOMPARE(native.copyCount(), 1);

        // Test set/get
        native.setCopyCount(9);
        QCOMPARE(native.copyCount(), 9);
        native.setCopyCount(7);
        QCOMPARE(native.copyCount(), 7);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.copyCount(), 7);

        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.copyCount(), 7);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::creator()
{
    // creator() / setCreator() / PPK_Creator
    // PdfFormat: Supported, default QString()
    // NativeFormat, Cups: Supported, default QString()
    // NativeFormat, Win: Supported, default QString()
    // NativeFormat, Mac: Supported, default QString()

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.creator(), QString());
    pdf.setCreator(QStringLiteral("Test Creator"));
    QCOMPARE(pdf.creator(), QStringLiteral("Test Creator"));

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QCOMPARE(native.creator(), QString());

        // Test set/get
        QString expected = QStringLiteral("Test Creator");
        native.setCreator(expected);
        QCOMPARE(native.creator(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.creator(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.creator(), expected);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::docName()
{
    // docName() / setDocName() / PPK_DocumentName
    // PdfFormat: Supported, default QString()
    // NativeFormat, Cups: Supported, default QString()
    // NativeFormat, Win: Supported, default QString()
    // NativeFormat, Mac: Supported, default QString()

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.docName(), QString());
    pdf.setDocName(QStringLiteral("Test Name"));
    QCOMPARE(pdf.docName(), QStringLiteral("Test Name"));

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QCOMPARE(native.docName(), QString());

        // Test set/get
        QString expected = QStringLiteral("Test Name");
        native.setDocName(expected);
        QCOMPARE(native.docName(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.docName(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.docName(), expected);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::duplex()
{
    // duplex()) / setDuplex() / PPK_Duplex
    // PdfFormat: Supported, default QPrinter::DuplexNone
    // NativeFormat, Cups: Supported, default to printer default
    // NativeFormat, Win: Supported, default to printer default
    // NativeFormat, Mac: Supported, default to printer default

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.duplex(), QPrinter::DuplexNone);
    pdf.setDuplex(QPrinter::DuplexAuto);
    QCOMPARE(pdf.duplex(), QPrinter::DuplexNone); // pdf doesn't have the concept of duplex

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QPrinterInfo printerInfo = QPrinterInfo::defaultPrinter();
        QPrinter::DuplexMode expected = printerInfo.defaultDuplexMode();
        QCOMPARE(native.duplex(), expected);
        // Test set/get (skipping Auto as that will return something different)
        const auto supported = printerInfo.supportedDuplexModes();
        for (QPrinter::DuplexMode mode : supported) {
            if (mode != expected && mode != QPrinter::DuplexAuto) {
                expected = mode;
                break;
            }
        }
        native.setDuplex(expected);
        QCOMPARE(native.duplex(), expected);

        // Test that PdfFormat printer has no duplex.
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.duplex(), QPrinter::DuplexNone);
        native.setOutputFormat(QPrinter::NativeFormat);

        // Test setting invalid option
        if (!printerInfo.supportedDuplexModes().contains(QPrinter::DuplexLongSide)) {
            native.setDuplex(QPrinter::DuplexLongSide);
            QCOMPARE(native.duplex(), expected);
        }
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::fontEmbedding()
{
    // fontEmbeddingEnabled() / setFontEmbeddingEnabled() / PPK_FontEmbedding
    // PdfFormat: Supported, default true
    // NativeFormat: Supported, default true

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.fontEmbeddingEnabled(), true);
    pdf.setFontEmbeddingEnabled(false);
    QCOMPARE(pdf.fontEmbeddingEnabled(), false);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QCOMPARE(native.fontEmbeddingEnabled(), true);

        // Test set/get
        native.setFontEmbeddingEnabled(true);
        QCOMPARE(native.fontEmbeddingEnabled(), true);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.fontEmbeddingEnabled(), true);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.fontEmbeddingEnabled(), true);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::fullPage()
{
    // fullPage() / setFullPage() / PPK_FullPage
    // PdfFormat: Supported, default false
    // NativeFormat, Cups: Supported, default false
    // NativeFormat, Win: Supported, default false
    // NativeFormat, Mac: Supported, default false

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.fullPage(), false);
    pdf.setFullPage(true);
    QCOMPARE(pdf.fullPage(), true);
    pdf.setFullPage(false);
    QCOMPARE(pdf.fullPage(), false);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QCOMPARE(native.fullPage(), false);

        // Test set/get
        bool expected = true;
        native.setFullPage(expected);
        QCOMPARE(native.fullPage(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.fullPage(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.fullPage(), expected);

        // Test set/get
        expected = false;
        native.setFullPage(expected);
        QCOMPARE(native.fullPage(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.fullPage(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.fullPage(), expected);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::orientation()
{
    // orientation() / setOrientation() / PPK_Orientation
    // PdfFormat: Supported, default QPageLayout::Portrait
    // NativeFormat, Cups: Supported, default QPageLayout::Portrait
    // NativeFormat, Win: Supported, default QPageLayout::Portrait
    // NativeFormat, Mac: Supported, default QPageLayout::Portrait

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.pageLayout().orientation(), QPageLayout::Portrait);
    pdf.setPageOrientation(QPageLayout::Landscape);
    QCOMPARE(pdf.pageLayout().orientation(), QPageLayout::Landscape);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        // TODO Printer specific, need QPrinterInfo::orientation()
        //QCOMPARE(native.orientation(), QPageLayout::Portrait);

        // Test set/get
        QPageLayout::Orientation expected = QPageLayout::Landscape;
        native.setPageOrientation(expected);
        QCOMPARE(native.pageLayout().orientation(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.pageLayout().orientation(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.pageLayout().orientation(), expected);

        // Test set/get
        expected = QPageLayout::Portrait;
        native.setPageOrientation(expected);
        QCOMPARE(native.pageLayout().orientation(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.pageLayout().orientation(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.pageLayout().orientation(), expected);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::outputFileName()
{
    // outputFileName() / setOutputFileName() / PPK_OutputFileName
    // PdfFormat: Supported, default QString()
    // NativeFormat, Cups: Supported, default QString()
    // NativeFormat, Win: Supported, default QString()
    // NativeFormat, Mac: Supported, default QString()

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.outputFileName(), QString());
    const QString fileName = testFileName(QStringLiteral("Test File"), QString());
    pdf.setOutputFileName(fileName);
    QCOMPARE(pdf.outputFileName(), fileName);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QCOMPARE(native.outputFileName(), QString());

        // Test set/get
        QString expected = fileName;
        native.setOutputFileName(expected);
        QCOMPARE(native.outputFileName(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.outputFileName(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.outputFileName(), expected);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::pageOrder()
{
    // pageOrder() / setPageOrder() / PPK_PageOrder
    // PdfFormat: Supported, default QPrinter::FirstPageFirst
    // NativeFormat, Cups: Supported, default QPrinter::FirstPageFirst
    // NativeFormat, Win: Unsupported, always QPrinter::FirstPageFirst
    // NativeFormat, Mac: Unsupported, always QPrinter::FirstPageFirst

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.pageOrder(), QPrinter::FirstPageFirst);
    pdf.setPageOrder(QPrinter::LastPageFirst);
    QCOMPARE(pdf.pageOrder(), QPrinter::LastPageFirst);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QCOMPARE(native.pageOrder(), QPrinter::FirstPageFirst);

        // Test set/get
        QPrinter::PageOrder expected = QPrinter::LastPageFirst;
        native.setPageOrder(expected);
#if defined Q_OS_MAC || defined Q_OS_WIN
        expected = QPrinter::FirstPageFirst;
#endif // Q_OS_MAC || Q_OS_WIN
        QCOMPARE(native.pageOrder(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.pageOrder(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.pageOrder(), expected);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::pageSize()
{
    // Note PPK_PaperSize == PPK_PageSize
    // pageSize() / setPageSize() / PPK_PageSize
    // PdfFormat: Supported, defaults to QPageSize::A4
    // NativeFormat, Cups: Supported, defaults to printer default
    // NativeFormat, Win: Supported, defaults to printer default
    // NativeFormat, Mac: Supported, must be supported size, defaults to printer default

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.pageLayout().pageSize().id(), QPageSize::A4);
    pdf.setPageSize(QPageSize(QPageSize::A1));
    QCOMPARE(pdf.pageLayout().pageSize().id(), QPageSize::A1);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        // TODO Printer specific, need QPrinterInfo::paperSize()
        //QCOMPARE(native.pageSize(), QPageSize::A4);

        // Test set/get
        QPageSize expected(QPageSize::A4);
        QPrinterInfo info = QPrinterInfo::printerInfo(native.printerName());
        const auto &pageSizes = info.supportedPageSizes();
        for (const auto &pageSize : pageSizes) {
            const auto supported = pageSize.id();
            if (supported != QPageSize::Custom && supported != native.pageLayout().pageSize().id()) {
                expected = QPageSize(supported);
                break;
            }
        }
        native.setPageSize(expected);
        QCOMPARE(native.pageLayout().pageSize().id(), expected.id());

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.pageLayout().pageSize().id(), expected.id());
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.pageLayout().pageSize().id(), expected.id());
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}


void tst_QPrinter::paperSource()
{
    // paperSource() / setPaperSource() / PPK_PaperSource
    // PdfFormat: Supported, defaults to QPrinter::Auto
    // NativeFormat, Cups: Supported, defaults to QPrinter::Auto
    // NativeFormat, Win: Supported if valid DevMode and in supportedPaperSources(), otherwise QPrinter::Auto
    // NativeFormat, Mac: Unsupported, always QPrinter::Auto

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.paperSource(), QPrinter::Auto);
    pdf.setPaperSource(QPrinter::Lower);
    QCOMPARE(pdf.paperSource(), QPrinter::Lower);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        // TODO Printer specific, need QPrinterInfo::paperSource()
        //QCOMPARE(native.paperSource(), QPrinter::Auto);

        // Test set/get
        QPrinter::PaperSource expected = QPrinter::Manual;
#ifdef Q_OS_WIN
        expected = QPrinter::Auto;
        const auto sources = native.supportedPaperSources();
        for (QPrinter::PaperSource supported : sources) {
            if (supported != QPrinter::Auto) {
                expected = supported;
                break;
            }
        }
#endif // Q_OS_WIN
        native.setPaperSource(expected);
#ifdef Q_OS_MAC
        expected = QPrinter::Auto;
#endif // Q_OS_MAC
        QCOMPARE(native.paperSource(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.paperSource(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.paperSource(), expected);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::printProgram()
{
    // printProgram() / setPrintProgram() / PPK_PrintProgram
    // PdfFormat: Supported, default QString()
    // NativeFormat, Cups: Supported, default QString()
    // NativeFormat, Win: Unsupported, always QString()
    // NativeFormat, Mac: Unsupported, always QString()

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.printProgram(), QString());
    pdf.setPrintProgram(QStringLiteral("/usr/bin/lpr"));
    QCOMPARE(pdf.printProgram(), QStringLiteral("/usr/bin/lpr"));

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QCOMPARE(native.printProgram(), QString());

        // Test set/get
        QString expected = QStringLiteral("/usr/bin/lpr");
        native.setPrintProgram(expected);
#if defined Q_OS_MAC || defined Q_OS_WIN
        expected.clear();
#endif // Q_OS_MAC || Q_OS_WIN
        QCOMPARE(native.printProgram(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.printProgram(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.printProgram(), expected);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::printRange()
{
    // printRange() / setPrintRange() / PPK_PrintRange
    // PdfFormat: Supported, default QPrinter::AllPages
    // NativeFormat, Cups: Supported, default QPrinter::AllPages
    // NativeFormat, Win: Supported, default QPrinter::AllPages
    // NativeFormat, Mac: Supported, default QPrinter::AllPages

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.printRange(), QPrinter::AllPages);
    pdf.setPrintRange(QPrinter::CurrentPage);
    QCOMPARE(pdf.printRange(), QPrinter::CurrentPage);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QCOMPARE(native.printRange(), QPrinter::AllPages);

        // Test set/get
        QPrinter::PrintRange expected = QPrinter::PageRange;
        native.setPrintRange(expected);
        QCOMPARE(native.printRange(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.printRange(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.printRange(), expected);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::printerName()
{
    // printerName() / setPrinterName() / PPK_PrinterName
    // PdfFormat: Supported, default QString
    // NativeFormat, Cups: Supported, default printer
    // NativeFormat, Win: Supported, default printer
    // NativeFormat, Mac: Supported, default printer

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.printerName(), QString());
    if (QPrinterInfo::availablePrinters().size() == 0) {
        pdf.setPrinterName(QStringLiteral("Test Printer"));
        QCOMPARE(pdf.printerName(), QString());
        QCOMPARE(pdf.outputFormat(), QPrinter::PdfFormat);
    } else {
        pdf.setPrinterName(QPrinterInfo::defaultPrinter().printerName());
        QCOMPARE(pdf.printerName(), QPrinterInfo::defaultPrinter().printerName());
        QCOMPARE(pdf.outputFormat(), QPrinter::NativeFormat);
    }

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QCOMPARE(native.printerName(), QPrinterInfo::defaultPrinter().printerName());

        // Test set/get
        QString expected = QPrinterInfo::defaultPrinter().printerName();
        const auto allAvailable = QPrinterInfo::availablePrinters();
        for (const QPrinterInfo &available : allAvailable) {
            if (available.printerName() != expected) {
                expected = available.printerName();
                break;
            }
        }
        native.setPrinterName(expected);
        QCOMPARE(native.printerName(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.printerName(), QString());
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.printerName(), QPrinterInfo::defaultPrinter().printerName());
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::printerSelectionOption()
{
    // printerSelectionOption() / setPrinterSelectionOption() / PPK_SelectionOption
    // PdfFormat: Supported
    // NativeFormat, Cups: Supported
    // NativeFormat, Win: Unsupported, always QString()
    // NativeFormat, Mac: Unsupported, always QString()

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.printerSelectionOption(), QString());
    pdf.setPrinterSelectionOption(QStringLiteral("Optional option"));
    QCOMPARE(pdf.printerSelectionOption(), QString("Optional option"));

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QCOMPARE(native.printerSelectionOption(), QString());

        // Test set/get
        QString expected = QStringLiteral("Optional option");
        native.setPrinterSelectionOption(expected);
#if defined Q_OS_MAC || defined Q_OS_WIN
        expected.clear();
#endif // Q_OS_MAC || Q_OS_WIN
        QCOMPARE(native.printerSelectionOption(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.printerSelectionOption(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.printerSelectionOption(), expected);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::resolution()
{
    // resolution() / setResolution() / PPK_Resolution
    // PdfFormat: Supported, can be any number, but only 72 returned by supportedResolutions()
    // NativeFormat, Cups: Supported, can be any number, but only 72 returned by supportedResolutions()
    // NativeFormat, Win: Supported, can be any number, but supportedResolutions() returns valid list
    // NativeFormat, Mac: Supported, but can only be value returned by supportedResolutions()

    QPrinter pdfScreen(QPrinter::ScreenResolution);
    pdfScreen.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdfScreen.resolution(), 96);
    pdfScreen.setResolution(333);
    QCOMPARE(pdfScreen.resolution(), 333);

    QPrinter pdfPrinter(QPrinter::PrinterResolution);
    pdfPrinter.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdfPrinter.resolution(), 72);
    pdfPrinter.setResolution(333);
    QCOMPARE(pdfPrinter.resolution(), 333);

    QPrinter pdfHigh(QPrinter::HighResolution);
    pdfHigh.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdfHigh.resolution(), 1200);
    pdfHigh.setResolution(333);
    QCOMPARE(pdfHigh.resolution(), 333);

    QPrinter native(QPrinter::HighResolution);
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        // TODO Printer specific, need QPrinterInfo::resolution()
        //QCOMPARE(native.resolution(), 300);

        // Test set/get
        int expected = 333;
#ifdef Q_OS_MAC
        // QMacPrintEngine chooses the closest supported resolution.
        const QList<int> all_supported = native.supportedResolutions();
        for (int supported : all_supported) {
            // Test setting a supported resolution
            int requested = supported;
            native.setResolution(requested);
            QCOMPARE(native.resolution(), requested);

            // Test setting an unsupported resolution
            do {
                requested += 5;
            } while (all_supported.contains(requested));
            native.setResolution(requested);
            int result = native.resolution();
            QVERIFY(all_supported.contains(result));
            QVERIFY(qAbs(result - requested) <= qAbs(supported - requested));
        }

        expected = native.resolution();
#endif // Q_OS_MAC
        native.setResolution(expected);
        QCOMPARE(native.resolution(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.resolution(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.resolution(), expected);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::supportedPaperSources()
{
    // supportedPaperSources() / PPK_PaperSources
    // PdfFormat: ifdef'd out TODO remove ifdef
    // NativeFormat, Cups: ifdef'd out TODO remove ifdef
    // NativeFormat, Win: Supported, defaults to printer default
    // NativeFormat, Mac: ifdef'd out TODO remove ifdef

#ifdef Q_OS_WIN
    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        native.supportedPaperSources();
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
#endif // Q_OS_WIN
}

void tst_QPrinter::supportedResolutions()
{
    // supportedResolutions() / PPK_SupportedResolutions
    // PdfFormat: Supported, only returns 72
    // NativeFormat, Cups: Supported, only returns 72
    // NativeFormat, Win: Supported, defaults to printer list
    // NativeFormat, Mac: Supported, defaults to printer list

    QList<int> expected;

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    expected << 72;
    QCOMPARE(pdf.supportedResolutions(), expected);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        native.supportedResolutions();
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

// Test QPrinter setters/getters for non-QPrintEngine options

void tst_QPrinter::outputFormat()
{
    QPrinter printer;
    if (QPrinterInfo::availablePrinters().size() == 0) {
        QCOMPARE(printer.outputFormat(), QPrinter::PdfFormat);
        QCOMPARE(printer.printerName(), QString());
    } else {
        QCOMPARE(printer.outputFormat(), QPrinter::NativeFormat);

        // If no printer is default, the first available printer should be used.
        // Otherwise, the default printer should be used.
        if (QPrinterInfo::defaultPrinter().isNull())
            QCOMPARE(printer.printerName(), QPrinterInfo::availablePrinters().at(0).printerName());
        else
            QCOMPARE(printer.printerName(), QPrinterInfo::defaultPrinter().printerName());
    }

    printer.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(printer.outputFormat(), QPrinter::PdfFormat);
    QCOMPARE(printer.printerName(), QString());
}

void tst_QPrinter::fromToPage()
{
    QPrinter printer;
    QCOMPARE(printer.fromPage(), 0);
    QCOMPARE(printer.toPage(), 0);
    printer.setFromTo(3, 7);
    QCOMPARE(printer.fromPage(), 3);
    QCOMPARE(printer.toPage(), 7);
}

void tst_QPrinter::testPageMetrics_data()
{
    QTest::addColumn<int>("outputFormat");
    QTest::addColumn<int>("pageSize");
    QTest::addColumn<qreal>("widthMMf");
    QTest::addColumn<qreal>("heightMMf");
    QTest::addColumn<bool>("setMargins");
    QTest::addColumn<qreal>("leftMMf");
    QTest::addColumn<qreal>("rightMMf");
    QTest::addColumn<qreal>("topMMf");
    QTest::addColumn<qreal>("bottomMMf");

    QTest::newRow("PDF A4")            << int(QPrinter::PdfFormat)    << int(QPageSize::A4) << 210.0 << 297.0 << false <<  0.0 <<  0.0 <<  0.0 <<  0.0;
    QTest::newRow("PDF A4 Margins")    << int(QPrinter::PdfFormat)    << int(QPageSize::A4) << 210.0 << 297.0 << true  << 20.0 << 30.0 << 40.0 << 50.0;
    QTest::newRow("Native A4")         << int(QPrinter::NativeFormat) << int(QPageSize::A4) << 210.0 << 297.0 << false <<  0.0 <<  0.0 <<  0.0 <<  0.0;
    QTest::newRow("Native A4 Margins") << int(QPrinter::NativeFormat) << int(QPageSize::A4) << 210.0 << 297.0 << true  << 20.0 << 30.0 << 40.0 << 50.0;

    QTest::newRow("PDF Portrait")             << int(QPrinter::PdfFormat)    << -1 << 200.0 << 300.0 << false <<  0.0 <<  0.0 <<  0.0 <<  0.0;
    QTest::newRow("PDF Portrait Margins")     << int(QPrinter::PdfFormat)    << -1 << 200.0 << 300.0 << true  << 20.0 << 30.0 << 40.0 << 50.0;
    QTest::newRow("PDF Landscape")            << int(QPrinter::PdfFormat)    << -1 << 300.0 << 200.0 << false <<  0.0 <<  0.0 <<  0.0 <<  0.0;
    QTest::newRow("PDF Landscape Margins")    << int(QPrinter::PdfFormat)    << -1 << 300.0 << 200.0 << true  << 20.0 << 30.0 << 40.0 << 50.0;
    QTest::newRow("Native Portrait")          << int(QPrinter::NativeFormat) << -1 << 200.0 << 300.0 << false <<  0.0 <<  0.0 <<  0.0 <<  0.0;
    QTest::newRow("Native Portrait Margins")  << int(QPrinter::NativeFormat) << -1 << 200.0 << 300.0 << true  << 20.0 << 30.0 << 40.0 << 50.0;
    QTest::newRow("Native Landscape")         << int(QPrinter::NativeFormat) << -1 << 300.0 << 200.0 << false <<  0.0 <<  0.0 <<  0.0 <<  0.0;
    QTest::newRow("Native Landscape Margins") << int(QPrinter::NativeFormat) << -1 << 300.0 << 200.0 << true  << 20.0 << 30.0 << 40.0 << 50.0;
}

void tst_QPrinter::testPageMetrics()
{
    QFETCH(int, outputFormat);
    QFETCH(int, pageSize);
    QFETCH(qreal, widthMMf);
    QFETCH(qreal, heightMMf);
    QFETCH(bool, setMargins);
    QFETCH(qreal, leftMMf);
    QFETCH(qreal, rightMMf);
    QFETCH(qreal, topMMf);
    QFETCH(qreal, bottomMMf);

    const QSizeF sizeMMf = QSizeF(widthMMf, heightMMf);

    QPrinter printer;
    printer.setOutputFormat(QPrinter::OutputFormat(outputFormat));
    if (printer.outputFormat() != QPrinter::OutputFormat(outputFormat))
        QSKIP("Please install a native printer to run this test");
    QCOMPARE(printer.outputFormat(), QPrinter::OutputFormat(outputFormat));
    QCOMPARE(printer.pageLayout().orientation(), QPageLayout::Portrait);

    if (setMargins) {
        // Setup the given margins
        QMarginsF margins(leftMMf, topMMf, rightMMf, bottomMMf);
        printer.setPageMargins(margins);
        QCOMPARE(printer.pageLayout().margins(), margins);
    }

    // Set the given size, in Portrait mode
    if (pageSize < 0) {
        printer.setPageSize(QPageSize(sizeMMf, QPageSize::Millimeter));
        QCOMPARE(printer.pageLayout().pageSize().id(), QPageSize::Custom);
    } else {
        printer.setPageSize(QPageSize(QPageSize::PageSizeId(pageSize)));
        QCOMPARE(printer.pageLayout().pageSize().id(), QPageSize::PageSizeId(pageSize));
    }
    QCOMPARE(printer.pageLayout().orientation(), QPageLayout::Portrait);
    if (setMargins) {
        // Check margins unchanged from page size change
        QMarginsF margins(leftMMf, topMMf, rightMMf, bottomMMf);
        QCOMPARE(printer.pageLayout().margins(), margins);
    } else {
        // Fetch the default margins for the printer and page size
        // TODO Check against margins from print device when api added
        leftMMf = printer.pageLayout().margins(QPageLayout::Millimeter).left();
        rightMMf = printer.pageLayout().margins(QPageLayout::Millimeter).right();
        topMMf = printer.pageLayout().margins(QPageLayout::Millimeter).top();
        bottomMMf = printer.pageLayout().margins(QPageLayout::Millimeter).bottom();
    }

    // QPageLayout::pageSize() always returns Portrait
    QCOMPARE(printer.pageLayout().pageSize().size(QPageSize::Millimeter), sizeMMf);

    // QPrinter::paperRect() always returns set orientation
    QCOMPARE(printer.paperRect(QPrinter::Millimeter).size(), sizeMMf);


    // QPaintDevice::widthMM() and heightMM() are paint metrics and always return set orientation
    QCOMPARE(printer.widthMM(), qRound(widthMMf - leftMMf - rightMMf));
    QCOMPARE(printer.heightMM(), qRound(heightMMf - topMMf - bottomMMf));

    // QPrinter::paperRect() always returns set orientation
    QCOMPARE(printer.paperRect(QPrinter::Millimeter), QRectF(0, 0, widthMMf, heightMMf));

    // QPrinter::pageRect() always returns set orientation
    QCOMPARE(printer.pageRect(QPrinter::Millimeter), QRectF(leftMMf, topMMf, widthMMf - leftMMf - rightMMf, heightMMf - topMMf - bottomMMf));


    // Now switch to Landscape mode, size should be unchanged, but rect and metrics should change
    printer.setPageOrientation(QPageLayout::Landscape);
    if (pageSize < 0) {
        QCOMPARE(printer.pageLayout().pageSize().id(), QPageSize::Custom);
    } else {
        QCOMPARE(printer.pageLayout().pageSize().id(), QPageSize::PageSizeId(pageSize));
    }
    QCOMPARE(printer.pageLayout().orientation(), QPageLayout::Landscape);
    if (setMargins) {
        // Check margins unchanged from page size change
        QMarginsF margins(leftMMf, topMMf, rightMMf, bottomMMf);
        QCOMPARE(printer.pageLayout().margins(), margins);
    } else {
        // Fetch the default margins for the printer and page size
        // TODO Check against margins from print device when api added
        leftMMf = printer.pageLayout().margins(QPageLayout::Millimeter).left();
        rightMMf = printer.pageLayout().margins(QPageLayout::Millimeter).right();
        topMMf = printer.pageLayout().margins(QPageLayout::Millimeter).top();
        bottomMMf = printer.pageLayout().margins(QPageLayout::Millimeter).bottom();
    }

    // QPageLayout::pageSize() always returns Portrait
    QCOMPARE(printer.pageLayout().pageSize().size(QPageSize::Millimeter), sizeMMf);

    // QPrinter::paperRect() always returns set orientation
    QCOMPARE(printer.paperRect(QPrinter::Millimeter).size(), sizeMMf.transposed());

    // QPaintDevice::widthMM() and heightMM() are paint metrics and always return set orientation
    QCOMPARE(printer.widthMM(), qRound(heightMMf - leftMMf - rightMMf));
    QCOMPARE(printer.heightMM(), qRound(widthMMf - topMMf - bottomMMf));

    // QPrinter::paperRect() always returns set orientation
    QCOMPARE(printer.paperRect(QPrinter::Millimeter), QRectF(0, 0, heightMMf, widthMMf));

    // QPrinter::pageRect() always returns set orientation
    QCOMPARE(printer.pageRect(QPrinter::Millimeter), QRectF(leftMMf, topMMf, heightMMf - leftMMf - rightMMf, widthMMf - topMMf - bottomMMf));

    // Now while in Landscape mode, set the size again, results should be the same
    if (pageSize < 0) {
        printer.setPageSize(QPageSize(sizeMMf, QPageSize::Millimeter));
        QCOMPARE(printer.pageLayout().pageSize().id(), QPageSize::Custom);
    } else {
        printer.setPageSize(QPageSize(QPageSize::PageSizeId(pageSize)));
        QCOMPARE(printer.pageLayout().pageSize().id(), QPageSize::PageSizeId(pageSize));
    }
    QCOMPARE(printer.pageLayout().orientation(), QPageLayout::Landscape);
    if (setMargins) {
        // Check margins unchanged from page size change
        QMarginsF margins(leftMMf, topMMf, rightMMf, bottomMMf);
        QCOMPARE(printer.pageLayout().margins(), margins);
    } else {
        // Fetch the default margins for the printer and page size
        // TODO Check against margins from print device when api added
        leftMMf = printer.pageLayout().margins(QPageLayout::Millimeter).left();
        rightMMf = printer.pageLayout().margins(QPageLayout::Millimeter).right();
        topMMf = printer.pageLayout().margins(QPageLayout::Millimeter).top();
        bottomMMf = printer.pageLayout().margins(QPageLayout::Millimeter).bottom();
    }

    // QPageLayout::pageSize() always returns Portrait
    QCOMPARE(printer.pageLayout().pageSize().size(QPageSize::Millimeter), sizeMMf);

    // QPrinter::paperRect() always returns set orientation
    QCOMPARE(printer.paperRect(QPrinter::Millimeter).size(), sizeMMf.transposed());

    // QPagedPaintDevice::widthMM() and heightMM() are paint metrics and always return set orientation
    QCOMPARE(printer.widthMM(), qRound(heightMMf - leftMMf - rightMMf));
    QCOMPARE(printer.heightMM(), qRound(widthMMf - topMMf - bottomMMf));

    // QPrinter::paperRect() always returns set orientation
    QCOMPARE(printer.paperRect(QPrinter::Millimeter), QRectF(0, 0, heightMMf, widthMMf));

    // QPrinter::pageRect() always returns set orientation
    QCOMPARE(printer.pageRect(QPrinter::Millimeter), QRectF(leftMMf, topMMf, heightMMf - leftMMf - rightMMf, widthMMf - topMMf - bottomMMf));
}

QString tst_QPrinter::testFileName(const QString &prefix, const QString &suffix)
{
    QString result = m_tempDir.path() + QLatin1Char('/') + prefix
        + QString::number(fileNumber++);
    if (!suffix.isEmpty())
        result += QLatin1Char('.') + suffix;
    return result;
}

void tst_QPrinter::reusePageMetrics()
{
    QList<QPrinterInfo> availablePrinters = QPrinterInfo::availablePrinters();
    if (availablePrinters.size() < 2)
        QSKIP("Not enough printers to do this test with, need at least 2 setup");
    QPrinter defaultP;
    QPrinterInfo info(defaultP);
    QString otherPrinterName;
    for (QPrinterInfo i : std::as_const(availablePrinters)) {
        if (i.printerName() != defaultP.printerName()) {
            otherPrinterName = i.printerName();
            break;
        }
    }
    QPrinter otherP(QPrinterInfo::printerInfo(otherPrinterName));
    QList<QPageSize> defaultPageSizes = info.supportedPageSizes();
    QList<QPageSize> otherPageSizes = QPrinterInfo(otherP).supportedPageSizes();
    QPageSize unavailableSizeToSet;
    for (QPageSize s : std::as_const(defaultPageSizes)) {
        bool found = false;
        for (QPageSize os : std::as_const(otherPageSizes)) {
            if (os.isEquivalentTo(s)) {
                found = true;
                break;
            }
        }
        const QPageSize tmpSize(s.size(QPageSize::Point), QPageSize::Point);
        if (!tmpSize.name().startsWith("Custom"))
            found = true;
        if (!found) {
            unavailableSizeToSet = s;
            break;
        }
    }
    if (!unavailableSizeToSet.isValid())
        QSKIP("Could not find a size that was not available on the non default printer. The test "
              "requires this");
    defaultP.setPageSize(unavailableSizeToSet);
    defaultP.setPrinterName(otherP.printerName());
    QVERIFY(defaultP.pageLayout().pageSize().isEquivalentTo(unavailableSizeToSet));
    QVERIFY(defaultP.pageLayout().pageSize().name() != unavailableSizeToSet.name());
    QCOMPARE(defaultP.pageLayout().pageSize().sizePoints(), unavailableSizeToSet.sizePoints());
}

#endif // QT_CONFIG(printer)

QTEST_MAIN(tst_QPrinter)
#include "tst_qprinter.moc"
