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

    return qMod(year < 0 ? year + 1 : year, 4) == 0;
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
    const qint64 j1 = qDiv(1461 * (year + c0), 4);
    const qint64 j2 = qDiv(153 * month - 1836 * c0 - 457, 5);
    *jd = j1 + j2 + day + 1721117;
    return true;
}

QCalendar::YearMonthDay QJulianCalendar::julianDayToDate(qint64 jd) const
{
    const qint64 y2 = jd - 1721118;
    const qint64 k2 = 4 * y2 + 3;
    const qint64 k1 = 5 * qDiv(qMod(k2, 1461), 4) + 2;
    const qint64 x1 = qDiv(k1, 153);
    const qint64 c0 = qDiv(x1 + 2, 12);
    const int y = qint16(qDiv(k2, 1461) + c0);
    const int month = quint8(x1 - 12 * c0 + 3);
    const int day = qDiv(qMod(k1, 153), 5) + 1;
    return QCalendar::YearMonthDay(y > 0 ? y : y - 1, month, day);
}

QT_END_NAMESPACE
