// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qelapsedtimer.h"
#include "qdeadlinetimer.h"
#include "qdeadlinetimer_p.h"
#if defined(Q_OS_VXWORKS)
#include "qfunctions_vxworks.h"
#else
#include <sys/time.h>
#include <time.h>
#endif
#include <unistd.h>

#include <qatomic.h>
#include "private/qcore_unix_p.h"

#if defined(QT_NO_CLOCK_MONOTONIC) || defined(QT_BOOTSTRAPPED)
// turn off the monotonic clock
# ifdef _POSIX_MONOTONIC_CLOCK
#  undef _POSIX_MONOTONIC_CLOCK
# endif
# define _POSIX_MONOTONIC_CLOCK -1
#endif

QT_BEGIN_NAMESPACE

/*
 * Design:
 *
 * POSIX offers a facility to select the system's monotonic clock when getting
 * the current timestamp. Whereas the functions are mandatory in POSIX.1-2008,
 * the presence of a monotonic clock is a POSIX Option (see the document
 *  http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap02.html#tag_02_01_06 )
 *
 * The macro _POSIX_MONOTONIC_CLOCK can therefore assume the following values:
 *  -1          monotonic clock is never supported on this system
 *   0          monotonic clock might be supported, runtime check is needed
 *  >1          (such as 200809L) monotonic clock is always supported
 *
 * The unixCheckClockType() function will return the clock to use: either
 * CLOCK_MONOTONIC or CLOCK_REALTIME. In the case the POSIX option has a value
 * of zero, then this function stores a static that contains the clock to be
 * used.
 *
 * There's one extra case, which is when CLOCK_REALTIME isn't defined. When
 * that's the case, we'll emulate the clock_gettime function with gettimeofday.
 *
 * Conforming to:
 *  POSIX.1b-1993 section "Clocks and Timers"
 *  included in UNIX98 (Single Unix Specification v2)
 *  included in POSIX.1-2001
 *  see http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_getres.html
 */

#if !defined(CLOCK_REALTIME)
#  define CLOCK_REALTIME 0
static inline void qt_clock_gettime(int, struct timespec *ts)
{
    // support clock_gettime with gettimeofday
    struct timeval tv;
    gettimeofday(&tv, 0);
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
}

static inline int regularClock()
{
    return 0;
}
#else
static inline void qt_clock_gettime(clockid_t clock, struct timespec *ts)
{
    clock_gettime(clock, ts);
}

static inline clock_t regularClockCheck()
{
    struct timespec regular_clock_resolution;
    int r = -1;

#  ifdef CLOCK_MONOTONIC
    // try the monotonic clock
    r = clock_getres(CLOCK_MONOTONIC, &regular_clock_resolution);

#    ifdef Q_OS_LINUX
    // Despite glibc claiming that we should check at runtime, the Linux kernel
    // always supports the monotonic clock
    Q_ASSERT(r == 0);
    return CLOCK_MONOTONIC;
#    endif

    if (r == 0)
        return CLOCK_MONOTONIC;
#  endif

    // no monotonic, try the realtime clock
    r = clock_getres(CLOCK_REALTIME, &regular_clock_resolution);
    Q_ASSERT(r == 0);
    return CLOCK_REALTIME;
}

static inline clock_t regularClock()
{
    static const clock_t clock = regularClockCheck();
    return clock;
}
#endif

bool QElapsedTimer::isMonotonic() noexcept
{
    return clockType() == MonotonicClock;
}

QElapsedTimer::ClockType QElapsedTimer::clockType() noexcept
{
    return regularClock() == CLOCK_REALTIME ? SystemTime : MonotonicClock;
}

static inline void do_gettime(qint64 *sec, qint64 *frac)
{
    timespec ts;
    qt_clock_gettime(regularClock(), &ts);
    *sec = ts.tv_sec;
    *frac = ts.tv_nsec;
}

// used in qcore_unix.cpp and qeventdispatcher_unix.cpp
struct timespec qt_gettime() noexcept
{
    qint64 sec, frac;
    do_gettime(&sec, &frac);

    timespec tv;
    tv.tv_sec = sec;
    tv.tv_nsec = frac;

    return tv;
}

static qint64 elapsedAndRestart(qint64 sec, qint64 frac,
                                qint64 *nowsec, qint64 *nowfrac)
{
    do_gettime(nowsec, nowfrac);
    sec = *nowsec - sec;
    frac = *nowfrac - frac;
    return (sec * Q_INT64_C(1000000000) + frac) / Q_INT64_C(1000000);
}

void QElapsedTimer::start() noexcept
{
    do_gettime(&t1, &t2);
}

qint64 QElapsedTimer::restart() noexcept
{
    return elapsedAndRestart(t1, t2, &t1, &t2);
}

qint64 QElapsedTimer::nsecsElapsed() const noexcept
{
    qint64 sec, frac;
    do_gettime(&sec, &frac);
    sec = sec - t1;
    frac = frac - t2;
    return sec * Q_INT64_C(1000000000) + frac;
}

qint64 QElapsedTimer::elapsed() const noexcept
{
    return nsecsElapsed() / Q_INT64_C(1000000);
}

qint64 QElapsedTimer::msecsSinceReference() const noexcept
{
    return t1 * Q_INT64_C(1000) + t2 / Q_INT64_C(1000000);
}

qint64 QElapsedTimer::msecsTo(const QElapsedTimer &other) const noexcept
{
    qint64 secs = other.t1 - t1;
    qint64 fraction = other.t2 - t2;
    return (secs * Q_INT64_C(1000000000) + fraction) / Q_INT64_C(1000000);
}

qint64 QElapsedTimer::secsTo(const QElapsedTimer &other) const noexcept
{
    return other.t1 - t1;
}

bool operator<(const QElapsedTimer &v1, const QElapsedTimer &v2) noexcept
{
    return v1.t1 < v2.t1 || (v1.t1 == v2.t1 && v1.t2 < v2.t2);
}

QDeadlineTimer QDeadlineTimer::current(Qt::TimerType timerType) noexcept
{
    static_assert(QDeadlineTimerNanosecondsInT2);
    QDeadlineTimer result;
    qint64 cursec, curnsec;
    do_gettime(&cursec, &curnsec);
    result.t1 = cursec;
    result.t2 = curnsec;
    result.type = timerType;
    return result;
}

QT_END_NAMESPACE
