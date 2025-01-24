// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qchronotimer.h"
#include "qtimer_p.h"
#include "qsingleshottimer_p.h"

#include "qabstracteventdispatcher.h"
#include "qcoreapplication.h"
#include "qcoreapplication_p.h"
#include "qdeadlinetimer.h"
#include "qmetaobject_p.h"
#include "qobject_p.h"
#include "qproperty_p.h"
#include "qthread.h"

using namespace std::chrono_literals;

QT_BEGIN_NAMESPACE

/*!
    \class QChronoTimer
    \inmodule QtCore
    \since 6.8
    \ingroup events

    \brief The QChronoTimer class provides repetitive and single-shot timers.

    The QChronoTimer class provides a high-level programming interface for
    timers. To use it, create a QChronoTimer, either passing the interval to the
    constructor, or setting it after construction using setInterval(), connect
    its timeout() signal to the appropriate slots, and call start(). From then
    on, it will emit the timeout() signal at constant intervals. For example:

    \snippet timers/timers.cpp qchronotimer-interval-in-ctor
    \snippet timers/timers.cpp qchronotimer-setinterval

    You can set a timer to time out only once by calling setSingleShot(true).

    \note QChronoTimer has no singleShot() static methods, as the ones on
    QTimer already work with chrono types and nanoseconds resolution.

    In multithreaded applications, you can use QChronoTimer in any thread
    that has an event loop. To start an event loop from a non-GUI
    thread, use QThread::exec(). Qt uses the timer's
    \l{QObject::thread()}{thread affinity} to determine which thread
    will emit the \l{QChronoTimer::}{timeout()} signal. Because of this, you
    must start and stop the timer in its thread; it is not possible to
    start a timer from another thread.

    As a special case, a QChronoTimer with a timeout of \c 0ns will time out
    as soon as possible, though the ordering between zero timers and other
    sources of events is unspecified. Zero timers can be used to do some
    work while still providing a responsive user interface:

    \snippet timers/timers.cpp zero-timer

    From then on, \c processOneThing() will be called repeatedly. It should
    be written in such a way that it always returns quickly (for example,
    after processing one data item) so that Qt can deliver events to the user
    interface and stop the timer as soon as it has done all its work. This
    is the traditional way of implementing heavy work in GUI applications,
    but as multithreading is becoming available on more platforms, a modern
    alternative is doing the heavy work in a thread other than the GUI (main)
    thread. Qt has the QThread class, which can be used to achieve that.

    \section1 Accuracy and Timer Resolution

    The accuracy of timers depends on the underlying operating system and
    hardware. Most platforms support requesting nano-second precision for
    timers (for example, libc's \c nanosleep), though the accuracy of the
    timer will not equal this resolution in many real-world situations.

    You can set the \l{Qt::TimerType}{timer type} to tell QChronoTimer which
    precision to request from the system.

    For Qt::PreciseTimer, QChronoTimer will try to keep the precision at
    \c 1ns. Precise timers will never time out earlier than expected.

    For Qt::CoarseTimer and Qt::VeryCoarseTimer types, QChronoTimer may wake
    up earlier than expected, within the margins for those types:
    \list
        \li 5% of the interval for Qt::CoarseTimer
        \li \c 500ms for Qt::VeryCoarseTimer
    \endlist

    All timer types may time out later than expected if the system is busy or
    unable to provide the requested accuracy. In such a case of timeout
    overrun, Qt will emit timeout() only once, even if multiple timeouts have
    expired, and then will resume the original interval.

    \section1 Alternatives to QChronoTimer

    QChronoTimer provides nanosecond resolution and a ±292 years range
    (less chances of integer overflow if the interval is longer than \c
    std::numeric_limits<int>::max()). If you only need millisecond resolution
    and ±24 days range, you can continue to use the classical QTimer class

    \include timers-common.qdocinc q-chrono-timer-alternatives

    Some operating systems limit the number of timers that may be used;
    Qt does its best to work around these limitations.

    \sa QBasicTimer, QTimerEvent, QObject::timerEvent(), Timers,
        {Analog Clock}
*/

/*!
    Constructs a timer with the given \a parent, using the default interval,
    \c 0ns.
*/
QChronoTimer::QChronoTimer(QObject *parent)
    : QChronoTimer(0ns, parent)
{
}

/*!
    Constructs a timer with the given \a parent, using an interval of \a nsec.
*/
QChronoTimer::QChronoTimer(std::chrono::nanoseconds nsec, QObject *parent)
    : QObject(*new QTimerPrivate(nsec, this), parent)
{
    Q_ASSERT(!d_func()->isQTimer);
}

/*!
    Destroys the timer.
*/
QChronoTimer::~QChronoTimer()
{
    if (d_func()->isActive()) // stop running timer
        stop();
}

/*!
    \fn void QChronoTimer::timeout()

    This signal is emitted when the timer times out.

    \sa interval, start(), stop()
*/

/*!
    \property QChronoTimer::active

    This boolean property is \c true if the timer is running; otherwise
    \c false.
*/

/*!
    Returns \c true if the timer is running; otherwise returns \c false.
*/
bool QChronoTimer::isActive() const
{
    return d_func()->isActiveData.value();
}

QBindable<bool> QChronoTimer::bindableActive()
{
    return QBindable<bool>(&d_func()->isActiveData);
}

/*!
    Returns a Qt::TimerId representing the timer ID if the timer is running;
    otherwise returns \c Qt::TimerId::Invalid.

    \sa Qt::TimerId
*/
Qt::TimerId QChronoTimer::id() const
{
    return d_func()->id;
}

/*! \overload start()

    Starts or restarts the timer with the timeout specified in \l interval.

//! [stop-restart-timer]
    If the timer is already running, it will be
    \l{QChronoTimer::stop()}{stopped} and restarted. This will also change its
    id().
//! [stop-restart-timer]

    If \l singleShot is true, the timer will be activated only once.
*/
void QChronoTimer::start()
{
    auto *d = d_func();
    if (d->isActive()) // stop running timer
        stop();
    const auto id = Qt::TimerId{QObject::startTimer(d->intervalDuration, d->type)};
    if (id != Qt::TimerId::Invalid) {
        d->id = id;
        d->isActiveData.notify();
    }
}

/*!
    Stops the timer.

    \sa start()
*/
void QChronoTimer::stop()
{
    auto *d = d_func();
    if (d->isActive()) {
        QObject::killTimer(d->id);
        d->id = Qt::TimerId::Invalid;
        d->isActiveData.notify();
    }
}

/*!
  \reimp
*/
void QChronoTimer::timerEvent(QTimerEvent *e)
{
    auto *d = d_func();
    if (e->id() == d->id) {
        if (d->single)
            stop();
        Q_EMIT timeout(QPrivateSignal());
    }
}

/*!
    \fn template <typename Functor> QMetaObject::Connection QChronoTimer::callOnTimeout(const QObject *context, Functor &&slot, Qt::ConnectionType connectionType = Qt::AutoConnection)
    \overload callOnTimeout()

    Creates a connection from the timeout() signal to \a slot to be placed in a
    specific event loop of \a context, with connection type \a connectionType,
    and returns a handle to the connection.

    This method is provided as a convenience. It's equivalent to calling:
    \code
    QObject::connect(timer, &QChronoTimer::timeout, context, slot, connectionType);
    \endcode

    \sa QObject::connect(), timeout()
*/

/*!
    \property QChronoTimer::singleShot
    \brief Whether the timer is a single-shot timer

    A single-shot timer fires only once, non-single-shot timers fire every
    \l interval.

    The default value for this property is \c false.

    \sa interval
*/
void QChronoTimer::setSingleShot(bool singleShot)
{
    d_func()->single = singleShot;
}

bool QChronoTimer::isSingleShot() const
{
    return d_func()->single;
}

QBindable<bool> QChronoTimer::bindableSingleShot()
{
    return QBindable<bool>(&d_func()->single);
}

/*!
    \property QChronoTimer::interval
    \brief The timeout interval

    The default value for this property is \c 0ns.

    A QChronoTimer with a timeout of \c 0ns will time out as soon as all
    the events in the window system's event queue have been processed.

    Setting the interval of a running timer will change the interval,
    stop() and then start() the timer, and acquire a new id().
    If the timer is not running, only the interval is changed.

    \include timers-common.qdocinc negative-intervals-not-allowed

    \sa singleShot
*/
void QChronoTimer::setInterval(std::chrono::nanoseconds nsec)
{
    if (nsec < 0ns) {
        qWarning("QChronoTimer::setInterval: negative intervals aren't allowed; the "
                 "interval will be set to 1ms.");
        nsec = 1ms;
    }

    auto *d = d_func();
    d->intervalDuration.removeBindingUnlessInWrapper();
    const bool intervalChanged = nsec != d->intervalDuration.valueBypassingBindings();
    d->intervalDuration.setValueBypassingBindings(nsec);
    if (d->isActive()) { // Create new timer
        QObject::killTimer(d->id); // Restart timer
        const auto newId = Qt::TimerId{QObject::startTimer(nsec, d->type)};
        if (newId != Qt::TimerId::Invalid) {
            // Restarted successfully. No need to update the active state.
            d->id = newId;
        } else {
            // Failed to start the timer.
            // Need to notify about active state change.
            d->id = Qt::TimerId::Invalid;
            d->isActiveData.notify();
        }
    }
    if (intervalChanged)
        d->intervalDuration.notify();
}

std::chrono::nanoseconds QChronoTimer::interval() const
{
    return d_func()->intervalDuration.value();
}

QBindable<std::chrono::nanoseconds> QChronoTimer::bindableInterval()
{
    return {&d_func()->intervalDuration};
}

/*!
    \property QChronoTimer::remainingTime
    \brief The remaining time

    Returns the remaining duration until the timeout.

    If the timer is inactive, the returned duration will be negative.

    If the timer is overdue, the returned duration will be \c 0ns.

    \sa interval
*/
std::chrono::nanoseconds QChronoTimer::remainingTime() const
{
    if (isActive())
        return QAbstractEventDispatcher::instance()->remainingTime(d_func()->id);
    return std::chrono::nanoseconds::min();
}

/*!
    \property QChronoTimer::timerType
    \brief Controls the accuracy of the timer

    The default value for this property is \c Qt::CoarseTimer.

    \sa Qt::TimerType
*/
void QChronoTimer::setTimerType(Qt::TimerType atype)
{
    d_func()->type = atype;
}

Qt::TimerType QChronoTimer::timerType() const
{
    return d_func()->type;
}

QBindable<Qt::TimerType> QChronoTimer::bindableTimerType()
{
    return {&d_func()->type};
}

QT_END_NAMESPACE

#include "moc_qchronotimer.cpp"
