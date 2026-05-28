// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qplatformdefs.h"
#include "qmutex.h"
#include "qmutex_p.h"

#include "private/qcore_unix_p.h"

#include <mach/mach.h>
#include <mach/task.h>

#include <errno.h>

QT_BEGIN_NAMESPACE

QMutexPrivate::QMutexPrivate()
{
    kern_return_t r = semaphore_create(mach_task_self(), &mach_semaphore, SYNC_POLICY_FIFO, 0);
    if (r != KERN_SUCCESS)
        qWarning("QMutex: failed to create semaphore, error %d", r);
}

QMutexPrivate::~QMutexPrivate()
{
    kern_return_t r = semaphore_destroy(mach_task_self(), mach_semaphore);
    if (r != KERN_SUCCESS)
        qWarning("QMutex: failed to destroy semaphore, error %d", r);
}

bool QMutexPrivate::wait(QDeadlineTimer timeout)
{
    kern_return_t r;
    if (timeout.isForever()) {
        do {
            r = semaphore_wait(mach_semaphore);
        } while (r == KERN_ABORTED);
        Q_ASSERT(r == KERN_SUCCESS);
    } else {
        timespec tv = durationToTimespec(timeout.remainingTimeAsDuration());
        mach_timespec_t ts;
        ts.tv_nsec = tv.tv_nsec;
        ts.tv_sec = tv.tv_sec;
        r = semaphore_timedwait(mach_semaphore, ts);
    }
    return (r == KERN_SUCCESS);
}

void QMutexPrivate::wakeUp() noexcept
{
    semaphore_signal(mach_semaphore);
}


QT_END_NAMESPACE
