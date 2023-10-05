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

#ifndef QEVENTDISPATCHER_CF_P_H
#define QEVENTDISPATCHER_CF_P_H

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
#include <QtCore/private/qtimerinfo_unix_p.h>
#include <QtCore/private/qcfsocketnotifier_p.h>
#include <QtCore/private/qcore_mac_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

#include <CoreFoundation/CoreFoundation.h>

Q_FORWARD_DECLARE_OBJC_CLASS(QT_MANGLE_NAMESPACE(RunLoopModeTracker));

QT_BEGIN_NAMESPACE

namespace QtPrivate {
Q_DECLARE_EXPORTED_LOGGING_CATEGORY(lcEventDispatcher, Q_CORE_EXPORT)
Q_DECLARE_EXPORTED_LOGGING_CATEGORY(lcEventDispatcherTimers, Q_CORE_EXPORT)
}

class QEventDispatcherCoreFoundation;

template <class T = QEventDispatcherCoreFoundation>
class RunLoopSource
{
public:
    typedef bool (T::*CallbackFunction)();

    enum { kHighestPriority = 0 } RunLoopSourcePriority;

    RunLoopSource(T *delegate, CallbackFunction callback)
        : m_delegate(delegate), m_callback(callback)
    {
        CFRunLoopSourceContext context = {};
        context.info = this;
        context.perform = RunLoopSource::process;

        m_source = CFRunLoopSourceCreate(kCFAllocatorDefault, kHighestPriority, &context);
        Q_ASSERT(m_source);
    }

    ~RunLoopSource()
    {
        CFRunLoopSourceInvalidate(m_source);
        CFRelease(m_source);
    }

    void addToMode(CFStringRef mode, CFRunLoopRef runLoop = 0)
    {
        if (!runLoop)
            runLoop = CFRunLoopGetCurrent();

        CFRunLoopAddSource(runLoop, m_source, mode);
    }

    void signal() { CFRunLoopSourceSignal(m_source); }

private:
    static void process(void *info)
    {
        RunLoopSource *self = static_cast<RunLoopSource *>(info);
        ((self->m_delegate)->*(self->m_callback))();
    }

    T *m_delegate;
    CallbackFunction m_callback;
    CFRunLoopSourceRef m_source;
};

template <class T = QEventDispatcherCoreFoundation>
class RunLoopObserver
{
public:
    typedef void (T::*CallbackFunction) (CFRunLoopActivity activity);

    RunLoopObserver(T *delegate, CallbackFunction callback, CFOptionFlags activities)
        : m_delegate(delegate), m_callback(callback)
    {
        CFRunLoopObserverContext context = {};
        context.info = this;

        m_observer = CFRunLoopObserverCreate(kCFAllocatorDefault, activities, true, 0, process, &context);
        Q_ASSERT(m_observer);
    }

    ~RunLoopObserver()
    {
        CFRunLoopObserverInvalidate(m_observer);
        CFRelease(m_observer);
    }

    void addToMode(CFStringRef mode, CFRunLoopRef runLoop = 0)
    {
        if (!runLoop)
            runLoop = CFRunLoopGetCurrent();

        if (!CFRunLoopContainsObserver(runLoop, m_observer, mode))
            CFRunLoopAddObserver(runLoop, m_observer, mode);
    }

    void removeFromMode(CFStringRef mode, CFRunLoopRef runLoop = 0)
    {
        if (!runLoop)
            runLoop = CFRunLoopGetCurrent();

        if (CFRunLoopContainsObserver(runLoop, m_observer, mode))
            CFRunLoopRemoveObserver(runLoop, m_observer, mode);
    }

private:
    static void process(CFRunLoopObserverRef, CFRunLoopActivity activity, void *info)
    {
        RunLoopObserver *self = static_cast<RunLoopObserver *>(info);
        ((self->m_delegate)->*(self->m_callback))(activity);
    }

    T *m_delegate;
    CallbackFunction m_callback;
    CFRunLoopObserverRef m_observer;
};

class Q_CORE_EXPORT QEventDispatcherCoreFoundation : public QAbstractEventDispatcher
{
    Q_OBJECT

public:
    explicit QEventDispatcherCoreFoundation(QObject *parent = nullptr);
    void startingUp() override;
    ~QEventDispatcherCoreFoundation();

    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;

    void registerSocketNotifier(QSocketNotifier *notifier) override;
    void unregisterSocketNotifier(QSocketNotifier *notifier) override;

    void registerTimer(int timerId, qint64 interval, Qt::TimerType timerType, QObject *object) override;
    bool unregisterTimer(int timerId) override;
    bool unregisterTimers(QObject *object) override;
    QList<QAbstractEventDispatcher::TimerInfo> registeredTimers(QObject *object) const override;

    int remainingTime(int timerId) override;

    void wakeUp() override;
    void interrupt() override;

protected:
    QEventLoop *currentEventLoop() const;

    virtual bool processPostedEvents();

    struct ProcessEventsState
    {
        ProcessEventsState(QEventLoop::ProcessEventsFlags f)
         : flags(f.toInt()), wasInterrupted(false)
         , processedPostedEvents(false), processedTimers(false)
         , deferredWakeUp(false), deferredUpdateTimers(false) {}

        ProcessEventsState(const ProcessEventsState &other)
            : flags(other.flags.loadAcquire())
            , wasInterrupted(other.wasInterrupted.loadAcquire())
            , processedPostedEvents(other.processedPostedEvents.loadAcquire())
            , processedTimers(other.processedTimers.loadAcquire())
            , deferredWakeUp(other.deferredWakeUp.loadAcquire())
            , deferredUpdateTimers(other.deferredUpdateTimers) {}

        ProcessEventsState &operator=(const ProcessEventsState &other)
        {
            flags.storeRelease(other.flags.loadAcquire());
            wasInterrupted.storeRelease(other.wasInterrupted.loadAcquire());
            processedPostedEvents.storeRelease(other.processedPostedEvents.loadAcquire());
            processedTimers.storeRelease(other.processedTimers.loadAcquire());
            deferredWakeUp.storeRelease(other.deferredWakeUp.loadAcquire());
            deferredUpdateTimers = other.deferredUpdateTimers;
            return *this;
        }

        QAtomicInt flags;
        QAtomicInteger<char> wasInterrupted;
        QAtomicInteger<char> processedPostedEvents;
        QAtomicInteger<char> processedTimers;
        QAtomicInteger<char> deferredWakeUp;
        bool deferredUpdateTimers;
    };

    ProcessEventsState m_processEvents;

private:
    RunLoopSource<> m_postedEventsRunLoopSource;
    RunLoopObserver<> m_runLoopActivityObserver;

    QT_MANGLE_NAMESPACE(RunLoopModeTracker) *m_runLoopModeTracker;

    QTimerInfoList m_timerInfoList;
    CFRunLoopTimerRef m_runLoopTimer;
    CFRunLoopTimerRef m_blockedRunLoopTimer;
    QCFType<CFRunLoopRef> m_runLoop;
    bool m_overdueTimerScheduled;

    QCFSocketNotifier m_cfSocketNotifier;

    void processTimers(CFRunLoopTimerRef);

    void handleRunLoopActivity(CFRunLoopActivity activity);

    void updateTimers();
    void invalidateTimer();
};

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_CF_P_H
