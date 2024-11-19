// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsystemlibrary_p.h"
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/private/wcharhelpers_win_p.h>

/*!

    \internal
    \class QSystemLibrary
    \inmodule QtCore

    The purpose of this class is to load only libraries that are located in
    well-known and trusted locations on the filesystem. It does not suffer from
    the security problem that QLibrary has, therefore it will never search in
    the current directory.

    The search order is the same as the order in DLL Safe search mode Windows,
    except that we don't search:
    * The current directory
    * The 16-bit system directory. (normally \c{c:\windows\system})
    * The Windows directory.  (normally \c{c:\windows})

    This means that the effective search order is:
    1. Application path.
    2. System libraries path.
    3. Trying all paths inside the PATH environment variable.

    Note, when onlySystemDirectory is true it will skip 1) and 3).

    DLL Safe search mode is documented in the "Dynamic-Link Library Search
    Order" document on MSDN.
*/

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if !defined(QT_BOOTSTRAPPED)
extern QString qAppFileName();
#endif

static QString qSystemDirectory()
{
    static const QString result = []() -> QString {
        QVarLengthArray<wchar_t, MAX_PATH> fullPath = {};
        UINT retLen = ::GetSystemDirectoryW(fullPath.data(), MAX_PATH);
        if (retLen > MAX_PATH) {
            fullPath.resize(retLen);
            retLen = ::GetSystemDirectoryW(fullPath.data(), retLen);
        }
        // in some rare cases retLen might be 0
        return QString::fromWCharArray(fullPath.constData(), int(retLen));
    }();
    return result;
}

HINSTANCE QSystemLibrary::load(const wchar_t *libraryName, bool onlySystemDirectory /* = true */)
{
    if (onlySystemDirectory)
        return ::LoadLibraryExW(libraryName, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

    QStringList searchOrder;

#if !defined(QT_BOOTSTRAPPED)
    searchOrder << QFileInfo(qAppFileName()).path();
#endif
    searchOrder << qSystemDirectory();

    const QString PATH(QLatin1StringView(qgetenv("PATH")));
    searchOrder << PATH.split(u';', Qt::SkipEmptyParts);

    const QString fileName = QString::fromWCharArray(libraryName);

    // Start looking in the order specified
    for (int i = 0; i < searchOrder.count(); ++i) {
        QString fullPathAttempt = searchOrder.at(i);
        if (!fullPathAttempt.endsWith(u'\\')) {
            fullPathAttempt.append(u'\\');
        }
        fullPathAttempt.append(fileName);
        HINSTANCE inst = ::LoadLibrary(qt_castToWchar(fullPathAttempt));
        if (inst != nullptr)
            return inst;
    }
    return nullptr;
}

QT_END_NAMESPACE
