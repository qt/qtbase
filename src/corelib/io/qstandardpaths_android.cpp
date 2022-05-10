// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qstandardpaths.h"

#ifndef QT_NO_STANDARDPATHS

#include <QtCore/qjniobject.h>
#include <QtCore/qmap.h>
#include <QtCore/qcoreapplication.h>
#include <QDir>

QT_BEGIN_NAMESPACE

using namespace QNativeInterface;
using namespace Qt::StringLiterals;

typedef QMap<QString, QString> AndroidDirCache;
Q_GLOBAL_STATIC(AndroidDirCache, androidDirCache)

static QString testDir()
{
    return QStandardPaths::isTestModeEnabled() ? "/qttest"_L1 : ""_L1;
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
 * Locations where applications can place persistent files it owns.
 * E.g., /storage/org.app/Music
 */
static QString getExternalFilesDir(const char *directoryField = nullptr)
{
    QString &path = (*androidDirCache)["APPNAME_%1"_L1.arg(QLatin1StringView(directoryField))];
    if (!path.isEmpty())
        return path;

    QJniObject appCtx = QAndroidApplication::context();
    if (!appCtx.isValid())
        return QString();

    QJniObject dirField = QJniObject::fromString(""_L1);
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

QString QStandardPaths::writableLocation(StandardLocation type)
{
    switch (type) {
    case QStandardPaths::MusicLocation:
        return getExternalFilesDir("DIRECTORY_MUSIC");
    case QStandardPaths::MoviesLocation:
        return getExternalFilesDir("DIRECTORY_MOVIES");
    case QStandardPaths::PicturesLocation:
        return getExternalFilesDir("DIRECTORY_PICTURES");
    case QStandardPaths::DocumentsLocation:
        return getExternalFilesDir("DIRECTORY_DOCUMENTS");
    case QStandardPaths::DownloadLocation:
        return getExternalFilesDir("DIRECTORY_DOWNLOADS");
    case QStandardPaths::GenericConfigLocation:
    case QStandardPaths::ConfigLocation:
    case QStandardPaths::AppConfigLocation:
        return getFilesDir() + testDir() + "/settings"_L1;
    case QStandardPaths::GenericDataLocation:
        return getExternalFilesDir() + testDir();
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
    case QStandardPaths::PublicShareLocation:
    case QStandardPaths::TemplatesLocation:
    default:
        break;
    }

    return QString();
}

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    if (type == MusicLocation) {
        return QStringList() << writableLocation(type)
                             << getExternalFilesDir("DIRECTORY_MUSIC")
                             << getExternalFilesDir("DIRECTORY_PODCASTS")
                             << getExternalFilesDir("DIRECTORY_NOTIFICATIONS")
                             << getExternalFilesDir("DIRECTORY_ALARMS");
    }

    if (type == MoviesLocation) {
        return QStringList() << writableLocation(type)
                             << getExternalFilesDir("DIRECTORY_MOVIES");
    }

    if (type == PicturesLocation) {
        return QStringList()  << writableLocation(type)
                              << getExternalFilesDir("DIRECTORY_PICTURES");
    }

    if (type == DocumentsLocation) {
        return QStringList() << writableLocation(type)
                             << getExternalFilesDir("DIRECTORY_DOCUMENTS");
    }

    if (type == DownloadLocation) {
        return QStringList() << writableLocation(type)
                             << getExternalFilesDir("DIRECTORY_DOWNLOADS");
    }

    if (type == AppDataLocation || type == AppLocalDataLocation) {
        return QStringList() << writableLocation(type)
                             << getExternalFilesDir();
    }

    if (type == CacheLocation) {
        return QStringList() << writableLocation(type)
                             << getExternalCacheDir();
    }

    if (type == FontsLocation) {
        QString &fontLocation = (*androidDirCache)[QStringLiteral("FONT_LOCATION")];
        if (!fontLocation.isEmpty())
            return QStringList(fontLocation);

        const QByteArray ba = qgetenv("QT_ANDROID_FONT_LOCATION");
        if (!ba.isEmpty())
            return QStringList((fontLocation = QDir::cleanPath(QString::fromLocal8Bit(ba))));

        // Don't cache the fallback, as we might just have been called before
        // QT_ANDROID_FONT_LOCATION has been set.
        return QStringList("/system/fonts"_L1);
    }

    return QStringList(writableLocation(type));
}

QT_END_NAMESPACE

#endif // QT_NO_STANDARDPATHS
