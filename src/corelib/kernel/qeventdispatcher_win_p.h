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

#include "qabstracteventdispatcher_p.h"

QT_BEGIN_NAMESPACE

class QWinEventNotifier;
class QEventDispatcherWin32Private;

// forward declaration
LRESULT QT_WIN_CALLBACK qt_internal_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
quint64 qt_msectime();

class Q_CORE_EXPORT QEventDispatcherWin32 : public QAbstractEventDispatcher
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QEventDispatcherWin32)

protected:
    void createInternalHwnd();

public:
    explicit QEventDispatcherWin32(QObject *parent = 0);
    ~QEventDispatcherWin32();

    bool QT_ENSURE_STACK_ALIGNED_FOR_SSE processEvents(QEventLoop::ProcessEventsFlags flags) override;
    bool hasPendingEvents() override;

    void registerSocketNotifier(QSocketNotifier *notifier) override;
    void unregisterSocketNotifier(QSocketNotifier *notifier) override;

    void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object) override;
    bool unregisterTimer(int timerId) override;
    bool unregisterTimers(QObject *object) override;
    QList<TimerInfo> registeredTimers(QObject *object) const override;

    bool registerEventNotifier(QWinEventNotifier *notifier) override;
    void unregisterEventNotifier(QWinEventNotifier *notifier) override;
    void activateEventNotifiers();

    int remainingTime(int timerId) override;

    void wakeUp() override;
    void interrupt() override;
    void flush() override;

    void startingUp() override;
    void closingDown() override;

    bool event(QEvent *e) override;

    HWND internalHwnd();

protected:
    QEventDispatcherWin32(QEventDispatcherWin32Private &dd, QObject *parent = 0);
    virtual void sendPostedEvents();
    void doUnregisterSocketNotifier(QSocketNotifier *notifier);
    void doUnregisterEventNotifier(QWinEventNotifier *notifier);

private:
    friend LRESULT QT_WIN_CALLBACK qt_internal_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
    friend LRESULT QT_WIN_CALLBACK qt_GetMessageHook(int, WPARAM, LPARAM);
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
    int interval;
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

typedef QList<WinTimerInfo*>  WinTimerVec;      // vector of TimerInfo structs
typedef QHash<int, WinTimerInfo*> WinTimerDict; // fast dict of timers

class Q_CORE_EXPORT QEventDispatcherWin32Private : public QAbstractEventDispatcherPrivate
{
    Q_DECLARE_PUBLIC(QEventDispatcherWin32)
public:
    QEventDispatcherWin32Private();
    ~QEventDispatcherWin32Private();
    static QEventDispatcherWin32Private *get(QEventDispatcherWin32 *q) { return q->d_func(); }

    DWORD threadId;

    QAtomicInt interrupt;

    // internal window handle used for socketnotifiers/timers/etc
    HWND internalHwnd;
    HHOOK getMessageHook;

    // for controlling when to send posted events
    UINT_PTR sendPostedEventsTimerId;
    QAtomicInt wakeUps;

    // timers
    WinTimerVec timerVec;
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

    bool winEventNotifierListModified = false;
    HANDLE winEventNotifierActivatedEvent;
    QList<QWinEventNotifier *> winEventNotifierList;
    void activateEventNotifier(QWinEventNotifier * wen);

    QList<MSG> queuedUserInputEvents;
    QList<MSG> queuedSocketEvents;
};

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_WIN_P_H
