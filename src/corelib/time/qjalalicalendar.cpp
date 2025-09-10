// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qglobal.h"
#include "qjalalicalendar_p.h"
#include "qjalalicalendar_data_p.h"
#include "qcalendarmath_p.h"
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

using namespace QRoundingDown;

// Constants

constexpr qint64 cycleDays = 1029983;
constexpr int cycleYears = 2820;
constexpr double yearLength = 365.24219858156028368; // 365 + 683 / 2820.
constexpr qint64 jalaliEpoch = 2121446; // 475/01/01 AP, start of 2820 cycle
// This appears to be based on Ahmad Birashk's algorithm.

// Calendar implementation

static inline int cycle(qint64 jdn)
{
    return qDiv<cycleDays>(jdn - jalaliEpoch);
}

qint64 cycleStart(int cycleNo)
{
    return jalaliEpoch + cycleNo * cycleDays;
}

qint64 firstDayOfYear(int year, int cycleNo)
{
    qint64 firstDOYinEra = static_cast<qint64>(qFloor(year * yearLength));
    return jalaliEpoch + cycleNo * cycleDays + firstDOYinEra;
}

/*!
    \since 5.14

    \class QJalaliCalendar
    \inmodule QtCore
    \brief The QJalaliCalendar class provides Jalali (Hijri Shamsi) calendar
    system implementation.

    \section1 Solar Hijri Calendar System

    The Solar Hijri calendar, also called the Solar Hejri calendar, Shamsi
    Hijri calendar or Jalali calendar, is the official calendar of Iran and
    Afghanistan. It begins on the vernal equinox (Nowruz) as determined by
    astronomical calculation for the Iran Standard Time meridian
    (52.5°E or GMT+3.5h). This determination of starting moment is more accurate
    than the Gregorian calendar for predicting the date of the vernal equinox,
    because it uses astronomical observations rather than mathematical rules.

    \section2 Calendar Organization

    Each of the twelve months corresponds with a zodiac sign. The first six
    months have 31 days, the next five have 30 days, and the last month has 29
    days in usual years but 30 days in leap years. The New Year's Day always
    falls on the March equinox.

    \section2 Leap Year Rules

    The Solar Hijri calendar produces a five-year leap year interval after about
    every seven four-year leap year intervals. It usually follows a 33-year
    cycle with occasional interruptions by single 29-year or 37-year subcycles.
    The reason for this behavior is that it tracks the observed vernal equinox.
    By contrast, some less accurate predictive algorithms are in use based
    on confusion between the average tropical year (365.2422 days, approximated
    with near 128-year cycles or 2820-year great cycles) and the mean interval
    between spring equinoxes (365.2424 days, approximated with a near 33-year
    cycle).

    Source: \l {https://en.wikipedia.org/wiki/Solar_Hijri_calendar}{Wikipedia
    page on Solar Hijri Calendar}
*/

QString QJalaliCalendar::name() const
{
    return QStringLiteral("Jalali");
}

QStringList QJalaliCalendar::nameList()
{
    return {
        QStringLiteral("Jalali"),
        QStringLiteral("Persian"),
    };
}

bool QJalaliCalendar::isLeapYear(int year) const
{
    if (year == QCalendar::Unspecified)
        return false;
    if (year < 0)
        year++;
    return qMod<2820>((year + 2346) * 683) < 683;
}

bool QJalaliCalendar::isLunar() const
{
    return false;
}

bool QJalaliCalendar::isLuniSolar() const
{
    return false;
}

bool QJalaliCalendar::isSolar() const
{
    return true;
}

bool QJalaliCalendar::dateToJulianDay(int year, int month, int day, qint64 *jd) const
{
    Q_ASSERT(jd);
    if (!isDateValid(year, month, day))
        return false;

    const int y = year - (year < 0 ? 474 : 475);
    const int c = qDiv<cycleYears>(y);
    const int yearInCycle = y - c * cycleYears;
    int dayInYear = day;
    for (int i = 1; i < month; ++i)
        dayInYear += daysInMonth(i, year);
    *jd = firstDayOfYear(yearInCycle, c) + dayInYear - 1;
    return true;
}

QCalendar::YearMonthDay QJalaliCalendar::julianDayToDate(qint64 jd) const
{
    const int c = cycle(jd);
    int yearInCycle = qFloor((jd - cycleStart(c)) / yearLength);
    int year = yearInCycle + 475 + c * cycleYears;
    int day = jd - firstDayOfYear(yearInCycle, c) + 1;
    if (day > daysInYear(year <= 0 ? year - 1 : year)) {
        year++;
        day = 1;
    }
    if (year <= 0)
        year--;
    int month;
    for (month = 1; month < 12; ++month) {
        const int last = daysInMonth(month, year);
        if (day <= last)
            break;
        day -= last;
    }
    return QCalendar::YearMonthDay(year, month, day);
}

int QJalaliCalendar::daysInMonth(int month, int year) const
{
    if (year && month > 0 && month <= 12)
        return month < 7 ? 31 : month < 12 || isLeapYear(year) ? 30 : 29;

    return 0;
}

const QCalendarLocale *QJalaliCalendar::localeMonthIndexData() const
{
    return QtPrivate::Jalali::locale_data;
}

const char16_t *QJalaliCalendar::localeMonthData() const
{
    return QtPrivate::Jalali::months_data;
}

QT_END_NAMESPACE
