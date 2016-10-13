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

#ifndef QT_NO_STANDARDPATHS

#include <qdir.h>
#include <qurl.h>
#include <private/qcore_mac_p.h>

#ifndef QT_BOOTSTRAPPED
#include <qcoreapplication.h>
#endif

#import <Foundation/Foundation.h>

QT_BEGIN_NAMESPACE

static QString pathForDirectory(NSSearchPathDirectory directory,
                                NSSearchPathDomainMask mask)
{
    return QString::fromNSString(
        [NSSearchPathForDirectoriesInDomains(directory, mask, YES) lastObject]);
}

static NSSearchPathDirectory searchPathDirectory(QStandardPaths::StandardLocation type)
{
    switch (type) {
    case QStandardPaths::DesktopLocation:
        return NSDesktopDirectory;
    case QStandardPaths::DocumentsLocation:
        return NSDocumentDirectory;
    case QStandardPaths::ApplicationsLocation:
        return NSApplicationDirectory;
    case QStandardPaths::MusicLocation:
        return NSMusicDirectory;
    case QStandardPaths::MoviesLocation:
        return NSMoviesDirectory;
    case QStandardPaths::PicturesLocation:
        return NSPicturesDirectory;
    case QStandardPaths::GenericDataLocation:
    case QStandardPaths::RuntimeLocation:
    case QStandardPaths::AppDataLocation:
    case QStandardPaths::AppLocalDataLocation:
        return NSApplicationSupportDirectory;
    case QStandardPaths::GenericCacheLocation:
    case QStandardPaths::CacheLocation:
        return NSCachesDirectory;
    case QStandardPaths::DownloadLocation:
        return NSDownloadsDirectory;
    default:
        return (NSSearchPathDirectory)0;
    }
}

static void appendOrganizationAndApp(QString &path)
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

static QString baseWritableLocation(QStandardPaths::StandardLocation type,
                                    NSSearchPathDomainMask mask = NSUserDomainMask,
                                    bool appendOrgAndApp = false)
{
    QString path;
    const NSSearchPathDirectory dir = searchPathDirectory(type);
    switch (type) {
    case QStandardPaths::HomeLocation:
        path = QDir::homePath();
        break;
    case QStandardPaths::TempLocation:
        path = QDir::tempPath();
        break;
#if defined(QT_PLATFORM_UIKIT)
    // These locations point to non-existing write-protected paths. Use sensible fallbacks.
    case QStandardPaths::MusicLocation:
        path = pathForDirectory(NSDocumentDirectory, mask) + QLatin1String("/Music");
        break;
    case QStandardPaths::MoviesLocation:
        path = pathForDirectory(NSDocumentDirectory, mask) + QLatin1String("/Movies");
        break;
    case QStandardPaths::PicturesLocation:
        path = pathForDirectory(NSDocumentDirectory, mask) + QLatin1String("/Pictures");
        break;
    case QStandardPaths::DownloadLocation:
        path = pathForDirectory(NSDocumentDirectory, mask) + QLatin1String("/Downloads");
        break;
    case QStandardPaths::DesktopLocation:
        path = pathForDirectory(NSDocumentDirectory, mask) + QLatin1String("/Desktop");
        break;
    case QStandardPaths::ApplicationsLocation:
        break;
#endif
    case QStandardPaths::FontsLocation:
        path = pathForDirectory(NSLibraryDirectory, mask) + QLatin1String("/Fonts");
        break;
    case QStandardPaths::ConfigLocation:
    case QStandardPaths::GenericConfigLocation:
    case QStandardPaths::AppConfigLocation:
        path = pathForDirectory(NSLibraryDirectory, mask) + QLatin1String("/Preferences");
        break;
    default:
        path = pathForDirectory(dir, mask);
        break;
    }

    if (appendOrgAndApp) {
        switch (type) {
        case QStandardPaths::AppDataLocation:
        case QStandardPaths::AppLocalDataLocation:
        case QStandardPaths::AppConfigLocation:
        case QStandardPaths::CacheLocation:
            appendOrganizationAndApp(path);
            break;
        default:
            break;
        }
    }

    return path;
}

QString QStandardPaths::writableLocation(StandardLocation type)
{
    QString location = baseWritableLocation(type, NSUserDomainMask, true);
    if (isTestModeEnabled())
        location = location.replace(QDir::homePath(), QDir::homePath() + QLatin1String("/.qttest"));

    return location;
}

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    QStringList dirs;

#if defined(QT_PLATFORM_UIKIT)
    if (type == PicturesLocation)
        dirs << writableLocation(PicturesLocation) << QLatin1String("assets-library://");
#endif

    if (type == GenericDataLocation || type == FontsLocation || type == ApplicationsLocation
            || type == AppDataLocation || type == AppLocalDataLocation
            || type == GenericCacheLocation || type == CacheLocation) {
        QList<NSSearchPathDomainMask> masks;
        masks << NSLocalDomainMask;
        if (type == FontsLocation || type == GenericCacheLocation)
            masks << NSSystemDomainMask;

        for (QList<NSSearchPathDomainMask>::const_iterator it = masks.begin();
             it != masks.end(); ++it) {
            const QString path = baseWritableLocation(type, *it, true);
            if (!path.isEmpty() && !dirs.contains(path))
                dirs.append(path);
        }
    }

    if (type == AppDataLocation || type == AppLocalDataLocation) {
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        if (mainBundle) {
            CFURLRef bundleUrl = CFBundleCopyBundleURL(mainBundle);
            CFStringRef cfBundlePath = CFURLCopyPath(bundleUrl);
            QString bundlePath = QString::fromCFString(cfBundlePath);
            CFRelease(cfBundlePath);
            CFRelease(bundleUrl);

            CFURLRef resourcesUrl = CFBundleCopyResourcesDirectoryURL(mainBundle);
            CFStringRef cfResourcesPath = CFURLCopyPath(resourcesUrl);
            QString resourcesPath = QString::fromCFString(cfResourcesPath);
            CFRelease(cfResourcesPath);
            CFRelease(resourcesUrl);

            // Handle bundled vs unbundled executables. CFBundleGetMainBundle() returns
            // a valid bundle in both cases. CFBundleCopyResourcesDirectoryURL() returns
            // an absolute path for unbundled executables.
            if (resourcesPath.startsWith(QLatin1Char('/')))
                dirs.append(resourcesPath);
            else
                dirs.append(bundlePath + resourcesPath);
        }
    }
    const QString localDir = writableLocation(type);
    if (!localDir.isEmpty())
        dirs.prepend(localDir);
    return dirs;
}

#ifndef QT_BOOTSTRAPPED
QString QStandardPaths::displayName(StandardLocation type)
{
    // Use "Home" instead of the user's Unix username
    if (QStandardPaths::HomeLocation == type)
        return QCoreApplication::translate("QStandardPaths", "Home");

    // The temporary directory returned by the old Carbon APIs is ~/Library/Caches/TemporaryItems,
    // the display name of which ("TemporaryItems") isn't translated by the system. The standard
    // temporary directory has no reasonable display name either, so use something more sensible.
    if (QStandardPaths::TempLocation == type)
        return QCoreApplication::translate("QStandardPaths", "Temporary Items");

    // standardLocations() may return an empty list on some platforms
    if (QStandardPaths::ApplicationsLocation == type)
        return QCoreApplication::translate("QStandardPaths", "Applications");

    if (QCFType<CFURLRef> url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
            standardLocations(type).constFirst().toCFString(),
            kCFURLPOSIXPathStyle, true)) {
        QCFString name;
        CFURLCopyResourcePropertyForKey(url, kCFURLLocalizedNameKey, &name, NULL);
        if (name && CFStringGetLength(name))
            return QString::fromCFString(name);
    }

    return QFileInfo(baseWritableLocation(type)).fileName();
}
#endif

QT_END_NAMESPACE

#endif // QT_NO_STANDARDPATHS
