// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    if (qMod<100>(year) == 0) {
        const qint16 century = qMod<9>(qDiv<100>(year));
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
    const qint16 x3 = qDiv<100>(x4);
    const qint16 x2 = qMod<100>(x4);
    *jd = qDiv<9>(328718 * x3 + 6)
        + qDiv<100>(36525 * x2)
        + qDiv<5>(153 * x1 + 2)
        + day + 1721119;
    return true;
}

QCalendar::YearMonthDay  QMilankovicCalendar::julianDayToDate(qint64 jd) const
{
    const qint64 k3 = 9 * (jd - 1721120) + 2;
    const qint64 x3 = qDiv<328718>(k3);
    const qint64 k2 = 100 * qDiv<9>(qMod<328718>(k3)) + 99;
    const qint64 k1 = qDiv<100>(qMod<36525>(k2)) * 5 + 2;
    const qint64 x2 = qDiv<36525>(k2);
    const qint64 x1 = qDiv<153>(5 * qDiv<100>(qMod<36525>(k2)) + 2);
    const qint64 c0 = qDiv<12>(x1 + 2);
    const int y = 100 * x3 + x2 + c0;
    const int month = x1 - 12 * c0 + 3;
    const int day = qDiv<5>(qMod<153>(k1)) + 1;
    return QCalendar::YearMonthDay(y > 0 ? y : y - 1, month, day);
}

QT_END_NAMESPACE
