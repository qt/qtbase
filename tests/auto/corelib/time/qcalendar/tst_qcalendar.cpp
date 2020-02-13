/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QCalendar>
Q_DECLARE_METATYPE(QCalendar::System)

class tst_QCalendar : public QObject
{
    Q_OBJECT
private:
    void checkYear(const QCalendar &cal, int year, bool normal=false);

private slots:
    void basic_data();
    void basic();
    void unspecified_data() { basic_data(); }
    void unspecified();
    void nameCase();
    void specific_data();
    void specific();
    void daily_data() { basic_data(); }
    void daily();
};

// Support for basic():
void tst_QCalendar::checkYear(const QCalendar &cal, int year, bool normal)
{
    const int moons = cal.monthsInYear(year);
    // Months are numbered from 1 to moons:
    QVERIFY(moons > 0);
    QVERIFY(!cal.isDateValid(year, moons + 1, 1));
    QVERIFY(!cal.isDateValid(year, 0, 1));
    QVERIFY(moons <= cal.maximumMonthsInYear());

    const int days = cal.daysInYear(year);
    QVERIFY(days > 0);

    int sum = 0;
    const int longest = cal.maximumDaysInMonth();
    for (int i = moons; i > 0; i--) {
        const int last = cal.daysInMonth(i, year);
        sum += last;
        // Valid month has some days and no more than max:
        QVERIFY(last > 0);
        QVERIFY(last <= longest);
        // Days are numbered from 1 to last:
        QVERIFY(cal.isDateValid(year, i, 1));
        QVERIFY(cal.isDateValid(year, i, last));
        QVERIFY(!cal.isDateValid(year, i, 0));
        QVERIFY(!cal.isDateValid(year, i, last + 1));
        if (normal) // Unspecified year gets same daysInMonth():
            QCOMPARE(cal.daysInMonth(i), last);
    }
    // Months add up to the whole year:
    QCOMPARE(sum, days);
}

#define CHECKYEAR(cal, year) checkYear(cal, year);   \
    if (QTest::currentTestFailed()) \
        return

#define NORMALYEAR(cal, year) checkYear(cal, year, true); \
    if (QTest::currentTestFailed()) \
        return

void tst_QCalendar::basic_data()
{
    QTest::addColumn<QCalendar::System>("system");

    QMetaEnum e = QCalendar::staticMetaObject.enumerator(0);
    Q_ASSERT(qstrcmp(e.name(), "System") == 0);

    for (int i = 0; i <= int(QCalendar::System::Last); ++i) {
        // There may be gaps in the enum's numbering; and Last is a duplicate:
        if (e.value(i) != -1 && qstrcmp(e.key(i), "Last"))
            QTest::newRow(e.key(i)) << QCalendar::System(e.value(i));
    }
}

void tst_QCalendar::basic()
{
    QFETCH(QCalendar::System, system);
    QCalendar cal(system);
    QVERIFY(cal.isValid());
    QCOMPARE(QCalendar(cal.name()).isGregorian(), cal.isGregorian());
    QCOMPARE(QCalendar(cal.name()).name(), cal.name());

    if (cal.hasYearZero()) {
        CHECKYEAR(cal, 0);
    } else {
        QCOMPARE(cal.monthsInYear(0), 0);
        QCOMPARE(cal.daysInYear(0), 0);
        QVERIFY(!cal.isDateValid(0, 1, 1));
    }

    if (cal.isProleptic()) {
        CHECKYEAR(cal, -1);
    } else {
        QCOMPARE(cal.monthsInYear(-1), 0);
        QCOMPARE(cal.daysInYear(-1), 0);
        QVERIFY(!cal.isDateValid(-1, 1, 1));
    }

    // Look for a leap year in the last decade.
    int year = QDate::currentDate().year(cal);
    for (int i = 10; i > 0 && !cal.isLeapYear(year); --i)
        --year;
    if (cal.isLeapYear(year)) {
        // ... and a non-leap year within a decade before it.
        int leap = year--;
        for (int i = 10; i > 0 && cal.isLeapYear(year); --i)
            year--;
        if (!cal.isLeapYear(year))
            QVERIFY(cal.daysInYear(year) < cal.daysInYear(leap));

        CHECKYEAR(cal, leap);
    }
    // Either year is non-leap or we have a decade of leap years together;
    // expect daysInMonth() to treat year the same as unspecified.
    NORMALYEAR(cal, year);
}

void tst_QCalendar::unspecified()
{
    QFETCH(QCalendar::System, system);
    QCalendar cal(system);

    const QDate today = QDate::currentDate();
    const int thisYear = today.year();
    QCOMPARE(cal.monthsInYear(QCalendar::Unspecified), cal.maximumMonthsInYear());
    for (int month = cal.maximumMonthsInYear(); month > 0; month--) {
        const int days = cal.daysInMonth(month);
        int count = 0;
        // 19 years = one Metonic cycle (used by some lunar calendars)
        for (int i = 19; i > 0; --i) {
            if (cal.daysInMonth(month, thisYear - i) == days)
                count++;
        }
        // Require a majority of the years tested:
        QVERIFY2(count > 9, "Default daysInMonth() should be for a normal year");
    }
}

void tst_QCalendar::nameCase()
{
    QVERIFY(QCalendar::availableCalendars().contains(QStringLiteral("Gregorian")));
}

void tst_QCalendar::specific_data()
{
    QTest::addColumn<QCalendar::System>("system");
    // Date in that system:
    QTest::addColumn<int>("sysyear");
    QTest::addColumn<int>("sysmonth");
    QTest::addColumn<int>("sysday");
    // Gregorian equivalent:
    QTest::addColumn<int>("gregyear");
    QTest::addColumn<int>("gregmonth");
    QTest::addColumn<int>("gregday");

#define ADDROW(cal, year, month, day, gy, gm, gd) \
    QTest::newRow(#cal) << QCalendar::System::cal << year << month << day << gy << gm << gd

    ADDROW(Gregorian, 1970, 1, 1, 1970, 1, 1);

    // One known specific date, for each calendar
#ifndef QT_BOOTSTRAPPED
    // Julian 1582-10-4 was followed by Gregorian 1582-10-15
    ADDROW(Julian, 1582, 10, 4, 1582, 10, 14);
    // Milankovic matches Gregorian for a few centuries
    ADDROW(Milankovic, 1923, 3, 20, 1923, 3, 20);
#endif

#if QT_CONFIG(jalalicalendar)
    // Jalali year 1355 started on Gregorian 1976-3-21:
    ADDROW(Jalali, 1355, 1, 1, 1976, 3, 21);
#endif // jalali
#if QT_CONFIG(islamiccivilcalendar)
    // TODO: confirm this is correct
    ADDROW(IslamicCivil, 1, 1, 1, 622, 7, 19);
#endif

#undef ADDROW
}

void tst_QCalendar::specific()
{
    QFETCH(QCalendar::System, system);
    QFETCH(int, sysyear);
    QFETCH(int, sysmonth);
    QFETCH(int, sysday);
    QFETCH(int, gregyear);
    QFETCH(int, gregmonth);
    QFETCH(int, gregday);

    const QCalendar cal(system);
    const QDate date(sysyear, sysmonth, sysday, cal), gregory(gregyear, gregmonth, gregday);
    QCOMPARE(date, gregory);
    QCOMPARE(gregory.year(cal), sysyear);
    QCOMPARE(gregory.month(cal), sysmonth);
    QCOMPARE(gregory.day(cal), sysday);
    QCOMPARE(date.year(), gregyear);
    QCOMPARE(date.month(), gregmonth);
    QCOMPARE(date.day(), gregday);
}

void tst_QCalendar::daily()
{
    QFETCH(QCalendar::System, system);
    QCalendar calendar(system);
    const quint64 startJDN = 0, endJDN = 2488070;
    // Iterate from -4713-01-01 (Julian calendar) to 2100-01-01
    for (quint64 expect = startJDN; expect <= endJDN; ++expect)
    {
        QDate date = QDate::fromJulianDay(expect);
        auto parts = calendar.partsFromDate(date);
        if (!parts.isValid())
            continue;

        const int year = date.year(calendar);
        QCOMPARE(year, parts.year);
        const int month = date.month(calendar);
        QCOMPARE(month, parts.month);
        const int day = date.day(calendar);
        QCOMPARE(day, parts.day);
        const quint64 actual = QDate(year, month, day, calendar).toJulianDay();
        QCOMPARE(actual, expect);
    }
}

QTEST_APPLESS_MAIN(tst_QCalendar)
#include "tst_qcalendar.moc"
