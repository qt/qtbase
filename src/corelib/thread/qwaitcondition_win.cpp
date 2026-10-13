// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaitcondition.h"
#include "qdeadlinetimer.h"
#include "qnamespace.h"
#include "qmutex.h"
#include "qreadwritelock.h"
#include "qlist.h"
#include "qalgorithms.h"

#define Q_MUTEX_T void *
#include <private/qmutex_p.h>
#include <private/qreadwritelock_p.h>
#include <qt_windows.h>

QT_BEGIN_NAMESPACE

//***********************************************************************
// QWaitConditionPrivate
// **********************************************************************

class QWaitConditionEvent
{
public:
    inline QWaitConditionEvent() : priority(0), wokenUp(false)
    {
        event = CreateEvent(NULL, TRUE, FALSE, NULL);
    }
    inline ~QWaitConditionEvent() { CloseHandle(event); }
    int priority;
    bool wokenUp;
    HANDLE event;
};

typedef QList<QWaitConditionEvent *> EventQueue;

class QWaitConditionPrivate
{
public:
    QMutex mtx;
    EventQueue queue;
    EventQueue freeQueue;

    QWaitConditionEvent *pre();
    bool wait(QWaitConditionEvent *wce, QDeadlineTimer deadline);
    void post(QWaitConditionEvent *wce, bool ret);
};

QWaitConditionEvent *QWaitConditionPrivate::pre()
{
    mtx.lock();
    QWaitConditionEvent *wce =
            freeQueue.isEmpty() ? new QWaitConditionEvent : freeQueue.takeFirst();
    wce->priority = GetThreadPriority(GetCurrentThread());
    wce->wokenUp = false;

    // insert 'wce' into the queue (sorted by priority)
    int index = 0;
    for (; index < queue.size(); ++index) {
        QWaitConditionEvent *current = queue.at(index);
        if (current->priority < wce->priority)
            break;
    }
    queue.insert(index, wce);
    mtx.unlock();

    return wce;
}

bool QWaitConditionPrivate::wait(QWaitConditionEvent *wce, QDeadlineTimer deadline)
{
    // wait for the event
    while (true) {
        const DWORD timeout = deadline.isForever()
                ? INFINITE
                : DWORD(std::min(deadline.remainingTime(), qint64(INFINITE - 1)));

        switch (WaitForSingleObjectEx(wce->event, timeout, FALSE)) {
        case WAIT_OBJECT_0:
            return true;
        case WAIT_TIMEOUT:
            if (deadline.hasExpired())
                return false;
            break;
        default:
            return false;
        }
    }
}

void QWaitConditionPrivate::post(QWaitConditionEvent *wce, bool ret)
{
    mtx.lock();

    // remove 'wce' from the queue
    queue.removeAll(wce);
    ResetEvent(wce->event);
    freeQueue.append(wce);

    // wakeups delivered after the timeout should be forwarded to the next waiter
    if (!ret && wce->wokenUp && !queue.isEmpty()) {
        QWaitConditionEvent *other = queue.constFirst();
        SetEvent(other->event);
        other->wokenUp = true;
    }

    mtx.unlock();
}

//***********************************************************************
// QWaitCondition implementation
//***********************************************************************

QWaitCondition::QWaitCondition()
{
    d = new QWaitConditionPrivate;
}

QWaitCondition::~QWaitCondition()
{
    if (!d->queue.isEmpty()) {
        qWarning("QWaitCondition: Destroyed while threads are still waiting");
        qDeleteAll(d->queue);
    }

    qDeleteAll(d->freeQueue);
    delete d;
}

bool QWaitCondition::wait(QMutex *mutex, unsigned long time)
{
    if (time == std::numeric_limits<unsigned long>::max())
        return wait(mutex, QDeadlineTimer(QDeadlineTimer::Forever));
    return wait(mutex, QDeadlineTimer(time));
}

bool QWaitCondition::wait(QMutex *mutex, QDeadlineTimer deadline)
{
    if (!mutex)
        return false;

    QWaitConditionEvent *wce = d->pre();
    mutex->unlock();

    bool returnValue = d->wait(wce, deadline);

    mutex->lock();
    d->post(wce, returnValue);

    return returnValue;
}

bool QWaitCondition::wait(QReadWriteLock *readWriteLock, unsigned long time)
{
    if (time == std::numeric_limits<unsigned long>::max())
        return wait(readWriteLock, QDeadlineTimer(QDeadlineTimer::Forever));
    return wait(readWriteLock, QDeadlineTimer(time));
}

bool QWaitCondition::wait(QReadWriteLock *readWriteLock, QDeadlineTimer deadline)
{
    using namespace QReadWriteLockStates;

    if (!readWriteLock)
        return false;
    auto previousState = QReadWriteLockPrivate::stateForWaitCondition(readWriteLock);
    if (previousState == Unlocked)
        return false;
    if (previousState == RecursivelyLocked) {
        qWarning("QWaitCondition: cannot wait on QReadWriteLocks with recursive lockForWrite()");
        return false;
    }

    QWaitConditionEvent *wce = d->pre();
    readWriteLock->unlock();

    bool returnValue = d->wait(wce, deadline);

    if (previousState == LockedForWrite)
        readWriteLock->lockForWrite();
    else
        readWriteLock->lockForRead();
    d->post(wce, returnValue);

    return returnValue;
}

void QWaitCondition::wakeOne()
{
    // wake up the first waiting thread in the queue
    QMutexLocker locker(&d->mtx);
    for (QWaitConditionEvent *current : std::as_const(d->queue)) {
        if (current->wokenUp)
            continue;
        SetEvent(current->event);
        current->wokenUp = true;
        break;
    }
}

void QWaitCondition::wakeAll()
{
    // wake up the all threads in the queue
    QMutexLocker locker(&d->mtx);
    for (QWaitConditionEvent *current : std::as_const(d->queue)) {
        SetEvent(current->event);
        current->wokenUp = true;
    }
}

QT_END_NAMESPACE
