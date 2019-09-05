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

#include "qglobal.h"
#include "qromancalendar_p.h"
#include "qromancalendar_data_p.h"

QT_BEGIN_NAMESPACE

/*!
    \since 5.14

    \class QRomanCalendar
    \inmodule QtCore
    \brief The QRomanCalendar class is a shared base for calendars based on the
    ancient Roman calendar.

    \section1

    Calendars based on the ancient Roman calendar share the names of months,
    whose lengths depend in a common way on whether the year is a leap
    year. They differ in how they determine which years are leap years.

    \sa QGregorianCalendar, QJulianCalendar, QMilankovicCalendar
*/

int QRomanCalendar::daysInMonth(int month, int year) const
{
    if (!year || month < 1 || month > 12)
        return 0;

    if (month == 2)
        return isLeapYear(year) ? 29 : 28;

    // Long if odd up to July = 7, or if even from 8 = August onwards:
    return 30 | ((month & 1) ^ (month >> 3));
}

int QRomanCalendar::minimumDaysInMonth() const
{
    return 28;
}

bool QRomanCalendar::isLunar() const
{
    return false;
}

bool QRomanCalendar::isLuniSolar() const
{
    return false;
}

bool QRomanCalendar::isSolar() const
{
    return true;
}

const QCalendarLocale *QRomanCalendar::localeMonthIndexData() const
{
    return locale_data;
}

const ushort *QRomanCalendar::localeMonthData() const
{
    return months_data;
}

QT_END_NAMESPACE
