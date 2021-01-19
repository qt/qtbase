/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qplatformdefs.h"
#include "qlibrary_p.h"
#include "qfile.h"
#include "qdir.h"
#include "qfileinfo.h"
#include <private/qfilesystementry_p.h>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

extern QString qt_error_string(int code);

QStringList QLibraryPrivate::suffixes_sys(const QString& fullVersion)
{
    Q_UNUSED(fullVersion);
    return QStringList(QStringLiteral(".dll"));
}

QStringList QLibraryPrivate::prefixes_sys()
{
    return QStringList();
}

bool QLibraryPrivate::load_sys()
{
#ifndef Q_OS_WINRT
    //avoid 'Bad Image' message box
    UINT oldmode = SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
#endif
    // We make the following attempts at locating the library:
    //
    // Windows
    // if (absolute)
    //     fileName
    //     fileName + ".dll"
    // else
    //     fileName + ".dll"
    //     fileName
    //
    // NB If it's a plugin we do not ever try the ".dll" extension
    QMutexLocker locker(&mutex);
    QStringList attempts;

    if (pluginState != IsAPlugin)
        attempts.append(fileName + QLatin1String(".dll"));

    // If the fileName is an absolute path we try that first, otherwise we
    // use the system-specific suffix first
    QFileSystemEntry fsEntry(fileName);
    if (fsEntry.isAbsolute())
        attempts.prepend(fileName);
    else
        attempts.append(fileName);
#ifdef Q_OS_WINRT
    if (fileName.startsWith(QLatin1Char('/')))
        attempts.prepend(QDir::rootPath() + fileName);
#endif

    locker.unlock();
    Handle hnd = nullptr;
    for (const QString &attempt : qAsConst(attempts)) {
#ifndef Q_OS_WINRT
        hnd = LoadLibrary(reinterpret_cast<const wchar_t*>(QDir::toNativeSeparators(attempt).utf16()));
#else // Q_OS_WINRT
        QString path = QDir::toNativeSeparators(QDir::current().relativeFilePath(attempt));
        hnd = LoadPackagedLibrary(reinterpret_cast<LPCWSTR>(path.utf16()), 0);
        if (hnd)
            qualifiedFileName = attempt;
#endif // !Q_OS_WINRT

        // If we have a handle or the last error is something other than "unable
        // to find the module", then bail out
        if (hnd || ::GetLastError() != ERROR_MOD_NOT_FOUND)
            break;
    }

#ifndef Q_OS_WINRT
    SetErrorMode(oldmode);
#endif
    locker.relock();
    if (!hnd) {
        errorString = QLibrary::tr("Cannot load library %1: %2").arg(
                    QDir::toNativeSeparators(fileName), qt_error_string());
    } else {
        // Query the actual name of the library that was loaded
        errorString.clear();

#ifndef Q_OS_WINRT
        wchar_t buffer[MAX_PATH];
        ::GetModuleFileName(hnd, buffer, MAX_PATH);

        QString moduleFileName = QString::fromWCharArray(buffer);
        moduleFileName.remove(0, 1 + moduleFileName.lastIndexOf(QLatin1Char('\\')));
        const QDir dir(fsEntry.path());
        if (dir.path() == QLatin1String("."))
            qualifiedFileName = moduleFileName;
        else
            qualifiedFileName = dir.filePath(moduleFileName);

        if (loadHints() & QLibrary::PreventUnloadHint) {
            // prevent the unloading of this component
            HMODULE hmod;
            bool ok = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_PIN |
                                        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                                        reinterpret_cast<const wchar_t *>(hnd),
                                        &hmod);
            Q_ASSERT(!ok || hmod == hnd);
            Q_UNUSED(ok);
        }
#endif // !Q_OS_WINRT
    }
    pHnd.storeRelaxed(hnd);
    return (pHnd != nullptr);
}

bool QLibraryPrivate::unload_sys()
{
    if (!FreeLibrary(pHnd.loadAcquire())) {
        errorString = QLibrary::tr("Cannot unload library %1: %2").arg(
                    QDir::toNativeSeparators(fileName),  qt_error_string());
        return false;
    }
    errorString.clear();
    return true;
}

QFunctionPointer QLibraryPrivate::resolve_sys(const char* symbol)
{
    FARPROC address = GetProcAddress(pHnd.loadAcquire(), symbol);
    return QFunctionPointer(address);
}
QT_END_NAMESPACE
