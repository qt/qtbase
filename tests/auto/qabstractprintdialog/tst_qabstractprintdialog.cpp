/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qabstractprintdialog.h>
#include <qprinter.h>

//TESTED_CLASS=
//TESTED_FILES=

#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)

class tst_QAbstractPrintDialog : public QObject
{
Q_OBJECT

public:
    tst_QAbstractPrintDialog();
    virtual ~tst_QAbstractPrintDialog();

private slots:
    void getSetCheck();
    void setMinMax();
    void setFromTo();
};

class MyAbstractPrintDialog : public QAbstractPrintDialog
{
public:
    MyAbstractPrintDialog(QPrinter *p) : QAbstractPrintDialog(p) {}
    int exec() { return 0; }
};

tst_QAbstractPrintDialog::tst_QAbstractPrintDialog()
{
}

tst_QAbstractPrintDialog::~tst_QAbstractPrintDialog()
{
}

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
    QCOMPARE(obj1.minPage(), 1);
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

QTEST_MAIN(tst_QAbstractPrintDialog)
#include "tst_qabstractprintdialog.moc"

#else

QTEST_NOOP_MAIN

#endif
