// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qsingleshottimer_p.h>

#include "qcoreapplication.h"
#include "qmetaobject_p.h"
#include "private/qnumeric_p.h"
#include "qthread.h"

QT_BEGIN_NAMESPACE

QSingleShotTimer::QSingleShotTimer(Duration interval, Qt::TimerType timerType,
                                   const QObject *r, const char *member)
    : QObject(QAbstractEventDispatcher::instance())
{
    connect(this, SIGNAL(timeout()), r, member);
    startTimerForReceiver(interval, timerType, r);
}

QSingleShotTimer::QSingleShotTimer(Duration interval, Qt::TimerType timerType,
                                   const QObject *r, QtPrivate::QSlotObjectBase *slotObj)
    : QObject(QAbstractEventDispatcher::instance())
{
    int signal_index = QMetaObjectPrivate::signalOffset(&staticMetaObject);
    Q_ASSERT(QMetaObjectPrivate::signal(&staticMetaObject, signal_index).name() == "timeout");
    QObjectPrivate::connectImpl(this, signal_index, r ? r : this, nullptr, slotObj,
                                Qt::AutoConnection, nullptr, &staticMetaObject);

    startTimerForReceiver(interval, timerType, r);
}

QSingleShotTimer::~QSingleShotTimer() = default;

/*
    Move the timer, and the dispatching and handling of the timer event, into
    the same thread as where it will be handled, so that it fires reliably even
    if the thread that set up the timer is busy.
*/
void QSingleShotTimer::startTimerForReceiver(Duration interval, Qt::TimerType timerType,
                                             const QObject *receiver)
{
    if (receiver && receiver->thread() != thread()) {
        QObjectPrivate *d_ptr = QObjectPrivate::get(this);
        d_ptr->sendChildEvents = false;

        setParent(nullptr);
        moveToThread(receiver->thread());

        QCoreApplication::postEvent(this,
                                    new StartTimerEvent(this, QDeadlineTimer(interval, timerType)));
        // the event owns "this" and is handled concurrently, so unsafe to
        // access "this" beyond this point
    } else {
        timer.start(interval, timerType, this);
    }
}

void QSingleShotTimer::timerFinished()
{
    Q_EMIT timeout();
    delete this;
}

void QSingleShotTimer::timerEvent(QTimerEvent *event)
{
    if (event->id() == Qt::TimerId::Invalid) {
        StartTimerEvent *startTimerEvent = static_cast<StartTimerEvent *>(event);
        Q_UNUSED(startTimerEvent->timer.release()); // it's the same as "this"
        const QDeadlineTimer &deadline = startTimerEvent->deadline;
        if (deadline.hasExpired()) {
            timerFinished();
        } else {
            timer.start(deadline.remainingTimeAsDuration(), deadline.timerType(), this);
            // we are now definitely in a thread that has an event dispatcher
            setParent(QThread::currentThread()->eventDispatcher());
        }
    } else {
        // need to kill the timer _before_ we emit timeout() in case the
        // slot connected to timeout calls processEvents()
        timer.stop();

        timerFinished();
    }
}

QT_END_NAMESPACE

#include "moc_qsingleshottimer_p.cpp"
