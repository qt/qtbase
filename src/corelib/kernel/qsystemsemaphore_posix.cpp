/****************************************************************************
**
** Copyright (C) 2015 Konstantin Ritt <ritt.ks@gmail.com>
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsystemsemaphore.h"
#include "qsystemsemaphore_p.h"

#include <qdebug.h>
#include <qfile.h>
#include <qcoreapplication.h>

#ifdef QT_POSIX_IPC

#ifndef QT_NO_SYSTEMSEMAPHORE

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include "private/qcore_unix_p.h"

// OpenBSD 4.2 doesn't define EIDRM, see BUGS section:
// http://www.openbsd.org/cgi-bin/man.cgi?query=semop&manpath=OpenBSD+4.2
#if defined(Q_OS_OPENBSD) && !defined(EIDRM)
#define EIDRM EINVAL
#endif

QT_BEGIN_NAMESPACE

bool QSystemSemaphorePrivate::handle(QSystemSemaphore::AccessMode mode)
{
    if (semaphore != SEM_FAILED)
        return true;  // we already have a semaphore

    if (fileName.isEmpty()) {
        errorString = QCoreApplication::tr("%1: key is empty", "QSystemSemaphore").arg(QLatin1String("QSystemSemaphore::handle"));
        error = QSystemSemaphore::KeyError;
        return false;
    }

    const QByteArray semName = QFile::encodeName(fileName);

    // Always try with O_EXCL so we know whether we created the semaphore.
    int oflag = O_CREAT | O_EXCL;
    for (int tryNum = 0, maxTries = 1; tryNum < maxTries; ++tryNum) {
        do {
            semaphore = ::sem_open(semName.constData(), oflag, 0600, initialValue);
        } while (semaphore == SEM_FAILED && errno == EINTR);
        if (semaphore == SEM_FAILED && errno == EEXIST) {
            if (mode == QSystemSemaphore::Create) {
                if (::sem_unlink(semName.constData()) == -1 && errno != ENOENT) {
                    setErrorString(QLatin1String("QSystemSemaphore::handle (sem_unlink)"));
                    return false;
                }
                // Race condition: the semaphore might be recreated before
                // we call sem_open again, so we'll retry several times.
                maxTries = 3;
            } else {
                // Race condition: if it no longer exists at the next sem_open
                // call, we won't realize we created it, so we'll leak it later.
                oflag &= ~O_EXCL;
                maxTries = 2;
            }
        } else {
            break;
        }
    }

    if (semaphore == SEM_FAILED) {
        setErrorString(QLatin1String("QSystemSemaphore::handle"));
        return false;
    }

    createdSemaphore = (oflag & O_EXCL) != 0;

    return true;
}

void QSystemSemaphorePrivate::cleanHandle()
{
    if (semaphore != SEM_FAILED) {
        if (::sem_close(semaphore) == -1) {
            setErrorString(QLatin1String("QSystemSemaphore::cleanHandle (sem_close)"));
#if defined QSYSTEMSEMAPHORE_DEBUG
            qDebug("QSystemSemaphore::cleanHandle sem_close failed.");
#endif
        }
        semaphore = SEM_FAILED;
    }

    if (createdSemaphore) {
        if (::sem_unlink(QFile::encodeName(fileName).constData()) == -1 && errno != ENOENT) {
            setErrorString(QLatin1String("QSystemSemaphore::cleanHandle (sem_unlink)"));
#if defined QSYSTEMSEMAPHORE_DEBUG
            qDebug("QSystemSemaphore::cleanHandle sem_unlink failed.");
#endif
        }
        createdSemaphore = false;
    }
}

bool QSystemSemaphorePrivate::modifySemaphore(int count)
{
    if (!handle())
        return false;

    if (count > 0) {
        int cnt = count;
        do {
            if (::sem_post(semaphore) == -1) {
                setErrorString(QLatin1String("QSystemSemaphore::modifySemaphore (sem_post)"));
#if defined QSYSTEMSEMAPHORE_DEBUG
                qDebug("QSystemSemaphore::modify sem_post failed %d %d", count, errno);
#endif
                // rollback changes to preserve the SysV semaphore behavior
                for ( ; cnt < count; ++cnt) {
                    int res;
                    EINTR_LOOP(res, ::sem_wait(semaphore));
                }
                return false;
            }
            --cnt;
        } while (cnt > 0);
    } else {
        int res;
        EINTR_LOOP(res, ::sem_wait(semaphore));
        if (res == -1) {
            // If the semaphore was removed be nice and create it and then modifySemaphore again
            if (errno == EINVAL || errno == EIDRM) {
                semaphore = SEM_FAILED;
                return modifySemaphore(count);
            }
            setErrorString(QLatin1String("QSystemSemaphore::modifySemaphore (sem_wait)"));
#if defined QSYSTEMSEMAPHORE_DEBUG
            qDebug("QSystemSemaphore::modify sem_wait failed %d %d", count, errno);
#endif
            return false;
        }
    }

    clearError();
    return true;
}

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMSEMAPHORE

#endif // QT_POSIX_IPC
