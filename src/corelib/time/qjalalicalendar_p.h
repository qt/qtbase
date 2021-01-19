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

#ifndef QJALALI_CALENDAR_P_H
#define QJALALI_CALENDAR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of calendar implementations.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qcalendarbackend_p.h"

QT_REQUIRE_CONFIG(jalalicalendar);

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QJalaliCalendar : public QCalendarBackend
{
public:
    QJalaliCalendar();
    // Calendar properties:
    QString name() const override;
    QCalendar::System calendarSystem() const override;
    // Date queries:
    int daysInMonth(int month, int year = QCalendar::Unspecified) const override;
    bool isLeapYear(int year) const override;
    // Properties of the calendar
    bool isLunar() const override;
    bool isLuniSolar() const override;
    bool isSolar() const override;
    // Julian Day conversions:
    bool dateToJulianDay(int year, int month, int day, qint64 *jd) const override;
    QCalendar::YearMonthDay julianDayToDate(qint64 jd) const override;

protected:
    // locale support:
    const QCalendarLocale *localeMonthIndexData() const override;
    const ushort *localeMonthData() const override;
};

QT_END_NAMESPACE

#endif // QJALALI_CALENDAR_P_H
