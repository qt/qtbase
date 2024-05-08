// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qelapsedtimer.h"

QT_BEGIN_NAMESPACE

/*!
    \class QElapsedTimer
    \inmodule QtCore
    \brief The QElapsedTimer class provides a fast way to calculate elapsed times.
    \since 4.7

    \reentrant
    \ingroup tools

    \compares strong

    The QElapsedTimer class is usually used to quickly calculate how much
    time has elapsed between two events. Its API is similar to that of QTime,
    so code that was using that can be ported quickly to the new class.

    However, unlike QTime, QElapsedTimer tries to use monotonic clocks if
    possible. This means it's not possible to convert QElapsedTimer objects
    to a human-readable time.

    The typical use-case for the class is to determine how much time was
    spent in a slow operation. The simplest example of such a case is for
    debugging purposes, as in the following example:

    \snippet qelapsedtimer/main.cpp 0

    In this example, the timer is started by a call to start() and the
    elapsed time is calculated by the elapsed() function.

    The time elapsed can also be used to recalculate the time available for
    another operation, after the first one is complete. This is useful when
    the execution must complete within a certain time period, but several
    steps are needed. The \tt{waitFor}-type functions in QIODevice and its
    subclasses are good examples of such need. In that case, the code could
    be as follows:

    \snippet qelapsedtimer/main.cpp 1

    Another use-case is to execute a certain operation for a specific
    timeslice. For this, QElapsedTimer provides the hasExpired() convenience
    function, which can be used to determine if a certain number of
    milliseconds has already elapsed:

    \snippet qelapsedtimer/main.cpp 2

    It is often more convenient to use \l{QDeadlineTimer} in this case, which
    counts towards a timeout in the future instead of tracking elapsed time.

    \section1 Reference Clocks

    QElapsedTimer will use the platform's monotonic reference clock in all
    platforms that support it (see QElapsedTimer::isMonotonic()). This has
    the added benefit that QElapsedTimer is immune to time adjustments, such
    as the user correcting the time. Also unlike QTime, QElapsedTimer is
    immune to changes in the timezone settings, such as daylight-saving
    periods.

    On the other hand, this means QElapsedTimer values can only be compared
    with other values that use the same reference. This is especially true if
    the time since the reference is extracted from the QElapsedTimer object
    (QElapsedTimer::msecsSinceReference()) and serialised. These values
    should never be exchanged across the network or saved to disk, since
    there's no telling whether the computer node receiving the data is the
    same as the one originating it or if it has rebooted since.

    It is, however, possible to exchange the value with other processes
    running on the same machine, provided that they also use the same
    reference clock. QElapsedTimer will always use the same clock, so it's
    safe to compare with the value coming from another process in the same
    machine. If comparing to values produced by other APIs, you should check
    that the clock used is the same as QElapsedTimer (see
    QElapsedTimer::clockType()).

    \sa QTime, QChronoTimer, QDeadlineTimer
*/

/*!
    \enum QElapsedTimer::ClockType

    This enum contains the different clock types that QElapsedTimer may use.

    QElapsedTimer will always use the same clock type in a particular
    machine, so this value will not change during the lifetime of a program.
    It is provided so that QElapsedTimer can be used with other non-Qt
    implementations, to guarantee that the same reference clock is being
    used.

    \value SystemTime         The human-readable system time. This clock is not monotonic.
    \value MonotonicClock     The system's monotonic clock, usually found in Unix systems.
                              This clock is monotonic.
    \value TickCounter        Not used anymore.
    \value MachAbsoluteTime   The Mach kernel's absolute time (\macos and iOS).
                              This clock is monotonic.
    \value PerformanceCounter The performance counter provided by Windows.
                              This clock is monotonic.

    \section2 SystemTime

    The system time clock is purely the real time, expressed in milliseconds
    since Jan 1, 1970 at 0:00 UTC. It's equivalent to the value returned by
    the C and POSIX \tt{time} function, with the milliseconds added. This
    clock type is currently only used on Unix systems that do not support
    monotonic clocks (see below).

    This is the only non-monotonic clock that QElapsedTimer may use.

    \section2 MonotonicClock

    This is the system's monotonic clock, expressed in milliseconds since an
    arbitrary point in the past. This clock type is used on Unix systems
    which support POSIX monotonic clocks (\tt{_POSIX_MONOTONIC_CLOCK}).

    \section2 MachAbsoluteTime

    This clock type is based on the absolute time presented by Mach kernels,
    such as that found on \macos. This clock type is presented separately
    from MonotonicClock since \macos and iOS are also Unix systems and may support
    a POSIX monotonic clock with values differing from the Mach absolute
    time.

    This clock is monotonic.

    \section2 PerformanceCounter

    This clock uses the Windows functions \tt{QueryPerformanceCounter} and
    \tt{QueryPerformanceFrequency} to access the system's performance counter.

    This clock is monotonic.

    \sa clockType(), isMonotonic()
*/

/*!
    \fn QElapsedTimer::QElapsedTimer()
    \since 5.4

    Constructs an invalid QElapsedTimer. A timer becomes valid once it has been
    started.

    \sa isValid(), start()
*/

/*!
    \fn bool QElapsedTimer::operator==(const QElapsedTimer &lhs, const QElapsedTimer &rhs) noexcept

    Returns \c true if \a lhs and \a rhs contain the same time, false otherwise.
*/
/*!
    \fn bool QElapsedTimer::operator!=(const QElapsedTimer &lhs, const QElapsedTimer &rhs) noexcept

    Returns \c true if \a lhs and \a rhs contain different times, false otherwise.
*/
/*!
    \fn bool QElapsedTimer::operator<(const QElapsedTimer &lhs, const QElapsedTimer &rhs) noexcept

    Returns \c true if \a lhs was started before \a rhs, false otherwise.

    The returned value is undefined if one of the two parameters is invalid
    and the other isn't. However, two invalid timers are equal and thus this
    function will return false.
*/

/*!
    \fn QElapsedTimer::clockType() noexcept

    Returns the clock type that this QElapsedTimer implementation uses.

    Since Qt 6.6, QElapsedTimer uses \c{std::chrono::steady_clock}, so the
    clock type is always \l MonotonicClock.

    \sa isMonotonic()
*/

QElapsedTimer::ClockType QElapsedTimer::clockType() noexcept
{
    // we use std::chrono::steady_clock
    return MonotonicClock;
}

/*!
    \fn QElapsedTimer::isMonotonic() noexcept

    Returns \c true if this is a monotonic clock, false otherwise. See the
    information on the different clock types to understand which ones are
    monotonic.

    Since Qt 6.6, QElapsedTimer uses \c{std::chrono::steady_clock}, so this
    function now always returns true.

    \sa clockType(), QElapsedTimer::ClockType
*/
bool QElapsedTimer::isMonotonic() noexcept
{
    // We trust std::chrono::steady_clock to be steady (monotonic); if the
    // Standard Library is lying to us, users must complain to their vendor.
    return true;
}

/*!
    \typealias QElapsedTimer::Duration
    Synonym for \c std::chrono::nanoseconds.
*/

/*!
    \typealias QElapsedTimer::TimePoint
    Synonym for \c {std::chrono::time_point<std::chrono::steady_clock, Duration>}.
*/

/*!
    Starts this timer. Once started, a timer value can be checked with elapsed() or msecsSinceReference().

    Normally, a timer is started just before a lengthy operation, such as:
    \snippet qelapsedtimer/main.cpp 0

    Also, starting a timer makes it valid again.

    \sa restart(), invalidate(), elapsed()
*/
void QElapsedTimer::start() noexcept
{
    static_assert(sizeof(t1) == sizeof(Duration::rep));

    // This assignment will work so long as TimePoint uses the same time
    // duration or one of finer granularity than steady_clock::time_point. That
    // means it will work until the first steady_clock using picoseconds.
    TimePoint now = std::chrono::steady_clock::now();
    t1 = now.time_since_epoch().count();
    QT6_ONLY(t2 = 0);
}

/*!
    Restarts the timer and returns the number of milliseconds elapsed since
    the previous start.
    This function is equivalent to obtaining the elapsed time with elapsed()
    and then starting the timer again with start(), but it does so in one
    single operation, avoiding the need to obtain the clock value twice.

    Calling this function on a QElapsedTimer that is invalid
    results in undefined behavior.

    The following example illustrates how to use this function to calibrate a
    parameter to a slow operation (for example, an iteration count) so that
    this operation takes at least 250 milliseconds:

    \snippet qelapsedtimer/main.cpp 3

    \sa start(), invalidate(), elapsed(), isValid()
*/
qint64 QElapsedTimer::restart() noexcept
{
    QElapsedTimer old = *this;
    start();
    return old.msecsTo(*this);
}

/*!
    \since 6.6

    Returns a \c{std::chrono::nanoseconds} with the time since this QElapsedTimer was last
    started.

    Calling this function on a QElapsedTimer that is invalid
    results in undefined behavior.

    On platforms that do not provide nanosecond resolution, the value returned
    will be the best estimate available.

    \sa start(), restart(), hasExpired(), invalidate()
*/
auto QElapsedTimer::durationElapsed() const noexcept -> Duration
{
    TimePoint then{Duration(t1)};
    return std::chrono::steady_clock::now() - then;
}

/*!
    \since 4.8

    Returns the number of nanoseconds since this QElapsedTimer was last
    started.

    Calling this function on a QElapsedTimer that is invalid
    results in undefined behavior.

    On platforms that do not provide nanosecond resolution, the value returned
    will be the best estimate available.

    \sa start(), restart(), hasExpired(), invalidate()
*/
qint64 QElapsedTimer::nsecsElapsed() const noexcept
{
    return durationElapsed().count();
}

/*!
    Returns the number of milliseconds since this QElapsedTimer was last
    started.

    Calling this function on a QElapsedTimer that is invalid
    results in undefined behavior.

    \sa start(), restart(), hasExpired(), isValid(), invalidate()
*/
qint64 QElapsedTimer::elapsed() const noexcept
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(durationElapsed()).count();
}

/*!
    Returns the number of milliseconds between last time this QElapsedTimer
    object was started and its reference clock's start.

    This number is usually arbitrary for all clocks except the
    QElapsedTimer::SystemTime clock. For that clock type, this number is the
    number of milliseconds since January 1st, 1970 at 0:00 UTC (that is, it
    is the Unix time expressed in milliseconds).

    On Linux, Windows and Apple platforms, this value is usually the time
    since the system boot, though it usually does not include the time the
    system has spent in sleep states.

    \sa clockType(), elapsed()
*/
qint64 QElapsedTimer::msecsSinceReference() const noexcept
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(Duration(t1)).count();
}

/*!
    \since 6.6

    Returns the time difference between this QElapsedTimer and \a other as a
    \c{std::chrono::nanoseconds}. If \a other was started before this object,
    the returned value will be negative. If it was started later, the returned
    value will be positive.

    The return value is undefined if this object or \a other were invalidated.

    \sa secsTo(), elapsed()
*/
auto QElapsedTimer::durationTo(const QElapsedTimer &other) const noexcept -> Duration
{
    Duration d1(t1);
    Duration d2(other.t1);
    return d2 - d1;
}

/*!
    Returns the number of milliseconds between this QElapsedTimer and \a
    other. If \a other was started before this object, the returned value
    will be negative. If it was started later, the returned value will be
    positive.

    The return value is undefined if this object or \a other were invalidated.

    \sa secsTo(), elapsed()
*/
qint64 QElapsedTimer::msecsTo(const QElapsedTimer &other) const noexcept
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(durationTo(other)).count();
}

/*!
    Returns the number of seconds between this QElapsedTimer and \a other. If
    \a other was started before this object, the returned value will be
    negative. If it was started later, the returned value will be positive.

    Calling this function on or with a QElapsedTimer that is invalid
    results in undefined behavior.

    \sa msecsTo(), elapsed()
*/
qint64 QElapsedTimer::secsTo(const QElapsedTimer &other) const noexcept
{
    using namespace std::chrono;
    return duration_cast<seconds>(durationTo(other)).count();
}

static const qint64 invalidData = Q_INT64_C(0x8000000000000000);

/*!
    \fn QElapsedTimer::invalidate() noexcept
    Marks this QElapsedTimer object as invalid.

    An invalid object can be checked with isValid(). Calculations of timer
    elapsed since invalid data are undefined and will likely produce bizarre
    results.

    \sa isValid(), start(), restart()
*/
void QElapsedTimer::invalidate() noexcept
{
    t1 = t2 = invalidData;
}

/*!
    Returns \c false if the timer has never been started or invalidated by a
    call to invalidate().

    \sa invalidate(), start(), restart()
*/
bool QElapsedTimer::isValid() const noexcept
{
    return t1 != invalidData && t2 != invalidData;
}

/*!
    Returns \c true if elapsed() exceeds the given \a timeout, otherwise \c false.

    A negative \a timeout is interpreted as infinite, so \c false is returned in
    this case. Otherwise, this is equivalent to \c {elapsed() > timeout}. You
    can do the same for a duration by comparing durationElapsed() to a duration
    timeout.

    \sa elapsed(), QDeadlineTimer
*/
bool QElapsedTimer::hasExpired(qint64 timeout) const noexcept
{
    // if timeout is -1, quint64(timeout) is LLINT_MAX, so this will be
    // considered as never expired
    return quint64(elapsed()) > quint64(timeout);
}

bool operator<(const QElapsedTimer &lhs, const QElapsedTimer &rhs) noexcept
{
    return lhs.t1 < rhs.t1;
}

QT_END_NAMESPACE
