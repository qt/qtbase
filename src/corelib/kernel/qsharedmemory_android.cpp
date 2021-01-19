/****************************************************************************
**
** Copyright (C) 2012 Collabora Ltd, author <robin.burchell@collabora.co.uk>
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

void QSharedMemoryPrivate::setErrorString(QLatin1String function)
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

bool QSharedMemoryPrivate::create(int size)
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
