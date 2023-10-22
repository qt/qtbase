// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/****************************************************************************
**
** Copyright (c) 2007-2008, Apple, Inc.
**
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
**   * Redistributions of source code must retain the above copyright notice,
**     this list of conditions and the following disclaimer.
**
**   * Redistributions in binary form must reproduce the above copyright notice,
**     this list of conditions and the following disclaimer in the documentation
**     and/or other materials provided with the distribution.
**
**   * Neither the name of Apple, Inc. nor the names of its contributors
**     may be used to endorse or promote products derived from this software
**     without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/

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
class QCocoaEventDispatcher : public QAbstractEventDispatcher
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QCocoaEventDispatcher)

public:
    QCocoaEventDispatcher(QAbstractEventDispatcherPrivate &priv, QObject *parent = nullptr);
    explicit QCocoaEventDispatcher(QObject *parent = nullptr);
    ~QCocoaEventDispatcher();

    bool processEvents(QEventLoop::ProcessEventsFlags flags);

    void registerSocketNotifier(QSocketNotifier *notifier);
    void unregisterSocketNotifier(QSocketNotifier *notifier);

    void registerTimer(int timerId, qint64 interval, Qt::TimerType timerType, QObject *object);
    bool unregisterTimer(int timerId);
    bool unregisterTimers(QObject *object);
    QList<TimerInfo> registeredTimers(QObject *object) const;

    int remainingTime(int timerId);

    void wakeUp();
    void interrupt();

    static void clearCurrentThreadCocoaEventDispatcherInterruptFlag();

    friend void qt_mac_maybeCancelWaitForMoreEventsForwarder(QAbstractEventDispatcher *eventDispatcher);
};

class QCocoaEventDispatcherPrivate : public QAbstractEventDispatcherPrivate
{
    Q_DECLARE_PUBLIC(QCocoaEventDispatcher)

public:
    QCocoaEventDispatcherPrivate();

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
