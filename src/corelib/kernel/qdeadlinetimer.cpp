/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeadlinetimer.h"
#include "qdeadlinetimer_p.h"
#include <qpair.h>

QT_BEGIN_NAMESPACE

Q_DECL_CONST_FUNCTION static inline QPair<qint64, qint64> toSecsAndNSecs(qint64 nsecs)
{
    qint64 secs = nsecs / (1000*1000*1000);
    if (nsecs < 0)
        --secs;
    nsecs -= secs * 1000*1000*1000;
    return qMakePair(secs, nsecs);
}

/*!
    \class QDeadlineTimer
    \inmodule QtCore
    \brief The QDeadlineTimer class marks a deadline in the future.
    \since 5.8

    \reentrant
    \ingroup tools

    The QDeadlineTimer class is usually used to calculate future deadlines and
    verify whether the deadline has expired. QDeadlineTimer can also be used
    for deadlines without expiration ("forever"). It forms a counterpart to
    QElapsedTimer, which calculates how much time has elapsed since
    QElapsedTimer::start() was called.

    QDeadlineTimer provides a more convenient API compared to
    QElapsedTimer::hasExpired().

    The typical use-case for the class is to create a QDeadlineTimer before the
    operation in question is started, and then use remainingTime() or
    hasExpired() to determine whether to continue trying the operation.
    QDeadlineTimer objects can be passed to functions being called to execute
    this operation so they know how long to still operate.

    \code
    void executeOperation(int msecs)
    {
        QDeadlineTimer deadline(msecs);
        do {
            if (readFromDevice(deadline.remainingTime())
                break;
            waitForReadyRead(deadline);
        } while (!deadline.hasExpired());
    }
    \endcode

    Many QDeadlineTimer functions deal with time out values, which all are
    measured in milliseconds. There are two special values, the same as many
    other Qt functions named \c{waitFor} or similar:

    \list
      \li 0: no time left, expired
      \li -1: infinite time left, timer never expires
    \endlist

    \section1 Reference Clocks

    QDeadlineTimer will use the same clock as QElapsedTimer (see
    QElapsedTimer::clockType() and QElapsedTimer::isMonotonic()).

    \section1 Timer types

    Like QTimer, QDeadlineTimer can select among different levels of coarseness
    on the timers. You can select precise timing by passing Qt::PreciseTimer to
    the functions that set of change the timer, or you can select coarse timing
    by passing Qt::CoarseTimer. Qt::VeryCoarseTimer is currently interpreted
    the same way as Qt::CoarseTimer.

    This feature is dependent on support from the operating system: if the OS
    does not support a coarse timer functionality, then QDeadlineTimer will
    behave like Qt::PreciseTimer was passed.

    QDeadlineTimer defaults to Qt::CoarseTimer because on operating systems
    that do support coarse timing, making timing calls to that clock source is
    often much more efficient. The level of coarseness depends on the
    operating system, but should be in the order of a couple of milliseconds.

    \section1 \c{std::chrono} Compatibility

    QDeadlineTimer is compatible with the \c{std::chrono} API from C++11 and
    can be constructed from or compared to both \c{std::chrono::duration} and
    \c{std::chrono::time_point} objects. In addition, it is fully compatible
    with the time literals from C++14, which allow one to write code as:

    \code
        using namespace std::chrono;
        using namespace std::chrono_literals;

        QDeadlineTimer deadline(30s);
        device->waitForReadyRead(deadline);
        if (deadline.remainingTime<nanoseconds>() > 300ms)
            cleanup();
    \endcode

    As can be seen in the example above, QDeadlineTimer offers a templated
    version of remainingTime() and deadline() that can be used to return
    \c{std::chrono} objects.

    Note that comparing to \c{time_point} is not as efficient as comparing to
    \c{duration}, since QDeadlineTimer may need to convert from its own
    internal clock source to the clock source used by the \c{time_point} object.
    Also note that, due to this conversion, the deadlines will not be precise,
    so the following code is not expected to compare equally:

    \code
        using namespace std::chrono;
        using namespace std::chrono_literals;
        auto now = steady_clock::now();
        QDeadlineTimer deadline(now + 1s);
        Q_ASSERT(deadline == now + 1s);
    \endcode

    \sa QTime, QTimer, QDeadlineTimer, Qt::TimerType
*/

/*!
    \enum QDeadlineTimer::ForeverConstant

    \value Forever      Used when creating a QDeadlineTimer to indicate the
                        deadline should not expire
*/

/*!
    \fn QDeadlineTimer::QDeadlineTimer(Qt::TimerType timerType)

    Constructs an expired QDeadlineTimer object. For this object,
    remainingTime() will return 0.

    The timer type \a timerType may be ignored, since the timer is already
    expired. Similarly, for optimization purposes, this function will not
    attempt to obtain the current time and will use a value known to be in the
    past. Therefore, deadline() may return an unexpected value and this object
    cannot be used in calculation of how long it is overdue. If that
    functionality is required, use QDeadlineTimer::current().

    \sa hasExpired(), remainingTime(), Qt::TimerType, current()
*/

/*!
    \fn QDeadlineTimer::QDeadlineTimer(ForeverConstant foreverConstant, Qt::TimerType timerType)

    QDeadlineTimer objects created with parameter \a foreverConstant never expire.
    For such objects, remainingTime() will return -1, deadline() will return the
    maximum value, and isForever() will return true.

    The timer type \a timerType may be ignored, since the timer is already
    expired.

    \sa ForeverConstant, hasExpired(), isForever(), remainingTime(), timerType()
*/

/*!
    Constructs a QDeadlineTimer object with an expiry time of \a msecs msecs
    from the moment of the creation of this object, if msecs is positive. If \a
    msecs is zero, this QDeadlineTimer will be marked as expired, causing
    remainingTime() to return zero and deadline() to return an indeterminate
    time point in the past. If \a msecs is -1, the timer will be set it to
    never expire, causing remainingTime() to return -1 and deadline() to return
    the maximum value.

    The QDeadlineTimer object will be constructed with the specified timer \a type.

    For optimization purposes, if \a msecs is zero, this function may skip
    obtaining the current time and may instead use a value known to be in the
    past. If that happens, deadline() may return an unexpected value and this
    object cannot be used in calculation of how long it is overdue. If that
    functionality is required, use QDeadlineTimer::current() and add time to
    it.

    \sa hasExpired(), isForever(), remainingTime(), setRemainingTime()
*/
QDeadlineTimer::QDeadlineTimer(qint64 msecs, Qt::TimerType type) Q_DECL_NOTHROW
    : t2(0)
{
    setRemainingTime(msecs, type);
}

/*!
    \fn QDeadlineTimer::QDeadlineTimer(std::chrono::time_point<Clock, Duration> deadline, Qt::TimerType type)

    Constructs a QDeadlineTimer object with a deadline at \a deadline time
    point, converting from the clock source \c{Clock} to Qt's internal clock
    source (see QElapsedTimer::clcokType()).

    If \a deadline is in the past, this QDeadlineTimer object is set to
    expired, whereas if \a deadline is equal to \c{Duration::max()}, then this
    object is set to never expire.

    The QDeadlineTimer object will be constructed with the specified timer \a type.

    \sa hasExpired(), isForever(), remainingTime(), setDeadline()
*/

/*!
    \fn QDeadlineTimer::QDeadlineTimer(std::chrono::duration<Rep, Period> remaining, Qt::TimerType type)

    Constructs a QDeadlineTimer object with a remaining time of \a remaining.
    If \a remaining is zero or negative, this QDeadlineTimer object will be
    mark as expired, whereas if \a remaining is equal to \c{duration::max()},
    the object will be set to never expire.

    The QDeadlineTimer object will be constructed with the specified timer \a type.

    This constructor can be used with C++14's user-defined literals for time, such as in:

    \code
        using namespace std::chrono_literals;
        QDeadlineTimer deadline(250ms);
    \endcode

    For optimization purposes, if \a remaining is zero or negative, this
    function may skip obtaining the current time and may instead use a value
    known to be in the past. If that happens, deadline() may return an
    unexpected value and this object cannot be used in calculation of how long
    it is overdue. If that functionality is required, use
    QDeadlineTimer::current() and add time to it.

    \sa hasExpired(), isForever(), remainingTime(), setRemainingTime()
*/

/*!
    \fn void QDeadlineTimer::setDeadline(std::chrono::time_point<Clock, Duration> deadline, Qt::TimerType type)

    Sets this QDeadlineTimer to the deadline marked by \a deadline time
    point, converting from the clock source \c{Clock} to Qt's internal clock
    source (see QElapsedTimer::clcokType()).

    If \a deadline is in the past, this QDeadlineTimer object is set to
    expired, whereas if \a deadline is equal to \c{Duration::max()}, then this
    object is set to never expire.

    The timer type for this QDeadlineTimer object will be set to the specified \a type.

    \sa hasExpired(), isForever(), remainingTime(),
*/

/*!
    Sets the remaining time for this QDeadlineTimer object to \a msecs
    milliseconds from now, if \a msecs has a positive value. If \a msecs is
    zero, this QDeadlineTimer object will be marked as expired, whereas a value
    of -1 will set it to never expire.

    The timer type for this QDeadlineTimer object will be set to the specified \a timerType.

    \sa setPreciseRemainingTime(), hasExpired(), isForever(), remainingTime()
*/
void QDeadlineTimer::setRemainingTime(qint64 msecs, Qt::TimerType timerType) Q_DECL_NOTHROW
{
    if (msecs == -1)
        *this = QDeadlineTimer(Forever, timerType);
    else
        setPreciseRemainingTime(0, msecs * 1000 * 1000, timerType);
}

/*!
    Sets the remaining time for this QDeadlineTimer object to \a secs seconds
    plus \a nsecs nanoseconds from now, if \a secs has a positive value. If \a
    secs is -1, this QDeadlineTimer will be set it to never expire. If both
    parameters are zero, this QDeadlineTimer will be marked as expired.

    The timer type for this QDeadlineTimer object will be set to the specified
    \a timerType.

    \sa setRemainingTime(), hasExpired(), isForever(), remainingTime()
*/
void QDeadlineTimer::setPreciseRemainingTime(qint64 secs, qint64 nsecs, Qt::TimerType timerType) Q_DECL_NOTHROW
{
    if (secs == -1) {
        *this = QDeadlineTimer(Forever, timerType);
        return;
    }

    *this = current(timerType);
    if (QDeadlineTimerNanosecondsInT2) {
        t1 += secs + toSecsAndNSecs(nsecs).first;
        t2 += toSecsAndNSecs(nsecs).second;
        if (t2 > 1000*1000*1000) {
            t2 -= 1000*1000*1000;
            ++t1;
        }
    } else {
        t1 += secs * 1000 * 1000 * 1000 + nsecs;
    }
}

/*!
    \overload
    \fn void QDeadlineTimer::setRemainingTime(std::chrono::duration<Rep, Period> remaining, Qt::TimerType type)

    Sets the remaining time for this QDeadlineTimer object to \a remaining. If
    \a remaining is zero or negative, this QDeadlineTimer object will be mark
    as expired, whereas if \a remaining is equal to \c{duration::max()}, the
    object will be set to never expire.

    The timer type for this QDeadlineTimer object will be set to the specified \a type.

    This function can be used with C++14's user-defined literals for time, such as in:

    \code
        using namespace std::chrono_literals;
        deadline.setRemainingTime(250ms);
    \endcode

    \sa setDeadline(), remainingTime(), hasExpired(), isForever()
*/

/*!
    \fn std::chrono::nanoseconds remainingTimeAsDuration() const

    Returns a \c{std::chrono::duration} object of type \c{Duration} containing
    the remaining time in this QDeadlineTimer, if it still has time left. If
    the deadline has passed, this returns \c{Duration::zero()}, whereas if the
    object is set to never expire, it returns \c{Duration::max()} (instead of
    -1).

    It is not possible to obtain the overdue time for expired timers with this
    function. To do that, see deadline().

    \note The overload of this function without template parameter always
    returns milliseconds.

    \sa setRemainingTime(), deadline<Clock, Duration>()
*/

/*!
    \overload
    \fn std::chrono::time_point<Clock, Duration> QDeadlineTimer::deadline() const

    Returns the absolute time point for the deadline stored in QDeadlineTimer
    object as a \c{std::chrono::time_point} object. The template parameter
    \c{Clock} is mandatory and indicates which of the C++ timekeeping clocks to
    use as a reference. The value will be in the past if this QDeadlineTimer
    has expired.

    If this QDeadlineTimer never expires, this function returns
    \c{std::chrono::time_point<Clock, Duration>::max()}.

    This function can be used to calculate the amount of time a timer is
    overdue, by subtracting the current time point of the reference clock, as
    in the following example:

    \code
        auto realTimeLeft = std::chrono::nanoseconds::max();
        auto tp = deadline.deadline<std::chrono::steady_clock>();
        if (tp != std::chrono::steady_clock::max())
            realTimeLeft = tp - std::chrono::steady_clock::now();
    \endcode

    \note Timers that were created as expired have an indetermine time point in
    the past as their deadline, so the above calculation may not work.

    \sa remainingTime(), deadlineNSecs(), setDeadline()
*/

/*!
    \fn bool QDeadlineTimer::isForever() const

    Returns true if this QDeadlineTimer object never expires, false otherwise.
    For timers that never expire, remainingTime() always returns -1 and
    deadline() returns the maximum value.

    \sa ForeverConstant, hasExpired(), remainingTime()
*/

/*!
    Returns true if this QDeadlineTimer object has expired, false if there
    remains time left. For objects that have expired, remainingTime() will
    return zero and deadline() will return a time point in the past.

    QDeadlineTimer objects created with the \l {ForeverConstant} never expire
    and this function always returns false for them.

    \sa isForever(), remainingTime()
*/
bool QDeadlineTimer::hasExpired() const Q_DECL_NOTHROW
{
    if (isForever())
        return false;
    return *this <= current(timerType());
}

/*!
    \fn Qt::TimerType QDeadlineTimer::timerType() const

    Returns the timer type is active for this object.

    \sa setTimerType()
*/

/*!
    Changes the timer type for this object to \a timerType.

    The behavior for each possible value of \a timerType is operating-system
    dependent. Qt::PreciseTimer will use the most precise timer that Qt can
    find, with resolution of 1 millisecond or better, whereas QDeadlineTimer
    will try to use a more coarse timer for Qt::CoarseTimer and
    Qt::VeryCoarseTimer.

    \sa Qt::TimerType
 */
void QDeadlineTimer::setTimerType(Qt::TimerType timerType)
{
    type = timerType;
}

/*!
    Returns the remaining time in this QDeadlineTimer object in milliseconds.
    If the timer has already expired, this function will return zero and it is
    not possible to obtain the amount of time overdue with this function (to do
    that, see deadline()). If the timer was set to never expire, this function
    returns -1.

    This function is suitable for use in Qt APIs that take a millisecond
    timeout, such as the many \l QIODevice \c waitFor functions or the timed
    lock functions in \l QMutex, \l QWaitCondition, \l QSemaphore, or
    \l QReadWriteLock. For example:

    \code
        mutex.tryLock(deadline.remainingTime());
    \endcode

    \sa remainingTimeNSecs(), isForever(), hasExpired()
*/
qint64 QDeadlineTimer::remainingTime() const Q_DECL_NOTHROW
{
    qint64 ns = remainingTimeNSecs();
    return ns <= 0 ? ns : ns / (1000 * 1000);
}

/*!
    Returns the remaining time in this QDeadlineTimer object in nanoseconds. If
    the timer has already expired, this function will return zero and it is not
    possible to obtain the amount of time overdue with this function. If the
    timer was set to never expire, this function returns -1.

    \sa remainingTime(), isForever(), hasExpired()
*/
qint64 QDeadlineTimer::remainingTimeNSecs() const Q_DECL_NOTHROW
{
    if (isForever())
        return -1;
    qint64 raw = rawRemainingTimeNSecs();
    return raw < 0 ? 0 : raw;
}

/*!
    \internal
    Same as remainingTimeNSecs, but may return negative remaining times. Does
    not deal with Forever.
*/
qint64 QDeadlineTimer::rawRemainingTimeNSecs() const Q_DECL_NOTHROW
{
    QDeadlineTimer now = current(timerType());
    if (QDeadlineTimerNanosecondsInT2)
        return (t1 - now.t1) * (1000*1000*1000) + t2 - now.t2;
    return t1 - now.t1;
}

/*!
    Returns the absolute time point for the deadline stored in QDeadlineTimer
    object, calculated in milliseconds relative to the reference clock, the
    same as QElapsedTimer::msecsSinceReference(). The value will be in the past
    if this QDeadlineTimer has expired.

    If this QDeadlineTimer never expires, this function returns
    \c{std::numeric_limits<qint64>::max()}.

    This function can be used to calculate the amount of time a timer is
    overdue, by subtracting QDeadlineTimer::current() or
    QElapsedTimer::msecsSinceReference(), as in the following example:

    \code
        qint64 realTimeLeft = deadline.deadline();
        if (realTimeLeft != (std::numeric_limits<qint64>::max)()) {
            realTimeLeft -= QDeadlineTimer::current().deadline();
            // or:
            //QElapsedTimer timer;
            //timer.start();
            //realTimeLeft -= timer.msecsSinceReference();
        }
    \endcode

    \note Timers that were created as expired have an indetermine time point in
    the past as their deadline, so the above calculation may not work.

    \sa remainingTime(), deadlineNSecs(), setDeadline()
*/
qint64 QDeadlineTimer::deadline() const Q_DECL_NOTHROW
{
    if (isForever())
        return t1;
    return deadlineNSecs() / (1000 * 1000);
}

/*!
    Returns the absolute time point for the deadline stored in QDeadlineTimer
    object, calculated in nanoseconds relative to the reference clock, the
    same as QElapsedTimer::msecsSinceReference(). The value will be in the past
    if this QDeadlineTimer has expired.

    If this QDeadlineTimer never expires, this function returns
    \c{std::numeric_limits<qint64>::max()}.

    This function can be used to calculate the amount of time a timer is
    overdue, by subtracting QDeadlineTimer::current(), as in the following
    example:

    \code
        qint64 realTimeLeft = deadline.deadlineNSecs();
        if (realTimeLeft != std::numeric_limits<qint64>::max())
            realTimeLeft -= QDeadlineTimer::current().deadlineNSecs();
    \endcode

    \note Timers that were created as expired have an indetermine time point in
    the past as their deadline, so the above calculation may not work.

    \sa remainingTime(), deadlineNSecs()
*/
qint64 QDeadlineTimer::deadlineNSecs() const Q_DECL_NOTHROW
{
    if (isForever())
        return t1;
    if (QDeadlineTimerNanosecondsInT2)
        return t1 * 1000 * 1000 * 1000 + t2;
    return t1;
}

/*!
    Sets the deadline for this QDeadlineTimer object to be the \a msecs
    absolute time point, counted in milliseconds since the reference clock (the
    same as QElapsedTimer::msecsSinceReference()), and the timer type to \a
    timerType. If the value is in the past, this QDeadlineTimer will be marked
    as expired.

    If \a msecs is \c{std::numeric_limits<qint64>::max()}, this QDeadlineTimer
    will be set to never expire.

    \sa setPreciseDeadline(), deadline(), deadlineNSecs(), setRemainingTime()
*/
void QDeadlineTimer::setDeadline(qint64 msecs, Qt::TimerType timerType) Q_DECL_NOTHROW
{
    if (msecs == (std::numeric_limits<qint64>::max)()) {
        setPreciseDeadline(msecs, 0, timerType);    // msecs == MAX implies Forever
    } else {
        setPreciseDeadline(msecs / 1000, msecs % 1000 * 1000 * 1000, timerType);
    }
}

/*!
    Sets the deadline for this QDeadlineTimer object to be \a secs seconds and
    \a nsecs nanoseconds since the reference clock epoch (the same as
    QElapsedTimer::msecsSinceReference()), and the timer type to \a timerType.
    If the value is in the past, this QDeadlineTimer will be marked as expired.

    If \a secs or \a nsecs is \c{std::numeric_limits<qint64>::max()}, this
    QDeadlineTimer will be set to never expire. If \a nsecs is more than 1
    billion nanoseconds (1 second), then \a secs will be adjusted accordingly.

    \sa setDeadline(), deadline(), deadlineNSecs(), setRemainingTime()
*/
void QDeadlineTimer::setPreciseDeadline(qint64 secs, qint64 nsecs, Qt::TimerType timerType) Q_DECL_NOTHROW
{
    type = timerType;
    if (secs == (std::numeric_limits<qint64>::max)() || nsecs == (std::numeric_limits<qint64>::max)()) {
        *this = QDeadlineTimer(Forever, timerType);
    } else if (QDeadlineTimerNanosecondsInT2) {
        t1 = secs + toSecsAndNSecs(nsecs).first;
        t2 = toSecsAndNSecs(nsecs).second;
    } else {
        t1 = secs * (1000*1000*1000) + nsecs;
    }
}

/*!
    Returns a QDeadlineTimer object whose deadline is extended from \a dt's
    deadline by \a nsecs nanoseconds. If \a dt was set to never expire, this
    function returns a QDeadlineTimer that will not expire either.

    \note if \a dt was created as expired, its deadline is indeterminate and
    adding an amount of time may or may not cause it to become unexpired.
*/
QDeadlineTimer QDeadlineTimer::addNSecs(QDeadlineTimer dt, qint64 nsecs) Q_DECL_NOTHROW
{
    if (dt.isForever() || nsecs == (std::numeric_limits<qint64>::max)()) {
        dt = QDeadlineTimer(Forever, dt.timerType());
    } else if (QDeadlineTimerNanosecondsInT2) {
        dt.t1 += toSecsAndNSecs(nsecs).first;
        dt.t2 += toSecsAndNSecs(nsecs).second;
        if (dt.t2 > 1000*1000*1000) {
            dt.t2 -= 1000*1000*1000;
            ++dt.t1;
        }
    } else {
        dt.t1 += nsecs;
    }
    return dt;
}

/*!
    \fn QDeadlineTimer QDeadlineTimer::current(Qt::TimerType timerType)

    Returns a QDeadlineTimer that is expired but is guaranteed to contain the
    current time. Objects created by this function can participate in the
    calculation of how long a timer is overdue, using the deadline() function.

    The QDeadlineTimer object will be constructed with the specified \a timerType.
*/

/*!
    \fn bool operator==(QDeadlineTimer d1, QDeadlineTimer d2)
    \relates QDeadlineTimer

    Returns true if the deadline on \a d1 and the deadline in \a d2 are the
    same, false otherwise. The timer type used to create the two deadlines is
    ignored. This function is equivalent to:

    \code
        return d1.deadlineNSecs() == d2.deadlineNSecs();
    \endcode

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn bool operator!=(QDeadlineTimer d1, QDeadlineTimer d2)
    \relates QDeadlineTimer

    Returns true if the deadline on \a d1 and the deadline in \a d2 are
    diferent, false otherwise. The timer type used to create the two deadlines
    is ignored. This function is equivalent to:

    \code
        return d1.deadlineNSecs() != d2.deadlineNSecs();
    \endcode

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn bool operator<(QDeadlineTimer d1, QDeadlineTimer d2)
    \relates QDeadlineTimer

    Returns true if the deadline on \a d1 is earlier than the deadline in \a
    d2, false otherwise. The timer type used to create the two deadlines is
    ignored. This function is equivalent to:

    \code
        return d1.deadlineNSecs() < d2.deadlineNSecs();
    \endcode

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn bool operator<=(QDeadlineTimer d1, QDeadlineTimer d2)
    \relates QDeadlineTimer

    Returns true if the deadline on \a d1 is earlier than or the same as the
    deadline in \a d2, false otherwise. The timer type used to create the two
    deadlines is ignored. This function is equivalent to:

    \code
        return d1.deadlineNSecs() <= d2.deadlineNSecs();
    \endcode

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn bool operator>(QDeadlineTimer d1, QDeadlineTimer d2)
    \relates QDeadlineTimer

    Returns true if the deadline on \a d1 is later than the deadline in \a
    d2, false otherwise. The timer type used to create the two deadlines is
    ignored. This function is equivalent to:

    \code
        return d1.deadlineNSecs() > d2.deadlineNSecs();
    \endcode

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn bool operator>=(QDeadlineTimer d1, QDeadlineTimer d2)
    \relates QDeadlineTimer

    Returns true if the deadline on \a d1 is later than or the same as the
    deadline in \a d2, false otherwise. The timer type used to create the two
    deadlines is ignored. This function is equivalent to:

    \code
        return d1.deadlineNSecs() >= d2.deadlineNSecs();
    \endcode

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn QDeadlineTimer operator+(QDeadlineTimer dt, qint64 msecs)
    \relates QDeadlineTimer

    Returns a QDeadlineTimer object whose deadline is \a msecs later than the
    deadline stored in \a dt. If \a dt is set to never expire, this function
    returns a QDeadlineTimer that does not expire either.

    To add times of precision greater than 1 millisecond, use addNSecs().
*/

/*!
    \fn QDeadlineTimer operator+(qint64 msecs, QDeadlineTimer dt)
    \relates QDeadlineTimer

    Returns a QDeadlineTimer object whose deadline is \a msecs later than the
    deadline stored in \a dt. If \a dt is set to never expire, this function
    returns a QDeadlineTimer that does not expire either.

    To add times of precision greater than 1 millisecond, use addNSecs().
*/

/*!
    \fn QDeadlineTimer operator-(QDeadlineTimer dt, qint64 msecs)
    \relates QDeadlineTimer

    Returns a QDeadlineTimer object whose deadline is \a msecs before the
    deadline stored in \a dt. If \a dt is set to never expire, this function
    returns a QDeadlineTimer that does not expire either.

    To subtract times of precision greater than 1 millisecond, use addNSecs().
*/

/*!
    \fn QDeadlineTimer &QDeadlineTimer::operator+=(qint64 msecs)

    Extends this QDeadlineTimer object by \a msecs milliseconds and returns
    itself. If this object is set to never expire, this function does nothing.

    To add times of precision greater than 1 millisecond, use addNSecs().
*/

/*!
    \fn QDeadlineTimer &QDeadlineTimer::operator-=(qint64 msecs)

    Shortens this QDeadlineTimer object by \a msecs milliseconds and returns
    itself. If this object is set to never expire, this function does nothing.

    To subtract times of precision greater than 1 millisecond, use addNSecs().
*/

// the rest of the functions are in qelapsedtimer_xxx.cpp

QT_END_NAMESPACE
