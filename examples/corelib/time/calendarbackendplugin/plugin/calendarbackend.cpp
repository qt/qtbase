// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "calendarbackend.h"

#include <QCalendar>

JulianGregorianCalendar::JulianGregorianCalendar(QDate endJulian, QAnyStringView name = {})
    : m_julianUntil(julian.partsFromDate(endJulian)),
      m_gregorianSince(gregorian.partsFromDate(endJulian.addDays(1))),
      m_name(name.isEmpty()
    ? endJulian.toString(u"Julian until yyyy-MM-dd", julian)
    : name.toString())
{
    Q_ASSERT_X(m_julianUntil.year < m_gregorianSince.year
                || (m_julianUntil.year == m_gregorianSince.year
                    && (m_julianUntil.month < m_gregorianSince.month
                        || (m_julianUntil.month == m_gregorianSince.month
                           && m_julianUntil.day < m_gregorianSince.day))),
               "JulianGregorianCalendar::JulianGregorianCalendar()",
               "Perversely early date for Julian-to-Gregorian transition");
}

QString JulianGregorianCalendar::name() const
{
    return QStringLiteral("JulianGregorian");
}

int JulianGregorianCalendar::daysInMonth(int month, int year) const
{
    if (year == QCalendar::Unspecified)
        return QRomanCalendar::daysInMonth(month, year);
    if (year < m_julianUntil.year
            || (year == m_julianUntil.year && month < m_julianUntil.month)) {
        return julian.daysInMonth(month, year);
    }
    if ((year > m_gregorianSince.year)
        || (year == m_gregorianSince.year && month > m_gregorianSince.month)) {
        return gregorian.daysInMonth(month, year);
    }
    if (m_julianUntil.year == m_gregorianSince.year) {
        Q_ASSERT(year == m_julianUntil.year);
        if (m_julianUntil.month == m_gregorianSince.month) {
            Q_ASSERT(month == m_julianUntil.month);
            return QRomanCalendar::daysInMonth(month, year)
                    + m_julianUntil.day - m_gregorianSince.day + 1;
        }
    }
    if (year == m_julianUntil.year && month == m_julianUntil.month)
        return m_julianUntil.day;
    if (year == m_gregorianSince.year && month == m_gregorianSince.month)
        return gregorian.daysInMonth(month, year) + 1 - m_gregorianSince.day;
    Q_ASSERT(year > 3900);
    return 0;
}

bool JulianGregorianCalendar::isLeapYear(int year) const
{
    if (year < m_julianUntil.year
        || (year == m_julianUntil.year
            && (m_julianUntil.month > 2
                || (m_julianUntil.month == 2 && m_julianUntil.day == 29)))) {
        return julian.isLeapYear(year);
    }
    return gregorian.isLeapYear(year);
}
//![0]
bool JulianGregorianCalendar::dateToJulianDay(int year, int month, int day, qint64 *jd) const
{
    if (year == m_julianUntil.year && month == m_julianUntil.month) {
        if (m_julianUntil.day < day && day < m_gregorianSince.day) {
            // Requested date is in the gap skipped over by the transition.
            *jd = 0;
            return false;
        }
    }
    QDate givenDate = gregorian.dateFromParts(year, month, day);
    QDate julianUntil = julian.dateFromParts(m_julianUntil);
    if (givenDate > julianUntil) {
        *jd = givenDate.toJulianDay();
        return true;
    }
    *jd = julian.dateFromParts(year, month, day).toJulianDay();
    return true;
}
//![0]
//![1]
QCalendar::YearMonthDay JulianGregorianCalendar::julianDayToDate(qint64 jd) const
{
    const qint64 jdForChange = julian.dateFromParts(m_julianUntil).toJulianDay();
    if (jdForChange < jd) {
        QCalendar gregorian(QCalendar::System::Gregorian);
        QDate date = QDate::fromJulianDay(jd);
        return gregorian.partsFromDate(date);
    } else if (jd <= jdForChange) {
        QCalendar julian(QCalendar::System::Julian);
        QDate date = QDate::fromJulianDay(jd);
        return julian.partsFromDate(date);
    }
    return QCalendar::YearMonthDay(QCalendar::Unspecified, QCalendar::Unspecified,
                                   QCalendar::Unspecified);
}
//![1]
