// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSHAREDMEMORY_P_H
#define QSHAREDMEMORY_P_H

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

#include "qsharedmemory.h"

#include <QtCore/qstring.h>

#ifdef QT_NO_SHAREDMEMORY
#    ifndef QT_NO_SYSTEMSEMAPHORE

QT_BEGIN_NAMESPACE

namespace QSharedMemoryPrivate
{
    int createUnixKeyFile(const QString &fileName);
    QString makePlatformSafeKey(const QString &key,
            const QString &prefix = QStringLiteral("qipc_sharedmemory_"));
}

QT_END_NAMESPACE

#    endif
#else

#include "qsystemsemaphore.h"

#ifndef QT_NO_QOBJECT
# include "private/qobject_p.h"
#endif

#if !defined(Q_OS_WIN) && !defined(Q_OS_ANDROID) && !defined(Q_OS_INTEGRITY) && !defined(Q_OS_RTEMS)
#  include <sys/sem.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SYSTEMSEMAPHORE
/*!
  Helper class
  */
class QSharedMemoryLocker
{

public:
    inline QSharedMemoryLocker(QSharedMemory *sharedMemory) : q_sm(sharedMemory)
    {
        Q_ASSERT(q_sm);
    }

    inline ~QSharedMemoryLocker()
    {
        if (q_sm)
            q_sm->unlock();
    }

    inline bool lock()
    {
        if (q_sm && q_sm->lock())
            return true;
        q_sm = nullptr;
        return false;
    }

private:
    QSharedMemory *q_sm;
};
#endif // QT_NO_SYSTEMSEMAPHORE

class Q_AUTOTEST_EXPORT QSharedMemoryPrivate
#ifndef QT_NO_QOBJECT
        : public QObjectPrivate
#endif
{
#ifndef QT_NO_QOBJECT
    Q_DECLARE_PUBLIC(QSharedMemory)
#endif

public:
    QSharedMemoryPrivate();

    void *memory;
    qsizetype size;
    QString key;
    QString nativeKey;
    QSharedMemory::SharedMemoryError error;
    QString errorString;
#ifndef QT_NO_SYSTEMSEMAPHORE
    QSystemSemaphore systemSemaphore;
    bool lockedByMe;
#endif

    static int createUnixKeyFile(const QString &fileName);
    static QString makePlatformSafeKey(const QString &key,
            const QString &prefix = QStringLiteral("qipc_sharedmemory_"));
#ifdef Q_OS_WIN
    Qt::HANDLE handle();
#elif defined(QT_POSIX_IPC)
    int handle();
#else
    key_t handle();
#endif
    bool initKey();
    bool cleanHandle();
    bool create(qsizetype size);
    bool attach(QSharedMemory::AccessMode mode);
    bool detach();

    void setErrorString(QLatin1StringView function);

#ifndef QT_NO_SYSTEMSEMAPHORE
    bool tryLocker(QSharedMemoryLocker *locker, const QString &function) {
        if (!locker->lock()) {
            errorString = QSharedMemory::tr("%1: unable to lock").arg(function);
            error = QSharedMemory::LockError;
            return false;
        }
        return true;
    }
#endif // QT_NO_SYSTEMSEMAPHORE

private:
#ifdef Q_OS_WIN
    Qt::HANDLE hand;
#elif defined(QT_POSIX_IPC)
    int hand;
#else
    key_t unix_key;
#endif
};

QT_END_NAMESPACE

#endif // QT_NO_SHAREDMEMORY

#endif // QSHAREDMEMORY_P_H

