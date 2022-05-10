// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qlcdnumber.h>

class tst_QLCDNumber : public QObject
{
Q_OBJECT

public:
    tst_QLCDNumber();
    virtual ~tst_QLCDNumber();

private slots:
    void getSetCheck();
};

tst_QLCDNumber::tst_QLCDNumber()
{
}

tst_QLCDNumber::~tst_QLCDNumber()
{
}

// Testing get/set functions
void tst_QLCDNumber::getSetCheck()
{
    QLCDNumber obj1;
    // int QLCDNumber::digitCount()
    // void QLCDNumber::setDigitCount(int)
    obj1.setDigitCount(0);
    QCOMPARE(0, obj1.digitCount());
    obj1.setDigitCount(INT_MIN);
    QCOMPARE(0, obj1.digitCount()); // Range<0, 99>
    obj1.setDigitCount(INT_MAX);
    QCOMPARE(99, obj1.digitCount()); // Range<0, 99>
}

QTEST_MAIN(tst_QLCDNumber)
#include "tst_qlcdnumber.moc"
