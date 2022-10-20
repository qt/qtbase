/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qelapsedtimer.h"

QT_BEGIN_NAMESPACE

/*!
    \class QElapsedTimer
    \inmodule QtCore
    \brief The QElapsedTimer class provides a fast way to calculate elapsed times.
    \since 4.7

    \reentrant
    \ingroup tools

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

    \sa QTime, QTimer, QDeadlineTimer
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
    \fn bool operator<(const QElapsedTimer &lhs, const QElapsedTimer &rhs) noexcept
    \relates QElapsedTimer

    Returns \c true if \a lhs was started before \a rhs, false otherwise.

    The returned value is undefined if one of the two parameters is invalid
    and the other isn't. However, two invalid timers are equal and thus this
    function will return false.
*/

static const qint64 invalidData = Q_INT64_C(0x8000000000000000);

/*!
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
    Returns \c true if this QElapsedTimer has already expired by \a timeout
    milliseconds (that is, more than \a timeout milliseconds have elapsed).
    The value of \a timeout can be -1 to indicate that this timer does not
    expire, in which case this function will always return false.

    \sa elapsed(), QDeadlineTimer
*/
bool QElapsedTimer::hasExpired(qint64 timeout) const noexcept
{
    // if timeout is -1, quint64(timeout) is LLINT_MAX, so this will be
    // considered as never expired
    return quint64(elapsed()) > quint64(timeout);
}

QT_END_NAMESPACE
