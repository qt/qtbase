/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qsystemlock.h"
#include "qsystemlock_p.h"
#include <qdebug.h>
#include <QtCore>
QSystemLockPrivate::QSystemLockPrivate() :
        semaphore(0), semaphoreLock(0),
	lockCount(0), error(QSystemLock::NoError)
{
}

void QSystemLockPrivate::setErrorString(const QString &function)
{
    BOOL windowsError = GetLastError();
    if (windowsError == 0)
        return;
    errorString = function + QLatin1String(": ")
                    + QLatin1String("Unknown error");
    error = QSystemLock::UnknownError;
    qWarning() << errorString << "key" << key << (int)windowsError << semaphore << semaphoreLock;
}

/*!
    \internal

    Setup the semaphore
 */
HANDLE QSystemLockPrivate::handle()
{
    // don't allow making handles on empty keys
    if (key.isEmpty())
        return 0;

    // Create it if it doesn't already exists.
    if (semaphore == 0) {
        QString safeName = makeKeyFileName();
        semaphore = CreateSemaphore(0, MAX_LOCKS, MAX_LOCKS, (wchar_t*)safeName.utf16());

        if (semaphore == 0) {
            setErrorString(QLatin1String("QSystemLockPrivate::handle"));
            return 0;
        }
    }

    if (semaphoreLock == 0) {
        QString safeLockName = QSharedMemoryPrivate::makePlatformSafeKey(key + QLatin1String("lock"), QLatin1String("qipc_systemlock_"));
        semaphoreLock = CreateSemaphore(0, 1, 1, (wchar_t*)safeLockName.utf16());

        if (semaphoreLock == 0) {
            setErrorString(QLatin1String("QSystemLockPrivate::handle"));
            return 0;
        }
    }

    return semaphore;
}

/*!
    \internal

    Cleanup the semaphore
 */
void QSystemLockPrivate::cleanHandle()
{
    if (semaphore && !CloseHandle(semaphore))
        setErrorString(QLatin1String("QSystemLockPrivate::cleanHandle:"));
    if (semaphoreLock && !CloseHandle(semaphoreLock))
        setErrorString(QLatin1String("QSystemLockPrivate::cleanHandle:"));
    semaphore = 0;
    semaphoreLock = 0;
}

bool QSystemLockPrivate::lock(HANDLE handle, int count)
{
    if (count == 1) {
	WaitForSingleObject(handle, INFINITE);
	return true;
    }

    int i = count;
    while (i > 0) {
	if (WAIT_OBJECT_0 == WaitForSingleObject(handle, 0)) {
	    --i;
	} else {
	    // undo what we have done, sleep and then try again later
	    ReleaseSemaphore(handle, (count - i), 0);
	    i = count;
	    ReleaseSemaphore(semaphoreLock, 1, 0);
	    Sleep(1);
	    WaitForSingleObject(semaphoreLock, INFINITE);
	}
    }
    return true;
}

bool QSystemLockPrivate::unlock(HANDLE handle, int count)
{
    if (0 == ReleaseSemaphore(handle, count, 0)) {
        setErrorString(QLatin1String("QSystemLockPrivate::unlock"));
        return false;
    }
    return true;
}

/*!
    \internal

    modifySemaphore handles recursive behavior and modifies the semaphore.
 */
bool QSystemLockPrivate::modifySemaphore(QSystemLockPrivate::Operation op,
        QSystemLock::LockMode mode)
{
    if (0 == handle())
        return false;

    if ((lockCount == 0 && op == Lock) || (lockCount > 0 && op == Unlock)) {
        if (op == Unlock) {
            --lockCount;
            if (lockCount < 0)
                qFatal("%s: lockCount must not be negative", Q_FUNC_INFO);
            if (lockCount > 0)
                return true;
        }

        int count = (mode == QSystemLock::ReadWrite) ? MAX_LOCKS : 1;
        if (op == Lock) {
            lock(semaphoreLock, 1);
            lock(semaphore, count);
            if (count != MAX_LOCKS) unlock(semaphoreLock, 1);
	    lockedMode = mode;
        } else {
            if (count == MAX_LOCKS) unlock(semaphoreLock, 1);
	    unlock(semaphore, count);
        }

    }
    if (op == Lock)
        lockCount++;

    return true;
}

