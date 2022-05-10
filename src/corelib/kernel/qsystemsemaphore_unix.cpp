// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsystemsemaphore.h"
#include "qsystemsemaphore_p.h"

#include <qdebug.h>
#include <qcoreapplication.h>

#ifndef QT_NO_SYSTEMSEMAPHORE

#include <sys/types.h>
#ifndef QT_POSIX_IPC
#include <sys/ipc.h>
#include <sys/sem.h>
#endif
#include <fcntl.h>
#include <errno.h>

#include "private/qcore_unix_p.h"

QT_BEGIN_NAMESPACE

QSystemSemaphorePrivate::QSystemSemaphorePrivate() :
#ifndef QT_POSIX_IPC
    unix_key(-1), semaphore(-1), createdFile(false),
#else
    semaphore(SEM_FAILED),
#endif // QT_POSIX_IPC
    createdSemaphore(false), error(QSystemSemaphore::NoError)
{
}

void QSystemSemaphorePrivate::setErrorString(const QString &function)
{
    // EINVAL is handled in functions so they can give better error strings
    switch (errno) {
    case EPERM:
    case EACCES:
        errorString = QSystemSemaphore::tr("%1: permission denied").arg(function);
        error = QSystemSemaphore::PermissionDenied;
        break;
    case EEXIST:
        errorString = QSystemSemaphore::tr("%1: already exists").arg(function);
        error = QSystemSemaphore::AlreadyExists;
        break;
    case ENOENT:
        errorString = QSystemSemaphore::tr("%1: does not exist").arg(function);
        error = QSystemSemaphore::NotFound;
        break;
    case ERANGE:
    case ENOSPC:
        errorString = QSystemSemaphore::tr("%1: out of resources").arg(function);
        error = QSystemSemaphore::OutOfResources;
        break;
#if defined(QT_POSIX_IPC)
    case ENAMETOOLONG:
        errorString = QSystemSemaphore::tr("%1: key too long").arg(function);
        error = QSystemSemaphore::KeyError;
        break;
#endif
    default:
        errorString = QSystemSemaphore::tr("%1: unknown error %2").arg(function).arg(errno);
        error = QSystemSemaphore::UnknownError;
#if defined QSYSTEMSEMAPHORE_DEBUG
        qDebug() << errorString << "key" << key << "errno" << errno << EINVAL;
#endif
    }
}

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMSEMAPHORE
