/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qgregoriancalendar_p.h"
#include "qcalendarmath_p.h"
#include <QtCore/qdatetime.h>

QT_BEGIN_NAMESPACE

using namespace QRoundingDown;

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

QGregorianCalendar::QGregorianCalendar()
    : QRomanCalendar(QStringLiteral("Gregorian"), QCalendar::System::Gregorian)
{
    registerAlias(QStringLiteral("gregory"));
}

QString QGregorianCalendar::name() const
{
    return QStringLiteral("Gregorian");
}

QCalendar::System QGregorianCalendar::calendarSystem() const
{
    return QCalendar::System::Gregorian;
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
    return qMod(jd, 7) + 1;
}

bool QGregorianCalendar::dateToJulianDay(int year, int month, int day, qint64 *jd) const
{
    return julianFromParts(year, month, day, jd);
}

bool QGregorianCalendar::julianFromParts(int year, int month, int day, qint64 *jd)
{
    Q_ASSERT(jd);
    if (!validParts(year, month, day))
        return false;

    if (year < 0)
        ++year;

    /*
     * Math from The Calendar FAQ at http://www.tondering.dk/claus/cal/julperiod.php
     * This formula is correct for all julian days, when using mathematical integer
     * division (round to negative infinity), not c++11 integer division (round to zero)
     */
    int    a = month < 3 ? 1 : 0;
    qint64 y = qint64(year) + 4800 - a;
    int    m = month + 12 * a - 3;
    *jd = day + qDiv(153 * m + 2, 5) - 32045
        + 365 * y + qDiv(y, 4) - qDiv(y, 100) + qDiv(y, 400);
    return true;
}

QCalendar::YearMonthDay QGregorianCalendar::julianDayToDate(qint64 jd) const
{
    return partsFromJulian(jd);
}

QCalendar::YearMonthDay QGregorianCalendar::partsFromJulian(qint64 jd)
{
    /*
     * Math from The Calendar FAQ at http://www.tondering.dk/claus/cal/julperiod.php
     * This formula is correct for all julian days, when using mathematical integer
     * division (round to negative infinity), not c++11 integer division (round to zero)
     */
    qint64 a = jd + 32044;
    qint64 b = qDiv(4 * a + 3, 146097);
    int    c = a - qDiv(146097 * b, 4);

    int    d = qDiv(4 * c + 3, 1461);
    int    e = c - qDiv(1461 * d, 4);
    int    m = qDiv(5 * e + 2, 153);

    int y = 100 * b + d - 4800 + qDiv(m, 10);

    // Adjust for no year 0
    int year = y > 0 ? y : y - 1;
    int month = m + 3 - 12 * qDiv(m, 10);
    int day = e - qDiv(153 * m + 2, 5) + 1;

    return QCalendar::YearMonthDay(year, month, day);
}

QT_END_NAMESPACE
