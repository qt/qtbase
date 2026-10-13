// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qsystemsemaphore.h"
#include "qsystemsemaphore_p.h"

#include <qdebug.h>
#include <qfile.h>
#include <qcoreapplication.h>

#if QT_CONFIG(sysv_sem)

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <errno.h>

#if defined(Q_OS_DARWIN)
#include "private/qcore_mac_p.h"
#endif

#include "private/qcore_unix_p.h"

// OpenBSD 4.2 doesn't define EIDRM, see BUGS section:
// http://www.openbsd.org/cgi-bin/man.cgi?query=semop&manpath=OpenBSD+4.2
#if defined(Q_OS_OPENBSD) && !defined(EIDRM)
#define EIDRM EINVAL
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

bool QSystemSemaphoreSystemV::runtimeSupportCheck()
{
#if defined(Q_OS_DARWIN)
    if (qt_apple_isSandboxed())
        return false;
#endif
    static const bool result = []() {
        (void)semget(IPC_PRIVATE, -1, 0);     // this will fail
        return errno != ENOSYS;
    }();
    return result;
}

/*!
    \internal

    Setup unix_key
 */
key_t QSystemSemaphoreSystemV::handle(QSystemSemaphorePrivate *self, QSystemSemaphore::AccessMode mode)
{
    if (unix_key != -1)
        return unix_key;  // we already have a semaphore

#if defined(Q_OS_DARWIN)
    if (qt_apple_isSandboxed()) {
        // attempting to use System V semaphores will get us a SIGSYS
        self->setError(QSystemSemaphore::PermissionDenied,
                       QSystemSemaphore::tr("%1: System V semaphores are not available for "
                                            "sandboxed applications. Please build Qt with "
                                            "-feature-ipc_posix")
                       .arg("QSystemSemaphore::handle:"_L1));
        return -1;
    }
#endif

    nativeKeyFile = QFile::encodeName(self->nativeKey.nativeKey());
    if (nativeKeyFile.isEmpty()) {
        self->setError(QSystemSemaphore::KeyError,
                       QSystemSemaphore::tr("%1: key is empty")
                       .arg("QSystemSemaphore::handle:"_L1));
        return -1;
    }

    // ftok requires that an actual file exists somewhere
    int built = QtIpcCommon::createUnixKeyFile(nativeKeyFile);
    if (-1 == built) {
        self->setError(QSystemSemaphore::KeyError,
                       QSystemSemaphore::tr("%1: unable to make key")
                       .arg("QSystemSemaphore::handle:"_L1));

        return -1;
    }
    createdFile = (1 == built);

    // Get the unix key for the created file
    unix_key = ftok(nativeKeyFile, int(self->nativeKey.type()));
    if (-1 == unix_key) {
        self->setError(QSystemSemaphore::KeyError,
                       QSystemSemaphore::tr("%1: ftok failed")
                       .arg("QSystemSemaphore::handle:"_L1));
        return -1;
    }

    // Get semaphore
    semaphore = semget(unix_key, 1, 0600 | IPC_CREAT | IPC_EXCL);
    if (-1 == semaphore) {
        if (errno == EEXIST)
            semaphore = semget(unix_key, 1, 0600 | IPC_CREAT);
        if (-1 == semaphore) {
            self->setUnixErrorString("QSystemSemaphore::handle"_L1);
            cleanHandle(self);
            return -1;
        }
    } else {
        createdSemaphore = true;
        // Force cleanup of file, it is possible that it can be left over from a crash
        createdFile = true;
    }

    if (mode == QSystemSemaphore::Create) {
        createdSemaphore = true;
        createdFile = true;
    }

    // Created semaphore so initialize its value.
    if (createdSemaphore && self->initialValue >= 0) {
        qt_semun init_op;
        init_op.val = self->initialValue;
        if (-1 == semctl(semaphore, 0, SETVAL, init_op)) {
            self->setUnixErrorString("QSystemSemaphore::handle"_L1);
            cleanHandle(self);
            return -1;
        }
    }

    return unix_key;
}

/*!
    \internal

    Cleanup the unix_key
 */
void QSystemSemaphoreSystemV::cleanHandle(QSystemSemaphorePrivate *self)
{
    unix_key = -1;

    // remove the file if we made it
    if (createdFile) {
        unlink(nativeKeyFile.constData());
        createdFile = false;
    }

    if (createdSemaphore) {
        if (-1 != semaphore) {
            if (-1 == semctl(semaphore, 0, IPC_RMID, 0)) {
                self->setUnixErrorString("QSystemSemaphore::cleanHandle"_L1);
#if defined QSYSTEMSEMAPHORE_DEBUG
                qDebug("QSystemSemaphoreSystemV::cleanHandle semctl failed.");
#endif
            }
            semaphore = -1;
        }
        createdSemaphore = false;
    }
}

/*!
    \internal
 */
bool QSystemSemaphoreSystemV::modifySemaphore(QSystemSemaphorePrivate *self, int count)
{
    if (handle(self, QSystemSemaphore::Open) == -1)
        return false;

    struct sembuf operation;
    operation.sem_num = 0;
    operation.sem_op = count;
    operation.sem_flg = SEM_UNDO;

    int res;
    QT_EINTR_LOOP(res, semop(semaphore, &operation, 1));
    if (-1 == res) {
        // If the semaphore was removed be nice and create it and then modifySemaphore again
        if (errno == EINVAL || errno == EIDRM) {
            semaphore = -1;
            cleanHandle(self);
            handle(self, QSystemSemaphore::Open);
            return modifySemaphore(self, count);
        }
        self->setUnixErrorString("QSystemSemaphore::modifySemaphore"_L1);
#if defined QSYSTEMSEMAPHORE_DEBUG
        qDebug("QSystemSemaphoreSystemV::modify failed %d %d %d %d %d",
               count, int(semctl(semaphore, 0, GETVAL)), int(errno), int(EIDRM), int(EINVAL);
#endif
        return false;
    }

    self->clearError();
    return true;
}


QT_END_NAMESPACE

#endif // QT_CONFIG(sysv_sem)
