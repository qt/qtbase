// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qelapsedtimer.h"
#include "qdeadlinetimer.h"
#include "qdeadlinetimer_p.h"
#include <qt_windows.h>

QT_BEGIN_NAMESPACE

// Result of QueryPerformanceFrequency
static quint64 counterFrequency = 0;

static void resolveCounterFrequency()
{
    static bool done = false;
    if (done)
        return;

    // Retrieve the number of high-resolution performance counter ticks per second
    LARGE_INTEGER frequency;
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart == 0)
        qFatal("QueryPerformanceFrequency failed, even though Microsoft documentation promises it wouldn't.");
    counterFrequency = frequency.QuadPart;

    done = true;
}

static inline qint64 ticksToNanoseconds(qint64 ticks)
{
    // QueryPerformanceCounter uses an arbitrary frequency
    qint64 seconds = ticks / counterFrequency;
    qint64 nanoSeconds = (ticks - seconds * counterFrequency) * 1000000000 / counterFrequency;
    return seconds * 1000000000 + nanoSeconds;
}


static quint64 getTickCount()
{
    resolveCounterFrequency();

    LARGE_INTEGER counter;
    bool ok = QueryPerformanceCounter(&counter);
    Q_ASSERT_X(ok, "QElapsedTimer::start()",
               "QueryPerformanceCounter failed, although QueryPerformanceFrequency succeeded.");
    Q_UNUSED(ok);
    return counter.QuadPart;
}

quint64 qt_msectime()
{
    return ticksToNanoseconds(getTickCount()) / 1000000;
}

QElapsedTimer::ClockType QElapsedTimer::clockType() noexcept
{
    resolveCounterFrequency();

    return PerformanceCounter;
}

bool QElapsedTimer::isMonotonic() noexcept
{
    return true;
}

void QElapsedTimer::start() noexcept
{
    t1 = getTickCount();
    t2 = 0;
}

qint64 QElapsedTimer::restart() noexcept
{
    qint64 oldt1 = t1;
    t1 = getTickCount();
    t2 = 0;
    return ticksToNanoseconds(t1 - oldt1) / 1000000;
}

qint64 QElapsedTimer::nsecsElapsed() const noexcept
{
    qint64 elapsed = getTickCount() - t1;
    return ticksToNanoseconds(elapsed);
}

qint64 QElapsedTimer::elapsed() const noexcept
{
    qint64 elapsed = getTickCount() - t1;
    return ticksToNanoseconds(elapsed) / 1000000;
}

qint64 QElapsedTimer::msecsSinceReference() const noexcept
{
    return ticksToNanoseconds(t1) / 1000000;
}

qint64 QElapsedTimer::msecsTo(const QElapsedTimer &other) const noexcept
{
    qint64 difference = other.t1 - t1;
    return ticksToNanoseconds(difference) / 1000000;
}

qint64 QElapsedTimer::secsTo(const QElapsedTimer &other) const noexcept
{
    return msecsTo(other) / 1000;
}

bool operator<(const QElapsedTimer &v1, const QElapsedTimer &v2) noexcept
{
    return (v1.t1 - v2.t1) < 0;
}

QDeadlineTimer QDeadlineTimer::current(Qt::TimerType timerType) noexcept
{
    static_assert(!QDeadlineTimerNanosecondsInT2);
    QDeadlineTimer result;
    result.t1 = ticksToNanoseconds(getTickCount());
    result.type = timerType;
    return result;
}

QT_END_NAMESPACE
