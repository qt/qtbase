// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qglobal_p.h>
#include <private/qcomparisontesthelper_p.h>
#include <QTest>
#include "qdatetime.h"
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#  include <locale.h>
#endif

using namespace Qt::StringLiterals;

class tst_QTime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
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
    void orderingCompiles();
    void operator_eq_eq_data();
    void operator_eq_eq();
    void ordering_data();
    void ordering();
#if QT_CONFIG(datestring)
# if QT_CONFIG(datetimeparser)
    void fromStringFormat_data();
    void fromStringFormat();
# endif
    void fromStringDateFormat_data();
    void fromStringDateFormat();
    void toStringDateFormat_data();
    void toStringDateFormat();
    void toStringFormat_data();
    void toStringFormat();
#endif
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
    QTest::newRow("overflow")
        << QTime(0,0,0) << (INT_MAX / 1000 + 1)
        << QTime::fromMSecsSinceStartOfDay(((INT_MAX / 1000 + 1) % 86400) * 1000);
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
    QTest::newRow("disregard msec (-0s)") << QTime(12, 30, 1, 900) << QTime(12, 30, 1, 500) << 0;
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

void tst_QTime::orderingCompiles()
{
    QTestPrivate::testAllComparisonOperatorsCompile<QTime>();
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

    QT_TEST_EQUALITY_OPS(t1, t2, expectEqual);

    if (expectEqual)
        QVERIFY(qHash(t1) == qHash(t2));
}

void tst_QTime::ordering_data()
{
    QTest::addColumn<QTime>("left");
    QTest::addColumn<QTime>("right");
    QTest::addColumn<Qt::strong_ordering>("expectedOrdering");

    auto generateRow = [](QTime t1, QTime t2, Qt::strong_ordering ordering) {
        const QByteArray t1Str = t1.toString(u"hh:mm:ss.zz").toLatin1();
        const QByteArray t2Str = t2.toString(u"hh:mm:ss.zz").toLatin1();
        QTest::addRow("%s_vs_%s", t1Str.constData(), t2Str.constData()) << t1 << t2 << ordering;
    };

    generateRow(QTime(0, 0), QTime(0, 0), Qt::strong_ordering::equivalent);
    generateRow(QTime(12, 34, 56, 20), QTime(12, 34, 56, 30), Qt::strong_ordering::less);
    generateRow(QTime(13, 34, 46, 20), QTime(13, 34, 56, 20), Qt::strong_ordering::less);
    generateRow(QTime(13, 24, 56, 20), QTime(13, 34, 56, 20), Qt::strong_ordering::less);
    generateRow(QTime(12, 34, 56, 20), QTime(13, 34, 56, 20), Qt::strong_ordering::less);
    generateRow(QTime(14, 34, 56, 20), QTime(13, 34, 56, 20), Qt::strong_ordering::greater);
    generateRow(QTime(13, 44, 56, 20), QTime(13, 34, 56, 20), Qt::strong_ordering::greater);
    generateRow(QTime(13, 34, 56, 20), QTime(13, 34, 46, 20), Qt::strong_ordering::greater);
    generateRow(QTime(13, 34, 56, 30), QTime(13, 34, 56, 20), Qt::strong_ordering::greater);
}

void tst_QTime::ordering()
{
    QFETCH(QTime, left);
    QFETCH(QTime, right);
    QFETCH(Qt::strong_ordering, expectedOrdering);

    QT_TEST_ALL_COMPARISON_OPS(left, right, expectedOrdering);
}

#if QT_CONFIG(datestring)
# if QT_CONFIG(datetimeparser)
void tst_QTime::fromStringFormat_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QTime>("expected");

    QTest::newRow("data0") << u"1010"_s << u"mmm"_s << QTime(0, 10, 0);
    QTest::newRow("data1") << u"00"_s << u"hm"_s << QTime(0, 0);
    QTest::newRow("data2") << u"10am"_s << u"hap"_s << QTime(10, 0, 0);
    QTest::newRow("data3") << u"10pm"_s << u"hap"_s << QTime(22, 0, 0);
    QTest::newRow("data4") << u"10pmam"_s << u"hapap"_s << invalidTime();
    QTest::newRow("data5") << u"1070"_s << u"hhm"_s << invalidTime();
    QTest::newRow("data6") << u"1011"_s << u"hh"_s << invalidTime();
    QTest::newRow("data7") << u"25"_s << u"hh"_s << invalidTime();
    QTest::newRow("data8") << u"22pm"_s << u"Hap"_s << QTime(22, 0, 0);
    QTest::newRow("data9") << u"2221"_s << u"hhhh"_s << invalidTime();
    // Parsing of am/pm indicators is case-insensitive
    QTest::newRow("pm-upper") << u"02:23PM"_s << u"hh:mmAp"_s << QTime(14, 23);
    QTest::newRow("pm-lower") << u"02:23pm"_s << u"hh:mmaP"_s << QTime(14, 23);
    QTest::newRow("pm-as-upper") << u"02:23Pm"_s << u"hh:mmAP"_s << QTime(14, 23);
    QTest::newRow("pm-as-lower") << u"02:23pM"_s << u"hh:mmap"_s << QTime(14, 23);
    // Millisecond parsing must interpolate 0s only at the end and notice them at the start.
    QTest::newRow("short-msecs-lt100")
        << u"10:12:34:045"_s << u"hh:m:ss:z"_s << QTime(10, 12, 34, 45);
    QTest::newRow("short-msecs-gt100")
        << u"10:12:34:45"_s << u"hh:m:ss:z"_s << QTime(10, 12, 34, 450);
    QTest::newRow("late")
        << u"23:59:59.999"_s << u"hh:mm:ss.z"_s << QTime(23, 59, 59, 999);
    QTest::newRow("ms-0pad") // QTBUG-147354
        << u"090924.000"_s << u"hhmmss.z"_s << QTime(9, 9, 24, 0);

    // Test unicode handling.
    QTest::newRow("emoji in format string 1")
        << u"12👍31:25.05"_s << u"hh👍mm:ss.z"_s << QTime(12, 31, 25, 50);
    QTest::newRow("emoji in format string 2")
        << u"💖12👍31🌈25😺05🚀"_s << u"💖hh👍mm🌈ss😺z🚀"_s << QTime(12, 31, 25, 50);
}

void tst_QTime::fromStringFormat()
{
    QFETCH(QString, string);
    QFETCH(QString, format);
    QFETCH(QTime, expected);

    QTime dt = QTime::fromString(string, format);
    QCOMPARE(dt, expected);
}
# endif // datetimeparser

void tst_QTime::fromStringDateFormat_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<Qt::DateFormat>("format");
    QTest::addColumn<QTime>("expected");

    QTest::newRow("TextDate - zero") << u"00:00:00"_s << Qt::TextDate << QTime(0, 0);
    QTest::newRow("TextDate - ordinary")
        << u"10:12:34"_s << Qt::TextDate << QTime(10, 12, 34);
    QTest::newRow("TextDate - milli-max")
        << u"19:03:54.998601"_s << Qt::TextDate << QTime(19, 3, 54, 999);
    QTest::newRow("TextDate - milli-wrap")
        << u"19:03:54.999601"_s << Qt::TextDate << QTime(19, 3, 55);
    QTest::newRow("TextDate - no-secs")
        << u"10:12"_s << Qt::TextDate << QTime(10, 12);
    QTest::newRow("TextDate - midnight-nowrap")
        << u"23:59:59.9999"_s << Qt::TextDate << QTime(23, 59, 59, 999);
    QTest::newRow("TextDate - invalid, minutes") << u"23:XX:00"_s << Qt::TextDate << invalidTime();
    QTest::newRow("TextDate - invalid, minute fraction") << u"23:00.123456"_s << Qt::TextDate << invalidTime();
    QTest::newRow("TextDate - invalid, seconds") << u"23:00:XX"_s << Qt::TextDate << invalidTime();
    QTest::newRow("TextDate - invalid, milliseconds") << u"23:01:01:XXXX"_s << Qt::TextDate
        << invalidTime();
    QTest::newRow("TextDate - midnight 24") << u"24:00:00"_s << Qt::TextDate << QTime();

    QTest::newRow("IsoDate - valid, start of day, omit seconds") << u"00:00"_s << Qt::ISODate << QTime(0, 0, 0);
    QTest::newRow("IsoDate - valid, omit seconds") << u"22:21"_s << Qt::ISODate << QTime(22, 21, 0);
    QTest::newRow("IsoDate - minute fraction") // 60 * 0.816666 = 48.99996 should round up:
        << u"22:21.816666"_s << Qt::ISODate << QTime(22, 21, 49);
    QTest::newRow("IsoDate - valid, omit seconds (2)") << u"23:59"_s << Qt::ISODate << QTime(23, 59, 0);
    QTest::newRow("IsoDate - valid, end of day") << u"23:59:59"_s << Qt::ISODate << QTime(23, 59, 59);

    QTest::newRow("IsoDate - invalid, empty string") << u""_s << Qt::ISODate << invalidTime();
    QTest::newRow("IsoDate - invalid, too many hours") << u"25:00"_s << Qt::ISODate << invalidTime();
    QTest::newRow("IsoDate - invalid, too many minutes") << u"10:70"_s << Qt::ISODate << invalidTime();
    // This is a valid time if it happens on June 30 or December 31 (leap seconds).
    QTest::newRow("IsoDate - invalid, too many seconds") << u"23:59:60"_s << Qt::ISODate << invalidTime();
    QTest::newRow("IsoDate - invalid, minutes") << u"23:XX:00"_s << Qt::ISODate << invalidTime();
    QTest::newRow("IsoDate - invalid, not enough minutes") << u"23:0"_s << Qt::ISODate << invalidTime();
    QTest::newRow("IsoDate - invalid, minute fraction") << u"23:00,XX"_s << Qt::ISODate << invalidTime();
    QTest::newRow("IsoDate - invalid, seconds") << u"23:00:XX"_s << Qt::ISODate << invalidTime();
    QTest::newRow("IsoDate - invalid, milliseconds") << u"23:01:01:XXXX"_s << Qt::ISODate
        << invalidTime();
    QTest::newRow("IsoDate - zero") << u"00:00:00"_s << Qt::ISODate << QTime(0, 0);
    QTest::newRow("IsoDate - ordinary") << u"10:12:34"_s << Qt::ISODate << QTime(10, 12, 34);
    QTest::newRow("IsoDate - milli-max")
        << u"19:03:54.998601"_s << Qt::ISODate << QTime(19, 3, 54, 999);
    QTest::newRow("IsoDate - milli-wrap")
        << u"19:03:54.999601"_s << Qt::ISODate << QTime(19, 3, 55);
    QTest::newRow("IsoDate - midnight-nowrap")
        << u"23:59:59.9999"_s << Qt::ISODate << QTime(23, 59, 59, 999);
    QTest::newRow("IsoDate - midnight 24")
        << u"24:00:00"_s << Qt::ISODate << QTime(0, 0);
    QTest::newRow("IsoDate - minute fraction midnight")
        << u"24:00,0"_s << Qt::ISODate << QTime(0, 0);

    // Test Qt::RFC2822Date format (RFC 2822).
    QTest::newRow("RFC 2822") << u"13 Feb 1987 13:24:51 +0100"_s
        << Qt::RFC2822Date << QTime(13, 24, 51);
    QTest::newRow("RFC 2822 after space")
        << u" 13 Feb 1987 13:24:51 +0100"_s
        << Qt::RFC2822Date << QTime(13, 24, 51);
    QTest::newRow("RFC 2822 with day") << u"Thu, 01 Jan 1970 00:12:34 +0000"_s
        << Qt::RFC2822Date << QTime(0, 12, 34);
    QTest::newRow("RFC 2822 with day after space")
        << u" Thu, 01 Jan 1970 00:12:34 +0000"_s
        << Qt::RFC2822Date << QTime(0, 12, 34);
    // No timezone
    QTest::newRow("RFC 2822 no timezone") << u"01 Jan 1970 00:12:34"_s
        << Qt::RFC2822Date << QTime(0, 12, 34);
    // No time specified
    QTest::newRow("RFC 2822 date only") << u"01 Nov 2002"_s
        << Qt::RFC2822Date << invalidTime();
    QTest::newRow("RFC 2822 with day date only") << u"Fri, 01 Nov 2002"_s
        << Qt::RFC2822Date << invalidTime();
    QTest::newRow("RFC 2822 malformed time")
        << u"01 Nov 2002 0:"_s << Qt::RFC2822Date << QTime();
    // Test invalid month, day, year are ignored:
    QTest::newRow("RFC 2822 invalid month name") << u"13 Fev 1987 13:24:51 +0100"_s
        << Qt::RFC2822Date << QTime(13, 24, 51);
    QTest::newRow("RFC 2822 invalid day") << u"36 Feb 1987 13:24:51 +0100"_s
        << Qt::RFC2822Date << QTime(13, 24, 51);
    QTest::newRow("RFC 2822 invalid day name") << u"Mud, 23 Feb 1987 13:24:51 +0100"_s
        << Qt::RFC2822Date << QTime(13, 24, 51);
    QTest::newRow("RFC 2822 invalid year") << u"13 Feb 0000 13:24:51 +0100"_s
        << Qt::RFC2822Date << QTime(13, 24, 51);
    // Test invalid characters:
    QTest::newRow("RFC 2822 invalid character at end")
        << u"01 Jan 2012 08:00:00 +0100!"_s
        << Qt::RFC2822Date << invalidTime();
    QTest::newRow("RFC 2822 invalid character at front")
        << u"!01 Jan 2012 08:00:00 +0100"_s
        << Qt::RFC2822Date << invalidTime();
    QTest::newRow("RFC 2822 invalid character both ends")
        << u"!01 Jan 2012 08:00:00 +0100!"_s
        << Qt::RFC2822Date << invalidTime();
    QTest::newRow("RFC 2822 invalid character at front, 2 at back")
        << u"!01 Jan 2012 08:00:00 +0100.."_s
        << Qt::RFC2822Date << invalidTime();
    QTest::newRow("RFC 2822 invalid character 2 at front")
        << u"!!01 Jan 2012 08:00:00 +0100"_s
        << Qt::RFC2822Date << invalidTime();
    // The common date text used by the "invalid character" tests, just to be
    // sure *it's* not what's invalid:
    QTest::newRow("RFC 2822 (not invalid)")
        << u"01 Jan 2012 08:00:00 +0100"_s
        << Qt::RFC2822Date << QTime(8, 0, 0);

    // Test Qt::RFC2822Date format (RFC 850 and 1036, permissive).
    QTest::newRow("RFC 850 and 1036") << u"Fri Feb 13 13:24:51 1987 +0100"_s
        << Qt::RFC2822Date << QTime(13, 24, 51);
    QTest::newRow("RFC 850 and 1036 after space")
        << u" Fri Feb 13 13:24:51 1987 +0100"_s
        << Qt::RFC2822Date << QTime(13, 24, 51);
    // No timezone
    QTest::newRow("RFC 850 and 1036 no timezone") << u"Thu Jan 01 00:12:34 1970"_s
        << Qt::RFC2822Date << QTime(0, 12, 34);
    // No time specified
    QTest::newRow("RFC 850 and 1036 date only") << u"Fri Nov 01 2002"_s
        << Qt::RFC2822Date << invalidTime();
    // Test invalid characters.
    QTest::newRow("RFC 850 and 1036 invalid character at end")
        << u"Sun Jan 01 08:00:00 2012 +0100!"_s
        << Qt::RFC2822Date << invalidTime();
    QTest::newRow("RFC 850 and 1036 invalid character at front")
        << u"!Sun Jan 01 08:00:00 2012 +0100"_s
        << Qt::RFC2822Date << invalidTime();
    QTest::newRow("RFC 850 and 1036 invalid character both ends")
        << u"!Sun Jan 01 08:00:00 2012 +0100!"_s
        << Qt::RFC2822Date << invalidTime();
    QTest::newRow("RFC 850 and 1036 invalid character at front, 2 at back")
        << u"!Sun Jan 01 08:00:00 2012 +0100.."_s
        << Qt::RFC2822Date << invalidTime();
    // The common date text used by the "invalid character" tests, just to be
    // sure *it's* not what's invalid:
    QTest::newRow("RFC 850 and 1036 no invalid character")
        << u"Sun Jan 01 08:00:00 2012 +0100"_s
        << Qt::RFC2822Date << QTime(8, 0, 0);

    QTest::newRow("RFC empty") << u""_s << Qt::RFC2822Date << invalidTime();
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

    QTest::newRow("00:00:00.000") << QTime(0, 0, 0, 0) << Qt::TextDate << u"00:00:00"_s;
    QTest::newRow("ISO 00:00:00.000") << QTime(0, 0, 0, 0) << Qt::ISODate << u"00:00:00"_s;
    QTest::newRow("Text 10:12:34.000") << QTime(10, 12, 34, 0) << Qt::TextDate << u"10:12:34"_s;
    QTest::newRow("ISO 10:12:34.000") << QTime(10, 12, 34, 0) << Qt::ISODate << u"10:12:34"_s;
    QTest::newRow("Text 10:12:34.001") << QTime(10, 12, 34, 001) << Qt::TextDate << u"10:12:34"_s;
    QTest::newRow("ISO 10:12:34.001") << QTime(10, 12, 34, 001) << Qt::ISODate << u"10:12:34"_s;
    QTest::newRow("Text 10:12:34.999") << QTime(10, 12, 34, 999) << Qt::TextDate << u"10:12:34"_s;
    QTest::newRow("ISO 10:12:34.999") << QTime(10, 12, 34, 999) << Qt::ISODate << u"10:12:34"_s;
    QTest::newRow("RFC2822Date") << QTime(10, 12, 34, 999) << Qt::RFC2822Date << u"10:12:34"_s;
    QTest::newRow("ISOWithMs 10:12:34.000") << QTime(10, 12, 34, 0) << Qt::ISODateWithMs << u"10:12:34.000"_s;
    QTest::newRow("ISOWithMs 10:12:34.020") << QTime(10, 12, 34, 20) << Qt::ISODateWithMs << u"10:12:34.020"_s;
    QTest::newRow("ISOWithMs 10:12:34.999") << QTime(10, 12, 34, 999) << Qt::ISODateWithMs << u"10:12:34.999"_s;
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

    QTest::newRow( "midnight" ) << QTime(0,0,0,0) << u"h:m:s:z"_s << u"0:0:0:0"_s;
    QTest::newRow( "full" ) << QTime(10,12,34,53) << u"hh:mm:ss:zzz"_s << u"10:12:34:053"_s;
    QTest::newRow( "short-msecs-lt100" ) << QTime(10,12,34,45) << u"hh:m:ss:z"_s << u"10:12:34:045"_s;
    QTest::newRow( "short-msecs-gt100" ) << QTime(10,12,34,450) << u"hh:m:ss:z"_s << u"10:12:34:45"_s;
    QTest::newRow( "am-pm" ) << QTime(10,12,34,45) << u"hh:ss ap"_s << u"10:34 am"_s;
    QTest::newRow( "AM-PM" ) << QTime(22,12,34,45) << u"hh:zzz AP"_s << u"10:045 PM"_s;
    QTest::newRow( "invalid" ) << QTime(230,230,230,230) << u"hh:mm:ss"_s << QString();
    QTest::newRow( "empty format" ) << QTime(4,5,6,6) << u""_s << u""_s;
}

void tst_QTime::toStringFormat()
{
    QFETCH( QTime, t );
    QFETCH( QString, format );
    QFETCH( QString, str );

    QCOMPARE( t.toString( format ), str );
}
#endif // datestring

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
    QCOMPARE(time.msecsSinceStartOfDay(), msecs < 0 ? 0 : msecs);
    QCOMPARE(time.hour(), hour);
    QCOMPARE(time.minute(), minute);
    QCOMPARE(time.second(), second);
    QCOMPARE(time.msec(), msec);
}

QTEST_APPLESS_MAIN(tst_QTime)
#include "tst_qtime.moc"
