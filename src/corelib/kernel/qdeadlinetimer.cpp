/****************************************************************************
**
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

#include "qdeadlinetimer.h"
#include "qdeadlinetimer_p.h"
#include "private/qnumeric_p.h"

QT_BEGIN_NAMESPACE

namespace {
    class TimeReference
    {
        enum : unsigned {
            umega = 1000 * 1000,
            ugiga = umega * 1000
        };

        enum : qint64 {
            kilo = 1000,
            mega = kilo * 1000,
            giga = mega * 1000
        };

    public:
        enum RoundingStrategy {
            RoundDown,
            RoundUp,
            RoundDefault = RoundDown
        };

        static constexpr qint64 Min = std::numeric_limits<qint64>::min();
        static constexpr qint64 Max = std::numeric_limits<qint64>::max();

        inline TimeReference(qint64 = 0, unsigned = 0);
        inline void updateTimer(qint64 &, unsigned &);

        inline bool addNanoseconds(qint64);
        inline bool addMilliseconds(qint64);
        bool addSecsAndNSecs(qint64, qint64);

        inline bool subtract(const qint64, const unsigned);

        inline bool toMilliseconds(qint64 *, RoundingStrategy = RoundDefault) const;
        inline bool toNanoseconds(qint64 *) const;

        inline void saturate(bool toMax);
        static bool sign(qint64, qint64);

    private:
        bool adjust(const qint64, const unsigned, qint64 = 0);

    private:
        qint64 secs;
        unsigned nsecs;
    };
}

inline TimeReference::TimeReference(qint64 t1, unsigned t2)
    : secs(t1), nsecs(t2)
{
}

inline void TimeReference::updateTimer(qint64 &t1, unsigned &t2)
{
    t1 = secs;
    t2 = nsecs;
}

inline void TimeReference::saturate(bool toMax)
{
    secs = toMax ? Max : Min;
}

/*!
 * \internal
 *
 * Determines the sign of a (seconds, nanoseconds) pair
 * for differentiating overflow from underflow. It doesn't
 * deal with equality as it shouldn't ever be called in that case.
 *
 * Returns true if the pair represents a positive time offset
 * false otherwise.
 */
bool TimeReference::sign(qint64 secs, qint64 nsecs)
{
    if (secs > 0) {
        if (nsecs > 0)
            return true;
    } else {
        if (nsecs < 0)
            return false;
    }

    // They are different in sign
    secs += nsecs / giga;
    if (secs > 0)
        return true;
    else if (secs < 0)
        return false;

    // We should never get over|underflow out of
    // the case: secs * giga == -nsecs
    // So the sign of nsecs is the deciding factor
    Q_ASSERT(nsecs % giga != 0);
    return nsecs > 0;
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
inline bool TimeReference::addNanoseconds(qint64 arg)
{
    return addSecsAndNSecs(arg / giga, arg % giga);
}

inline bool TimeReference::addMilliseconds(qint64 arg)
{
    return addSecsAndNSecs(arg / kilo, (arg % kilo) * mega);
}

/*!
 * \internal
 *
 * Adds \a t1 addSecs seconds and \a addNSecs nanoseconds to the
 * time reference. The arguments are normalized to seconds (qint64)
 * and nanoseconds (unsigned) before the actual calculation is
 * delegated to adjust(). If the nanoseconds are negative the
 * owed second used for the normalization is passed on to adjust()
 * as third argument.
 *
 * Returns true if operation was successful, false on over|underflow
 */
bool TimeReference::addSecsAndNSecs(qint64 addSecs, qint64 addNSecs)
{
    // Normalize the arguments
    if (qAbs(addNSecs) >= giga) {
        if (add_overflow<qint64>(addSecs, addNSecs / giga, &addSecs))
            return false;

        addNSecs %= giga;
    }

    if (addNSecs < 0)
        return adjust(addSecs, ugiga - unsigned(-addNSecs), -1);

    return adjust(addSecs, unsigned(addNSecs));
}

/*!
 * \internal
 *
 * Adds \a t1 seconds and \a t2 nanoseconds to the internal members.
 * Takes into account the additional \a carrySeconds we may owe or need to carry over.
 *
 * Returns true if operation was successful, false on over|underflow
 */
bool TimeReference::adjust(const qint64 t1, const unsigned t2, qint64 carrySeconds)
{
    Q_STATIC_ASSERT(QDeadlineTimerNanosecondsInT2);
    nsecs += t2;
    if (nsecs >= ugiga) {
        nsecs -= ugiga;
        carrySeconds++;
    }

    // We don't worry about the order of addition, because the result returned by
    // callers of this function is unchanged regardless of us over|underflowing.
    // If we do, we do so by no more than a second, thus saturating the timer to
    // Forever has the same effect as if we did the arithmetic exactly and salvaged
    // the overflow.
    return !add_overflow<qint64>(secs, t1, &secs) && !add_overflow<qint64>(secs, carrySeconds, &secs);
}

/*!
 * \internal
 *
 * Subtracts \a t1 seconds and \a t2 nanoseconds from the time reference.
 * When normalizing the nanoseconds to a positive number the owed seconds is
 * passed as third argument to adjust() as the seconds may over|underflow
 * if we do the calculation directly. There is little sense to check the
 * seconds for over|underflow here in case we are going to need to carry
 * over a second _after_ we add the nanoseconds.
 *
 * Returns true if operation was successful, false on over|underflow
 */
inline bool TimeReference::subtract(const qint64 t1, const unsigned t2)
{
    Q_ASSERT(t2 < ugiga);
    return adjust(-t1, ugiga - t2, -1);
}

/*!
 * \internal
 *
 * Converts the time reference to milliseconds.
 *
 * Checks are done without making use of mul_overflow because it may
 * not be implemented on some 32bit platforms.
 *
 * Returns true if operation was successful, false on over|underflow
 */
inline bool TimeReference::toMilliseconds(qint64 *result, RoundingStrategy rounding) const
{
    static constexpr qint64 maxSeconds = Max / kilo;
    static constexpr qint64 minSeconds = Min / kilo;
    if (secs > maxSeconds || secs < minSeconds)
        return false;

    unsigned ns = rounding == RoundDown ? nsecs : nsecs + umega - 1;

    return !add_overflow<qint64>(secs * kilo, ns / umega, result);
}

/*!
 * \internal
 *
 * Converts the time reference to nanoseconds.
 *
 * Checks are done without making use of mul_overflow because it may
 * not be implemented on some 32bit platforms.
 *
 * Returns true if operation was successful, false on over|underflow
 */
inline bool TimeReference::toNanoseconds(qint64 *result) const
{
    static constexpr qint64 maxSeconds = Max / giga;
    static constexpr qint64 minSeconds = Min / giga;
    if (secs > maxSeconds || secs < minSeconds)
        return false;

    return !add_overflow<qint64>(secs * giga, nsecs, result);
}
#else
inline bool TimeReference::addNanoseconds(qint64 arg)
{
    return adjust(arg, 0);
}

inline bool TimeReference::addMilliseconds(qint64 arg)
{
    static constexpr qint64 maxMilliseconds = Max / mega;
    if (qAbs(arg) > maxMilliseconds)
        return false;

    return addNanoseconds(arg * mega);
}

inline bool TimeReference::addSecsAndNSecs(qint64 addSecs, qint64 addNSecs)
{
    static constexpr qint64 maxSeconds = Max / giga;
    static constexpr qint64 minSeconds = Min / giga;
    if (addSecs > maxSeconds || addSecs < minSeconds || add_overflow<qint64>(addSecs * giga, addNSecs, &addNSecs))
        return false;

    return addNanoseconds(addNSecs);
}

inline bool TimeReference::adjust(const qint64 t1, const unsigned t2, qint64 carrySeconds)
{
    Q_STATIC_ASSERT(!QDeadlineTimerNanosecondsInT2);
    Q_UNUSED(t2);
    Q_UNUSED(carrySeconds);

    return !add_overflow<qint64>(secs, t1, &secs);
}

inline bool TimeReference::subtract(const qint64 t1, const unsigned t2)
{
    Q_UNUSED(t2);

    return addNanoseconds(-t1);
}

inline bool TimeReference::toMilliseconds(qint64 *result, RoundingStrategy rounding) const
{
    // Force QDeadlineTimer to treat the border cases as
    // over|underflow and saturate the results returned to the user.
    // We don't want to get valid milliseconds out of saturated timers.
    if (secs == Max || secs == Min)
        return false;

    *result = secs / mega;
    if (rounding == RoundUp && secs > *result * mega)
        (*result)++;

    return true;
}

inline bool TimeReference::toNanoseconds(qint64 *result) const
{
    *result = secs;
    return true;
}
#endif

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

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 0

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

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 1

    As can be seen in the example above, QDeadlineTimer offers a templated
    version of remainingTime() and deadline() that can be used to return
    \c{std::chrono} objects.

    Note that comparing to \c{time_point} is not as efficient as comparing to
    \c{duration}, since QDeadlineTimer may need to convert from its own
    internal clock source to the clock source used by the \c{time_point} object.
    Also note that, due to this conversion, the deadlines will not be precise,
    so the following code is not expected to compare equally:

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 2

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
    \fn QDeadlineTimer::QDeadlineTimer(ForeverConstant, Qt::TimerType timerType)

    QDeadlineTimer objects created with ForeverConstant never expire.
    For such objects, remainingTime() will return -1, deadline() will return the
    maximum value, and isForever() will return true.

    The timer type \a timerType may be ignored, since the timer will never
    expire.

    \sa ForeverConstant, hasExpired(), isForever(), remainingTime(), timerType()
*/

/*!
    Constructs a QDeadlineTimer object with an expiry time of \a msecs msecs
    from the moment of the creation of this object, if msecs is positive. If \a
    msecs is zero, this QDeadlineTimer will be marked as expired, causing
    remainingTime() to return zero and deadline() to return an indeterminate
    time point in the past. If \a msecs is -1, the timer will be set to never
    expire, causing remainingTime() to return -1 and deadline() to return the
    maximum value.

    The QDeadlineTimer object will be constructed with the specified timer \a type.

    For optimization purposes, if \a msecs is zero, this function may skip
    obtaining the current time and may instead use a value known to be in the
    past. If that happens, deadline() may return an unexpected value and this
    object cannot be used in calculation of how long it is overdue. If that
    functionality is required, use QDeadlineTimer::current() and add time to
    it.

    \sa hasExpired(), isForever(), remainingTime(), setRemainingTime()
*/
QDeadlineTimer::QDeadlineTimer(qint64 msecs, Qt::TimerType type) noexcept
    : t2(0)
{
    setRemainingTime(msecs, type);
}

/*!
    \fn template <class Clock, class Duration> QDeadlineTimer::QDeadlineTimer(std::chrono::time_point<Clock, Duration> deadline, Qt::TimerType type)

    Constructs a QDeadlineTimer object with a deadline at \a deadline time
    point, converting from the clock source \c{Clock} to Qt's internal clock
    source (see QElapsedTimer::clockType()).

    If \a deadline is in the past, this QDeadlineTimer object is set to
    expired, whereas if \a deadline is equal to \c{Duration::max()}, then this
    object is set to never expire.

    The QDeadlineTimer object will be constructed with the specified timer \a type.

    \sa hasExpired(), isForever(), remainingTime(), setDeadline()
*/

/*!
    \fn template <class Rep, class Period> QDeadlineTimer::QDeadlineTimer(std::chrono::duration<Rep, Period> remaining, Qt::TimerType type)

    Constructs a QDeadlineTimer object with a remaining time of \a remaining.
    If \a remaining is zero or negative, this QDeadlineTimer object will be
    mark as expired, whereas if \a remaining is equal to \c{duration::max()},
    the object will be set to never expire.

    The QDeadlineTimer object will be constructed with the specified timer \a type.

    This constructor can be used with C++14's user-defined literals for time, such as in:

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 3

    For optimization purposes, if \a remaining is zero or negative, this
    function may skip obtaining the current time and may instead use a value
    known to be in the past. If that happens, deadline() may return an
    unexpected value and this object cannot be used in calculation of how long
    it is overdue. If that functionality is required, use
    QDeadlineTimer::current() and add time to it.

    \sa hasExpired(), isForever(), remainingTime(), setRemainingTime()
*/

/*!
    \fn template <class Clock, class Duration> void QDeadlineTimer::setDeadline(std::chrono::time_point<Clock, Duration> deadline, Qt::TimerType type)

    Sets this QDeadlineTimer to the deadline marked by \a deadline time
    point, converting from the clock source \c{Clock} to Qt's internal clock
    source (see QElapsedTimer::clockType()).

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
void QDeadlineTimer::setRemainingTime(qint64 msecs, Qt::TimerType timerType) noexcept
{
    if (msecs == -1) {
        *this = QDeadlineTimer(Forever, timerType);
        return;
    }

    *this = current(timerType);

    TimeReference ref(t1, t2);
    if (!ref.addMilliseconds(msecs))
        ref.saturate(msecs > 0);
    ref.updateTimer(t1, t2);
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
void QDeadlineTimer::setPreciseRemainingTime(qint64 secs, qint64 nsecs, Qt::TimerType timerType) noexcept
{
    if (secs == -1) {
        *this = QDeadlineTimer(Forever, timerType);
        return;
    }

    *this = current(timerType);
    TimeReference ref(t1, t2);
    if (!ref.addSecsAndNSecs(secs, nsecs))
        ref.saturate(TimeReference::sign(secs, nsecs));
    ref.updateTimer(t1, t2);
}

/*!
    \overload
    \fn template <class Rep, class Period> void QDeadlineTimer::setRemainingTime(std::chrono::duration<Rep, Period> remaining, Qt::TimerType type)

    Sets the remaining time for this QDeadlineTimer object to \a remaining. If
    \a remaining is zero or negative, this QDeadlineTimer object will be mark
    as expired, whereas if \a remaining is equal to \c{duration::max()}, the
    object will be set to never expire.

    The timer type for this QDeadlineTimer object will be set to the specified \a type.

    This function can be used with C++14's user-defined literals for time, such as in:

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 4

    \note Qt detects the necessary C++14 compiler support by way of the feature
    test recommendations from
    \l{https://isocpp.org/std/standing-documents/sd-6-sg10-feature-test-recommendations}
    {C++ Committee's Standing Document 6}.

    \sa setDeadline(), remainingTime(), hasExpired(), isForever()
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
bool QDeadlineTimer::hasExpired() const noexcept
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

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 5

    \sa remainingTimeNSecs(), isForever(), hasExpired()
*/
qint64 QDeadlineTimer::remainingTime() const noexcept
{
    if (isForever())
        return -1;

    QDeadlineTimer now = current(timerType());
    TimeReference ref(t1, t2);

    qint64 msecs;
    if (!ref.subtract(now.t1, now.t2))
        return 0;   // We can only underflow here

    // If we fail the conversion, t1 < now.t1 means we underflowed,
    // thus the deadline had long expired
    if (!ref.toMilliseconds(&msecs, TimeReference::RoundUp))
        return t1 < now.t1 ? 0 : -1;

    return msecs < 0 ? 0 : msecs;
}

/*!
    Returns the remaining time in this QDeadlineTimer object in nanoseconds. If
    the timer has already expired, this function will return zero and it is not
    possible to obtain the amount of time overdue with this function. If the
    timer was set to never expire, this function returns -1.

    \sa remainingTime(), isForever(), hasExpired()
*/
qint64 QDeadlineTimer::remainingTimeNSecs() const noexcept
{
    if (isForever())
        return -1;
    qint64 raw = rawRemainingTimeNSecs();
    return raw < 0 ? 0 : raw;
}

/*!
    \internal
    Same as remainingTimeNSecs, but may return negative remaining times. Does
    not deal with Forever. In case of underflow the result is saturated to
    the minimum possible value, on overflow  - the maximum possible value.
*/
qint64 QDeadlineTimer::rawRemainingTimeNSecs() const noexcept
{
    QDeadlineTimer now = current(timerType());
    TimeReference ref(t1, t2);

    qint64 nsecs;
    if (!ref.subtract(now.t1, now.t2))
        return TimeReference::Min;  // We can only underflow here

    // If we fail the conversion, t1 < now.t1 means we underflowed,
    // thus the deadline had long expired
    if (!ref.toNanoseconds(&nsecs))
        return t1 < now.t1 ? TimeReference::Min : TimeReference::Max;
    return nsecs;
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

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 6

    \note Timers that were created as expired have an indetermine time point in
    the past as their deadline, so the above calculation may not work.

    \sa remainingTime(), deadlineNSecs(), setDeadline()
*/
qint64 QDeadlineTimer::deadline() const noexcept
{
    if (isForever())
        return TimeReference::Max;

    qint64 result;
    if (!TimeReference(t1, t2).toMilliseconds(&result))
        return t1 < 0 ? TimeReference::Min : TimeReference::Max;

    return result;
}

/*!
    Returns the absolute time point for the deadline stored in QDeadlineTimer
    object, calculated in nanoseconds relative to the reference clock, the
    same as QElapsedTimer::msecsSinceReference(). The value will be in the past
    if this QDeadlineTimer has expired.

    If this QDeadlineTimer never expires or the number of nanoseconds until the
    deadline can't be accommodated in the return type, this function returns
    \c{std::numeric_limits<qint64>::max()}.

    This function can be used to calculate the amount of time a timer is
    overdue, by subtracting QDeadlineTimer::current(), as in the following
    example:

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 7

    \note Timers that were created as expired have an indetermine time point in
    the past as their deadline, so the above calculation may not work.

    \sa remainingTime(), deadlineNSecs()
*/
qint64 QDeadlineTimer::deadlineNSecs() const noexcept
{
    if (isForever())
        return TimeReference::Max;

    qint64 result;
    if (!TimeReference(t1, t2).toNanoseconds(&result))
        return t1 < 0 ? TimeReference::Min : TimeReference::Max;

    return result;
}

/*!
    Sets the deadline for this QDeadlineTimer object to be the \a msecs
    absolute time point, counted in milliseconds since the reference clock (the
    same as QElapsedTimer::msecsSinceReference()), and the timer type to \a
    timerType. If the value is in the past, this QDeadlineTimer will be marked
    as expired.

    If \a msecs is \c{std::numeric_limits<qint64>::max()} or the deadline is
    beyond a representable point in the future, this QDeadlineTimer will be set
    to never expire.

    \sa setPreciseDeadline(), deadline(), deadlineNSecs(), setRemainingTime()
*/
void QDeadlineTimer::setDeadline(qint64 msecs, Qt::TimerType timerType) noexcept
{
    if (msecs == TimeReference::Max) {
        *this = QDeadlineTimer(Forever, timerType);
        return;
    }

    type = timerType;

    TimeReference ref;
    if (!ref.addMilliseconds(msecs))
        ref.saturate(msecs > 0);
    ref.updateTimer(t1, t2);
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
void QDeadlineTimer::setPreciseDeadline(qint64 secs, qint64 nsecs, Qt::TimerType timerType) noexcept
{
    type = timerType;

    // We don't pass the seconds to the constructor, because we don't know
    // at this point if t1 holds the seconds or nanoseconds; it's platform specific.
    TimeReference ref;
    if (!ref.addSecsAndNSecs(secs, nsecs))
        ref.saturate(TimeReference::sign(secs, nsecs));
    ref.updateTimer(t1, t2);
}

/*!
    Returns a QDeadlineTimer object whose deadline is extended from \a dt's
    deadline by \a nsecs nanoseconds. If \a dt was set to never expire, this
    function returns a QDeadlineTimer that will not expire either.

    \note if \a dt was created as expired, its deadline is indeterminate and
    adding an amount of time may or may not cause it to become unexpired.
*/
QDeadlineTimer QDeadlineTimer::addNSecs(QDeadlineTimer dt, qint64 nsecs) noexcept
{
    if (dt.isForever())
        return dt;

    TimeReference ref(dt.t1, dt.t2);
    if (!ref.addNanoseconds(nsecs))
        ref.saturate(nsecs > 0);
    ref.updateTimer(dt.t1, dt.t2);

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

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 8

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn bool operator!=(QDeadlineTimer d1, QDeadlineTimer d2)
    \relates QDeadlineTimer

    Returns true if the deadline on \a d1 and the deadline in \a d2 are
    diferent, false otherwise. The timer type used to create the two deadlines
    is ignored. This function is equivalent to:

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 9

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn bool operator<(QDeadlineTimer d1, QDeadlineTimer d2)
    \relates QDeadlineTimer

    Returns true if the deadline on \a d1 is earlier than the deadline in \a
    d2, false otherwise. The timer type used to create the two deadlines is
    ignored. This function is equivalent to:

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 10

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn bool operator<=(QDeadlineTimer d1, QDeadlineTimer d2)
    \relates QDeadlineTimer

    Returns true if the deadline on \a d1 is earlier than or the same as the
    deadline in \a d2, false otherwise. The timer type used to create the two
    deadlines is ignored. This function is equivalent to:

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 11

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn bool operator>(QDeadlineTimer d1, QDeadlineTimer d2)
    \relates QDeadlineTimer

    Returns true if the deadline on \a d1 is later than the deadline in \a
    d2, false otherwise. The timer type used to create the two deadlines is
    ignored. This function is equivalent to:

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 12

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn bool operator>=(QDeadlineTimer d1, QDeadlineTimer d2)
    \relates QDeadlineTimer

    Returns true if the deadline on \a d1 is later than or the same as the
    deadline in \a d2, false otherwise. The timer type used to create the two
    deadlines is ignored. This function is equivalent to:

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 13

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

QDeadlineTimer operator+(QDeadlineTimer dt, qint64 msecs)
{
    if (dt.isForever())
        return dt;

    TimeReference ref(dt.t1, dt.t2);
    if (!ref.addMilliseconds(msecs))
        ref.saturate(msecs > 0);
    ref.updateTimer(dt.t1, dt.t2);

    return dt;
}

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

/*!
  \fn void QDeadlineTimer::swap(QDeadlineTimer &other)

  Swaps this deadline timer with the \a other deadline timer.
 */

/*!
  \fn template <class Clock, class Duration> QDeadlineTimer & QDeadlineTimer::operator=(std::chrono::time_point<Clock, Duration> deadline_)

  Assigns \a deadline_ to this deadline timer.
 */

/*!
  \fn template <class Rep, class Period> QDeadlineTimer & QDeadlineTimer::operator=(std::chrono::duration<Rep, Period> remaining)

  Sets this deadline timer to the \a remaining time.
 */

/*!
  \fn std::chrono::nanoseconds QDeadlineTimer::remainingTimeAsDuration() const

  Returns the time remaining before the deadline.
 */

/*!
  \fn QPair<qint64, unsigned> QDeadlineTimer::_q_data() const
  \internal
*/

// the rest of the functions are in qelapsedtimer_xxx.cpp

QT_END_NAMESPACE
