// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbasictimer.h"
#include "qabstracteventdispatcher.h"
#include "qabstracteventdispatcher_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QBasicTimer
    \inmodule QtCore
    \brief The QBasicTimer class provides timer events for objects.

    \ingroup events

    This is a fast, lightweight, and low-level class used by Qt
    internally. We recommend using the higher-level QTimer class
    rather than this class if you want to use timers in your
    applications. Note that this timer is a repeating timer that
    will send subsequent timer events unless the stop() function is called.

    To use this class, create a QBasicTimer, and call its start()
    function with a timeout interval and with a pointer to a QObject
    subclass. When the timer times out it will send a timer event to
    the QObject subclass. The timer can be stopped at any time using
    stop(). isActive() returns \c true for a timer that is running;
    i.e. it has been started, has not reached the timeout time, and
    has not been stopped. The timer's ID can be retrieved using
    timerId().

    Objects of this class cannot be copied, but can be moved, so you
    can maintain a list of basic timers by holding them in container
    that supports move-only types, e.g. std::vector.

    \sa QTimer, QTimerEvent, QObject::timerEvent(), Timers, {Affine Transformations}
*/


/*!
    \fn QBasicTimer::QBasicTimer()

    Constructs a basic timer.

    \sa start()
*/

/*!
    \fn QBasicTimer::QBasicTimer(QBasicTimer &&other)
    \since 5.14

    Move-constructs a basic timer from \a other, which is left
    \l{isActive()}{inactive}.

    \sa isActive(), swap()
*/

/*!
    \fn QBasicTimer &QBasicTimer::operator=(QBasicTimer &&other)
    \since 5.14

    Move-assigns \a other to this basic timer. The timer
    previously represented by this basic timer is stopped.
    \a other is left as \l{isActive()}{inactive}.

    \sa stop(), isActive(), swap()
*/

/*!
    \fn QBasicTimer::~QBasicTimer()

    Destroys the basic timer.
*/

/*!
    \fn bool QBasicTimer::isActive() const

    Returns \c true if the timer is running and has not been stopped; otherwise
    returns \c false.

    \sa start(), stop()
*/

/*!
    \fn QBasicTimer::swap(QBasicTimer &other)
    \since 5.14

    Swaps the timer \a other with this timer.
    This operation is very fast and never fails.
*/

/*!
    \fn swap(QBasicTimer &lhs, QBasicTimer &rhs)
    \relates QBasicTimer
    \since 5.14

    Swaps the timer \a lhs with \a rhs.
    This operation is very fast and never fails.
*/

/*!
    \fn int QBasicTimer::timerId() const

    Returns the timer's ID.

    \sa QTimerEvent::timerId()
*/

/*!
    \fn void QBasicTimer::start(int msec, QObject *object)

    \obsolete Use chrono overload instead.
*/

/*!
    \since 6.5

    Starts (or restarts) the timer with a \a duration timeout. The
    timer will be a Qt::CoarseTimer. See Qt::TimerType for information on the
    different timer types.

    The given \a object will receive timer events.

    \sa stop(), isActive(), QObject::timerEvent(), Qt::CoarseTimer
 */
void QBasicTimer::start(std::chrono::milliseconds duration, QObject *object)
{
    start(duration, Qt::CoarseTimer, object);
}

/*!
    \fn QBasicTimer::start(int msec, Qt::TimerType timerType, QObject *obj)
    \overload
    \obsolete

    Use chrono overload instead.
*/

/*!
    \since 6.5

    Starts (or restarts) the timer with a \a duration timeout and the
    given \a timerType. See Qt::TimerType for information on the different
    timer types.

    \a obj will receive timer events.

    \sa stop(), isActive(), QObject::timerEvent(), Qt::TimerType
 */
void QBasicTimer::start(std::chrono::milliseconds duration, Qt::TimerType timerType, QObject *obj)
{
    QAbstractEventDispatcher *eventDispatcher = QAbstractEventDispatcher::instance();
    if (Q_UNLIKELY(duration.count() < 0)) {
        qWarning("QBasicTimer::start: Timers cannot have negative timeouts");
        return;
    }
    if (Q_UNLIKELY(!eventDispatcher)) {
        qWarning("QBasicTimer::start: QBasicTimer can only be used with threads started with QThread");
        return;
    }
    if (Q_UNLIKELY(obj && obj->thread() != eventDispatcher->thread())) {
        qWarning("QBasicTimer::start: Timers cannot be started from another thread");
        return;
    }
    stop();
    if (obj)
        id = eventDispatcher->registerTimer(duration.count(), timerType, obj);
}

/*!
    Stops the timer.

    \sa start(), isActive()
*/
void QBasicTimer::stop()
{
    if (id) {
        QAbstractEventDispatcher *eventDispatcher = QAbstractEventDispatcher::instance();
        if (eventDispatcher && !eventDispatcher->unregisterTimer(id)) {
            qWarning("QBasicTimer::stop: Failed. Possibly trying to stop from a different thread");
            return;
        }
        QAbstractEventDispatcherPrivate::releaseTimerId(id);
    }
    id = 0;
}

QT_END_NAMESPACE
