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
#include "qmilankoviccalendar_p.h"
#include "qcalendarmath_p.h"
#include <QtCore/qmath.h>
#include <QtCore/qlocale.h>
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

QMilankovicCalendar::QMilankovicCalendar()
    : QRomanCalendar(QStringLiteral("Milankovic"), QCalendar::System::Milankovic) {}

QString QMilankovicCalendar::name() const
{
    return QStringLiteral("Milankovic");
}

QCalendar::System QMilankovicCalendar::calendarSystem() const
{
    return QCalendar::System::Milankovic;
}

bool QMilankovicCalendar::isLeapYear(int year) const
{
    if (year == QCalendar::Unspecified)
        return false;
    if (year <= 0)
        ++year;
    if (qMod(year, 4))
        return false;
    if (qMod(year, 100) == 0) {
        const qint16 century = qMod(qDiv(year, 100), 9);
        if (century != 2 && century != 6)
            return false;
    }
    return true;
}

bool QMilankovicCalendar::dateToJulianDay(int year, int month, int day, qint64 *jd) const
{
    Q_ASSERT(jd);
    if (!isDateValid(year, month, day))
        return false;
    if (year <= 0)
        ++year;
    const qint16 c0 = month < 3 ? -1 : 0;
    const qint16 x1 = month - 12 * c0 - 3;
    const qint16 x4 = year + c0;
    const qint16 x3 = qDiv(x4, 100);
    const qint16 x2 = qMod(x4, 100);
    *jd = qDiv(328718 * x3 + 6, 9)
        + qDiv(36525 * x2 , 100)
        + qDiv(153 * x1 + 2 , 5)
        + day + 1721119;
    return true;
}

QCalendar::YearMonthDay  QMilankovicCalendar::julianDayToDate(qint64 jd) const
{
    const qint64 k3 = 9 * (jd - 1721120) + 2;
    const qint64 x3 = qDiv(k3, 328718);
    const qint64 k2 = 100 * qDiv(qMod(k3, 328718), 9) + 99;
    const qint64 k1 = qDiv(qMod(k2, 36525), 100) * 5 + 2;
    const qint64 x2 = qDiv(k2, 36525);
    const qint64 x1 = qDiv(5 * qDiv(qMod(k2, 36525), 100) + 2, 153);
    const qint64 c0 = qDiv(x1 + 2, 12);
    const int y = 100 * x3 + x2 + c0;
    const int month = x1 - 12 * c0 + 3;
    const int day = qDiv(qMod(k1, 153), 5) + 1;
    return QCalendar::YearMonthDay(y > 0 ? y : y - 1, month, day);
}

QT_END_NAMESPACE
