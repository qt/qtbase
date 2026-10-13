// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qsystemsemaphore.h"
#include "qsystemsemaphore_p.h"
#include "qcoreapplication.h"
#include <qdebug.h>
#include <qt_windows.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if QT_CONFIG(systemsemaphore)

void QSystemSemaphorePrivate::setWindowsErrorString(QLatin1StringView function)
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
        errorString = QCoreApplication::translate("QSystemSemaphore", "%1: unknown error: %2")
                .arg(function, qt_error_string(windowsError));
        error = QSystemSemaphore::UnknownError;
#if defined QSYSTEMSEMAPHORE_DEBUG
        qDebug() << errorString << "key" << key;
#endif
    }
}

HANDLE QSystemSemaphoreWin32::handle(QSystemSemaphorePrivate *self, QSystemSemaphore::AccessMode)
{
    // don't allow making handles on empty keys
    if (self->nativeKey.isEmpty())
        return 0;

    // Create it if it doesn't already exists.
    if (semaphore == 0) {
        semaphore = CreateSemaphore(0, self->initialValue, MAXLONG,
                                    reinterpret_cast<const wchar_t*>(self->nativeKey.nativeKey().utf16()));
        if (semaphore == NULL)
            self->setWindowsErrorString("QSystemSemaphore::handle"_L1);
    }

    return semaphore;
}

void QSystemSemaphoreWin32::cleanHandle(QSystemSemaphorePrivate *)
{
    if (semaphore && !CloseHandle(semaphore)) {
#if defined QSYSTEMSEMAPHORE_DEBUG
        qDebug("QSystemSemaphoreWin32::CloseHandle: sem failed");
#endif
    }
    semaphore = 0;
}

bool QSystemSemaphoreWin32::modifySemaphore(QSystemSemaphorePrivate *self, int count)
{
    if (handle(self, QSystemSemaphore::Open) == nullptr)
        return false;

    if (count > 0) {
        if (0 == ReleaseSemaphore(semaphore, count, 0)) {
            self->setWindowsErrorString("QSystemSemaphore::modifySemaphore"_L1);
#if defined QSYSTEMSEMAPHORE_DEBUG
            qDebug("QSystemSemaphore::modifySemaphore ReleaseSemaphore failed");
#endif
            return false;
        }
    } else {
        if (WAIT_OBJECT_0 != WaitForSingleObjectEx(semaphore, INFINITE, FALSE)) {
            self->setWindowsErrorString("QSystemSemaphore::modifySemaphore"_L1);
#if defined QSYSTEMSEMAPHORE_DEBUG
            qDebug("QSystemSemaphore::modifySemaphore WaitForSingleObject failed");
#endif
            return false;
        }
    }

    self->clearError();
    return true;
}

#endif // QT_CONFIG(systemsemaphore)

QT_END_NAMESPACE
