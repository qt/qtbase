/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWINRTEVENTDISPATCHER_H
#define QWINRTEVENTDISPATCHER_H

#include <QtCore/private/qeventdispatcher_winrt_p.h>

QT_BEGIN_NAMESPACE

class QWinRTEventDispatcher : public QEventDispatcherWinRT
{
    Q_OBJECT
public:
    explicit QWinRTEventDispatcher(QObject *parent = nullptr);

protected:
    bool hasPendingEvents() override;
    bool sendPostedEvents(QEventLoop::ProcessEventsFlags flags) override;
};

QT_END_NAMESPACE

#endif // QWINRTEVENTDISPATCHER_H
