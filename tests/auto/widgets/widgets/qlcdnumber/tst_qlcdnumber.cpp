// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


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

    void displayEdgeCases_data();
    void displayEdgeCases();
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

// Test case for undefined behavior when displaying INT_MIN
void tst_QLCDNumber::displayEdgeCases_data()
{
    QTest::addColumn<int>("number");
    QTest::addColumn<QString>("expected");

    // INT_MIN previously caused UB due to -INT_MIN overflow in int2string().
    QTest::newRow("INT_MIN") << INT_MIN << QString::number(INT_MIN);
    QTest::newRow("INT_MAX") << INT_MAX << QString::number(INT_MAX);
    QTest::newRow("Zero")    << 0       << QStringLiteral("0");
}

void tst_QLCDNumber::displayEdgeCases()
{
    QFETCH(int, number);
    QFETCH(QString, expected);

    QLCDNumber lcd;
    lcd.setDigitCount(12);
    lcd.display(number);

    QString result = QString::number(lcd.intValue());
    QCOMPARE(result, expected);
}

QTEST_MAIN(tst_QLCDNumber)
#include "tst_qlcdnumber.moc"
