/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
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
#ifndef Q_OS_WINCE
#   include <sys/types.h>
#endif

#ifdef Q_OS_SYMBIAN
class RSemaphore;
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

#ifdef Q_OS_WIN
    HANDLE handle(QSystemSemaphore::AccessMode mode = QSystemSemaphore::Open);
    void setErrorString(const QString &function);
#elif defined(Q_OS_SYMBIAN)
    int handle(QSystemSemaphore::AccessMode mode = QSystemSemaphore::Open);
    void setErrorString(const QString &function,int err = 0);
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
    HANDLE semaphore;
    HANDLE semaphoreLock;
#elif defined(Q_OS_SYMBIAN)
    RSemaphore semaphore;
#else
    int semaphore;
    bool createdFile;
    bool createdSemaphore;
    key_t unix_key;
#endif
    QString errorString;
    QSystemSemaphore::SystemSemaphoreError error;
};

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMSEMAPHORE

#endif // QSYSTEMSEMAPHORE_P_H

