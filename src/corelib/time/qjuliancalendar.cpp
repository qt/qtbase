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

QJulianCalendar::QJulianCalendar()
    : QRomanCalendar(QStringLiteral("Julian"), QCalendar::System::Julian) {}

QString QJulianCalendar::name() const
{
    return QStringLiteral("Julian");
}

QCalendar::System QJulianCalendar::calendarSystem() const
{
    return QCalendar::System::Julian;
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
