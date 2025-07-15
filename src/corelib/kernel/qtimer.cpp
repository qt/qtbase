// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtimer.h"
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

QTimerPrivate::~QTimerPrivate()
    = default;

/*!
    \class QTimer
    \inmodule QtCore
    \brief The QTimer class provides repetitive and single-shot timers.

    \ingroup events


    The QTimer class provides a high-level programming interface for
    timers. To use it, create a QTimer, connect its timeout() signal
    to the appropriate slots, and call start(). From then on, it will
    emit the timeout() signal at constant intervals.

    Example for a one second (1000 millisecond) timer (from the
    \l{widgets/analogclock}{Analog Clock} example):

    \snippet ../widgets/widgets/analogclock/analogclock.cpp 4
    \snippet ../widgets/widgets/analogclock/analogclock.cpp 5
    \snippet ../widgets/widgets/analogclock/analogclock.cpp 6

    From then on, the \c update() slot is called every second.

    You can set a timer to time out only once by calling
    setSingleShot(true). You can also use the static
    QTimer::singleShot() function to call a slot after a specified
    interval:

    \snippet timers/timers.cpp 3

    In multithreaded applications, you can use QTimer in any thread
    that has an event loop. To start an event loop from a non-GUI
    thread, use QThread::exec(). Qt uses the timer's
    \l{QObject::thread()}{thread affinity} to determine which thread
    will emit the \l{QTimer::}{timeout()} signal. Because of this, you
    must start and stop the timer in its thread; it is not possible to
    start a timer from another thread.

    As a special case, a QTimer with a timeout of 0 will time out as soon as
    possible, though the ordering between zero timers and other sources of
    events is unspecified. Zero timers can be used to do some work while still
    providing a snappy user interface:

    \snippet timers/timers.cpp 4
    \snippet timers/timers.cpp 5
    \snippet timers/timers.cpp 6

    From then on, \c processOneThing() will be called repeatedly. It
    should be written in such a way that it always returns quickly
    (typically after processing one data item) so that Qt can deliver
    events to the user interface and stop the timer as soon as it has done all
    its work. This is the traditional way of implementing heavy work
    in GUI applications, but as multithreading is nowadays becoming available on
    more and more platforms, we expect that zero-millisecond
    QTimer objects will gradually be replaced by \l{QThread}s.

    \section1 Accuracy and Timer Resolution

    The accuracy of timers depends on the underlying operating system
    and hardware. Most platforms support a resolution of 1 millisecond,
    though the accuracy of the timer will not equal this resolution
    in many real-world situations.

    The accuracy also depends on the \l{Qt::TimerType}{timer type}. For
    Qt::PreciseTimer, QTimer will try to keep the accuracy at 1 millisecond.
    Precise timers will also never time out earlier than expected.

    For Qt::CoarseTimer and Qt::VeryCoarseTimer types, QTimer may wake up
    earlier than expected, within the margins for those types: 5% of the
    interval for Qt::CoarseTimer and 500 ms for Qt::VeryCoarseTimer.

    All timer types may time out later than expected if the system is busy or
    unable to provide the requested accuracy. In such a case of timeout
    overrun, Qt will emit timeout() only once, even if multiple timeouts have
    expired, and then will resume the original interval.

    \section1 Alternatives to QTimer

    Qt 6.8 introduced QChronoTimer. The main difference between the two
    classes, is that QChronoTimer supports a larger interval range and a
    higher precision (\c std::chrono::nanoseconds). For QTimer the maximum
    supported interval is ±24 days, whereas for QChronoTimer it is ±292
    years (less chances of interger overflow with intervals longer than
    \c std::numeric_limits<int>::max()). If you only need millisecond
    resolution and ±24 days range, you can continue to use QTimer.

    \include timers-common.qdocinc q-chrono-timer-alternatives

    Some operating systems limit the number of timers that may be
    used; Qt tries to work around these limitations.

    \sa QBasicTimer, QTimerEvent, QObject::timerEvent(), Timers,
        {Analog Clock}
*/

/*!
    Constructs a timer with the given \a parent.
*/

QTimer::QTimer(QObject *parent)
    : QObject(*new QTimerPrivate(this), parent)
{
    Q_ASSERT(d_func()->isQTimer);
}


/*!
    Destroys the timer.
*/

QTimer::~QTimer()
{
    if (d_func()->isActive()) // stop running timer
        stop();
}


/*!
    \fn void QTimer::timeout()

    This signal is emitted when the timer times out.

    \sa interval, start(), stop()
*/

/*!
    \property QTimer::active
    \since 4.3

    This boolean property is \c true if the timer is running; otherwise
    false.
*/

/*!
    \fn bool QTimer::isActive() const

    Returns \c true if the timer is running; otherwise returns \c false.
*/
bool QTimer::isActive() const
{
    return d_func()->isActiveData.value();
}

QBindable<bool> QTimer::bindableActive()
{
    return QBindable<bool>(&d_func()->isActiveData);
}

/*!
    \fn int QTimer::timerId() const

    Returns the ID of the timer if the timer is running; otherwise returns
    -1.
*/
int QTimer::timerId() const
{
    auto v = qToUnderlying(id());
    return v == 0 ? -1 : v;
}

/*!
    \since 6.8
    Returns a Qt::TimerId representing the timer ID if the timer is running;
    otherwise returns \c Qt::TimerId::Invalid.

    \sa Qt::TimerId
*/
Qt::TimerId QTimer::id() const
{
    return d_func()->id;
}

/*! \overload start()

    Starts or restarts the timer with the timeout specified in \l interval.

//! [stop-restart-timer]
    If the timer is already running, it will be
    \l{QTimer::stop()}{stopped} and restarted. This will also change its id().
//! [stop-restart-timer]

//! [singleshot-activation]
    If \l singleShot is true, the timer will be activated only once.
//! [singleshot-activation]
*/
void QTimer::start()
{
    Q_D(QTimer);
    if (d->isActive()) // stop running timer
        stop();

    Qt::TimerId newId{ QObject::startTimer(d->inter * 1ms, d->type) }; // overflow impossible
    if (newId > Qt::TimerId::Invalid) {
        d->id = newId;
        d->isActiveData.notify();
    }
}

/*!
    Starts or restarts the timer with a timeout interval of \a msec
    milliseconds.

    This is equivalent to:

    \code
        timer.setInterval(msec);
        timer.start();
    \endcode

    \include qtimer.cpp stop-restart-timer

    \include qtimer.cpp singleshot-activation

    \include timers-common.qdocinc negative-intervals-not-allowed

    \note   Keeping the event loop busy with a zero-timer is bound to
            cause trouble and highly erratic behavior of the UI.
*/
void QTimer::start(int msec)
{
    start(msec * 1ms);
}

static std::chrono::milliseconds
checkInterval(const char *caller, std::chrono::milliseconds interval)
{
    constexpr auto maxInterval = INT_MAX * 1ms;
    if (interval < 0ms) {
        qWarning("%s: negative intervals aren't allowed; the interval will be set to 1ms.", caller);
        interval = 1ms;
    } else if (interval > maxInterval) {
        qWarning("%s: interval exceeds maximum allowed interval, it will be clamped to "
                 "INT_MAX ms (about 24 days).", caller);
        interval = maxInterval;
    }
    return interval;
}

/*!
    \since 5.8
    \overload

    Starts or restarts the timer with a timeout of duration \a interval milliseconds.

    This is equivalent to:

    \code
        timer.setInterval(interval);
        timer.start();
    \endcode

    \include qtimer.cpp stop-restart-timer

    \include qtimer.cpp singleshot-activation

    \include timers-common.qdocinc negative-intervals-not-allowed
*/
void QTimer::start(std::chrono::milliseconds interval)
{
    Q_D(QTimer);

    interval = checkInterval("QTimer::start", interval);
    const int msec = interval.count();
    const bool intervalChanged = msec != d->inter;
    d->inter.setValue(msec);
    start();
    if (intervalChanged)
        d->inter.notify();
}



/*!
    Stops the timer.

    \sa start()
*/

void QTimer::stop()
{
    Q_D(QTimer);
    if (d->isActive()) {
        QObject::killTimer(d->id);
        d->id = Qt::TimerId::Invalid;
        d->isActiveData.notify();
    }
}


/*!
  \reimp
*/
void QTimer::timerEvent(QTimerEvent *e)
{
    Q_D(QTimer);
    if (e->id() == d->id) {
        if (d->single)
            stop();
        emit timeout(QPrivateSignal());
    }
}

QAbstractEventDispatcher::Duration // statically asserts that Duration is nanoseconds
QTimer::from_msecs(std::chrono::milliseconds ms)
{
    using Duration = QAbstractEventDispatcher::Duration;

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

/*!
    \internal

    Implementation of the template version of singleShot

    \a msec is the timer interval
    \a timerType is the timer type
    \a receiver is the receiver object, can be null. In such a case, it will be the same
                as the final sender class.
    \a slotObj the slot object
*/
void QTimer::singleShotImpl(std::chrono::nanoseconds ns, Qt::TimerType timerType,
                            const QObject *receiver,
                            QtPrivate::QSlotObjectBase *slotObj)
{
    if (ns == 0ns) {
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

    (void) new QSingleShotTimer(ns, timerType, receiver, slotObj);
}

/*!
    \fn void QTimer::singleShot(int msec, const QObject *receiver, const char *member)
    \reentrant
    \deprecated [6.8] Use the chrono overloads.
    This static function calls a slot after a given time interval.

    It is very convenient to use this function because you do not need
    to bother with a \l{QObject::timerEvent()}{timerEvent} or
    create a local QTimer object.

    Example:
    \snippet code/src_corelib_kernel_qtimer.cpp 0

    This sample program automatically terminates after 10 minutes
    (600,000 milliseconds).

    The \a receiver is the receiving object and the \a member is the
    slot. The time interval is \a msec milliseconds.

    \include timers-common.qdocinc negative-intervals-not-allowed

    \sa start()
*/

/*!
    \fn void QTimer::singleShot(int msec, Qt::TimerType timerType, const QObject *receiver, const char *member)
    \overload
    \reentrant
    \deprecated [6.8] Use the chrono overloads.
    This static function calls a slot after a given time interval.

    It is very convenient to use this function because you do not need
    to bother with a \l{QObject::timerEvent()}{timerEvent} or
    create a local QTimer object.

    The \a receiver is the receiving object and the \a member is the slot. The
    time interval is \a msec milliseconds. The \a timerType affects the
    accuracy of the timer.

    \include timers-common.qdocinc negative-intervals-not-allowed

    \sa start()
*/

void QTimer::singleShot(std::chrono::nanoseconds ns, Qt::TimerType timerType,
                        const QObject *receiver, const char *member)
{
    if (ns < 0ns) {
        qWarning("QTimer::singleShot: negative intervals aren't allowed; the "
                 "interval will be set to 1ms.");
        ns = 1ms;
    }
    if (receiver && member) {
        if (ns == 0ns) {
            // special code shortpath for 0-timers
            const char* bracketPosition = strchr(member, '(');
            if (!bracketPosition || !(member[0] >= '0' && member[0] <= '2')) {
                qWarning("QTimer::singleShot: Invalid slot specification");
                return;
            }
            const auto methodName = QByteArrayView(member + 1, // extract method name
                                                   bracketPosition - 1 - member).trimmed();
            QMetaObject::invokeMethod(const_cast<QObject *>(receiver), methodName.toByteArray().constData(),
                                      Qt::QueuedConnection);
            return;
        }
        (void) new QSingleShotTimer(ns, timerType, receiver, member);
    }
}

/*! \fn template<typename Duration, typename Functor> void QTimer::singleShot(Duration interval, const QObject *context, Functor &&functor)
    \fn template<typename Duration, typename Functor> void QTimer::singleShot(Duration interval, Qt::TimerType timerType, const QObject *context, Functor &&functor)
    \fn template<typename Duration, typename Functor> void QTimer::singleShot(Duration interval, Functor &&functor)
    \fn template<typename Duration, typename Functor> void QTimer::singleShot(Duration interval, Qt::TimerType timerType, Functor &&functor)
    \since 5.4

    \reentrant
    This static function calls \a functor after \a interval.

    It is very convenient to use this function because you do not need
    to bother with a \l{QObject::timerEvent()}{timerEvent} or
    create a local QTimer object.

    If \a context is specified, then the \a functor will be called only if the
    \a context object has not been destroyed before the interval occurs. The functor
    will then be run the thread of \a context. The context's thread must have a
    running Qt event loop.

    If \a functor is a member
    function of \a context, then the function will be called on the object.

    The \a interval parameter can be an \c int (interpreted as a millisecond
    count) or a \c std::chrono type that implicitly converts to nanoseconds.

    \include timers-common.qdocinc negative-intervals-not-allowed

    \note In Qt versions prior to 6.8, the chrono overloads took chrono::milliseconds,
    not chrono::nanoseconds. The compiler will automatically convert for you,
    but the conversion may overflow for extremely large milliseconds counts.

    \sa start()
*/

/*!
    \fn void QTimer::singleShot(std::chrono::nanoseconds nsec, const QObject *receiver, const char *member)
    \since 5.8
    \overload
    \reentrant

    This static function calls a slot after a given time interval.

    It is very convenient to use this function because you do not need
    to bother with a \l{QObject::timerEvent()}{timerEvent} or
    create a local QTimer object.

    The \a receiver is the receiving object and the \a member is the slot. The
    time interval is given in the duration object \a nsec.

    \include timers-common.qdocinc negative-intervals-not-allowed

//! [qtimer-ns-overflow]
    \note In Qt versions prior to 6.8, this function took chrono::milliseconds,
    not chrono::nanoseconds. The compiler will automatically convert for you,
    but the conversion may overflow for extremely large milliseconds counts.
//! [qtimer-ns-overflow]

    \sa start()
*/

/*!
    \fn void QTimer::singleShot(std::chrono::nanoseconds nsec, Qt::TimerType timerType, const QObject *receiver, const char *member)
    \since 5.8
    \overload
    \reentrant

    This static function calls a slot after a given time interval.

    It is very convenient to use this function because you do not need
    to bother with a \l{QObject::timerEvent()}{timerEvent} or
    create a local QTimer object.

    The \a receiver is the receiving object and the \a member is the slot. The
    time interval is given in the duration object \a nsec. The \a timerType affects the
    accuracy of the timer.


    \include timers-common.qdocinc negative-intervals-not-allowed

    \include qtimer.cpp qtimer-ns-overflow

    \sa start()
*/

/*!
    \fn template <typename Functor> QMetaObject::Connection QTimer::callOnTimeout(Functor &&slot)
    \since 5.12

    Creates a connection from the timer's timeout() signal to \a slot.
    Returns a handle to the connection.

    This method is provided for convenience. It's equivalent to calling:
    \code
    QObject::connect(timer, &QTimer::timeout, timer, slot, Qt::DirectConnection);
    \endcode

    \note This overload is not available when \c {QT_NO_CONTEXTLESS_CONNECT} is
    defined, instead use the callOnTimeout() overload that takes a context object.

    \sa QObject::connect(), timeout()
*/

/*!
    \fn template <typename Functor> QMetaObject::Connection QTimer::callOnTimeout(const QObject *context, Functor &&slot, Qt::ConnectionType connectionType = Qt::AutoConnection)
    \since 5.12
    \overload callOnTimeout()

    Creates a connection from the timeout() signal to \a slot to be placed in a specific
    event loop of \a context, and returns a handle to the connection.

    This method is provided for convenience. It's equivalent to calling:
    \code
    QObject::connect(timer, &QTimer::timeout, context, slot, connectionType);
    \endcode

    \sa QObject::connect(), timeout()
*/

/*!
    \fn std::chrono::milliseconds QTimer::intervalAsDuration() const
    \since 5.8

    Returns the interval of this timer as a \c std::chrono::milliseconds object.

    \sa interval
*/

/*!
    \fn std::chrono::milliseconds QTimer::remainingTimeAsDuration() const
    \since 5.8

    Returns the time remaining in this timer object as a \c
    std::chrono::milliseconds object. If this timer is due or overdue, the
    returned value is \c std::chrono::milliseconds::zero(). If the remaining
    time could not be found or the timer is not running, this function returns a
    negative duration.

    \sa remainingTime()
*/

/*!
    \property QTimer::singleShot
    \brief whether the timer is a single-shot timer

    A single-shot timer fires only once, non-single-shot timers fire
    every \l interval milliseconds.

    The default value for this property is \c false.

    \sa interval, singleShot()
*/
void QTimer::setSingleShot(bool singleShot)
{
    d_func()->single = singleShot;
}

bool QTimer::isSingleShot() const
{
    return d_func()->single;
}

QBindable<bool> QTimer::bindableSingleShot()
{
    return QBindable<bool>(&d_func()->single);
}

/*!
    \property QTimer::interval
    \brief the timeout interval in milliseconds

    The default value for this property is 0.  A QTimer with a timeout
    interval of 0 will time out as soon as all the events in the window
    system's event queue have been processed.

    Setting the interval of a running timer will change the interval,
    stop() and then start() the timer, and acquire a new id().
    If the timer is not running, only the interval is changed.

    \include timers-common.qdocinc negative-intervals-not-allowed

    \sa singleShot
*/
void QTimer::setInterval(int msec)
{
    setInterval(std::chrono::milliseconds{msec});
}

void QTimer::setInterval(std::chrono::milliseconds interval)
{
    Q_D(QTimer);

    interval = checkInterval("QTimer::setInterval", interval);
    const int msec = interval.count();
    d->inter.removeBindingUnlessInWrapper();
    const bool intervalChanged = msec != d->inter.valueBypassingBindings();
    d->inter.setValueBypassingBindings(msec);
    if (d->isActive()) { // create new timer
        QObject::killTimer(d->id);                        // restart timer
        Qt::TimerId newId{ QObject::startTimer(msec * 1ms, d->type) };  // overflow impossible
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
        d->inter.notify();
}

int QTimer::interval() const
{
    return d_func()->inter;
}

QBindable<int> QTimer::bindableInterval()
{
    return QBindable<int>(&d_func()->inter);
}

/*!
    \property QTimer::remainingTime
    \since 5.0
    \brief the remaining time in milliseconds

    Returns the timer's remaining value in milliseconds left until the timeout.
    If the timer is inactive, the returned value will be -1. If the timer is
    overdue, the returned value will be 0.

    \sa interval
*/
int QTimer::remainingTime() const
{
    Q_D(const QTimer);
    if (d->isActive()) {
        using namespace std::chrono;
        auto remaining = QAbstractEventDispatcher::instance()->remainingTime(d->id);
        return ceil<milliseconds>(remaining).count();
    }

    return -1;
}

/*!
    \property QTimer::timerType
    \brief controls the accuracy of the timer

    The default value for this property is \c Qt::CoarseTimer.

    \sa Qt::TimerType
*/
void QTimer::setTimerType(Qt::TimerType atype)
{
    d_func()->type = atype;
}

Qt::TimerType QTimer::timerType() const
{
    return d_func()->type;
}

QBindable<Qt::TimerType> QTimer::bindableTimerType()
{
    return QBindable<Qt::TimerType>(&d_func()->type);
}

QT_END_NAMESPACE

#include "moc_qtimer.cpp"
