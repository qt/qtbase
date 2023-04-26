// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgregoriancalendar_p.h"
#include "qcalendarmath_p.h"

#include <QtCore/qdatetime.h>

QT_BEGIN_NAMESPACE

using namespace QRoundingDown;

// Verification that QRoundingDown::qDivMod() works correctly:
static_assert(qDivMod<2>(-86400).quotient == -43200);
static_assert(qDivMod<2>(-86400).remainder == 0);
static_assert(qDivMod<86400>(-86400).quotient == -1);
static_assert(qDivMod<86400>(-86400).remainder == 0);
static_assert(qDivMod<86400>(-86401).quotient == -2);
static_assert(qDivMod<86400>(-86401).remainder == 86399);
static_assert(qDivMod<86400>(-100000).quotient == -2);
static_assert(qDivMod<86400>(-100000).remainder == 72800);
static_assert(qDivMod<86400>(-172799).quotient == -2);
static_assert(qDivMod<86400>(-172799).remainder == 1);
static_assert(qDivMod<86400>(-172800).quotient == -2);
static_assert(qDivMod<86400>(-172800).remainder == 0);

// Uncomment to verify error on bad denominator is clear and intelligible:
// static_assert(qDivMod<1>(17).remainder == 0);
// static_assert(qDivMod<0>(17).remainder == 0);
// static_assert(qDivMod<std::numeric_limits<unsigned>::max()>(17).remainder == 0);

/*!
    \since 5.14

    \class QGregorianCalendar
    \inmodule QtCore
    \brief The QGregorianCalendar class implements the Gregorian calendar.

    \section1 The Gregorian Calendar

    The Gregorian calendar is a refinement of the earlier Julian calendar,
    itself a late form of the Roman calendar. It is widely used.

    \sa QRomanCalendar, QJulianCalendar, QCalendar
*/

QString QGregorianCalendar::name() const
{
    return QStringLiteral("Gregorian");
}

QStringList QGregorianCalendar::nameList()
{
    return {
        QStringLiteral("Gregorian"),
        QStringLiteral("gregory"),
    };
}

bool QGregorianCalendar::isLeapYear(int year) const
{
    return leapTest(year);
}

bool QGregorianCalendar::leapTest(int year)
{
    if (year == QCalendar::Unspecified)
        return false;

    // No year 0 in Gregorian calendar, so -1, -5, -9 etc are leap years
    if (year < 1)
        ++year;

    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

// Duplicating code from QRomanCalendar, but inlining isLeapYear() as leapTest():
int QGregorianCalendar::monthLength(int month, int year)
{
    if (month < 1 || month > 12)
        return 0;

    if (month == 2)
        return leapTest(year) ? 29 : 28;

    return 30 | ((month & 1) ^ (month >> 3));
}

bool QGregorianCalendar::validParts(int year, int month, int day)
{
    return year && 0 < day && day <= monthLength(month, year);
}

int QGregorianCalendar::weekDayOfJulian(qint64 jd)
{
    return int(qMod<7>(jd) + 1);
}

bool QGregorianCalendar::dateToJulianDay(int year, int month, int day, qint64 *jd) const
{
    const auto maybe = julianFromParts(year, month, day);
    if (maybe)
        *jd = *maybe;
    return bool(maybe);
}

QCalendar::YearMonthDay QGregorianCalendar::julianDayToDate(qint64 jd) const
{
    return partsFromJulian(jd);
}

int QGregorianCalendar::yearStartWeekDay(int year)
{
    // Equivalent to weekDayOfJulian(julianForParts({year, 1, 1})
    const int y = year - (year < 0 ? 800 : 801);
    return qMod<7>(y + qDiv<4>(y) - qDiv<100>(y) + qDiv<400>(y)) + 1;
}

int QGregorianCalendar::yearSharingWeekDays(QDate date)
{
    // Returns a post-epoch year, no later than 2400, that has the same pattern
    // of week-days (in the proleptic Gregorian calendar) as the year in which
    // the given date falls. This will be the year in question if it's in the
    // given range. Otherwise, the returned year's last two (decimal) digits
    // won't coincide with the month number or day-of-month of the given date.
    // For positive years, except when necessary to avoid such a clash, the
    // returned year's last two digits shall coincide with those of the original
    // year.

    // Needed when formatting dates using system APIs with limited year ranges
    // and possibly only a two-digit year. (The need to be able to safely
    // replace the two-digit form of the returned year with a suitable form of
    // the true year, when they don't coincide, is why the last two digits are
    // treated specially.)

    static_assert((400 * 365 + 97) % 7 == 0);
    // A full 400-year cycle of the Gregorian calendar has 97 + 400 * 365 days;
    // as 365 is one more than a multiple of seven and 497 is a multiple of
    // seven, that full cycle is a whole number of weeks. So adding a multiple
    // of four hundred years should get us a result that meets our needs.

    const int year = date.year();
    int res = (year < 1970
               ? 2400 - (2000 - (year < 0 ? year + 1 : year)) % 400
               : year > 2399 ? 2000 + (year - 2000) % 400 : year);
    Q_ASSERT(res > 0);
    if (res != year) {
        const int lastTwo = res % 100;
        if (lastTwo == date.month() || lastTwo == date.day()) {
            Q_ASSERT(lastTwo && !(lastTwo & ~31));
            // Last two digits of these years are all > 31:
            static constexpr int usual[] = { 2198, 2199, 2098, 2099, 2399, 2298, 2299 };
            static constexpr int leaps[] = { 2396, 2284, 2296, 2184, 2196, 2084, 2096 };
            // Indexing is: first day of year's day-of-week, Monday = 0, one less
            // than Qt's, as it's simpler to subtract one than to s/7/0/.
            res = (leapTest(year) ? leaps : usual)[yearStartWeekDay(year) - 1];
        }
        Q_ASSERT(QDate(res, 1, 1).dayOfWeek() == QDate(year, 1, 1).dayOfWeek());
        Q_ASSERT(QDate(res, 12, 31).dayOfWeek() == QDate(year, 12, 31).dayOfWeek());
    }
    Q_ASSERT(res >= 1970 && res <= 2400);
    return res;
}

/*
 * Math from The Calendar FAQ at http://www.tondering.dk/claus/cal/julperiod.php
 * This formula is correct for all julian days, when using mathematical integer
 * division (round to negative infinity), not c++11 integer division (round to zero).
 *
 * The source given uses 4801 BCE as base date; the following adjusts that by
 * 4800 years to simplify part of the arithmetic (and match more closely what we
 * do for Milankovic).
 */

using namespace QRomanCalendrical;
// End a Gregorian four-century cycle on 1 BC's leap day:
constexpr qint64 BaseJd = LeapDayGregorian1Bce;
// Every four centures there are 97 leap years:
constexpr unsigned FourCenturies = 400 * 365 + 97;

std::optional<qint64> QGregorianCalendar::julianFromParts(int year, int month, int day)
{
    if (!validParts(year, month, day))
        return std::nullopt;

    const auto yearDays = yearMonthToYearDays(year, month);
    const qint64 y = yearDays.year;
    const qint64 fromYear = 365 * y + qDiv<4>(y) - qDiv<100>(y) + qDiv<400>(y);
    return fromYear + yearDays.days + day + BaseJd;
}

QCalendar::YearMonthDay QGregorianCalendar::partsFromJulian(qint64 jd)
{
    const qint64 dayNumber = jd - BaseJd;
    const qint64 century = qDiv<FourCenturies>(4 * dayNumber - 1);
    const int dayInCentury = dayNumber - qDiv<4>(FourCenturies * century);

    const int yearInCentury = qDiv<FourYears>(4 * dayInCentury - 1);
    const int dayInYear = dayInCentury - qDiv<4>(FourYears * yearInCentury);
    const int m = qDiv<FiveMonths>(5 * dayInYear - 3);
    Q_ASSERT(m < 12 && m >= 0);
    // That m is a month adjusted to March = 0, with Jan = 10, Feb = 11 in the previous year.
    const int yearOffset = m < 10 ? 0 : 1;

    const int y = 100 * century + yearInCentury + yearOffset;
    const int month = m + 3 - 12 * yearOffset;
    const int day = dayInYear - qDiv<5>(FiveMonths * m + 2);

    // Adjust for no year 0
    return QCalendar::YearMonthDay(y > 0 ? y : y - 1, month, day);
}

QT_END_NAMESPACE
