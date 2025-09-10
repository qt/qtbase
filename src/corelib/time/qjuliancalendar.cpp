// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qglobal.h"
#include "qjuliancalendar_p.h"
#include "qcalendarmath_p.h"

#include <QtCore/qdatetime.h>

QT_BEGIN_NAMESPACE

using namespace QRoundingDown;

/*!
    \since 5.14

    \class QJulianCalendar
    \inmodule QtCore
    \brief The QJulianCalendar class provides Julian calendar system
    implementation.

    \section1 Julian Calendar

    The Julian calendar, proposed by Julius Caesar in 46 BC (708 AUC), was a
    reform of the Roman calendar. It took effect on 1 January 45 BC (AUC 709),
    by edict. It was the predominant calendar in the Roman world, most of
    Europe, and in European settlements in the Americas and elsewhere, until it
    was refined and gradually replaced by the Gregorian calendar,
    promulgated in 1582 by Pope Gregory XIII.

    The Julian calendar gains against the mean tropical year at the rate of one
    day in 128 years. For the Gregorian calendar, the figure is one day in
    3030 years. The difference in the average length of the year
    between Julian (365.25 days) and Gregorian (365.2425 days) is 0.002%.

    Source: \l {https://en.wikipedia.org/wiki/Julian_calendar}{Wikipedia page on
    Julian Calendar}
 */

QString QJulianCalendar::name() const
{
    return QStringLiteral("Julian");
}

QStringList QJulianCalendar::nameList()
{
    return { QStringLiteral("Julian") };
}

bool QJulianCalendar::isLeapYear(int year) const
{
    if (year == QCalendar::Unspecified || !year)
        return false;

    return qMod<4>(year < 0 ? year + 1 : year) == 0;
}

// Julian Day 0 was January the first in the proleptic Julian calendar's 4713 BC.
using namespace QRomanCalendrical;
// End a Julian four-year cycle on 1 BC's leap day (Gregorian Feb 27th):
constexpr qint64 JulianBaseJd = LeapDayGregorian1Bce - 2;

bool QJulianCalendar::dateToJulianDay(int year, int month, int day, qint64 *jd) const
{
    Q_ASSERT(jd);
    if (!isDateValid(year, month, day))
        return false;

    const auto yearDays = yearMonthToYearDays(year, month);
    *jd = qDiv<4>(FourYears * yearDays.year) + yearDays.days + day + JulianBaseJd;
    return true;
}

QCalendar::YearMonthDay QJulianCalendar::julianDayToDate(qint64 jd) const
{
    const auto year4Day = qDivMod<FourYears>(4 * (jd - JulianBaseJd) - 1);
    // Its remainder changes by 4 per day, except at roughly yearly quotient steps.
    const auto ymd = dayInYearToYmd(qDiv<4>(year4Day.remainder));
    const int y = year4Day.quotient + ymd.year;
    return QCalendar::YearMonthDay(y > 0 ? y : y - 1, ymd.month, ymd.day);
}

QT_END_NAMESPACE
