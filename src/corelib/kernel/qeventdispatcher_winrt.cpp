/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qeventdispatcher_winrt_p.h"

#include "qelapsedtimer.h"
#include "qcoreapplication.h"
#include "qthread.h"

#include <private/qcoreapplication_p.h>
#include <private/qthread_p.h>

#include <windows.foundation.h>
#include <windows.system.threading.h>
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::System::Threading;
using namespace ABI::Windows::Foundation;

QT_BEGIN_NAMESPACE

QEventDispatcherWinRT::QEventDispatcherWinRT(QObject *parent)
    : QAbstractEventDispatcher(*new QEventDispatcherWinRTPrivate, parent)
{
}

QEventDispatcherWinRT::QEventDispatcherWinRT(QEventDispatcherWinRTPrivate &dd, QObject *parent)
    : QAbstractEventDispatcher(dd, parent)
{ }

QEventDispatcherWinRT::~QEventDispatcherWinRT()
{
}

bool QEventDispatcherWinRT::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_UNUSED(flags);

    // we are awake, broadcast it
    emit awake();
    QCoreApplicationPrivate::sendPostedEvents(0, 0, QThreadData::current());

    return false;
}

bool QEventDispatcherWinRT::hasPendingEvents()
{
    return qGlobalPostedEventsCount();
}

void QEventDispatcherWinRT::registerSocketNotifier(QSocketNotifier *notifier)
{
    Q_UNUSED(notifier);
    Q_UNIMPLEMENTED();
}
void QEventDispatcherWinRT::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    Q_UNUSED(notifier);
    Q_UNIMPLEMENTED();
}

void QEventDispatcherWinRT::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object)
{
    Q_UNUSED(timerType);

    if (timerId < 1 || interval < 0 || !object) {
        qWarning("QEventDispatcherWinRT::registerTimer: invalid arguments");
        return;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QObject::startTimer: timers cannot be started from another thread");
        return;
    }

    Q_D(QEventDispatcherWinRT);

    WinRTTimerInfo *t = new WinRTTimerInfo();
    t->dispatcher = this;
    t->timerId  = timerId;
    t->interval = interval;
    t->timeout = interval;
    t->timerType = timerType;
    t->obj = object;
    t->inTimerEvent = false;

    d->registerTimer(t);
    d->timerVec.append(t);                      // store in timer vector
    d->timerDict.insert(t->timerId, t);          // store timers in dict
}

bool QEventDispatcherWinRT::unregisterTimer(int timerId)
{
    if (timerId < 1) {
        qWarning("QEventDispatcherWinRT::unregisterTimer: invalid argument");
        return false;
    }
    if (thread() != QThread::currentThread()) {
        qWarning("QObject::killTimer: timers cannot be stopped from another thread");
        return false;
    }

    Q_D(QEventDispatcherWinRT);
    if (d->timerVec.isEmpty() || timerId <= 0)
        return false;

    WinRTTimerInfo *t = d->timerDict.value(timerId);
    if (!t)
        return false;

    if (t->timer)
        d->threadPoolTimerDict.remove(t->timer);
    d->timerDict.remove(t->timerId);
    d->timerVec.removeAll(t);
    d->unregisterTimer(t);
    return true;
}

bool QEventDispatcherWinRT::unregisterTimers(QObject *object)
{
    if (!object) {
        qWarning("QEventDispatcherWinRT::unregisterTimers: invalid argument");
        return false;
    }
    QThread *currentThread = QThread::currentThread();
    if (object->thread() != thread() || thread() != currentThread) {
        qWarning("QObject::killTimers: timers cannot be stopped from another thread");
        return false;
    }

    Q_D(QEventDispatcherWinRT);
    if (d->timerVec.isEmpty())
        return false;
    register WinRTTimerInfo *t;
    for (int i = 0; i < d->timerVec.size(); i++) {
        t = d->timerVec.at(i);
        if (t && t->obj == object) {                // object found
            if (t->timer)
                d->threadPoolTimerDict.remove(t->timer);
            d->timerDict.remove(t->timerId);
            d->timerVec.removeAt(i);
            d->unregisterTimer(t);
            --i;
        }
    }
    return true;
}

QList<QAbstractEventDispatcher::TimerInfo> QEventDispatcherWinRT::registeredTimers(QObject *object) const
{
    if (!object) {
        qWarning("QEventDispatcherWinRT:registeredTimers: invalid argument");
        return QList<TimerInfo>();
    }

    Q_D(const QEventDispatcherWinRT);
    QList<TimerInfo> list;
    for (int i = 0; i < d->timerVec.size(); ++i) {
        const WinRTTimerInfo *t = d->timerVec.at(i);
        if (t && t->obj == object)
            list << TimerInfo(t->timerId, t->interval, t->timerType);
    }
    return list;
}

bool QEventDispatcherWinRT::registerEventNotifier(QWinEventNotifier *notifier)
{
    Q_UNUSED(notifier);
    Q_UNIMPLEMENTED();
    return false;
}

void QEventDispatcherWinRT::unregisterEventNotifier(QWinEventNotifier *notifier)
{
    Q_UNUSED(notifier);
    Q_UNIMPLEMENTED();
}

int QEventDispatcherWinRT::remainingTime(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QEventDispatcherWinRT::remainingTime: invalid argument");
        return -1;
    }
#endif

    Q_D(QEventDispatcherWinRT);

    if (d->timerVec.isEmpty())
        return -1;

    quint64 currentTime = qt_msectime();

    register WinRTTimerInfo *t;
    for (int i = 0; i < d->timerVec.size(); i++) {
        t = d->timerVec.at(i);
        if (t && t->timerId == timerId) {                // timer found
            if (currentTime < t->timeout) {
                // time to wait
                return t->timeout - currentTime;
            } else {
                return 0;
            }
        }
    }

#ifndef QT_NO_DEBUG
    qWarning("QEventDispatcherWinRT::remainingTime: timer id %d not found", timerId);
#endif

    return -1;
}

void QEventDispatcherWinRT::wakeUp()
{
    Q_D(QEventDispatcherWinRT);
    if (d->wakeUps.testAndSetAcquire(0, 1)) {
        // ###TODO: is there any thing to wake up?
    }
}

void QEventDispatcherWinRT::interrupt()
{
    Q_D(QEventDispatcherWinRT);
    d->interrupt = true;
    wakeUp();
}

void QEventDispatcherWinRT::flush()
{
}

void QEventDispatcherWinRT::startingUp()
{
}

void QEventDispatcherWinRT::closingDown()
{
    Q_D(QEventDispatcherWinRT);

    // clean up any timers
    for (int i = 0; i < d->timerVec.count(); ++i)
        d->unregisterTimer(d->timerVec.at(i));
    d->timerVec.clear();
    d->timerDict.clear();
    d->threadPoolTimerDict.clear();
}

bool QEventDispatcherWinRT::event(QEvent *e)
{
    Q_D(QEventDispatcherWinRT);
    if (e->type() == QEvent::ZeroTimerEvent) {
        QZeroTimerEvent *zte = static_cast<QZeroTimerEvent*>(e);
        WinRTTimerInfo *t = d->timerDict.value(zte->timerId());
        if (t) {
            t->inTimerEvent = true;

            QTimerEvent te(zte->timerId());
            QCoreApplication::sendEvent(t->obj, &te);

            t = d->timerDict.value(zte->timerId());
            if (t) {
                if (t->interval == 0 && t->inTimerEvent) {
                    // post the next zero timer event as long as the timer was not restarted
                    QCoreApplication::postEvent(this, new QZeroTimerEvent(zte->timerId()));
                }

                t->inTimerEvent = false;
            }
        }
        return true;
    } else if (e->type() == QEvent::Timer) {
        QTimerEvent *te = static_cast<QTimerEvent*>(e);
        d->sendTimerEvent(te->timerId());
    }
    return QAbstractEventDispatcher::event(e);
}

QEventDispatcherWinRTPrivate::QEventDispatcherWinRTPrivate()
    : interrupt(false)
    , timerFactory(0)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    HRESULT hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_System_Threading_ThreadPoolTimer).Get(), &timerFactory);
    if (FAILED(hr))
        qWarning("QEventDispatcherWinRTPrivate::QEventDispatcherWinRTPrivate: Could not obtain timer factory: %lx", hr);
}

QEventDispatcherWinRTPrivate::~QEventDispatcherWinRTPrivate()
{
    if (timerFactory)
        timerFactory->Release();
    CoUninitialize();
}

void QEventDispatcherWinRTPrivate::registerTimer(WinRTTimerInfo *t)
{
    Q_Q(QEventDispatcherWinRT);

    int ok = 0;
    uint interval = t->interval;
    if (interval == 0u) {
        // optimization for single-shot-zero-timer
        QCoreApplication::postEvent(q, new QZeroTimerEvent(t->timerId));
        ok = 1;
    } else {
        TimeSpan period;
        period.Duration = interval * 10000; // TimeSpan is based on 100-nanosecond units
        ok = SUCCEEDED(timerFactory->CreatePeriodicTimer(
                           Callback<ITimerElapsedHandler>(this, &QEventDispatcherWinRTPrivate::timerExpiredCallback).Get(), period, &t->timer));
        if (ok)
            threadPoolTimerDict.insert(t->timer, t);
    }
    t->timeout = qt_msectime() + interval;
    if (ok == 0)
        qErrnoWarning("QEventDispatcherWinRT::registerTimer: Failed to create a timer");
}

void QEventDispatcherWinRTPrivate::unregisterTimer(WinRTTimerInfo *t)
{
    if (t->timer) {
        t->timer->Cancel();
        t->timer->Release();
    }
    delete t;
    t = 0;
}

void QEventDispatcherWinRTPrivate::sendTimerEvent(int timerId)
{
    WinRTTimerInfo *t = timerDict.value(timerId);
    if (t && !t->inTimerEvent) {
        // send event, but don't allow it to recurse
        t->inTimerEvent = true;

        QTimerEvent e(t->timerId);
        QCoreApplication::sendEvent(t->obj, &e);

        // timer could have been removed
        t = timerDict.value(timerId);
        if (t) {
            t->inTimerEvent = false;
        }
    }
}

HRESULT QEventDispatcherWinRTPrivate::timerExpiredCallback(IThreadPoolTimer *source)
{
    register WinRTTimerInfo *t = threadPoolTimerDict.value(source);
    if (t)
        QCoreApplication::postEvent(t->dispatcher, new QTimerEvent(t->timerId));
    else
        qWarning("QEventDispatcherWinRT::timerExpiredCallback: Could not find timer %d in timer list", source);
    return S_OK;
}

QT_END_NAMESPACE
