/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2020 Intel Corporation.
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
#include <qfile.h>
#include <qhash.h>
#include <qtextstream.h>
#if QT_CONFIG(regularexpression)
#include <qregularexpression.h>
#endif
#include <private/qfilesystemengine_p.h>
#include <errno.h>
#include <stdlib.h>

#ifndef QT_BOOTSTRAPPED
#include <qcoreapplication.h>
#endif

#ifndef QT_NO_STANDARDPATHS

QT_BEGIN_NAMESPACE

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

#if QT_CONFIG(regularexpression)
static QLatin1String xdg_key_name(QStandardPaths::StandardLocation type)
{
    switch (type) {
    case QStandardPaths::DesktopLocation:
        return QLatin1String("DESKTOP");
    case QStandardPaths::DocumentsLocation:
        return QLatin1String("DOCUMENTS");
    case QStandardPaths::PicturesLocation:
        return QLatin1String("PICTURES");
    case QStandardPaths::MusicLocation:
        return QLatin1String("MUSIC");
    case QStandardPaths::MoviesLocation:
        return QLatin1String("VIDEOS");
    case QStandardPaths::DownloadLocation:
        return QLatin1String("DOWNLOAD");
    default:
        return QLatin1String();
    }
}
#endif

static bool checkXdgRuntimeDir(const QString &xdgRuntimeDir)
{
    auto describeMetaData = [](const QFileSystemMetaData &metaData) -> QByteArray {
        if (!metaData.exists())
            return "a broken symlink";

        QByteArray description;
        if (metaData.isLink())
            description = "a symbolic link to ";

        if (metaData.isFile())
            description += "a regular file";
        else if (metaData.isDirectory())
            description += "a directory";
        else if (metaData.isSequential())
            description += "a character device, socket or FIFO";
        else
            description += "a block device";

        // convert QFileSystemMetaData permissions back to Unix
        mode_t perms = 0;
        if (metaData.permissions() & QFile::ReadOwner)
            perms |= S_IRUSR;
        if (metaData.permissions() & QFile::WriteOwner)
            perms |= S_IWUSR;
        if (metaData.permissions() & QFile::ExeOwner)
            perms |= S_IXUSR;
        if (metaData.permissions() & QFile::ReadGroup)
            perms |= S_IRGRP;
        if (metaData.permissions() & QFile::WriteGroup)
            perms |= S_IWGRP;
        if (metaData.permissions() & QFile::ExeGroup)
            perms |= S_IXGRP;
        if (metaData.permissions() & QFile::ReadOther)
            perms |= S_IROTH;
        if (metaData.permissions() & QFile::WriteOther)
            perms |= S_IWOTH;
        if (metaData.permissions() & QFile::ExeOther)
            perms |= S_IXOTH;
        description += " permissions 0" + QByteArray::number(perms, 8);

        return description
                + " owned by UID " + QByteArray::number(metaData.userId())
                + " GID " + QByteArray::number(metaData.groupId());
    };

    // http://standards.freedesktop.org/basedir-spec/latest/
    const uint myUid = uint(geteuid());
    const QFile::Permissions wantedPerms = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner;
    const QFileSystemMetaData::MetaDataFlags statFlags = QFileSystemMetaData::PosixStatFlags
                                                         | QFileSystemMetaData::LinkType;
    QFileSystemMetaData metaData;
    QFileSystemEntry entry(xdgRuntimeDir);

    // Check that the xdgRuntimeDir is a directory by attempting to create it.
    // A stat() before mkdir() that concluded it doesn't exist is a meaningless
    // result: we'd race against someone else attempting to create it.
    // ### QFileSystemEngine::createDirectory cannot take the extra mode argument.
    if (QT_MKDIR(entry.nativeFilePath(), 0700) == 0)
        return true;
    if (errno != EEXIST) {
        qErrnoWarning("QStandardPaths: error creating runtime directory '%ls'",
                      qUtf16Printable(xdgRuntimeDir));
        return false;
    }

    // We use LinkType to force an lstat(), but fillMetaData() still returns error
    // on broken symlinks.
    if (!QFileSystemEngine::fillMetaData(entry, metaData, statFlags) && !metaData.isLink()) {
        qErrnoWarning("QStandardPaths: error obtaining permissions of runtime directory '%ls'",
                      qUtf16Printable(xdgRuntimeDir));
        return false;
    }

    // Checks:
    // - is a directory
    // - is not a symlink (even is pointing to a directory)
    if (metaData.isLink() || !metaData.isDirectory()) {
        qWarning("QStandardPaths: runtime directory '%ls' is not a directory, but %s",
                 qUtf16Printable(xdgRuntimeDir), describeMetaData(metaData).constData());
        return false;
    }

    // - "The directory MUST be owned by the user"
    if (metaData.userId() != myUid) {
        qWarning("QStandardPaths: runtime directory '%ls' is not owned by UID %d, but %s",
                 qUtf16Printable(xdgRuntimeDir), myUid, describeMetaData(metaData).constData());
        return false;
    }

    // "and he MUST be the only one having read and write access to it. Its Unix access mode MUST be 0700."
    if (metaData.permissions() != wantedPerms) {
        // attempt to correct:
        QSystemError error;
        if (!QFileSystemEngine::setPermissions(entry, wantedPerms, error)) {
            qErrnoWarning("QStandardPaths: could not set correct permissions on runtime directory "
                          "'%ls', which is %s", qUtf16Printable(xdgRuntimeDir),
                          describeMetaData(metaData).constData());
            return false;
        }
    }

    return true;
}

QString QStandardPaths::writableLocation(StandardLocation type)
{
    switch (type) {
    case HomeLocation:
        return QDir::homePath();
    case TempLocation:
        return QDir::tempPath();
    case CacheLocation:
    case GenericCacheLocation:
    {
        // http://standards.freedesktop.org/basedir-spec/basedir-spec-0.6.html
        QString xdgCacheHome = QFile::decodeName(qgetenv("XDG_CACHE_HOME"));
        if (isTestModeEnabled())
            xdgCacheHome = QDir::homePath() + QLatin1String("/.qttest/cache");
        if (xdgCacheHome.isEmpty())
            xdgCacheHome = QDir::homePath() + QLatin1String("/.cache");
        if (type == QStandardPaths::CacheLocation)
            appendOrganizationAndApp(xdgCacheHome);
        return xdgCacheHome;
    }
    case AppDataLocation:
    case AppLocalDataLocation:
    case GenericDataLocation:
    {
        QString xdgDataHome = QFile::decodeName(qgetenv("XDG_DATA_HOME"));
        if (isTestModeEnabled())
            xdgDataHome = QDir::homePath() + QLatin1String("/.qttest/share");
        if (xdgDataHome.isEmpty())
            xdgDataHome = QDir::homePath() + QLatin1String("/.local/share");
        if (type == AppDataLocation || type == AppLocalDataLocation)
            appendOrganizationAndApp(xdgDataHome);
        return xdgDataHome;
    }
    case ConfigLocation:
    case GenericConfigLocation:
    case AppConfigLocation:
    {
        // http://standards.freedesktop.org/basedir-spec/latest/
        QString xdgConfigHome = QFile::decodeName(qgetenv("XDG_CONFIG_HOME"));
        if (isTestModeEnabled())
            xdgConfigHome = QDir::homePath() + QLatin1String("/.qttest/config");
        if (xdgConfigHome.isEmpty())
            xdgConfigHome = QDir::homePath() + QLatin1String("/.config");
        if (type == AppConfigLocation)
            appendOrganizationAndApp(xdgConfigHome);
        return xdgConfigHome;
    }
    case RuntimeLocation:
    {
        QString xdgRuntimeDir = QFile::decodeName(qgetenv("XDG_RUNTIME_DIR"));
        bool fromEnv = !xdgRuntimeDir.isEmpty();
        if (xdgRuntimeDir.isEmpty() || !checkXdgRuntimeDir(xdgRuntimeDir)) {
            // environment variable not set or is set to something unsuitable
            const uint myUid = uint(geteuid());
            const QString userName = QFileSystemEngine::resolveUserName(myUid);
            xdgRuntimeDir = QDir::tempPath() + QLatin1String("/runtime-") + userName;

            if (!fromEnv) {
#ifndef Q_OS_WASM
                qWarning("QStandardPaths: XDG_RUNTIME_DIR not set, defaulting to '%ls'", qUtf16Printable(xdgRuntimeDir));
#endif
            }

            if (!checkXdgRuntimeDir(xdgRuntimeDir))
                xdgRuntimeDir.clear();
        }

        return xdgRuntimeDir;
    }
    default:
        break;
    }

#if QT_CONFIG(regularexpression)
    // http://www.freedesktop.org/wiki/Software/xdg-user-dirs
    QString xdgConfigHome = QFile::decodeName(qgetenv("XDG_CONFIG_HOME"));
    if (xdgConfigHome.isEmpty())
        xdgConfigHome = QDir::homePath() + QLatin1String("/.config");
    QFile file(xdgConfigHome + QLatin1String("/user-dirs.dirs"));
    const QLatin1String key = xdg_key_name(type);
    if (!key.isEmpty() && !isTestModeEnabled() && file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        // Only look for lines like: XDG_DESKTOP_DIR="$HOME/Desktop"
        QRegularExpression exp(QLatin1String("^XDG_(.*)_DIR=(.*)$"));
        QString result;
        while (!stream.atEnd()) {
            const QString &line = stream.readLine();
            QRegularExpressionMatch match = exp.match(line);
            if (match.hasMatch() && match.capturedView(1) == key) {
                QStringView value = match.capturedView(2);
                if (value.length() > 2
                    && value.startsWith(QLatin1Char('\"'))
                    && value.endsWith(QLatin1Char('\"')))
                    value = value.mid(1, value.length() - 2);
                // value can start with $HOME
                if (value.startsWith(QLatin1String("$HOME")))
                    result = QDir::homePath() + value.mid(5);
                else
                    result = value.toString();
                if (result.length() > 1 && result.endsWith(QLatin1Char('/')))
                    result.chop(1);
            }
        }
        if (!result.isNull())
            return result;
    }
#endif // QT_CONFIG(regularexpression)

    QString path;
    switch (type) {
    case DesktopLocation:
        path = QDir::homePath() + QLatin1String("/Desktop");
        break;
    case DocumentsLocation:
        path = QDir::homePath() + QLatin1String("/Documents");
       break;
    case PicturesLocation:
        path = QDir::homePath() + QLatin1String("/Pictures");
        break;

    case FontsLocation:
        path = writableLocation(GenericDataLocation) + QLatin1String("/fonts");
        break;

    case MusicLocation:
        path = QDir::homePath() + QLatin1String("/Music");
        break;

    case MoviesLocation:
        path = QDir::homePath() + QLatin1String("/Videos");
        break;
    case DownloadLocation:
        path = QDir::homePath() + QLatin1String("/Downloads");
        break;
    case ApplicationsLocation:
        path = writableLocation(GenericDataLocation) + QLatin1String("/applications");
        break;

    default:
        break;
    }

    return path;
}

static QStringList xdgDataDirs()
{
    QStringList dirs;
    // http://standards.freedesktop.org/basedir-spec/latest/
    QString xdgDataDirsEnv = QFile::decodeName(qgetenv("XDG_DATA_DIRS"));
    if (xdgDataDirsEnv.isEmpty()) {
        dirs.append(QString::fromLatin1("/usr/local/share"));
        dirs.append(QString::fromLatin1("/usr/share"));
    } else {
        const auto parts = xdgDataDirsEnv.splitRef(QLatin1Char(':'), Qt::SkipEmptyParts);

        // Normalize paths, skip relative paths
        for (const QStringRef &dir : parts) {
            if (dir.startsWith(QLatin1Char('/')))
                dirs.push_back(QDir::cleanPath(dir.toString()));
        }

        // Remove duplicates from the list, there's no use for duplicated
        // paths in XDG_DATA_DIRS - if it's not found in the given
        // directory the first time, it won't be there the second time.
        // Plus duplicate paths causes problems for example for mimetypes,
        // where duplicate paths here lead to duplicated mime types returned
        // for a file, eg "text/plain,text/plain" instead of "text/plain"
        dirs.removeDuplicates();
    }
    return dirs;
}

static QStringList xdgConfigDirs()
{
    QStringList dirs;
    // http://standards.freedesktop.org/basedir-spec/latest/
    const QString xdgConfigDirs = QFile::decodeName(qgetenv("XDG_CONFIG_DIRS"));
    if (xdgConfigDirs.isEmpty())
        dirs.append(QString::fromLatin1("/etc/xdg"));
    else
        dirs = xdgConfigDirs.split(QLatin1Char(':'));
    return dirs;
}

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    QStringList dirs;
    switch (type) {
    case ConfigLocation:
    case GenericConfigLocation:
        dirs = xdgConfigDirs();
        break;
    case AppConfigLocation:
        dirs = xdgConfigDirs();
        for (int i = 0; i < dirs.count(); ++i)
            appendOrganizationAndApp(dirs[i]);
        break;
    case GenericDataLocation:
        dirs = xdgDataDirs();
        break;
    case ApplicationsLocation:
        dirs = xdgDataDirs();
        for (int i = 0; i < dirs.count(); ++i)
            dirs[i].append(QLatin1String("/applications"));
        break;
    case AppDataLocation:
    case AppLocalDataLocation:
        dirs = xdgDataDirs();
        for (int i = 0; i < dirs.count(); ++i)
            appendOrganizationAndApp(dirs[i]);
        break;
    case FontsLocation:
        dirs += QDir::homePath() + QLatin1String("/.fonts");
        dirs += xdgDataDirs();
        for (int i = 1; i < dirs.count(); ++i)
            dirs[i].append(QLatin1String("/fonts"));
        break;
    default:
        break;
    }
    const QString localDir = writableLocation(type);
    dirs.prepend(localDir);
    return dirs;
}

QT_END_NAMESPACE

#endif // QT_NO_STANDARDPATHS
