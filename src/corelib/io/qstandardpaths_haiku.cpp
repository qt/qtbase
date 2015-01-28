/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
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
        path += QLatin1Char('/') + org;
    const QString appName = QCoreApplication::applicationName();
    if (!appName.isEmpty())
        path += QLatin1Char('/') + appName;
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
