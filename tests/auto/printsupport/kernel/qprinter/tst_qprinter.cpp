/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

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

#include <math.h>

#ifdef Q_OS_WIN
#include <windows.h>
#endif


QT_FORWARD_DECLARE_CLASS(QPrinter)

// Helper class to make sure temp files are cleaned up after test complete
class TempFileCleanup
{
public:
    TempFileCleanup(const QString &file)
        : m_file(file)
    {
    }

    ~TempFileCleanup()
    {
        QFile::remove(m_file);
    }
private:
    QString m_file;
};

class tst_QPrinter : public QObject
{
    Q_OBJECT

public slots:
#ifdef QT_NO_PRINTER
    void initTestCase();
    void cleanupTestCase();
#else
private slots:
    void getSetCheck();
// Add your testfunctions and testdata create functions here
#ifdef Q_OS_WIN
    void testPageSize();
    void testNonExistentPrinter();
#endif
    void testPageRectAndPaperRect();
    void testPageRectAndPaperRect_data();
    void testSetOptions();
    void testMargins_data();
    void testMargins();
    void testPageSetupDialog();
    void testPrintPreviewDialog();
    void testMulitpleSets_data();
    void testMulitpleSets();
    void testPageMargins_data();
    void testPageMargins();
    void changingOutputFormat();
    void outputFormatFromSuffix();
    void setGetPaperSize();
    void valuePreservation();
    void errorReporting();
    void testCustomPageSizes();
    void customPaperSizeAndMargins_data();
    void customPaperSizeAndMargins();
#if !defined(QT_NO_COMPLETER) && !defined(QT_NO_FILEDIALOG)
    void printDialogCompleter();
#endif

    void testCopyCount();
    void testCurrentPage();

    void taskQTBUG4497_reusePrinterOnDifferentFiles();
    void testPdfTitle();
    void testPageMetrics_data();
    void testPageMetrics();
#endif
};

#ifdef QT_NO_PRINTER
void tst_QPrinter::initTestCase()
{
    QSKIP("This test requires printing support");
}

void tst_QPrinter::cleanupTestCase()
{
    QSKIP("This test requires printing support");
}
#else
// Testing get/set functions
void tst_QPrinter::getSetCheck()
{
    QPrinter obj1;
    // OutputFormat QPrinter::outputFormat()
    // void QPrinter::setOutputFormat(OutputFormat)
    obj1.setOutputFormat(QPrinter::OutputFormat(QPrinter::PdfFormat));
    QCOMPARE(QPrinter::OutputFormat(QPrinter::PdfFormat), obj1.outputFormat());

    // bool QPrinter::collateCopies()
    // void QPrinter::setCollateCopies(bool)
    obj1.setCollateCopies(false);
    QCOMPARE(false, obj1.collateCopies());
    obj1.setCollateCopies(true);
    QCOMPARE(true, obj1.collateCopies());

    obj1.setColorMode(QPrinter::GrayScale);
    QCOMPARE(obj1.colorMode(), QPrinter::GrayScale);
    obj1.setColorMode(QPrinter::Color);
    QCOMPARE(obj1.colorMode(), QPrinter::Color);

    obj1.setCreator(QString::fromLatin1("RandomQtUser"));
    QCOMPARE(obj1.creator(), QString::fromLatin1("RandomQtUser"));

    obj1.setDocName(QString::fromLatin1("RandomQtDocument"));
    QCOMPARE(obj1.docName(), QString::fromLatin1("RandomQtDocument"));

    obj1.setDoubleSidedPrinting(true);
    QCOMPARE(obj1.doubleSidedPrinting(), true);
    obj1.setDoubleSidedPrinting(false);
    QCOMPARE(obj1.doubleSidedPrinting(), false);

    obj1.setFromTo(1, 4);
    QCOMPARE(obj1.fromPage(), 1);
    QCOMPARE(obj1.toPage(), 4);

    obj1.setFullPage(true);
    QCOMPARE(obj1.fullPage(), true);
    obj1.setFullPage(false);
    QCOMPARE(obj1.fullPage(), false);

    obj1.setOrientation(QPrinter::Landscape);
    QCOMPARE(obj1.orientation(), QPrinter::Landscape);
    obj1.setOrientation(QPrinter::Portrait);
    QCOMPARE(obj1.orientation(), QPrinter::Portrait);

    obj1.setOutputFileName(QString::fromLatin1("RandomQtName"));
    QCOMPARE(obj1.outputFileName(), QString::fromLatin1("RandomQtName"));

    obj1.setPageOrder(QPrinter::FirstPageFirst);
    QCOMPARE(obj1.pageOrder(), QPrinter::FirstPageFirst);
    obj1.setPageOrder(QPrinter::LastPageFirst);
    QCOMPARE(obj1.pageOrder(), QPrinter::LastPageFirst);

    obj1.setPaperSource(QPrinter::Cassette);
    QCOMPARE(obj1.paperSource(), QPrinter::Cassette);
    obj1.setPaperSource(QPrinter::Middle);
    QCOMPARE(obj1.paperSource(), QPrinter::Middle);

#ifdef Q_OS_UNIX
    obj1.setPrintProgram(QString::fromLatin1("/bin/true"));
    QCOMPARE(obj1.printProgram(), QString::fromLatin1("/bin/true"));

    obj1.setPrinterSelectionOption(QString::fromLatin1("--option"));
    QCOMPARE(obj1.printerSelectionOption(), QString::fromLatin1("--option"));
#endif

    obj1.setPrinterName(QString::fromLatin1("myPrinter"));
    QCOMPARE(obj1.printerName(), QString::fromLatin1("myPrinter"));

    // bool QPrinter::fontEmbeddingEnabled()
    // void QPrinter::setFontEmbeddingEnabled(bool)
    obj1.setFontEmbeddingEnabled(false);
    QCOMPARE(false, obj1.fontEmbeddingEnabled());
    obj1.setFontEmbeddingEnabled(true);
    QCOMPARE(true, obj1.fontEmbeddingEnabled());

    // PageSize QPrinter::pageSize()
    // void QPrinter::setPageSize(PageSize)
    obj1.setPageSize(QPrinter::PageSize(QPrinter::A4));
    QCOMPARE(QPrinter::PageSize(QPrinter::A4), obj1.pageSize());
    obj1.setPageSize(QPrinter::PageSize(QPrinter::Letter));
    QCOMPARE(QPrinter::PageSize(QPrinter::Letter), obj1.pageSize());
    obj1.setPageSize(QPrinter::PageSize(QPrinter::Legal));
    QCOMPARE(QPrinter::PageSize(QPrinter::Legal), obj1.pageSize());

    // PrintRange QPrinter::printRange()
    // void QPrinter::setPrintRange(PrintRange)
    obj1.setPrintRange(QPrinter::PrintRange(QPrinter::AllPages));
    QCOMPARE(QPrinter::PrintRange(QPrinter::AllPages), obj1.printRange());
    obj1.setPrintRange(QPrinter::PrintRange(QPrinter::Selection));
    QCOMPARE(QPrinter::PrintRange(QPrinter::Selection), obj1.printRange());
    obj1.setPrintRange(QPrinter::PrintRange(QPrinter::PageRange));
    QCOMPARE(QPrinter::PrintRange(QPrinter::PageRange), obj1.printRange());
}

#define MYCOMPARE(a, b) QCOMPARE(QVariant((int)a), QVariant((int)b))

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

#ifdef Q_OS_WIN
// QPrinter::winPageSize(): Windows only.
void tst_QPrinter::testPageSize()
{
    QPrinter prn;

    prn.setPageSize(QPrinter::Letter);
    MYCOMPARE(prn.pageSize(), QPrinter::Letter);
    MYCOMPARE(prn.winPageSize(), DMPAPER_LETTER);

    prn.setPageSize(QPrinter::A4);
    MYCOMPARE(prn.pageSize(), QPrinter::A4);
    MYCOMPARE(prn.winPageSize(), DMPAPER_A4);

    prn.setWinPageSize(DMPAPER_LETTER);
    MYCOMPARE(prn.winPageSize(), DMPAPER_LETTER);
    MYCOMPARE(prn.pageSize(), QPrinter::Letter);

    prn.setWinPageSize(DMPAPER_A4);
    MYCOMPARE(prn.winPageSize(), DMPAPER_A4);
    MYCOMPARE(prn.pageSize(), QPrinter::A4);
}
#endif // Q_OS_WIN

void tst_QPrinter::testPageRectAndPaperRect_data()
{
    QTest::addColumn<int>("orientation");
    QTest::addColumn<bool>("withPainter");
    QTest::addColumn<int>("resolution");
    QTest::addColumn<bool>("doPaperRect");

    // paperrect
    QTest::newRow("paperRect0") << int(QPrinter::Portrait) << true << 300 << true;
    QTest::newRow("paperRect1") << int(QPrinter::Portrait) << false << 300 << true;
    QTest::newRow("paperRect2") << int(QPrinter::Landscape) << true << 300 << true;
    QTest::newRow("paperRect3") << int(QPrinter::Landscape) << false << 300 << true;
    QTest::newRow("paperRect4") << int(QPrinter::Portrait) << true << 600 << true;
    QTest::newRow("paperRect5") << int(QPrinter::Portrait) << false << 600 << true;
    QTest::newRow("paperRect6") << int(QPrinter::Landscape) << true << 600 << true;
    QTest::newRow("paperRect7") << int(QPrinter::Landscape) << false << 600 << true;
    QTest::newRow("paperRect8") << int(QPrinter::Portrait) << true << 1200 << true;
    QTest::newRow("paperRect9") << int(QPrinter::Portrait) << false << 1200 << true;
    QTest::newRow("paperRect10") << int(QPrinter::Landscape) << true << 1200 << true;
    QTest::newRow("paperRect11") << int(QPrinter::Landscape) << false << 1200 << true;

    // page rect
    QTest::newRow("pageRect0") << int(QPrinter::Portrait) << true << 300 << false;
    QTest::newRow("pageRect1") << int(QPrinter::Portrait) << false << 300 << false;
    QTest::newRow("pageRect2") << int(QPrinter::Landscape) << true << 300 << false;
    QTest::newRow("pageRect3") << int(QPrinter::Landscape) << false << 300 << false;
    QTest::newRow("pageRect4") << int(QPrinter::Portrait) << true << 600 << false;
    QTest::newRow("pageRect5") << int(QPrinter::Portrait) << false << 600 << false;
    QTest::newRow("pageRect6") << int(QPrinter::Landscape) << true << 600 << false;
    QTest::newRow("pageRect7") << int(QPrinter::Landscape) << false << 600 << false;
    QTest::newRow("pageRect8") << int(QPrinter::Portrait) << true << 1200 << false;
    QTest::newRow("pageRect9") << int(QPrinter::Portrait) << false << 1200 << false;
    QTest::newRow("pageRect10") << int(QPrinter::Landscape) << true << 1200 << false;
    QTest::newRow("pageRect11") << int(QPrinter::Landscape) << false << 1200 << false;
}

void tst_QPrinter::testPageRectAndPaperRect()
{
    QFETCH(bool,  withPainter);
    QFETCH(int,  orientation);
    QFETCH(int, resolution);
    QFETCH(bool, doPaperRect);

    QPainter *painter = 0;
    QPrinter printer(QPrinter::HighResolution);
    printer.setOrientation(QPrinter::Orientation(orientation));
    printer.setOutputFileName("silly");
    TempFileCleanup tmpFile("silly");

    QRect pageRect = doPaperRect ? printer.paperRect() : printer.pageRect();
    float inchesX = float(pageRect.width()) / float(printer.resolution());
    float inchesY = float(pageRect.height()) / float(printer.resolution());
    printer.setResolution(resolution);
    if (withPainter)
        painter = new QPainter(&printer);

    QRect otherRect = doPaperRect ? printer.paperRect() : printer.pageRect();
    float otherInchesX = float(otherRect.width()) / float(printer.resolution());
    float otherInchesY = float(otherRect.height()) / float(printer.resolution());
    if (painter != 0)
        delete painter;

    QVERIFY(qAbs(otherInchesX - inchesX) < 0.01);
    QVERIFY(qAbs(otherInchesY - inchesY) < 0.01);

    QVERIFY(printer.orientation() == QPrinter::Portrait || pageRect.width() > pageRect.height());
    QVERIFY(printer.orientation() != QPrinter::Portrait || pageRect.width() < pageRect.height());
}

void tst_QPrinter::testSetOptions()
{
    QPrinter prn;
    QPrintDialog dlg(&prn);

    // Verify default values
    MYCOMPARE(dlg.isOptionEnabled(QAbstractPrintDialog::PrintToFile), true);
    MYCOMPARE(dlg.isOptionEnabled(QAbstractPrintDialog::PrintSelection), false);
    MYCOMPARE(dlg.isOptionEnabled(QAbstractPrintDialog::PrintPageRange), true);

    dlg.setEnabledOptions(QAbstractPrintDialog::PrintPageRange);
    MYCOMPARE(dlg.isOptionEnabled(QAbstractPrintDialog::PrintToFile), false);
    MYCOMPARE(dlg.isOptionEnabled(QAbstractPrintDialog::PrintSelection), false);
    MYCOMPARE(dlg.isOptionEnabled(QAbstractPrintDialog::PrintPageRange), true);

    dlg.setEnabledOptions((QAbstractPrintDialog::PrintDialogOptions(QAbstractPrintDialog::PrintSelection
                                                                    | QAbstractPrintDialog::PrintPageRange)));
    MYCOMPARE(dlg.isOptionEnabled(QAbstractPrintDialog::PrintToFile), false);
    MYCOMPARE(dlg.isOptionEnabled(QAbstractPrintDialog::PrintSelection), true);
    MYCOMPARE(dlg.isOptionEnabled(QAbstractPrintDialog::PrintPageRange), true);

    dlg.setEnabledOptions(QAbstractPrintDialog::PrintSelection);
    MYCOMPARE(dlg.isOptionEnabled(QAbstractPrintDialog::PrintToFile), false);
    MYCOMPARE(dlg.isOptionEnabled(QAbstractPrintDialog::PrintSelection), true);
    MYCOMPARE(dlg.isOptionEnabled(QAbstractPrintDialog::PrintPageRange), false);
}

void tst_QPrinter::testMargins_data()
{
    QTest::addColumn<int>("orientation");
    QTest::addColumn<bool>("fullpage");
    QTest::addColumn<int>("pagesize");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");
    QTest::addColumn<bool>("withPainter");

    QTest::newRow("data0") << int(QPrinter::Portrait) << true << int(QPrinter::A4) << 210 << 297 << false;
    QTest::newRow("data1") << int(QPrinter::Landscape) << true << int(QPrinter::A4) << 297 << 210 << false;
    QTest::newRow("data2") << int(QPrinter::Landscape) << false << int(QPrinter::A4) << 297 << 210 << false;
    QTest::newRow("data3") << int(QPrinter::Portrait) << false << int(QPrinter::A4) << 210 << 297 << false;
    QTest::newRow("data4") << int(QPrinter::Portrait) << true << int(QPrinter::A4) << 210 << 297 << true;
    QTest::newRow("data5") << int(QPrinter::Landscape) << true << int(QPrinter::A4) << 297 << 210 << true;
    QTest::newRow("data6") << int(QPrinter::Landscape) << false << int(QPrinter::A4) << 297 << 210 << true;
    QTest::newRow("data7") << int(QPrinter::Portrait) << false << int(QPrinter::A4) << 210 << 297 << true;
}

void tst_QPrinter::testMargins()
{
    QFETCH(bool,  withPainter);
    QFETCH(int,  orientation);
    QFETCH(int,  pagesize);
    QFETCH(int,  width);
    QFETCH(int,  height);
    QFETCH(bool, fullpage);
    Q_UNUSED(width);
    Q_UNUSED(height);
    QPrinter printer;
    QPainter *painter = 0;
    printer.setOutputFileName("silly");
    printer.setOrientation((QPrinter::Orientation)orientation);
    printer.setFullPage(fullpage);
    printer.setPageSize((QPrinter::PageSize)pagesize);
    if (withPainter)
        painter = new QPainter(&printer);

    if (painter)
        delete painter;
    QFile::remove("silly");
}

#ifdef Q_OS_WIN
// QPrinter::testNonExistentPrinter() is not relevant for this platform
void tst_QPrinter::testNonExistentPrinter()
{
    QPrinter printer;
    QPainter painter;

    // Make sure it doesn't crash on setting or getting properties
    printer.setPrinterName("some non existing printer");
    printer.setPageSize(QPrinter::A4);
    printer.setOrientation(QPrinter::Portrait);
    printer.setFullPage(true);
    printer.pageSize();
    printer.orientation();
    printer.fullPage();
    printer.setCopyCount(1);
    printer.printerName();

    // nor metrics
    QCOMPARE(printer.printEngine()->metric(QPaintDevice::PdmWidth), 0);
    QCOMPARE(printer.printEngine()->metric(QPaintDevice::PdmHeight), 0);
    QCOMPARE(printer.printEngine()->metric(QPaintDevice::PdmWidthMM), 0);
    QCOMPARE(printer.printEngine()->metric(QPaintDevice::PdmHeightMM), 0);
    QCOMPARE(printer.printEngine()->metric(QPaintDevice::PdmNumColors), 0);
    QCOMPARE(printer.printEngine()->metric(QPaintDevice::PdmDepth), 0);
    QCOMPARE(printer.printEngine()->metric(QPaintDevice::PdmDpiX), 0);
    QCOMPARE(printer.printEngine()->metric(QPaintDevice::PdmDpiY), 0);
    QCOMPARE(printer.printEngine()->metric(QPaintDevice::PdmPhysicalDpiX), 0);
    QCOMPARE(printer.printEngine()->metric(QPaintDevice::PdmPhysicalDpiY), 0);

    QVERIFY(!painter.begin(&printer));
}
#endif

void tst_QPrinter::testMulitpleSets_data()
{
    QTest::addColumn<int>("resolution");
    QTest::addColumn<int>("pageSize");
    QTest::addColumn<int>("widthMMAfter");
    QTest::addColumn<int>("heightMMAfter");


    QTest::newRow("lowRes") << int(QPrinter::ScreenResolution) << int(QPrinter::A4) << 210 << 297;
    QTest::newRow("lowResLetter") << int(QPrinter::ScreenResolution) << int(QPrinter::Letter) << 216 << 279;
    QTest::newRow("lowResA5") << int(QPrinter::ScreenResolution) << int(QPrinter::A5) << 148 << 210;
    QTest::newRow("midRes") << int(QPrinter::PrinterResolution) << int(QPrinter::A4) << 210 << 297;
    QTest::newRow("midResLetter") << int(QPrinter::PrinterResolution) << int(QPrinter::Letter) << 216 << 279;
    QTest::newRow("midResA5") << int(QPrinter::PrinterResolution) << int(QPrinter::A5) << 148 << 210;
    QTest::newRow("highRes") << int(QPrinter::HighResolution) << int(QPrinter::A4) << 210 << 297;
    QTest::newRow("highResLetter") << int(QPrinter::HighResolution) << int(QPrinter::Letter) << 216 << 279;
    QTest::newRow("highResA5") << int(QPrinter::HighResolution) << int(QPrinter::A5) << 148 << 210;
}

static void computePageValue(const QPrinter &printer, int &retWidth, int &retHeight)
{
    const double Inch2MM = 25.4;

    double width = double(printer.paperRect().width()) / printer.logicalDpiX() * Inch2MM;
    double height = double(printer.paperRect().height()) / printer.logicalDpiY() * Inch2MM;
    retWidth = qRound(width);
    retHeight = qRound(height);
}

void tst_QPrinter::testMulitpleSets()
{
    // A very simple test, but Mac needs to have its format "validated" if the format is changed
    // This takes care of that.
    QFETCH(int, resolution);
    QFETCH(int, pageSize);
    QFETCH(int, widthMMAfter);
    QFETCH(int, heightMMAfter);


    QPrinter::PrinterMode mode = QPrinter::PrinterMode(resolution);
    QPrinter::PageSize printerPageSize = QPrinter::PageSize(pageSize);
    QPrinter printer(mode);
    printer.setFullPage(true);

    int paperWidth, paperHeight;
    //const int Tolerance = 2;

    computePageValue(printer, paperWidth, paperHeight);
    printer.setPageSize(printerPageSize);

    if (printer.pageSize() != printerPageSize) {
        QSKIP("Current page size is not supported on this printer");
        return;
    }

    QVERIFY(qAbs(printer.widthMM() - widthMMAfter) <= 2);
    QVERIFY(qAbs(printer.heightMM() - heightMMAfter) <= 2);

    computePageValue(printer, paperWidth, paperHeight);

    QVERIFY(qAbs(paperWidth - widthMMAfter) <= 2);
    QVERIFY(qAbs(paperHeight - heightMMAfter) <= 2);

    // Set it again and see if it still works.
    printer.setPageSize(printerPageSize);
    QVERIFY(qAbs(printer.widthMM() - widthMMAfter) <= 2);
    QVERIFY(qAbs(printer.heightMM() - heightMMAfter) <= 2);

    printer.setOrientation(QPrinter::Landscape);
    computePageValue(printer, paperWidth, paperHeight);
    QVERIFY(qAbs(paperWidth - heightMMAfter) <= 2);
    QVERIFY(qAbs(paperHeight - widthMMAfter) <= 2);
}

void tst_QPrinter::changingOutputFormat()
{
#if QT_VERSION < 0x050000
    QPrinter p;
    p.setOutputFormat(QPrinter::PostScriptFormat);
    p.setPageSize(QPrinter::A8);
    p.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(p.pageSize(), QPrinter::A8);
#endif
}

void tst_QPrinter::outputFormatFromSuffix()
{
    if (QPrinterInfo::availablePrinters().size() == 0)
        QSKIP("No printers available.");
    QPrinter p;
    QVERIFY(p.outputFormat() == QPrinter::NativeFormat);
    p.setOutputFileName("test.pdf");
    TempFileCleanup tmpFile("test.pdf");
    QVERIFY(p.outputFormat() == QPrinter::PdfFormat);
    p.setOutputFileName(QString());
    QVERIFY(p.outputFormat() == QPrinter::NativeFormat);
}

void tst_QPrinter::setGetPaperSize()
{
    QPrinter p;
    p.setOutputFormat(QPrinter::PdfFormat);
    QSizeF size(500, 10);
    p.setPaperSize(size, QPrinter::Millimeter);
    QCOMPARE(p.paperSize(QPrinter::Millimeter), size);
    QSizeF ptSize = p.paperSize(QPrinter::Point);
    //qDebug() << ptSize;
    QVERIFY(qAbs(ptSize.width() - size.width() * (72/25.4)) < 1E-4);
    QVERIFY(qAbs(ptSize.height() - size.height() * (72/25.4)) < 1E-4);
}

void tst_QPrinter::testPageMargins_data()
{
    QTest::addColumn<qreal>("left");
    QTest::addColumn<qreal>("top");
    QTest::addColumn<qreal>("right");
    QTest::addColumn<qreal>("bottom");
    QTest::addColumn<int>("unit");

    QTest::newRow("data0") << qreal(5.5) << qreal(6.5) << qreal(7.5) << qreal(8.5) << static_cast<int>(QPrinter::Millimeter);
    QTest::newRow("data1") << qreal(5.5) << qreal(6.5) << qreal(7.5) << qreal(8.5) << static_cast<int>(QPrinter::Point);
    QTest::newRow("data2") << qreal(5.5) << qreal(6.5) << qreal(7.5) << qreal(8.5) << static_cast<int>(QPrinter::Inch);
    QTest::newRow("data3") << qreal(5.5) << qreal(6.5) << qreal(7.5) << qreal(8.5) << static_cast<int>(QPrinter::Pica);
    QTest::newRow("data4") << qreal(5.5) << qreal(6.5) << qreal(7.5) << qreal(8.5) << static_cast<int>(QPrinter::Didot);
    QTest::newRow("data5") << qreal(5.5) << qreal(6.5) << qreal(7.5) << qreal(8.5) << static_cast<int>(QPrinter::Cicero);
}

void tst_QPrinter::testPageMargins()
{
    QPrinter obj1;

    qreal toMillimeters[6];
    toMillimeters[QPrinter::Millimeter] = 1;
    toMillimeters[QPrinter::Point] = 0.352777778;
    toMillimeters[QPrinter::Inch] = 25.4;
    toMillimeters[QPrinter::Pica] = 4.23333333;
    toMillimeters[QPrinter::Didot] = 0.376;
    toMillimeters[QPrinter::Cicero] = 4.51166667;

    QFETCH(qreal, left);
    QFETCH(qreal, top);
    QFETCH(qreal, right);
    QFETCH(qreal, bottom);
    QFETCH(int, unit);

    qreal nLeft, nTop, nRight, nBottom;

    obj1.setPageMargins(left, top, right, bottom, static_cast<QPrinter::Unit>(unit));

    qreal tolerance = 0.05;

    obj1.getPageMargins(&nLeft, &nTop, &nRight, &nBottom, QPrinter::Millimeter);
    QVERIFY(fabs(left*toMillimeters[unit] - nLeft*toMillimeters[QPrinter::Millimeter]) < tolerance);
    QVERIFY(fabs(top*toMillimeters[unit] - nTop*toMillimeters[QPrinter::Millimeter]) < tolerance);
    QVERIFY(fabs(right*toMillimeters[unit] - nRight*toMillimeters[QPrinter::Millimeter]) < tolerance);
    QVERIFY(fabs(bottom*toMillimeters[unit] - nBottom*toMillimeters[QPrinter::Millimeter]) < tolerance);

    obj1.getPageMargins(&nLeft, &nTop, &nRight, &nBottom, QPrinter::Point);
    QVERIFY(fabs(left*toMillimeters[unit] - nLeft*toMillimeters[QPrinter::Point]) < tolerance);
    QVERIFY(fabs(top*toMillimeters[unit] - nTop*toMillimeters[QPrinter::Point]) < tolerance);
    QVERIFY(fabs(right*toMillimeters[unit] - nRight*toMillimeters[QPrinter::Point]) < tolerance);
    QVERIFY(fabs(bottom*toMillimeters[unit] - nBottom*toMillimeters[QPrinter::Point]) < tolerance);

    obj1.getPageMargins(&nLeft, &nTop, &nRight, &nBottom, QPrinter::Inch);
    QVERIFY(fabs(left*toMillimeters[unit] - nLeft*toMillimeters[QPrinter::Inch]) < tolerance);
    QVERIFY(fabs(top*toMillimeters[unit] - nTop*toMillimeters[QPrinter::Inch]) < tolerance);
    QVERIFY(fabs(right*toMillimeters[unit] - nRight*toMillimeters[QPrinter::Inch]) < tolerance);
    QVERIFY(fabs(bottom*toMillimeters[unit] - nBottom*toMillimeters[QPrinter::Inch]) < tolerance);

    obj1.getPageMargins(&nLeft, &nTop, &nRight, &nBottom, QPrinter::Pica);
    QVERIFY(fabs(left*toMillimeters[unit] - nLeft*toMillimeters[QPrinter::Pica]) < tolerance);
    QVERIFY(fabs(top*toMillimeters[unit] - nTop*toMillimeters[QPrinter::Pica]) < tolerance);
    QVERIFY(fabs(right*toMillimeters[unit] - nRight*toMillimeters[QPrinter::Pica]) < tolerance);
    QVERIFY(fabs(bottom*toMillimeters[unit] - nBottom*toMillimeters[QPrinter::Pica]) < tolerance);

    obj1.getPageMargins(&nLeft, &nTop, &nRight, &nBottom, QPrinter::Didot);
    QVERIFY(fabs(left*toMillimeters[unit] - nLeft*toMillimeters[QPrinter::Didot]) < tolerance);
    QVERIFY(fabs(top*toMillimeters[unit] - nTop*toMillimeters[QPrinter::Didot]) < tolerance);
    QVERIFY(fabs(right*toMillimeters[unit] - nRight*toMillimeters[QPrinter::Didot]) < tolerance);
    QVERIFY(fabs(bottom*toMillimeters[unit] - nBottom*toMillimeters[QPrinter::Didot]) < tolerance);

    obj1.getPageMargins(&nLeft, &nTop, &nRight, &nBottom, QPrinter::Cicero);
    QVERIFY(fabs(left*toMillimeters[unit] - nLeft*toMillimeters[QPrinter::Cicero]) < tolerance);
    QVERIFY(fabs(top*toMillimeters[unit] - nTop*toMillimeters[QPrinter::Cicero]) < tolerance);
    QVERIFY(fabs(right*toMillimeters[unit] - nRight*toMillimeters[QPrinter::Cicero]) < tolerance);
    QVERIFY(fabs(bottom*toMillimeters[unit] - nBottom*toMillimeters[QPrinter::Cicero]) < tolerance);
}

void tst_QPrinter::valuePreservation()
{
    QPrinter::OutputFormat oldFormat = QPrinter::PdfFormat;
    QPrinter::OutputFormat newFormat = QPrinter::NativeFormat; // TODO: Correct?

    // Some properties are documented to only be supported by NativeFormat in X11 environment
    bool doX11Tests = QGuiApplication::platformName().compare(QLatin1String("xcb"), Qt::CaseInsensitive) == 0;
    bool windowsPlatform = QGuiApplication::platformName().compare(QLatin1String("windows"), Qt::CaseInsensitive) == 0;
    bool manualSourceSupported = true;

#ifdef Q_OS_WIN
    // QPrinter::supportedPaperSources() is only available on Windows, so just assuming manual is supported on others.
    QPrinter printer;
    printer.setOutputFormat(newFormat);
    QList<QPrinter::PaperSource> sources = printer.supportedPaperSources();
    if (!sources.contains(QPrinter::Manual)) {
        manualSourceSupported = false;
        qWarning() << "Manual paper source not supported by native printer, skipping related test.";
    }
#endif // Q_OS_WIN

    // Querying PPK_CollateCopies is hardcoded to return false with Windows native print engine,
    // so skip testing that in Windows.
    if (!windowsPlatform) {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        bool status = printer.collateCopies();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.collateCopies(), status);

        printer.setCollateCopies(!status);
        printer.setOutputFormat(newFormat);
#ifdef Q_OS_MAC
        QEXPECT_FAIL("","QTBUG-26430", Abort);
#endif
        QCOMPARE(printer.collateCopies(), !status);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.collateCopies(), !status);
    }
    {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        QPrinter::ColorMode status = printer.colorMode();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.colorMode(), status);

        printer.setColorMode(QPrinter::ColorMode(!status));
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.colorMode(), QPrinter::ColorMode(!status));
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.colorMode(), QPrinter::ColorMode(!status));
    }
    if (doX11Tests) {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        QString status = printer.creator();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.creator(), status);

        status = QString::fromLatin1("Mr. Test");
        printer.setCreator(status);
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.creator(), status);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.creator(), status);
    }
    {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        QString status = printer.docName();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.docName(), status);

        status = QString::fromLatin1("Test document");
        printer.setDocName(status);
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.docName(), status);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.docName(), status);
    }
    if (doX11Tests) {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        bool status = printer.doubleSidedPrinting();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.doubleSidedPrinting(), status);

        printer.setDoubleSidedPrinting(!status);
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.doubleSidedPrinting(), !status);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.doubleSidedPrinting(), !status);
    }
    if (doX11Tests) {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        bool status = printer.fontEmbeddingEnabled();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.fontEmbeddingEnabled(), status);

        printer.setFontEmbeddingEnabled(!status);
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.fontEmbeddingEnabled(), !status);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.fontEmbeddingEnabled(), !status);
    }
    {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        bool status = printer.fullPage();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.fullPage(), status);

        printer.setFullPage(!status);
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.fullPage(), !status);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.fullPage(), !status);
    }
    {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        QPrinter::Orientation status = printer.orientation();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.orientation(), status);

        printer.setOrientation(QPrinter::Orientation(!status));
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.orientation(), QPrinter::Orientation(!status));
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.orientation(), QPrinter::Orientation(!status));
    }
    {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        QString status = printer.outputFileName();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.outputFileName(), status);

        status = QString::fromLatin1("Test file");
        printer.setOutputFileName(status);
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.outputFileName(), status);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.outputFileName(), status);
    }
    if (doX11Tests) {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        QPrinter::PageOrder status = printer.pageOrder();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.pageOrder(), status);

        printer.setPageOrder(QPrinter::PageOrder(!status));
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.pageOrder(), QPrinter::PageOrder(!status));
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.pageOrder(), QPrinter::PageOrder(!status));
    }
    {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        QPrinter::PageSize status = printer.pageSize();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.pageSize(), status);

        printer.setPageSize(QPrinter::B5);
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.pageSize(), QPrinter::B5);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.pageSize(), QPrinter::B5);
    }
    if (manualSourceSupported) {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        QPrinter::PaperSource status = printer.paperSource();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.paperSource(), status);

        printer.setPaperSource(QPrinter::Manual);
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.paperSource(), QPrinter::Manual);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.paperSource(), QPrinter::Manual);
    }
    if (doX11Tests) {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        QString status = printer.printProgram();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.printProgram(), status);

        status = QString::fromLatin1("/usr/local/bin/lpr");
        printer.setPrintProgram(status);
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.printProgram(), status);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.printProgram(), status);
    }
    {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        QPrinter::PrintRange status = printer.printRange();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.printRange(), status);

        printer.setPrintRange(QPrinter::PrintRange(!status));
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.printRange(), QPrinter::PrintRange(!status));
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.printRange(), QPrinter::PrintRange(!status));
    }
    {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        QString status = printer.printerName();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.printerName(), status);

        status = QString::fromLatin1("SuperDuperPrinter");
        printer.setPrinterName(status);
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.printerName(), status);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.printerName(), status);
    }
    // QPrinter::printerSelectionOption is explicitly documented not to be available on Windows.
#ifndef Q_OS_WIN
    {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        QString status = printer.printerSelectionOption();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.printerSelectionOption(), status);

        status = QString::fromLatin1("Optional option");
        printer.setPrinterSelectionOption(status);
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.printerSelectionOption(), status);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.printerSelectionOption(), status);
    }
#endif // Q_OS_WIN
    {
        QPrinter printer;
        printer.setOutputFormat(oldFormat);
        int status = printer.resolution();
        printer.setOutputFormat(newFormat);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.resolution(), status);

        printer.setResolution(status-150);
        printer.setOutputFormat(newFormat);
        QCOMPARE(printer.resolution(), status-150);
        printer.setOutputFormat(oldFormat);
        QCOMPARE(printer.resolution(), status-150);
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
    p.setOutputFileName("test.pdf");
    TempFileCleanup tmpFile("test.pdf");
    QCOMPARE(painter.begin(&p), true); // it should check the output
    QCOMPARE(p.isValid(), true);
    painter.end();
}

void tst_QPrinter::testCustomPageSizes()
{
    QPrinter p;

    QSizeF customSize(8.5, 11.0);
    p.setPaperSize(customSize, QPrinter::Inch);

    QSizeF paperSize = p.paperSize(QPrinter::Inch);
    QCOMPARE(paperSize, customSize);

    QPrinter p2(QPrinter::HighResolution);
    p2.setPaperSize(customSize, QPrinter::Inch);
    paperSize = p.paperSize(QPrinter::Inch);
    QCOMPARE(paperSize, customSize);
}

void tst_QPrinter::customPaperSizeAndMargins_data()
{
    QTest::addColumn<bool>("pdf");
    QTest::addColumn<bool>("before");
    QTest::addColumn<qreal>("left");
    QTest::addColumn<qreal>("top");
    QTest::addColumn<qreal>("right");
    QTest::addColumn<qreal>("bottom");

    QTest::newRow("beforeNoPDF") << false << true << qreal(2) << qreal(2) << qreal(2) << qreal(2);
    QTest::newRow("beforePDF") << true << true << qreal(2) << qreal(2) << qreal(2) << qreal(2);
    QTest::newRow("afterNoPDF") << false << false << qreal(2) << qreal(2) << qreal(2) << qreal(2);
    QTest::newRow("afterAfterPDF") << true << false << qreal(2) << qreal(2) << qreal(2) << qreal(2);
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
    qreal getLeft = 0;
    qreal getRight = 0;
    qreal getTop = 0;
    qreal getBottom = 0;
    QSizeF customSize(8.5, 11.0);

    QPrinter p;
    if (pdf)
        p.setOutputFormat(QPrinter::PdfFormat);
    if (before)
        p.setPageMargins(left, top, right, bottom, QPrinter::Millimeter);
    p.setPaperSize(customSize, QPrinter::Millimeter);
    p.getPageMargins(&getLeft, &getTop, &getRight, &getBottom, QPrinter::Millimeter);
    if (before) {
        QVERIFY(fabs(left - getLeft) < tolerance);
        QVERIFY(fabs(left - getTop) < tolerance);
        QVERIFY(fabs(left - getRight) < tolerance);
        QVERIFY(fabs(left - getBottom) < tolerance);
    } else {
        QVERIFY(getLeft == 0);
        QVERIFY(getTop == 0);
        QVERIFY(getRight == 0);
        QVERIFY(getBottom == 0);
        p.setPageMargins(left, top, right, bottom, QPrinter::Millimeter);
        p.getPageMargins(&getLeft, &getTop, &getRight, &getBottom, QPrinter::Millimeter);
        QVERIFY(fabs(left - getLeft) < tolerance);
        QVERIFY(fabs(left - getTop) < tolerance);
        QVERIFY(fabs(left - getRight) < tolerance);
        QVERIFY(fabs(left - getBottom) < tolerance);
    }
}

#if !defined(QT_NO_COMPLETER) && !defined(QT_NO_FILEDIALOG)
void tst_QPrinter::printDialogCompleter()
{
    QPrintDialog dialog;
    dialog.printer()->setOutputFileName("file.pdf");
    TempFileCleanup tmpFile("file.pdf");
    dialog.setEnabledOptions(QAbstractPrintDialog::PrintToFile);
    dialog.show();

    QTest::qWait(100);

    QTest::keyClick(&dialog, Qt::Key_Tab);
    QTest::keyClick(&dialog, 'P');
    // The test passes if it doesn't crash.
}
#endif

void tst_QPrinter::testCopyCount()
{
    QPrinter p;
    p.setCopyCount(15);
    QCOMPARE(p.copyCount(), 15);
}

static void printPage(QPainter *painter)
{
    painter->setPen(QPen(Qt::black, 4));
    painter->drawRect(50, 60, 70, 80);
}

void tst_QPrinter::taskQTBUG4497_reusePrinterOnDifferentFiles()
{
    TempFileCleanup tmpFile1("out1.pdf");
    TempFileCleanup tmpFile2("out2.pdf");

    QPrinter printer;
    {

        printer.setOutputFileName("out1.pdf");
        QPainter painter(&printer);
        printPage(&painter);

    }
    {

        printer.setOutputFileName("out2.pdf");
        QPainter painter(&printer);
        printPage(&painter);

    }
    QFile file1("out1.pdf");
    QVERIFY(file1.open(QIODevice::ReadOnly));

    QFile file2("out2.pdf");
    QVERIFY(file2.open(QIODevice::ReadOnly));

    while (!file1.atEnd() && !file2.atEnd()) {
        QByteArray file1Line = file1.readLine();
        QByteArray file2Line = file2.readLine();

        if (!file1Line.startsWith("%%CreationDate"))
            QCOMPARE(file1Line, file2Line);
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
    QCOMPARE(dialog.isOptionEnabled(QPrintDialog::PrintCurrentPage), false);

    // Test enable Current Page option
    dialog.setOption(QPrintDialog::PrintCurrentPage);
    QCOMPARE(dialog.isOptionEnabled(QPrintDialog::PrintCurrentPage), true);

}

void tst_QPrinter::testPdfTitle()
{
    // Check the document name is represented correctly in produced pdf
    {
        QPainter painter;
        QPrinter printer;
        // This string is just the UTF-8 encoding of the string: \()f &oslash; hiragana o
        const unsigned char titleBuf[]={0x5c, 0x28, 0x29, 0x66, 0xc3, 0xb8, 0xe3, 0x81, 0x8a, 0x00};
        const char *title = reinterpret_cast<const char*>(titleBuf);
        printer.setOutputFileName("file.pdf");
        printer.setDocName(QString::fromUtf8(title));
        painter.begin(&printer);
        painter.end();
    }
    TempFileCleanup tmpFile("file.pdf");
    QFile file("file.pdf");
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

void tst_QPrinter::testPageMetrics_data()
{
    QTest::addColumn<int>("pageSize");
    QTest::addColumn<int>("widthMM");
    QTest::addColumn<int>("heightMM");
    QTest::addColumn<float>("widthMMf");
    QTest::addColumn<float>("heightMMf");

    QTest::newRow("A4")     << int(QPrinter::A4)     << 210 << 297 << 210.0f << 297.0f;
    QTest::newRow("A5")     << int(QPrinter::A5)     << 148 << 210 << 148.0f << 210.0f;
    QTest::newRow("Letter") << int(QPrinter::Letter) << 216 << 279 << 215.9f << 279.4f;
}

void tst_QPrinter::testPageMetrics()
{
    QFETCH(int, pageSize);
    QFETCH(int, widthMM);
    QFETCH(int, heightMM);
    QFETCH(float, widthMMf);
    QFETCH(float, heightMMf);

    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    printer.setPageSize(QPrinter::PageSize(pageSize));

    if (printer.pageSize() != pageSize) {
        QSKIP("Current page size is not supported on this printer");
        return;
    }

    QCOMPARE(printer.widthMM(), int(widthMM));
    QCOMPARE(printer.heightMM(), int(heightMM));
    QCOMPARE(printer.pageSizeMM(), QSizeF(widthMMf, heightMMf));
}

#endif // QT_NO_PRINTER

QTEST_MAIN(tst_QPrinter)
#include "tst_qprinter.moc"
