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

    \snippet timers/timers.cpp timer-interval-in-ctor
    \snippet timers/timers.cpp timer-setinterval

    You can set a timer to time out only once by calling setSingleShot(true).

    QChronoTimer also has singleShot() static methods:

    \snippet timers/timers.cpp qchronotimer-singleshot

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

    An alternative to using QChronoTimer is to call QObject::startTimer()
    for your object and reimplement the QObject::timerEvent() event handler
    in your class (which must be a sub-class of QObject). The disadvantage
    is that timerEvent() does not support such high-level features as
    single-shot timers or signals.

    Another alternative is QBasicTimer. It is typically less cumbersome
    than using QObject::startTimer() directly. See \l{Timers} for an
    overview of all three approaches.

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
    Returns \c true if the timer is running (pending); otherwise returns
    false.
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
    if (id > Qt::TimerId::Invalid) {
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
    if (Qt::TimerId{e->timerId()} == d->id) {
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

    \sa interval, QChronoTimer::singleShot()
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

    Setting the interval of an active timer will change the interval,
    stop() and then start() the timer, and acquire a new id().
    If the timer is not active, only the interval is changed.

    \sa singleShot
*/
void QChronoTimer::setInterval(std::chrono::nanoseconds nsec)
{
    auto *d = d_func();
    d->intervalDuration.removeBindingUnlessInWrapper();
    const bool intervalChanged = nsec != d->intervalDuration.valueBypassingBindings();
    d->intervalDuration.setValueBypassingBindings(nsec);
    if (d->isActive()) { // Create new timer
        QObject::killTimer(d->id); // Restart timer
        const auto newId = Qt::TimerId{QObject::startTimer(nsec, d->type)};
        if (newId > Qt::TimerId::Invalid) {
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

/*!
    \overload
    \reentrant

    This static function calls the slot \a member, on object \a receiver, after
    time interval \a interval. \a timerType affects the precision of the timer

    \a member has to be a member function of \a receiver; you need to use the
    \c SLOT() macro to get this parameter.

    This function is provided as a convenience to save the need to use a
    \l{QObject::timerEvent()}{timerEvent} or create a local QChronoTimer
    object.

    \sa start(), Qt::TimerType
*/
void QChronoTimer::singleShot(std::chrono::nanoseconds interval, Qt::TimerType timerType,
                              const QObject *receiver, const char *member)
{
    if (Q_UNLIKELY(interval < 0ns)) {
        qWarning("QChronoTimer::singleShot: Timers cannot have negative timeouts");
        return;
    }
    if (receiver && member) {
        if (interval == 0ns) {
            // special code shortpath for 0-timers
            const char* bracketPosition = strchr(member, '(');
            if (!bracketPosition || !(member[0] >= '0' && member[0] <= '2')) {
                qWarning("QChronoTimer::singleShot: Invalid slot specification");
                return;
            }
            const auto methodName = QByteArrayView(member + 1, // extract method name
                                                   bracketPosition - 1 - member).trimmed();
            QMetaObject::invokeMethod(const_cast<QObject *>(receiver),
                                      methodName.toByteArray().constData(),
                                      Qt::QueuedConnection);
            return;
        }
        (void) new QSingleShotTimer(interval, timerType, receiver, member);
    }
}

/*!
    \internal

    \list
        \li \a interval the time interval
        \li \a timerType the type of the timer; this affects the precision of
            the timer
        \li \a receiver the receiver or context object; if this is \c nullptr,
            this method will figure out a context object to use, see code
            comments below
        \li \a slotObj a callable, for example a lambda
    \endlist
*/
void QChronoTimer::singleShotImpl(std::chrono::nanoseconds interval, Qt::TimerType timerType,
                                  const QObject *receiver, QtPrivate::QSlotObjectBase *slotObj)
{
    if (interval == 0ns) {
        bool deleteReceiver = false;
        // Optimize: set a receiver context when none is given, such that we can use
        // QMetaObject::invokeMethod which is more efficient than going through a timer.
        // We need a QObject living in the current thread. But the QThread itself lives
        // in a different thread - with the exception of the main QThread which lives in
        // itself. And QThread::currentThread() is among the few QObjects we know that will
        // most certainly be there. Note that one can actually call singleShot before the
        // QApplication is created!
        if (!receiver && QThread::currentThread() == QCoreApplicationPrivate::mainThread()) {
            // reuse main thread as context object
            receiver = QThread::currentThread();
        } else if (!receiver) {
            // Create a receiver context object on-demand. According to the benchmarks,
            // this is still more efficient than going through a timer.
            receiver = new QObject;
            deleteReceiver = true;
        }

        auto h = QtPrivate::invokeMethodHelper({});
        QMetaObject::invokeMethodImpl(const_cast<QObject *>(receiver), slotObj,
                Qt::QueuedConnection, h.parameterCount(), h.parameters.data(), h.typeNames.data(),
                h.metaTypes.data());

        if (deleteReceiver)
            const_cast<QObject *>(receiver)->deleteLater();
        return;
    }

    new QSingleShotTimer(interval, timerType, receiver, slotObj);
}

QT_END_NAMESPACE

#include "moc_qchronotimer.cpp"
