/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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
#include <qdatetime.h>
#include <qlocale.h>

class tst_QDate : public QObject
{
    Q_OBJECT
private slots:
    void isNull_data();
    void isNull();
    void isValid_data();
    void isValid();
    void julianDay_data();
    void julianDay();
    void dayOfWeek_data();
    void dayOfWeek();
    void dayOfYear_data();
    void dayOfYear();
    void daysInMonth_data();
    void daysInMonth();
    void daysInYear_data();
    void daysInYear();
    void getDate();
    void weekNumber_invalid_data();
    void weekNumber_invalid();
    void weekNumber_data();
    void weekNumber();
    void julianDaysLimits();
    void addDays_data();
    void addDays();
    void addMonths_data();
    void addMonths();
    void addYears_data();
    void addYears();
    void daysTo();
    void operator_eq_eq_data();
    void operator_eq_eq();
    void operator_lt();
    void operator_gt();
    void operator_lt_eq();
    void operator_gt_eq();
    void operator_insert_extract_data();
    void operator_insert_extract();
    void fromStringDateFormat_data();
    void fromStringDateFormat();
    void fromStringFormat_data();
    void fromStringFormat();
    void toStringFormat_data();
    void toStringFormat();
    void toStringDateFormat_data();
    void toStringDateFormat();
    void isLeapYear();
    void yearsZeroToNinetyNine();
    void negativeYear() const;
    void printNegativeYear() const;
    void roundtripGermanLocale() const;
    void shortDayName() const;
    void standaloneShortDayName() const;
    void longDayName() const;
    void standaloneLongDayName() const;
    void shortMonthName() const;
    void standaloneShortMonthName() const;
    void longMonthName() const;
    void standaloneLongMonthName() const;
    void roundtrip() const;
    void qdebug() const;
private:
    QDate defDate() const { return QDate(1900, 1, 1); }
    QDate invalidDate() const { return QDate(); }
};

Q_DECLARE_METATYPE(Qt::DateFormat)

void tst_QDate::isNull_data()
{
    QTest::addColumn<qint64>("jd");
    QTest::addColumn<bool>("null");

    qint64 minJd = Q_INT64_C(-784350574879);
    qint64 maxJd = Q_INT64_C( 784354017364);

    QTest::newRow("qint64 min") << std::numeric_limits<qint64>::min() << true;
    QTest::newRow("minJd - 1")  << minJd - 1                          << true;
    QTest::newRow("minJd")      << minJd                              << false;
    QTest::newRow("minJd + 1")  << minJd + 1                          << false;
    QTest::newRow("maxJd - 1")  << maxJd - 1                          << false;
    QTest::newRow("maxJd")      << maxJd                              << false;
    QTest::newRow("maxJd + 1")  << maxJd + 1                          << true;
    QTest::newRow("qint64 max") << std::numeric_limits<qint64>::max() << true;
}

void tst_QDate::isNull()
{
    QFETCH(qint64, jd);

    QDate d = QDate::fromJulianDay(jd);
    QTEST(d.isNull(), "null");
}

void tst_QDate::isValid_data()
{
    qint64 nullJd = std::numeric_limits<qint64>::min();

    QTest::addColumn<int>("year");
    QTest::addColumn<int>("month");
    QTest::addColumn<int>("day");
    QTest::addColumn<qint64>("jd");
    QTest::addColumn<bool>("valid");

    QTest::newRow("0-0-0")    <<    0 <<  0 << 0 << nullJd << false;
    QTest::newRow("month 0")  << 2000 <<  0 << 1 << nullJd << false;
    QTest::newRow("day 0")    << 2000 <<  1 << 0 << nullJd << false;

    QTest::newRow("month 13") << 2000 << 13 << 1 << nullJd << false;

    // test leap years
    QTest::newRow("non-leap")            << 2006 <<  2 << 29 << nullJd  << false;
    QTest::newRow("normal leap")         << 2004 <<  2 << 29 << qint64(2453065) << true;
    QTest::newRow("century leap 1900")   << 1900 <<  2 << 29 << nullJd  << false;
    QTest::newRow("century leap 2100")   << 2100 <<  2 << 29 << nullJd  << false;
    QTest::newRow("400-years leap 2000") << 2000 <<  2 << 29 << qint64(2451604) << true;
    QTest::newRow("400-years leap 2400") << 2400 <<  2 << 29 << qint64(2597701) << true;
    QTest::newRow("400-years leap 1600") << 1600 <<  2 << 29 << qint64(2305507) << true;
    QTest::newRow("year 0")              <<    0 <<  2 << 27 << nullJd  << false;

    // test the number of days in months:
    QTest::newRow("jan") << 2000 <<  1 << 31 << qint64(2451575) << true;
    QTest::newRow("feb") << 2000 <<  2 << 29 << qint64(2451604) << true; // same data as 400-years leap
    QTest::newRow("mar") << 2000 <<  3 << 31 << qint64(2451635) << true;
    QTest::newRow("apr") << 2000 <<  4 << 30 << qint64(2451665) << true;
    QTest::newRow("may") << 2000 <<  5 << 31 << qint64(2451696) << true;
    QTest::newRow("jun") << 2000 <<  6 << 30 << qint64(2451726) << true;
    QTest::newRow("jul") << 2000 <<  7 << 31 << qint64(2451757) << true;
    QTest::newRow("aug") << 2000 <<  8 << 31 << qint64(2451788) << true;
    QTest::newRow("sep") << 2000 <<  9 << 30 << qint64(2451818) << true;
    QTest::newRow("oct") << 2000 << 10 << 31 << qint64(2451849) << true;
    QTest::newRow("nov") << 2000 << 11 << 30 << qint64(2451879) << true;
    QTest::newRow("dec") << 2000 << 12 << 31 << qint64(2451910) << true;

    // and invalid dates:
    QTest::newRow("ijan") << 2000 <<  1 << 32 << nullJd << false;
    QTest::newRow("ifeb") << 2000 <<  2 << 30 << nullJd << false;
    QTest::newRow("imar") << 2000 <<  3 << 32 << nullJd << false;
    QTest::newRow("iapr") << 2000 <<  4 << 31 << nullJd << false;
    QTest::newRow("imay") << 2000 <<  5 << 32 << nullJd << false;
    QTest::newRow("ijun") << 2000 <<  6 << 31 << nullJd << false;
    QTest::newRow("ijul") << 2000 <<  7 << 32 << nullJd << false;
    QTest::newRow("iaug") << 2000 <<  8 << 32 << nullJd << false;
    QTest::newRow("isep") << 2000 <<  9 << 31 << nullJd << false;
    QTest::newRow("ioct") << 2000 << 10 << 32 << nullJd << false;
    QTest::newRow("inov") << 2000 << 11 << 31 << nullJd << false;
    QTest::newRow("idec") << 2000 << 12 << 32 << nullJd << false;

    // the beginning of the Julian Day calendar:
    QTest::newRow("jd earliest formula") <<   -4800 <<  1 <<  1 << qint64(   -31738) << true;
    QTest::newRow("jd -1")               <<   -4714 << 11 << 23 << qint64(       -1) << true;
    QTest::newRow("jd 0")                <<   -4714 << 11 << 24 << qint64(        0) << true;
    QTest::newRow("jd 1")                <<   -4714 << 11 << 25 << qint64(        1) << true;
    QTest::newRow("jd latest formula")   << 1400000 << 12 << 31 << qint64(513060925) << true;
}

void tst_QDate::isValid()
{
    QFETCH(int, year);
    QFETCH(int, month);
    QFETCH(int, day);
    QFETCH(qint64, jd);
    QFETCH(bool, valid);

    QCOMPARE(QDate::isValid(year, month, day), valid);

    QDate d;
    d.setDate(year, month, day);
    QCOMPARE(d.isValid(), valid);
    QCOMPARE(d.toJulianDay(), jd);

    if (valid) {
        QCOMPARE(d.year(), year);
        QCOMPARE(d.month(), month);
        QCOMPARE(d.day(), day);
    } else {
        QCOMPARE(d.year(), 0);
        QCOMPARE(d.month(), 0);
        QCOMPARE(d.day(), 0);
    }
}

void tst_QDate::julianDay_data()
{
    isValid_data();
}

void tst_QDate::julianDay()
{
    QFETCH(int, year);
    QFETCH(int, month);
    QFETCH(int, day);
    QFETCH(qint64, jd);

    {
        QDate d;
        d.setDate(year, month, day);
        QCOMPARE(d.toJulianDay(), jd);
    }

    if (jd != std::numeric_limits<qint64>::min()) {
        QDate d = QDate::fromJulianDay(jd);
        QCOMPARE(d.year(), year);
        QCOMPARE(d.month(), month);
        QCOMPARE(d.day(), day);
    }
}

void tst_QDate::dayOfWeek_data()
{
    QTest::addColumn<int>("year");
    QTest::addColumn<int>("month");
    QTest::addColumn<int>("day");
    QTest::addColumn<int>("dayOfWeek");

    QTest::newRow("data0")  <<     0 <<  0 <<  0 << 0;
    QTest::newRow("data1")  <<  2000 <<  1 <<  3 << 1;
    QTest::newRow("data2")  <<  2000 <<  1 <<  4 << 2;
    QTest::newRow("data3")  <<  2000 <<  1 <<  5 << 3;
    QTest::newRow("data4")  <<  2000 <<  1 <<  6 << 4;
    QTest::newRow("data5")  <<  2000 <<  1 <<  7 << 5;
    QTest::newRow("data6")  <<  2000 <<  1 <<  8 << 6;
    QTest::newRow("data7")  <<  2000 <<  1 <<  9 << 7;
    QTest::newRow("data8")  << -4800 <<  1 <<  1 << 1;
    QTest::newRow("data9")  << -4800 <<  1 <<  2 << 2;
    QTest::newRow("data10") << -4800 <<  1 <<  3 << 3;
    QTest::newRow("data11") << -4800 <<  1 <<  4 << 4;
    QTest::newRow("data12") << -4800 <<  1 <<  5 << 5;
    QTest::newRow("data13") << -4800 <<  1 <<  6 << 6;
    QTest::newRow("data14") << -4800 <<  1 <<  7 << 7;
    QTest::newRow("data15") << -4800 <<  1 <<  8 << 1;
}

void tst_QDate::dayOfWeek()
{
    QFETCH(int, year);
    QFETCH(int, month);
    QFETCH(int, day);
    QFETCH(int, dayOfWeek);

    QDate dt(year, month, day);
    QCOMPARE(dt.dayOfWeek(), dayOfWeek);
}

void tst_QDate::dayOfYear_data()
{
    QTest::addColumn<int>("year");
    QTest::addColumn<int>("month");
    QTest::addColumn<int>("day");
    QTest::addColumn<int>("dayOfYear");

    QTest::newRow("data0")  <<     0 <<  0 <<  0 <<   0;
    QTest::newRow("data1")  <<  2000 <<  1 <<  1 <<   1;
    QTest::newRow("data2")  <<  2000 <<  1 <<  2 <<   2;
    QTest::newRow("data3")  <<  2000 <<  1 <<  3 <<   3;
    QTest::newRow("data4")  <<  2000 << 12 << 31 << 366;
    QTest::newRow("data5")  <<  2001 << 12 << 31 << 365;
    QTest::newRow("data6")  <<  1815 <<  1 <<  1 <<   1;
    QTest::newRow("data7")  <<  1815 << 12 << 31 << 365;
    QTest::newRow("data8")  <<  1500 <<  1 <<  1 <<   1;
    QTest::newRow("data9")  <<  1500 << 12 << 31 << 365;
    QTest::newRow("data10") << -1500 <<  1 <<  1 <<   1;
    QTest::newRow("data11") << -1500 << 12 << 31 << 365;
    QTest::newRow("data12") << -4800 <<  1 <<  1 <<   1;
    QTest::newRow("data13") << -4800 << 12 << 31 << 365;
}

void tst_QDate::dayOfYear()
{
    QFETCH(int, year);
    QFETCH(int, month);
    QFETCH(int, day);
    QFETCH(int, dayOfYear);

    QDate dt(year, month, day);
    QCOMPARE(dt.dayOfYear(), dayOfYear);
}

void tst_QDate::daysInMonth_data()
{
    QTest::addColumn<int>("year");
    QTest::addColumn<int>("month");
    QTest::addColumn<int>("day");
    QTest::addColumn<int>("daysInMonth");

    QTest::newRow("data0")  <<     0 <<  0 <<  0 <<   0;
    QTest::newRow("data1")  <<  2000 <<  1 <<  1 <<  31;
    QTest::newRow("data2")  <<  2000 <<  2 <<  1 <<  29;
    QTest::newRow("data3")  <<  2000 <<  3 <<  1 <<  31;
    QTest::newRow("data4")  <<  2000 <<  4 <<  1 <<  30;
    QTest::newRow("data5")  <<  2000 <<  5 <<  1 <<  31;
    QTest::newRow("data6")  <<  2000 <<  6 <<  1 <<  30;
    QTest::newRow("data7")  <<  2000 <<  7 <<  1 <<  31;
    QTest::newRow("data8")  <<  2000 <<  8 <<  1 <<  31;
    QTest::newRow("data9")  <<  2000 <<  9 <<  1 <<  30;
    QTest::newRow("data10") <<  2000 << 10 <<  1 <<  31;
    QTest::newRow("data11") <<  2000 << 11 <<  1 <<  30;
    QTest::newRow("data12") <<  2000 << 12 <<  1 <<  31;
    QTest::newRow("data13") <<  2001 <<  2 <<  1 <<  28;
    QTest::newRow("data14")  <<  2000 <<  0 <<  1 <<   0;
}

void tst_QDate::daysInMonth()
{
    QFETCH(int, year);
    QFETCH(int, month);
    QFETCH(int, day);
    QFETCH(int, daysInMonth);

    QDate dt(year, month, day);
    QCOMPARE(dt.daysInMonth(), daysInMonth);
}

void tst_QDate::daysInYear_data()
{
    QTest::addColumn<QDate>("date");
    QTest::addColumn<int>("expectedDaysInYear");

    QTest::newRow("2000, 1, 1") << QDate(2000, 1, 1) << 366;
    QTest::newRow("2001, 1, 1") << QDate(2001, 1, 1) << 365;
    QTest::newRow("4, 1, 1") << QDate(4, 1, 1) << 366;
    QTest::newRow("5, 1, 1") << QDate(5, 1, 1) << 365;
    QTest::newRow("0, 0, 0") << QDate(0, 0, 0) << 0;
}

void tst_QDate::daysInYear()
{
    QFETCH(QDate, date);
    QFETCH(int, expectedDaysInYear);

    QCOMPARE(date.daysInYear(), expectedDaysInYear);
}

void tst_QDate::getDate()
{
    int y, m, d;
    QDate dt(2000, 1, 1);
    dt.getDate(&y, &m, &d);
    QCOMPARE(y, 2000);
    QCOMPARE(m, 1);
    QCOMPARE(d, 1);
    dt.setDate(0, 0, 0);
    dt.getDate(&y, &m, &d);
    QCOMPARE(y, 0);
    QCOMPARE(m, 0);
    QCOMPARE(d, 0);
}

void tst_QDate::weekNumber_data()
{
    QTest::addColumn<int>("expectedWeekNum");
    QTest::addColumn<int>("expectedYearNum");
    QTest::addColumn<int>("year");
    QTest::addColumn<int>("month");
    QTest::addColumn<int>("day");

    enum { Thursday = 4 };
    bool wasLastYearLong = false;   // 1999 was not a long (53-week) year
    bool isLongYear;

    // full 400-year cycle for Jan 1, 4 and Dec 28, 31
    for (int yr = 2000; yr < 2400; ++yr, wasLastYearLong = isLongYear) {
        QByteArray yrstr = QByteArray::number(yr);
        int wday = QDate(yr, 1, 1).dayOfWeek();

        // the year is 53-week long if Jan 1 is Thursday or, if it's a leap year, a Wednesday
        isLongYear = (wday == Thursday) || (QDate::isLeapYear(yr) && wday == Thursday - 1);

        // Jan 4 is always on week 1
        QTest::newRow(yrstr + "-01-04") << 1 << yr << yr << 1 << 4;

        // Dec 28 is always on the last week
        QTest::newRow(yrstr + "-12-28") << (52 + isLongYear) << yr << yr << 12 << 28;

        // Jan 1 is on either on week 1 or on the last week of the previous year
        QTest::newRow(yrstr + "-01-01")
                << (wday <= Thursday ? 1 : 52 + wasLastYearLong)
                << (wday <= Thursday ? yr : yr - 1)
                << yr << 1 << 1;

        // Dec 31 is either on the last week or week 1 of the next year
        wday = QDate(yr, 12, 31).dayOfWeek();
        QTest::newRow(yrstr + "-12-31")
                << (wday >= Thursday ? 52 + isLongYear : 1)
                << (wday >= Thursday ? yr : yr + 1)
                << yr << 12 << 31;
    }
}

void tst_QDate::weekNumber()
{
    int yearNumber;
    QFETCH( int, year );
    QFETCH( int, month );
    QFETCH( int, day );
    QFETCH( int, expectedWeekNum );
    QFETCH( int, expectedYearNum );
    QDate dt1( year, month, day );
    QCOMPARE( dt1.weekNumber( &yearNumber ), expectedWeekNum );
    QCOMPARE( yearNumber, expectedYearNum );
}

void tst_QDate::weekNumber_invalid_data()
{
    QTest::addColumn<int>("year");
    QTest::addColumn<int>("month");
    QTest::addColumn<int>("day");

    //next we fill it with data
    QTest::newRow( "data0" )  << 0 << 0 << 0;
    QTest::newRow( "data1" )  << 2001 << 1 << 32;
    QTest::newRow( "data2" )  << 1999 << 2 << 29;
}

void tst_QDate::weekNumber_invalid()
{
    QDate dt;
    int yearNumber;
    QCOMPARE( dt.weekNumber( &yearNumber ), 0 );
}

void tst_QDate::julianDaysLimits()
{
    qint64 min = std::numeric_limits<qint64>::min();
    qint64 max = std::numeric_limits<qint64>::max();
    qint64 minJd = Q_INT64_C(-784350574879);
    qint64 maxJd = Q_INT64_C( 784354017364);

    QDate maxDate = QDate::fromJulianDay(maxJd);
    QDate minDate = QDate::fromJulianDay(minJd);
    QDate zeroDate = QDate::fromJulianDay(0);

    QDate dt = QDate::fromJulianDay(min);
    QCOMPARE(dt.isValid(), false);
    dt = QDate::fromJulianDay(minJd - 1);
    QCOMPARE(dt.isValid(), false);
    dt = QDate::fromJulianDay(minJd);
    QCOMPARE(dt.isValid(), true);
    dt = QDate::fromJulianDay(minJd + 1);
    QCOMPARE(dt.isValid(), true);
    dt = QDate::fromJulianDay(maxJd - 1);
    QCOMPARE(dt.isValid(), true);
    dt = QDate::fromJulianDay(maxJd);
    QCOMPARE(dt.isValid(), true);
    dt = QDate::fromJulianDay(maxJd + 1);
    QCOMPARE(dt.isValid(), false);
    dt = QDate::fromJulianDay(max);
    QCOMPARE(dt.isValid(), false);

    dt = maxDate.addDays(1);
    QCOMPARE(dt.isValid(), false);
    dt = maxDate.addDays(0);
    QCOMPARE(dt.isValid(), true);
    dt = maxDate.addDays(-1);
    QCOMPARE(dt.isValid(), true);
    dt = maxDate.addDays(max);
    QCOMPARE(dt.isValid(), false);
    dt = maxDate.addDays(min);
    QCOMPARE(dt.isValid(), false);

    dt = minDate.addDays(-1);
    QCOMPARE(dt.isValid(), false);
    dt = minDate.addDays(0);
    QCOMPARE(dt.isValid(), true);
    dt = minDate.addDays(1);
    QCOMPARE(dt.isValid(), true);
    dt = minDate.addDays(min);
    QCOMPARE(dt.isValid(), false);
    dt = minDate.addDays(max);
    QCOMPARE(dt.isValid(), false);

    dt = zeroDate.addDays(-1);
    QCOMPARE(dt.isValid(), true);
    dt = zeroDate.addDays(0);
    QCOMPARE(dt.isValid(), true);
    dt = zeroDate.addDays(1);
    QCOMPARE(dt.isValid(), true);
    dt = zeroDate.addDays(min);
    QCOMPARE(dt.isValid(), false);
    dt = zeroDate.addDays(max);
    QCOMPARE(dt.isValid(), false);
}

void tst_QDate::addDays()
{
    QFETCH( int, year );
    QFETCH( int, month );
    QFETCH( int, day );
    QFETCH( int, amountToAdd );
    QFETCH( int, expectedYear );
    QFETCH( int, expectedMonth );
    QFETCH( int, expectedDay );

    QDate dt( year, month, day );
    dt = dt.addDays( amountToAdd );

    QCOMPARE( dt.year(), expectedYear );
    QCOMPARE( dt.month(), expectedMonth );
    QCOMPARE( dt.day(), expectedDay );
}

void tst_QDate::addDays_data()
{
    QTest::addColumn<int>("year");
    QTest::addColumn<int>("month");
    QTest::addColumn<int>("day");
    QTest::addColumn<int>("amountToAdd");
    QTest::addColumn<int>("expectedYear");
    QTest::addColumn<int>("expectedMonth");
    QTest::addColumn<int>("expectedDay");

    QTest::newRow( "data0" ) << 2000 << 1 << 1 << 1 << 2000 << 1 << 2;
    QTest::newRow( "data1" ) << 2000 << 1 << 31 << 1 << 2000 << 2 << 1;
    QTest::newRow( "data2" ) << 2000 << 2 << 28 << 1 << 2000 << 2 << 29;
    QTest::newRow( "data3" ) << 2000 << 2 << 29 << 1 << 2000 << 3 << 1;
    QTest::newRow( "data4" ) << 2000 << 12 << 31 << 1 << 2001 << 1 << 1;
    QTest::newRow( "data5" ) << 2001 << 2 << 28 << 1 << 2001 << 3 << 1;
    QTest::newRow( "data6" ) << 2001 << 2 << 28 << 30 << 2001 << 3 << 30;
    QTest::newRow( "data7" ) << 2001 << 3 << 30 << 5 << 2001 << 4 << 4;

    QTest::newRow( "data8" ) << 2000 << 1 << 1 << -1 << 1999 << 12 << 31;
    QTest::newRow( "data9" ) << 2000 << 1 << 31 << -1 << 2000 << 1 << 30;
    QTest::newRow( "data10" ) << 2000 << 2 << 28 << -1 << 2000 << 2 << 27;
    QTest::newRow( "data11" ) << 2001 << 2 << 28 << -30 << 2001 << 1 << 29;

    QTest::newRow( "data12" ) << -4713 << 1 << 2 << -2 << -4714 << 12 << 31;
    QTest::newRow( "data13" ) << -4713 << 1 << 2 <<  2 << -4713 <<  1 <<  4;

    QTest::newRow( "invalid" ) << 0 << 0 << 0 << 1 << 0 << 0 << 0;
}

void tst_QDate::addMonths()
{
    QFETCH( int, year );
    QFETCH( int, month );
    QFETCH( int, day );
    QFETCH( int, amountToAdd );
    QFETCH( int, expectedYear );
    QFETCH( int, expectedMonth );
    QFETCH( int, expectedDay );

    QDate dt( year, month, day );
    dt = dt.addMonths( amountToAdd );

    QCOMPARE( dt.year(), expectedYear );
    QCOMPARE( dt.month(), expectedMonth );
    QCOMPARE( dt.day(), expectedDay );
}

void tst_QDate::addMonths_data()
{
    QTest::addColumn<int>("year");
    QTest::addColumn<int>("month");
    QTest::addColumn<int>("day");
    QTest::addColumn<int>("amountToAdd");
    QTest::addColumn<int>("expectedYear");
    QTest::addColumn<int>("expectedMonth");
    QTest::addColumn<int>("expectedDay");

    QTest::newRow( "data0" ) << 2000 << 1 << 1 << 1 << 2000 << 2 << 1;
    QTest::newRow( "data1" ) << 2000 << 1 << 31 << 1 << 2000 << 2 << 29;
    QTest::newRow( "data2" ) << 2000 << 2 << 28 << 1 << 2000 << 3 << 28;
    QTest::newRow( "data3" ) << 2000 << 2 << 29 << 1 << 2000 << 3 << 29;
    QTest::newRow( "data4" ) << 2000 << 12 << 31 << 1 << 2001 << 1 << 31;
    QTest::newRow( "data5" ) << 2001 << 2 << 28 << 1 << 2001 << 3 << 28;
    QTest::newRow( "data6" ) << 2001 << 2 << 28 << 12 << 2002 << 2 << 28;
    QTest::newRow( "data7" ) << 2000 << 2 << 29 << 12 << 2001 << 2 << 28;
    QTest::newRow( "data8" ) << 2000 << 10 << 15 << 4 << 2001 << 2 << 15;

    QTest::newRow( "data9" ) << 2000 << 1 << 1 << -1 << 1999 << 12 << 1;
    QTest::newRow( "data10" ) << 2000 << 1 << 31 << -1 << 1999 << 12 << 31;
    QTest::newRow( "data11" ) << 2000 << 12 << 31 << -1 << 2000 << 11 << 30;
    QTest::newRow( "data12" ) << 2001 << 2 << 28 << -12 << 2000 << 2 << 28;
    QTest::newRow( "data13" ) << 2000 << 1 << 31 << -7 << 1999 << 6 << 30;
    QTest::newRow( "data14" ) << 2000 << 2 << 29 << -12 << 1999 << 2 << 28;

    // year sign change:
    QTest::newRow( "data15" ) << 1 << 1 << 1 << -1 << -1 << 12 << 1;
    QTest::newRow( "data16" ) << 1 << 1 << 1 << -12 << -1 << 1 << 1;
    QTest::newRow( "data17" ) << -1 << 12 << 1 << 1 << 1 << 1 << 1;
    QTest::newRow( "data18" ) << -1 << 1 << 1 << 12 << 1 << 1 << 1;
    QTest::newRow( "data19" ) << -2 << 1 << 1 << 12 << -1 << 1 << 1;

    QTest::newRow( "invalid" ) << 0 << 0 << 0 << 1 << 0 << 0 << 0;
}

void tst_QDate::addYears()
{
    QFETCH( int, year );
    QFETCH( int, month );
    QFETCH( int, day );
    QFETCH( int, amountToAdd );
    QFETCH( int, expectedYear );
    QFETCH( int, expectedMonth );
    QFETCH( int, expectedDay );

    QDate dt( year, month, day );
    dt = dt.addYears( amountToAdd );

    QCOMPARE( dt.year(), expectedYear );
    QCOMPARE( dt.month(), expectedMonth );
    QCOMPARE( dt.day(), expectedDay );
}

void tst_QDate::addYears_data()
{
    QTest::addColumn<int>("year");
    QTest::addColumn<int>("month");
    QTest::addColumn<int>("day");
    QTest::addColumn<int>("amountToAdd");
    QTest::addColumn<int>("expectedYear");
    QTest::addColumn<int>("expectedMonth");
    QTest::addColumn<int>("expectedDay");

    QTest::newRow( "data0" ) << 2000 << 1 << 1 << 1 << 2001 << 1 << 1;
    QTest::newRow( "data1" ) << 2000 << 1 << 31 << 1 << 2001 << 1 << 31;
    QTest::newRow( "data2" ) << 2000 << 2 << 28 << 1 << 2001 << 2 << 28;
    QTest::newRow( "data3" ) << 2000 << 2 << 29 << 1 << 2001 << 2 << 28;
    QTest::newRow( "data4" ) << 2000 << 12 << 31 << 1 << 2001 << 12 << 31;
    QTest::newRow( "data5" ) << 2001 << 2 << 28 << 3 << 2004 << 2 << 28;
    QTest::newRow( "data6" ) << 2000 << 2 << 29 << 4 << 2004 << 2 << 29;

    QTest::newRow( "data7" ) << 2000 << 1 << 31 << -1 << 1999 << 1 << 31;
    QTest::newRow( "data9" ) << 2000 << 2 << 29 << -1 << 1999 << 2 << 28;
    QTest::newRow( "data10" ) << 2000 << 12 << 31 << -1 << 1999 << 12 << 31;
    QTest::newRow( "data11" ) << 2001 << 2 << 28 << -3 << 1998 << 2 << 28;
    QTest::newRow( "data12" ) << 2000 << 2 << 29 << -4 << 1996 << 2 << 29;
    QTest::newRow( "data13" ) << 2000 << 2 << 29 << -5 << 1995 << 2 << 28;

    QTest::newRow( "data14" ) << 2000 << 1 << 1 << -1999 << 1 << 1 << 1;
    QTest::newRow( "data15" ) << 2000 << 1 << 1 << -2000 << -1 << 1 << 1;
    QTest::newRow( "data16" ) << 2000 << 1 << 1 << -2001 << -2 << 1 << 1;
    QTest::newRow( "data17" ) << -2000 << 1 << 1 << 1999 << -1 << 1 << 1;
    QTest::newRow( "data18" ) << -2000 << 1 << 1 << 2000 << 1 << 1 << 1;
    QTest::newRow( "data19" ) << -2000 << 1 << 1 << 2001 << 2 << 1 << 1;

    QTest::newRow( "invalid" ) << 0 << 0 << 0 << 1 << 0 << 0 << 0;
}

void tst_QDate::daysTo()
{
    qint64 minJd = Q_INT64_C(-784350574879);
    qint64 maxJd = Q_INT64_C( 784354017364);

    QDate dt1(2000, 1, 1);
    QDate dt2(2000, 1, 5);
    QCOMPARE(dt1.daysTo(dt2), (qint64) 4);
    QCOMPARE(dt2.daysTo(dt1), (qint64) -4);

    dt1.setDate(0, 0, 0);
    QCOMPARE(dt1.daysTo(dt2), (qint64) 0);
    dt1.setDate(2000, 1, 1);
    dt2.setDate(0, 0, 0);
    QCOMPARE(dt1.daysTo(dt2), (qint64) 0);


    QDate maxDate = QDate::fromJulianDay(maxJd);
    QDate minDate = QDate::fromJulianDay(minJd);
    QDate zeroDate = QDate::fromJulianDay(0);

    QCOMPARE(maxDate.daysTo(minDate), minJd - maxJd);
    QCOMPARE(minDate.daysTo(maxDate), maxJd - minJd);
    QCOMPARE(maxDate.daysTo(zeroDate), -maxJd);
    QCOMPARE(zeroDate.daysTo(maxDate), maxJd);
    QCOMPARE(minDate.daysTo(zeroDate), -minJd);
    QCOMPARE(zeroDate.daysTo(minDate), minJd);
}

void tst_QDate::operator_eq_eq_data()
{
    QTest::addColumn<QDate>("d1");
    QTest::addColumn<QDate>("d2");
    QTest::addColumn<bool>("expectEqual");

    QTest::newRow("data0") << QDate(2000,1,2) << QDate(2000,1,2) << true;
    QTest::newRow("data1") << QDate(2001,12,5) << QDate(2001,12,5) << true;
    QTest::newRow("data2") << QDate(2001,12,5) << QDate(2001,12,5) << true;
    QTest::newRow("data3") << QDate(2001,12,5) << QDate(2002,12,5) << false;

    QDate date1(1900, 1, 1);
    QDate date2 = date1.addDays(1);
    QDate date3 = date1.addDays(-1);
    QDate date4 = date1.addMonths(1);
    QDate date5 = date1.addMonths(-1);
    QDate date6 = date1.addYears(1);
    QDate date7 = date1.addYears(-1);

    QTest::newRow("data4") << date2 << date3 << false;
    QTest::newRow("data5") << date4 << date5 << false;
    QTest::newRow("data6") << date6 << date7 << false;
    QTest::newRow("data7") << date1 << date2 << false;
    QTest::newRow("data8") << date1 << date3 << false;
    QTest::newRow("data9") << date1 << date4 << false;
    QTest::newRow("data10") << date1 << date5 << false;
    QTest::newRow("data11") << date1 << date6 << false;
    QTest::newRow("data12") << date1 << date7 << false;
}

void tst_QDate::operator_eq_eq()
{
    QFETCH(QDate, d1);
    QFETCH(QDate, d2);
    QFETCH(bool, expectEqual);

    bool equal = d1 == d2;
    QCOMPARE(equal, expectEqual);
    bool notEqual = d1 != d2;
    QCOMPARE(notEqual, !expectEqual);

    if (equal)
        QVERIFY(qHash(d1) == qHash(d2));
}

void tst_QDate::operator_lt()
{
    QDate d1(2000,1,2);
    QDate d2(2000,1,2);
    QVERIFY( !(d1 < d2) );

    d1 = QDate(2001,12,4);
    d2 = QDate(2001,12,5);
    QVERIFY( d1 < d2 );

    d1 = QDate(2001,11,5);
    d2 = QDate(2001,12,5);
    QVERIFY( d1 < d2 );

    d1 = QDate(2000,12,5);
    d2 = QDate(2001,12,5);
    QVERIFY( d1 < d2 );

    d1 = QDate(2002,12,5);
    d2 = QDate(2001,12,5);
    QVERIFY( !(d1 < d2) );

    d1 = QDate(2001,12,5);
    d2 = QDate(2001,11,5);
    QVERIFY( !(d1 < d2) );

    d1 = QDate(2001,12,6);
    d2 = QDate(2001,12,5);
    QVERIFY( !(d1 < d2) );
}

void tst_QDate::operator_gt()
{
    QDate d1(2000,1,2);
    QDate d2(2000,1,2);
    QVERIFY( !(d1 > d2) );

    d1 = QDate(2001,12,4);
    d2 = QDate(2001,12,5);
    QVERIFY( !(d1 > d2) );

    d1 = QDate(2001,11,5);
    d2 = QDate(2001,12,5);
    QVERIFY( !(d1 > d2) );

    d1 = QDate(2000,12,5);
    d2 = QDate(2001,12,5);
    QVERIFY( !(d1 > d2) );

    d1 = QDate(2002,12,5);
    d2 = QDate(2001,12,5);
    QVERIFY( d1 > d2 );

    d1 = QDate(2001,12,5);
    d2 = QDate(2001,11,5);
    QVERIFY( d1 > d2 );

    d1 = QDate(2001,12,6);
    d2 = QDate(2001,12,5);
    QVERIFY( d1 > d2 );
}

void tst_QDate::operator_lt_eq()
{
    QDate d1(2000,1,2);
    QDate d2(2000,1,2);
    QVERIFY( d1 <= d2 );

    d1 = QDate(2001,12,4);
    d2 = QDate(2001,12,5);
    QVERIFY( d1 <= d2 );

    d1 = QDate(2001,11,5);
    d2 = QDate(2001,12,5);
    QVERIFY( d1 <= d2 );

    d1 = QDate(2000,12,5);
    d2 = QDate(2001,12,5);
    QVERIFY( d1 <= d2 );

    d1 = QDate(2002,12,5);
    d2 = QDate(2001,12,5);
    QVERIFY( !(d1 <= d2) );

    d1 = QDate(2001,12,5);
    d2 = QDate(2001,11,5);
    QVERIFY( !(d1 <= d2) );

    d1 = QDate(2001,12,6);
    d2 = QDate(2001,12,5);
    QVERIFY( !(d1 <= d2) );
}

void tst_QDate::operator_gt_eq()
{
    QDate d1(2000,1,2);
    QDate d2(2000,1,2);
    QVERIFY( d1 >= d2 );

    d1 = QDate(2001,12,4);
    d2 = QDate(2001,12,5);
    QVERIFY( !(d1 >= d2) );

    d1 = QDate(2001,11,5);
    d2 = QDate(2001,12,5);
    QVERIFY( !(d1 >= d2) );

    d1 = QDate(2000,12,5);
    d2 = QDate(2001,12,5);
    QVERIFY( !(d1 >= d2) );

    d1 = QDate(2002,12,5);
    d2 = QDate(2001,12,5);
    QVERIFY( d1 >= d2 );

    d1 = QDate(2001,12,5);
    d2 = QDate(2001,11,5);
    QVERIFY( d1 >= d2 );

    d1 = QDate(2001,12,6);
    d2 = QDate(2001,12,5);
    QVERIFY( d1 >= d2 );
}

Q_DECLARE_METATYPE(QDataStream::Version)

void tst_QDate::operator_insert_extract_data()
{
    QTest::addColumn<QDate>("date");
    QTest::addColumn<QDataStream::Version>("dataStreamVersion");

    QMap<QDataStream::Version, QString> versionsToTest;
    versionsToTest.insert(QDataStream::Qt_1_0, QString::fromLatin1("Qt_1_0"));
    versionsToTest.insert(QDataStream::Qt_2_0, QString::fromLatin1("Qt_2_0"));
    versionsToTest.insert(QDataStream::Qt_2_1, QString::fromLatin1("Qt_2_1"));
    versionsToTest.insert(QDataStream::Qt_3_0, QString::fromLatin1("Qt_3_0"));
    versionsToTest.insert(QDataStream::Qt_3_1, QString::fromLatin1("Qt_3_1"));
    versionsToTest.insert(QDataStream::Qt_3_3, QString::fromLatin1("Qt_3_3"));
    versionsToTest.insert(QDataStream::Qt_4_0, QString::fromLatin1("Qt_4_0"));
    versionsToTest.insert(QDataStream::Qt_4_1, QString::fromLatin1("Qt_4_1"));
    versionsToTest.insert(QDataStream::Qt_4_2, QString::fromLatin1("Qt_4_2"));
    versionsToTest.insert(QDataStream::Qt_4_3, QString::fromLatin1("Qt_4_3"));
    versionsToTest.insert(QDataStream::Qt_4_4, QString::fromLatin1("Qt_4_4"));
    versionsToTest.insert(QDataStream::Qt_4_5, QString::fromLatin1("Qt_4_5"));
    versionsToTest.insert(QDataStream::Qt_4_6, QString::fromLatin1("Qt_4_6"));
    versionsToTest.insert(QDataStream::Qt_4_7, QString::fromLatin1("Qt_4_7"));
    versionsToTest.insert(QDataStream::Qt_4_8, QString::fromLatin1("Qt_4_8"));
    versionsToTest.insert(QDataStream::Qt_4_9, QString::fromLatin1("Qt_4_9"));
    versionsToTest.insert(QDataStream::Qt_5_0, QString::fromLatin1("Qt_5_0"));

    for (QMap<QDataStream::Version, QString>::ConstIterator it = versionsToTest.constBegin();
            it != versionsToTest.constEnd(); ++it) {
        const QString &version(it.value());
        QTest::newRow(("(invalid) " + version).toLocal8Bit().constData()) << invalidDate() << it.key();
        QTest::newRow(("(1, 1, 1) " + version).toLocal8Bit().constData()) << QDate(1, 1, 1) << it.key();
        QTest::newRow(("(-1, 1, 1) " + version).toLocal8Bit().constData()) << QDate(-1, 1, 1) << it.key();
        QTest::newRow(("(1995, 5, 20) " + version).toLocal8Bit().constData()) << QDate(1995, 5, 20) << it.key();

        // Test minimums for quint32/qint64.
        if (it.key() >= QDataStream::Qt_5_0)
            QTest::newRow(("(-4714, 11, 24) " + version).toLocal8Bit().constData()) << QDate(-4714, 11, 24) << it.key();
        else
            QTest::newRow(("(-4713, 1, 2) " + version).toLocal8Bit().constData()) << QDate(-4713, 1, 2) << it.key();
    }
}

void tst_QDate::operator_insert_extract()
{
    QFETCH(QDate, date);
    QFETCH(QDataStream::Version, dataStreamVersion);

    QByteArray byteArray;
    QDataStream dataStream(&byteArray, QIODevice::ReadWrite);
    dataStream.setVersion(dataStreamVersion);
    dataStream << date;
    dataStream.device()->reset();
    QDate deserialised;
    dataStream >> deserialised;
    QCOMPARE(dataStream.status(), QDataStream::Ok);

    QCOMPARE(deserialised, date);
}

void tst_QDate::fromStringDateFormat_data()
{
    QTest::addColumn<QString>("dateStr");
    QTest::addColumn<Qt::DateFormat>("dateFormat");
    QTest::addColumn<QDate>("expectedDate");

    QTest::newRow("text0") << QString("Sat May 20 1995") << Qt::TextDate << QDate(1995, 5, 20);
    QTest::newRow("text1") << QString("Tue Dec 17 2002") << Qt::TextDate << QDate(2002, 12, 17);
    QTest::newRow("text2") << QDate(1999, 11, 14).toString(Qt::TextDate) << Qt::TextDate << QDate(1999, 11, 14);
    QTest::newRow("text3") << QString("xxx Jan 1 0999") << Qt::TextDate << QDate(999, 1, 1);
    QTest::newRow("text3b") << QString("xxx Jan 1 999") << Qt::TextDate << QDate(999, 1, 1);
    QTest::newRow("text4") << QString("xxx Jan 1 12345") << Qt::TextDate << QDate(12345, 1, 1);
    QTest::newRow("text5") << QString("xxx Jan 1 -0001") << Qt::TextDate << QDate(-1, 1, 1);
    QTest::newRow("text6") << QString("xxx Jan 1 -4712") << Qt::TextDate << QDate(-4712, 1, 1);
    QTest::newRow("text7") << QString("xxx Nov 25 -4713") << Qt::TextDate << QDate(-4713, 11, 25);
    QTest::newRow("text, empty") << QString() << Qt::TextDate << QDate();
    QTest::newRow("text, 3 part") << QString("part1 part2 part3") << Qt::TextDate << QDate();
    QTest::newRow("text, invalid month name") << QString("Wed BabytownFrolics 8 2012") << Qt::TextDate << QDate();
    QTest::newRow("text, invalid day") << QString("Wed May Wilhelm 2012") << Qt::TextDate << QDate();
    QTest::newRow("text, invalid year") << QString("Wed May 8 Cats") << Qt::TextDate << QDate();

    QTest::newRow("iso0") << QString("1995-05-20") << Qt::ISODate << QDate(1995, 5, 20);
    QTest::newRow("iso1") << QString("2002-12-17") << Qt::ISODate << QDate(2002, 12, 17);
    QTest::newRow("iso2") << QDate(1999, 11, 14).toString(Qt::ISODate) << Qt::ISODate << QDate(1999, 11, 14);
    QTest::newRow("iso3") << QString("0999-01-01") << Qt::ISODate << QDate(999, 1, 1);
    QTest::newRow("iso3b") << QString("0999-01-01") << Qt::ISODate << QDate(999, 1, 1);
    QTest::newRow("iso4") << QString("2000101101")      << Qt::ISODate << QDate();
    QTest::newRow("iso5") << QString("2000/01/01")      << Qt::ISODate << QDate(2000, 1, 1);
    QTest::newRow("iso6") << QString("2000-01-01 blah") << Qt::ISODate << QDate(2000, 1, 1);
    QTest::newRow("iso7") << QString("2000-01-011blah") << Qt::ISODate << QDate();
    QTest::newRow("iso8") << QString("2000-01-01blah")  << Qt::ISODate << QDate(2000, 1, 1);
    QTest::newRow("iso9") << QString("-001-01-01")      << Qt::ISODate << QDate();
    QTest::newRow("iso10") << QString("99999-01-01")    << Qt::ISODate << QDate();

    // Test Qt::RFC2822Date format (RFC 2822).
    QTest::newRow("RFC 2822") << QString::fromLatin1("13 Feb 1987 13:24:51 +0100")
        << Qt::RFC2822Date << QDate(1987, 2, 13);
    QTest::newRow("RFC 2822 with day") << QString::fromLatin1("Thu, 01 Jan 1970 00:12:34 +0000")
        << Qt::RFC2822Date << QDate(1970, 1, 1);
    // No timezone
    QTest::newRow("RFC 2822 no timezone") << QString::fromLatin1("01 Jan 1970 00:12:34")
        << Qt::RFC2822Date << QDate(1970, 1, 1);
    // No time specified
    QTest::newRow("RFC 2822 date only") << QString::fromLatin1("01 Nov 2002")
        << Qt::RFC2822Date << QDate(2002, 11, 1);
    QTest::newRow("RFC 2822 with day date only") << QString::fromLatin1("Fri, 01 Nov 2002")
        << Qt::RFC2822Date << QDate(2002, 11, 1);
    // Test invalid month, day, year
    QTest::newRow("RFC 2822 invalid month name") << QString::fromLatin1("13 Fev 1987 13:24:51 +0100")
        << Qt::RFC2822Date << QDate();
    QTest::newRow("RFC 2822 invalid day") << QString::fromLatin1("36 Fev 1987 13:24:51 +0100")
        << Qt::RFC2822Date << QDate();
    QTest::newRow("RFC 2822 invalid year") << QString::fromLatin1("13 Fev 0000 13:24:51 +0100")
        << Qt::RFC2822Date << QDate();
    // Test invalid characters (should ignore invalid characters at end of string).
    QTest::newRow("RFC 2822 invalid character at end") << QString::fromLatin1("01 Jan 2012 08:00:00 +0100!")
        << Qt::RFC2822Date << QDate(2012, 1, 1);
    QTest::newRow("RFC 2822 invalid character at front") << QString::fromLatin1("!01 Jan 2012 08:00:00 +0000")
        << Qt::RFC2822Date << QDate();
    QTest::newRow("RFC 2822 invalid character both ends") << QString::fromLatin1("!01 Jan 2012 08:00:00 +0000!")
        << Qt::RFC2822Date << QDate();
    QTest::newRow("RFC 2822 invalid character at front, 2 at back") << QString::fromLatin1("!01 Jan 2012 08:00:00 +0000..")
        << Qt::RFC2822Date << QDate();
    QTest::newRow("RFC 2822 invalid character 2 at front") << QString::fromLatin1("!!01 Jan 2012 08:00:00 +0000")
        << Qt::RFC2822Date << QDate();

    // Test Qt::RFC2822Date format (RFC 850 and 1036).
    QTest::newRow("RFC 850 and 1036") << QString::fromLatin1("Fri Feb 13 13:24:51 1987 +0100")
        << Qt::RFC2822Date << QDate(1987, 2, 13);
    // No timezone
    QTest::newRow("RFC 850 and 1036 no timezone") << QString::fromLatin1("Thu Jan 01 00:12:34 1970")
        << Qt::RFC2822Date << QDate(1970, 1, 1);
    // No time specified
    QTest::newRow("RFC 850 and 1036 date only") << QString::fromLatin1("Fri Nov 01 2002")
        << Qt::RFC2822Date << QDate(2002, 11, 1);
    // Test invalid characters (should ignore invalid characters at end of string).
    QTest::newRow("RFC 850 and 1036 invalid character at end") << QString::fromLatin1("Sun Jan 01 08:00:00 2012 +0100!")
        << Qt::RFC2822Date << QDate(2012, 1, 1);
    QTest::newRow("RFC 850 and 1036 invalid character at front") << QString::fromLatin1("!Sun Jan 01 08:00:00 2012 +0000")
        << Qt::RFC2822Date << QDate();
    QTest::newRow("RFC 850 and 1036 invalid character both ends") << QString::fromLatin1("!Sun Jan 01 08:00:00 2012 +0000!")
        << Qt::RFC2822Date << QDate();
    QTest::newRow("RFC 850 and 1036 invalid character at front, 2 at back") << QString::fromLatin1("!Sun Jan 01 08:00:00 2012 +0000..")
        << Qt::RFC2822Date << QDate();
    QTest::newRow("RFC 850 and 1036 invalid character 2 at front") << QString::fromLatin1("!!Sun Jan 01 08:00:00 2012 +0000")
        << Qt::RFC2822Date << QDate();

    QTest::newRow("RFC empty") << QString::fromLatin1("") << Qt::RFC2822Date << QDate();
}

void tst_QDate::fromStringDateFormat()
{
    QFETCH(QString, dateStr);
    QFETCH(Qt::DateFormat, dateFormat);
    QFETCH(QDate, expectedDate);

    QCOMPARE(QDate::fromString(dateStr, dateFormat), expectedDate);
}

void tst_QDate::fromStringFormat_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QDate>("expected");

    //get localized names
    QString january = QDate::longMonthName(1);
    QString february = QDate::longMonthName(2);
    QString march = QDate::longMonthName(3);
    QString august = QDate::longMonthName(8);
    QString mon = QDate::shortDayName(1);
    QString monday = QDate::longDayName(1);
    QString tuesday = QDate::longDayName(2);
    QString wednesday = QDate::longDayName(3);
    QString thursday = QDate::longDayName(4);
    QString friday = QDate::longDayName(5);
    QString saturday = QDate::longDayName(6);
    QString sunday = QDate::longDayName(7);

    QTest::newRow("data0") << QString("") << QString("") << defDate();
    QTest::newRow("data1") << QString(" ") << QString("") << invalidDate();
    QTest::newRow("data2") << QString(" ") << QString(" ") << defDate();
    QTest::newRow("data3") << QString("-%$%#") << QString("$*(#@") << invalidDate();
    QTest::newRow("data4") << QString("d") << QString("'d'") << defDate();
    QTest::newRow("data5") << QString("101010") << QString("dMyy") << QDate(1910, 10, 10);
    QTest::newRow("data6") << QString("101010b") << QString("dMyy") << invalidDate();
    QTest::newRow("data7") << january << QString("MMMM") << defDate();
    QTest::newRow("data8") << QString("ball") << QString("balle") << invalidDate();
    QTest::newRow("data9") << QString("balleh") << QString("balleh") << defDate();
    QTest::newRow("data10") << QString("10.01.1") << QString("M.dd.d") << QDate(defDate().year(), 10, 1);
    QTest::newRow("data11") << QString("-1.01.1") << QString("M.dd.d") << invalidDate();
    QTest::newRow("data12") << QString("11010") << QString("dMMyy") << invalidDate();
    QTest::newRow("data13") << QString("-2") << QString("d") << invalidDate();
    QTest::newRow("data14") << QString("132") << QString("Md") << invalidDate();
    QTest::newRow("data15") << february << QString("MMMM") << QDate(defDate().year(), 2, 1);

    QString date = mon + QLatin1Char(' ') + august + " 8 2005";
    QTest::newRow("data16") << date << QString("ddd MMMM d yyyy") << QDate(2005, 8, 8);
    QTest::newRow("data17") << QString("2000:00") << QString("yyyy:yy") << QDate(2000, 1, 1);
    QTest::newRow("data18") << QString("1999:99") << QString("yyyy:yy") << QDate(1999, 1, 1);
    QTest::newRow("data19") << QString("2099:99") << QString("yyyy:yy") << QDate(2099, 1, 1);
    QTest::newRow("data20") << QString("2001:01") << QString("yyyy:yy") << QDate(2001, 1, 1);
    QTest::newRow("data21") << QString("99") << QString("yy") << QDate(1999, 1, 1);
    QTest::newRow("data22") << QString("01") << QString("yy") << QDate(1901, 1, 1);

    QTest::newRow("data23") << monday << QString("dddd") << QDate(1900, 1, 1);
    QTest::newRow("data24") << tuesday << QString("dddd") << QDate(1900, 1, 2);
    QTest::newRow("data25") << wednesday << QString("dddd") << QDate(1900, 1, 3);
    QTest::newRow("data26") << thursday << QString("dddd") << QDate(1900, 1, 4);
    QTest::newRow("data27") << friday << QString("dddd") << QDate(1900, 1, 5);
    QTest::newRow("data28") << saturday << QString("dddd") << QDate(1900, 1, 6);
    QTest::newRow("data29") << sunday << QString("dddd") << QDate(1900, 1, 7);

    QTest::newRow("data30") << monday + " 2006" << QString("dddd yyyy") << QDate(2006, 1, 2);
    QTest::newRow("data31") << tuesday + " 2006" << QString("dddd yyyy") << QDate(2006, 1, 3);
    QTest::newRow("data32") << wednesday + " 2006" << QString("dddd yyyy") << QDate(2006, 1, 4);
    QTest::newRow("data33") << thursday + " 2006" << QString("dddd yyyy") << QDate(2006, 1, 5);
    QTest::newRow("data34") << friday + " 2006" << QString("dddd yyyy") << QDate(2006, 1, 6);
    QTest::newRow("data35") << saturday + " 2006" << QString("dddd yyyy") << QDate(2006, 1, 7);
    QTest::newRow("data36") << sunday + " 2006" << QString("dddd yyyy") << QDate(2006, 1, 1);

    QTest::newRow("data37") << tuesday + " 2007 " + march << QString("dddd yyyy MMMM") << QDate(2007, 3, 6);

    QTest::newRow("data38") << QString("21052006") << QString("ddMMyyyy") << QDate(2006,5,21);
    QTest::newRow("data39") << QString("210506") << QString("ddMMyy") << QDate(1906,5,21);
    QTest::newRow("data40") << QString("21/5/2006") << QString("d/M/yyyy") << QDate(2006,5,21);
    QTest::newRow("data41") << QString("21/5/06") << QString("d/M/yy") << QDate(1906,5,21);
    QTest::newRow("data42") << QString("20060521") << QString("yyyyMMdd") << QDate(2006,5,21);
    QTest::newRow("data43") << QString("060521") << QString("yyMMdd") << QDate(1906,5,21);
}


void tst_QDate::fromStringFormat()
{
    QFETCH(QString, string);
    QFETCH(QString, format);
    QFETCH(QDate, expected);

    QDate dt = QDate::fromString(string, format);
    QCOMPARE(dt, expected);
}

void tst_QDate::toStringFormat_data()
{
    QTest::addColumn<QDate>("t");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("str");

    QTest::newRow( "data0" ) << QDate(1995,5,20) << QString("d-M-yy") << QString("20-5-95");
    QTest::newRow( "data1" ) << QDate(2002,12,17) << QString("dd-MM-yyyy") << QString("17-12-2002");
    QTest::newRow( "data2" ) << QDate(1995,5,20) << QString("M-yy") << QString("5-95");
    QTest::newRow( "data3" ) << QDate(2002,12,17) << QString("dd") << QString("17");
    QTest::newRow( "data4" ) << QDate() << QString("dd-mm-yyyy") << QString();
}

void tst_QDate::toStringFormat()
{
    QFETCH( QDate, t );
    QFETCH( QString, format );
    QFETCH( QString, str );

    QCOMPARE( t.toString( format ), str );
}

void tst_QDate::toStringDateFormat_data()
{
    QTest::addColumn<QDate>("date");
    QTest::addColumn<Qt::DateFormat>("format");
    QTest::addColumn<QString>("expectedStr");

    QTest::newRow("data0") << QDate(1,1,1) << Qt::ISODate << QString("0001-01-01");
    QTest::newRow("data1") << QDate(11,1,1) << Qt::ISODate << QString("0011-01-01");
    QTest::newRow("data2") << QDate(111,1,1) << Qt::ISODate << QString("0111-01-01");
    QTest::newRow("data3") << QDate(1974,12,1) << Qt::ISODate << QString("1974-12-01");
    QTest::newRow("year < 0") << QDate(-1,1,1) << Qt::ISODate << QString();
    QTest::newRow("year > 9999") << QDate(-1,1,1) << Qt::ISODate << QString();
    QTest::newRow("RFC2822Date") << QDate(1974,12,1) << Qt::RFC2822Date << QString("01 Dec 1974");
    QTest::newRow("ISODateWithMs") << QDate(1974,12,1) << Qt::ISODateWithMs << QString("1974-12-01");
}

void tst_QDate::toStringDateFormat()
{
    QFETCH(QDate, date);
    QFETCH(Qt::DateFormat, format);
    QFETCH(QString, expectedStr);

    QCOMPARE(date.toString(Qt::SystemLocaleShortDate), QLocale::system().toString(date, QLocale::ShortFormat));
    QCOMPARE(date.toString(Qt::DefaultLocaleShortDate), QLocale().toString(date, QLocale::ShortFormat));
    QCOMPARE(date.toString(Qt::SystemLocaleLongDate), QLocale::system().toString(date, QLocale::LongFormat));
    QCOMPARE(date.toString(Qt::DefaultLocaleLongDate), QLocale().toString(date, QLocale::LongFormat));
    QLocale::setDefault(QLocale::German);
    QCOMPARE(date.toString(Qt::SystemLocaleShortDate), QLocale::system().toString(date, QLocale::ShortFormat));
    QCOMPARE(date.toString(Qt::DefaultLocaleShortDate), QLocale().toString(date, QLocale::ShortFormat));
    QCOMPARE(date.toString(Qt::SystemLocaleLongDate), QLocale::system().toString(date, QLocale::LongFormat));
    QCOMPARE(date.toString(Qt::DefaultLocaleLongDate), QLocale().toString(date, QLocale::LongFormat));

    QCOMPARE(date.toString(format), expectedStr);
}

void tst_QDate::isLeapYear()
{
    QVERIFY(QDate::isLeapYear(-4801));
    QVERIFY(!QDate::isLeapYear(-4800));
    QVERIFY(QDate::isLeapYear(-4445));
    QVERIFY(!QDate::isLeapYear(-4444));
    QVERIFY(!QDate::isLeapYear(-6));
    QVERIFY(QDate::isLeapYear(-5));
    QVERIFY(!QDate::isLeapYear(-4));
    QVERIFY(!QDate::isLeapYear(-3));
    QVERIFY(!QDate::isLeapYear(-2));
    QVERIFY(QDate::isLeapYear(-1));
    QVERIFY(!QDate::isLeapYear(0)); // Doesn't exist
    QVERIFY(!QDate::isLeapYear(1));
    QVERIFY(!QDate::isLeapYear(2));
    QVERIFY(!QDate::isLeapYear(3));
    QVERIFY(QDate::isLeapYear(4));
    QVERIFY(!QDate::isLeapYear(7));
    QVERIFY(QDate::isLeapYear(8));
    QVERIFY(!QDate::isLeapYear(100));
    QVERIFY(QDate::isLeapYear(400));
    QVERIFY(!QDate::isLeapYear(700));
    QVERIFY(!QDate::isLeapYear(1500));
    QVERIFY(QDate::isLeapYear(1600));
    QVERIFY(!QDate::isLeapYear(1700));
    QVERIFY(!QDate::isLeapYear(1800));
    QVERIFY(!QDate::isLeapYear(1900));
    QVERIFY(QDate::isLeapYear(2000));
    QVERIFY(!QDate::isLeapYear(2100));
    QVERIFY(!QDate::isLeapYear(2200));
    QVERIFY(!QDate::isLeapYear(2300));
    QVERIFY(QDate::isLeapYear(2400));
    QVERIFY(!QDate::isLeapYear(2500));
    QVERIFY(!QDate::isLeapYear(2600));
    QVERIFY(!QDate::isLeapYear(2700));
    QVERIFY(QDate::isLeapYear(2800));

    for (int i = -4713; i <= 10000; ++i) {
        if (i == 0)
            continue;
        QVERIFY(!QDate(i, 2, 29).isValid() == !QDate::isLeapYear(i));
    }
}

void tst_QDate::yearsZeroToNinetyNine()
{
    {
        QDate dt(-1, 2, 3);
        QCOMPARE(dt.year(), -1);
        QCOMPARE(dt.month(), 2);
        QCOMPARE(dt.day(), 3);
    }

    {
        QDate dt(1, 2, 3);
        QCOMPARE(dt.year(), 1);
        QCOMPARE(dt.month(), 2);
        QCOMPARE(dt.day(), 3);
    }

    {
        QDate dt(99, 2, 3);
        QCOMPARE(dt.year(), 99);
        QCOMPARE(dt.month(), 2);
        QCOMPARE(dt.day(), 3);
    }

    QVERIFY(!QDate::isValid(0, 2, 3));
    QVERIFY(QDate::isValid(1, 2, 3));
    QVERIFY(QDate::isValid(-1, 2, 3));

#if QT_DEPRECATED_SINCE(5,0)
    {
        QDate dt;
        dt.setYMD(1, 2, 3);
        QCOMPARE(dt.year(), 1901);
        QCOMPARE(dt.month(), 2);
        QCOMPARE(dt.day(), 3);
    }
#endif

    {
        QDate dt;
        dt.setDate(1, 2, 3);
        QCOMPARE(dt.year(), 1);
        QCOMPARE(dt.month(), 2);
        QCOMPARE(dt.day(), 3);

        dt.setDate(0, 2, 3);
        QVERIFY(!dt.isValid());
    }
}


void tst_QDate::negativeYear() const
{
    QDate y(-20, 3, 4);
    QVERIFY(y.isValid());
    QCOMPARE(y.year(), -20);
}

void tst_QDate::printNegativeYear() const
{
    {
        QDate date(-500, 3, 4);
        QVERIFY(date.isValid());
        QCOMPARE(date.toString(QLatin1String("yyyy")), QString::fromLatin1("-0500"));
    }

    {
        QDate date(-10, 3, 4);
        QVERIFY(date.isValid());
        QCOMPARE(date.toString(QLatin1String("yyyy")), QString::fromLatin1("-0010"));
    }

    {
        QDate date(-2, 3, 4);
        QVERIFY(date.isValid());
        QCOMPARE(date.toString(QLatin1String("yyyy")), QString::fromLatin1("-0002"));
    }
}

void tst_QDate::roundtripGermanLocale() const
{
    /* This code path should not result in warnings. */
    const QDate theDate(QDate::currentDate());
    theDate.fromString(theDate.toString(Qt::TextDate), Qt::TextDate);

    const QDateTime theDateTime(QDateTime::currentDateTime());
    theDateTime.fromString(theDateTime.toString(Qt::TextDate), Qt::TextDate);
}

void tst_QDate::shortDayName() const
{
    QCOMPARE(QDate::shortDayName(0), QString());
    QCOMPARE(QDate::shortDayName(8), QString());

    if (QLocale::system().language() == QLocale::C) {
        QCOMPARE(QDate::shortDayName(1), QLatin1String("Mon"));
        QCOMPARE(QDate::shortDayName(7), QLatin1String("Sun"));
    }

    QLocale locale = QLocale::system();
    for(int i = 1; i <= 7; ++i) {
        QCOMPARE(QDate::shortDayName(i), locale.dayName(i, QLocale::ShortFormat));
    }
}

void tst_QDate::standaloneShortDayName() const
{
    QCOMPARE(QDate::shortDayName(0, QDate::StandaloneFormat), QString());
    QCOMPARE(QDate::shortDayName(8, QDate::StandaloneFormat), QString());

    if (QLocale::system().language() == QLocale::C) {
        QCOMPARE(QDate::shortDayName(1, QDate::StandaloneFormat), QLatin1String("Mon"));
        QCOMPARE(QDate::shortDayName(7, QDate::StandaloneFormat), QLatin1String("Sun"));
    }

    QLocale locale = QLocale::system();
    for(int i = 1; i <= 7; ++i) {
        QCOMPARE(QDate::shortDayName(i, QDate::StandaloneFormat), locale.standaloneDayName(i, QLocale::ShortFormat));
    }
}

void tst_QDate::longDayName() const
{
    QCOMPARE(QDate::longDayName(0), QString());
    QCOMPARE(QDate::longDayName(8), QString());

    if (QLocale::system().language() == QLocale::C) {
        QCOMPARE(QDate::longDayName(1), QLatin1String("Monday"));
        QCOMPARE(QDate::longDayName(7), QLatin1String("Sunday"));
    }

    QLocale locale = QLocale::system();
    for(int i = 1; i <= 7; ++i) {
        QCOMPARE(QDate::longDayName(i), locale.dayName(i, QLocale::LongFormat));
    }
}

void tst_QDate::standaloneLongDayName() const
{
    QCOMPARE(QDate::longDayName(0, QDate::StandaloneFormat), QString());
    QCOMPARE(QDate::longDayName(8, QDate::StandaloneFormat), QString());

    if (QLocale::system().language() == QLocale::C) {
        QCOMPARE(QDate::longDayName(1, QDate::StandaloneFormat), QLatin1String("Monday"));
        QCOMPARE(QDate::longDayName(7, QDate::StandaloneFormat), QLatin1String("Sunday"));
    }

    QLocale locale = QLocale::system();
    for(int i = 1; i <= 7; ++i) {
        QCOMPARE(QDate::longDayName(i, QDate::StandaloneFormat), locale.standaloneDayName(i, QLocale::LongFormat));
    }
}

void tst_QDate::shortMonthName() const
{
    QCOMPARE(QDate::shortMonthName(0), QString());
    QCOMPARE(QDate::shortMonthName(13), QString());

    if (QLocale::system().language() == QLocale::C) {
        QCOMPARE(QDate::shortMonthName(1), QLatin1String("Jan"));
        QCOMPARE(QDate::shortMonthName(8), QLatin1String("Aug"));
    }

    QLocale locale = QLocale::system();
    for(int i = 1; i <= 12; ++i) {
        QCOMPARE(QDate::shortMonthName(i), locale.monthName(i, QLocale::ShortFormat));
    }
}

void tst_QDate::standaloneShortMonthName() const
{
    QCOMPARE(QDate::shortMonthName(0, QDate::StandaloneFormat), QString());
    QCOMPARE(QDate::shortMonthName(13, QDate::StandaloneFormat), QString());

    if (QLocale::system().language() == QLocale::C) {
        QCOMPARE(QDate::shortMonthName(1, QDate::StandaloneFormat), QLatin1String("Jan"));
        QCOMPARE(QDate::shortMonthName(8, QDate::StandaloneFormat), QLatin1String("Aug"));
    }

    QLocale locale = QLocale::system();
    for(int i = 1; i <= 12; ++i) {
        QCOMPARE(QDate::shortMonthName(i, QDate::StandaloneFormat), locale.standaloneMonthName(i, QLocale::ShortFormat));
    }
}

void tst_QDate::longMonthName() const
{
    QCOMPARE(QDate::longMonthName(0), QString());
    QCOMPARE(QDate::longMonthName(13), QString());

    if (QLocale::system().language() == QLocale::C) {
        QCOMPARE(QDate::longMonthName(1), QLatin1String("January"));
        QCOMPARE(QDate::longMonthName(8), QLatin1String("August"));
    }

    QLocale locale = QLocale::system();
    for(int i = 1; i <= 12; ++i) {
        QCOMPARE(QDate::longMonthName(i), locale.monthName(i, QLocale::LongFormat));
    }
}

void tst_QDate::standaloneLongMonthName() const
{
    QCOMPARE(QDate::longMonthName(0, QDate::StandaloneFormat), QString());
    QCOMPARE(QDate::longMonthName(13, QDate::StandaloneFormat), QString());

    if (QLocale::system().language() == QLocale::C) {
        QCOMPARE(QDate::longMonthName(1, QDate::StandaloneFormat), QLatin1String("January"));
        QCOMPARE(QDate::longMonthName(8, QDate::StandaloneFormat), QLatin1String("August"));
    }

    QLocale locale = QLocale::system();
    for(int i = 1; i <= 12; ++i) {
        QCOMPARE(QDate::longMonthName(i, QDate::StandaloneFormat), locale.standaloneMonthName(i, QLocale::LongFormat));
    }
}

void tst_QDate::roundtrip() const
{
    // Test round trip, this exercises setDate(), isValid(), isLeapYear(),
    // year(), month(), day(), julianDayFromDate(), and getDateFromJulianDay()
    // to ensure they are internally consistent (but doesn't guarantee correct)

    // Test Julian round trip around JD 0 and the c++ integer division rounding
    // problem point (eg. negative numbers) in the conversion functions.
    QDate testDate;
    QDate loopDate = QDate::fromJulianDay(-50001); // 1 Jan 4850 BC
    while (loopDate.toJulianDay() <= 5150) {     // 31 Dec 4700 BC
        testDate.setDate(loopDate.year(), loopDate.month(), loopDate.day());
        QCOMPARE(loopDate.toJulianDay(), testDate.toJulianDay());
        loopDate = loopDate.addDays(1);
    }

    // Test Julian round trip in both BC and AD
    loopDate = QDate::fromJulianDay(1684901);       //  1 Jan 100 BC
    while (loopDate.toJulianDay() <= 1757949) {   // 31 Dec 100 AD
        testDate.setDate(loopDate.year(), loopDate.month(), loopDate.day());
        QCOMPARE(loopDate.toJulianDay(), testDate.toJulianDay());
        loopDate = loopDate.addDays(1);
    }

    // Test Gregorian round trip during current useful period
    loopDate = QDate::fromJulianDay(2378497);     //  1 Jan 1900 AD
    while (loopDate.toJulianDay() <= 2488433) { // 31 Dec 2100 AD
        testDate.setDate(loopDate.year(), loopDate.month(), loopDate.day());
        QCOMPARE(loopDate.toJulianDay(), testDate.toJulianDay());
        loopDate = loopDate.addDays(1);
    }

    // Test Gregorian round trip at top end of widget/format range
    loopDate = QDate::fromJulianDay(5336961);     //  1 Jan 9900 AD
    while (loopDate.toJulianDay() <= 5373484) { // 31 Dec 9999 AD
        testDate.setDate(loopDate.year(), loopDate.month(), loopDate.day());
        QCOMPARE(loopDate.toJulianDay(), testDate.toJulianDay());
        loopDate = loopDate.addDays(1);
    }

    qint64 minJd = Q_INT64_C(-784350574879);
    qint64 maxJd = Q_INT64_C( 784354017364);

    // Test Gregorian round trip at top end of conversion range
    loopDate = QDate::fromJulianDay(maxJd);
    while (loopDate.toJulianDay() >= maxJd - 146397) {
        testDate.setDate(loopDate.year(), loopDate.month(), loopDate.day());
        QCOMPARE(loopDate.toJulianDay(), testDate.toJulianDay());
        loopDate = loopDate.addDays(-1);
    }

    // Test Gregorian round trip at low end of conversion range
    loopDate = QDate::fromJulianDay(minJd);
    while (loopDate.toJulianDay() <= minJd + 146397) {
        testDate.setDate(loopDate.year(), loopDate.month(), loopDate.day());
        QCOMPARE(loopDate.toJulianDay(), testDate.toJulianDay());
        loopDate = loopDate.addDays(1);
    }
}

void tst_QDate::qdebug() const
{
    QTest::ignoreMessage(QtDebugMsg, "QDate(\"\")");
    qDebug() << QDate();
    QTest::ignoreMessage(QtDebugMsg, "QDate(\"1983-08-07\")");
    qDebug() << QDate(1983, 8, 7);
}

QTEST_APPLESS_MAIN(tst_QDate)
#include "tst_qdate.moc"
