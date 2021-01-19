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

#ifndef QHIJRI_CALENDAR_P_H
#define QHIJRI_CALENDAR_P_H

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

#include <QtCore/private/qglobal_p.h>
#include "qcalendarbackend_p.h"

QT_REQUIRE_CONFIG(hijricalendar);

QT_BEGIN_NAMESPACE

// Base for sharing with other variants on the Islamic calendar, as needed:
class Q_CORE_EXPORT QHijriCalendar : public QCalendarBackend
{
public:
    int daysInMonth(int month, int year = QCalendar::Unspecified) const override;
    int maximumDaysInMonth() const override;
    int daysInYear(int year) const override;

    bool isLunar() const override;
    bool isLuniSolar() const override;
    bool isSolar() const override;

protected:
    const QCalendarLocale *localeMonthIndexData() const override;
    const ushort *localeMonthData() const override;
    // (The INTEGRITY compiler got upset at: using QCalendarBackend:QCalendarBackend;)
    QHijriCalendar(const QString &name, QCalendar::System id = QCalendar::System::User)
        : QCalendarBackend(name, id) {}
};

QT_END_NAMESPACE

#endif // QHIJRI_CALENDAR_P_H
