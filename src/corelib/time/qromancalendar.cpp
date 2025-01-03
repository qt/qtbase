// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

    Calendars based on the ancient Roman calendar have several common properties:
    they have the same names for months, the month lengths depend in a common
    way on whether the year is a leap year. They differ in how they determine
    which years are leap years.

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
    return QtPrivate::Roman::locale_data;
}

const char16_t *QRomanCalendar::localeMonthData() const
{
    return QtPrivate::Roman::months_data;
}

QT_END_NAMESPACE
