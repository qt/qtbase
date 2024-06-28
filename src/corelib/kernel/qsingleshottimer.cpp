// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qsingleshottimer_p.h>

#include "qcoreapplication.h"
#include "qmetaobject_p.h"
#include "private/qnumeric_p.h"

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

QSingleShotTimer::~QSingleShotTimer()
{
    if (timerId > Qt::TimerId::Invalid)
        killTimer(timerId);
}

/*
    Move the timer, and the dispatching and handling of the timer event, into
    the same thread as where it will be handled, so that it fires reliably even
    if the thread that set up the timer is busy.
*/
void QSingleShotTimer::startTimerForReceiver(Duration interval, Qt::TimerType timerType,
                                             const QObject *receiver)
{
    if (receiver && receiver->thread() != thread()) {
        // Avoid leaking the QSingleShotTimer instance in case the application exits before the
        // timer fires
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this,
                &QObject::deleteLater);
        setParent(nullptr);
        moveToThread(receiver->thread());

        QDeadlineTimer deadline(interval, timerType);
        auto invokable = [this, deadline, timerType] {
            if (deadline.hasExpired()) {
                Q_EMIT timeout();
            } else {
                timerId = Qt::TimerId{startTimer(deadline.remainingTimeAsDuration(), timerType)};
            }
        };
        QMetaObject::invokeMethod(this, invokable, Qt::QueuedConnection);
    } else {
        timerId = Qt::TimerId{startTimer(interval, timerType)};
    }
}

void QSingleShotTimer::timerEvent(QTimerEvent *)
{
    // need to kill the timer _before_ we emit timeout() in case the
    // slot connected to timeout calls processEvents()
    if (timerId > Qt::TimerId::Invalid)
        killTimer(std::exchange(timerId, Qt::TimerId::Invalid));

    Q_EMIT timeout();

    // we would like to use delete later here, but it feels like a
    // waste to post a new event to handle this event, so we just unset the flag
    // and explicitly delete...
    delete this;
}

QT_END_NAMESPACE

#include "moc_qsingleshottimer_p.cpp"
