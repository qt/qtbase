/****************************************************************************
**
** Copyright (C) 2012 Collabora Ltd, author <robin.burchell@collabora.co.uk>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
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
