// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QCalendar>
#include <private/qgregoriancalendar_p.h>

#include <limits>

Q_DECLARE_METATYPE(QCalendar::System)

using namespace Qt::StringLiterals;

class tst_QCalendar : public QObject
{
    Q_OBJECT
private:
    void checkYear(const QCalendar &cal, int year);

    // QDate's minJd(), maxJd() are private, so copy their values:
    static constexpr auto minDate = QDate::fromJulianDay(Q_INT64_C(-784350574879));
    static constexpr auto maxDate = QDate::fromJulianDay(Q_INT64_C( 784354017364));
private Q_SLOTS:
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
    void extremes_data();
    void extremes();

    void gregory();
};

static void checkCenturyResolution(const QCalendar &cal, const QCalendar::YearMonthDay &base)
{
    quint8 weekDayMask = 0;
    for (int offset = -7; offset < 8; ++offset) {
        const auto probe = QDate(base.year, base.month, base.day, cal).addYears(100 * offset, cal);
        const int dow = cal.dayOfWeek(probe);
        if (probe.isValid() && dow > 0 && dow < 8)
            weekDayMask |= 1 << quint8(dow - 1);
    }
    for (int j = 1; j < 8; ++j) {
        const bool seen = weekDayMask & (1 << quint8(j - 1));
        const QDate check = cal.matchCenturyToWeekday(base, j);
        if (check.isValid()) {
            const auto parts = cal.partsFromDate(check);
            const int dow = cal.dayOfWeek(check);
            QCOMPARE(dow, j);
            QCOMPARE(parts.day, base.day);
            QCOMPARE(parts.month, base.month);
            int gap = parts.year - base.year;
            if (!cal.hasYearZero() && (parts.year > 0) != (base.year > 0))
                gap += parts.year > 0 ? -1 : +1;
            auto report = qScopeGuard([parts, base]() {
                qDebug("Wrongly matched year: %d replaced %d", parts.year, base.year);
            });
            QCOMPARE(gap % 100, 0);
            // We searched 7 centuries each side of base.
            if (seen) {
                QCOMPARE_LT(gap / 100, 8);
                QCOMPARE_GT(gap / 100, -8);
            } else {
                QCOMPARE_GE(qAbs(gap) / 100, 8);
            }
            report.dismiss();
        } else {
            auto report = qScopeGuard([j, base]() {
                qDebug("Missed dow[%d] for %d/%d/%d", j, base.year, base.month, base.day);
            });
            QVERIFY(!seen);
            report.dismiss();
        }
    }
}

// Support for basic():
void tst_QCalendar::checkYear(const QCalendar &cal, int year)
{
    const int moons = cal.monthsInYear(year);
    // Months are numbered from 1 to moons:
    QCOMPARE_GT(moons, 0);
    QVERIFY(!cal.isDateValid(year, moons + 1, 1));
    QVERIFY(!cal.isDateValid(year, 0, 1));
    QVERIFY(!QDate(year, 0, 1, cal).isValid());
    QCOMPARE_LE(moons, cal.maximumMonthsInYear());
    QCOMPARE(cal.standaloneMonthName(QLocale::c(), moons + 1, year), QString());
    QCOMPARE(cal.monthName(QLocale::c(), 0, year), QString());

    const int days = cal.daysInYear(year);
    QCOMPARE_GT(days, 0);

    int sum = 0;
    const int longest = cal.maximumDaysInMonth();
    for (int i = moons; i > 0; --i) {
        const int last = cal.daysInMonth(i, year);
        sum += last;
        // Valid month has some days and no more than max:
        QCOMPARE_GT(last, 0);
        QCOMPARE_LE(last, longest);
        // Days are numbered from 1 to last:
        QVERIFY(cal.isDateValid(year, i, 1));
        QVERIFY(cal.isDateValid(year, i, last));
        QVERIFY(!cal.isDateValid(year, i, 0));
        QVERIFY(!cal.isDateValid(year, i, last + 1));
        // Unspecified year gets max daysInMonth():
        QCOMPARE_GE(cal.daysInMonth(i), last);

        checkCenturyResolution(cal, {year, i, (last + 1) / 2});
        if (QTest::currentTestFailed())
            return;
    }
    // Months add up to the whole year:
    QCOMPARE(sum, days);
}

#define CHECKYEAR(cal, year) checkYear(cal, year); \
    if (QTest::currentTestFailed()) \
        return

void tst_QCalendar::basic_data()
{
    QTest::addColumn<QCalendar::System>("system");

    const QMetaEnum e = QMetaEnum::fromType<QCalendar::System>();
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
            QCOMPARE_LT(cal.daysInYear(year), cal.daysInYear(leap));

        CHECKYEAR(cal, leap);
    }
    // Either year is non-leap or we have a decade of leap years together;
    // expect daysInMonth() to treat year the same as unspecified.
    CHECKYEAR(cal, year);
}

void tst_QCalendar::unspecified()
{
    QFETCH(QCalendar::System, system);
    QCalendar cal(system);

    const QDate today = QDate::currentDate();
    const int thisYear = today.year();
    QCOMPARE(cal.monthsInYear(QCalendar::Unspecified), cal.maximumMonthsInYear());
    for (int month = cal.maximumMonthsInYear(); month > 0; month--) {
        const int maxDays = cal.daysInMonth(month);
        bool hitMax = false;
        // 19 years = one Metonic cycle (used by some lunar calendars)
        for (int i = 19; i > 0; --i) {
            int days = cal.daysInMonth(month, thisYear - i);
            if (days == maxDays)
                hitMax = true;
            else
                QCOMPARE_LT(days, maxDays);
        }
        // Require a majority of the years tested:
        QVERIFY2(hitMax, "Default daysInMonth() should be the longest that month gets");
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
    QCOMPARE(QCalendar("islamic"_L1).name(), u"Islamic Civil");
    QCOMPARE(QCalendar(u"Islamic"_s).name(), u"Islamic Civil");
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
        QCOMPARE_GE(match, 1970);
        QCOMPARE_LE(match, 2400);
        // Either that's the year we started with or:
        if (match != year) {
            // Its last two digits can't be mistaken for month or day:
            QCOMPARE_NE(match % 100, probe.month());
            QCOMPARE_NE(match % 100, probe.day());
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

void tst_QCalendar::extremes_data()
{
    QTest::addColumn<QCalendar::System>("system");
    // First and last leap years representable as int for this system:
    QTest::addColumn<int>("minLeap");
    QTest::addColumn<int>("maxLeap");
    // First and last full dates representable by this system and QDate:
    QTest::addColumn<int>("minYear");
    QTest::addColumn<int>("minMonth");
    QTest::addColumn<int>("minDay");
    QTest::addColumn<int>("maxYear");
    QTest::addColumn<int>("maxMonth");
    QTest::addColumn<int>("maxDay");

    auto newRow = [](QCalendar::System sys, int minLeap, int maxLeap) {
        const QCalendar cal(sys);
        const QByteArray name = cal.name().toUtf8();
        auto minParts = cal.partsFromDate(minDate);
        auto maxParts = cal.partsFromDate(maxDate);
        // Those should produce invalid if an end of the range is beyond the
        // range of int year; in that case, use the start or end of the year at
        // the missed end of int year's range instead.
        using Bound = std::numeric_limits<int>;
        if (!minParts.isValid()) {
            // minDate is out of range, so year INT_MIN should be in range:
            minParts.year = Bound::min();
            minParts.month = minParts.day = 1;
        }
        if (!maxParts.isValid()) {
            // maxDate is out of range, so year INT_MAX should be in range:
            maxParts.year = Bound::max();
            maxParts.month = cal.monthsInYear(maxParts.year);
            maxParts.day = cal.daysInMonth(maxParts.month, maxParts.year);
            if (!maxParts.day) {
                qDebug("%s can't represent max QDate or the end of %d/%d",
                       name.data(), maxParts.year, maxParts.month);
            }
        }
#if 0 // Activate to see ranges of representable dates for each calendar:
        qDebug("%s covers from 0x%x/%d/%d to 0x%x/%d/%d", name.data(),
               minParts.year, minParts.month, minParts.day,
               maxParts.year, maxParts.month, maxParts.day);
        // If that reports a minParts.year > 0, or maxParts.year < 0,
        // daysInMonth() is failing to detect when out of range.
#endif

        QTest::newRow(name.data())
            << sys << minLeap << maxLeap
            << minParts.year << minParts.month << minParts.day
            << maxParts.year << maxParts.month << maxParts.day;
    };
#define NEWROW(sys, minLeap, maxLeap) newRow(QCalendar::System::sys, minLeap, maxLeap)

    NEWROW(Gregorian, -2147483645, 2147483644);
#ifndef QT_BOOTSTRAPPED
    NEWROW(Julian, -2147483645, 2147483644);
    NEWROW(Milankovic, -2147483645, 2147483644);
#endif
#if QT_CONFIG(jalalicalendar)
    NEWROW(Jalali, -2147483647, 2147483646);
#endif
#if QT_CONFIG(islamiccivilcalendar)
    NEWROW(IslamicCivil, -2147483647, 2147483647);
#endif
}

void tst_QCalendar::extremes()
{
    QFETCH(const QCalendar::System, system);
    QFETCH(const int, minLeap);
    QFETCH(const int, maxLeap);
    // First and last full dates representable by this system and QDate:
    QFETCH(const int, minYear);
    QFETCH(const int, minMonth);
    QFETCH(const int, minDay);
    QFETCH(const int, maxYear);
    QFETCH(const int, maxMonth);
    QFETCH(const int, maxDay);
    const QCalendar cal(system);

    QVERIFY(cal.isLeapYear(minLeap));
    QVERIFY(cal.isLeapYear(maxLeap));

    using Bound = std::numeric_limits<int>;
    const QDate early = cal.dateFromParts(minYear, minMonth, minDay);
    if (minYear == Bound::min() && minMonth == 1 && minDay == 1) {
        const auto parts = cal.partsFromDate(early);
        QCOMPARE(parts.year, minYear);
        QCOMPARE(parts.month, minMonth);
        QCOMPARE(parts.day, minDay);
    } else {
        QCOMPARE(early, minDate);
    }
    const QDate late = cal.dateFromParts(maxYear, maxMonth, maxDay);
    if (maxYear == Bound::max() && maxMonth == cal.monthsInYear(maxYear)
        && maxDay == cal.daysInMonth(maxMonth, maxYear)) {
        const auto parts = cal.partsFromDate(late);
        QCOMPARE(parts.year, maxYear);
        QCOMPARE(parts.month, maxMonth);
        QCOMPARE(parts.day, maxDay);
    } else {
        QCOMPARE(late, maxDate);
    }
}

QTEST_APPLESS_MAIN(tst_QCalendar)
#include "tst_qcalendar.moc"
