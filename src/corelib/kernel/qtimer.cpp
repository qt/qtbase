/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtimer.h"
#include "qabstracteventdispatcher.h"
#include "qcoreapplication.h"
#include "qobject_p.h"

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

    As a special case, a QTimer with a timeout of 0 will time out as
    soon as all the events in the window system's event queue have
    been processed. This can be used to do heavy work while providing
    a snappy user interface:

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
        {Analog Clock Example}, {Wiggly Example}
*/

static const int INV_TIMER = -1;                // invalid timer id

/*!
    Constructs a timer with the given \a parent.
*/

QTimer::QTimer(QObject *parent)
    : QObject(parent), id(INV_TIMER), inter(0), del(0), single(0), nulltimer(0), type(Qt::CoarseTimer)
{
    Q_UNUSED(del);  // ### Qt 6: remove field
}


/*!
    Destroys the timer.
*/

QTimer::~QTimer()
{
    if (id != INV_TIMER)                        // stop running timer
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

/*!
    \fn int QTimer::timerId() const

    Returns the ID of the timer if the timer is running; otherwise returns
    -1.
*/


/*! \overload start()

    Starts or restarts the timer with the timeout specified in \l interval.

    If the timer is already running, it will be
    \l{QTimer::stop()}{stopped} and restarted.

    If \l singleShot is true, the timer will be activated only once.
*/
void QTimer::start()
{
    if (id != INV_TIMER)                        // stop running timer
        stop();
    nulltimer = (!inter && single);
    id = QObject::startTimer(inter, Qt::TimerType(type));
}

/*!
    Starts or restarts the timer with a timeout interval of \a msec
    milliseconds.

    If the timer is already running, it will be
    \l{QTimer::stop()}{stopped} and restarted.

    If \l singleShot is true, the timer will be activated only once.

*/
void QTimer::start(int msec)
{
    inter = msec;
    start();
}



/*!
    Stops the timer.

    \sa start()
*/

void QTimer::stop()
{
    if (id != INV_TIMER) {
        QObject::killTimer(id);
        id = INV_TIMER;
    }
}


/*!
  \reimp
*/
void QTimer::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == id) {
        if (single)
            stop();
        emit timeout(QPrivateSignal());
    }
}

class QSingleShotTimer : public QObject
{
    Q_OBJECT
    int timerId;
    bool hasValidReceiver;
    QPointer<const QObject> receiver;
    QtPrivate::QSlotObjectBase *slotObj;
public:
    ~QSingleShotTimer();
    QSingleShotTimer(int msec, Qt::TimerType timerType, const QObject *r, const char * m);
    QSingleShotTimer(int msec, Qt::TimerType timerType, const QObject *r, QtPrivate::QSlotObjectBase *slotObj);

Q_SIGNALS:
    void timeout();
protected:
    void timerEvent(QTimerEvent *) override;
};

QSingleShotTimer::QSingleShotTimer(int msec, Qt::TimerType timerType, const QObject *r, const char *member)
    : QObject(QAbstractEventDispatcher::instance()), hasValidReceiver(true), slotObj(0)
{
    timerId = startTimer(msec, timerType);
    connect(this, SIGNAL(timeout()), r, member);
}

QSingleShotTimer::QSingleShotTimer(int msec, Qt::TimerType timerType, const QObject *r, QtPrivate::QSlotObjectBase *slotObj)
    : QObject(QAbstractEventDispatcher::instance()), hasValidReceiver(r), receiver(r), slotObj(slotObj)
{
    timerId = startTimer(msec, timerType);
    if (r && thread() != r->thread()) {
        // Avoid leaking the QSingleShotTimer instance in case the application exits before the timer fires
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &QObject::deleteLater);
        setParent(0);
        moveToThread(r->thread());
    }
}

QSingleShotTimer::~QSingleShotTimer()
{
    if (timerId > 0)
        killTimer(timerId);
    if (slotObj)
        slotObj->destroyIfLastRef();
}

void QSingleShotTimer::timerEvent(QTimerEvent *)
{
    // need to kill the timer _before_ we emit timeout() in case the
    // slot connected to timeout calls processEvents()
    if (timerId > 0)
        killTimer(timerId);
    timerId = -1;

    if (slotObj) {
        // If the receiver was destroyed, skip this part
        if (Q_LIKELY(!receiver.isNull() || !hasValidReceiver)) {
            // We allocate only the return type - we previously checked the function had
            // no arguments.
            void *args[1] = { 0 };
            slotObj->call(const_cast<QObject*>(receiver.data()), args);
        }
    } else {
        emit timeout();
    }

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
            QMetaObject::invokeMethod(const_cast<QObject *>(receiver), methodName.constData(), Qt::QueuedConnection);
            return;
        }
        (void) new QSingleShotTimer(msec, timerType, receiver, member);
    }
}

/*! \fn template<typename PointerToMemberFunction> void QTimer::singleShot(int msec, const QObject *receiver, PointerToMemberFunction method)

    \since 5.4

    \overload
    \reentrant
    This static function calls a member function of a QObject after a given time interval.

    It is very convenient to use this function because you do not need
    to bother with a \l{QObject::timerEvent()}{timerEvent} or
    create a local QTimer object.

    The \a receiver is the receiving object and the \a method is the member function. The
    time interval is \a msec milliseconds.

    If \a receiver is destroyed before the interval occurs, the method will not be called.
    The function will be run in the thread of \a receiver. The receiver's thread must have
    a running Qt event loop.

    \sa start()
*/

/*! \fn template<typename PointerToMemberFunction> void QTimer::singleShot(int msec, Qt::TimerType timerType, const QObject *receiver, PointerToMemberFunction method)

    \since 5.4

    \overload
    \reentrant
    This static function calls a member function of a QObject after a given time interval.

    It is very convenient to use this function because you do not need
    to bother with a \l{QObject::timerEvent()}{timerEvent} or
    create a local QTimer object.

    The \a receiver is the receiving object and the \a method is the member function. The
    time interval is \a msec milliseconds. The \a timerType affects the
    accuracy of the timer.

    If \a receiver is destroyed before the interval occurs, the method will not be called.
    The function will be run in the thread of \a receiver. The receiver's thread must have
    a running Qt event loop.

    \sa start()
*/

/*! \fn template<typename Functor> void QTimer::singleShot(int msec, Functor functor)

    \since 5.4

    \overload
    \reentrant
    This static function calls \a functor after a given time interval.

    It is very convenient to use this function because you do not need
    to bother with a \l{QObject::timerEvent()}{timerEvent} or
    create a local QTimer object.

    The time interval is \a msec milliseconds.

    \sa start()
*/

/*! \fn template<typename Functor> void QTimer::singleShot(int msec, Qt::TimerType timerType, Functor functor)

    \since 5.4

    \overload
    \reentrant
    This static function calls \a functor after a given time interval.

    It is very convenient to use this function because you do not need
    to bother with a \l{QObject::timerEvent()}{timerEvent} or
    create a local QTimer object.

    The time interval is \a msec milliseconds. The \a timerType affects the
    accuracy of the timer.

    \sa start()
*/

/*! \fn template<typename Functor> void QTimer::singleShot(int msec, const QObject *context, Functor functor)

    \since 5.4

    \overload
    \reentrant
    This static function calls \a functor after a given time interval.

    It is very convenient to use this function because you do not need
    to bother with a \l{QObject::timerEvent()}{timerEvent} or
    create a local QTimer object.

    The time interval is \a msec milliseconds.

    If \a context is destroyed before the interval occurs, the method will not be called.
    The function will be run in the thread of \a context. The context's thread must have
    a running Qt event loop.

    \sa start()
*/

/*! \fn template<typename Functor> void QTimer::singleShot(int msec, Qt::TimerType timerType, const QObject *context, Functor functor)

    \since 5.4

    \overload
    \reentrant
    This static function calls \a functor after a given time interval.

    It is very convenient to use this function because you do not need
    to bother with a \l{QObject::timerEvent()}{timerEvent} or
    create a local QTimer object.

    The time interval is \a msec milliseconds. The \a timerType affects the
    accuracy of the timer.

    If \a context is destroyed before the interval occurs, the method will not be called.
    The function will be run in the thread of \a context. The context's thread must have
    a running Qt event loop.

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
    \fn void QTimer::start(std::chrono::milliseconds msec)
    \since 5.8
    \overload

    Starts or restarts the timer with a timeout of duration \a msec milliseconds.

    If the timer is already running, it will be
    \l{QTimer::stop()}{stopped} and restarted.

    If \l singleShot is true, the timer will be activated only once.
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
    inter = msec;
    if (id != INV_TIMER) {                        // create new timer
        QObject::killTimer(id);                        // restart timer
        id = QObject::startTimer(msec, Qt::TimerType(type));
    }
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
    if (id != INV_TIMER) {
        return QAbstractEventDispatcher::instance()->remainingTime(id);
    }

    return -1;
}

/*!
    \property QTimer::timerType
    \brief controls the accuracy of the timer

    The default value for this property is \c Qt::CoarseTimer.

    \sa Qt::TimerType
*/

QT_END_NAMESPACE

#include "qtimer.moc"
#include "moc_qtimer.cpp"
