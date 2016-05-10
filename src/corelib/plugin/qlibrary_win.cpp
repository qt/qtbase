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

#include "qplatformdefs.h"
#include "qlibrary_p.h"
#include "qfile.h"
#include "qdir.h"
#include "qfileinfo.h"
#include <private/qfilesystementry_p.h>

#ifndef QT_NO_LIBRARY

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
    // WinCE
    // if (absolute)
    //     fileName
    //     fileName + ".dll"
    // else
    //     fileName + ".dll"
    //     fileName
    //     QFileInfo(fileName).absoluteFilePath()
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
    QStringList attempts;

    if (pluginState != IsAPlugin)
        attempts.append(fileName + QLatin1String(".dll"));

    // If the fileName is an absolute path we try that first, otherwise we
    // use the system-specific suffix first
    QFileSystemEntry fsEntry(fileName);
    if (fsEntry.isAbsolute()) {
        attempts.prepend(fileName);
    } else {
        attempts.append(fileName);
#if defined(Q_OS_WINCE)
        attempts.append(QFileInfo(fileName).absoluteFilePath());
#endif
    }
#ifdef Q_OS_WINRT
    if (fileName.startsWith(QLatin1Char('/')))
        attempts.prepend(QDir::rootPath() + fileName);
#endif

    for (const QString &attempt : qAsConst(attempts)) {
#ifndef Q_OS_WINRT
        pHnd = LoadLibrary((wchar_t*)QDir::toNativeSeparators(attempt).utf16());
#else // Q_OS_WINRT
        QString path = QDir::toNativeSeparators(QDir::current().relativeFilePath(attempt));
        pHnd = LoadPackagedLibrary((LPCWSTR)path.utf16(), 0);
        if (pHnd)
            qualifiedFileName = attempt;
#endif // !Q_OS_WINRT

        // If we have a handle or the last error is something other than "unable
        // to find the module", then bail out
        if (pHnd || ::GetLastError() != ERROR_MOD_NOT_FOUND)
            break;
    }

#ifndef Q_OS_WINRT
    SetErrorMode(oldmode);
#endif
    if (!pHnd) {
        errorString = QLibrary::tr("Cannot load library %1: %2").arg(
                    QDir::toNativeSeparators(fileName)).arg(qt_error_string());
    } else {
        // Query the actual name of the library that was loaded
        errorString.clear();

#ifndef Q_OS_WINRT
        wchar_t buffer[MAX_PATH];
        ::GetModuleFileName(pHnd, buffer, MAX_PATH);

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
                                        reinterpret_cast<const wchar_t *>(pHnd),
                                        &hmod);
            Q_ASSERT(!ok || hmod == pHnd);
            Q_UNUSED(ok);
        }
#endif // !Q_OS_WINRT
    }
    return (pHnd != 0);
}

bool QLibraryPrivate::unload_sys()
{
    if (!FreeLibrary(pHnd)) {
        errorString = QLibrary::tr("Cannot unload library %1: %2").arg(
                    QDir::toNativeSeparators(fileName)).arg(qt_error_string());
        return false;
    }
    errorString.clear();
    return true;
}

QFunctionPointer QLibraryPrivate::resolve_sys(const char* symbol)
{
#ifdef Q_OS_WINCE
    FARPROC address = GetProcAddress(pHnd, (const wchar_t*)QString::fromLatin1(symbol).utf16());
#else
    FARPROC address = GetProcAddress(pHnd, symbol);
#endif
    if (!address) {
        errorString = QLibrary::tr("Cannot resolve symbol \"%1\" in %2: %3").arg(
            QString::fromLatin1(symbol)).arg(
                    QDir::toNativeSeparators(fileName)).arg(qt_error_string());
    } else {
        errorString.clear();
    }
    return QFunctionPointer(address);
}
QT_END_NAMESPACE

#endif // QT_NO_LIBRARY
