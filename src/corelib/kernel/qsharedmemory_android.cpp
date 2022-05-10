// Copyright (C) 2012 Collabora Ltd, author <robin.burchell@collabora.co.uk>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsharedmemory.h"
#include "qsharedmemory_p.h"
#include <qdebug.h>

#ifndef QT_NO_SHAREDMEMORY
QT_BEGIN_NAMESPACE

QSharedMemoryPrivate::QSharedMemoryPrivate()
    : QObjectPrivate(), memory(0), size(0), error(QSharedMemory::NoError),
#ifndef QT_NO_SYSTEMSEMAPHORE
      systemSemaphore(QString()), lockedByMe(false),
#endif
      unix_key(0)
{
}

void QSharedMemoryPrivate::setErrorString(QLatin1StringView function)
{
    Q_UNUSED(function);
    Q_UNIMPLEMENTED();
}

key_t QSharedMemoryPrivate::handle()
{
    Q_UNIMPLEMENTED();
    return 0;
}

#endif // QT_NO_SHAREDMEMORY

#if !(defined(QT_NO_SHAREDMEMORY) && defined(QT_NO_SYSTEMSEMAPHORE))
int QSharedMemoryPrivate::createUnixKeyFile(const QString &fileName)
{
    Q_UNUSED(fileName);
    Q_UNIMPLEMENTED();
    return 0;
}
#endif // QT_NO_SHAREDMEMORY && QT_NO_SYSTEMSEMAPHORE

#ifndef QT_NO_SHAREDMEMORY

bool QSharedMemoryPrivate::cleanHandle()
{
    Q_UNIMPLEMENTED();
    return true;
}

bool QSharedMemoryPrivate::create(qsizetype size)
{
    Q_UNUSED(size);
    Q_UNIMPLEMENTED();
    return false;
}

bool QSharedMemoryPrivate::attach(QSharedMemory::AccessMode mode)
{
    Q_UNUSED(mode);
    Q_UNIMPLEMENTED();
    return false;
}

bool QSharedMemoryPrivate::detach()
{
    Q_UNIMPLEMENTED();
    return false;
}


QT_END_NAMESPACE

#endif // QT_NO_SHAREDMEMORY
