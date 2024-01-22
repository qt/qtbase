/****************************************************************************
**
** Copyright (C) 2023 The Qt Company Ltd.
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

#include <QtCore/qjniobject.h>
#include <QtCore/qmap.h>
#include <QtCore/qcoreapplication.h>
#include <QDir>

QT_BEGIN_NAMESPACE

using namespace QNativeInterface;

typedef QMap<QString, QString> AndroidDirCache;
Q_GLOBAL_STATIC(AndroidDirCache, androidDirCache)

static QString testDir()
{
    return QStandardPaths::isTestModeEnabled() ? QLatin1String("/qttest")
                                               : QLatin1String("");
}

static inline QString getAbsolutePath(const QJniObject &file)
{
    QJniObject path = file.callObjectMethod("getAbsolutePath",
                                            "()Ljava/lang/String;");
    if (!path.isValid())
        return QString();

    return path.toString();
}

/*
 * The root of the external storage
 *
 */
static QString getExternalStorageDirectory()
{
    QString &path = (*androidDirCache)[QStringLiteral("EXT_ROOT")];
    if (!path.isEmpty())
        return path;

    QJniObject file = QJniObject::callStaticObjectMethod("android/os/Environment",
                                                         "getExternalStorageDirectory",
                                                         "()Ljava/io/File;");
    if (!file.isValid())
        return QString();

    return (path = getAbsolutePath(file));
}

/*
 * Locations where applications can place user files shared by all apps (public).
 * E.g., /storage/Music
 */
static QString getExternalStoragePublicDirectory(const char *directoryField)
{
    QString &path = (*androidDirCache)[QLatin1String(directoryField)];
    if (!path.isEmpty())
        return path;

    QJniObject dirField = QJniObject::getStaticObjectField("android/os/Environment",
                                                           directoryField,
                                                           "Ljava/lang/String;");
    if (!dirField.isValid())
        return QString();

    QJniObject file = QJniObject::callStaticObjectMethod("android/os/Environment",
                                                         "getExternalStoragePublicDirectory",
                                                         "(Ljava/lang/String;)Ljava/io/File;",
                                                         dirField.object());
    if (!file.isValid())
        return QString();

    return (path = getAbsolutePath(file));
}

/*
 * Locations where applications can place persistent files it owns.
 * E.g., /storage/org.app/Music
 */
static QString getExternalFilesDir(const char *directoryField = nullptr)
{
    QString &path = (*androidDirCache)[QLatin1String("APPNAME_%1").arg(QLatin1String(directoryField))];
    if (!path.isEmpty())
        return path;

    QJniObject appCtx = QAndroidApplication::context();
    if (!appCtx.isValid())
        return QString();

    QJniObject dirField = QJniObject::fromString(QLatin1String(""));
    if (directoryField && strlen(directoryField) > 0) {
        dirField = QJniObject::getStaticObjectField("android/os/Environment",
                                                    directoryField,
                                                    "Ljava/lang/String;");
        if (!dirField.isValid())
            return QString();
    }

    QJniObject file = appCtx.callObjectMethod("getExternalFilesDir",
                                              "(Ljava/lang/String;)Ljava/io/File;",
                                              dirField.object());

    if (!file.isValid())
        return QString();

    return (path = getAbsolutePath(file));
}

/*
 * Directory where applications can store cache files it owns (public).
 * E.g., /storage/org.app/
 */
static QString getExternalCacheDir()
{
    QString &path = (*androidDirCache)[QStringLiteral("APPNAME_CACHE")];
    if (!path.isEmpty())
        return path;

    QJniObject appCtx = QAndroidApplication::context();
    if (!appCtx.isValid())
        return QString();

    QJniObject file = appCtx.callObjectMethod("getExternalCacheDir",
                                              "()Ljava/io/File;");

    if (!file.isValid())
        return QString();

    return (path = getAbsolutePath(file));
}

/*
 * Directory where applications can store cache files it owns (private).
 */
static QString getCacheDir()
{
    QString &path = (*androidDirCache)[QStringLiteral("APPROOT_CACHE")];
    if (!path.isEmpty())
        return path;

    QJniObject appCtx = QAndroidApplication::context();
    if (!appCtx.isValid())
        return QString();

    QJniObject file = appCtx.callObjectMethod("getCacheDir",
                                              "()Ljava/io/File;");
    if (!file.isValid())
        return QString();

    return (path = getAbsolutePath(file));
}

/*
 * Directory where applications can store files it owns (private).
 * (Same location as $HOME)
 */
static QString getFilesDir()
{
    QString &path = (*androidDirCache)[QStringLiteral("APPROOT_FILES")];
    if (!path.isEmpty())
        return path;

    QJniObject appCtx = QAndroidApplication::context();
    if (!appCtx.isValid())
        return QString();

    QJniObject file = appCtx.callObjectMethod("getFilesDir",
                                              "()Ljava/io/File;");
    if (!file.isValid())
        return QString();

    return (path = getAbsolutePath(file));
}

static QString getSdkBasedExternalDir(const char *directoryField = nullptr)
{
    return (QNativeInterface::QAndroidApplication::sdkVersion() >= 30)
            ? getExternalFilesDir(directoryField)
            : getExternalStoragePublicDirectory(directoryField);
}

QString QStandardPaths::writableLocation(StandardLocation type)
{
    switch (type) {
    case QStandardPaths::MusicLocation:
        return getSdkBasedExternalDir("DIRECTORY_MUSIC");
    case QStandardPaths::MoviesLocation:
        return getSdkBasedExternalDir("DIRECTORY_MOVIES");
    case QStandardPaths::PicturesLocation:
        return getSdkBasedExternalDir("DIRECTORY_PICTURES");
    case QStandardPaths::DocumentsLocation:
        return getSdkBasedExternalDir("DIRECTORY_DOCUMENTS");
    case QStandardPaths::DownloadLocation:
        return getSdkBasedExternalDir("DIRECTORY_DOWNLOADS");
    case QStandardPaths::GenericConfigLocation:
    case QStandardPaths::ConfigLocation:
    case QStandardPaths::AppConfigLocation:
        return getFilesDir() + testDir() + QLatin1String("/settings");
    case QStandardPaths::GenericDataLocation:
    {
        return QAndroidApplication::sdkVersion() >= 30 ?
                getExternalFilesDir() + testDir() : getExternalStorageDirectory() + testDir();
    }
    case QStandardPaths::AppDataLocation:
    case QStandardPaths::AppLocalDataLocation:
        return getFilesDir() + testDir();
    case QStandardPaths::GenericCacheLocation:
    case QStandardPaths::RuntimeLocation:
    case QStandardPaths::TempLocation:
    case QStandardPaths::CacheLocation:
        return getCacheDir() + testDir();
    case QStandardPaths::DesktopLocation:
    case QStandardPaths::HomeLocation:
        return getFilesDir();
    case QStandardPaths::ApplicationsLocation:
    case QStandardPaths::FontsLocation:
    default:
        break;
    }

    return QString();
}

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    QStringList locations;

    if (type == MusicLocation) {
        locations << getExternalFilesDir("DIRECTORY_MUSIC");
        // Place the public dirs before the app own dirs
        if (QNativeInterface::QAndroidApplication::sdkVersion() < 30) {
            locations << getExternalStoragePublicDirectory("DIRECTORY_PODCASTS")
                      << getExternalStoragePublicDirectory("DIRECTORY_NOTIFICATIONS")
                      << getExternalStoragePublicDirectory("DIRECTORY_ALARMS");
        }
        locations << getExternalFilesDir("DIRECTORY_PODCASTS")
                  << getExternalFilesDir("DIRECTORY_NOTIFICATIONS")
                  << getExternalFilesDir("DIRECTORY_ALARMS");
    } else if (type == MoviesLocation) {
        locations << getExternalFilesDir("DIRECTORY_MOVIES");
    } else if (type == PicturesLocation) {
        locations << getExternalFilesDir("DIRECTORY_PICTURES");
    } else if (type == DocumentsLocation) {
        locations << getExternalFilesDir("DIRECTORY_DOCUMENTS");
    } else if (type == DownloadLocation) {
        locations << getExternalFilesDir("DIRECTORY_DOWNLOADS");
    } else if (type == AppDataLocation || type == AppLocalDataLocation) {
        locations << getExternalFilesDir();
    } else if (type == CacheLocation) {
        locations << getExternalCacheDir();
    } else if (type == FontsLocation) {
        QString &fontLocation = (*androidDirCache)[QStringLiteral("FONT_LOCATION")];
        if (!fontLocation.isEmpty()) {
            locations << fontLocation;
        } else {
            const QByteArray ba = qgetenv("QT_ANDROID_FONT_LOCATION");
            if (!ba.isEmpty()) {
                locations << (fontLocation = QDir::cleanPath(QString::fromLocal8Bit(ba)));
            } else {
                // Don't cache the fallback, as we might just have been called before
                // QT_ANDROID_FONT_LOCATION has been set.
                locations << QLatin1String("/system/fonts");
            }
        }
    }

    const QString writable = writableLocation(type);
    if (!writable.isEmpty())
        locations.prepend(writable);

    locations.removeDuplicates();
    return locations;
}

QT_END_NAMESPACE

#endif // QT_NO_STANDARDPATHS
