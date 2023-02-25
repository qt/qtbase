// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qstandardpaths.h"

#ifndef QT_NO_STANDARDPATHS

#include <QtCore/qjniobject.h>
#include <QtCore/qmap.h>
#include <QtCore/qcoreapplication.h>
#include <QDir>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(Environment, "android/os/Environment");
Q_DECLARE_JNI_TYPE(File, "Ljava/io/File;");

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
    QJniObject path = file.callMethod<jstring>("getAbsolutePath");

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

    QJniObject file = QJniObject::callStaticMethod<QtJniTypes::File>("android/os/Environment",
                                                                     "getExternalStorageDirectory");
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

    QJniObject dirField = QJniObject::getStaticField<jstring>("android/os/Environment",
                                                              directoryField);
    if (!dirField.isValid())
        return QString();

    QJniObject file = QJniObject::callStaticMethod<QtJniTypes::File>("android/os/Environment",
                                                            "getExternalStoragePublicDirectory",
                                                            dirField.object<jstring>());
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
    QString &path = (*androidDirCache)["APPNAME_%1"_L1.arg(QLatin1StringView(directoryField))];
    if (!path.isEmpty())
        return path;

    QJniObject appCtx = QAndroidApplication::context();
    if (!appCtx.isValid())
        return QString();

    QJniObject dirField = QJniObject::fromString(""_L1);
    if (directoryField && strlen(directoryField) > 0) {
        dirField = QJniObject::getStaticField<QtJniTypes::Environment, jstring>(directoryField);
        if (!dirField.isValid())
            return QString();
    }

    QJniObject file = appCtx.callMethod<QtJniTypes::File>("getExternalFilesDir",
                                                          dirField.object<jstring>());

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

    QJniObject file = appCtx.callMethod<QtJniTypes::File>("getExternalCacheDir");

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

    QJniObject file = appCtx.callMethod<QtJniTypes::File>("getCacheDir");
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

    QJniObject file = appCtx.callMethod<QtJniTypes::File>("getFilesDir");
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
        return getFilesDir() + testDir() + "/settings"_L1;
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
    case QStandardPaths::PublicShareLocation:
    case QStandardPaths::TemplatesLocation:
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
                locations << "/system/fonts"_L1;
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
