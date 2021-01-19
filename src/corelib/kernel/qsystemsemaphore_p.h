/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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

#ifndef QT_NO_SYSTEMSEMAPHORE

#include "qsharedmemory_p.h"
#include <sys/types.h>
#ifdef QT_POSIX_IPC
#   include <semaphore.h>
#endif

QT_BEGIN_NAMESPACE

class QSystemSemaphorePrivate
{

public:
    QSystemSemaphorePrivate();

    QString makeKeyFileName()
    {
        return QSharedMemoryPrivate::makePlatformSafeKey(key, QLatin1String("qipc_systemsem_"));
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
    Qt::HANDLE semaphore;
    Qt::HANDLE semaphoreLock;
#elif defined(QT_POSIX_IPC)
    sem_t *semaphore;
    bool createdSemaphore;
#else
    key_t unix_key;
    int semaphore;
    bool createdFile;
    bool createdSemaphore;
#endif
    QString errorString;
    QSystemSemaphore::SystemSemaphoreError error;
};

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMSEMAPHORE

#endif // QSYSTEMSEMAPHORE_P_H

