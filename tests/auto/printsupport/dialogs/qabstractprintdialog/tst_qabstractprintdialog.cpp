/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <QtPrintSupport/qtprintsupportglobal.h>
#if QT_CONFIG(printdialog)
#include <qabstractprintdialog.h>
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

class MyAbstractPrintDialog : public QAbstractPrintDialog
{
public:
    MyAbstractPrintDialog(QPrinter *p) : QAbstractPrintDialog(p) {}
    int exec() { return 0; }
};

// Testing get/set functions
void tst_QAbstractPrintDialog::getSetCheck()
{
    QPrinter printer;
    MyAbstractPrintDialog obj1(&printer);
    QCOMPARE(obj1.printer(), &printer);
    // PrintDialogOptions QAbstractPrintDialog::enabledOptions()
    // void QAbstractPrintDialog::setEnabledOptions(PrintDialogOptions)
    obj1.setEnabledOptions(QAbstractPrintDialog::PrintDialogOptions(QAbstractPrintDialog::None));
    QCOMPARE(QAbstractPrintDialog::PrintDialogOptions(QAbstractPrintDialog::None), obj1.enabledOptions());
    obj1.setEnabledOptions(QAbstractPrintDialog::PrintDialogOptions(QAbstractPrintDialog::PrintToFile));
    QCOMPARE(QAbstractPrintDialog::PrintDialogOptions(QAbstractPrintDialog::PrintToFile), obj1.enabledOptions());
    obj1.setEnabledOptions(QAbstractPrintDialog::PrintDialogOptions(QAbstractPrintDialog::PrintSelection));
    QCOMPARE(QAbstractPrintDialog::PrintDialogOptions(QAbstractPrintDialog::PrintSelection), obj1.enabledOptions());
    obj1.setEnabledOptions(QAbstractPrintDialog::PrintDialogOptions(QAbstractPrintDialog::PrintPageRange));
    QCOMPARE(QAbstractPrintDialog::PrintDialogOptions(QAbstractPrintDialog::PrintPageRange), obj1.enabledOptions());
    obj1.setEnabledOptions(QAbstractPrintDialog::PrintDialogOptions(QAbstractPrintDialog::PrintCollateCopies));
    QCOMPARE(QAbstractPrintDialog::PrintDialogOptions(QAbstractPrintDialog::PrintCollateCopies), obj1.enabledOptions());

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
    MyAbstractPrintDialog obj1(&printer);
    obj1.setEnabledOptions(QAbstractPrintDialog::PrintDialogOptions(QAbstractPrintDialog::None));
    QCOMPARE(obj1.minPage(), 0);
    QCOMPARE(obj1.maxPage(), INT_MAX);
    QVERIFY(!obj1.isOptionEnabled(QAbstractPrintDialog::PrintPageRange));
    obj1.setMinMax(2,5);
    QCOMPARE(obj1.minPage(), 2);
    QCOMPARE(obj1.maxPage(), 5);
    QVERIFY(obj1.enabledOptions() & QAbstractPrintDialog::PrintPageRange);
    QVERIFY(obj1.isOptionEnabled(QAbstractPrintDialog::PrintPageRange));
}

void tst_QAbstractPrintDialog::setFromTo()
{
    QPrinter printer;
    MyAbstractPrintDialog obj1(&printer);
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
