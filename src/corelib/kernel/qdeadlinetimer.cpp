// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdeadlinetimer.h"
#include "private/qnumeric_p.h"

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QDeadlineTimer)

using namespace std::chrono;

namespace {
struct TimeReference : std::numeric_limits<qint64>
{
    static constexpr qint64 Min = min();
    static constexpr qint64 Max = max();
};
}

template <typename Duration1, typename... Durations>
static qint64 add_saturate(qint64 t1, Duration1 dur, Durations... extra)
{
    qint64 v = dur.count();
    qint64 saturated = std::numeric_limits<qint64>::max();
    if (v < 0)
        saturated = std::numeric_limits<qint64>::min();

    // convert to nanoseconds with saturation
    using Ratio = std::ratio_divide<typename Duration1::period, nanoseconds::period>;
    static_assert(Ratio::den == 1, "sub-multiples of nanosecond are not supported");
    if (qMulOverflow<Ratio::num>(v, &v))
        return saturated;

    qint64 r;
    if (qAddOverflow(t1, v, &r))
        return saturated;
    if constexpr (sizeof...(Durations)) {
        // chain more additions
        return add_saturate(r, extra...);
    }
    return r;
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
    \fn QDeadlineTimer::QDeadlineTimer()
    \fn QDeadlineTimer::QDeadlineTimer(Qt::TimerType timerType)

    Constructs an expired QDeadlineTimer object. For this object,
    remainingTime() will return 0. If \a timerType is not set, then the object
    will use the \l{Qt::CoarseTimer}{coarse} \l{QDeadlineTimer#Timer types}{timer type}.

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
    time point in the past. If \a msecs is negative, the timer will be set to never
    expire, causing remainingTime() to return -1 and deadline() to return the
    maximum value.

    The QDeadlineTimer object will be constructed with the specified timer \a type.

    For optimization purposes, if \a msecs is zero, this function may skip
    obtaining the current time and may instead use a value known to be in the
    past. If that happens, deadline() may return an unexpected value and this
    object cannot be used in calculation of how long it is overdue. If that
    functionality is required, use QDeadlineTimer::current() and add time to
    it.

    \note Prior to Qt 6.6, the only value that caused the timer to never expire
    was -1.

    \sa hasExpired(), isForever(), remainingTime(), setRemainingTime()
*/
QDeadlineTimer::QDeadlineTimer(qint64 msecs, Qt::TimerType type) noexcept
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
    zero, this QDeadlineTimer object will be marked as expired, whereas a
    negative value will set it to never expire.

    For optimization purposes, if \a msecs is zero, this function may skip
    obtaining the current time and may instead use a value known to be in the
    past. If that happens, deadline() may return an unexpected value and this
    object cannot be used in calculation of how long it is overdue. If that
    functionality is required, use QDeadlineTimer::current() and add time to
    it.

    The timer type for this QDeadlineTimer object will be set to the specified \a timerType.

    \note Prior to Qt 6.6, the only value that caused the timer to never expire
    was -1.

    \sa setPreciseRemainingTime(), hasExpired(), isForever(), remainingTime()
*/
void QDeadlineTimer::setRemainingTime(qint64 msecs, Qt::TimerType timerType) noexcept
{
    if (msecs < 0) {
        *this = QDeadlineTimer(Forever, timerType);
    } else if (msecs == 0) {
        *this = QDeadlineTimer(timerType);
        t1 = std::numeric_limits<qint64>::min();
    } else {
        *this = current(timerType);
        milliseconds ms(msecs);
        t1 = add_saturate(t1, ms);
    }
}

/*!
    Sets the remaining time for this QDeadlineTimer object to \a secs seconds
    plus \a nsecs nanoseconds from now, if \a secs has a positive value. If \a
    secs is negative, this QDeadlineTimer will be set it to never expire (this
    behavior does not apply to \a nsecs). If both parameters are zero, this
    QDeadlineTimer will be marked as expired.

    For optimization purposes, if both \a secs and \a nsecs are zero, this
    function may skip obtaining the current time and may instead use a value
    known to be in the past. If that happens, deadline() may return an
    unexpected value and this object cannot be used in calculation of how long
    it is overdue. If that functionality is required, use
    QDeadlineTimer::current() and add time to it.

    The timer type for this QDeadlineTimer object will be set to the specified
    \a timerType.

    \note Prior to Qt 6.6, the only condition that caused the timer to never
    expire was when \a secs was -1.

    \sa setRemainingTime(), hasExpired(), isForever(), remainingTime()
*/
void QDeadlineTimer::setPreciseRemainingTime(qint64 secs, qint64 nsecs, Qt::TimerType timerType) noexcept
{
    if (secs < 0) {
        *this = QDeadlineTimer(Forever, timerType);
    } else if (secs == 0 && nsecs == 0) {
        *this = QDeadlineTimer(timerType);
        t1 = std::numeric_limits<qint64>::min();
    } else {
        *this = current(timerType);
        t1 = add_saturate(t1, seconds{secs}, nanoseconds{nsecs});
    }
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
    if (t1 == std::numeric_limits<qint64>::min())
        return true;
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

    nanoseconds nsecs(remainingTimeNSecs());
    return ceil<milliseconds>(nsecs).count();
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
    not deal with Forever. In case of underflow, which is only possible if the
    timer has expired, an arbitrary negative value is returned.
*/
qint64 QDeadlineTimer::rawRemainingTimeNSecs() const noexcept
{
    if (t1 == std::numeric_limits<qint64>::min())
        return t1;          // we'd saturate to this anyway

    QDeadlineTimer now = current(timerType());
    qint64 r;
    if (qSubOverflow(t1, now.t1, &r))
        return -1;      // any negative number is fine
    return r;
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
    if (t1 == TimeReference::Min)
        return t1;

    nanoseconds ns(t1);
    return duration_cast<milliseconds>(ns).count();
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

    return t1;
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
    t1 = add_saturate(0, milliseconds{msecs});
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
    t1 = add_saturate(0, seconds{secs}, nanoseconds{nsecs});
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

    dt.t1 = add_saturate(dt.t1, nanoseconds{nsecs});
    return dt;
}

/*!
    \fn QDeadlineTimer QDeadlineTimer::current(Qt::TimerType timerType)

    Returns a QDeadlineTimer that is expired but is guaranteed to contain the
    current time. Objects created by this function can participate in the
    calculation of how long a timer is overdue, using the deadline() function.

    The QDeadlineTimer object will be constructed with the specified \a timerType.
*/
QDeadlineTimer QDeadlineTimer::current(Qt::TimerType timerType) noexcept
{
    // ensure we get nanoseconds; this will work so long as steady_clock's
    // time_point isn't of finer resolution (picoseconds)
    std::chrono::nanoseconds ns = std::chrono::steady_clock::now().time_since_epoch();

    QDeadlineTimer result;
    result.t1 = ns.count();
    result.type = timerType;
    return result;
}

/*!
    \fn bool QDeadlineTimer::operator==(QDeadlineTimer d1, QDeadlineTimer d2)

    Returns true if the deadline on \a d1 and the deadline in \a d2 are the
    same, false otherwise. The timer type used to create the two deadlines is
    ignored. This function is equivalent to:

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 8

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn bool QDeadlineTimer::operator!=(QDeadlineTimer d1, QDeadlineTimer d2)

    Returns true if the deadline on \a d1 and the deadline in \a d2 are
    different, false otherwise. The timer type used to create the two deadlines
    is ignored. This function is equivalent to:

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 9

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn bool QDeadlineTimer::operator<(QDeadlineTimer d1, QDeadlineTimer d2)

    Returns true if the deadline on \a d1 is earlier than the deadline in \a
    d2, false otherwise. The timer type used to create the two deadlines is
    ignored. This function is equivalent to:

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 10

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn bool QDeadlineTimer::operator<=(QDeadlineTimer d1, QDeadlineTimer d2)

    Returns true if the deadline on \a d1 is earlier than or the same as the
    deadline in \a d2, false otherwise. The timer type used to create the two
    deadlines is ignored. This function is equivalent to:

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 11

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn bool QDeadlineTimer::operator>(QDeadlineTimer d1, QDeadlineTimer d2)

    Returns true if the deadline on \a d1 is later than the deadline in \a
    d2, false otherwise. The timer type used to create the two deadlines is
    ignored. This function is equivalent to:

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 12

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn bool QDeadlineTimer::operator>=(QDeadlineTimer d1, QDeadlineTimer d2)

    Returns true if the deadline on \a d1 is later than or the same as the
    deadline in \a d2, false otherwise. The timer type used to create the two
    deadlines is ignored. This function is equivalent to:

    \snippet code/src_corelib_kernel_qdeadlinetimer.cpp 13

    \note comparing QDeadlineTimer objects with different timer types is
    not supported and may result in unpredictable behavior.
*/

/*!
    \fn QDeadlineTimer QDeadlineTimer::operator+(QDeadlineTimer dt, qint64 msecs)

    Returns a QDeadlineTimer object whose deadline is \a msecs later than the
    deadline stored in \a dt. If \a dt is set to never expire, this function
    returns a QDeadlineTimer that does not expire either.

    To add times of precision greater than 1 millisecond, use addNSecs().
*/

QDeadlineTimer operator+(QDeadlineTimer dt, qint64 msecs)
{
    if (dt.isForever())
        return dt;

    dt.t1 = add_saturate(dt.t1, milliseconds{msecs});
    return dt;
}

/*!
    \fn QDeadlineTimer QDeadlineTimer::operator+(qint64 msecs, QDeadlineTimer dt)

    Returns a QDeadlineTimer object whose deadline is \a msecs later than the
    deadline stored in \a dt. If \a dt is set to never expire, this function
    returns a QDeadlineTimer that does not expire either.

    To add times of precision greater than 1 millisecond, use addNSecs().
*/

/*!
    \fn QDeadlineTimer QDeadlineTimer::operator-(QDeadlineTimer dt, qint64 msecs)

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

QT_END_NAMESPACE
