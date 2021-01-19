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

#include "qsystemsemaphore.h"
#include "qsystemsemaphore_p.h"

#include <qdebug.h>

#ifndef QT_NO_SYSTEMSEMAPHORE

QT_BEGIN_NAMESPACE

QSystemSemaphorePrivate::QSystemSemaphorePrivate() :
        unix_key(-1), semaphore(-1), createdFile(false),
        createdSemaphore(false), error(QSystemSemaphore::NoError)
{
}

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

#endif // QT_NO_SYSTEMSEMAPHORE
