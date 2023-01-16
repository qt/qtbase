// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qglobal.h"
#include "qjuliancalendar_p.h"
#include "qromancalendar_data_p.h"
#include "qcalendarmath_p.h"
#include <QtCore/qmath.h>
#include <QtCore/qlocale.h>
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

// Julian Day 0 was January the first in the proleptic Julian calendar's 4713 BC

bool QJulianCalendar::dateToJulianDay(int year, int month, int day, qint64 *jd) const
{
    Q_ASSERT(jd);
    if (!isDateValid(year, month, day))
        return false;
    if (year < 0)
        ++year;
    const qint64 c0 = month < 3 ? -1 : 0;
    const qint64 j1 = qDiv<4>(1461 * (year + c0));
    const qint64 j2 = qDiv<5>(153 * month - 1836 * c0 - 457);
    *jd = j1 + j2 + day + 1721117;
    return true;
}

QCalendar::YearMonthDay QJulianCalendar::julianDayToDate(qint64 jd) const
{
    const auto k2dm = qDivMod<1461>(4 * (jd - 1721118) + 3);
    const auto k1dm = qDivMod<153>(5 * qDiv<4>(k2dm.remainder) + 2);
    const auto c0dm = qDivMod<12>(k1dm.quotient + 2);
    const int y = qint16(k2dm.quotient + c0dm.quotient);
    const int month = quint8(c0dm.remainder + 1);
    const int day = qDiv<5>(k1dm.remainder) + 1;
    return QCalendar::YearMonthDay(y > 0 ? y : y - 1, month, day);
}

QT_END_NAMESPACE
