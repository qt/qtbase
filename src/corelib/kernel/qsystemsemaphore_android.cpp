/****************************************************************************
**
** Copyright (C) 2012 Collabora Ltd, author <robin.burchell@collabora.co.uk>
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
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
        semaphore(-1), createdFile(false),
        createdSemaphore(false), unix_key(-1), error(QSystemSemaphore::NoError)
{
}

void QSystemSemaphorePrivate::setErrorString(const QString &function)
{
    qWarning() << Q_FUNC_INFO << "Not yet implemented on Android";
}

/*!
    \internal

    Setup unix_key
 */
key_t QSystemSemaphorePrivate::handle(QSystemSemaphore::AccessMode mode)
{
    qWarning() << Q_FUNC_INFO << "Not yet implemented on Android";
    return -1;
}

/*!
    \internal

    Cleanup the unix_key
 */
void QSystemSemaphorePrivate::cleanHandle()
{
    qWarning() << Q_FUNC_INFO << "Not yet implemented on Android";
}

/*!
    \internal
 */
bool QSystemSemaphorePrivate::modifySemaphore(int count)
{
    qWarning() << Q_FUNC_INFO << "Not yet implemented on Android";
    return false;
}


QT_END_NAMESPACE

#endif // QT_NO_SYSTEMSEMAPHORE
