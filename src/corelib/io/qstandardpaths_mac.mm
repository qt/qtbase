// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

using namespace Qt::StringLiterals;

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
    case QStandardPaths::PublicShareLocation:
        return NSSharedPublicDirectory;
    case QStandardPaths::TemplatesLocation:
    default:
        return (NSSearchPathDirectory)0;
    }
}

static void appendOrganizationAndApp(QString &path)
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
        path = pathForDirectory(NSDocumentDirectory, mask) + "/Music"_L1;
        break;
    case QStandardPaths::MoviesLocation:
        path = pathForDirectory(NSDocumentDirectory, mask) + "/Movies"_L1;
        break;
    case QStandardPaths::PicturesLocation:
        path = pathForDirectory(NSDocumentDirectory, mask) + "/Pictures"_L1;
        break;
    case QStandardPaths::DownloadLocation:
        path = pathForDirectory(NSDocumentDirectory, mask) + "/Downloads"_L1;
        break;
    case QStandardPaths::DesktopLocation:
        path = pathForDirectory(NSDocumentDirectory, mask) + "/Desktop"_L1;
        break;
    case QStandardPaths::ApplicationsLocation:
        break;
    case QStandardPaths::PublicShareLocation:
        path = pathForDirectory(NSDocumentDirectory, mask) + "/Public"_L1;
        break;
    case QStandardPaths::TemplatesLocation:
        path = pathForDirectory(NSDocumentDirectory, mask) + "/Templates"_L1;
        break;
#endif
    case QStandardPaths::FontsLocation:
        path = pathForDirectory(NSLibraryDirectory, mask) + "/Fonts"_L1;
        break;
    case QStandardPaths::ConfigLocation:
    case QStandardPaths::GenericConfigLocation:
    case QStandardPaths::AppConfigLocation:
        path = pathForDirectory(NSLibraryDirectory, mask) + "/Preferences"_L1;
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
        location = location.replace(QDir::homePath(), QDir::homePath() + "/.qttest"_L1);

    return location;
}

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    QStringList dirs;

#if defined(QT_PLATFORM_UIKIT)
    if (type == PicturesLocation)
        dirs << writableLocation(PicturesLocation) << "assets-library://"_L1;
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
            if (QCFType<CFURLRef> resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle)) {
                if (QCFType<CFURLRef> absoluteResouresURL = CFURLCopyAbsoluteURL(resourcesURL)) {
                    if (QCFType<CFStringRef> path = CFURLCopyFileSystemPath(absoluteResouresURL,
                                                                            kCFURLPOSIXPathStyle)) {
                        dirs.append(QString::fromCFString(path));
                    }
                }
            }
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
    if (QStandardPaths::TempLocation == type) {
        //: macOS: Temporary directory
        return QCoreApplication::translate("QStandardPaths", "Temporary Items");
    }

    // standardLocations() may return an empty list on some platforms
    if (QStandardPaths::ApplicationsLocation == type)
        return QCoreApplication::translate("QStandardPaths", "Applications");

    const QCFString fsPath(standardLocations(type).constFirst());
    if (QCFType<CFURLRef> url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
            fsPath, kCFURLPOSIXPathStyle, true)) {
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
