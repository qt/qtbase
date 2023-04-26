// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include <QCalendar>
#include <private/qgregoriancalendar_p.h>
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
    void testYearMonthDate();
    void properties_data();
    void properties();
    void aliases();

    void gregory();
};

// Support for basic():
void tst_QCalendar::checkYear(const QCalendar &cal, int year, bool normal)
{
    const int moons = cal.monthsInYear(year);
    // Months are numbered from 1 to moons:
    QVERIFY(moons > 0);
    QVERIFY(!cal.isDateValid(year, moons + 1, 1));
    QVERIFY(!cal.isDateValid(year, 0, 1));
    QVERIFY(!QDate(year, 0, 1, cal).isValid());
    QVERIFY(moons <= cal.maximumMonthsInYear());
    QCOMPARE(cal.standaloneMonthName(QLocale::c(), moons + 1, year), QString());
    QCOMPARE(cal.monthName(QLocale::c(), 0, year), QString());

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
        QVERIFY(!QDate(0, 1, 1, cal).isValid());
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
    QTest::addColumn<QString>("monthName");
    QTest::addColumn<int>("sysyear");
    QTest::addColumn<int>("sysmonth");
    QTest::addColumn<int>("sysday");
    // Gregorian equivalent:
    QTest::addColumn<int>("gregyear");
    QTest::addColumn<int>("gregmonth");
    QTest::addColumn<int>("gregday");

#define ADDROW(cal, monthName, year, month, day, gy, gm, gd) \
    QTest::newRow(#cal) << QCalendar::System::cal << QStringLiteral(monthName) \
                        << year << month << day << gy << gm << gd

    ADDROW(Gregorian, "January", 1970, 1, 1, 1970, 1, 1);

    // One known specific date, for each calendar
#ifndef QT_BOOTSTRAPPED
    // Julian 1582-10-4 was followed by Gregorian 1582-10-15
    ADDROW(Julian, "October", 1582, 10, 4, 1582, 10, 14);
    // Milankovic matches Gregorian for a few centuries
    ADDROW(Milankovic, "March", 1923, 3, 20, 1923, 3, 20);
#endif

#if QT_CONFIG(jalalicalendar)
    // Jalali year 1355 started on Gregorian 1976-3-21:
    ADDROW(Jalali, "Farvardin", 1355, 1, 1, 1976, 3, 21);
#endif // jalali
#if QT_CONFIG(islamiccivilcalendar)
    // TODO: confirm this is correct
    ADDROW(IslamicCivil, "Muharram", 1, 1, 1, 622, 7, 19);
#endif

#undef ADDROW
}

void tst_QCalendar::specific()
{
    QFETCH(QCalendar::System, system);
    QFETCH(const QString, monthName);
    QFETCH(int, sysyear);
    QFETCH(int, sysmonth);
    QFETCH(int, sysday);
    QFETCH(int, gregyear);
    QFETCH(int, gregmonth);
    QFETCH(int, gregday);

    const QCalendar cal(system);
    QCOMPARE(cal.monthName(QLocale::c(), sysmonth), monthName);
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

void tst_QCalendar::testYearMonthDate()
{
    QCalendar::YearMonthDay defYMD;
    QCOMPARE(defYMD.year, QCalendar::Unspecified);
    QCOMPARE(defYMD.month, QCalendar::Unspecified);
    QCOMPARE(defYMD.day, QCalendar::Unspecified);

    QCalendar::YearMonthDay ymd2020(2020);
    QCOMPARE(ymd2020.year, 2020);
    QCOMPARE(ymd2020.month, 1);
    QCOMPARE(ymd2020.day, 1);

    QVERIFY(!QCalendar::YearMonthDay(
            QCalendar::Unspecified, QCalendar::Unspecified, QCalendar::Unspecified).isValid());
    QVERIFY(!QCalendar::YearMonthDay(
            QCalendar::Unspecified, QCalendar::Unspecified, 1).isValid());
    QVERIFY(!QCalendar::YearMonthDay(
            QCalendar::Unspecified, 1, QCalendar::Unspecified).isValid());
    QVERIFY(QCalendar::YearMonthDay(
            QCalendar::Unspecified, 1, 1).isValid());
    QVERIFY(!QCalendar::YearMonthDay(
            2020, QCalendar::Unspecified, QCalendar::Unspecified).isValid());
    QVERIFY(!QCalendar::YearMonthDay(
            2020, QCalendar::Unspecified, 1).isValid());
    QVERIFY(!QCalendar::YearMonthDay(
            2020, 1, QCalendar::Unspecified).isValid());
    QVERIFY(QCalendar::YearMonthDay(
            2020, 1, 1).isValid());
}

void tst_QCalendar::properties_data()
{
    QTest::addColumn<QCalendar::System>("system");
    QTest::addColumn<bool>("gregory");
    QTest::addColumn<bool>("lunar");
    QTest::addColumn<bool>("luniSolar");
    QTest::addColumn<bool>("solar");
    QTest::addColumn<bool>("proleptic");
    QTest::addColumn<bool>("yearZero");
    QTest::addColumn<int>("monthMax");
    QTest::addColumn<int>("monthMin");
    QTest::addColumn<int>("yearMax");
    QTest::addColumn<QString>("name");

    QTest::addRow("Gregorian")
        << QCalendar::System::Gregorian << true << false << false << true << true << false
        << 31 << 28 << 12 << QStringLiteral("Gregorian");
#ifndef QT_BOOTSTRAPPED
    QTest::addRow("Julian")
        << QCalendar::System::Julian << false << false << false << true << true << false
        << 31 << 28 << 12 << QStringLiteral("Julian");
    QTest::addRow("Milankovic")
        << QCalendar::System::Milankovic << false << false << false << true << true << false
        << 31 << 28 << 12 << QStringLiteral("Milankovic");
#endif

#if QT_CONFIG(jalalicalendar)
    QTest::addRow("Jalali")
        << QCalendar::System::Jalali << false << false << false << true << true << false
        << 31 << 29 << 12 << QStringLiteral("Jalali");
#endif
#if QT_CONFIG(islamiccivilcalendar)
    QTest::addRow("IslamicCivil")
        << QCalendar::System::IslamicCivil << false << true << false << false << true << false
        << 30 << 29 << 12 << QStringLiteral("Islamic Civil");
#endif
}

void tst_QCalendar::properties()
{
    QFETCH(const QCalendar::System, system);
    QFETCH(const bool, gregory);
    QFETCH(const bool, lunar);
    QFETCH(const bool, luniSolar);
    QFETCH(const bool, solar);
    QFETCH(const bool, proleptic);
    QFETCH(const bool, yearZero);
    QFETCH(const int, monthMax);
    QFETCH(const int, monthMin);
    QFETCH(const int, yearMax);
    QFETCH(const QString, name);

    const QCalendar cal(system);
    QCOMPARE(cal.isGregorian(), gregory);
    QCOMPARE(cal.isLunar(), lunar);
    QCOMPARE(cal.isLuniSolar(), luniSolar);
    QCOMPARE(cal.isSolar(), solar);
    QCOMPARE(cal.isProleptic(), proleptic);
    QCOMPARE(cal.hasYearZero(), yearZero);
    QCOMPARE(cal.maximumDaysInMonth(), monthMax);
    QCOMPARE(cal.minimumDaysInMonth(), monthMin);
    QCOMPARE(cal.maximumMonthsInYear(), yearMax);
    QCOMPARE(cal.name(), name);
}

void tst_QCalendar::aliases()
{
    QCOMPARE(QCalendar(u"gregory").name(), u"Gregorian");
#if QT_CONFIG(jalalicalendar)
    QCOMPARE(QCalendar(u"Persian").name(), u"Jalali");
#endif
#if QT_CONFIG(islamiccivilcalendar)
    // Exercise all constructors from name, while we're at it:
    QCOMPARE(QCalendar(u"islamic-civil").name(), u"Islamic Civil");
    QCOMPARE(QCalendar(QLatin1String("islamic")).name(), u"Islamic Civil");
    QCOMPARE(QCalendar(QStringLiteral("Islamic")).name(), u"Islamic Civil");
#endif

    // Invalid is handled gracefully:
    QCOMPARE(QCalendar(u"").name(), QString());
    QCOMPARE(QCalendar(QCalendar::System::User).name(), QString());
}

void tst_QCalendar::gregory()
{
    // Test QGregorianCalendar's internal-use methods.

    // Julian day number 0 is in 4713; and reach past the end of four-digit years:
    for (int year = -4720; year < 12345; ++year) {
        // Test yearStartWeekDay() and yearSharingWeekDays() are consistent with
        // dateToJulianDay() and weekDayOfJulian():
        if (!year) // No year zero.
            continue;
        const auto first = QGregorianCalendar::julianFromParts(year, 1, 1);
        QVERIFY2(first, "Only year zero should lack a first day");
        QCOMPARE(QGregorianCalendar::yearStartWeekDay(year),
                 QGregorianCalendar::weekDayOfJulian(*first));
        const auto last = QGregorianCalendar::julianFromParts(year, 12, 31);
        QVERIFY2(last, "Only year zero should lack a last day");

        const int lastTwo = (year + (year < 0 ? 1 : 0)) % 100 + (year < -1 ? 100 : 0);
        const QDate probe(year, lastTwo && lastTwo <= 12 ? lastTwo : 8,
                          lastTwo <= 31 && lastTwo > 12 ? lastTwo : 17);
        const int match = QGregorianCalendar::yearSharingWeekDays(probe);
        // A post-epoch year, no later than 2400 (implies four-digit):
        QVERIFY(match >= 1970);
        QVERIFY(match <= 2400);
        // Either that's the year we started with or:
        if (match != year) {
            // Its last two digits can't be mistaken for month or day:
            QVERIFY(match % 100 != probe.month());
            QVERIFY(match % 100 != probe.day());
            // If that wasn't in danger of happening, with year positive, they match lastTwo:
            if (year > 0 && lastTwo > 31)
                QCOMPARE(match % 100, lastTwo);
            // Its first and last days of the year match those of year:
            auto day = QGregorianCalendar::julianFromParts(match, 1, 1);
            QVERIFY(day);
            QCOMPARE(QGregorianCalendar::weekDayOfJulian(*day),
                     QGregorianCalendar::weekDayOfJulian(*first));
            day = QGregorianCalendar::julianFromParts(match, 12, 31);
            QVERIFY(day);
            QCOMPARE(QGregorianCalendar::weekDayOfJulian(*day),
                     QGregorianCalendar::weekDayOfJulian(*last));
        }
    }
}

QTEST_APPLESS_MAIN(tst_QCalendar)
#include "tst_qcalendar.moc"
