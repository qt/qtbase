// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformdefs.h"

#include "qsharedmemory.h"
#include "qsharedmemory_p.h"
#include "qsystemsemaphore.h"
#include <qdebug.h>

#include <errno.h>

#if QT_CONFIG(sharedmemory)
#include <sys/types.h>
#ifndef QT_POSIX_IPC
#include <sys/ipc.h>
#include <sys/shm.h>
#else
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif // QT_CONFIG(sharedmemory)

#include "private/qcore_unix_p.h"

#if QT_CONFIG(sharedmemory)
QT_BEGIN_NAMESPACE

void QSharedMemoryPrivate::setErrorString(QLatin1StringView function)
{
    // EINVAL is handled in functions so they can give better error strings
    switch (errno) {
    case EACCES:
        errorString = QSharedMemory::tr("%1: permission denied").arg(function);
        error = QSharedMemory::PermissionDenied;
        break;
    case EEXIST:
        errorString = QSharedMemory::tr("%1: already exists").arg(function);
        error = QSharedMemory::AlreadyExists;
        break;
    case ENOENT:
        errorString = QSharedMemory::tr("%1: doesn't exist").arg(function);
        error = QSharedMemory::NotFound;
        break;
    case EMFILE:
    case ENOMEM:
    case ENOSPC:
        errorString = QSharedMemory::tr("%1: out of resources").arg(function);
        error = QSharedMemory::OutOfResources;
        break;
    default:
        errorString = QSharedMemory::tr("%1: unknown error %2").arg(function).arg(errno);
        error = QSharedMemory::UnknownError;
#if defined QSHAREDMEMORY_DEBUG
        qDebug() << errorString << "key" << key << "errno" << errno << EINVAL;
#endif
    }
}

QT_END_NAMESPACE

#endif // QT_CONFIG(sharedmemory)
