// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <QtPrintSupport/qtprintsupportglobal.h>
#if QT_CONFIG(printdialog)
#include <qprintdialog.h>
#include <qprinter.h>
#endif

class tst_QAbstractPrintDialog : public QObject
{
Q_OBJECT

#if !QT_CONFIG(printdialog)
public slots:
    void initTestCase();
#else
private slots:
    void getSetCheck();
    void setMinMax();
    void setFromTo();
#endif
};

#if !QT_CONFIG(printdialog)
void tst_QAbstractPrintDialog::initTestCase()
{
    QSKIP("This test requires printing and print dialog support");
}

#else

class MyPrintDialog : public QPrintDialog
{
public:
    MyPrintDialog(QPrinter *p)
        : QPrintDialog(p)
    {
    }
    int exec() override { return 0; }

    void resetAllOptions()
    {
        setOption(QAbstractPrintDialog::PrintToFile, false);
        setOption(QAbstractPrintDialog::PrintSelection, false);
        setOption(QAbstractPrintDialog::PrintPageRange, false);
        setOption(QAbstractPrintDialog::PrintShowPageSize, false);
        setOption(QAbstractPrintDialog::PrintCollateCopies, false);
        setOption(QAbstractPrintDialog::PrintCurrentPage, false);
    }
};

// Testing get/set functions
void tst_QAbstractPrintDialog::getSetCheck()
{
    QPrinter printer;
    MyPrintDialog obj1(&printer);
    QCOMPARE(obj1.printer(), &printer);

    // QPrintDialog::options APIs
    QCOMPARE(obj1.options(), QAbstractPrintDialog::PrintToFile
                            |QAbstractPrintDialog::PrintPageRange
                            |QAbstractPrintDialog::PrintShowPageSize
                            |QAbstractPrintDialog::PrintCollateCopies);
    obj1.setOptions(QAbstractPrintDialog::PrintSelection
                   |QAbstractPrintDialog::PrintCurrentPage);
    QCOMPARE(obj1.options(), QAbstractPrintDialog::PrintSelection
                            |QAbstractPrintDialog::PrintCurrentPage);
    obj1.resetAllOptions();
    QCOMPARE(obj1.options(), QAbstractPrintDialog::PrintDialogOptions());

    obj1.setOption(QAbstractPrintDialog::PrintToFile);
    QVERIFY(obj1.testOption(QAbstractPrintDialog::PrintToFile));
    QCOMPARE(obj1.options(), QAbstractPrintDialog::PrintToFile);
    obj1.setOption(QAbstractPrintDialog::PrintSelection);
    QVERIFY(obj1.testOption(QAbstractPrintDialog::PrintSelection));
    QCOMPARE(obj1.options(), QAbstractPrintDialog::PrintToFile
                            |QAbstractPrintDialog::PrintSelection);
    obj1.setOption(QAbstractPrintDialog::PrintPageRange);
    QVERIFY(obj1.testOption(QAbstractPrintDialog::PrintPageRange));
    QCOMPARE(obj1.options(), QAbstractPrintDialog::PrintToFile
                            |QAbstractPrintDialog::PrintSelection
                            |QAbstractPrintDialog::PrintPageRange);
    obj1.setOption(QAbstractPrintDialog::PrintCollateCopies);
    QVERIFY(obj1.testOption(QAbstractPrintDialog::PrintCollateCopies));
    QCOMPARE(obj1.options(), QAbstractPrintDialog::PrintToFile
                            |QAbstractPrintDialog::PrintSelection
                            |QAbstractPrintDialog::PrintPageRange
                            |QAbstractPrintDialog::PrintCollateCopies);

    // PrintRange QAbstractPrintDialog::printRange()
    // void QAbstractPrintDialog::setPrintRange(PrintRange)
    obj1.setPrintRange(QAbstractPrintDialog::PrintRange(QAbstractPrintDialog::AllPages));
    QCOMPARE(QAbstractPrintDialog::PrintRange(QAbstractPrintDialog::AllPages), obj1.printRange());
    obj1.setPrintRange(QAbstractPrintDialog::PrintRange(QAbstractPrintDialog::Selection));
    QCOMPARE(QAbstractPrintDialog::PrintRange(QAbstractPrintDialog::Selection), obj1.printRange());
    obj1.setPrintRange(QAbstractPrintDialog::PrintRange(QAbstractPrintDialog::PageRange));
    QCOMPARE(QAbstractPrintDialog::PrintRange(QAbstractPrintDialog::PageRange), obj1.printRange());
}

void tst_QAbstractPrintDialog::setMinMax()
{
    QPrinter printer;
    MyPrintDialog obj1(&printer);
    obj1.resetAllOptions();

    QCOMPARE(obj1.minPage(), 0);
    QCOMPARE(obj1.maxPage(), INT_MAX);
    QVERIFY(!obj1.testOption(QAbstractPrintDialog::PrintPageRange));
    obj1.setMinMax(2,5);
    QCOMPARE(obj1.minPage(), 2);
    QCOMPARE(obj1.maxPage(), 5);
    QVERIFY(obj1.options() & QAbstractPrintDialog::PrintPageRange);
    QVERIFY(obj1.testOption(QAbstractPrintDialog::PrintPageRange));
}

void tst_QAbstractPrintDialog::setFromTo()
{
    QPrinter printer;
    MyPrintDialog obj1(&printer);
    QCOMPARE(obj1.fromPage(), 0);
    QCOMPARE(obj1.toPage(), 0);
    obj1.setMinMax(0,0);
    QCOMPARE(obj1.minPage(), 0);
    QCOMPARE(obj1.maxPage(), 0);
    obj1.setFromTo(20,50);
    QCOMPARE(obj1.fromPage(), 20);
    QCOMPARE(obj1.toPage(), 50);
    QCOMPARE(obj1.minPage(), 1);
    QCOMPARE(obj1.maxPage(), 50);
}

#endif

QTEST_MAIN(tst_QAbstractPrintDialog)
#include "tst_qabstractprintdialog.moc"
