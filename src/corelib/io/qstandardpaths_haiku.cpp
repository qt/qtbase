// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qstandardpaths.h"

#ifndef QT_NO_STANDARDPATHS

#ifndef QT_BOOTSTRAPPED
#include <qcoreapplication.h>
#endif

#include <qfile.h>

#include <FindDirectory.h>
#include <Path.h>
#include <PathFinder.h>
#include <StringList.h>

QT_BEGIN_NAMESPACE

namespace {

void appendOrganizationAndApp(QString &path)
{
#ifndef QT_BOOTSTRAPPED
    const QString org = QCoreApplication::organizationName();
    if (!org.isEmpty())
        path += u'/' + org;
    const QString appName = QCoreApplication::applicationName();
    if (!appName.isEmpty())
        path += u'/' + appName;
#else
    Q_UNUSED(path);
#endif
}

/*
 * Returns the generic standard path for given directory type.
 */
QString haikuStandardPath(directory_which which)
{
    BPath standardPath;

    if (find_directory(which, &standardPath, false) != B_OK)
        return QString();

    return QFile::decodeName(standardPath.Path());
}

/*
 * Returns the generic standard paths for given path type.
 */
QStringList haikuStandardPaths(path_base_directory baseDirectory)
{
    BStringList paths;

    if (BPathFinder::FindPaths(baseDirectory, paths) != B_OK)
        return QStringList();

    QStringList standardPaths;
    for (int i = 0; i < paths.CountStrings(); ++i) {
        standardPaths << QFile::decodeName(paths.StringAt(i).String());
    }

    return standardPaths;
}

/*
 * Returns the application specific standard path for given directory type.
 */
QString haikuAppStandardPath(directory_which which)
{
    QString path = haikuStandardPath(which);
    if (!path.isEmpty())
        appendOrganizationAndApp(path);

    return path;
}

/*
 * Returns the application specific standard paths for given path type.
 */
QStringList haikuAppStandardPaths(path_base_directory baseDirectory)
{
    QStringList paths = haikuStandardPaths(baseDirectory);
    for (int i = 0; i < paths.count(); ++i)
        appendOrganizationAndApp(paths[i]);

    return paths;
}

} // namespace

QString QStandardPaths::writableLocation(StandardLocation type)
{
    switch (type) {
    case DesktopLocation:
        return haikuStandardPath(B_DESKTOP_DIRECTORY);
    case DocumentsLocation: // fall through
    case PicturesLocation:
    case MusicLocation:
    case MoviesLocation:
    case DownloadLocation:
    case PublicShareLocation:
    case TemplatesLocation:
    case HomeLocation:
        return haikuStandardPath(B_USER_DIRECTORY);
    case FontsLocation:
        return haikuStandardPath(B_USER_NONPACKAGED_FONTS_DIRECTORY);
    case ApplicationsLocation:
        return haikuStandardPath(B_USER_NONPACKAGED_BIN_DIRECTORY);
    case TempLocation:
        return haikuStandardPath(B_SYSTEM_TEMP_DIRECTORY);
    case AppDataLocation: // fall through
    case AppLocalDataLocation:
        return haikuAppStandardPath(B_USER_NONPACKAGED_DATA_DIRECTORY);
    case GenericDataLocation:
        return haikuStandardPath(B_USER_NONPACKAGED_DATA_DIRECTORY);
    case CacheLocation:
        return haikuAppStandardPath(B_USER_CACHE_DIRECTORY);
    case GenericCacheLocation:
        return haikuStandardPath(B_USER_CACHE_DIRECTORY);
    case ConfigLocation: // fall through
    case AppConfigLocation:
        return haikuAppStandardPath(B_USER_SETTINGS_DIRECTORY);
    case GenericConfigLocation:
        return haikuStandardPath(B_USER_SETTINGS_DIRECTORY);
    default:
        return QString();
    }
}

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    QStringList paths;

    const QString writablePath = writableLocation(type);
    if (!writablePath.isEmpty())
        paths += writablePath;

    switch (type) {
    case DocumentsLocation: // fall through
    case PicturesLocation:
    case MusicLocation:
    case MoviesLocation:
    case DownloadLocation:
    case PublicShareLocation:
    case TemplatesLocation:
    case HomeLocation:
        paths += haikuStandardPath(B_USER_NONPACKAGED_DIRECTORY);
        break;
    case FontsLocation:
        paths += haikuStandardPaths(B_FIND_PATH_FONTS_DIRECTORY);
        break;
    case ApplicationsLocation:
        paths += haikuStandardPaths(B_FIND_PATH_BIN_DIRECTORY);
        paths += haikuStandardPaths(B_FIND_PATH_APPS_DIRECTORY);
        break;
    case AppDataLocation: // fall through
    case AppLocalDataLocation:
        paths += haikuAppStandardPaths(B_FIND_PATH_DATA_DIRECTORY);
        break;
    case GenericDataLocation:
        paths += haikuStandardPaths(B_FIND_PATH_DATA_DIRECTORY);
        break;
    case CacheLocation:
        paths += haikuAppStandardPath(B_SYSTEM_CACHE_DIRECTORY);
        break;
    case GenericCacheLocation:
        paths += haikuStandardPath(B_SYSTEM_CACHE_DIRECTORY);
        break;
    case ConfigLocation: // fall through
    case AppConfigLocation:
        paths += haikuAppStandardPath(B_SYSTEM_SETTINGS_DIRECTORY);
        break;
    case GenericConfigLocation:
        paths += haikuStandardPath(B_SYSTEM_SETTINGS_DIRECTORY);
        break;
    default:
        break;
    }

    return paths;
}

QT_END_NAMESPACE

#endif // QT_NO_STANDARDPATHS
