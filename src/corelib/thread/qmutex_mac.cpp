/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qplatformdefs.h"
#include "qmutex.h"
#include "qmutex_p.h"

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

bool QMutexPrivate::wait(int timeout)
{
    kern_return_t r;
    if (timeout < 0) {
        do {
            r = semaphore_wait(mach_semaphore);
        } while (r == KERN_ABORTED);
        Q_ASSERT(r == KERN_SUCCESS);
    } else {
        mach_timespec_t ts;
        ts.tv_nsec = ((timeout % 1000) * 1000) * 1000;
        ts.tv_sec = (timeout / 1000);
        r = semaphore_timedwait(mach_semaphore, ts);
    }
    return (r == KERN_SUCCESS);
}

void QMutexPrivate::wakeUp() noexcept
{
    semaphore_signal(mach_semaphore);
}


QT_END_NAMESPACE
