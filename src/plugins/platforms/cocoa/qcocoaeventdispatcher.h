// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (c) 2007-2008, Apple, Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef QEVENTDISPATCHER_MAC_P_H
#define QEVENTDISPATCHER_MAC_P_H

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

#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/qstack.h>
#include <QtGui/qwindowdefs.h>
#include <QtCore/private/qabstracteventdispatcher_p.h>
#include <QtCore/private/qcfsocketnotifier_p.h>
#include <QtCore/private/qtimerinfo_unix_p.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qpointer.h>

#include <CoreFoundation/CoreFoundation.h>

Q_FORWARD_DECLARE_OBJC_CLASS(NSWindow);

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcEventDispatcher);

typedef struct _NSModalSession *NSModalSession;
typedef struct _QCocoaModalSessionInfo {
    QPointer<QWindow> window;
    NSModalSession session;
    NSWindow *nswindow;
} QCocoaModalSessionInfo;

class QCocoaEventDispatcherPrivate;
class QCocoaEventDispatcher : public QAbstractEventDispatcherV2
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QCocoaEventDispatcher)

public:
    QCocoaEventDispatcher(QAbstractEventDispatcherPrivate &priv, QObject *parent = nullptr);
    explicit QCocoaEventDispatcher(QObject *parent = nullptr);
    ~QCocoaEventDispatcher() override;

    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;

    void registerSocketNotifier(QSocketNotifier *notifier) override;
    void unregisterSocketNotifier(QSocketNotifier *notifier) override;

    void registerTimer(Qt::TimerId timerId, Duration interval, Qt::TimerType timerType,
                       QObject *object) final;
    bool unregisterTimer(Qt::TimerId timerId) final;
    bool unregisterTimers(QObject *object) final;
    QList<TimerInfoV2> timersForObject(QObject *object) const final;
    Duration remainingTime(Qt::TimerId timerId) const final;

    void wakeUp() override;
    void interrupt() override;

    static void clearCurrentThreadCocoaEventDispatcherInterruptFlag();

    friend void qt_mac_maybeCancelWaitForMoreEventsForwarder(QAbstractEventDispatcher *eventDispatcher);
};

class QCocoaEventDispatcherPrivate : public QAbstractEventDispatcherPrivate
{
    Q_DECLARE_PUBLIC(QCocoaEventDispatcher)

public:
    QCocoaEventDispatcherPrivate();
    ~QCocoaEventDispatcherPrivate() override;

    uint processEventsFlags;

    // timer handling
    QTimerInfoList timerInfoList;
    CFRunLoopTimerRef runLoopTimerRef;
    CFRunLoopSourceRef activateTimersSourceRef;
    void maybeStartCFRunLoopTimer();
    void maybeStopCFRunLoopTimer();
    static void runLoopTimerCallback(CFRunLoopTimerRef, void *info);
    static void activateTimersSourceCallback(void *info);
    bool processTimers();

    // Set 'blockSendPostedEvents' to true if you _really_ need
    // to make sure that qt events are not posted while calling
    // low-level cocoa functions (like beginModalForWindow). And
    // use a QBoolBlocker to be safe:
    bool blockSendPostedEvents;
    // The following variables help organizing modal sessions:
    QStack<QCocoaModalSessionInfo> cocoaModalSessionStack;
    bool currentExecIsNSAppRun;
    bool nsAppRunCalledByQt;
    bool initializingNSApplication = false;
    bool cleanupModalSessionsNeeded;
    uint processEventsCalled;
    NSModalSession currentModalSessionCached;
    NSModalSession currentModalSession();
    void temporarilyStopAllModalSessions();
    void beginModalSession(QWindow *widget);
    void endModalSession(QWindow *widget);
    bool hasModalSession() const;
    void cleanupModalSessions();

    void cancelWaitForMoreEvents();
    void maybeCancelWaitForMoreEvents();
    void ensureNSAppInitialized();

    QCFSocketNotifier cfSocketNotifier;
    QList<void *> queuedUserInputEvents; // NSEvent *
    CFRunLoopSourceRef postedEventsSource;
    CFRunLoopObserverRef waitingObserver;
    QAtomicInt serialNumber;
    int lastSerial;
    bool interrupt;
    bool propagateInterrupt = false;

    static void postedEventsSourceCallback(void *info);
    static void waitingObserverCallback(CFRunLoopObserverRef observer,
                                        CFRunLoopActivity activity, void *info);
    bool sendQueuedUserInputEvents();
    void processPostedEvents();
};

class QtCocoaInterruptDispatcher : public QObject
{
    static QtCocoaInterruptDispatcher *instance;
    bool cancelled;

    QtCocoaInterruptDispatcher();
    ~QtCocoaInterruptDispatcher();

    public:
    static void interruptLater();
    static void cancelInterruptLater();
};

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_MAC_P_H
