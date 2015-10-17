/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include "qcoreapplication.h"
#include <qdebug.h>
#include <qt_windows.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SYSTEMSEMAPHORE

QSystemSemaphorePrivate::QSystemSemaphorePrivate() :
        semaphore(0), error(QSystemSemaphore::NoError)
{
}

void QSystemSemaphorePrivate::setErrorString(const QString &function)
{
    BOOL windowsError = GetLastError();
    if (windowsError == 0)
        return;

    switch (windowsError) {
    case ERROR_NO_SYSTEM_RESOURCES:
    case ERROR_NOT_ENOUGH_MEMORY:
        error = QSystemSemaphore::OutOfResources;
        errorString = QCoreApplication::translate("QSystemSemaphore", "%1: out of resources").arg(function);
        break;
    case ERROR_ACCESS_DENIED:
        error = QSystemSemaphore::PermissionDenied;
        errorString = QCoreApplication::translate("QSystemSemaphore", "%1: permission denied").arg(function);
        break;
    default:
        errorString = QCoreApplication::translate("QSystemSemaphore", "%1: unknown error %2").arg(function).arg(windowsError);
        error = QSystemSemaphore::UnknownError;
#if defined QSYSTEMSEMAPHORE_DEBUG
        qDebug() << errorString << "key" << key;
#endif
    }
}

HANDLE QSystemSemaphorePrivate::handle(QSystemSemaphore::AccessMode)
{
    // don't allow making handles on empty keys
    if (key.isEmpty())
        return 0;

    // Create it if it doesn't already exists.
    if (semaphore == 0) {
#if defined(Q_OS_WINRT)
        semaphore = CreateSemaphoreEx(0, initialValue, MAXLONG, (wchar_t*)fileName.utf16(), 0, SEMAPHORE_ALL_ACCESS);
#else
        semaphore = CreateSemaphore(0, initialValue, MAXLONG, (wchar_t*)fileName.utf16());
#endif
        if (semaphore == NULL)
            setErrorString(QLatin1String("QSystemSemaphore::handle"));
    }

    return semaphore;
}

void QSystemSemaphorePrivate::cleanHandle()
{
    if (semaphore && !CloseHandle(semaphore)) {
#if defined QSYSTEMSEMAPHORE_DEBUG
        qDebug("QSystemSemaphorePrivate::CloseHandle: sem failed");
#endif
    }
    semaphore = 0;
}

bool QSystemSemaphorePrivate::modifySemaphore(int count)
{
    if (0 == handle())
        return false;

    if (count > 0) {
        if (0 == ReleaseSemaphore(semaphore, count, 0)) {
            setErrorString(QLatin1String("QSystemSemaphore::modifySemaphore"));
#if defined QSYSTEMSEMAPHORE_DEBUG
            qDebug("QSystemSemaphore::modifySemaphore ReleaseSemaphore failed");
#endif
            return false;
        }
    } else {
#if !defined(Q_OS_WINCE)
        if (WAIT_OBJECT_0 != WaitForSingleObjectEx(semaphore, INFINITE, FALSE)) {
#else
        if (WAIT_OBJECT_0 != WaitForSingleObject(semaphore, INFINITE)) {
#endif
            setErrorString(QLatin1String("QSystemSemaphore::modifySemaphore"));
#if defined QSYSTEMSEMAPHORE_DEBUG
            qDebug("QSystemSemaphore::modifySemaphore WaitForSingleObject failed");
#endif
            return false;
        }
    }

    clearError();
    return true;
}

#endif //QT_NO_SYSTEMSEMAPHORE

QT_END_NAMESPACE
