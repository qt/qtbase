// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qelapsedtimer.h"
#include "qdeadlinetimer.h"
#include "qdatetime.h"

QT_BEGIN_NAMESPACE

/*!
    Returns the clock type that this QElapsedTimer implementation uses.

    \sa isMonotonic()
*/
QElapsedTimer::ClockType QElapsedTimer::clockType() noexcept
{
    return SystemTime;
}

/*!
    Returns \c true if this is a monotonic clock, false otherwise. See the
    information on the different clock types to understand which ones are
    monotonic.

    \sa clockType(), QElapsedTimer::ClockType
*/
bool QElapsedTimer::isMonotonic() noexcept
{
    return false;
}

/*!
    Starts this timer. Once started, a timer value can be checked with elapsed() or msecsSinceReference().

    Normally, a timer is started just before a lengthy operation, such as:
    \snippet qelapsedtimer/main.cpp 0

    Also, starting a timer makes it valid again.

    \sa restart(), invalidate(), elapsed()
*/
void QElapsedTimer::start() noexcept
{
    restart();
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
    qint64 old = t1;
    t1 = QDateTime::currentMSecsSinceEpoch();
    t2 = 0;
    return t1 - old;
}

/*! \since 4.8

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
    return elapsed() * 1000000;
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
    return QDateTime::currentMSecsSinceEpoch() - t1;
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
    return t1;
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
    qint64 diff = other.t1 - t1;
    return diff;
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
    return msecsTo(other) / 1000;
}

bool operator<(const QElapsedTimer &lhs, const QElapsedTimer &rhs) noexcept
{
    return lhs.t1 < rhs.t1;
}

QDeadlineTimer QDeadlineTimer::current(Qt::TimerType timerType) noexcept
{
    QDeadlineTimer result;
    result.t1 = QDateTime::currentMSecsSinceEpoch() * 1000 * 1000;
    result.type = timerType;
    return result;
}

QT_END_NAMESPACE
