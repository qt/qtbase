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

static void qt_report_error(int code, const char *where, const char *what)
{
    if (code != 0)
        qErrnoWarning(code, "%s: %s failure", where, what);
}

QMutexPrivate::QMutexPrivate()
{
    qt_report_error(sem_init(&semaphore, 0, 0), "QMutex", "sem_init");
}

QMutexPrivate::~QMutexPrivate()
{

    qt_report_error(sem_destroy(&semaphore), "QMutex", "sem_destroy");
}

bool QMutexPrivate::wait(QDeadlineTimer timeout)
{
    int errorCode;
    if (timeout.isForever()) {
        do {
            errorCode = sem_wait(&semaphore);
        } while (errorCode && errno == EINTR);
        qt_report_error(errorCode, "QMutex::lock()", "sem_wait");
    } else {
        do {
            auto tp = timeout.deadline<std::chrono::system_clock>();
            timespec ts = durationToTimespec(tp.time_since_epoch());
            errorCode = sem_timedwait(&semaphore, &ts);
        } while (errorCode && errno == EINTR);

        if (errorCode && errno == ETIMEDOUT)
            return false;
        qt_report_error(errorCode, "QMutex::lock()", "sem_timedwait");
    }
    return true;
}

void QMutexPrivate::wakeUp() noexcept
{
    qt_report_error(sem_post(&semaphore), "QMutex::unlock", "sem_post");
}

QT_END_NAMESPACE
