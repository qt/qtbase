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
#include "qhijricalendar_p.h"
#include "qhijricalendar_data_p.h"

QT_BEGIN_NAMESPACE

/*!
    \since 5.14
    \internal

    \class QHijriCalendar
    \inmodule QtCore
    \brief The QHijriCalendar class supports Islamic (Hijri) calendar implementations.

    \section1 Islamic Calendar System

    The Islamic, Muslim, or Hijri calendar is a lunar calendar consisting of 12
    months in a year of 354 or 355 days. It is used (often alongside the
    Gregorian calendar) to date events in many Muslim countries. It is also used
    by Muslims to determine the proper days of Islamic holidays and rituals,
    such as the annual period of fasting and the proper time for the pilgrimage
    to Mecca.

    Source: \l {https://en.wikipedia.org/wiki/Islamic_calendar}{Wikipedia page
    on Hijri Calendar}

    \section1 Support for variants

    This base class provides the common details shared by all variants on the
    Islamic calendar. Each year comprises 12 months of 29 or 30 days each; most
    years have as many of 29 as of 30, but leap years extend one 29-day month to
    30 days. In tabular versions of the calendar (where mathematical rules are
    used to determine the details), odd-numbered months have 30 days, as does
    the last (twelfth) month of a leap year; all other months have 29
    days. Other versions are based on actual astronomical observations of the
    moon's phase at sunset, which vary from place to place.

    \sa QIslamicCivilCalendar, QCalendar
*/

bool QHijriCalendar::isLunar() const
{
    return true;
}

bool QHijriCalendar::isLuniSolar() const
{
    return false;
}

bool QHijriCalendar::isSolar() const
{
    return false;
}

int QHijriCalendar::daysInMonth(int month, int year) const
{
    if (year == 0 || month < 1 || month > 12)
        return 0;

    if (month == 12 && isLeapYear(year))
        return 30;

    return month % 2 == 0 ? 29 : 30;
}

int QHijriCalendar::maximumDaysInMonth() const
{
    return 30;
}

int QHijriCalendar::daysInYear(int year) const
{
    return monthsInYear(year) ? isLeapYear(year) ? 355 : 354 : 0;
}

const QCalendarLocale *QHijriCalendar::localeMonthIndexData() const
{
    return locale_data;
}

const ushort *QHijriCalendar::localeMonthData() const
{
    return months_data;
}

QT_END_NAMESPACE
