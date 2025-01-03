// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qstandardpaths.h"

#include <qdir.h>
#include <qstringlist.h>

#ifndef QT_BOOTSTRAPPED
#include <qcoreapplication.h>
#endif

#include <qt_windows.h>
#include <shlobj.h>
#include <intshcut.h>
#include <qvarlengtharray.h>

#ifndef QT_NO_STANDARDPATHS

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
        path += u'/' + org;
    const QString &appName = QCoreApplication::applicationName();
    if (!appName.isEmpty())
        path += u'/' + appName;
#else // !QT_BOOTSTRAPPED
    Q_UNUSED(path);
#endif
}

static inline void appendTestMode(QString &path)
{
    if (QStandardPaths::isTestModeEnabled())
        path += "/qttest"_L1;
}

static bool isProcessLowIntegrity()
{
    // same as GetCurrentProcessToken()
    const auto process_token = HANDLE(quintptr(-4));

    QVarLengthArray<char,256> token_info_buf(256);
    auto* token_info = reinterpret_cast<TOKEN_MANDATORY_LABEL*>(token_info_buf.data());
    DWORD token_info_length = token_info_buf.size();
    if (!GetTokenInformation(process_token, TokenIntegrityLevel, token_info, token_info_length, &token_info_length)) {
        // grow buffer and retry GetTokenInformation
        token_info_buf.resize(token_info_length);
        token_info = reinterpret_cast<TOKEN_MANDATORY_LABEL*>(token_info_buf.data());
        if (!GetTokenInformation(process_token, TokenIntegrityLevel, token_info, token_info_length, &token_info_length))
            return false; // assume "normal" process
    }

    // The GetSidSubAuthorityCount return-code is undefined on failure, so
    // there's no point in checking before dereferencing
    DWORD integrity_level = *GetSidSubAuthority(token_info->Label.Sid, *GetSidSubAuthorityCount(token_info->Label.Sid) - 1);
    return (integrity_level < SECURITY_MANDATORY_MEDIUM_RID);
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
        FOLDERID_LocalAppData,  // AppLocalDataLocation ("Local" path)
        GUID(),                 // CacheLocation
        FOLDERID_LocalAppData,  // GenericDataLocation ("Local" path)
        GUID(),                 // RuntimeLocation
        FOLDERID_LocalAppData,  // ConfigLocation ("Local" path)
        FOLDERID_Downloads,     // DownloadLocation
        GUID(),                 // GenericCacheLocation
        FOLDERID_LocalAppData,  // GenericConfigLocation ("Local" path)
        FOLDERID_RoamingAppData,// AppDataLocation ("Roaming" path)
        FOLDERID_LocalAppData,  // AppConfigLocation ("Local" path)
        FOLDERID_Public,        // PublicShareLocation
        FOLDERID_Templates,     // TemplatesLocation
    };
    static_assert(sizeof(folderIds) / sizeof(folderIds[0]) == size_t(QStandardPaths::TemplatesLocation + 1));

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
        FOLDERID_LocalAppDataLow,// AppLocalDataLocation ("Local" path)
        GUID(),                  // CacheLocation
        FOLDERID_LocalAppDataLow,// GenericDataLocation ("Local" path)
        GUID(),                  // RuntimeLocation
        FOLDERID_LocalAppDataLow,// ConfigLocation ("Local" path)
        FOLDERID_Downloads,      // DownloadLocation
        GUID(),                  // GenericCacheLocation
        FOLDERID_LocalAppDataLow,// GenericConfigLocation ("Local" path)
        FOLDERID_RoamingAppData, // AppDataLocation ("Roaming" path)
        FOLDERID_LocalAppDataLow,// AppConfigLocation ("Local" path)
        FOLDERID_Public,         // PublicShareLocation
        FOLDERID_Templates,      // TemplatesLocation
    };
    static_assert(sizeof(folderIds_li) == sizeof(folderIds));

    static bool low_integrity_process = isProcessLowIntegrity();
    if (size_t(type) < sizeof(folderIds) / sizeof(folderIds[0]))
        return low_integrity_process ? folderIds_li[type] : folderIds[type];
    return GUID();
}

// Convenience for SHGetKnownFolderPath().
static QString sHGetKnownFolderPath(const GUID &clsid)
{
    QString result;
    LPWSTR path;
    if (Q_LIKELY(SUCCEEDED(SHGetKnownFolderPath(clsid, KF_FLAG_DONT_VERIFY, 0, &path)))) {
        result = convertCharArray(path);
        CoTaskMemFree(path);
    }
    return result;
}

QString QStandardPaths::writableLocation(StandardLocation type)
{
    QString result;
    switch (type) {
    case CacheLocation:
        // Although Microsoft has a Cache key it is a pointer to IE's cache, not a cache
        // location for everyone.  Most applications seem to be using a
        // cache directory located in their AppData directory
        result = sHGetKnownFolderPath(writableSpecialFolderId(AppLocalDataLocation));
        if (!result.isEmpty()) {
            appendTestMode(result);
            appendOrganizationAndApp(result);
            result += "/cache"_L1;
        }
        break;

    case GenericCacheLocation:
        result = sHGetKnownFolderPath(writableSpecialFolderId(GenericDataLocation));
        if (!result.isEmpty()) {
            appendTestMode(result);
            result += "/cache"_L1;
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
        const QString dataDir = applicationDirPath + "/data"_L1;
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
