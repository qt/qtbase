// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSINGLESHOTTIMER_P_H
#define QSINGLESHOTTIMER_P_H

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

#include "qabstracteventdispatcher.h"
#include "qcoreapplication.h"
#include "qmetaobject_p.h"
#include "private/qnumeric_p.h"

#include <chrono>

QT_BEGIN_NAMESPACE

class QSingleShotTimer : public QObject
{
    Q_OBJECT

    Qt::TimerId timerId = Qt::TimerId::Invalid;

public:
    // use the same duration type
    using Duration = QAbstractEventDispatcher::Duration;

    inline ~QSingleShotTimer();
    inline QSingleShotTimer(Duration interval, Qt::TimerType timerType,
                            const QObject *r, const char *member);
    inline QSingleShotTimer(Duration interval, Qt::TimerType timerType,
                            const QObject *r, QtPrivate::QSlotObjectBase *slotObj);

    inline void startTimerForReceiver(Duration interval, Qt::TimerType timerType,
                                      const QObject *receiver);

    static Duration fromMsecs(std::chrono::milliseconds ms)
    {
        using namespace std::chrono;
        using ratio = std::ratio_divide<std::milli, Duration::period>;
        static_assert(ratio::den == 1);

        Duration::rep r;
        if (qMulOverflow<ratio::num>(ms.count(), &r)) {
            qWarning("QTimer::singleShot(std::chrono::milliseconds, ...): "
                     "interval argument overflowed when converted to nanoseconds.");
            return Duration::max();
        }
        return Duration{r};
    }
Q_SIGNALS:
    void timeout();

private:
    inline void timerEvent(QTimerEvent *) override;
};

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

#endif // QSINGLESHOTTIMER_P_H
