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

#ifndef QGREGORIAN_CALENDAR_P_H
#define QGREGORIAN_CALENDAR_P_H

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

#include "qromancalendar_p.h"

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QGregorianCalendar : public QRomanCalendar
{
    // TODO: provide static methods, called by the overrides, that can be called
    // directly by QDate to optimize various parts.
public:
    QGregorianCalendar();
    // CAlendar properties:
    QString name() const override;
    QCalendar::System calendarSystem() const override;
    // Date queries:
    bool isLeapYear(int year) const override;
    // Julian Day conversions:
    bool dateToJulianDay(int year, int month, int day, qint64 *jd) const override;
    QCalendar::YearMonthDay julianDayToDate(qint64 jd) const override;

    // Names of months (implemented in qlocale.cpp):
    QString monthName(const QLocale &locale, int month, int year,
                      QLocale::FormatType format) const override;
    QString standaloneMonthName(const QLocale &locale, int month, int year,
                                QLocale::FormatType format) const override;

    // Static optimized versions for the benefit of QDate:
    static int weekDayOfJulian(qint64 jd);
    static bool leapTest(int year);
    static int monthLength(int month, int year);
    static bool validParts(int year, int month, int day);
    static QCalendar::YearMonthDay partsFromJulian(qint64 jd);
    static bool julianFromParts(int year, int month, int day, qint64 *jd);
};

QT_END_NAMESPACE

#endif // QGREGORIAN_CALENDAR_P_H
