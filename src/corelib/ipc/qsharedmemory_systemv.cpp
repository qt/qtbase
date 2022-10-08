// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsharedmemory.h"
#include "qsharedmemory_p.h"

#include "qtipccommon_p.h"

#include <qdir.h>
#include <qdebug.h>

#include <errno.h>

#ifndef QT_POSIX_IPC

#if QT_CONFIG(sharedmemory)
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif // QT_CONFIG(sharedmemory)

#include "private/qcore_unix_p.h"

#if QT_CONFIG(sharedmemory)
QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QtIpcCommon;

/*!
    \internal

    If not already made create the handle used for accessing the shared memory.
*/
key_t QSharedMemoryPrivate::handle()
{
    // already made
    if (unix_key)
        return unix_key;

    // don't allow making handles on empty keys
    if (nativeKey.isEmpty()) {
        errorString = QSharedMemory::tr("%1: key is empty").arg("QSharedMemory::handle:"_L1);
        error = QSharedMemory::KeyError;
        return 0;
    }

    // ftok requires that an actual file exists somewhere
    if (!QFile::exists(nativeKey)) {
        errorString = QSharedMemory::tr("%1: UNIX key file doesn't exist").arg("QSharedMemory::handle:"_L1);
        error = QSharedMemory::NotFound;
        return 0;
    }

    unix_key = ftok(QFile::encodeName(nativeKey).constData(), 'Q');
    if (-1 == unix_key) {
        errorString = QSharedMemory::tr("%1: ftok failed").arg("QSharedMemory::handle:"_L1);
        error = QSharedMemory::KeyError;
        unix_key = 0;
    }
    return unix_key;
}

bool QSharedMemoryPrivate::cleanHandle()
{
    unix_key = 0;
    return true;
}

bool QSharedMemoryPrivate::create(qsizetype size)
{
    // build file if needed
    bool createdFile = false;
    QByteArray nativeKeyFile = QFile::encodeName(nativeKey);
    int built = createUnixKeyFile(nativeKeyFile);
    if (built == -1) {
        errorString = QSharedMemory::tr("%1: unable to make key").arg("QSharedMemory::handle:"_L1);
        error = QSharedMemory::KeyError;
        return false;
    }
    if (built == 1) {
        createdFile = true;
    }

    // get handle
    if (!handle()) {
        if (createdFile)
            unlink(nativeKeyFile);
        return false;
    }

    // create
    if (-1 == shmget(unix_key, size_t(size), 0600 | IPC_CREAT | IPC_EXCL)) {
        const auto function = "QSharedMemory::create"_L1;
        switch (errno) {
        case EINVAL:
            errorString = QSharedMemory::tr("%1: system-imposed size restrictions").arg("QSharedMemory::handle"_L1);
            error = QSharedMemory::InvalidSize;
            break;
        default:
            setErrorString(function);
        }
        if (createdFile && error != QSharedMemory::AlreadyExists)
            unlink(nativeKeyFile);
        return false;
    }

    return true;
}

bool QSharedMemoryPrivate::attach(QSharedMemory::AccessMode mode)
{
    // grab the shared memory segment id
    int id = shmget(unix_key, 0, (mode == QSharedMemory::ReadOnly ? 0400 : 0600));
    if (-1 == id) {
        setErrorString("QSharedMemory::attach (shmget)"_L1);
        return false;
    }

    // grab the memory
    memory = shmat(id, nullptr, (mode == QSharedMemory::ReadOnly ? SHM_RDONLY : 0));
    if ((void *)-1 == memory) {
        memory = nullptr;
        setErrorString("QSharedMemory::attach (shmat)"_L1);
        return false;
    }

    // grab the size
    shmid_ds shmid_ds;
    if (!shmctl(id, IPC_STAT, &shmid_ds)) {
        size = (qsizetype)shmid_ds.shm_segsz;
    } else {
        setErrorString("QSharedMemory::attach (shmctl)"_L1);
        return false;
    }

    return true;
}

bool QSharedMemoryPrivate::detach()
{
    // detach from the memory segment
    if (-1 == shmdt(memory)) {
        const auto function = "QSharedMemory::detach"_L1;
        switch (errno) {
        case EINVAL:
            errorString = QSharedMemory::tr("%1: not attached").arg(function);
            error = QSharedMemory::NotFound;
            break;
        default:
            setErrorString(function);
        }
        return false;
    }
    memory = nullptr;
    size = 0;

    // Get the number of current attachments
    int id = shmget(unix_key, 0, 0400);
    cleanHandle();

    struct shmid_ds shmid_ds;
    if (0 != shmctl(id, IPC_STAT, &shmid_ds)) {
        switch (errno) {
        case EINVAL:
            return true;
        default:
            return false;
        }
    }
    // If there are no attachments then remove it.
    if (shmid_ds.shm_nattch == 0) {
        // mark for removal
        if (-1 == shmctl(id, IPC_RMID, &shmid_ds)) {
            setErrorString("QSharedMemory::remove"_L1);
            switch (errno) {
            case EINVAL:
                return true;
            default:
                return false;
            }
        }

        // remove file
        if (!unlink(QFile::encodeName(nativeKey)))
            return false;
    }
    return true;
}


QT_END_NAMESPACE

#endif // QT_CONFIG(sharedmemory)

#endif // QT_POSIX_IPC
