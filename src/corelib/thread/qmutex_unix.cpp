/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2015 Olivier Goffart <ogoffart@woboq.com>
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
