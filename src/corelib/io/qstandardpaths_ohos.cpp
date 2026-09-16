// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qstandardpaths.h"

#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qohosappcontext_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohosmemoizingjsthreadfetcher_p.h>
#include <QtCore/qdir.h>
#include <QtCore/qlogging.h>
#include <filemanagement/environment/oh_environment.h>
#include <optional>

#ifndef QT_NO_STANDARDPATHS

QT_BEGIN_NAMESPACE

namespace {

template<FileManagement_ErrCode (*ohEnvironmentDirGetter)(char **)>
std::optional<QString> tryGetUserDirFromOhEnvironment(QOhosJsState &)
{
    char *dirPath = nullptr;
    FileManagement_ErrCode dirGetterRetVal = ohEnvironmentDirGetter(&dirPath);
    if (dirGetterRetVal != FileManagement_ErrCode::ERR_OK || dirPath == nullptr) {
        qOhosPrintfDebug("OH_Environment_GetUser* dir getter failed, retval: %d", static_cast<int>(dirGetterRetVal));
        return {};
    }

    auto result = QString::fromUtf8(dirPath);

    ::free(dirPath);

    return result;
}

QOhosMemoizingJsThreadFetcher<QString> userDesktopDirFetcher(
    tryGetUserDirFromOhEnvironment<OH_Environment_GetUserDesktopDir>, "OH_Environment_GetUserDesktopDir()");

QOhosMemoizingJsThreadFetcher<QString> userDocumentDirFetcher(
    tryGetUserDirFromOhEnvironment<OH_Environment_GetUserDocumentDir>, "OH_Environment_GetUserDocumentDir()");

QOhosMemoizingJsThreadFetcher<QString> userDownloadDirFetcher(
    tryGetUserDirFromOhEnvironment<OH_Environment_GetUserDownloadDir>, "OH_Environment_GetUserDownloadDir()");

}

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    return QStringList(writableLocation(type));
}

QString QStandardPaths::writableLocation(StandardLocation type)
{
    auto testDirSuffix = QStandardPaths::isTestModeEnabled() ? QLatin1String("/.qttest") : QLatin1String("");

    switch (type) {
    case DesktopLocation:
        // NOTE: return empty string if the dir isn't available, even in the doc says otherwise
        return userDesktopDirFetcher.optValue().value_or(QLatin1String());
    case DocumentsLocation:
        // NOTE: return empty string if the dir isn't available, even in the doc says otherwise
        return userDocumentDirFetcher.optValue().value_or(QLatin1String());
    case FontsLocation:
        // NOT SUPPORTED
        break;
    case ApplicationsLocation:
        // NOT SUPPORTED
        break;
    case MusicLocation:
        // NOT SUPPORTED
        break;
    case MoviesLocation:
        // NOT SUPPORTED
        break;
    case PicturesLocation:
        // NOT SUPPORTED
        break;
    case DownloadLocation:
        // NOTE: return empty string if the dir isn't available, even in the doc says otherwise
        return userDownloadDirFetcher.optValue().value_or(QLatin1String());
    case PublicShareLocation:
        // NOT SUPPORTED
        break;
    case TemplatesLocation:
        // NOT SUPPORTED
        break;
    case TempLocation:
        return QDir::tempPath();
    case HomeLocation:
        return QDir::homePath();
    case GenericDataLocation:
    case AppDataLocation:
    case StateLocation:
    case GenericStateLocation:
        return QOhosAppContext::getProperty(QOhosAppContext::Type::filesDir) + testDirSuffix;
    case CacheLocation:
    case GenericCacheLocation:
        return QOhosAppContext::getProperty(QOhosAppContext::Type::cacheDir) + testDirSuffix;
    case RuntimeLocation:
    case ConfigLocation:
    case GenericConfigLocation:
    case AppConfigLocation:
    case AppLocalDataLocation:
        return QOhosAppContext::getProperty(QOhosAppContext::Type::preferencesDir) + testDirSuffix;
    default:
        break;
    }

    return QLatin1String();
}

QT_END_NAMESPACE

#endif // QT_NO_STANDARDPATHS
