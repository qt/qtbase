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

#include "qstandardpaths.h"

#include <qdir.h>
#include <private/qsystemlibrary_p.h>
#include <qstringlist.h>

#ifndef QT_BOOTSTRAPPED
#include <qcoreapplication.h>
#endif

#include <qoperatingsystemversion.h>
#include <qt_windows.h>
#include <shlobj.h>
#include <intshcut.h>
#include <qvarlengtharray.h>

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

static inline void appendTestMode(QString &path)
{
    if (QStandardPaths::isTestModeEnabled())
        path += QLatin1String("/qttest");
}

static bool isProcessLowIntegrity() {
#ifdef Q_CC_MINGW
    // GetCurrentProcessToken was introduced in MinGW w64 in v7
    // Disable function until Qt CI is updated
    return false;
#else
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows8)
        return false;
    // non-leaking pseudo-handle. Expanded inline function GetCurrentProcessToken()
    // (was made an inline function in Windows 8).
    const auto process_token = HANDLE(quintptr(-4));

    QVarLengthArray<char,256> token_info_buf(256);
    auto* token_info = reinterpret_cast<TOKEN_MANDATORY_LABEL*>(token_info_buf.data());
    DWORD token_info_length = token_info_buf.size();
    if (!GetTokenInformation(process_token, TokenIntegrityLevel, token_info, token_info_length, &token_info_length)) {
        // grow bufer and retry GetTokenInformation
        token_info_buf.resize(token_info_length);
        token_info = reinterpret_cast<TOKEN_MANDATORY_LABEL*>(token_info_buf.data());
        if (!GetTokenInformation(process_token, TokenIntegrityLevel, token_info, token_info_length, &token_info_length))
            return false; // assume "normal" process
    }

    // The GetSidSubAuthorityCount return-code is undefined on failure, so
    // there's no point in checking before dereferencing
    DWORD integrity_level = *GetSidSubAuthority(token_info->Label.Sid, *GetSidSubAuthorityCount(token_info->Label.Sid) - 1);
    return (integrity_level < SECURITY_MANDATORY_MEDIUM_RID);
#endif
}

// Map QStandardPaths::StandardLocation to KNOWNFOLDERID of SHGetKnownFolderPath()
static GUID writableSpecialFolderId(QStandardPaths::StandardLocation type)
{
    // folders for medium & high integrity processes
    static const GUID folderIds[] = {
        FOLDERID_Desktop,       // DesktopLocation
        FOLDERID_Documents,     // DocumentsLocation
        FOLDERID_Fonts,         // FontsLocation
        FOLDERID_Programs,      // ApplicationsLocation
        FOLDERID_Music,         // MusicLocation
        FOLDERID_Videos,        // MoviesLocation
        FOLDERID_Pictures,      // PicturesLocation
        GUID(), GUID(),         // TempLocation/HomeLocation
        FOLDERID_LocalAppData,  // AppLocalDataLocation ("Local" path), AppLocalDataLocation = DataLocation
        GUID(),                 // CacheLocation
        FOLDERID_LocalAppData,  // GenericDataLocation ("Local" path)
        GUID(),                 // RuntimeLocation
        FOLDERID_LocalAppData,  // ConfigLocation ("Local" path)
        GUID(), GUID(),         // DownloadLocation/GenericCacheLocation
        FOLDERID_LocalAppData,  // GenericConfigLocation ("Local" path)
        FOLDERID_RoamingAppData,// AppDataLocation ("Roaming" path)
        FOLDERID_LocalAppData,  // AppConfigLocation ("Local" path)
    };
    Q_STATIC_ASSERT(sizeof(folderIds) / sizeof(folderIds[0]) == size_t(QStandardPaths::AppConfigLocation + 1));

    // folders for low integrity processes
    static const GUID folderIds_li[] = {
        FOLDERID_Desktop,        // DesktopLocation
        FOLDERID_Documents,      // DocumentsLocation
        FOLDERID_Fonts,          // FontsLocation
        FOLDERID_Programs,       // ApplicationsLocation
        FOLDERID_Music,          // MusicLocation
        FOLDERID_Videos,         // MoviesLocation
        FOLDERID_Pictures,       // PicturesLocation
        GUID(), GUID(),          // TempLocation/HomeLocation
        FOLDERID_LocalAppDataLow,// AppLocalDataLocation ("Local" path), AppLocalDataLocation = DataLocation
        GUID(),                  // CacheLocation
        FOLDERID_LocalAppDataLow,// GenericDataLocation ("Local" path)
        GUID(),                  // RuntimeLocation
        FOLDERID_LocalAppDataLow,// ConfigLocation ("Local" path)
        GUID(), GUID(),          // DownloadLocation/GenericCacheLocation
        FOLDERID_LocalAppDataLow,// GenericConfigLocation ("Local" path)
        FOLDERID_RoamingAppData, // AppDataLocation ("Roaming" path)
        FOLDERID_LocalAppDataLow,// AppConfigLocation ("Local" path)
    };
    Q_STATIC_ASSERT(sizeof(folderIds_li) == sizeof(folderIds));

    static bool low_integrity_process = isProcessLowIntegrity();
    if (size_t(type) < sizeof(folderIds) / sizeof(folderIds[0]))
        return low_integrity_process ? folderIds_li[type] : folderIds[type];
    return GUID();
}

// Convenience for SHGetKnownFolderPath().
static QString sHGetKnownFolderPath(const GUID &clsid)
{
    QString result;
    typedef HRESULT (WINAPI *GetKnownFolderPath)(const GUID&, DWORD, HANDLE, LPWSTR*);

    static const GetKnownFolderPath sHGetKnownFolderPath = // Vista onwards.
        reinterpret_cast<GetKnownFolderPath>(QSystemLibrary::resolve(QLatin1String("shell32"), "SHGetKnownFolderPath"));

    LPWSTR path;
    if (Q_LIKELY(sHGetKnownFolderPath && SUCCEEDED(sHGetKnownFolderPath(clsid, KF_FLAG_DONT_VERIFY, 0, &path)))) {
        result = convertCharArray(path);
        CoTaskMemFree(path);
    }
    return result;
}

QString QStandardPaths::writableLocation(StandardLocation type)
{
    QString result;
    switch (type) {
    case DownloadLocation:
        result = sHGetKnownFolderPath(FOLDERID_Downloads);
        if (result.isEmpty())
            result = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        break;

    case CacheLocation:
        // Although Microsoft has a Cache key it is a pointer to IE's cache, not a cache
        // location for everyone.  Most applications seem to be using a
        // cache directory located in their AppData directory
        result = sHGetKnownFolderPath(writableSpecialFolderId(AppLocalDataLocation));
        if (!result.isEmpty()) {
            appendTestMode(result);
            appendOrganizationAndApp(result);
            result += QLatin1String("/cache");
        }
        break;

    case GenericCacheLocation:
        result = sHGetKnownFolderPath(writableSpecialFolderId(GenericDataLocation));
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
        result = sHGetKnownFolderPath(writableSpecialFolderId(type));
        if (!result.isEmpty() && isConfigLocation(type)) {
            appendTestMode(result);
            if (!isGenericConfigLocation(type))
                appendOrganizationAndApp(result);
        }
        break;
    }
    return result;
}

#ifndef QT_BOOTSTRAPPED
extern QString qAppFileName();
#endif

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    QStringList dirs;
    const QString localDir = writableLocation(type);
    if (!localDir.isEmpty())
        dirs.append(localDir);

    // type-specific handling goes here
    if (isConfigLocation(type)) {
        QString programData = sHGetKnownFolderPath(FOLDERID_ProgramData);
        if (!programData.isEmpty()) {
            if (!isGenericConfigLocation(type))
                appendOrganizationAndApp(programData);
            dirs.append(programData);
        }
#ifndef QT_BOOTSTRAPPED
        // Note: QCoreApplication::applicationDirPath(), while static, requires
        // an application instance. But we might need to resolve the standard
        // locations earlier than that, so we fall back to qAppFileName().
        QString applicationDirPath = qApp ? QCoreApplication::applicationDirPath()
            : QFileInfo(qAppFileName()).path();
        dirs.append(applicationDirPath);
        const QString dataDir = applicationDirPath + QLatin1String("/data");
        dirs.append(dataDir);

        if (!isGenericConfigLocation(type)) {
            QString appDataDir = dataDir;
            appendOrganizationAndApp(appDataDir);
            if (appDataDir != dataDir)
                dirs.append(appDataDir);
        }
#endif // !QT_BOOTSTRAPPED
    } // isConfigLocation()

    return dirs;
}

QT_END_NAMESPACE

#endif // QT_NO_STANDARDPATHS
