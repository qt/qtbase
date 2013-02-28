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

#include "qioseventdispatcher.h"
#import "qiosapplicationdelegate.h"
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

void QIOSEventDispatcher::postedEventsRunLoopCallback(void *info)
{
    QIOSEventDispatcher *self = static_cast<QIOSEventDispatcher *>(info);
    self->processPostedEvents();
}

void QIOSEventDispatcher::nonBlockingTimerRunLoopCallback(CFRunLoopTimerRef, void *info)
{
    // The (one and only) CFRunLoopTimer has fired, which means that at least
    // one QTimer should now fire as well. Note that CFRunLoopTimer's callback will
    // never recurse. So if the app starts a new QEventLoop within this callback, other
    // timers will stop working. The work-around is to forward the callback to a
    // dedicated CFRunLoopSource that can recurse:
    QIOSEventDispatcher *self = static_cast<QIOSEventDispatcher *>(info);
    CFRunLoopSourceSignal(self->m_blockingTimerRunLoopSource);
}

void QIOSEventDispatcher::blockingTimerRunLoopCallback(void *info)
{
    // TODO:
    // We also need to block this new timer source
    // along with the posted event source when calling processEvents()
    // "manually" to prevent livelock deep in CFRunLoop.

    QIOSEventDispatcher *self = static_cast<QIOSEventDispatcher *>(info);
    self->m_timerInfoList.activateTimers();
    self->maybeStartCFRunLoopTimer();
}

void QIOSEventDispatcher::maybeStartCFRunLoopTimer()
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
        m_runLoopTimerRef = CFRunLoopTimerCreate(0, ttf, oneyear, 0, 0, QIOSEventDispatcher::nonBlockingTimerRunLoopCallback, &info);
        Q_ASSERT(m_runLoopTimerRef != 0);

        CFRunLoopAddTimer(CFRunLoopGetMain(), m_runLoopTimerRef, kCFRunLoopCommonModes);
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

void QIOSEventDispatcher::maybeStopCFRunLoopTimer()
{
    if (m_runLoopTimerRef == 0)
        return;

    CFRunLoopTimerInvalidate(m_runLoopTimerRef);
    CFRelease(m_runLoopTimerRef);
    m_runLoopTimerRef = 0;
}

void QIOSEventDispatcher::processPostedEvents()
{
    QWindowSystemInterface::sendWindowSystemEvents(QEventLoop::AllEvents);
}

QIOSEventDispatcher::QIOSEventDispatcher(QObject *parent)
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
    context.perform = QIOSEventDispatcher::blockingTimerRunLoopCallback;
    m_blockingTimerRunLoopSource = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &context);
    Q_ASSERT(m_blockingTimerRunLoopSource);
    CFRunLoopAddSource(mainRunLoop, m_blockingTimerRunLoopSource, kCFRunLoopCommonModes);

    // source used to handle posted events:
    context.perform = QIOSEventDispatcher::postedEventsRunLoopCallback;
    m_postedEventsRunLoopSource = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &context);
    Q_ASSERT(m_postedEventsRunLoopSource);
    CFRunLoopAddSource(mainRunLoop, m_postedEventsRunLoopSource, kCFRunLoopCommonModes);
}

QIOSEventDispatcher::~QIOSEventDispatcher()
{
    CFRunLoopRef mainRunLoop = CFRunLoopGetMain();
    CFRunLoopRemoveSource(mainRunLoop, m_postedEventsRunLoopSource, kCFRunLoopCommonModes);
    CFRelease(m_postedEventsRunLoopSource);

    qDeleteAll(m_timerInfoList);
    maybeStopCFRunLoopTimer();
    CFRunLoopRemoveSource(CFRunLoopGetMain(), m_blockingTimerRunLoopSource, kCFRunLoopCommonModes);
    CFRelease(m_blockingTimerRunLoopSource);

    m_cfSocketNotifier.removeSocketNotifiers();
}

bool QIOSEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    m_interrupted = false;
    bool eventsProcessed = false;

    bool excludeUserEvents = flags & QEventLoop::ExcludeUserInputEvents;
    bool execFlagSet = (flags & QEventLoop::DialogExec) || (flags & QEventLoop::EventLoopExec);
    bool useExecMode = execFlagSet && !excludeUserEvents;

    if (useExecMode) {
        NSRunLoop *runLoop = [NSRunLoop currentRunLoop];
        while ([runLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]] && !m_interrupted);
        eventsProcessed = true;
    } else {
        if (!(flags & QEventLoop::WaitForMoreEvents))
            wakeUp();
        eventsProcessed = [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
    }
    return eventsProcessed;
}

bool QIOSEventDispatcher::hasPendingEvents()
{
    qDebug() << __FUNCTION__ << "not implemented";
    return false;
}

void QIOSEventDispatcher::registerSocketNotifier(QSocketNotifier *notifier)
{
    m_cfSocketNotifier.registerSocketNotifier(notifier);
}

void QIOSEventDispatcher::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    m_cfSocketNotifier.unregisterSocketNotifier(notifier);
}

void QIOSEventDispatcher::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *obj)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1 || interval < 0 || !obj) {
        qWarning("QIOSEventDispatcher::registerTimer: invalid arguments");
        return;
    } else if (obj->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QIOSEventDispatcher: timers cannot be started from another thread");
        return;
    }
#endif

    m_timerInfoList.registerTimer(timerId, interval, timerType, obj);
    maybeStartCFRunLoopTimer();
}

bool QIOSEventDispatcher::unregisterTimer(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QIOSEventDispatcher::unregisterTimer: invalid argument");
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

bool QIOSEventDispatcher::unregisterTimers(QObject *object)
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QIOSEventDispatcher::unregisterTimers: invalid argument");
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

QList<QAbstractEventDispatcher::TimerInfo> QIOSEventDispatcher::registeredTimers(QObject *object) const
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QIOSEventDispatcher:registeredTimers: invalid argument");
        return QList<TimerInfo>();
    }
#endif

    return m_timerInfoList.registeredTimers(object);
}

int QIOSEventDispatcher::remainingTime(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QIOSEventDispatcher::remainingTime: invalid argument");
        return -1;
    }
#endif

    return m_timerInfoList.timerRemainingTime(timerId);
}

void QIOSEventDispatcher::wakeUp()
{
    CFRunLoopSourceSignal(m_postedEventsRunLoopSource);
    CFRunLoopWakeUp(CFRunLoopGetMain());
}

void QIOSEventDispatcher::interrupt()
{
    wakeUp();
    m_interrupted = true;
}

void QIOSEventDispatcher::flush()
{
    // X11 only.
}

QT_END_NAMESPACE

