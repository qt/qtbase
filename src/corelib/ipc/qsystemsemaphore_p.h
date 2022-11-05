// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
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
#include "qtipccommon_p.h"
#include "private/qtcore-config_p.h"

#include <sys/types.h>
#if QT_CONFIG(posix_sem)
#  include <semaphore.h>
#endif
#ifndef SEM_FAILED
#  define SEM_FAILED     nullptr
struct sem_t;
#endif
#if QT_CONFIG(sysv_sem)
#  include <sys/sem.h>
#endif

QT_BEGIN_NAMESPACE

class QSystemSemaphorePrivate;

struct QSystemSemaphorePosix
{
    bool handle(QSystemSemaphorePrivate *self, QSystemSemaphore::AccessMode mode);
    void cleanHandle(QSystemSemaphorePrivate *self);
    bool modifySemaphore(QSystemSemaphorePrivate *self, int count);

    sem_t *semaphore = SEM_FAILED;
    bool createdSemaphore = false;
};

struct QSystemSemaphoreSystemV
{
#if QT_CONFIG(sysv_sem)
    key_t handle(QSystemSemaphorePrivate *self, QSystemSemaphore::AccessMode mode);
    void cleanHandle(QSystemSemaphorePrivate *self);
    bool modifySemaphore(QSystemSemaphorePrivate *self, int count);

    key_t unix_key = -1;
    int semaphore = -1;
    bool createdFile = false;
    bool createdSemaphore = false;
#endif
};

struct QSystemSemaphoreWin32
{
//#ifdef Q_OS_WIN32     but there's nothing Windows-specific in the header
    Qt::HANDLE handle(QSystemSemaphorePrivate *self, QSystemSemaphore::AccessMode mode);
    void cleanHandle(QSystemSemaphorePrivate *self);
    bool modifySemaphore(QSystemSemaphorePrivate *self, int count);

    Qt::HANDLE semaphore = nullptr;
};

class QSystemSemaphorePrivate
{
public:
    QString makeKeyFileName()
    {
        return QtIpcCommon::legacyPlatformSafeKey(key, QtIpcCommon::IpcType::SystemSemaphore);
    }

    void setWindowsErrorString(QLatin1StringView function);    // Windows only
    void setUnixErrorString(QLatin1StringView function);
    inline void setError(QSystemSemaphore::SystemSemaphoreError e, const QString &message)
    { error = e; errorString = message; }
    inline void clearError()
    { setError(QSystemSemaphore::NoError, QString()); }

    QString key;
    QString fileName;
    QString errorString;
    int initialValue;
    QSystemSemaphore::SystemSemaphoreError error = QSystemSemaphore::NoError;

#if defined(Q_OS_WIN)
    using DefaultBackend = QSystemSemaphoreWin32;
#elif defined(QT_POSIX_IPC)
    using DefaultBackend = QSystemSemaphorePosix;
#else
    using DefaultBackend = QSystemSemaphoreSystemV;
#endif
    DefaultBackend backend;

    void handle(QSystemSemaphore::AccessMode mode)
    {
        backend.handle(this, mode);
    }
    void cleanHandle()
    {
        backend.cleanHandle(this);
    }
    bool modifySemaphore(int count)
    {
        return backend.modifySemaphore(this, count);
    }
};

QT_END_NAMESPACE

#endif // QT_CONFIG(systemsemaphore)

#endif // QSYSTEMSEMAPHORE_P_H

