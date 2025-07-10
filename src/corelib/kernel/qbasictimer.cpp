// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbasictimer.h"
#include "qabstracteventdispatcher.h"
#include "qabstracteventdispatcher_p.h"

#include <private/qthread_p.h>

using namespace std::chrono_literals;

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

    \sa QTimer, QChronoTimer, QTimerEvent, QObject::timerEvent(),
    Timers, {Affine Transformations}
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
    \memberswap{timer}
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
    \obsolete

    Returns the timer's ID.

    In new code use id() instead.

    \sa QTimerEvent::timerId()
*/

/*!
    \fn Qt::TimerId QBasicTimer::id() const
    \since 6.8

    Returns the timer's ID.

    \sa QTimerEvent::id()
*/

/*!
    \fn void QBasicTimer::start(int msec, QObject *object)

    \obsolete Use chrono overload instead.
*/

/*!
    \typedef QBasicTimer::Duration

    A \c{std::chrono::duration} type that is used in various API in this class.
    This type exists to facilitate a possible transition to a higher or lower
    granularity.

    In all current platforms, it is \c nanoseconds.
*/

/*!
    \fn void QBasicTimer::start(Duration duration, QObject *object)
    \since 6.5

    Starts (or restarts) the timer with a \a duration timeout. The
    timer will be a Qt::CoarseTimer. See Qt::TimerType for information on the
    different timer types.

    The given \a object will receive timer events.

    \include timers-common.qdocinc negative-intervals-not-allowed

//! [start-nanoseconds-note]
    \note Starting from Qt 6.9 this method takes std::chrono::nanoseconds,
          before that it took std::chrono::milliseconds. This change is
          backwards compatible.
//! [start-nanoseconds-note]

    \sa stop(), isActive(), QObject::timerEvent(), Qt::CoarseTimer
 */

/*!
    \fn QBasicTimer::start(int msec, Qt::TimerType timerType, QObject *obj)
    \overload
    \obsolete

    Use chrono overload instead.
*/

/*!
    \since 6.5
    \overload

    Starts (or restarts) the timer with a \a duration timeout and the
    given \a timerType. See Qt::TimerType for information on the different
    timer types.

    \include timers-common.qdocinc negative-intervals-not-allowed

    \a obj will receive timer events.

    \include qbasictimer.cpp start-nanoseconds-note

    \sa stop(), isActive(), QObject::timerEvent(), Qt::TimerType
 */
void QBasicTimer::start(Duration duration, Qt::TimerType timerType, QObject *obj)
{
    QAbstractEventDispatcher *eventDispatcher = QAbstractEventDispatcher::instance();
    if (duration < 0ns) {
        qWarning("QBasicTimer::start: negative intervals aren't allowed; the "
                 "interval will be set to 1ms.");
        duration = 1ms;
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
        m_id = eventDispatcher->registerTimer(duration, timerType, obj);
}

/*!
    Stops the timer.

    \sa start(), isActive()
*/
void QBasicTimer::stop()
{
    if (isActive()) {
        QAbstractEventDispatcher *eventDispatcher = nullptr;

        // don't create the current thread data if it's already been destroyed
        if (QThreadData *data = QThreadData::currentThreadData())
            eventDispatcher = data->eventDispatcher.loadRelaxed();

        if (eventDispatcher && !eventDispatcher->unregisterTimer(m_id)) {
            qWarning("QBasicTimer::stop: Failed. Possibly trying to stop from a different thread");
            return;
        }
        QAbstractEventDispatcherPrivate::releaseTimerId(m_id);
    }
    m_id = Qt::TimerId::Invalid;
}

QT_END_NAMESPACE
