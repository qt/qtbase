// Copyright (C) 2012 Collabora Ltd, author <robin.burchell@collabora.co.uk>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsystemsemaphore.h"
#include "qsystemsemaphore_p.h"

#include <qdebug.h>

#if QT_CONFIG(systemsemaphore)

QT_BEGIN_NAMESPACE

void QSystemSemaphorePrivate::setErrorString(const QString &function)
{
    Q_UNUSED(function);
    Q_UNIMPLEMENTED();
}

key_t QSystemSemaphorePrivate::handle(QSystemSemaphore::AccessMode mode)
{
    Q_UNUSED(mode);
    Q_UNIMPLEMENTED();
    return -1;
}

void QSystemSemaphorePrivate::cleanHandle()
{
    Q_UNIMPLEMENTED();
}

bool QSystemSemaphorePrivate::modifySemaphore(int count)
{
    Q_UNUSED(count);
    Q_UNIMPLEMENTED();
    return false;
}


QT_END_NAMESPACE

#endif // QT_CONFIG(systemsemaphore)
