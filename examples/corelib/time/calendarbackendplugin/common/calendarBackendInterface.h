// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CALENDARINTERFACE_H
#define CALENDARINTERFACE_H

#include <QCalendar>
#include <QObject>

//![0]
class RequestedCalendarInterface
{
public:
    RequestedCalendarInterface() = default;
    virtual QCalendar::SystemId loadCalendar(QAnyStringView requested) = 0;
    virtual ~RequestedCalendarInterface() = default;
};
//![0]
QT_BEGIN_NAMESPACE
//![1]
#define RequestedCalendarInterface_iid \
"org.qt-project.Qt.Examples.CalendarBackend.RequestedCalendarInterface/1.0"
Q_DECLARE_INTERFACE(RequestedCalendarInterface, RequestedCalendarInterface_iid)
//![1]
QT_END_NAMESPACE

#endif // CALENDARINTERFACE_H
