// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// Copyright (C) 2012 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMUTEX_P_H
#define QMUTEX_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience of
// qmutex.cpp and qmutex_unix.cpp. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qmutex.h>
#include <QtCore/qatomic.h>
#include <QtCore/qdeadlinetimer.h>

#if defined(Q_OS_MAC)
# include <mach/semaphore.h>
#elif defined(Q_OS_UNIX)
# if _POSIX_VERSION-0 >= 200112L || _XOPEN_VERSION-0 >= 600
#  include <semaphore.h>
#  define QT_UNIX_SEMAPHORE
# endif
#endif

struct timespec;

QT_BEGIN_NAMESPACE

// We manipulate the pointer to this class in inline, atomic code,
// so syncqt mustn't mark them as private, so ELFVERSION:ignore-next
class QMutexPrivate
{
public:
    ~QMutexPrivate();
    QMutexPrivate();

    bool wait(int timeout = -1);
    void wakeUp() noexcept;

    // Control the lifetime of the privates
    QAtomicInt refCount;
    int id;

    bool ref()
    {
        Q_ASSERT(refCount.loadRelaxed() >= 0);
        int c;
        do {
            c = refCount.loadRelaxed();
            if (c == 0)
                return false;
        } while (!refCount.testAndSetRelaxed(c, c + 1));
        Q_ASSERT(refCount.loadRelaxed() >= 0);
        return true;
    }
    void deref()
    {
        Q_ASSERT(refCount.loadRelaxed() >= 0);
        if (!refCount.deref())
            release();
        Q_ASSERT(refCount.loadRelaxed() >= 0);
    }
    void release();
    static QMutexPrivate *allocate();

    QAtomicInt waiters; // Number of threads waiting on this mutex. (may be offset by -BigNumber)
    QAtomicInt possiblyUnlocked; /* Boolean indicating that a timed wait timed out.
                                    When it is true, a reference is held.
                                    It is there to avoid a race that happens if unlock happens right
                                    when the mutex is unlocked.
                                  */
    enum { BigNumber = 0x100000 }; //Must be bigger than the possible number of waiters (number of threads)
    void derefWaiters(int value) noexcept;

    //platform specific stuff
#if defined(Q_OS_MAC)
    semaphore_t mach_semaphore;
#elif defined(QT_UNIX_SEMAPHORE)
    sem_t semaphore;
#elif defined(Q_OS_UNIX)
    bool wakeup;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#endif
};


#ifdef Q_OS_UNIX
// helper functions for qmutex_unix.cpp and qwaitcondition_unix.cpp
// they are in qwaitcondition_unix.cpp actually
void qt_initialize_pthread_cond(pthread_cond_t *cond, const char *where);
void qt_abstime_for_timeout(struct timespec *ts, QDeadlineTimer deadline);
#endif

QT_END_NAMESPACE

#endif // QMUTEX_P_H
