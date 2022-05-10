// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEVENTDISPATCHER_WIN_P_H
#define QEVENTDISPATCHER_WIN_P_H

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

#include "QtCore/qabstracteventdispatcher.h"
#include "QtCore/qt_windows.h"
#include "QtCore/qhash.h"
#include "QtCore/qatomic.h"

#include "qabstracteventdispatcher_p.h"

QT_BEGIN_NAMESPACE

class QEventDispatcherWin32Private;

// forward declaration
LRESULT QT_WIN_CALLBACK qt_internal_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
quint64 qt_msectime();

class Q_CORE_EXPORT QEventDispatcherWin32 : public QAbstractEventDispatcher
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QEventDispatcherWin32)

public:
    explicit QEventDispatcherWin32(QObject *parent = nullptr);
    ~QEventDispatcherWin32();

    bool QT_ENSURE_STACK_ALIGNED_FOR_SSE processEvents(QEventLoop::ProcessEventsFlags flags) override;

    void registerSocketNotifier(QSocketNotifier *notifier) override;
    void unregisterSocketNotifier(QSocketNotifier *notifier) override;

    void registerTimer(int timerId, qint64 interval, Qt::TimerType timerType, QObject *object) override;
    bool unregisterTimer(int timerId) override;
    bool unregisterTimers(QObject *object) override;
    QList<TimerInfo> registeredTimers(QObject *object) const override;

    int remainingTime(int timerId) override;

    void wakeUp() override;
    void interrupt() override;

    void startingUp() override;
    void closingDown() override;

    bool event(QEvent *e) override;

    HWND internalHwnd();

protected:
    QEventDispatcherWin32(QEventDispatcherWin32Private &dd, QObject *parent = nullptr);
    virtual void sendPostedEvents();
    void doUnregisterSocketNotifier(QSocketNotifier *notifier);

private:
    friend LRESULT QT_WIN_CALLBACK qt_internal_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
};

struct QSockNot {
    QSocketNotifier *obj;
    int fd;
};
typedef QHash<int, QSockNot *> QSNDict;

struct QSockFd {
    long event;
    long mask;
    bool selected;

    explicit inline QSockFd(long ev = 0, long ma = 0) : event(ev), mask(ma), selected(false) { }
};
typedef QHash<int, QSockFd> QSFDict;

struct WinTimerInfo {                           // internal timer info
    QObject *dispatcher;
    int timerId;
    qint64 interval;
    Qt::TimerType timerType;
    quint64 timeout;                            // - when to actually fire
    QObject *obj;                               // - object to receive events
    bool inTimerEvent;
    UINT fastTimerId;
};

class QZeroTimerEvent : public QTimerEvent
{
public:
    explicit inline QZeroTimerEvent(int timerId)
        : QTimerEvent(timerId)
    { t = QEvent::ZeroTimerEvent; }
};

typedef QHash<int, WinTimerInfo*> WinTimerDict; // fast dict of timers

class Q_CORE_EXPORT QEventDispatcherWin32Private : public QAbstractEventDispatcherPrivate
{
    Q_DECLARE_PUBLIC(QEventDispatcherWin32)
public:
    QEventDispatcherWin32Private();
    ~QEventDispatcherWin32Private();

    QAtomicInt interrupt;

    // internal window handle used for socketnotifiers/timers/etc
    HWND internalHwnd;

    // for controlling when to send posted events
    UINT_PTR sendPostedEventsTimerId;
    QAtomicInt wakeUps;
    void startPostedEventsTimer();

    // timers
    WinTimerDict timerDict;
    void registerTimer(WinTimerInfo *t);
    void unregisterTimer(WinTimerInfo *t);
    void sendTimerEvent(int timerId);

    // socket notifiers
    QSNDict sn_read;
    QSNDict sn_write;
    QSNDict sn_except;
    QSFDict active_fd;
    bool activateNotifiersPosted;
    void postActivateSocketNotifiers();
    void doWsaAsyncSelect(int socket, long event);

    bool closingDown = false;

    QList<MSG> queuedUserInputEvents;
    QList<MSG> queuedSocketEvents;
};

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_WIN_P_H
