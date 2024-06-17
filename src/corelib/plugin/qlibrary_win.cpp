// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformdefs.h"
#include "qlibrary_p.h"

#include "qdir.h"
#include "qfile.h"
#include "qfileinfo.h"
#include <private/qfilesystementry_p.h>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

extern QString qt_error_string(int code);

QStringList QLibraryPrivate::suffixes_sys(const QString& fullVersion)
{
    Q_UNUSED(fullVersion);
    return QStringList(QStringLiteral(".dll"));
}

bool QLibraryPrivate::load_sys()
{
    //avoid 'Bad Image' message box
    UINT oldmode = SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
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
        attempts.append(fileName + ".dll"_L1);

    // If the fileName is an absolute path we try that first, otherwise we
    // use the system-specific suffix first
    QFileSystemEntry fsEntry(fileName);
    if (fsEntry.isAbsolute())
        attempts.prepend(fileName);
    else
        attempts.append(fileName);

    locker.unlock();
    Handle hnd = nullptr;
    for (const QString &attempt : std::as_const(attempts)) {
        hnd = LoadLibrary(reinterpret_cast<const wchar_t*>(QDir::toNativeSeparators(attempt).utf16()));

        // If we have a handle or the last error is something other than "unable
        // to find the module", then bail out
        if (hnd || ::GetLastError() != ERROR_MOD_NOT_FOUND)
            break;
    }

    SetErrorMode(oldmode);
    locker.relock();
    if (!hnd) {
        errorString = QLibrary::tr("Cannot load library %1: %2").arg(
                    QDir::toNativeSeparators(fileName), qt_error_string());
    } else {
        // Query the actual name of the library that was loaded
        errorString.clear();

        wchar_t buffer[MAX_PATH];
        ::GetModuleFileName(hnd, buffer, MAX_PATH);

        QString moduleFileName = QString::fromWCharArray(buffer);
        moduleFileName.remove(0, 1 + moduleFileName.lastIndexOf(u'\\'));
        const QDir dir(fsEntry.path());
        if (dir.path() == "."_L1)
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

QFunctionPointer QLibraryPrivate::resolve_sys(const char *symbol)
{
    FARPROC address = GetProcAddress(pHnd.loadAcquire(), symbol);
    return QFunctionPointer(address);
}
QT_END_NAMESPACE
