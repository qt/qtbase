// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformdefs.h"
#include "qmutex.h"
#include "qstring.h"
#include "qelapsedtimer.h"
#include "qatomic.h"
#include "qmutex_p.h"
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include "private/qcore_unix_p.h"

#if defined(Q_OS_VXWORKS) && defined(wakeup)
#undef wakeup
#endif

QT_BEGIN_NAMESPACE

static void report_error(int code, const char *where, const char *what)
{
    if (code != 0)
        qErrnoWarning(code, "%s: %s failure", where, what);
}

#ifdef QT_UNIX_SEMAPHORE

QMutexPrivate::QMutexPrivate()
{
    report_error(sem_init(&semaphore, 0, 0), "QMutex", "sem_init");
}

QMutexPrivate::~QMutexPrivate()
{

    report_error(sem_destroy(&semaphore), "QMutex", "sem_destroy");
}

bool QMutexPrivate::wait(int timeout)
{
    int errorCode;
    if (timeout < 0) {
        do {
            errorCode = sem_wait(&semaphore);
        } while (errorCode && errno == EINTR);
        report_error(errorCode, "QMutex::lock()", "sem_wait");
    } else {
        timespec ts;
        report_error(clock_gettime(CLOCK_REALTIME, &ts), "QMutex::lock()", "clock_gettime");
        ts.tv_sec += timeout / 1000;
        ts.tv_nsec += timeout % 1000 * Q_UINT64_C(1000) * 1000;
        normalizedTimespec(ts);
        do {
            errorCode = sem_timedwait(&semaphore, &ts);
        } while (errorCode && errno == EINTR);

        if (errorCode && errno == ETIMEDOUT)
            return false;
        report_error(errorCode, "QMutex::lock()", "sem_timedwait");
    }
    return true;
}

void QMutexPrivate::wakeUp() noexcept
{
    report_error(sem_post(&semaphore), "QMutex::unlock", "sem_post");
}

#else // QT_UNIX_SEMAPHORE

QMutexPrivate::QMutexPrivate()
    : wakeup(false)
{
    report_error(pthread_mutex_init(&mutex, NULL), "QMutex", "mutex init");
    qt_initialize_pthread_cond(&cond, "QMutex");
}

QMutexPrivate::~QMutexPrivate()
{
    report_error(pthread_cond_destroy(&cond), "QMutex", "cv destroy");
    report_error(pthread_mutex_destroy(&mutex), "QMutex", "mutex destroy");
}

bool QMutexPrivate::wait(int timeout)
{
    report_error(pthread_mutex_lock(&mutex), "QMutex::lock", "mutex lock");
    int errorCode = 0;
    while (!wakeup) {
        if (timeout < 0) {
            errorCode = pthread_cond_wait(&cond, &mutex);
        } else {
            timespec ti;
            qt_abstime_for_timeout(&ti, QDeadlineTimer(timeout));
            errorCode = pthread_cond_timedwait(&cond, &mutex, &ti);
        }
        if (errorCode) {
            if (errorCode == ETIMEDOUT) {
                if (wakeup)
                    errorCode = 0;
                break;
            }
            report_error(errorCode, "QMutex::lock()", "cv wait");
        }
    }
    bool ret = wakeup;
    wakeup = false;
    report_error(pthread_mutex_unlock(&mutex), "QMutex::lock", "mutex unlock");
    return ret;
}

void QMutexPrivate::wakeUp() noexcept
{
    report_error(pthread_mutex_lock(&mutex), "QMutex::unlock", "mutex lock");
    wakeup = true;
    report_error(pthread_cond_signal(&cond), "QMutex::unlock", "cv signal");
    report_error(pthread_mutex_unlock(&mutex), "QMutex::unlock", "mutex unlock");
}

#endif

QT_END_NAMESPACE
