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
    const auto yeardm = qDivMod<100>(year);
    if (yeardm.remainder == 0) {
        const qint16 century = qMod<9>(yeardm.quotient);
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
    const auto x4dm = qDivMod<100>(year + c0);
    *jd = qDiv<9>(328718 * x4dm.quotient + 6)
        + qDiv<100>(36525 * x4dm.remainder)
        + qDiv<5>(153 * x1 + 2)
        + day + 1721119;
    return true;
}

QCalendar::YearMonthDay  QMilankovicCalendar::julianDayToDate(qint64 jd) const
{
    const auto k3dm = qDivMod<328718>(9 * (jd - 1721120) + 2);
    const auto k2dm = qDivMod<36525>(100 * qDiv<9>(k3dm.remainder) + 99);
    const auto k1dm = qDivMod<153>(qDiv<100>(k2dm.remainder) * 5 + 2);
    const auto c0dm = qDivMod<12>(k1dm.quotient + 2);
    const int y = 100 * k3dm.quotient + k2dm.quotient + c0dm.quotient;
    const int month = c0dm.remainder + 1;
    const int day = qDiv<5>(k1dm.remainder) + 1;
    return QCalendar::YearMonthDay(y > 0 ? y : y - 1, month, day);
}

QT_END_NAMESPACE
