// Copyright (C) 2015 Konstantin Ritt <ritt.ks@gmail.com>
// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qsystemsemaphore.h"
#include "qsystemsemaphore_p.h"

#include <qdebug.h>
#include <qfile.h>
#include <qcoreapplication.h>

#if QT_CONFIG(posix_sem)

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#ifdef Q_OS_UNIX
#  include "private/qcore_unix_p.h"
#else
#  define QT_EINTR_LOOP_VAL(var, val, cmd)       \
    (void)var; var = cmd
#  define QT_EINTR_LOOP(var, cmd)    QT_EINTR_LOOP_VAL(var, -1, cmd)
#endif

// OpenBSD 4.2 doesn't define EIDRM, see BUGS section:
// http://www.openbsd.org/cgi-bin/man.cgi?query=semop&manpath=OpenBSD+4.2
#if defined(Q_OS_OPENBSD) && !defined(EIDRM)
#define EIDRM EINVAL
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

bool QSystemSemaphorePosix::runtimeSupportCheck()
{
    static const bool result = []() {
        sem_open("/", 0, 0, 0);     // this WILL fail
        return errno != ENOSYS;
    }();
    return result;
}

bool QSystemSemaphorePosix::handle(QSystemSemaphorePrivate *self, QSystemSemaphore::AccessMode mode)
{
    if (semaphore != SEM_FAILED)
        return true;  // we already have a semaphore

    const QByteArray semName = QFile::encodeName(self->nativeKey.nativeKey());
    if (semName.isEmpty()) {
        self->setError(QSystemSemaphore::KeyError,
                       QSystemSemaphore::tr("%1: key is empty")
                       .arg("QSystemSemaphore::handle"_L1));
        return false;
    }

    // Always try with O_EXCL so we know whether we created the semaphore.
    int oflag = O_CREAT | O_EXCL;
    for (int tryNum = 0, maxTries = 1; tryNum < maxTries; ++tryNum) {
        do {
            semaphore = ::sem_open(semName.constData(), oflag, 0600, self->initialValue);
        } while (semaphore == SEM_FAILED && errno == EINTR);
        if (semaphore == SEM_FAILED && errno == EEXIST) {
            if (mode == QSystemSemaphore::Create) {
                if (::sem_unlink(semName.constData()) == -1 && errno != ENOENT) {
                    self->setUnixErrorString("QSystemSemaphore::handle (sem_unlink)"_L1);
                    return false;
                }
                // Race condition: the semaphore might be recreated before
                // we call sem_open again, so we'll retry several times.
                maxTries = 3;
            } else {
                // Race condition: if it no longer exists at the next sem_open
                // call, we won't realize we created it, so we'll leak it later.
                oflag &= ~O_EXCL;
                maxTries = 2;
            }
        } else {
            break;
        }
    }

    if (semaphore == SEM_FAILED) {
        self->setUnixErrorString("QSystemSemaphore::handle"_L1);
        return false;
    }

    createdSemaphore = (oflag & O_EXCL) != 0;

    return true;
}

void QSystemSemaphorePosix::cleanHandle(QSystemSemaphorePrivate *self)
{
    if (semaphore != SEM_FAILED) {
        if (::sem_close(semaphore) == -1) {
            self->setUnixErrorString("QSystemSemaphore::cleanHandle (sem_close)"_L1);
#if defined QSYSTEMSEMAPHORE_DEBUG
            qDebug("QSystemSemaphore::cleanHandle sem_close failed.");
#endif
        }
        semaphore = SEM_FAILED;
    }

    if (createdSemaphore) {
        const QByteArray semName = QFile::encodeName(self->nativeKey.nativeKey());
        if (::sem_unlink(semName) == -1 && errno != ENOENT) {
            self->setUnixErrorString("QSystemSemaphore::cleanHandle (sem_unlink)"_L1);
#if defined QSYSTEMSEMAPHORE_DEBUG
            qDebug("QSystemSemaphorePosix::cleanHandle sem_unlink failed.");
#endif
        }
        createdSemaphore = false;
    }
}

bool QSystemSemaphorePosix::modifySemaphore(QSystemSemaphorePrivate *self, int count)
{
    if (!handle(self, QSystemSemaphore::Open))
        return false;

    if (count > 0) {
        int cnt = count;
        do {
            if (::sem_post(semaphore) == -1) {
#if defined(Q_OS_VXWORKS)
                if (errno == EINVAL) {
                    semaphore = SEM_FAILED;
                    return modifySemaphore(self, cnt);
                }
#endif
                self->setUnixErrorString("QSystemSemaphore::modifySemaphore (sem_post)"_L1);
#if defined QSYSTEMSEMAPHORE_DEBUG
                qDebug("QSystemSemaphorePosix::modify sem_post failed %d %d", count, errno);
#endif
                // rollback changes to preserve the SysV semaphore behavior
                for ( ; cnt < count; ++cnt) {
                    int res;
                    QT_EINTR_LOOP(res, ::sem_wait(semaphore));
                }
                return false;
            }
            --cnt;
        } while (cnt > 0);
    } else {
        int res;
        QT_EINTR_LOOP(res, ::sem_wait(semaphore));
        if (res == -1) {
            // If the semaphore was removed be nice and create it and then modifySemaphore again
            if (errno == EINVAL || errno == EIDRM) {
                semaphore = SEM_FAILED;
                return modifySemaphore(self, count);
            }
            self->setUnixErrorString("QSystemSemaphore::modifySemaphore (sem_wait)"_L1);
#if defined QSYSTEMSEMAPHORE_DEBUG
            qDebug("QSystemSemaphorePosix::modify sem_wait failed %d %d", count, errno);
#endif
            return false;
        }
    }

    self->clearError();
    return true;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(posix_sem)
