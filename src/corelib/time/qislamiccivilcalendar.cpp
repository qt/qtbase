/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qglobal.h"
#include "qislamiccivilcalendar_p.h"
#include "qcalendarmath_p.h"
#include <QtCore/qmath.h>

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

QIslamicCivilCalendar::QIslamicCivilCalendar()
    : QHijriCalendar(QStringLiteral("Islamic Civil"), QCalendar::System::IslamicCivil)
{
    registerAlias(QStringLiteral("islamic-civil")); // CLDR name
    registerAlias(QStringLiteral("islamicc")); // old CLDR name, still (2018) used by Mozilla
    // Until we have a oncrete implementation that knows all the needed ephemerides:
    registerAlias(QStringLiteral("Islamic"));
}

QString QIslamicCivilCalendar::name() const
{
    return QStringLiteral("Islamic Civil");
}

QCalendar::System QIslamicCivilCalendar::calendarSystem() const
{
    return QCalendar::System::IslamicCivil;
}

bool QIslamicCivilCalendar::isLeapYear(int year) const
{
    if (year == QCalendar::Unspecified)
        return false;
    if (year < 0)
        ++year;
    return qMod(year * 11 + 14, 30) < 11;
}

bool QIslamicCivilCalendar::dateToJulianDay(int year, int month, int day, qint64 *jd) const
{
    Q_ASSERT(jd);
    if (!isDateValid(year, month, day))
        return false;
    if (year <= 0)
        ++year;
    *jd = qDiv(10631 * year - 10617, 30)
            + qDiv(325 * month - 320, 11)
            + day + 1948439;
    return true;
}

QCalendar::YearMonthDay QIslamicCivilCalendar::julianDayToDate(qint64 jd) const
{
    const qint64 epoch = 1948440;
    const int32_t k2 = 30 * (jd - epoch) + 15;
    const int32_t k1 = 11 * qDiv(qMod(k2, 10631), 30) + 5;
    int y = qDiv(k2, 10631) + 1;
    const int month = qDiv(k1, 325) + 1;
    const int day = qDiv(qMod(k1, 325), 11) + 1;
    return QCalendar::YearMonthDay(y > 0 ? y : y - 1, month, day);
}

QT_END_NAMESPACE
