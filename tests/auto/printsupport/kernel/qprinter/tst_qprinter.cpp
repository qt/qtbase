/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <qpagelayout.h>

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
    void outputFormatFromSuffix();
    void errorReporting();
    void testCustomPageSizes();
    void customPaperSizeAndMargins_data();
    void customPaperSizeAndMargins();
    void customPaperNameSettingBySize();
    void customPaperNameSettingByName();
#if !defined(QT_NO_COMPLETER) && !defined(QT_NO_FILEDIALOG)
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
    void doubleSidedPrinting();
    void duplex();
    void fontEmbedding();
    void fullPage();
    void orientation();
    void outputFileName();
    void pageOrder();
    void pageSize();
    void paperSize();
    void paperSource();
    void printerName();
    void printerSelectionOption();
    void printProgram();
    void printRange();
    void resolution();
    void supportedPaperSources();
    void supportedResolutions();
    void windowsPageSize();

    // Test QPrinter setters/getters for non-QPrintEngine options
    void outputFormat();
    void fromToPage();

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

void tst_QPrinter::outputFormatFromSuffix()
{
    if (QPrinterInfo::availablePrinters().size() == 0)
        QSKIP("No printers available.");
    QPrinter p;
    QCOMPARE(p.outputFormat(), QPrinter::NativeFormat);
    p.setOutputFileName("test.pdf");
    TempFileCleanup tmpFile("test.pdf");
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
    QTest::addColumn<int>("unit");

    // Use custom margins that will exceed most printers minimum allowed
    QTest::newRow("data0") << qreal(25.5) << qreal(26.5) << qreal(27.5) << qreal(28.5) << static_cast<int>(QPrinter::Millimeter);
    QTest::newRow("data1") << qreal(55.5) << qreal(56.5) << qreal(57.5) << qreal(58.5) << static_cast<int>(QPrinter::Point);
    QTest::newRow("data2") << qreal(5.5) << qreal(6.5) << qreal(7.5) << qreal(8.5) << static_cast<int>(QPrinter::Inch);
    QTest::newRow("data3") << qreal(5.5) << qreal(6.5) << qreal(7.5) << qreal(8.5) << static_cast<int>(QPrinter::Pica);
    QTest::newRow("data4") << qreal(55.5) << qreal(56.5) << qreal(57.5) << qreal(58.5) << static_cast<int>(QPrinter::Didot);
    QTest::newRow("data5") << qreal(5.5) << qreal(6.5) << qreal(7.5) << qreal(8.5) << static_cast<int>(QPrinter::Cicero);
}

void tst_QPrinter::testPageMargins()
{
    QPrinter obj1;

    QFETCH(qreal, left);
    QFETCH(qreal, top);
    QFETCH(qreal, right);
    QFETCH(qreal, bottom);
    QFETCH(int, unit);

    QPageLayout layout = QPageLayout(QPageSize(QPageSize::A0), QPageLayout::Portrait,
                                     QMarginsF(left, top, right, bottom), QPageLayout::Unit(unit));

    qreal nLeft, nTop, nRight, nBottom;

    obj1.setPageMargins(left, top, right, bottom, QPrinter::Unit(unit));

    obj1.getPageMargins(&nLeft, &nTop, &nRight, &nBottom, QPrinter::Millimeter);
    QCOMPARE(nLeft, layout.margins(QPageLayout::Millimeter).left());
    QCOMPARE(nRight, layout.margins(QPageLayout::Millimeter).right());
    QCOMPARE(nTop, layout.margins(QPageLayout::Millimeter).top());
    QCOMPARE(nBottom, layout.margins(QPageLayout::Millimeter).bottom());

    obj1.getPageMargins(&nLeft, &nTop, &nRight, &nBottom, QPrinter::Point);
    QCOMPARE(nLeft, layout.margins(QPageLayout::Point).left());
    QCOMPARE(nRight, layout.margins(QPageLayout::Point).right());
    QCOMPARE(nTop, layout.margins(QPageLayout::Point).top());
    QCOMPARE(nBottom, layout.margins(QPageLayout::Point).bottom());

    obj1.getPageMargins(&nLeft, &nTop, &nRight, &nBottom, QPrinter::Inch);
    QCOMPARE(nLeft, layout.margins(QPageLayout::Inch).left());
    QCOMPARE(nRight, layout.margins(QPageLayout::Inch).right());
    QCOMPARE(nTop, layout.margins(QPageLayout::Inch).top());
    QCOMPARE(nBottom, layout.margins(QPageLayout::Inch).bottom());

    obj1.getPageMargins(&nLeft, &nTop, &nRight, &nBottom, QPrinter::Pica);
    QCOMPARE(nLeft, layout.margins(QPageLayout::Pica).left());
    QCOMPARE(nRight, layout.margins(QPageLayout::Pica).right());
    QCOMPARE(nTop, layout.margins(QPageLayout::Pica).top());
    QCOMPARE(nBottom, layout.margins(QPageLayout::Pica).bottom());

    obj1.getPageMargins(&nLeft, &nTop, &nRight, &nBottom, QPrinter::Didot);
    QCOMPARE(nLeft, layout.margins(QPageLayout::Didot).left());
    QCOMPARE(nRight, layout.margins(QPageLayout::Didot).right());
    QCOMPARE(nTop, layout.margins(QPageLayout::Didot).top());
    QCOMPARE(nBottom, layout.margins(QPageLayout::Didot).bottom());

    obj1.getPageMargins(&nLeft, &nTop, &nRight, &nBottom, QPrinter::Cicero);
    QCOMPARE(nLeft, layout.margins(QPageLayout::Cicero).left());
    QCOMPARE(nRight, layout.margins(QPageLayout::Cicero).right());
    QCOMPARE(nTop, layout.margins(QPageLayout::Cicero).top());
    QCOMPARE(nBottom, layout.margins(QPageLayout::Cicero).bottom());
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

    QSizeF customSize(7.0, 11.0);
    p.setPaperSize(customSize, QPrinter::Inch);

    QSizeF paperSize = p.paperSize(QPrinter::Inch);
    QCOMPARE(paperSize.width(), customSize.width());
    QCOMPARE(paperSize.height(), customSize.height());

    QPrinter p2(QPrinter::HighResolution);
    p2.setPaperSize(customSize, QPrinter::Inch);
    paperSize = p.paperSize(QPrinter::Inch);
    QCOMPARE(paperSize.width(), customSize.width());
    QCOMPARE(paperSize.height(), customSize.height());

    const QSizeF sizeInPixels = p.paperSize(QPrinter::DevicePixel);
    QPrinter p3;
    p3.setPaperSize(sizeInPixels, QPrinter::DevicePixel);
    paperSize = p3.paperSize(QPrinter::Inch);
    QCOMPARE(paperSize.width(), customSize.width());
    QCOMPARE(paperSize.height(), customSize.height());
    QPageSize pageSize = p3.pageLayout().pageSize();
    QCOMPARE(pageSize.key(), QString("Custom.504x792"));
    QCOMPARE(pageSize.name(), QString("Custom (504pt x 792pt)"));
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
    qreal getLeft = 0;
    qreal getRight = 0;
    qreal getTop = 0;
    qreal getBottom = 0;
    // Use a custom page size that most printers should support, A4 is 210x297
    // TODO Use print device api when available
    QSizeF customSize(200.0, 300.0);

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

void tst_QPrinter::customPaperNameSettingBySize()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrinterInfo info(printer);
    QList<QPageSize> sizes = info.supportedPageSizes();
    if (sizes.size() == 0)
        QSKIP("No printers installed on this machine");
    for (int i=0; i<sizes.size(); i++) {
        printer.setPaperSize(sizes.at(i).size(QPageSize::Millimeter), QPrinter::Millimeter);
        QCOMPARE(sizes.at(i).size(QPageSize::Millimeter), printer.paperSize(QPrinter::Millimeter));
        // Some printers have the same size under different names which can cause a problem for the test
        // So we look at all the other sizes to see if one also matches as we don't know which order they are in
        QSizeF paperSize = sizes.at(i).size(QPageSize::Millimeter);
        QString paperName = printer.paperName();
        bool paperNameFound = (sizes.at(i).name() == paperName);
        if (!paperNameFound) {
            for (int j = 0; j < sizes.size(); ++j) {
                if (j != i
                    && sizes.at(j).size(QPageSize::Millimeter) == paperSize
                    && sizes.at(j).name() == paperName) {
                    paperNameFound = true;
                    break;
                }
            }
        }
        // Fail with the original values
        if (!paperNameFound) {
            qDebug() << "supportedPageSizes() = " << sizes;
            QEXPECT_FAIL("", "Paper Name mismatch: please report this failure at bugreports.qt.io", Continue);
            QCOMPARE(sizes.at(i).name(), printer.paperName());
        }
    }

    // Check setting a custom size after setting a standard one works
    QSizeF customSize(200, 300);
    printer.setPaperSize(customSize, QPrinter::Millimeter);
    QCOMPARE(printer.paperSize(QPrinter::Millimeter), customSize);
    QCOMPARE(printer.paperSize(), QPrinter::Custom);

    // Finally check setting a standard size after a custom one works
    printer.setPaperSize(sizes.at(0).size(QPageSize::Millimeter), QPrinter::Millimeter);
    QCOMPARE(printer.paperName(), sizes.at(0).name());
    QCOMPARE(printer.paperSize(QPrinter::Millimeter), sizes.at(0).size(QPageSize::Millimeter));
}

void tst_QPrinter::customPaperNameSettingByName()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrinterInfo info(printer);
    QList<QPageSize> sizes = info.supportedPageSizes();
    if (sizes.size() == 0)
        QSKIP("No printers installed on this machine");
    for (int i=0; i<sizes.size(); i++) {
        printer.setPaperName(sizes.at(i).name());
        QCOMPARE(sizes.at(i).name(), printer.paperName());
        QCOMPARE(sizes.at(i).size(QPageSize::Millimeter), printer.paperSize(QPrinter::Millimeter));
    }
}

// Test QPrintEngine keys and their QPrinter setters/getters

void tst_QPrinter::testMultipleKeys()
{
    // Tests multiple keys preservation, note are only ones that are consistent across all engines

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Check default values
        QCOMPARE(native.fullPage(), false);
        QCOMPARE(native.orientation(), QPrinter::Portrait);
        QCOMPARE(native.copyCount(), 1);
        QCOMPARE(native.collateCopies(), true);
        QCOMPARE(native.printRange(), QPrinter::AllPages);

        // Change values
        native.setFullPage(true);
        native.setOrientation(QPrinter::Landscape);
        native.setCopyCount(9);
        native.setCollateCopies(false);
        native.setPrintRange(QPrinter::CurrentPage);

        // Check changed values
        QCOMPARE(native.fullPage(), true);
        QCOMPARE(native.orientation(), QPrinter::Landscape);
        QCOMPARE(native.copyCount(), 9);
        QCOMPARE(native.collateCopies(), false);
        QCOMPARE(native.printRange(), QPrinter::CurrentPage);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.fullPage(), true);
        QCOMPARE(native.orientation(), QPrinter::Landscape);
        QCOMPARE(native.copyCount(), 9);
        QCOMPARE(native.collateCopies(), false);
        QCOMPARE(native.printRange(), QPrinter::CurrentPage);

        // Change values
        native.setFullPage(false);
        native.setOrientation(QPrinter::Portrait);
        native.setCopyCount(5);
        native.setCollateCopies(true);
        native.setPrintRange(QPrinter::PageRange);

        // Check changed values
        QCOMPARE(native.fullPage(), false);
        QCOMPARE(native.orientation(), QPrinter::Portrait);
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
    // copyCount() / setCopyCount() / PPK_CopyCount
    // numCopies() / setNumCopies() / PPK_NumberOfCopies
    // actualNumCopies() / supportsMultipleCopies()
    // PdfFormat: Supported, multiple copies unsupported, default 1
    // NativeFormat, Cups: Supported, multiple copies supported, default 1
    // NativeFormat, Win: Supported, multiple copies supported, default 1
    // NativeFormat, Mac: Supported, multiple copies supported, default 1

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.supportsMultipleCopies(), false);
    QCOMPARE(pdf.copyCount(), 1);
    QCOMPARE(pdf.numCopies(), 1);
    QCOMPARE(pdf.actualNumCopies(), 1);
    pdf.setCopyCount(9);
    QCOMPARE(pdf.copyCount(), 9);
    QCOMPARE(pdf.numCopies(), 9);
    QCOMPARE(pdf.actualNumCopies(), 9);
    pdf.setNumCopies(7);
    QCOMPARE(pdf.copyCount(), 7);
    QCOMPARE(pdf.numCopies(), 7);
    QCOMPARE(pdf.actualNumCopies(), 7);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QCOMPARE(native.supportsMultipleCopies(), true);
        QCOMPARE(native.copyCount(), 1);
        QCOMPARE(native.numCopies(), 1);
        QCOMPARE(native.actualNumCopies(), 1);

        // Test set/get
        native.setCopyCount(9);
        QCOMPARE(native.copyCount(), 9);
        QCOMPARE(native.numCopies(), 1);
        QCOMPARE(native.actualNumCopies(), 9);
        native.setNumCopies(7);
        QCOMPARE(native.copyCount(), 7);
        QCOMPARE(native.numCopies(), 1);
        QCOMPARE(native.actualNumCopies(), 7);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.copyCount(), 7);
        QCOMPARE(native.numCopies(), 7);
        QCOMPARE(native.actualNumCopies(), 7);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.copyCount(), 7);
        QCOMPARE(native.numCopies(), 1);
        QCOMPARE(native.actualNumCopies(), 7);
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
    QCOMPARE(pdf.duplex(), QPrinter::DuplexAuto);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QPrinterInfo printerInfo = QPrinterInfo::defaultPrinter();
        QPrinter::DuplexMode expected = printerInfo.defaultDuplexMode();
        QCOMPARE(native.duplex(), expected);
        // Test set/get (skipping Auto as that will return something different)
        foreach (QPrinter::DuplexMode mode, printerInfo.supportedDuplexModes()) {
            if (mode != expected && mode != QPrinter::DuplexAuto) {
                expected = mode;
                break;
            }
        }
        native.setDuplex(expected);
        QCOMPARE(native.duplex(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.duplex(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.duplex(), expected);

        // Test setting invalid option
        if (!printerInfo.supportedDuplexModes().contains(QPrinter::DuplexLongSide)) {
            native.setDuplex(QPrinter::DuplexLongSide);
            QCOMPARE(native.duplex(), expected);
        }
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::doubleSidedPrinting()
{
    // PdfFormat: Supported, default false
    // NativeFormat, Cups: Supported, default to printer default
    // NativeFormat, Win: Supported, default to printer default
    // NativeFormat, Mac: Supported, default to printer default

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.doubleSidedPrinting(), false);
    pdf.setDoubleSidedPrinting(true);
    QCOMPARE(pdf.doubleSidedPrinting(), true);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QPrinterInfo printerInfo(native);
        bool expected = (printerInfo.defaultDuplexMode() != QPrinter::DuplexNone);
        QCOMPARE(native.doubleSidedPrinting(), expected);

        // Test set/get, changing the expected value if possible
        expected = expected ? false : (printerInfo.supportedDuplexModes().count() > 1);
        native.setDoubleSidedPrinting(expected);
        QCOMPARE(native.doubleSidedPrinting(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.doubleSidedPrinting(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.doubleSidedPrinting(), expected);
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
    // PdfFormat: Supported, default QPrinter::Portrait
    // NativeFormat, Cups: Supported, default QPrinter::Portrait
    // NativeFormat, Win: Supported, default QPrinter::Portrait
    // NativeFormat, Mac: Supported, default QPrinter::Portrait

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.orientation(), QPrinter::Portrait);
    pdf.setOrientation(QPrinter::Landscape);
    QCOMPARE(pdf.orientation(), QPrinter::Landscape);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        // TODO Printer specific, need QPrinterInfo::orientation()
        //QCOMPARE(native.orientation(), QPrinter::Portrait);

        // Test set/get
        QPrinter::Orientation expected = QPrinter::Landscape;
        native.setOrientation(expected);
        QCOMPARE(native.orientation(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.orientation(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.orientation(), expected);

        // Test set/get
        expected = QPrinter::Portrait;
        native.setOrientation(expected);
        QCOMPARE(native.orientation(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.orientation(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.orientation(), expected);
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
    pdf.setOutputFileName(QStringLiteral("Test File"));
    QCOMPARE(pdf.outputFileName(), QString("Test File"));

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        QCOMPARE(native.outputFileName(), QString());

        // Test set/get
        QString expected = QStringLiteral("Test File");
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
    // PdfFormat: Supported, defaults to QPrinter::A4
    // NativeFormat, Cups: Supported, defaults to printer default
    // NativeFormat, Win: Supported, defaults to printer default
    // NativeFormat, Mac: Supported, must be supported size, defaults to printer default

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.pageSize(), QPrinter::A4);
    pdf.setPageSize(QPrinter::A1);
    QCOMPARE(pdf.pageSize(), QPrinter::A1);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        // TODO Printer specific, need QPrinterInfo::paperSize()
        //QCOMPARE(native.pageSize(), QPrinter::A4);

        // Test set/get
        QPrinter::PaperSize expected = QPrinter::A4;
        QPrinterInfo info = QPrinterInfo::printerInfo(native.printerName());
        foreach (QPrinter::PaperSize supported, info.supportedPaperSizes()) {
            if (supported != QPrinter::Custom && supported != native.paperSize()) {
                expected = supported;
                break;
            }
        }
        native.setPageSize(expected);
        QCOMPARE(native.pageSize(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.pageSize(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.pageSize(), expected);
    } else {
        QSKIP("No printers installed, cannot test NativeFormat, please install printers to test");
    }
}

void tst_QPrinter::paperSize()
{
    // PPK_PaperSize == PPK_PageSize
    // paperSize() / setPaperSize() / PPK_PaperSize
    // pageSize() / setPageSize() / PPK_PageSize
    // PdfFormat: Supported, defaults to QPrinter::A4
    // NativeFormat, Cups: Supported, defaults to printer default
    // NativeFormat, Win: Supported, defaults to printer default
    // NativeFormat, Mac: Supported, must be supported size, defaults to printer default

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.paperSize(), QPrinter::A4);
    pdf.setPaperSize(QPrinter::A1);
    QCOMPARE(pdf.paperSize(), QPrinter::A1);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test default
        // TODO Printer specific, need QPrinterInfo::paperSize()
        //QCOMPARE(native.paperSize(), QPrinter::A4);

        // Test set/get
        QPrinter::PaperSize expected = QPrinter::A4;
        QPrinterInfo info = QPrinterInfo::printerInfo(native.printerName());
        foreach (QPrinter::PaperSize supported, info.supportedPaperSizes()) {
            if (supported != QPrinter::Custom && supported != native.paperSize()) {
                expected = supported;
                break;
            }
        }
        native.setPaperSize(expected);
        QCOMPARE(native.paperSize(), expected);

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.paperSize(), expected);
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.paperSize(), expected);
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
        foreach (QPrinter::PaperSource supported, native.supportedPaperSources()) {
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
        foreach (const QPrinterInfo &available, QPrinterInfo::availablePrinters()) {
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
        // Set resolution does nothing on OSX, see QTBUG-7000
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

void tst_QPrinter::windowsPageSize()
{
    // winPageSize() / setWinPageSize() / PPK_WindowsPageSize
    // PdfFormat: Supported, defaults to printer default
    // NativeFormat, Cups: Supported, defaults to printer default
    // NativeFormat, Win: Supported, defaults to printer default
    // NativeFormat, Mac: Supported, defaults to printer default

    QPrinter pdf;
    pdf.setOutputFormat(QPrinter::PdfFormat);
    QCOMPARE(pdf.winPageSize(), 9);  // DMPAPER_A4
    pdf.setWinPageSize(1); // DMPAPER_LETTER
    QCOMPARE(pdf.winPageSize(), 1);

    QPrinter native;
    if (native.outputFormat() == QPrinter::NativeFormat) {
        // Test set/get
        native.setPaperSize(QPrinter::A4);
        QCOMPARE(native.pageSize(), QPrinter::A4);
        QCOMPARE(native.winPageSize(), 9);  // DMPAPER_A4

        native.setPaperSize(QPrinter::Letter);
        QCOMPARE(native.pageSize(), QPrinter::Letter);
        QCOMPARE(native.winPageSize(), 1); // DMPAPER_LETTER

        native.setWinPageSize(9);  // DMPAPER_A4
        QCOMPARE(native.pageSize(), QPrinter::A4);
        QCOMPARE(native.winPageSize(), 9);  // DMPAPER_A4

        native.setWinPageSize(1); // DMPAPER_LETTER
        QCOMPARE(native.pageSize(), QPrinter::Letter);
        QCOMPARE(native.winPageSize(), 1); // DMPAPER_LETTER

        // Test value preservation
        native.setOutputFormat(QPrinter::PdfFormat);
        QCOMPARE(native.pageSize(), QPrinter::Letter);
        QCOMPARE(native.winPageSize(), 1); // DMPAPER_LETTER
        native.setOutputFormat(QPrinter::NativeFormat);
        QCOMPARE(native.pageSize(), QPrinter::Letter);
        QCOMPARE(native.winPageSize(), 1); // DMPAPER_LETTER
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

    QTest::newRow("PDF A4")            << int(QPrinter::PdfFormat)    << int(QPrinter::A4) << 210.0 << 297.0 << false <<  0.0 <<  0.0 <<  0.0 <<  0.0;
    QTest::newRow("PDF A4 Margins")    << int(QPrinter::PdfFormat)    << int(QPrinter::A4) << 210.0 << 297.0 << true  << 20.0 << 30.0 << 40.0 << 50.0;
    QTest::newRow("Native A4")         << int(QPrinter::NativeFormat) << int(QPrinter::A4) << 210.0 << 297.0 << false <<  0.0 <<  0.0 <<  0.0 <<  0.0;
    QTest::newRow("Native A4 Margins") << int(QPrinter::NativeFormat) << int(QPrinter::A4) << 210.0 << 297.0 << true  << 20.0 << 30.0 << 40.0 << 50.0;

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

    QSizeF sizeMMf = QSizeF(widthMMf, heightMMf);

    QPrinter printer;
    printer.setOutputFormat(QPrinter::OutputFormat(outputFormat));
    if (printer.outputFormat() != QPrinter::OutputFormat(outputFormat))
        QSKIP("Please install a native printer to run this test");
    QCOMPARE(printer.outputFormat(), QPrinter::OutputFormat(outputFormat));
    QCOMPARE(printer.orientation(), QPrinter::Portrait);

    if (setMargins) {
        // Setup the given margins
        QPrinter::Margins margins;
        margins.left = leftMMf;
        margins.right = rightMMf;
        margins.top = topMMf;
        margins.bottom = bottomMMf;
        printer.setMargins(margins);
        QCOMPARE(printer.margins().left, leftMMf);
        QCOMPARE(printer.margins().right, rightMMf);
        QCOMPARE(printer.margins().top, topMMf);
        QCOMPARE(printer.margins().bottom, bottomMMf);
    }


    // Set the given size, in Portrait mode
    if (pageSize < 0) {
        printer.setPageSizeMM(sizeMMf);
        QCOMPARE(printer.pageSize(), QPrinter::Custom);
    } else {
        printer.setPageSize(QPrinter::PageSize(pageSize));
        QCOMPARE(printer.pageSize(), QPrinter::PageSize(pageSize));
    }
    QCOMPARE(printer.orientation(), QPrinter::Portrait);
    if (setMargins) {
        // Check margins unchanged from page size change
        QCOMPARE(printer.margins().left, leftMMf);
        QCOMPARE(printer.margins().right, rightMMf);
        QCOMPARE(printer.margins().top, topMMf);
        QCOMPARE(printer.margins().bottom, bottomMMf);
    } else {
        // Fetch the default margins for the printer and page size
        // TODO Check against margins from print device when api added
        leftMMf = printer.margins().left;
        rightMMf = printer.margins().right;
        topMMf = printer.margins().top;
        bottomMMf = printer.margins().bottom;
    }

    // QPagedPaintDevice::pageSizeMM() always returns Portrait
    QCOMPARE(printer.pageSizeMM(), sizeMMf);

    // QPrinter::paperSize() always returns set orientation
    QCOMPARE(printer.paperSize(QPrinter::Millimeter), sizeMMf);

    // QPagedPaintDevice::widthMM() and heightMM() are paint metrics and always return set orientation
    QCOMPARE(printer.widthMM(), qRound(widthMMf - leftMMf - rightMMf));
    QCOMPARE(printer.heightMM(), qRound(heightMMf - topMMf - bottomMMf));

    // QPrinter::paperRect() always returns set orientation
    QCOMPARE(printer.paperRect(QPrinter::Millimeter), QRectF(0, 0, widthMMf, heightMMf));

    // QPrinter::pageRect() always returns set orientation
    QCOMPARE(printer.pageRect(QPrinter::Millimeter), QRectF(leftMMf, topMMf, widthMMf - leftMMf - rightMMf, heightMMf - topMMf - bottomMMf));


    // Now switch to Landscape mode, size should be unchanged, but rect and metrics should change
    printer.setOrientation(QPrinter::Landscape);
    if (pageSize < 0) {
        QCOMPARE(printer.pageSize(), QPrinter::Custom);
    } else {
        QCOMPARE(printer.pageSize(), QPrinter::PageSize(pageSize));
    }
    QCOMPARE(printer.orientation(), QPrinter::Landscape);
    if (setMargins) {
        // Check margins unchanged from page size change
        QCOMPARE(printer.margins().left, leftMMf);
        QCOMPARE(printer.margins().right, rightMMf);
        QCOMPARE(printer.margins().top, topMMf);
        QCOMPARE(printer.margins().bottom, bottomMMf);
    } else {
        // Fetch the default margins for the printer and page size
        // TODO Check against margins from print device when api added
        leftMMf = printer.margins().left;
        rightMMf = printer.margins().right;
        topMMf = printer.margins().top;
        bottomMMf = printer.margins().bottom;
    }

    // QPagedPaintDevice::pageSizeMM() always returns Portrait
    QCOMPARE(printer.pageSizeMM(), sizeMMf);

    // QPrinter::paperSize() always returns set orientation
    QCOMPARE(printer.paperSize(QPrinter::Millimeter), sizeMMf.transposed());

    // QPagedPaintDevice::widthMM() and heightMM() are paint metrics and always return set orientation
    QCOMPARE(printer.widthMM(), qRound(heightMMf - leftMMf - rightMMf));
    QCOMPARE(printer.heightMM(), qRound(widthMMf - topMMf - bottomMMf));

    // QPrinter::paperRect() always returns set orientation
    QCOMPARE(printer.paperRect(QPrinter::Millimeter), QRectF(0, 0, heightMMf, widthMMf));

    // QPrinter::pageRect() always returns set orientation
    QCOMPARE(printer.pageRect(QPrinter::Millimeter), QRectF(leftMMf, topMMf, heightMMf - leftMMf - rightMMf, widthMMf - topMMf - bottomMMf));


    // Now while in Landscape mode, set the size again, results should be the same
    if (pageSize < 0) {
        printer.setPageSizeMM(sizeMMf);
        QCOMPARE(printer.pageSize(), QPrinter::Custom);
    } else {
        printer.setPageSize(QPrinter::PageSize(pageSize));
        QCOMPARE(printer.pageSize(), QPrinter::PageSize(pageSize));
    }
    QCOMPARE(printer.orientation(), QPrinter::Landscape);
    if (setMargins) {
        // Check margins unchanged from page size change
        QCOMPARE(printer.margins().left, leftMMf);
        QCOMPARE(printer.margins().right, rightMMf);
        QCOMPARE(printer.margins().top, topMMf);
        QCOMPARE(printer.margins().bottom, bottomMMf);
    } else {
        // Fetch the default margins for the printer and page size
        // TODO Check against margins from print device when api added
        leftMMf = printer.margins().left;
        rightMMf = printer.margins().right;
        topMMf = printer.margins().top;
        bottomMMf = printer.margins().bottom;
    }

    // QPagedPaintDevice::pageSizeMM() always returns Portrait
    QCOMPARE(printer.pageSizeMM(), sizeMMf);

    // QPrinter::paperSize() always returns set orientation
    QCOMPARE(printer.paperSize(QPrinter::Millimeter), sizeMMf.transposed());

    // QPagedPaintDevice::widthMM() and heightMM() are paint metrics and always return set orientation
    QCOMPARE(printer.widthMM(), qRound(heightMMf - leftMMf - rightMMf));
    QCOMPARE(printer.heightMM(), qRound(widthMMf - topMMf - bottomMMf));

    // QPrinter::paperRect() always returns set orientation
    QCOMPARE(printer.paperRect(QPrinter::Millimeter), QRectF(0, 0, heightMMf, widthMMf));

    // QPrinter::pageRect() always returns set orientation
    QCOMPARE(printer.pageRect(QPrinter::Millimeter), QRectF(leftMMf, topMMf, heightMMf - leftMMf - rightMMf, widthMMf - topMMf - bottomMMf));
}

#endif // QT_NO_PRINTER

QTEST_MAIN(tst_QPrinter)
#include "tst_qprinter.moc"
