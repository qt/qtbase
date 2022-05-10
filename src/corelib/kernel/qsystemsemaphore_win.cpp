// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsystemsemaphore.h"
#include "qsystemsemaphore_p.h"
#include "qcoreapplication.h"
#include <qdebug.h>
#include <qt_windows.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
        semaphore = CreateSemaphore(0, initialValue, MAXLONG,
                                    reinterpret_cast<const wchar_t*>(fileName.utf16()));
        if (semaphore == NULL)
            setErrorString("QSystemSemaphore::handle"_L1);
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
            setErrorString("QSystemSemaphore::modifySemaphore"_L1);
#if defined QSYSTEMSEMAPHORE_DEBUG
            qDebug("QSystemSemaphore::modifySemaphore ReleaseSemaphore failed");
#endif
            return false;
        }
    } else {
        if (WAIT_OBJECT_0 != WaitForSingleObjectEx(semaphore, INFINITE, FALSE)) {
            setErrorString("QSystemSemaphore::modifySemaphore"_L1);
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
