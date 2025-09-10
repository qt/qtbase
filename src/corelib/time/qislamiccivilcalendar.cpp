// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qglobal.h"
#include "qislamiccivilcalendar_p.h"
#include "qcalendarmath_p.h"

QT_BEGIN_NAMESPACE

using namespace QRoundingDown;

/*!
    \since 5.14
    \internal

    \class QIslamicCivilCalendar
    \inmodule QtCore
    \brief Implements a commonly-used computed version of the Islamic calendar.

    \section1 Civil Islamic Calendar

    QIslamicCivilCalendar implements a tabular version of the Hijri calendar
    which is known as the Islamic Civil Calendar. It has the same numbering of
    years and months, but the months are determined by arithmetical rules rather
    than by observation or astronomical calculations.

    \section2 Calendar Organization

    The civil calendar follows the usual tabular scheme of odd-numbered months
    and the last month of each leap year being 30 days long, the rest being 29
    days long. Its determination of leap years follows a 30-year cycle, in each
    of which the years 2, 5, 7, 10, 13, 16, 18, 21, 24, 26 and 29 are leap
    years.

    \sa QHijriCalendar, QCalendar
*/

QString QIslamicCivilCalendar::name() const
{
    return QStringLiteral("Islamic Civil");
}

QStringList QIslamicCivilCalendar::nameList()
{
    return {
        QStringLiteral("Islamic Civil"),
        QStringLiteral("islamic-civil"), // CLDR name
        QStringLiteral("islamicc"), // old CLDR name, still (2018) used by Mozilla
        // Until we have a concrete implementation that knows all the needed ephemerides:
        QStringLiteral("Islamic"),
    };
}

bool QIslamicCivilCalendar::isLeapYear(int year) const
{
    if (year == QCalendar::Unspecified)
        return false;
    if (year < 0)
        ++year;
    return qMod<30>(year * 11 + 14) < 11;
}

// First day of first year (Gregorian 622 CE July 19th) is the base date here:
constexpr qint64 EpochJd = 1948440;
// Each 30 years has 11 leap years of 355 days and 19 ordinary years of 354:
constexpr unsigned ThirtyYears = 11 * 355 + 19 * 354;
// The first eleven months of the year alternate 30, 29, ..., 29, 30 days in length.
constexpr unsigned ElevenMonths = 6 * 30 + 5 * 29;

bool QIslamicCivilCalendar::dateToJulianDay(int year, int month, int day, qint64 *jd) const
{
    Q_ASSERT(jd);
    if (!isDateValid(year, month, day))
        return false;

    *jd = qDiv<30>(qint64(ThirtyYears) * (year > 0 ? year - 1 : year) + 14)
        + qDiv<11>(ElevenMonths * (month - 1) + 5)
        + day + EpochJd - 1;
    return true;
}

QCalendar::YearMonthDay QIslamicCivilCalendar::julianDayToDate(qint64 jd) const
{
    const auto year30Day = qDivMod<ThirtyYears>(30 * (jd - EpochJd) + 15);
    // Its remainder changes by 30 per day, except roughly yearly.
    const auto month11Day = qDivMod<ElevenMonths>(11 * qDiv<30>(year30Day.remainder) + 5);
    // Its remainder changes by 11 per day except roughly monthly.
    const int month = month11Day.quotient + 1;
    const int day = qDiv<11>(month11Day.remainder) + 1;
    const int y = year30Day.quotient + 1;
    return QCalendar::YearMonthDay(y > 0 ? y : y - 1, month, day);
}

QT_END_NAMESPACE
