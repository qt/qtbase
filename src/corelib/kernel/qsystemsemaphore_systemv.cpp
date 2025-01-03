// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsystemsemaphore.h"
#include "qsystemsemaphore_p.h"

#include <qdebug.h>
#include <qfile.h>
#include <qcoreapplication.h>

#ifndef QT_POSIX_IPC

#if QT_CONFIG(systemsemaphore)

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <errno.h>

#if defined(Q_OS_DARWIN)
#include "qcore_mac_p.h"
#endif

#include "private/qcore_unix_p.h"

// OpenBSD 4.2 doesn't define EIDRM, see BUGS section:
// http://www.openbsd.org/cgi-bin/man.cgi?query=semop&manpath=OpenBSD+4.2
#if defined(Q_OS_OPENBSD) && !defined(EIDRM)
#define EIDRM EINVAL
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \internal

    Setup unix_key
 */
key_t QSystemSemaphorePrivate::handle(QSystemSemaphore::AccessMode mode)
{
#if defined(Q_OS_DARWIN)
    if (qt_apple_isSandboxed()) {
        errorString = QSystemSemaphore::tr("%1: System V semaphores are not available " \
            "for sandboxed applications. Please build Qt with -feature-ipc_posix")
                      .arg("QSystemSemaphore::handle:"_L1);
        error = QSystemSemaphore::PermissionDenied;
        return -1;
    }
#endif

    if (key.isEmpty()){
        errorString = QSystemSemaphore::tr("%1: key is empty")
                      .arg("QSystemSemaphore::handle:"_L1);
        error = QSystemSemaphore::KeyError;
        return -1;
    }

    // ftok requires that an actual file exists somewhere
    if (-1 != unix_key)
        return unix_key;

    // Create the file needed for ftok
    int built = QSharedMemoryPrivate::createUnixKeyFile(fileName);
    if (-1 == built) {
        errorString = QSystemSemaphore::tr("%1: unable to make key")
                      .arg("QSystemSemaphore::handle:"_L1);
        error = QSystemSemaphore::KeyError;
        return -1;
    }
    createdFile = (1 == built);

#if QT_CONFIG(sharedmemory) && !defined(QT_POSIX_IPC) && !defined(Q_OS_ANDROID)
    // Get the unix key for the created file
    unix_key = ftok(QFile::encodeName(fileName).constData(), 'Q');
#endif
    if (-1 == unix_key) {
        errorString = QSystemSemaphore::tr("%1: ftok failed")
                      .arg("QSystemSemaphore::handle:"_L1);
        error = QSystemSemaphore::KeyError;
        return -1;
    }

    // Get semaphore
    semaphore = semget(unix_key, 1, 0600 | IPC_CREAT | IPC_EXCL);
    if (-1 == semaphore) {
        if (errno == EEXIST)
            semaphore = semget(unix_key, 1, 0600 | IPC_CREAT);
        if (-1 == semaphore) {
            setErrorString("QSystemSemaphore::handle"_L1);
            cleanHandle();
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
    if (createdSemaphore && initialValue >= 0) {
        qt_semun init_op;
        init_op.val = initialValue;
        if (-1 == semctl(semaphore, 0, SETVAL, init_op)) {
            setErrorString("QSystemSemaphore::handle"_L1);
            cleanHandle();
            return -1;
        }
    }

    return unix_key;
}

/*!
    \internal

    Cleanup the unix_key
 */
void QSystemSemaphorePrivate::cleanHandle()
{
    unix_key = -1;

    // remove the file if we made it
    if (createdFile) {
        QFile::remove(fileName);
        createdFile = false;
    }

    if (createdSemaphore) {
        if (-1 != semaphore) {
            if (-1 == semctl(semaphore, 0, IPC_RMID, 0)) {
                setErrorString("QSystemSemaphore::cleanHandle"_L1);
#if defined QSYSTEMSEMAPHORE_DEBUG
                qDebug("QSystemSemaphore::cleanHandle semctl failed.");
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
bool QSystemSemaphorePrivate::modifySemaphore(int count)
{
    if (-1 == handle())
        return false;

    struct sembuf operation;
    operation.sem_num = 0;
    operation.sem_op = count;
    operation.sem_flg = SEM_UNDO;

    int res;
    EINTR_LOOP(res, semop(semaphore, &operation, 1));
    if (-1 == res) {
        // If the semaphore was removed be nice and create it and then modifySemaphore again
        if (errno == EINVAL || errno == EIDRM) {
            semaphore = -1;
            cleanHandle();
            handle();
            return modifySemaphore(count);
        }
        setErrorString("QSystemSemaphore::modifySemaphore"_L1);
#if defined QSYSTEMSEMAPHORE_DEBUG
        qDebug("QSystemSemaphore::modify failed %d %d %d %d %d",
               count, int(semctl(semaphore, 0, GETVAL)), int(errno), int(EIDRM), int(EINVAL);
#endif
        return false;
    }

    clearError();
    return true;
}


QT_END_NAMESPACE

#endif // QT_CONFIG(systemsemaphore)

#endif // QT_POSIX_IPC
