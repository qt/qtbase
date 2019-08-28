/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qglobal.h"
#include "qjalalicalendar_p.h"
#include "qjalalicalendar_data_p.h"
#include "qcalendarmath_p.h"
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

using namespace QRoundingDown;

// Constants

static const qint64 cycleDays = 1029983;
static const int cycleYears = 2820;
static const double yearLength = 365.24219858156028368; // 365 + leapRatio;
static const qint64 jalaliEpoch = 2121446; // 475/01/01 AP, start of 2820 cycle

// Calendar implementation

static inline int cycle(qint64 jdn)
{
    return qDiv(jdn - jalaliEpoch, cycleDays);
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

QJalaliCalendar::QJalaliCalendar()
    : QCalendarBackend(QStringLiteral("Jalali"), QCalendar::System::Jalali)
{
    registerAlias(QStringLiteral("Persian"));
}

QString QJalaliCalendar::name() const
{
    return QStringLiteral("Jalali");
}

QCalendar::System QJalaliCalendar::calendarSystem() const
{
    return QCalendar::System::Jalali;
}

bool QJalaliCalendar::isLeapYear(int year) const
{
    if (year == QCalendar::Unspecified)
        return false;
    if (year < 0)
        year++;
    return qMod((year + 2346) * 683, 2820) < 683;
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
    const int c = qDiv(y, cycleYears);
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
    return locale_data;
}

const ushort *QJalaliCalendar::localeMonthData() const
{
    return months_data;
}

QT_END_NAMESPACE
