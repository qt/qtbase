// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qelapsedtimer.h>
#include <qcoreapplication.h>

#include "private/qcore_unix_p.h"
#include "private/qtimerinfo_unix_p.h"
#include "private/qobject_p.h"
#include "private/qabstracteventdispatcher_p.h"

#include <sys/times.h>

using namespace std::chrono;
// Implied by "using namespace std::chrono", but be explicit about it, for grep-ability
using namespace std::chrono_literals;

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT bool qt_disable_lowpriority_timers=false;

/*
 * Internal functions for manipulating timer data structures.  The
 * timerBitVec array is used for keeping track of timer identifiers.
 */

QTimerInfoList::QTimerInfoList() = default;

steady_clock::time_point QTimerInfoList::updateCurrentTime() const
{
    currentTime = steady_clock::now();
    return currentTime;
}

/*! \internal
    Updates the currentTime member to the current time, and returns \c true if
    the first timer's timeout is in the future (after currentTime).

    The list is sorted by timeout, thus it's enough to check the first timer only.
*/
bool QTimerInfoList::hasPendingTimers()
{
    if (timers.isEmpty())
        return false;
    return updateCurrentTime() < timers.at(0)->timeout;
}

static bool byTimeout(const QTimerInfo *a, const QTimerInfo *b)
{ return a->timeout < b->timeout; };

/*
  insert timer info into list
*/
void QTimerInfoList::timerInsert(QTimerInfo *ti)
{
    timers.insert(std::upper_bound(timers.cbegin(), timers.cend(), ti, byTimeout),
                  ti);
}

static constexpr milliseconds roundToMillisecond(nanoseconds val)
{
    // always round up
    // worst case scenario is that the first trigger of a 1-ms timer is 0.999 ms late
    return ceil<milliseconds>(val);
}

static_assert(roundToMillisecond(0ns) == 0ms);
static_assert(roundToMillisecond(1ns) == 1ms);
static_assert(roundToMillisecond(999'999ns) == 1ms);
static_assert(roundToMillisecond(1'000'000ns) == 1ms);
static_assert(roundToMillisecond(999'000'000ns) == 999ms);
static_assert(roundToMillisecond(999'000'001ns) == 1000ms);
static_assert(roundToMillisecond(999'999'999ns) == 1000ms);
static_assert(roundToMillisecond(1s) == 1s);

static constexpr seconds roundToSecs(nanoseconds interval)
{
    // The very coarse timer is based on full second precision, so we want to
    // round the interval to the closest second, rounding 500ms up to 1s.
    //
    // std::chrono::round() wouldn't work with all multiples of 500 because for the
    // middle point it would round to even:
    // value  round()  wanted
    // 500      0        1
    // 1500     2        2
    // 2500     2        3

    auto secs = duration_cast<seconds>(interval);
    const nanoseconds frac = interval - secs;
    if (frac >= 500ms)
        ++secs;
    return secs;
}

static void calculateCoarseTimerTimeout(QTimerInfo *t, steady_clock::time_point now)
{
    // The coarse timer works like this:
    //  - interval under 40 ms: round to even
    //  - between 40 and 99 ms: round to multiple of 4
    //  - otherwise: try to wake up at a multiple of 25 ms, with a maximum error of 5%
    //
    // We try to wake up at the following second-fraction, in order of preference:
    //    0 ms
    //  500 ms
    //  250 ms or 750 ms
    //  200, 400, 600, 800 ms
    //  other multiples of 100
    //  other multiples of 50
    //  other multiples of 25
    //
    // The objective is to make most timers wake up at the same time, thereby reducing CPU wakeups.

    Q_ASSERT(t->interval >= 20ms);

    const auto timeoutInSecs = time_point_cast<seconds>(t->timeout);

    auto recalculate = [&](const milliseconds frac) {
        t->timeout = timeoutInSecs + frac;
        if (t->timeout < now)
            t->timeout += t->interval;
    };

    // Calculate how much we can round and still keep within 5% error
    milliseconds interval = roundToMillisecond(t->interval);
    const milliseconds absMaxRounding = interval / 20;

    auto fracMsec = duration_cast<milliseconds>(t->timeout - timeoutInSecs);

    if (interval < 100ms && interval != 25ms && interval != 50ms && interval != 75ms) {
        auto fracCount = fracMsec.count();
        // special mode for timers of less than 100 ms
        if (interval < 50ms) {
            // round to even
            // round towards multiples of 50 ms
            bool roundUp = (fracCount % 50) >= 25;
            fracCount >>= 1;
            fracCount |= roundUp;
            fracCount <<= 1;
        } else {
            // round to multiple of 4
            // round towards multiples of 100 ms
            bool roundUp = (fracCount % 100) >= 50;
            fracCount >>= 2;
            fracCount |= roundUp;
            fracCount <<= 2;
        }
        fracMsec = milliseconds{fracCount};
        recalculate(fracMsec);
        return;
    }

    milliseconds min = std::max(0ms, fracMsec - absMaxRounding);
    milliseconds max = std::min(1000ms, fracMsec + absMaxRounding);

    // find the boundary that we want, according to the rules above
    // extra rules:
    // 1) whatever the interval, we'll take any round-to-the-second timeout
    if (min == 0ms) {
        fracMsec = 0ms;
        recalculate(fracMsec);
        return;
    } else if (max == 1000ms) {
        fracMsec = 1000ms;
        recalculate(fracMsec);
        return;
    }

    milliseconds wantedBoundaryMultiple{25};

    // 2) if the interval is a multiple of 500 ms and > 5000 ms, we'll always round
    //    towards a round-to-the-second
    // 3) if the interval is a multiple of 500 ms, we'll round towards the nearest
    //    multiple of 500 ms
    if ((interval % 500) == 0ms) {
        if (interval >= 5s) {
            fracMsec = fracMsec >= 500ms ? max : min;
            recalculate(fracMsec);
            return;
        } else {
            wantedBoundaryMultiple = 500ms;
        }
    } else if ((interval % 50) == 0ms) {
        // 4) same for multiples of 250, 200, 100, 50
        milliseconds mult50 = interval / 50;
        if ((mult50 % 4) == 0ms) {
            // multiple of 200
            wantedBoundaryMultiple = 200ms;
        } else if ((mult50 % 2) == 0ms) {
            // multiple of 100
            wantedBoundaryMultiple = 100ms;
        } else if ((mult50 % 5) == 0ms) {
            // multiple of 250
            wantedBoundaryMultiple = 250ms;
        } else {
            // multiple of 50
            wantedBoundaryMultiple = 50ms;
        }
    }

    milliseconds base = (fracMsec / wantedBoundaryMultiple) * wantedBoundaryMultiple;
    milliseconds middlepoint = base + wantedBoundaryMultiple / 2;
    if (fracMsec < middlepoint)
        fracMsec = qMax(base, min);
    else
        fracMsec = qMin(base + wantedBoundaryMultiple, max);

    recalculate(fracMsec);
}

static void calculateNextTimeout(QTimerInfo *t, steady_clock::time_point now)
{
    switch (t->timerType) {
    case Qt::PreciseTimer:
    case Qt::CoarseTimer:
        t->timeout += t->interval;
        if (t->timeout < now) {
            t->timeout = now;
            t->timeout += t->interval;
        }
        if (t->timerType == Qt::CoarseTimer)
            calculateCoarseTimerTimeout(t, now);
        return;

    case Qt::VeryCoarseTimer:
        // t->interval already rounded to full seconds in registerTimer()
        t->timeout += t->interval;
        if (t->timeout <= now)
            t->timeout = time_point_cast<seconds>(now + t->interval);
        break;
    }
}

/*
    Returns the time to wait for the first timer that has not been activated yet,
    otherwise returns std::nullopt.
 */
std::optional<QTimerInfoList::Duration> QTimerInfoList::timerWait()
{
    steady_clock::time_point now = updateCurrentTime();

    auto isWaiting = [](QTimerInfo *tinfo) { return !tinfo->activateRef; };
    // Find first waiting timer not already active
    auto it = std::find_if(timers.cbegin(), timers.cend(), isWaiting);
    if (it == timers.cend())
        return std::nullopt;

    Duration timeToWait = (*it)->timeout - now;
    if (timeToWait > 0ns)
        return roundToMillisecond(timeToWait);
    return 0ms;
}

/*
  Returns the timer's remaining time in milliseconds with the given timerId.
  If the timer id is not found in the list, the returned value will be \c{Duration::min()}.
  If the timer is overdue, the returned value will be 0.
*/
QTimerInfoList::Duration QTimerInfoList::remainingDuration(Qt::TimerId timerId) const
{
    const steady_clock::time_point now = updateCurrentTime();

    auto it = findTimerById(timerId);
    if (it == timers.cend()) {
#ifndef QT_NO_DEBUG
        qWarning("QTimerInfoList::timerRemainingTime: timer id %i not found", int(timerId));
#endif
        return Duration::min();
    }

    const QTimerInfo *t = *it;
    if (now < t->timeout) // time to wait
        return t->timeout - now;
    return 0ms;
}

void QTimerInfoList::registerTimer(Qt::TimerId timerId, QTimerInfoList::Duration interval,
                                   Qt::TimerType timerType, QObject *object)
{
    // correct the timer type first
    if (timerType == Qt::CoarseTimer) {
        // this timer has up to 5% coarseness
        // so our boundaries are 20 ms and 20 s
        // below 20 ms, 5% inaccuracy is below 1 ms, so we convert to high precision
        // above 20 s, 5% inaccuracy is above 1 s, so we convert to VeryCoarseTimer
        if (interval >= 20s)
            timerType = Qt::VeryCoarseTimer;
        else if (interval <= 20ms)
            timerType = Qt::PreciseTimer;
    }

    QTimerInfo *t = new QTimerInfo(timerId, interval, timerType, object);
    QTimerInfo::TimePoint expected = updateCurrentTime() + interval;

    switch (timerType) {
    case Qt::PreciseTimer:
        // high precision timer is based on millisecond precision
        // so no adjustment is necessary
        t->timeout = expected;
        break;

    case Qt::CoarseTimer:
        t->timeout = expected;
        t->interval = roundToMillisecond(interval);
        calculateCoarseTimerTimeout(t, currentTime);
        break;

    case Qt::VeryCoarseTimer:
        t->interval = roundToSecs(t->interval);
        const auto currentTimeInSecs = floor<seconds>(currentTime);
        t->timeout = currentTimeInSecs + t->interval;
        // If we're past the half-second mark, increase the timeout again
        if (currentTime - currentTimeInSecs > 500ms)
            t->timeout += 1s;
    }

    timerInsert(t);
}

bool QTimerInfoList::unregisterTimer(Qt::TimerId timerId)
{
    auto it = findTimerById(timerId);
    if (it == timers.cend())
        return false; // id not found

    // set timer inactive
    QTimerInfo *t = *it;
    if (t == firstTimerInfo)
        firstTimerInfo = nullptr;
    if (t->activateRef)
        *(t->activateRef) = nullptr;
    delete t;
    timers.erase(it);
    return true;
}

bool QTimerInfoList::unregisterTimers(QObject *object)
{
    if (timers.isEmpty())
        return false;

    auto associatedWith = [this](QObject *o) {
        return [this, o](auto &t) {
            if (t->obj == o) {
                if (t == firstTimerInfo)
                    firstTimerInfo = nullptr;
                if (t->activateRef)
                    *(t->activateRef) = nullptr;
                delete t;
                return true;
            }
            return false;
        };
    };

    qsizetype count = timers.removeIf(associatedWith(object));
    return count > 0;
}

auto QTimerInfoList::registeredTimers(QObject *object) const -> QList<TimerInfo>
{
    QList<TimerInfo> list;
    for (const auto &t : timers) {
        if (t->obj == object)
            list.emplaceBack(TimerInfo{t->interval, t->id, t->timerType});
    }
    return list;
}

/*
    Activate pending timers, returning how many where activated.
*/
int QTimerInfoList::activateTimers()
{
    if (qt_disable_lowpriority_timers || timers.isEmpty())
        return 0; // nothing to do

    firstTimerInfo = nullptr;

    const steady_clock::time_point now = updateCurrentTime();
    // qDebug() << "Thread" << QThread::currentThreadId() << "woken up at" << now;
    // Find out how many timer have expired
    auto stillActive = [&now](const QTimerInfo *t) { return now < t->timeout; };
    // Find first one still active (list is sorted by timeout)
    auto it = std::find_if(timers.cbegin(), timers.cend(), stillActive);
    auto maxCount = it - timers.cbegin();

    int n_act = 0;
    //fire the timers.
    while (maxCount--) {
        if (timers.isEmpty())
            break;

        QTimerInfo *currentTimerInfo = timers.constFirst();
        if (now < currentTimerInfo->timeout)
            break; // no timer has expired

        if (!firstTimerInfo) {
            firstTimerInfo = currentTimerInfo;
        } else if (firstTimerInfo == currentTimerInfo) {
            // avoid sending the same timer multiple times
            break;
        } else if (currentTimerInfo->interval <  firstTimerInfo->interval
                   || currentTimerInfo->interval == firstTimerInfo->interval) {
            firstTimerInfo = currentTimerInfo;
        }

        // determine next timeout time
        calculateNextTimeout(currentTimerInfo, now);
        if (timers.size() > 1) {
            // Find where "currentTimerInfo" should be in the list so as
            // to keep the list ordered by timeout
            auto afterCurrentIt = timers.begin() + 1;
            auto iter = std::upper_bound(afterCurrentIt, timers.end(), currentTimerInfo, byTimeout);
            currentTimerInfo = *std::rotate(timers.begin(), afterCurrentIt, iter);
        }

        if (currentTimerInfo->interval > 0ms)
            n_act++;

        // Send event, but don't allow it to recurse:
        if (!currentTimerInfo->activateRef) {
            currentTimerInfo->activateRef = &currentTimerInfo;

            QTimerEvent e(currentTimerInfo->id);
            QCoreApplication::sendEvent(currentTimerInfo->obj, &e);

            // Storing currentTimerInfo's address in its activateRef allows the
            // handling of that event to clear this local variable on deletion
            // of the object it points to - if it didn't, clear activateRef:
            if (currentTimerInfo)
                currentTimerInfo->activateRef = nullptr;
        }
    }

    firstTimerInfo = nullptr;
    // qDebug() << "Thread" << QThread::currentThreadId() << "activated" << n_act << "timers";
    return n_act;
}

QT_END_NAMESPACE
