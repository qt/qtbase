// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSYSTEMSEMAPHORE_P_H
#define QSYSTEMSEMAPHORE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qsystemsemaphore.h"

#if QT_CONFIG(systemsemaphore)

#include "qcoreapplication.h"
#include "qsharedmemory_p.h"
#include <sys/types.h>
#ifdef QT_POSIX_IPC
#   include <semaphore.h>
#endif

QT_BEGIN_NAMESPACE

class QSystemSemaphorePrivate
{

public:

    QString makeKeyFileName()
    {
        return QSharedMemoryPrivate::makePlatformSafeKey(key, QLatin1StringView("qipc_systemsem_"));
    }

    inline void setError(QSystemSemaphore::SystemSemaphoreError e, const QString &message)
    { error = e; errorString = message; }
    inline void clearError()
    { setError(QSystemSemaphore::NoError, QString()); }

#ifdef Q_OS_WIN
    Qt::HANDLE handle(QSystemSemaphore::AccessMode mode = QSystemSemaphore::Open);
    void setErrorString(const QString &function);
#elif defined(QT_POSIX_IPC)
    bool handle(QSystemSemaphore::AccessMode mode = QSystemSemaphore::Open);
    void setErrorString(const QString &function);
#else
    key_t handle(QSystemSemaphore::AccessMode mode = QSystemSemaphore::Open);
    void setErrorString(const QString &function);
#endif
    void cleanHandle();
    bool modifySemaphore(int count);

    QString key;
    QString fileName;
    int initialValue;
#ifdef Q_OS_WIN
    Qt::HANDLE semaphore = nullptr;
#elif defined(QT_POSIX_IPC)
    sem_t *semaphore = SEM_FAILED;
    bool createdSemaphore = false;
#else
    key_t unix_key = -1;
    int semaphore = -1;
    bool createdFile = false;
    bool createdSemaphore = false;
#endif
    QString errorString;
    QSystemSemaphore::SystemSemaphoreError error = QSystemSemaphore::NoError;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(systemsemaphore)

#endif // QSYSTEMSEMAPHORE_P_H

