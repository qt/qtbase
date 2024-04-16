// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CALENDARBACKEND_H
#define CALENDARBACKEND_H

#include "private/qromancalendar_p.h"
#include "qdatetime.h"

#include <QtCore/private/qcalendarbackend_p.h>
//![0]
class JulianGregorianCalendar : public QRomanCalendar
{
public:
    JulianGregorianCalendar(QDate endJulian, QAnyStringView name);
    QString name() const override;
    int daysInMonth(int month, int year = QCalendar::Unspecified) const override;
    bool isLeapYear(int year) const override;
    bool dateToJulianDay(int year, int month, int day, qint64 *jd) const override;
    QCalendar::YearMonthDay julianDayToDate(qint64 jd) const override;
private:
    static inline const QCalendar julian = QCalendar(QCalendar::System::Julian);
    static inline const QCalendar gregorian = QCalendar(QCalendar::System::Gregorian);
    QCalendar::YearMonthDay m_julianUntil;
    QCalendar::YearMonthDay m_gregorianSince;
    QString m_name;
};
//![0]
#endif // CALENDARBACKEND_H
