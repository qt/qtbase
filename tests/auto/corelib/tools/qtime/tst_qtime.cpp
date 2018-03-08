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
#include "qdatetime.h"
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#  include <locale.h>
#endif

class tst_QTime : public QObject
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
public:
    tst_QTime()
    {
        // Some tests depend on C locale - BF&I it with belt *and* braces:
        qputenv("LC_ALL", "C");
        setlocale(LC_ALL, "C");
        // Need to instantiate as early as possible, before anything accesses
        // the QSystemLocale singleton; once it exists, there's no changing it.
    }
#endif // remove for ### Qt 6

private slots:
    void msecsTo_data();
    void msecsTo();
    void secsTo_data();
    void secsTo();
    void setHMS_data();
    void setHMS();
    void hour_data();
    void hour();
    void isValid();
    void isNull();
    void addMSecs_data();
    void addMSecs();
    void addSecs_data();
    void addSecs();
    void operator_eq_eq_data();
    void operator_eq_eq();
    void operator_lt();
    void operator_gt();
    void operator_lt_eq();
    void operator_gt_eq();
    void fromStringFormat_data();
    void fromStringFormat();
    void fromStringDateFormat_data();
    void fromStringDateFormat();
    void toStringDateFormat_data();
    void toStringDateFormat();
    void toStringFormat_data();
    void toStringFormat();
    void toStringLocale();
    void msecsSinceStartOfDay_data();
    void msecsSinceStartOfDay();

private:
    QTime invalidTime() { return QTime(-1, -1, -1); }
};

Q_DECLARE_METATYPE(Qt::DateFormat)

void tst_QTime::addSecs_data()
{
    QTest::addColumn<QTime>("t1");
    QTest::addColumn<int>("i");
    QTest::addColumn<QTime>("exp");

    QTest::newRow("Data0") << QTime(0,0,0) << 200 << QTime(0,3,20);
    QTest::newRow("Data1") << QTime(0,0,0) << 20 << QTime(0,0,20);
    QTest::newRow("overflow") << QTime(0,0,0) << (INT_MAX / 1000 + 1)
                              << QTime(0,0,0).addSecs((INT_MAX / 1000 + 1) % 86400);
}

void tst_QTime::addSecs()
{
    QFETCH( QTime, t1 );
    QFETCH( int, i );
    QTime t2;
    t2 = t1.addSecs( i );
    QFETCH( QTime, exp );
    QCOMPARE( t2, exp );
}

void tst_QTime::addMSecs_data()
{
    QTest::addColumn<QTime>("t1");
    QTest::addColumn<int>("i");
    QTest::addColumn<QTime>("exp");

    // start with testing positive values
    QTest::newRow( "Data1_0") << QTime(0,0,0,0) << 2000 << QTime(0,0,2,0);
    QTest::newRow( "Data1_1") << QTime(0,0,0,0) << 200 << QTime(0,0,0,200);
    QTest::newRow( "Data1_2") << QTime(0,0,0,0) << 20 << QTime(0,0,0,20);
    QTest::newRow( "Data1_3") << QTime(0,0,0,1) << 1 << QTime(0,0,0,2);
    QTest::newRow( "Data1_4") << QTime(0,0,0,0) << 0 << QTime(0,0,0,0);

    QTest::newRow( "Data2_0") << QTime(0,0,0,98) << 0 << QTime(0,0,0,98);
    QTest::newRow( "Data2_1") << QTime(0,0,0,98) << 1 << QTime(0,0,0,99);
    QTest::newRow( "Data2_2") << QTime(0,0,0,98) << 2 << QTime(0,0,0,100);
    QTest::newRow( "Data2_3") << QTime(0,0,0,98) << 3 << QTime(0,0,0,101);

    QTest::newRow( "Data3_0") << QTime(0,0,0,998) << 0 << QTime(0,0,0,998);
    QTest::newRow( "Data3_1") << QTime(0,0,0,998) << 1 << QTime(0,0,0,999);
    QTest::newRow( "Data3_2") << QTime(0,0,0,998) << 2 << QTime(0,0,1,0);
    QTest::newRow( "Data3_3") << QTime(0,0,0,998) << 3 << QTime(0,0,1,1);

    QTest::newRow( "Data4_0") << QTime(0,0,1,995) << 4 << QTime(0,0,1,999);
    QTest::newRow( "Data4_1") << QTime(0,0,1,995) << 5 << QTime(0,0,2,0);
    QTest::newRow( "Data4_2") << QTime(0,0,1,995) << 6 << QTime(0,0,2,1);
    QTest::newRow( "Data4_3") << QTime(0,0,1,995) << 100 << QTime(0,0,2,95);
    QTest::newRow( "Data4_4") << QTime(0,0,1,995) << 105 << QTime(0,0,2,100);

    QTest::newRow( "Data5_0") << QTime(0,0,59,995) << 4 << QTime(0,0,59,999);
    QTest::newRow( "Data5_1") << QTime(0,0,59,995) << 5 << QTime(0,1,0,0);
    QTest::newRow( "Data5_2") << QTime(0,0,59,995) << 6 << QTime(0,1,0,1);
    QTest::newRow( "Data5_3") << QTime(0,0,59,995) << 1006 << QTime(0,1,1,1);

    QTest::newRow( "Data6_0") << QTime(0,59,59,995) << 4 << QTime(0,59,59,999);
    QTest::newRow( "Data6_1") << QTime(0,59,59,995) << 5 << QTime(1,0,0,0);
    QTest::newRow( "Data6_2") << QTime(0,59,59,995) << 6 << QTime(1,0,0,1);
    QTest::newRow( "Data6_3") << QTime(0,59,59,995) << 106 << QTime(1,0,0,101);
    QTest::newRow( "Data6_4") << QTime(0,59,59,995) << 1004 << QTime(1,0,0,999);
    QTest::newRow( "Data6_5") << QTime(0,59,59,995) << 1005 << QTime(1,0,1,0);
    QTest::newRow( "Data6_6") << QTime(0,59,59,995) << 61006 << QTime(1,1,1,1);

    QTest::newRow( "Data7_0") << QTime(23,59,59,995) << 0 << QTime(23,59,59,995);
    QTest::newRow( "Data7_1") << QTime(23,59,59,995) << 4 << QTime(23,59,59,999);
    QTest::newRow( "Data7_2") << QTime(23,59,59,995) << 5 << QTime(0,0,0,0);
    QTest::newRow( "Data7_3") << QTime(23,59,59,995) << 6 << QTime(0,0,0,1);
    QTest::newRow( "Data7_4") << QTime(23,59,59,995) << 7 << QTime(0,0,0,2);

    // must test negative values too...
    QTest::newRow( "Data11_0") << QTime(0,0,2,0) << -2000 << QTime(0,0,0,0);
    QTest::newRow( "Data11_1") << QTime(0,0,0,200) << -200 << QTime(0,0,0,0);
    QTest::newRow( "Data11_2") << QTime(0,0,0,20) << -20 << QTime(0,0,0,0);
    QTest::newRow( "Data11_3") << QTime(0,0,0,2) << -1 << QTime(0,0,0,1);
    QTest::newRow( "Data11_4") << QTime(0,0,0,0) << -0 << QTime(0,0,0,0);

    QTest::newRow( "Data12_0") << QTime(0,0,0,98) << -0 << QTime(0,0,0,98);
    QTest::newRow( "Data12_1") << QTime(0,0,0,99) << -1 << QTime(0,0,0,98);
    QTest::newRow( "Data12_2") << QTime(0,0,0,100) << -2 << QTime(0,0,0,98);
    QTest::newRow( "Data12_3") << QTime(0,0,0,101) << -3 << QTime(0,0,0,98);

    QTest::newRow( "Data13_0") << QTime(0,0,0,998) << -0 << QTime(0,0,0,998);
    QTest::newRow( "Data13_1") << QTime(0,0,0,999) << -1 << QTime(0,0,0,998);
    QTest::newRow( "Data13_2") << QTime(0,0,1,0) << -2 << QTime(0,0,0,998);
    QTest::newRow( "Data13_3") << QTime(0,0,1,1) << -3 << QTime(0,0,0,998);

    QTest::newRow( "Data14_0") << QTime(0,0,1,999) << -4 << QTime(0,0,1,995);
    QTest::newRow( "Data14_1") << QTime(0,0,2,0) << -5 << QTime(0,0,1,995);
    QTest::newRow( "Data14_2") << QTime(0,0,2,1) << -6 << QTime(0,0,1,995);
    QTest::newRow( "Data14_3") << QTime(0,0,2,95) << -100 << QTime(0,0,1,995);
    QTest::newRow( "Data14_4") << QTime(0,0,2,100) << -105 << QTime(0,0,1,995);

    QTest::newRow( "Data15_0") << QTime(0,0,59,999) << -4 << QTime(0,0,59,995);
    QTest::newRow( "Data15_1") << QTime(0,1,0,0) << -5 << QTime(0,0,59,995);
    QTest::newRow( "Data15_2") << QTime(0,1,0,1) << -6 << QTime(0,0,59,995);
    QTest::newRow( "Data15_3") << QTime(0,1,1,1) << -1006 << QTime(0,0,59,995);

    QTest::newRow( "Data16_0") << QTime(0,59,59,999) << -4 << QTime(0,59,59,995);
    QTest::newRow( "Data16_1") << QTime(1,0,0,0) << -5 << QTime(0,59,59,995);
    QTest::newRow( "Data16_2") << QTime(1,0,0,1) << -6 << QTime(0,59,59,995);
    QTest::newRow( "Data16_3") << QTime(1,0,0,101) << -106 << QTime(0,59,59,995);
    QTest::newRow( "Data16_4") << QTime(1,0,0,999) << -1004 << QTime(0,59,59,995);
    QTest::newRow( "Data16_5") << QTime(1,0,1,0) << -1005 << QTime(0,59,59,995);
    QTest::newRow( "Data16_6") << QTime(1,1,1,1) << -61006 << QTime(0,59,59,995);

    QTest::newRow( "Data17_0") << QTime(23,59,59,995) << -0 << QTime(23,59,59,995);
    QTest::newRow( "Data17_1") << QTime(23,59,59,999) << -4 << QTime(23,59,59,995);
    QTest::newRow( "Data17_2") << QTime(0,0,0,0) << -5 << QTime(23,59,59,995);
    QTest::newRow( "Data17_3") << QTime(0,0,0,1) << -6 << QTime(23,59,59,995);
    QTest::newRow( "Data17_4") << QTime(0,0,0,2) << -7 << QTime(23,59,59,995);

    QTest::newRow( "Data18_0" ) << invalidTime() << 1 << invalidTime();
}

void tst_QTime::addMSecs()
{
    QFETCH( QTime, t1 );
    QFETCH( int, i );
    QTime t2;
    t2 = t1.addMSecs( i );
    QFETCH( QTime, exp );
    QCOMPARE( t2, exp );
}

void tst_QTime::isNull()
{
    QTime t1;
    QVERIFY( t1.isNull() );
    QTime t2(0,0,0);
    QVERIFY( !t2.isNull() );
    QTime t3(0,0,1);
    QVERIFY( !t3.isNull() );
    QTime t4(0,0,0,1);
    QVERIFY( !t4.isNull() );
    QTime t5(23,59,59);
    QVERIFY( !t5.isNull() );
}

void tst_QTime::isValid()
{
    QTime t1;
    QVERIFY( !t1.isValid() );
    QTime t2(24,0,0,0);
    QVERIFY( !t2.isValid() );
    QTime t3(23,60,0,0);
    QVERIFY( !t3.isValid() );
    QTime t4(23,0,-1,0);
    QVERIFY( !t4.isValid() );
    QTime t5(23,0,60,0);
    QVERIFY( !t5.isValid() );
    QTime t6(23,0,0,1000);
    QVERIFY( !t6.isValid() );
}

void tst_QTime::hour_data()
{
    QTest::addColumn<int>("hour");
    QTest::addColumn<int>("minute");
    QTest::addColumn<int>("sec");
    QTest::addColumn<int>("msec");

    QTest::newRow(  "data0" ) << 0 << 0 << 0 << 0;
    QTest::newRow(  "data1" ) << 0 << 0 << 0 << 1;
    QTest::newRow(  "data2" ) << 1 << 2 << 3 << 4;
    QTest::newRow(  "data3" ) << 2 << 12 << 13 << 65;
    QTest::newRow(  "data4" ) << 23 << 59 << 59 << 999;
    QTest::newRow(  "data5" ) << -1 << -1 << -1 << -1;
}

void tst_QTime::hour()
{
    QFETCH( int, hour );
    QFETCH( int, minute );
    QFETCH( int, sec );
    QFETCH( int, msec );

    QTime t1( hour, minute, sec, msec );
    QCOMPARE( t1.hour(), hour );
    QCOMPARE( t1.minute(), minute );
    QCOMPARE( t1.second(), sec );
    QCOMPARE( t1.msec(), msec );
}

void tst_QTime::setHMS_data()
{
    QTest::addColumn<int>("hour");
    QTest::addColumn<int>("minute");
    QTest::addColumn<int>("sec");

    QTest::newRow(  "data0" ) << 0 << 0 << 0;
    QTest::newRow(  "data1" ) << 1 << 2 << 3;
    QTest::newRow(  "data2" ) << 0 << 59 << 0;
    QTest::newRow(  "data3" ) << 0 << 59 << 59;
    QTest::newRow(  "data4" ) << 23 << 0 << 0;
    QTest::newRow(  "data5" ) << 23 << 59 << 0;
    QTest::newRow(  "data6" ) << 23 << 59 << 59;
    QTest::newRow(  "data7" ) << -1 << -1 << -1;
}

void tst_QTime::setHMS()
{
    QFETCH( int, hour );
    QFETCH( int, minute );
    QFETCH( int, sec );

    QTime t(3,4,5);
    t.setHMS( hour, minute, sec );
    QCOMPARE( t.hour(), hour );
    QCOMPARE( t.minute(), minute );
    QCOMPARE( t.second(), sec );
}

void tst_QTime::secsTo_data()
{
    QTest::addColumn<QTime>("t1");
    QTest::addColumn<QTime>("t2");
    QTest::addColumn<int>("delta");

    QTest::newRow(  "data0" ) << QTime(0,0,0) << QTime(0,0,59) << 59;
    QTest::newRow(  "data1" ) << QTime(0,0,0) << QTime(0,1,0) << 60;
    QTest::newRow(  "data2" ) << QTime(0,0,0) << QTime(0,10,0) << 600;
    QTest::newRow(  "data3" ) << QTime(0,0,0) << QTime(23,59,59) << 86399;
    QTest::newRow(  "data4" ) << QTime(-1, -1, -1) << QTime(0, 0, 0) << 0;
    QTest::newRow(  "data5" ) << QTime(0, 0, 0) << QTime(-1, -1, -1) << 0;
    QTest::newRow(  "data6" ) << QTime(-1, -1, -1) << QTime(-1, -1, -1) << 0;
    QTest::newRow("disregard msec (1s)") << QTime(12, 30, 1, 500) << QTime(12, 30, 2, 400) << 1;
    QTest::newRow("disregard msec (0s)") << QTime(12, 30, 1, 500) << QTime(12, 30, 1, 900) << 0;
    QTest::newRow("disregard msec (-1s)") << QTime(12, 30, 2, 400) << QTime(12, 30, 1, 500) << -1;
    QTest::newRow("disregard msec (0s)") << QTime(12, 30, 1, 900) << QTime(12, 30, 1, 500) << 0;
}

void tst_QTime::secsTo()
{
    QFETCH( QTime, t1 );
    QFETCH( QTime, t2 );
    QFETCH( int, delta );

    QCOMPARE( t1.secsTo( t2 ), delta );
}

void tst_QTime::msecsTo_data()
{
    QTest::addColumn<QTime>("t1");
    QTest::addColumn<QTime>("t2");
    QTest::addColumn<int>("delta");

    QTest::newRow(  "data0" ) << QTime(0,0,0,0) << QTime(0,0,0,0) << 0;
    QTest::newRow(  "data1" ) << QTime(0,0,0,0) << QTime(0,0,1,0) << 1000;
    QTest::newRow(  "data2" ) << QTime(0,0,0,0) << QTime(0,0,10,0) << 10000;
    QTest::newRow(  "data3" ) << QTime(0,0,0,0) << QTime(23,59,59,0) << 86399000;
    QTest::newRow(  "data4" ) << QTime(-1, -1, -1, -1) << QTime(0, 0, 0, 0) << 0;
    QTest::newRow(  "data5" ) << QTime(0, 0, 0, 0) << QTime(-1, -1, -1, -1) << 0;
    QTest::newRow(  "data6" ) << QTime(-1, -1, -1, -1) << QTime(-1, -1, -1, -1) << 0;
}

void tst_QTime::msecsTo()
{
    QFETCH( QTime, t1 );
    QFETCH( QTime, t2 );
    QFETCH( int, delta );

    QCOMPARE( t1.msecsTo( t2 ), delta );
}

void tst_QTime::operator_eq_eq_data()
{
    QTest::addColumn<QTime>("t1");
    QTest::addColumn<QTime>("t2");
    QTest::addColumn<bool>("expectEqual");

    QTime time1(0, 0, 0, 0);
    QTime time2 = time1.addMSecs(1);
    QTime time3 = time1.addMSecs(-1);
    QTime time4(23, 59, 59, 999);

    QTest::newRow("data0") << time1 << time2 << false;
    QTest::newRow("data1") << time2 << time3 << false;
    QTest::newRow("data2") << time4 << time1 << false;
    QTest::newRow("data3") << time1 << time1 << true;
    QTest::newRow("data4") << QTime(12,34,56,20) << QTime(12,34,56,20) << true;
    QTest::newRow("data5") << QTime(01,34,56,20) << QTime(13,34,56,20) << false;
}

void tst_QTime::operator_eq_eq()
{
    QFETCH(QTime, t1);
    QFETCH(QTime, t2);
    QFETCH(bool, expectEqual);

    bool equal = t1 == t2;
    QCOMPARE(equal, expectEqual);
    bool notEqual = t1 != t2;
    QCOMPARE(notEqual, !expectEqual);

    if (equal)
        QVERIFY(qHash(t1) == qHash(t2));
}

void tst_QTime::operator_lt()
{
    QTime t1(0,0,0,0);
    QTime t2(0,0,0,0);
    QVERIFY( !(t1 < t2) );

    t1 = QTime(12,34,56,20);
    t2 = QTime(12,34,56,30);
    QVERIFY( t1 < t2 );

    t1 = QTime(13,34,46,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( t1 < t2 );

    t1 = QTime(13,24,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( t1 < t2 );

    t1 = QTime(12,34,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( t1 < t2 );

    t1 = QTime(14,34,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( !(t1 < t2) );

    t1 = QTime(13,44,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( !(t1 < t2) );

    t1 = QTime(13,34,56,20);
    t2 = QTime(13,34,46,20);
    QVERIFY( !(t1 < t2) );

    t1 = QTime(13,44,56,30);
    t2 = QTime(13,44,56,20);
    QVERIFY( !(t1 < t2) );
}

void tst_QTime::operator_gt()
{
    QTime t1(0,0,0,0);
    QTime t2(0,0,0,0);
    QVERIFY( !(t1 > t2) );

    t1 = QTime(12,34,56,20);
    t2 = QTime(12,34,56,30);
    QVERIFY( !(t1 > t2) );

    t1 = QTime(13,34,46,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( !(t1 > t2) );

    t1 = QTime(13,24,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( !(t1 > t2) );

    t1 = QTime(12,34,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( !(t1 > t2) );

    t1 = QTime(14,34,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( t1 > t2 );

    t1 = QTime(13,44,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( t1 > t2 );

    t1 = QTime(13,34,56,20);
    t2 = QTime(13,34,46,20);
    QVERIFY( t1 > t2 );

    t1 = QTime(13,44,56,30);
    t2 = QTime(13,44,56,20);
    QVERIFY( t1 > t2 );
}

void tst_QTime::operator_lt_eq()
{
    QTime t1(0,0,0,0);
    QTime t2(0,0,0,0);
    QVERIFY( t1 <= t2 );

    t1 = QTime(12,34,56,20);
    t2 = QTime(12,34,56,30);
    QVERIFY( t1 <= t2 );

    t1 = QTime(13,34,46,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( t1 <= t2 );

    t1 = QTime(13,24,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( t1 <= t2 );

    t1 = QTime(12,34,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( t1 <= t2 );

    t1 = QTime(14,34,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( !(t1 <= t2) );

    t1 = QTime(13,44,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( !(t1 <= t2) );

    t1 = QTime(13,34,56,20);
    t2 = QTime(13,34,46,20);
    QVERIFY( !(t1 <= t2) );

    t1 = QTime(13,44,56,30);
    t2 = QTime(13,44,56,20);
    QVERIFY( !(t1 <= t2) );
}

void tst_QTime::operator_gt_eq()
{
    QTime t1(0,0,0,0);
    QTime t2(0,0,0,0);
    QVERIFY( t1 >= t2 );

    t1 = QTime(12,34,56,20);
    t2 = QTime(12,34,56,30);
    QVERIFY( !(t1 >= t2) );

    t1 = QTime(13,34,46,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( !(t1 >= t2) );

    t1 = QTime(13,24,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( !(t1 >= t2) );

    t1 = QTime(12,34,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( !(t1 >= t2) );

    t1 = QTime(14,34,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( t1 >= t2 );

    t1 = QTime(13,44,56,20);
    t2 = QTime(13,34,56,20);
    QVERIFY( t1 >= t2 );

    t1 = QTime(13,34,56,20);
    t2 = QTime(13,34,46,20);
    QVERIFY( t1 >= t2 );

    t1 = QTime(13,44,56,30);
    t2 = QTime(13,44,56,20);
    QVERIFY( t1 >= t2 );
}

void tst_QTime::fromStringFormat_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QTime>("expected");

    QTest::newRow("data0") << QString("1010") << QString("mmm") << QTime(0, 10, 0);
    QTest::newRow("data1") << QString("00") << QString("hm") << invalidTime();
    QTest::newRow("data2") << QString("10am") << QString("hap") << QTime(10, 0, 0);
    QTest::newRow("data3") << QString("10pm") << QString("hap") << QTime(22, 0, 0);
    QTest::newRow("data4") << QString("10pmam") << QString("hapap") << invalidTime();
    QTest::newRow("data5") << QString("1070") << QString("hhm") << invalidTime();
    QTest::newRow("data6") << QString("1011") << QString("hh") << invalidTime();
    QTest::newRow("data7") << QString("25") << QString("hh") << invalidTime();
    QTest::newRow("data8") << QString("22pm") << QString("Hap") << QTime(22, 0, 0);
    QTest::newRow("data9") << QString("2221") << QString("hhhh") << invalidTime();
    QTest::newRow("data10") << QString("02:23PM") << QString("hh:mmAP") << QTime(14,23,0,0);
    QTest::newRow("data11") << QString("02:23pm") << QString("hh:mmap") << QTime(14,23,0,0);
    QTest::newRow("short-msecs-lt100") << QString("10:12:34:045") << QString("hh:m:ss:z") << QTime(10,12,34,45);
    QTest::newRow("short-msecs-gt100") << QString("10:12:34:45") << QString("hh:m:ss:z") << QTime(10,12,34,450);
    QTest::newRow("late") << QString("23:59:59.999") << QString("hh:mm:ss.z") << QTime(23, 59, 59, 999);
}

void tst_QTime::fromStringFormat()
{
    QFETCH(QString, string);
    QFETCH(QString, format);
    QFETCH(QTime, expected);

    QTime dt = QTime::fromString(string, format);
    QCOMPARE(dt, expected);
}

void tst_QTime::fromStringDateFormat_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<Qt::DateFormat>("format");
    QTest::addColumn<QTime>("expected");

    QTest::newRow("TextDate - data0") << QString("00:00:00") << Qt::TextDate << QTime(0,0,0,0);
    QTest::newRow("TextDate - data1") << QString("10:12:34") << Qt::TextDate << QTime(10,12,34,0);
    QTest::newRow("TextDate - data2") << QString("19:03:54.998601") << Qt::TextDate << QTime(19, 3, 54, 999);
    QTest::newRow("TextDate - data3") << QString("19:03:54.999601") << Qt::TextDate << QTime(19, 3, 54, 999);
    QTest::newRow("TextDate - data4") << QString("10:12") << Qt::TextDate << QTime(10, 12, 0, 0);
    QTest::newRow("TextDate - invalid, minutes") << QString::fromLatin1("23:XX:00") << Qt::TextDate << invalidTime();
    QTest::newRow("TextDate - invalid, minute fraction") << QString::fromLatin1("23:00.123456") << Qt::TextDate << invalidTime();
    QTest::newRow("TextDate - invalid, seconds") << QString::fromLatin1("23:00:XX") << Qt::TextDate << invalidTime();
    QTest::newRow("TextDate - invalid, milliseconds") << QString::fromLatin1("23:01:01:XXXX") << Qt::TextDate << QTime(23, 1, 1, 0);
    QTest::newRow("TextDate - midnight 24") << QString("24:00:00") << Qt::TextDate << QTime();

    QTest::newRow("IsoDate - valid, start of day, omit seconds") << QString::fromLatin1("00:00") << Qt::ISODate << QTime(0, 0, 0);
    QTest::newRow("IsoDate - valid, omit seconds") << QString::fromLatin1("22:21") << Qt::ISODate << QTime(22, 21, 0);
    QTest::newRow("IsoDate - valid, omit seconds (2)") << QString::fromLatin1("23:59") << Qt::ISODate << QTime(23, 59, 0);
    QTest::newRow("IsoDate - valid, end of day") << QString::fromLatin1("23:59:59") << Qt::ISODate << QTime(23, 59, 59);

    QTest::newRow("IsoDate - invalid, empty string") << QString::fromLatin1("") << Qt::ISODate << invalidTime();
    QTest::newRow("IsoDate - invalid, too many hours") << QString::fromLatin1("25:00") << Qt::ISODate << invalidTime();
    QTest::newRow("IsoDate - invalid, too many minutes") << QString::fromLatin1("10:70") << Qt::ISODate << invalidTime();
    // This is a valid time if it happens on June 30 or December 31 (leap seconds).
    QTest::newRow("IsoDate - invalid, too many seconds") << QString::fromLatin1("23:59:60") << Qt::ISODate << invalidTime();
    QTest::newRow("IsoDate - invalid, minutes") << QString::fromLatin1("23:XX:00") << Qt::ISODate << invalidTime();
    QTest::newRow("IsoDate - invalid, not enough minutes") << QString::fromLatin1("23:0") << Qt::ISODate << invalidTime();
    QTest::newRow("IsoDate - invalid, minute fraction") << QString::fromLatin1("23:00,XX") << Qt::ISODate << invalidTime();
    QTest::newRow("IsoDate - invalid, seconds") << QString::fromLatin1("23:00:XX") << Qt::ISODate << invalidTime();
    QTest::newRow("IsoDate - invalid, milliseconds") << QString::fromLatin1("23:01:01:XXXX") << Qt::ISODate << QTime(23, 1, 1, 0);

    QTest::newRow("IsoDate - data0") << QString("00:00:00") << Qt::ISODate << QTime(0,0,0,0);
    QTest::newRow("IsoDate - data1") << QString("10:12:34") << Qt::ISODate << QTime(10,12,34,0);
    QTest::newRow("IsoDate - data2") << QString("19:03:54.998601") << Qt::ISODate << QTime(19, 3, 54, 999);
    QTest::newRow("IsoDate - data3") << QString("19:03:54.999601") << Qt::ISODate << QTime(19, 3, 54, 999);
    QTest::newRow("IsoDate - midnight 24") << QString("24:00:00") << Qt::ISODate << QTime(0, 0, 0, 0);
    QTest::newRow("IsoDate - minute fraction midnight") << QString("24:00,0") << Qt::ISODate << QTime(0, 0, 0, 0);

    // Test Qt::RFC2822Date format (RFC 2822).
    QTest::newRow("RFC 2822") << QString::fromLatin1("13 Feb 1987 13:24:51 +0100")
        << Qt::RFC2822Date << QTime(13, 24, 51);
    QTest::newRow("RFC 2822 with day") << QString::fromLatin1("Thu, 01 Jan 1970 00:12:34 +0000")
        << Qt::RFC2822Date << QTime(0, 12, 34);
    // No timezone
    QTest::newRow("RFC 2822 no timezone") << QString::fromLatin1("01 Jan 1970 00:12:34")
        << Qt::RFC2822Date << QTime(0, 12, 34);
    // No time specified
    QTest::newRow("RFC 2822 date only") << QString::fromLatin1("01 Nov 2002")
        << Qt::RFC2822Date << invalidTime();
    QTest::newRow("RFC 2822 with day date only") << QString::fromLatin1("Fri, 01 Nov 2002")
        << Qt::RFC2822Date << invalidTime();
    // Test invalid month, day, year
    QTest::newRow("RFC 2822 invalid month name") << QString::fromLatin1("13 Fev 1987 13:24:51 +0100")
        << Qt::RFC2822Date << QTime(13, 24, 51);
    QTest::newRow("RFC 2822 invalid day") << QString::fromLatin1("36 Fev 1987 13:24:51 +0100")
        << Qt::RFC2822Date << QTime(13, 24, 51);
    QTest::newRow("RFC 2822 invalid year") << QString::fromLatin1("13 Fev 0000 13:24:51 +0100")
        << Qt::RFC2822Date << QTime(13, 24, 51);
    // Test invalid characters (should ignore invalid characters at end of string).
    QTest::newRow("RFC 2822 invalid character at end") << QString::fromLatin1("01 Jan 2012 08:00:00 +0100!")
        << Qt::RFC2822Date << QTime(8, 0, 0);
    QTest::newRow("RFC 2822 invalid character at front") << QString::fromLatin1("!01 Jan 2012 08:00:00 +0000")
        << Qt::RFC2822Date << invalidTime();
    QTest::newRow("RFC 2822 invalid character both ends") << QString::fromLatin1("!01 Jan 2012 08:00:00 +0000!")
        << Qt::RFC2822Date << invalidTime();
    QTest::newRow("RFC 2822 invalid character at front, 2 at back") << QString::fromLatin1("!01 Jan 2012 08:00:00 +0000..")
        << Qt::RFC2822Date << invalidTime();
    QTest::newRow("RFC 2822 invalid character 2 at front") << QString::fromLatin1("!!01 Jan 2012 08:00:00 +0000")
        << Qt::RFC2822Date << invalidTime();

    // Test Qt::RFC2822Date format (RFC 850 and 1036).
    QTest::newRow("RFC 850 and 1036") << QString::fromLatin1("Fri Feb 13 13:24:51 1987 +0100")
        << Qt::RFC2822Date << QTime(13, 24, 51);
    // No timezone
    QTest::newRow("RFC 850 and 1036 no timezone") << QString::fromLatin1("Thu Jan 01 00:12:34 1970")
        << Qt::RFC2822Date << QTime(0, 12, 34);
    // No time specified
    QTest::newRow("RFC 850 and 1036 date only") << QString::fromLatin1("Fri Nov 01 2002")
        << Qt::RFC2822Date << invalidTime();
    // Test invalid characters (should ignore invalid characters at end of string).
    QTest::newRow("RFC 850 and 1036 invalid character at end") << QString::fromLatin1("Sun Jan 01 08:00:00 2012 +0100!")
        << Qt::RFC2822Date << QTime(8, 0, 0);
    QTest::newRow("RFC 850 and 1036 invalid character at front") << QString::fromLatin1("!Sun Jan 01 08:00:00 2012 +0000")
        << Qt::RFC2822Date << invalidTime();
    QTest::newRow("RFC 850 and 1036 invalid character both ends") << QString::fromLatin1("!Sun Jan 01 08:00:00 2012 +0000!")
        << Qt::RFC2822Date << invalidTime();
    QTest::newRow("RFC 850 and 1036 invalid character at front, 2 at back") << QString::fromLatin1("!Sun Jan 01 08:00:00 2012 +0000..")
        << Qt::RFC2822Date << invalidTime();

    QTest::newRow("RFC empty") << QString::fromLatin1("") << Qt::RFC2822Date << invalidTime();
}

void tst_QTime::fromStringDateFormat()
{
    QFETCH(QString, string);
    QFETCH(Qt::DateFormat, format);
    QFETCH(QTime, expected);

    QTime dt = QTime::fromString(string, format);
    QCOMPARE(dt, expected);
}

void tst_QTime::toStringDateFormat_data()
{
    QTest::addColumn<QTime>("time");
    QTest::addColumn<Qt::DateFormat>("format");
    QTest::addColumn<QString>("expected");

    QTest::newRow("00:00:00.000") << QTime(0, 0, 0, 0) << Qt::TextDate << QString("00:00:00");
    QTest::newRow("ISO 00:00:00.000") << QTime(0, 0, 0, 0) << Qt::ISODate << QString("00:00:00");
    QTest::newRow("Text 10:12:34.000") << QTime(10, 12, 34, 0) << Qt::TextDate << QString("10:12:34");
    QTest::newRow("ISO 10:12:34.000") << QTime(10, 12, 34, 0) << Qt::ISODate << QString("10:12:34");
    QTest::newRow("Text 10:12:34.001") << QTime(10, 12, 34, 001) << Qt::TextDate << QString("10:12:34");
    QTest::newRow("ISO 10:12:34.001") << QTime(10, 12, 34, 001) << Qt::ISODate << QString("10:12:34");
    QTest::newRow("Text 10:12:34.999") << QTime(10, 12, 34, 999) << Qt::TextDate << QString("10:12:34");
    QTest::newRow("ISO 10:12:34.999") << QTime(10, 12, 34, 999) << Qt::ISODate << QString("10:12:34");
    QTest::newRow("RFC2822Date") << QTime(10, 12, 34, 999) << Qt::RFC2822Date << QString("10:12:34");
    QTest::newRow("ISOWithMs 10:12:34.000") << QTime(10, 12, 34, 0) << Qt::ISODateWithMs << QString("10:12:34.000");
    QTest::newRow("ISOWithMs 10:12:34.020") << QTime(10, 12, 34, 20) << Qt::ISODateWithMs << QString("10:12:34.020");
    QTest::newRow("ISOWithMs 10:12:34.999") << QTime(10, 12, 34, 999) << Qt::ISODateWithMs << QString("10:12:34.999");
}

void tst_QTime::toStringDateFormat()
{
    QFETCH(QTime, time);
    QFETCH(Qt::DateFormat, format);
    QFETCH(QString, expected);

    QCOMPARE(time.toString(format), expected);
}

void tst_QTime::toStringFormat_data()
{
    QTest::addColumn<QTime>("t");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("str");

    QTest::newRow( "midnight" ) << QTime(0,0,0,0) << QString("h:m:s:z") << QString("0:0:0:0");
    QTest::newRow( "full" ) << QTime(10,12,34,53) << QString("hh:mm:ss:zzz") << QString("10:12:34:053");
    QTest::newRow( "short-msecs-lt100" ) << QTime(10,12,34,45) << QString("hh:m:ss:z") << QString("10:12:34:045");
    QTest::newRow( "short-msecs-gt100" ) << QTime(10,12,34,450) << QString("hh:m:ss:z") << QString("10:12:34:45");
    QTest::newRow( "am-pm" ) << QTime(10,12,34,45) << QString("hh:ss ap") << QString("10:34 am");
    QTest::newRow( "AM-PM" ) << QTime(22,12,34,45) << QString("hh:zzz AP") << QString("10:045 PM");
    QTest::newRow( "invalid" ) << QTime(230,230,230,230) << QString("hh:mm:ss") << QString();
}

void tst_QTime::toStringFormat()
{
    QFETCH( QTime, t );
    QFETCH( QString, format );
    QFETCH( QString, str );

    QCOMPARE( t.toString( format ), str );
}

void tst_QTime::toStringLocale()
{
    QTime time(18, 30);
    QCOMPARE(time.toString(Qt::SystemLocaleShortDate),
                QLocale::system().toString(time, QLocale::ShortFormat));
    QCOMPARE(time.toString(Qt::DefaultLocaleShortDate),
                QLocale().toString(time, QLocale::ShortFormat));
    QCOMPARE(time.toString(Qt::SystemLocaleLongDate),
                QLocale::system().toString(time, QLocale::LongFormat));
    QCOMPARE(time.toString(Qt::DefaultLocaleLongDate),
                QLocale().toString(time, QLocale::LongFormat));
    QLocale::setDefault(QLocale::German);
    QCOMPARE(time.toString(Qt::SystemLocaleShortDate),
                QLocale::system().toString(time, QLocale::ShortFormat));
    QCOMPARE(time.toString(Qt::DefaultLocaleShortDate),
                QLocale().toString(time, QLocale::ShortFormat));
    QCOMPARE(time.toString(Qt::SystemLocaleLongDate),
                QLocale::system().toString(time, QLocale::LongFormat));
    QCOMPARE(time.toString(Qt::DefaultLocaleLongDate),
                QLocale().toString(time, QLocale::LongFormat));
}

void tst_QTime::msecsSinceStartOfDay_data()
{
    QTest::addColumn<int>("msecs");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<int>("hour");
    QTest::addColumn<int>("minute");
    QTest::addColumn<int>("second");
    QTest::addColumn<int>("msec");

    QTest::newRow("00:00:00.000") << 0 << true
                                  << 0 << 0 << 0 << 0;
    QTest::newRow("01:00:00.001") << ((1 * 3600 * 1000) + 1) << true
                                  << 1 << 0 << 0 << 1;
    QTest::newRow("03:04:05.678") << ((3 * 3600 + 4 * 60 + 5) * 1000 + 678) << true
                                  << 3 << 4 << 5 << 678;
    QTest::newRow("23:59:59.999") << ((23 * 3600 + 59 * 60 + 59) * 1000 + 999) << true
                                  << 23 << 59 << 59 << 999;
    QTest::newRow("24:00:00.000") << ((24 * 3600) * 1000) << false
                                  << -1 << -1 << -1 << -1;
    QTest::newRow("-1 invalid")   << -1 << false
                                  << -1 << -1 << -1 << -1;
}

void tst_QTime::msecsSinceStartOfDay()
{
    QFETCH(int, msecs);
    QFETCH(bool, isValid);
    QFETCH(int, hour);
    QFETCH(int, minute);
    QFETCH(int, second);
    QFETCH(int, msec);

    QTime time = QTime::fromMSecsSinceStartOfDay(msecs);
    QCOMPARE(time.isValid(), isValid);
    if (msecs >= 0)
        QCOMPARE(time.msecsSinceStartOfDay(), msecs);
    else
        QCOMPARE(time.msecsSinceStartOfDay(), 0);
    QCOMPARE(time.hour(), hour);
    QCOMPARE(time.minute(), minute);
    QCOMPARE(time.second(), second);
    QCOMPARE(time.msec(), msec);
}

QTEST_APPLESS_MAIN(tst_QTime)
#include "tst_qtime.moc"
