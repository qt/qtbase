// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtimer.h"
#include "qtimer_p.h"

#include "qabstracteventdispatcher.h"
#include "qcoreapplication.h"
#include "qcoreapplication_p.h"
#include "qdeadlinetimer.h"
#include "qmetaobject_p.h"
#include "qobject_p.h"
#include "qproperty_p.h"
#include "qthread.h"

QT_BEGIN_NAMESPACE

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

    An alternative to using QTimer is to call QObject::startTimer()
    for your object and reimplement the QObject::timerEvent() event
    handler in your class (which must inherit QObject). The
    disadvantage is that timerEvent() does not support such
    high-level features as single-shot timers or signals.

    Another alternative is QBasicTimer. It is typically less
    cumbersome than using QObject::startTimer()
    directly. See \l{Timers} for an overview of all three approaches.

    Some operating systems limit the number of timers that may be
    used; Qt tries to work around these limitations.

    \sa QBasicTimer, QTimerEvent, QObject::timerEvent(), Timers,
        {Analog Clock}
*/

/*!
    Constructs a timer with the given \a parent.
*/

QTimer::QTimer(QObject *parent)
    : QObject(*new QTimerPrivate, parent)
{
}


/*!
    Destroys the timer.
*/

QTimer::~QTimer()
{
    if (d_func()->id != QTimerPrivate::INV_TIMER) // stop running timer
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

    Returns \c true if the timer is running (pending); otherwise returns
    false.
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
    return d_func()->id;
}


/*! \overload start()

    Starts or restarts the timer with the timeout specified in \l interval.

    If the timer is already running, it will be
    \l{QTimer::stop()}{stopped} and restarted.

    If \l singleShot is true, the timer will be activated only once.
*/
void QTimer::start()
{
    Q_D(QTimer);
    if (d->id != QTimerPrivate::INV_TIMER) // stop running timer
        stop();
    d->id = QObject::startTimer(std::chrono::milliseconds{d->inter}, d->type);
    d->isActiveData.notify();
}

/*!
    Starts or restarts the timer with a timeout interval of \a msec
    milliseconds.

    If the timer is already running, it will be
    \l{QTimer::stop()}{stopped} and restarted.

    If \l singleShot is true, the timer will be activated only once. This is
    equivalent to:

    \code
        timer.setInterval(msec);
        timer.start();
    \endcode

    \note   Keeping the event loop busy with a zero-timer is bound to
            cause trouble and highly erratic behavior of the UI.
*/
void QTimer::start(int msec)
{
    Q_D(QTimer);
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
    if (d->id != QTimerPrivate::INV_TIMER) {
        QObject::killTimer(d->id);
        d->id = QTimerPrivate::INV_TIMER;
        d->isActiveData.notify();
    }
}


/*!
  \reimp
*/
void QTimer::timerEvent(QTimerEvent *e)
{
    Q_D(QTimer);
    if (e->timerId() == d->id) {
        if (d->single)
            stop();
        emit timeout(QPrivateSignal());
    }
}

class QSingleShotTimer : public QObject
{
    Q_OBJECT
    int timerId = -1;
public:
    ~QSingleShotTimer();
    QSingleShotTimer(int msec, Qt::TimerType timerType, const QObject *r, const char * m);
    QSingleShotTimer(int msec, Qt::TimerType timerType, const QObject *r, QtPrivate::QSlotObjectBase *slotObj);

    void startTimerForReceiver(int msec, Qt::TimerType timerType, const QObject *receiver);

Q_SIGNALS:
    void timeout();
protected:
    void timerEvent(QTimerEvent *) override;
};

QSingleShotTimer::QSingleShotTimer(int msec, Qt::TimerType timerType, const QObject *r, const char *member)
    : QObject(QAbstractEventDispatcher::instance())
{
    connect(this, SIGNAL(timeout()), r, member);

    startTimerForReceiver(msec, timerType, r);
}

QSingleShotTimer::QSingleShotTimer(int msec, Qt::TimerType timerType, const QObject *r, QtPrivate::QSlotObjectBase *slotObj)
    : QObject(QAbstractEventDispatcher::instance())
{
    int signal_index = QMetaObjectPrivate::signalOffset(&staticMetaObject);
    Q_ASSERT(QMetaObjectPrivate::signal(&staticMetaObject, signal_index).name() == "timeout");
    QObjectPrivate::connectImpl(this, signal_index, r ? r : this, nullptr, slotObj,
                                Qt::AutoConnection, nullptr, &staticMetaObject);

    startTimerForReceiver(msec, timerType, r);
}

QSingleShotTimer::~QSingleShotTimer()
{
    if (timerId > 0)
        killTimer(timerId);
}

/*
    Move the timer, and the dispatching and handling of the timer event, into
    the same thread as where it will be handled, so that it fires reliably even
    if the thread that set up the timer is busy.
*/
void QSingleShotTimer::startTimerForReceiver(int msec, Qt::TimerType timerType, const QObject *receiver)
{
    if (receiver && receiver->thread() != thread()) {
        // Avoid leaking the QSingleShotTimer instance in case the application exits before the timer fires
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &QObject::deleteLater);
        setParent(nullptr);
        moveToThread(receiver->thread());

        QDeadlineTimer deadline(std::chrono::milliseconds{msec}, timerType);
        QMetaObject::invokeMethod(this, [this, deadline, timerType]{
            if (deadline.hasExpired())
                emit timeout();
            else
                timerId = startTimer(std::chrono::milliseconds{deadline.remainingTime()}, timerType);
        }, Qt::QueuedConnection);
    } else {
        timerId = startTimer(std::chrono::milliseconds{msec}, timerType);
    }
}


void QSingleShotTimer::timerEvent(QTimerEvent *)
{
    // need to kill the timer _before_ we emit timeout() in case the
    // slot connected to timeout calls processEvents()
    if (timerId > 0)
        killTimer(timerId);
    timerId = -1;

    emit timeout();

    // we would like to use delete later here, but it feels like a
    // waste to post a new event to handle this event, so we just unset the flag
    // and explicitly delete...
    qDeleteInEventHandler(this);
}

/*!
    \internal

    Implementation of the template version of singleShot

    \a msec is the timer interval
    \a timerType is the timer type
    \a receiver is the receiver object, can be null. In such a case, it will be the same
                as the final sender class.
    \a slot a pointer only used when using Qt::UniqueConnection
    \a slotObj the slot object
 */
void QTimer::singleShotImpl(int msec, Qt::TimerType timerType,
                            const QObject *receiver,
                            QtPrivate::QSlotObjectBase *slotObj)
{
    if (msec == 0) {
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

        QMetaObject::invokeMethodImpl(const_cast<QObject *>(receiver), slotObj,
                                      Qt::QueuedConnection, nullptr);

        if (deleteReceiver)
            const_cast<QObject *>(receiver)->deleteLater();
        return;
    }

    new QSingleShotTimer(msec, timerType, receiver, slotObj);
}

/*!
    \reentrant
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

    \sa start()
*/

void QTimer::singleShot(int msec, const QObject *receiver, const char *member)
{
    // coarse timers are worst in their first firing
    // so we prefer a high precision timer for something that happens only once
    // unless the timeout is too big, in which case we go for coarse anyway
    singleShot(msec, msec >= 2000 ? Qt::CoarseTimer : Qt::PreciseTimer, receiver, member);
}

/*! \overload
    \reentrant
    This static function calls a slot after a given time interval.

    It is very convenient to use this function because you do not need
    to bother with a \l{QObject::timerEvent()}{timerEvent} or
    create a local QTimer object.

    The \a receiver is the receiving object and the \a member is the slot. The
    time interval is \a msec milliseconds. The \a timerType affects the
    accuracy of the timer.

    \sa start()
*/
void QTimer::singleShot(int msec, Qt::TimerType timerType, const QObject *receiver, const char *member)
{
    if (Q_UNLIKELY(msec < 0)) {
        qWarning("QTimer::singleShot: Timers cannot have negative timeouts");
        return;
    }
    if (receiver && member) {
        if (msec == 0) {
            // special code shortpath for 0-timers
            const char* bracketPosition = strchr(member, '(');
            if (!bracketPosition || !(member[0] >= '0' && member[0] <= '2')) {
                qWarning("QTimer::singleShot: Invalid slot specification");
                return;
            }
            QByteArray methodName(member+1, bracketPosition - 1 - member); // extract method name
            QMetaObject::invokeMethod(const_cast<QObject *>(receiver), methodName.trimmed().constData(),
                                      Qt::QueuedConnection);
            return;
        }
        (void) new QSingleShotTimer(msec, timerType, receiver, member);
    }
}

/*! \fn template<typename Duration, typename Functor> void QTimer::singleShot(Duration msec, const QObject *context, Functor &&functor)
    \fn template<typename Duration, typename Functor> void QTimer::singleShot(Duration msec, Qt::TimerType timerType, const QObject *context, Functor &&functor)
    \fn template<typename Duration, typename Functor> void QTimer::singleShot(Duration msec, Functor &&functor)
    \fn template<typename Duration, typename Functor> void QTimer::singleShot(Duration msec, Qt::TimerType timerType, Functor &&functor)
    \since 5.4

    \reentrant
    This static function calls \a functor after \a msec milliseconds.

    It is very convenient to use this function because you do not need
    to bother with a \l{QObject::timerEvent()}{timerEvent} or
    create a local QTimer object.

    If \a context is specified, then the \a functor will be called only if the
    \a context object has not been destroyed before the interval occurs. The functor
    will then be run the thread of \a context. The context's thread must have a
    running Qt event loop.

    If \a functor is a member
    function of \a context, then the function will be called on the object.

    The \a msec parameter can be an \c int or a \c std::chrono::milliseconds value.

    \sa start()
*/

/*!
    \fn void QTimer::singleShot(std::chrono::milliseconds msec, const QObject *receiver, const char *member)
    \since 5.8
    \overload
    \reentrant

    This static function calls a slot after a given time interval.

    It is very convenient to use this function because you do not need
    to bother with a \l{QObject::timerEvent()}{timerEvent} or
    create a local QTimer object.

    The \a receiver is the receiving object and the \a member is the slot. The
    time interval is given in the duration object \a msec.

    \sa start()
*/

/*!
    \fn void QTimer::singleShot(std::chrono::milliseconds msec, Qt::TimerType timerType, const QObject *receiver, const char *member)
    \since 5.8
    \overload
    \reentrant

    This static function calls a slot after a given time interval.

    It is very convenient to use this function because you do not need
    to bother with a \l{QObject::timerEvent()}{timerEvent} or
    create a local QTimer object.

    The \a receiver is the receiving object and the \a member is the slot. The
    time interval is given in the duration object \a msec. The \a timerType affects the
    accuracy of the timer.

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

    \sa QObject::connect(), timeout()
*/

/*!
    \fn template <typename Functor> QMetaObject::Connection QTimer::callOnTimeout(const QObject *context, Functor &&slot, Qt::ConnectionType connectionType = Qt::AutoConnection)
    \since 5.12
    \overload callOnTimeout()

    Creates a connection from the timeout() signal to \a slot to be placed in a specific
    event loop of \a context, and returns a handle to the connection.

    This method is provided for convenience. It's equivalent to calling
    \c {QObject::connect(timer, &QTimer::timeout, context, slot, connectionType)}.

    \sa QObject::connect(), timeout()
*/

/*!
    \fn void QTimer::start(std::chrono::milliseconds msec)
    \since 5.8
    \overload

    Starts or restarts the timer with a timeout of duration \a msec milliseconds.

    If the timer is already running, it will be
    \l{QTimer::stop()}{stopped} and restarted.

    If \l singleShot is true, the timer will be activated only once. This is
    equivalent to:

    \code
        timer.setInterval(msec);
        timer.start();
    \endcode
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
    time could not be found or the timer is not active, this function returns a
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

    Setting the interval of an active timer changes its timerId().

    \sa singleShot
*/
void QTimer::setInterval(int msec)
{
    Q_D(QTimer);
    d->inter.removeBindingUnlessInWrapper();
    const bool intervalChanged = msec != d->inter.valueBypassingBindings();
    d->inter.setValueBypassingBindings(msec);
    if (d->id != QTimerPrivate::INV_TIMER) { // create new timer
        QObject::killTimer(d->id);                        // restart timer
        d->id = QObject::startTimer(std::chrono::milliseconds{msec}, d->type);
        // No need to call markDirty() for d->isActiveData here,
        // as timer state actually does not change
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
    if (d->id != QTimerPrivate::INV_TIMER) {
        return QAbstractEventDispatcher::instance()->remainingTime(d->id);
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

#include "qtimer.moc"
#include "moc_qtimer.cpp"
