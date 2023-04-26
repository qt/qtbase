// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaitcondition.h"

#include "qatomic.h"
#include "qdeadlinetimer.h"
#include "qelapsedtimer.h"
#include "qmutex.h"
#include "qplatformdefs.h"
#include "qreadwritelock.h"
#include "qstring.h"

#include "private/qcore_unix_p.h"
#include "qreadwritelock_p.h"

#include <errno.h>
#include <sys/time.h>
#include <time.h>

QT_BEGIN_NAMESPACE

static constexpr clockid_t SteadyClockClockId =
#if !defined(CLOCK_MONOTONIC)
        // we don't know how to set the monotonic clock
        CLOCK_REALTIME
#elif defined(_LIBCPP_VERSION) && defined(_LIBCPP_HAS_NO_MONOTONIC_CLOCK)
        // libc++ falling back to system_clock
        CLOCK_REALTIME
#elif defined(__GLIBCXX__) && !defined(_GLIBCXX_USE_CLOCK_MONOTONIC)
        // libstdc++ falling back to system_clock
        CLOCK_REALTIME
#elif defined(Q_OS_DARWIN)
        // Darwin lacks pthread_condattr_setclock()
        CLOCK_REALTIME
#elif defined(Q_OS_QNX)
        // unknown why
        CLOCK_REALTIME
#elif defined(__GLIBCXX__) || defined(_LIBCPP_VERSION)
        // both libstdc++ and libc++ do use CLOCK_MONOTONIC
        CLOCK_MONOTONIC
#else
#  warning "Unknown C++ Standard Library implementation - code may be sub-optimal"
        CLOCK_REALTIME
#endif
        ;

static void qt_report_pthread_error(int code, const char *where, const char *what)
{
    if (code != 0)
        qErrnoWarning(code, "%s: %s failure", where, what);
}

static void qt_initialize_pthread_cond(pthread_cond_t *cond, const char *where)
{
    pthread_condattr_t *attrp = nullptr;

#if defined(CLOCK_MONOTONIC) && !defined(Q_OS_DARWIN)
    pthread_condattr_t condattr;
    attrp = &condattr;

    pthread_condattr_init(&condattr);
    auto destroy = qScopeGuard([&] { pthread_condattr_destroy(&condattr); });
    if (SteadyClockClockId != CLOCK_REALTIME)
        pthread_condattr_setclock(&condattr, SteadyClockClockId);
#endif

    qt_report_pthread_error(pthread_cond_init(cond, attrp), where, "cv init");
}

static void qt_abstime_for_timeout(timespec *ts, QDeadlineTimer deadline)
{
    using namespace std::chrono;
    using Clock =
        std::conditional_t<SteadyClockClockId == CLOCK_REALTIME, system_clock, steady_clock>;
    auto timePoint = deadline.deadline<Clock>();
    *ts = durationToTimespec(timePoint.time_since_epoch());
}

class QWaitConditionPrivate
{
public:
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int waiters;
    int wakeups;

    int wait_relative(QDeadlineTimer deadline)
    {
        timespec ti;
        qt_abstime_for_timeout(&ti, deadline);
        return pthread_cond_timedwait(&cond, &mutex, &ti);
    }

    bool wait(QDeadlineTimer deadline)
    {
        int code;
        forever {
            if (!deadline.isForever()) {
                code = wait_relative(deadline);
            } else {
                code = pthread_cond_wait(&cond, &mutex);
            }
            if (code == 0 && wakeups == 0) {
                // spurious wakeup
                continue;
            }
            break;
        }

        Q_ASSERT_X(waiters > 0, "QWaitCondition::wait", "internal error (waiters)");
        --waiters;
        if (code == 0) {
            Q_ASSERT_X(wakeups > 0, "QWaitCondition::wait", "internal error (wakeups)");
            --wakeups;
        }
        qt_report_pthread_error(pthread_mutex_unlock(&mutex), "QWaitCondition::wait()",
                                "mutex unlock");

        if (code && code != ETIMEDOUT)
            qt_report_pthread_error(code, "QWaitCondition::wait()", "cv wait");

        return (code == 0);
    }
};

QWaitCondition::QWaitCondition()
{
    d = new QWaitConditionPrivate;
    qt_report_pthread_error(pthread_mutex_init(&d->mutex, nullptr), "QWaitCondition", "mutex init");
    qt_initialize_pthread_cond(&d->cond, "QWaitCondition");
    d->waiters = d->wakeups = 0;
}

QWaitCondition::~QWaitCondition()
{
    qt_report_pthread_error(pthread_cond_destroy(&d->cond), "QWaitCondition", "cv destroy");
    qt_report_pthread_error(pthread_mutex_destroy(&d->mutex), "QWaitCondition", "mutex destroy");
    delete d;
}

void QWaitCondition::wakeOne()
{
    qt_report_pthread_error(pthread_mutex_lock(&d->mutex), "QWaitCondition::wakeOne()",
                            "mutex lock");
    d->wakeups = qMin(d->wakeups + 1, d->waiters);
    qt_report_pthread_error(pthread_cond_signal(&d->cond), "QWaitCondition::wakeOne()",
                            "cv signal");
    qt_report_pthread_error(pthread_mutex_unlock(&d->mutex), "QWaitCondition::wakeOne()",
                            "mutex unlock");
}

void QWaitCondition::wakeAll()
{
    qt_report_pthread_error(pthread_mutex_lock(&d->mutex), "QWaitCondition::wakeAll()",
                            "mutex lock");
    d->wakeups = d->waiters;
    qt_report_pthread_error(pthread_cond_broadcast(&d->cond), "QWaitCondition::wakeAll()",
                            "cv broadcast");
    qt_report_pthread_error(pthread_mutex_unlock(&d->mutex), "QWaitCondition::wakeAll()",
                            "mutex unlock");
}

bool QWaitCondition::wait(QMutex *mutex, unsigned long time)
{
    if (time == std::numeric_limits<unsigned long>::max())
        return wait(mutex, QDeadlineTimer(QDeadlineTimer::Forever));
    return wait(mutex, QDeadlineTimer(time));
}

bool QWaitCondition::wait(QMutex *mutex, QDeadlineTimer deadline)
{
    if (!mutex)
        return false;

    qt_report_pthread_error(pthread_mutex_lock(&d->mutex), "QWaitCondition::wait()", "mutex lock");
    ++d->waiters;
    mutex->unlock();

    bool returnValue = d->wait(deadline);

    mutex->lock();

    return returnValue;
}

bool QWaitCondition::wait(QReadWriteLock *readWriteLock, unsigned long time)
{
    if (time == std::numeric_limits<unsigned long>::max())
        return wait(readWriteLock, QDeadlineTimer(QDeadlineTimer::Forever));
    return wait(readWriteLock, QDeadlineTimer(time));
}

bool QWaitCondition::wait(QReadWriteLock *readWriteLock, QDeadlineTimer deadline)
{
    using namespace QReadWriteLockStates;

    if (!readWriteLock)
        return false;
    auto previousState = QReadWriteLockPrivate::stateForWaitCondition(readWriteLock);
    if (previousState == Unlocked)
        return false;
    if (previousState == RecursivelyLocked) {
        qWarning("QWaitCondition: cannot wait on QReadWriteLocks with recursive lockForWrite()");
        return false;
    }

    qt_report_pthread_error(pthread_mutex_lock(&d->mutex), "QWaitCondition::wait()", "mutex lock");
    ++d->waiters;

    readWriteLock->unlock();

    bool returnValue = d->wait(deadline);

    if (previousState == LockedForWrite)
        readWriteLock->lockForWrite();
    else
        readWriteLock->lockForRead();

    return returnValue;
}

QT_END_NAMESPACE
