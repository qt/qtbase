// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qelapsedtimer.h>
#include <qcoreapplication.h>

#include "private/qcore_unix_p.h"
#include "private/qtimerinfo_unix_p.h"
#include "private/qobject_p.h"
#include "private/qabstracteventdispatcher_p.h"

#ifdef QTIMERINFO_DEBUG
#  include <QDebug>
#  include <QThread>
#endif

#include <sys/times.h>

using namespace std::chrono;

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT bool qt_disable_lowpriority_timers=false;

/*
 * Internal functions for manipulating timer data structures.  The
 * timerBitVec array is used for keeping track of timer identifiers.
 */

QTimerInfoList::QTimerInfoList()
{
    firstTimerInfo = nullptr;
}

timespec QTimerInfoList::updateCurrentTime()
{
    return (currentTime = qt_gettime());
}

/*
  insert timer info into list
*/
void QTimerInfoList::timerInsert(QTimerInfo *ti)
{
    int index = size();
    while (index--) {
        const QTimerInfo * const t = at(index);
        if (!(ti->timeout < t->timeout))
            break;
    }
    insert(index+1, ti);
}

static constexpr timespec roundToMillisecond(timespec val)
{
    // always round up
    // worst case scenario is that the first trigger of a 1-ms timer is 0.999 ms late

    int ns = val.tv_nsec % (1000 * 1000);
    if (ns)
        val.tv_nsec += 1000 * 1000 - ns;
    return normalizedTimespec(val);
}
static_assert(roundToMillisecond({0, 0}) == timespec{0, 0});
static_assert(roundToMillisecond({0, 1}) == timespec{0, 1'000'000});
static_assert(roundToMillisecond({0, 999'999}) == timespec{0, 1'000'000});
static_assert(roundToMillisecond({0, 1'000'000}) == timespec{0, 1'000'000});
static_assert(roundToMillisecond({0, 999'999'999}) == timespec{1, 0});
static_assert(roundToMillisecond({1, 0}) == timespec{1, 0});

static constexpr seconds roundToSecs(milliseconds msecs)
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

    auto secs = duration_cast<seconds>(msecs);
    const milliseconds frac = msecs - secs;
    if (frac >= 500ms)
        ++secs;
    return secs;
}

#ifdef QTIMERINFO_DEBUG
QDebug operator<<(QDebug s, timeval tv)
{
    QDebugStateSaver saver(s);
    s.nospace() << tv.tv_sec << "." << qSetFieldWidth(6) << qSetPadChar(QChar(48)) << tv.tv_usec << Qt::reset;
    return s;
}
QDebug operator<<(QDebug s, Qt::TimerType t)
{
    QDebugStateSaver saver(s);
    s << (t == Qt::PreciseTimer ? "P" :
          t == Qt::CoarseTimer ? "C" : "VC");
    return s;
}
#endif

static void calculateCoarseTimerTimeout(QTimerInfo *t, timespec now)
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

    auto recalculate = [&](const milliseconds fracMsec) {
        if (fracMsec == 1000ms) {
            ++t->timeout.tv_sec;
            t->timeout.tv_nsec = 0;
        } else {
            t->timeout.tv_nsec = nanoseconds{fracMsec}.count();
        }

        if (t->timeout < now)
            t->timeout += t->interval;
    };

    // Calculate how much we can round and still keep within 5% error
    const milliseconds absMaxRounding = t->interval / 20;

    auto fracMsec = duration_cast<milliseconds>(nanoseconds{t->timeout.tv_nsec});

    if (t->interval < 100ms && t->interval != 25ms && t->interval != 50ms && t->interval != 75ms) {
        auto fracCount = fracMsec.count();
        // special mode for timers of less than 100 ms
        if (t->interval < 50ms) {
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
    if ((t->interval % 500) == 0ms) {
        if (t->interval >= 5s) {
            fracMsec = fracMsec >= 500ms ? max : min;
            recalculate(fracMsec);
            return;
        } else {
            wantedBoundaryMultiple = 500ms;
        }
    } else if ((t->interval % 50) == 0ms) {
        // 4) same for multiples of 250, 200, 100, 50
        milliseconds mult50 = t->interval / 50;
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

static void calculateNextTimeout(QTimerInfo *t, timespec now)
{
    switch (t->timerType) {
    case Qt::PreciseTimer:
    case Qt::CoarseTimer:
        t->timeout += t->interval;
        if (t->timeout < now) {
            t->timeout = now;
            t->timeout += t->interval;
        }
#ifdef QTIMERINFO_DEBUG
        t->expected += t->interval;
        if (t->expected < currentTime) {
            t->expected = currentTime;
            t->expected += t->interval;
        }
#endif
        if (t->timerType == Qt::CoarseTimer)
            calculateCoarseTimerTimeout(t, now);
        return;

    case Qt::VeryCoarseTimer:
        // t->interval already rounded to full seconds in registerTimer()
        const auto secs = duration_cast<seconds>(t->interval).count();
        t->timeout.tv_sec += secs;
        if (t->timeout.tv_sec <= now.tv_sec)
            t->timeout.tv_sec = now.tv_sec + secs;
#ifdef QTIMERINFO_DEBUG
        t->expected.tv_sec += t->interval;
        if (t->expected.tv_sec <= currentTime.tv_sec)
            t->expected.tv_sec = currentTime.tv_sec + t->interval;
#endif
        return;
    }

#ifdef QTIMERINFO_DEBUG
    if (t->timerType != Qt::PreciseTimer)
    qDebug() << "timer" << t->timerType << Qt::hex << t->id << Qt::dec << "interval" << t->interval
            << "originally expected at" << t->expected << "will fire at" << t->timeout
            << "or" << (t->timeout - t->expected) << "s late";
#endif
}

/*
  Returns the time to wait for the next timer, or null if no timers
  are waiting.
*/
bool QTimerInfoList::timerWait(timespec &tm)
{
    timespec now = updateCurrentTime();

    auto isWaiting = [](QTimerInfo *tinfo) { return !tinfo->activateRef; };
    // Find first waiting timer not already active
    auto it = std::find_if(cbegin(), cend(), isWaiting);
    if (it == cend())
        return false;

    QTimerInfo *t = *it;
    if (now < t->timeout) // Time to wait
        tm = roundToMillisecond(t->timeout - now);
    else // No time to wait
        tm = {0, 0};

    return true;
}

/*
  Returns the timer's remaining time in milliseconds with the given timerId.
  If the timer id is not found in the list, the returned value will be -1.
  If the timer is overdue, the returned value will be 0.
*/
qint64 QTimerInfoList::timerRemainingTime(int timerId)
{
    return remainingDuration(timerId).count();
}

milliseconds QTimerInfoList::remainingDuration(int timerId)
{
    timespec now = updateCurrentTime();

    auto it = findTimerById(timerId);
    if (it == cend()) {
#ifndef QT_NO_DEBUG
        qWarning("QTimerInfoList::timerRemainingTime: timer id %i not found", timerId);
#endif
        return milliseconds{-1};
    }

    const QTimerInfo *t = *it;
    if (now < t->timeout) // time to wait
        return timespecToChronoMs(roundToMillisecond(t->timeout - now));
    else
        return milliseconds{0};
}

void QTimerInfoList::registerTimer(int timerId, qint64 interval, Qt::TimerType timerType, QObject *object)
{
    registerTimer(timerId, milliseconds{interval}, timerType, object);
}

void QTimerInfoList::registerTimer(int timerId, milliseconds interval,
                                   Qt::TimerType timerType, QObject *object)
{
    QTimerInfo *t = new QTimerInfo;
    t->id = timerId;
    t->interval = interval;
    t->timerType = timerType;
    t->obj = object;
    t->activateRef = nullptr;

    timespec expected = updateCurrentTime() + interval;

    switch (timerType) {
    case Qt::PreciseTimer:
        // high precision timer is based on millisecond precision
        // so no adjustment is necessary
        t->timeout = expected;
        break;

    case Qt::CoarseTimer:
        // this timer has up to 5% coarseness
        // so our boundaries are 20 ms and 20 s
        // below 20 ms, 5% inaccuracy is below 1 ms, so we convert to high precision
        // above 20 s, 5% inaccuracy is above 1 s, so we convert to VeryCoarseTimer
        if (interval >= 20s) {
            t->timerType = Qt::VeryCoarseTimer;
        } else {
            t->timeout = expected;
            if (interval <= 20ms) {
                t->timerType = Qt::PreciseTimer;
                // no adjustment is necessary
            } else if (interval <= 20s) {
                calculateCoarseTimerTimeout(t, currentTime);
            }
            break;
        }
        Q_FALLTHROUGH();
    case Qt::VeryCoarseTimer:
        const seconds secs = roundToSecs(t->interval);
        t->interval = secs;
        t->timeout.tv_sec = currentTime.tv_sec + secs.count();
        t->timeout.tv_nsec = 0;

        // if we're past the half-second mark, increase the timeout again
        if (currentTime.tv_nsec > nanoseconds{500ms}.count())
            ++t->timeout.tv_sec;
    }

    timerInsert(t);

#ifdef QTIMERINFO_DEBUG
    t->expected = expected;
    t->cumulativeError = 0;
    t->count = 0;
    if (t->timerType != Qt::PreciseTimer)
    qDebug() << "timer" << t->timerType << Qt::hex <<t->id << Qt::dec << "interval" << t->interval << "expected at"
            << t->expected << "will fire first at" << t->timeout;
#endif
}

bool QTimerInfoList::unregisterTimer(int timerId)
{
    auto it = findTimerById(timerId);
    if (it == cend())
        return false; // id not found

    // set timer inactive
    QTimerInfo *t = *it;
    if (t == firstTimerInfo)
        firstTimerInfo = nullptr;
    if (t->activateRef)
        *(t->activateRef) = nullptr;
    delete t;
    erase(it);
    return true;
}

bool QTimerInfoList::unregisterTimers(QObject *object)
{
    if (isEmpty())
        return false;
    for (int i = 0; i < size(); ++i) {
        QTimerInfo *t = at(i);
        if (t->obj == object) {
            // object found
            removeAt(i);
            if (t == firstTimerInfo)
                firstTimerInfo = nullptr;
            if (t->activateRef)
                *(t->activateRef) = nullptr;
            delete t;
            // move back one so that we don't skip the new current item
            --i;
        }
    }
    return true;
}

QList<QAbstractEventDispatcher::TimerInfo> QTimerInfoList::registeredTimers(QObject *object) const
{
    QList<QAbstractEventDispatcher::TimerInfo> list;
    for (const QTimerInfo *const t : std::as_const(*this)) {
        if (t->obj == object)
            list.emplaceBack(t->id, t->interval.count(), t->timerType);
    }
    return list;
}

/*
    Activate pending timers, returning how many where activated.
*/
int QTimerInfoList::activateTimers()
{
    if (qt_disable_lowpriority_timers || isEmpty())
        return 0; // nothing to do

    firstTimerInfo = nullptr;

    timespec now = updateCurrentTime();
    // qDebug() << "Thread" << QThread::currentThreadId() << "woken up at" << now;
    // Find out how many timer have expired
    auto stillActive = [&now](const QTimerInfo *t) { return now < t->timeout; };
    // Find first one still active (list is sorted by timeout)
    auto it = std::find_if(cbegin(), cend(), stillActive);
    auto maxCount = it - cbegin();

    int n_act = 0;
    //fire the timers.
    while (maxCount--) {
        if (isEmpty())
            break;

        QTimerInfo *currentTimerInfo = constFirst();
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

        // remove from list
        removeFirst();

#ifdef QTIMERINFO_DEBUG
        float diff;
        if (currentTime < currentTimerInfo->expected) {
            // early
            timeval early = currentTimerInfo->expected - currentTime;
            diff = -(early.tv_sec + early.tv_usec / 1000000.0);
        } else {
            timeval late = currentTime - currentTimerInfo->expected;
            diff = late.tv_sec + late.tv_usec / 1000000.0;
        }
        currentTimerInfo->cumulativeError += diff;
        ++currentTimerInfo->count;
        if (currentTimerInfo->timerType != Qt::PreciseTimer)
        qDebug() << "timer" << currentTimerInfo->timerType << Qt::hex << currentTimerInfo->id << Qt::dec << "interval"
                << currentTimerInfo->interval << "firing at" << currentTime
                << "(orig" << currentTimerInfo->expected << "scheduled at" << currentTimerInfo->timeout
                << ") off by" << diff << "activation" << currentTimerInfo->count
                << "avg error" << (currentTimerInfo->cumulativeError / currentTimerInfo->count);
#endif

        // determine next timeout time
        calculateNextTimeout(currentTimerInfo, now);

        // reinsert timer
        timerInsert(currentTimerInfo);
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
