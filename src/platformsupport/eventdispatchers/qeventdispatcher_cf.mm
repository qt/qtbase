/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeventdispatcher_cf_p.h"
#include <qdebug.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/QThread>
#include <QtCore/private/qcoreapplication_p.h>

#include <UIKit/UIApplication.h>

QT_BEGIN_NAMESPACE
QT_USE_NAMESPACE

static Boolean runLoopSourceEqualCallback(const void *info1, const void *info2)
{
    return info1 == info2;
}

void QEventDispatcherCoreFoundation::postedEventsRunLoopCallback(void *info)
{
    QEventDispatcherCoreFoundation *self = static_cast<QEventDispatcherCoreFoundation *>(info);
    self->processPostedEvents();
}

void QEventDispatcherCoreFoundation::nonBlockingTimerRunLoopCallback(CFRunLoopTimerRef, void *info)
{
    // The (one and only) CFRunLoopTimer has fired, which means that at least
    // one QTimer should now fire as well. Note that CFRunLoopTimer's callback will
    // never recurse. So if the app starts a new QEventLoop within this callback, other
    // timers will stop working. The work-around is to forward the callback to a
    // dedicated CFRunLoopSource that can recurse:
    QEventDispatcherCoreFoundation *self = static_cast<QEventDispatcherCoreFoundation *>(info);
    CFRunLoopSourceSignal(self->m_blockingTimerRunLoopSource);
}

void QEventDispatcherCoreFoundation::blockingTimerRunLoopCallback(void *info)
{
    // TODO:
    // We also need to block this new timer source
    // along with the posted event source when calling processEvents()
    // "manually" to prevent livelock deep in CFRunLoop.

    QEventDispatcherCoreFoundation *self = static_cast<QEventDispatcherCoreFoundation *>(info);
    self->m_timerInfoList.activateTimers();
    self->maybeStartCFRunLoopTimer();
}

void QEventDispatcherCoreFoundation::maybeStartCFRunLoopTimer()
{
    // Find out when the next registered timer should fire, and schedule
    // runLoopTimer accordingly. If the runLoopTimer does not yet exist, and
    // at least one timer is registered, start by creating the timer:
    if (m_timerInfoList.isEmpty()) {
        Q_ASSERT(m_runLoopTimerRef == 0);
        return;
    }

    CFAbsoluteTime ttf = CFAbsoluteTimeGetCurrent();
    CFTimeInterval interval;

    if (m_runLoopTimerRef == 0) {
        // start the CFRunLoopTimer
        CFTimeInterval oneyear = CFTimeInterval(3600. * 24. * 365.);

        // calculate when the next timer should fire:
        struct timespec tv;
        if (m_timerInfoList.timerWait(tv)) {
            interval = qMax(tv.tv_sec + tv.tv_nsec / 1000000000., 0.0000001);
        } else {
            // this shouldn't really happen, but in case it does, set the timer
            // to fire a some point in the distant future:
            interval = oneyear;
        }

        ttf += interval;
        CFRunLoopTimerContext info = { 0, this, 0, 0, 0 };
        // create the timer with a large interval, as recommended by the CFRunLoopTimerSetNextFireDate()
        // documentation, since we will adjust the timer's time-to-fire as needed to keep Qt timers working
        m_runLoopTimerRef = CFRunLoopTimerCreate(0, ttf, oneyear, 0, 0, QEventDispatcherCoreFoundation::nonBlockingTimerRunLoopCallback, &info);
        Q_ASSERT(m_runLoopTimerRef != 0);

        CFRunLoopRef mainRunLoop = CFRunLoopGetMain();
        CFRunLoopAddTimer(mainRunLoop, m_runLoopTimerRef, kCFRunLoopCommonModes);
        CFRunLoopAddTimer(mainRunLoop, m_runLoopTimerRef, (CFStringRef) UITrackingRunLoopMode);
    } else {
        struct timespec tv;
        // Calculate when the next timer should fire:
        if (m_timerInfoList.timerWait(tv)) {
            interval = qMax(tv.tv_sec + tv.tv_nsec / 1000000000., 0.0000001);
        } else {
            // no timers can fire, but we cannot stop the CFRunLoopTimer, set the timer to fire at some
            // point in the distant future (the timer interval is one year)
            interval = CFRunLoopTimerGetInterval(m_runLoopTimerRef);
        }

        ttf += interval;
        CFRunLoopTimerSetNextFireDate(m_runLoopTimerRef, ttf);
    }
}

void QEventDispatcherCoreFoundation::maybeStopCFRunLoopTimer()
{
    if (m_runLoopTimerRef == 0)
        return;

    CFRunLoopTimerInvalidate(m_runLoopTimerRef);
    CFRelease(m_runLoopTimerRef);
    m_runLoopTimerRef = 0;
}

void QEventDispatcherCoreFoundation::processPostedEvents()
{
    QWindowSystemInterface::sendWindowSystemEvents(QEventLoop::AllEvents);
}

QEventDispatcherCoreFoundation::QEventDispatcherCoreFoundation(QObject *parent)
    : QAbstractEventDispatcher(parent)
    , m_interrupted(false)
    , m_runLoopTimerRef(0)
{
    m_cfSocketNotifier.setHostEventDispatcher(this);

    CFRunLoopRef mainRunLoop = CFRunLoopGetMain();
    CFRunLoopSourceContext context;
    bzero(&context, sizeof(CFRunLoopSourceContext));
    context.equal = runLoopSourceEqualCallback;
    context.info = this;

    // source used to handle timers:
    context.perform = QEventDispatcherCoreFoundation::blockingTimerRunLoopCallback;
    m_blockingTimerRunLoopSource = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &context);
    Q_ASSERT(m_blockingTimerRunLoopSource);
    CFRunLoopAddSource(mainRunLoop, m_blockingTimerRunLoopSource, kCFRunLoopCommonModes);
    CFRunLoopAddSource(mainRunLoop, m_blockingTimerRunLoopSource, (CFStringRef) UITrackingRunLoopMode);

    // source used to handle posted events:
    context.perform = QEventDispatcherCoreFoundation::postedEventsRunLoopCallback;
    m_postedEventsRunLoopSource = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &context);
    Q_ASSERT(m_postedEventsRunLoopSource);
    CFRunLoopAddSource(mainRunLoop, m_postedEventsRunLoopSource, kCFRunLoopCommonModes);
}

QEventDispatcherCoreFoundation::~QEventDispatcherCoreFoundation()
{
    CFRunLoopRef mainRunLoop = CFRunLoopGetMain();
    CFRunLoopRemoveSource(mainRunLoop, m_postedEventsRunLoopSource, kCFRunLoopCommonModes);
    CFRelease(m_postedEventsRunLoopSource);

    qDeleteAll(m_timerInfoList);
    maybeStopCFRunLoopTimer();

    CFRunLoopRemoveSource(mainRunLoop, m_blockingTimerRunLoopSource, kCFRunLoopCommonModes);
    CFRunLoopRemoveSource(mainRunLoop, m_blockingTimerRunLoopSource, (CFStringRef) UITrackingRunLoopMode);
    CFRelease(m_blockingTimerRunLoopSource);

    m_cfSocketNotifier.removeSocketNotifiers();
}

bool QEventDispatcherCoreFoundation::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    m_interrupted = false;
    bool eventsProcessed = false;

    bool excludeUserEvents = flags & QEventLoop::ExcludeUserInputEvents;
    bool execFlagSet = (flags & QEventLoop::DialogExec) || (flags & QEventLoop::EventLoopExec);
    bool useExecMode = execFlagSet && !excludeUserEvents;

    CFTimeInterval distantFuture = CFTimeInterval(3600. * 24. * 365. * 10.);
    SInt32 result;

    if (useExecMode) {
        while (!m_interrupted) {
            // Run a single pass on the runloop to unblock it
            result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true);

            // Run the default runloop until interrupted (by Qt or UIKit)
            if (result != kCFRunLoopRunFinished)
                result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, distantFuture, false);

            // App has quit or Qt has interrupted?
            if (result == kCFRunLoopRunFinished || m_interrupted)
                break;

            // Runloop was interrupted by UIKit?
            if (result == kCFRunLoopRunStopped && !m_interrupted) {
                // Run runloop in UI tracking mode
                if (CFRunLoopRunInMode((CFStringRef) UITrackingRunLoopMode,
                                        distantFuture, false) == kCFRunLoopRunFinished)
                    break;
            }
        }
        eventsProcessed = true;
    } else {
        if (!(flags & QEventLoop::WaitForMoreEvents))
            wakeUp();

        // Run runloop in default mode
        result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, distantFuture, true);
        if (result != kCFRunLoopRunFinished) {
            // Run runloop in UI tracking mode
            CFRunLoopRunInMode((CFStringRef) UITrackingRunLoopMode, distantFuture, false);
        }
        eventsProcessed = (result == kCFRunLoopRunHandledSource);
    }
    return eventsProcessed;
}

bool QEventDispatcherCoreFoundation::hasPendingEvents()
{
    qDebug() << __FUNCTION__ << "not implemented";
    return false;
}

void QEventDispatcherCoreFoundation::registerSocketNotifier(QSocketNotifier *notifier)
{
    m_cfSocketNotifier.registerSocketNotifier(notifier);
}

void QEventDispatcherCoreFoundation::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    m_cfSocketNotifier.unregisterSocketNotifier(notifier);
}

void QEventDispatcherCoreFoundation::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *obj)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1 || interval < 0 || !obj) {
        qWarning("QEventDispatcherCoreFoundation::registerTimer: invalid arguments");
        return;
    } else if (obj->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherCoreFoundation: timers cannot be started from another thread");
        return;
    }
#endif

    m_timerInfoList.registerTimer(timerId, interval, timerType, obj);
    maybeStartCFRunLoopTimer();
}

bool QEventDispatcherCoreFoundation::unregisterTimer(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QEventDispatcherCoreFoundation::unregisterTimer: invalid argument");
        return false;
    } else if (thread() != QThread::currentThread()) {
        qWarning("QObject::killTimer: timers cannot be stopped from another thread");
        return false;
    }
#endif

    bool returnValue = m_timerInfoList.unregisterTimer(timerId);
    m_timerInfoList.isEmpty() ? maybeStopCFRunLoopTimer() : maybeStartCFRunLoopTimer();
    return returnValue;
}

bool QEventDispatcherCoreFoundation::unregisterTimers(QObject *object)
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherCoreFoundation::unregisterTimers: invalid argument");
        return false;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QObject::killTimers: timers cannot be stopped from another thread");
        return false;
    }
#endif

    bool returnValue = m_timerInfoList.unregisterTimers(object);
    m_timerInfoList.isEmpty() ? maybeStopCFRunLoopTimer() : maybeStartCFRunLoopTimer();
    return returnValue;
}

QList<QAbstractEventDispatcher::TimerInfo> QEventDispatcherCoreFoundation::registeredTimers(QObject *object) const
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherCoreFoundation:registeredTimers: invalid argument");
        return QList<TimerInfo>();
    }
#endif

    return m_timerInfoList.registeredTimers(object);
}

int QEventDispatcherCoreFoundation::remainingTime(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QEventDispatcherCoreFoundation::remainingTime: invalid argument");
        return -1;
    }
#endif

    return m_timerInfoList.timerRemainingTime(timerId);
}

void QEventDispatcherCoreFoundation::wakeUp()
{
    CFRunLoopSourceSignal(m_postedEventsRunLoopSource);
    CFRunLoopWakeUp(CFRunLoopGetMain());
}

void QEventDispatcherCoreFoundation::interrupt()
{
    // Stop the runloop, which will cause processEvents() to exit
    m_interrupted = true;
    CFRunLoopStop(CFRunLoopGetMain());
}

void QEventDispatcherCoreFoundation::flush()
{
    // X11 only.
}

QT_END_NAMESPACE

