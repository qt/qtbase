/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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


#include "qsystemlock.h"
#include "qsystemlock_p.h"

#include <qdebug.h>
#include <qfile.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/shm.h>
#include <unistd.h>

#include <sys/sem.h>
// We have to define this as on some sem.h will have it
union qt_semun {
    int val;                    /* value for SETVAL */
    struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
    unsigned short *array;      /* array for GETALL, SETALL */
};

#define tr(x) QT_TRANSLATE_NOOP(QLatin1String("QSystemLock"), (x))

#if defined(Q_OS_SYMBIAN)
int createUnixKeyFile(const QString &fileName)
{
    if (QFile::exists(fileName))
        return 0;

    int fd = open(QFile::encodeName(fileName).constData(),
            O_EXCL | O_CREAT | O_RDWR, 0640);
    if (-1 == fd) {
        if (errno == EEXIST)
            return 0;
        return -1;
    } else {
        close(fd);
    }
    return 1;
}
#endif

QSystemLockPrivate::QSystemLockPrivate() :
        semaphore(-1), lockCount(0),
        error(QSystemLock::NoError), unix_key(-1), createdFile(false), createdSemaphore(false)
{
}

void QSystemLockPrivate::setErrorString(const QString &function)
{
    switch (errno) {
    case EIDRM:
        errorString = function + QLatin1String(": ") + tr("The semaphore set was removed");
        error = QSystemLock::UnknownError;
        break;
    default:
        errorString = function + QLatin1String(": ") + tr("unknown error");
        error = QSystemLock::UnknownError;
        qWarning() << errorString << "key" << key << "errno" << errno << ERANGE << ENOMEM << EINVAL << EINTR << EFBIG << EFAULT << EAGAIN << EACCES << E2BIG;
    }
}

/*!
    \internal

    Setup unix_key
 */
key_t QSystemLockPrivate::handle()
{
    if (key.isEmpty())
        return -1;

    // ftok requires that an actual file exists somewhere
    // If we have already made at some point in the past,
    // double check that it is still there.
    if (-1 != unix_key) {
        int aNewunix_key = ftok(QFile::encodeName(fileName).constData(), 'Q');
        if (aNewunix_key != unix_key) {
            cleanHandle();
        } else {
            return unix_key;
        }
    }

    // Create the file needed for ftok
#if defined(Q_OS_SYMBIAN)
    int built = createUnixKeyFile(fileName);
#else
    int built = QSharedMemoryPrivate::createUnixKeyFile(fileName);
#endif
    if (-1 == built)
        return -1;
    createdFile = (1 == built);

    // Get the unix key for the created file
    unix_key = ftok(QFile::encodeName(fileName).constData(), 'Q');
    if (-1 == unix_key) {
        setErrorString(QLatin1String("QSystemLock::handle ftok"));
        return -1;
    }

    // Get semaphore
    semaphore = semget(unix_key, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if (-1 == semaphore) {
        if (errno == EEXIST)
            semaphore = semget(unix_key, 1, 0666 | IPC_CREAT);
        if (-1 == semaphore) {
            setErrorString(QLatin1String("QSystemLock::handle semget"));
            cleanHandle();
            return -1;
        }
    } else {
        // Created semaphore, initialize value.
        createdSemaphore = true;
        qt_semun init_op;
        init_op.val = MAX_LOCKS;
        if (-1 == semctl(semaphore, 0, SETVAL, init_op)) {
            setErrorString(QLatin1String("QSystemLock::handle semctl"));
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
void QSystemLockPrivate::cleanHandle()
{
    unix_key = -1;

    // remove the file if we made it
    if (createdFile) {
        if (!QFile::remove(fileName))
            setErrorString(QLatin1String("QSystemLock::cleanHandle QFile::remove"));
        createdFile = false;
    }

    if (createdSemaphore) {
        if (-1 != semaphore) {
            if (-1 == semctl(semaphore, 0, IPC_RMID)) {
                setErrorString(QLatin1String("QSystemLock::cleanHandle semctl"));
            }
            semaphore = -1;
        }
        createdSemaphore = false;
    }
}

/*!
    \internal

    modifySemaphore generates operation.sem_op and handles recursive behavior.
 */
bool QSystemLockPrivate::modifySemaphore(QSystemLockPrivate::Operation op,
        QSystemLock::LockMode mode)
{
    if (-1 == handle())
        return false;

    if ((lockCount == 0 && op == Lock) || (lockCount > 0 && op == Unlock)) {
        if (op == Unlock) {
            --lockCount;
            if (lockCount < 0)
                qFatal("%s: lockCount must not be negative", Q_FUNC_INFO);
            if (lockCount > 0)
                return true;
        }

        struct sembuf operation;
        operation.sem_num = 0;
        operation.sem_op = (mode == QSystemLock::ReadWrite) ? MAX_LOCKS : 1;
        if (op == Lock)
            operation.sem_op *= -1;
        operation.sem_flg = SEM_UNDO;

        if (-1 == semop(semaphore, &operation, 1)) {
            setErrorString(QLatin1String("QSystemLock::modify"));
            return false;
        }
        lockedMode = mode;
    }
    if (op == Lock)
        lockCount++;

    return true;
}

