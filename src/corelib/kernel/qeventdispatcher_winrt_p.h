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


#ifndef QEVENTDISPATCHER_WINRT_P_H
#define QEVENTDISPATCHER_WINRT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include "QtCore/qabstracteventdispatcher.h"

#include <qt_windows.h>

#include <functional>

QT_BEGIN_NAMESPACE

quint64 qt_msectime();

class QEventDispatcherWinRTPrivate;

class Q_CORE_EXPORT QEventDispatcherWinRT : public QAbstractEventDispatcher
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QEventDispatcherWinRT)

public:
    explicit QEventDispatcherWinRT(QObject *parent = 0);
    ~QEventDispatcherWinRT();

    static HRESULT runOnXamlThread(const std::function<HRESULT()> &delegate, bool waitForRun = true);
    static HRESULT runOnMainThread(const std::function<HRESULT()> &delegate, int timeout = 100);

    bool processEvents(QEventLoop::ProcessEventsFlags flags);
    bool hasPendingEvents();

    void registerSocketNotifier(QSocketNotifier *notifier);
    void unregisterSocketNotifier(QSocketNotifier *notifier);

    void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object);
    bool unregisterTimer(int timerId);
    bool unregisterTimers(QObject *object);
    QList<TimerInfo> registeredTimers(QObject *object) const;

    int remainingTime(int timerId);

    bool registerEventNotifier(QWinEventNotifier *notifier);
    void unregisterEventNotifier(QWinEventNotifier *notifier);

    void wakeUp();
    void interrupt();
    void flush();

    void startingUp();
    void closingDown();

protected:
    QEventDispatcherWinRT(QEventDispatcherWinRTPrivate &dd, QObject *parent = 0);

    virtual bool sendPostedEvents(QEventLoop::ProcessEventsFlags flags);
    bool event(QEvent *);
    int activateTimers();
};

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_WINRT_P_H
