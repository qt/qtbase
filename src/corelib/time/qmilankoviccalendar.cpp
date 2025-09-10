// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qglobal.h"
#include "qmilankoviccalendar_p.h"
#include "qcalendarmath_p.h"

#include <QtCore/qdatetime.h>

QT_BEGIN_NAMESPACE

using namespace QRoundingDown;

/*!
    \since 5.14

    \class QMilankovicCalendar
    \inmodule QtCore
    \brief The QMilankovicCalendar class provides Milanković calendar system
    implementation.

    The Revised Julian calendar, also known as the Milanković calendar, or,
    less formally, new calendar, is a calendar, developed and proposed by the
    Serbian scientist Milutin Milanković in 1923, which effectively discontinued
    the 340 years of divergence between the naming of dates sanctioned by those
    Eastern Orthodox churches adopting it and the Gregorian calendar that has
    come to predominate worldwide. This calendar was intended to replace the
    ecclesiastical calendar based on the Julian calendar hitherto in use by all
    of the Eastern Orthodox Church. The Revised Julian calendar temporarily
    aligned its dates with the Gregorian calendar proclaimed in 1582 by Pope
    Gregory XIII for adoption by the Christian world. The calendar has been
    adopted by the Orthodox churches of Constantinople, Albania, Alexandria,
    Antioch, Bulgaria, Cyprus, Greece, Poland, and Romania.

    Source: \l {https://en.wikipedia.org/wiki/Revised_Julian_calendar}{Wikipedia
    page on Milanković Calendar}
 */

QString QMilankovicCalendar::name() const
{
    return QStringLiteral("Milankovic");
}

QStringList QMilankovicCalendar::nameList()
{
    return { QStringLiteral("Milankovic") };
}

bool QMilankovicCalendar::isLeapYear(int year) const
{
    if (year == QCalendar::Unspecified)
        return false;
    if (year <= 0)
        ++year;
    if (qMod<4>(year))
        return false;
    const auto yeardm = qDivMod<100>(year);
    if (yeardm.remainder == 0) {
        const qint16 century = qMod<9>(yeardm.quotient);
        if (century != 2 && century != 6)
            return false;
    }
    return true;
}

using namespace QRomanCalendrical;
// End a Milankovic nine-century cycle on 1 BC, Feb 28 (Gregorian Feb 29):
constexpr qint64 MilankovicBaseJd = LeapDayGregorian1Bce;
// Leap years every 4 years, except for 7 turn-of-century years per nine centuries:
constexpr unsigned NineCenturies = 365 * 900 + 900 / 4 - 7;
// When the turn-of-century is a leap year, the century has 25 leap years in it:
constexpr unsigned LeapCentury = 365 * 100 + 25;

bool QMilankovicCalendar::dateToJulianDay(int year, int month, int day, qint64 *jd) const
{
    Q_ASSERT(jd);
    if (!isDateValid(year, month, day))
        return false;

    const auto yearDays = yearMonthToYearDays(year, month);
    const auto centuryYear = qDivMod<100>(yearDays.year);
    const qint64 fromYear = qDiv<9>(NineCenturies * centuryYear.quotient + 6)
                          + qDiv<100>(LeapCentury * centuryYear.remainder);
    *jd = fromYear + yearDays.days + day + MilankovicBaseJd;
    return true;
}

QCalendar::YearMonthDay  QMilankovicCalendar::julianDayToDate(qint64 jd) const
{
    const auto century9Day = qDivMod<NineCenturies>(9 * (jd - MilankovicBaseJd) - 7);
    // Its remainder changes by 9 per day, except roughly once per century.
    const auto year100Day = qDivMod<LeapCentury>(100 * qDiv<9>(century9Day.remainder) + 99);
    // Its remainder changes by 100 per day, except roughly once per year.
    const auto ymd = dayInYearToYmd(qDiv<100>(year100Day.remainder));
    const int y = 100 * century9Day.quotient + year100Day.quotient + ymd.year;
    return QCalendar::YearMonthDay(y > 0 ? y : y - 1, ymd.month, ymd.day);
}

QT_END_NAMESPACE
