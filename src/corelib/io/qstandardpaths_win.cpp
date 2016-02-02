/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qstandardpaths.h"

#include <qdir.h>
#include <private/qsystemlibrary_p.h>
#include <qstringlist.h>

#ifndef QT_BOOTSTRAPPED
#include <qcoreapplication.h>
#endif

const GUID qCLSID_FOLDERID_Downloads = { 0x374de290, 0x123f, 0x4565, { 0x91, 0x64,  0x39,  0xc4,  0x92,  0x5e,  0x46,  0x7b } };

#include <qt_windows.h>
#include <shlobj.h>
#if !defined(Q_OS_WINCE)
#  include <intshcut.h>
#else
#  if !defined(STANDARDSHELL_UI_MODEL)
#    include <winx.h>
#  endif
#endif

#ifndef CSIDL_MYMUSIC
#define CSIDL_MYMUSIC 13
#define CSIDL_MYVIDEO 14
#endif

#ifndef QT_NO_STANDARDPATHS

QT_BEGIN_NAMESPACE

static QString convertCharArray(const wchar_t *path)
{
    return QDir::fromNativeSeparators(QString::fromWCharArray(path));
}

static inline bool isGenericConfigLocation(QStandardPaths::StandardLocation type)
{
    return type == QStandardPaths::GenericConfigLocation || type == QStandardPaths::GenericDataLocation;
}

static inline bool isConfigLocation(QStandardPaths::StandardLocation type)
{
    return type == QStandardPaths::ConfigLocation || type == QStandardPaths::AppConfigLocation
        || type == QStandardPaths::AppDataLocation || type == QStandardPaths::AppLocalDataLocation
        || isGenericConfigLocation(type);
}

static void appendOrganizationAndApp(QString &path) // Courtesy qstandardpaths_unix.cpp
{
#ifndef QT_BOOTSTRAPPED
    const QString &org = QCoreApplication::organizationName();
    if (!org.isEmpty())
        path += QLatin1Char('/') + org;
    const QString &appName = QCoreApplication::applicationName();
    if (!appName.isEmpty())
        path += QLatin1Char('/') + appName;
#else // !QT_BOOTSTRAPPED
    Q_UNUSED(path)
#endif
}

static inline QString displayName(QStandardPaths::StandardLocation type)
{
#ifndef QT_BOOTSTRAPPED
    return QStandardPaths::displayName(type);
#else
    return QString::number(type);
#endif
}

static inline void appendTestMode(QString &path)
{
    if (QStandardPaths::isTestModeEnabled())
        path += QLatin1String("/qttest");
}

// Map QStandardPaths::StandardLocation to CLSID of SHGetSpecialFolderPath()
static int writableSpecialFolderClsid(QStandardPaths::StandardLocation type)
{
    static const int clsids[] = {
        CSIDL_DESKTOPDIRECTORY, // DesktopLocation
        CSIDL_PERSONAL,         // DocumentsLocation
        CSIDL_FONTS,            // FontsLocation
        CSIDL_PROGRAMS,         // ApplicationsLocation
        CSIDL_MYMUSIC,          // MusicLocation
        CSIDL_MYVIDEO,          // MoviesLocation
        CSIDL_MYPICTURES,       // PicturesLocation
        -1, -1,                 // TempLocation/HomeLocation
        CSIDL_LOCAL_APPDATA,    // AppLocalDataLocation ("Local" path), AppLocalDataLocation = DataLocation
        -1,                     // CacheLocation
        CSIDL_LOCAL_APPDATA,    // GenericDataLocation ("Local" path)
        -1,                     // RuntimeLocation
        CSIDL_LOCAL_APPDATA,    // ConfigLocation ("Local" path)
        -1, -1,                 // DownloadLocation/GenericCacheLocation
        CSIDL_LOCAL_APPDATA,    // GenericConfigLocation ("Local" path)
        CSIDL_APPDATA,          // AppDataLocation ("Roaming" path)
        CSIDL_LOCAL_APPDATA,    // AppConfigLocation ("Local" path)
    };

    Q_STATIC_ASSERT(sizeof(clsids) / sizeof(clsids[0]) == size_t(QStandardPaths::AppConfigLocation + 1));
    return size_t(type) < sizeof(clsids) / sizeof(clsids[0]) ? clsids[type] : -1;
};

// Convenience for SHGetSpecialFolderPath().
static QString sHGetSpecialFolderPath(int clsid, QStandardPaths::StandardLocation type, bool warn = false)
{
    QString result;
    wchar_t path[MAX_PATH];
    if (Q_LIKELY(clsid >= 0 && SHGetSpecialFolderPath(0, path, clsid, FALSE))) {
        result = convertCharArray(path);
    } else {
        if (warn) {
            qErrnoWarning("SHGetSpecialFolderPath() failed for standard location \"%s\", clsid=0x%x.",
                          qPrintable(displayName(type)), clsid);
        }
    }
    return result;
}

// Convenience for SHGetKnownFolderPath().
static QString sHGetKnownFolderPath(const GUID &clsid, QStandardPaths::StandardLocation type, bool warn = false)
{
    QString result;
#ifndef Q_OS_WINCE
    typedef HRESULT (WINAPI *GetKnownFolderPath)(const GUID&, DWORD, HANDLE, LPWSTR*);

    static const GetKnownFolderPath sHGetKnownFolderPath = // Vista onwards.
        reinterpret_cast<GetKnownFolderPath>(QSystemLibrary::resolve(QLatin1String("shell32"), "SHGetKnownFolderPath"));

    LPWSTR path;
    if (Q_LIKELY(sHGetKnownFolderPath && SUCCEEDED(sHGetKnownFolderPath(clsid, 0, 0, &path)))) {
        result = convertCharArray(path);
        CoTaskMemFree(path);
    } else {
        if (warn) {
            qErrnoWarning("SHGetKnownFolderPath() failed for standard location \"%s\".",
                          qPrintable(displayName(type)));
        }
    }
#else // !Q_OS_WINCE
    Q_UNUSED(clsid)
    Q_UNUSED(type)
    Q_UNUSED(warn)
#endif
    return result;
}

QString QStandardPaths::writableLocation(StandardLocation type)
{
    QString result;
    switch (type) {
    case DownloadLocation:
        result = sHGetKnownFolderPath(qCLSID_FOLDERID_Downloads, type);
        if (result.isEmpty())
            result = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        break;

    case CacheLocation:
        // Although Microsoft has a Cache key it is a pointer to IE's cache, not a cache
        // location for everyone.  Most applications seem to be using a
        // cache directory located in their AppData directory
        result = sHGetSpecialFolderPath(writableSpecialFolderClsid(AppLocalDataLocation), type, /* warn */ true);
        if (!result.isEmpty()) {
            appendTestMode(result);
            appendOrganizationAndApp(result);
            result += QLatin1String("/cache");
        }
        break;

    case GenericCacheLocation:
        result = sHGetSpecialFolderPath(writableSpecialFolderClsid(GenericDataLocation), type, /* warn */ true);
        if (!result.isEmpty()) {
            appendTestMode(result);
            result += QLatin1String("/cache");
        }
        break;

    case RuntimeLocation:
    case HomeLocation:
        result = QDir::homePath();
        break;

    case TempLocation:
        result = QDir::tempPath();
        break;

    default:
        result = sHGetSpecialFolderPath(writableSpecialFolderClsid(type), type, /* warn */ isConfigLocation(type));
        if (!result.isEmpty() && isConfigLocation(type)) {
            appendTestMode(result);
            if (!isGenericConfigLocation(type))
                appendOrganizationAndApp(result);
        }
        break;
    }
    return result;
}

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    QStringList dirs;
    const QString localDir = writableLocation(type);
    if (!localDir.isEmpty())
        dirs.append(localDir);

    // type-specific handling goes here
#ifndef Q_OS_WINCE
    if (isConfigLocation(type)) {
        QString programData = sHGetSpecialFolderPath(CSIDL_COMMON_APPDATA, type);
        if (!programData.isEmpty()) {
            if (!isGenericConfigLocation(type))
                appendOrganizationAndApp(programData);
            dirs.append(programData);
        }
#  ifndef QT_BOOTSTRAPPED
        dirs.append(QCoreApplication::applicationDirPath());
        dirs.append(QCoreApplication::applicationDirPath() + QLatin1String("/data"));
#  endif // !QT_BOOTSTRAPPED
    } // isConfigLocation()
#endif // !Q_OS_WINCE

    return dirs;
}

QT_END_NAMESPACE

#endif // QT_NO_STANDARDPATHS
