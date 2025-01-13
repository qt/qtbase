// Copyright (c) 2007-2008, Apple, Inc.
// SPDX-License-Identifier: BSD-3-Clause

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
#include <QtCore/qloggingcategory.h>

#include <CoreFoundation/CoreFoundation.h>

Q_FORWARD_DECLARE_OBJC_CLASS(QT_MANGLE_NAMESPACE(RunLoopModeTracker));

QT_BEGIN_NAMESPACE

namespace QtPrivate {
QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcEventDispatcher, Q_CORE_EXPORT)
QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcEventDispatcherTimers, Q_CORE_EXPORT)
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

class Q_CORE_EXPORT QEventDispatcherCoreFoundation : public QAbstractEventDispatcherV2
{
    Q_OBJECT

public:
    explicit QEventDispatcherCoreFoundation(QObject *parent = nullptr);
    void startingUp() override;
    ~QEventDispatcherCoreFoundation();

    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;

    void registerSocketNotifier(QSocketNotifier *notifier) override;
    void unregisterSocketNotifier(QSocketNotifier *notifier) override;

    void registerTimer(Qt::TimerId timerId, Duration interval, Qt::TimerType timerType,
                       QObject *object) override final;
    bool unregisterTimer(Qt::TimerId timerId) override final;
    bool unregisterTimers(QObject *object) override final;
    QList<TimerInfoV2> timersForObject(QObject *object) const override final;
    Duration remainingTime(Qt::TimerId timerId) const override final;

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
