/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformdefs.h"
#include "qmutex.h"

#ifndef QT_NO_THREAD
#include "qatomic.h"
#include "qmutex_p.h"
# include "qelapsedtimer.h"

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>

QT_BEGIN_NAMESPACE

static inline int _q_futex(void *addr, int op, int val, const struct timespec *timeout)
{
    volatile int *int_addr = reinterpret_cast<volatile int *>(addr);
#if Q_BYTE_ORDER == Q_BIG_ENDIAN && QT_POINTER_SIZE == 8
    int_addr++; //We want a pointer to the 32 least significant bit of QMutex::d
#endif
    int *addr2 = 0;
    int val2 = 0;
    return syscall(SYS_futex, int_addr, op, val, timeout, addr2, val2);
}

static inline QMutexPrivate *dummyFutexValue()
{
    return reinterpret_cast<QMutexPrivate *>(quintptr(3));
}


QMutexPrivate::~QMutexPrivate() {}
QMutexPrivate::QMutexPrivate(QMutex::RecursionMode mode)
    : recursive(mode == QMutex::Recursive) {}

bool QBasicMutex::lockInternal(int timeout)
{
    QElapsedTimer elapsedTimer;
    if (timeout >= 1)
        elapsedTimer.start();

    while (!fastTryLock()) {
        QMutexPrivate *d = this->d.load();
        if (!d) // if d is 0, the mutex is unlocked
            continue;

        if (quintptr(d) <= 0x3) { //d == dummyLocked() || d == dummyFutexValue()
            if (timeout == 0)
                return false;
            while (this->d.fetchAndStoreAcquire(dummyFutexValue()) != 0) {
                struct timespec ts, *pts = 0;
                if (timeout >= 1) {
                    // recalculate the timeout
                    qint64 xtimeout = timeout * 1000 * 1000;
                    xtimeout -= elapsedTimer.nsecsElapsed();
                    if (xtimeout <= 0) {
                        // timer expired after we returned
                        return false;
                    }
                    ts.tv_sec = xtimeout / Q_INT64_C(1000) / 1000 / 1000;
                    ts.tv_nsec = xtimeout % (Q_INT64_C(1000) * 1000 * 1000);
                    pts = &ts;
                }
                int r = _q_futex(&this->d, FUTEX_WAIT, quintptr(dummyFutexValue()), pts);
                if (r != 0 && errno == ETIMEDOUT)
                    return false;
            }
            return true;
        }
        Q_ASSERT(d->recursive);
        return static_cast<QRecursiveMutexPrivate *>(d)->lock(timeout);
    }
    Q_ASSERT(this->d.load());
    return true;
}

void QBasicMutex::unlockInternal()
{
    QMutexPrivate *d = this->d.load();
    Q_ASSERT(d); //we must be locked
    Q_ASSERT(d != dummyLocked()); // testAndSetRelease(dummyLocked(), 0) failed

    if (d == dummyFutexValue()) {
        this->d.fetchAndStoreRelease(0);
        _q_futex(&this->d, FUTEX_WAKE, 1, 0);
        return;
    }

    Q_ASSERT(d->recursive);
    static_cast<QRecursiveMutexPrivate *>(d)->unlock();
}


QT_END_NAMESPACE

#endif // QT_NO_THREAD
