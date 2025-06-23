// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2020 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:provides-trusted-directory-paths

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

using namespace Qt::StringLiterals;

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

#if QT_CONFIG(regularexpression)
static QLatin1StringView xdg_key_name(QStandardPaths::StandardLocation type)
{
    switch (type) {
    case QStandardPaths::DesktopLocation:
        return "DESKTOP"_L1;
    case QStandardPaths::DocumentsLocation:
        return "DOCUMENTS"_L1;
    case QStandardPaths::PicturesLocation:
        return "PICTURES"_L1;
    case QStandardPaths::MusicLocation:
        return "MUSIC"_L1;
    case QStandardPaths::MoviesLocation:
        return "VIDEOS"_L1;
    case QStandardPaths::DownloadLocation:
        return "DOWNLOAD"_L1;
    case QStandardPaths::PublicShareLocation:
        return "PUBLICSHARE"_L1;
    case QStandardPaths::TemplatesLocation:
        return "TEMPLATES"_L1;
    default:
        return {};
    }
}
#endif

static QByteArray unixPermissionsText(QFile::Permissions permissions)
{
    mode_t perms = 0;
    if (permissions & QFile::ReadOwner)
        perms |= S_IRUSR;
    if (permissions & QFile::WriteOwner)
        perms |= S_IWUSR;
    if (permissions & QFile::ExeOwner)
        perms |= S_IXUSR;
    if (permissions & QFile::ReadGroup)
        perms |= S_IRGRP;
    if (permissions & QFile::WriteGroup)
        perms |= S_IWGRP;
    if (permissions & QFile::ExeGroup)
        perms |= S_IXGRP;
    if (permissions & QFile::ReadOther)
        perms |= S_IROTH;
    if (permissions & QFile::WriteOther)
        perms |= S_IWOTH;
    if (permissions & QFile::ExeOther)
        perms |= S_IXOTH;
    return '0' + QByteArray::number(perms, 8);
}

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

        description += " permissions " + unixPermissionsText(metaData.permissions());

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
        qWarning("QStandardPaths: wrong permissions on runtime directory %ls, %s instead of %s",
                 qUtf16Printable(xdgRuntimeDir),
                 unixPermissionsText(metaData.permissions()).constData(),
                 unixPermissionsText(wantedPerms).constData());
        return false;
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
        QString xdgCacheHome;
        if (isTestModeEnabled()) {
            xdgCacheHome = QDir::homePath() + "/.qttest/cache"_L1;
        } else {
            // http://standards.freedesktop.org/basedir-spec/basedir-spec-0.6.html
            xdgCacheHome = qEnvironmentVariable("XDG_CACHE_HOME");
            if (!xdgCacheHome.startsWith(u'/'))
                xdgCacheHome.clear(); // spec says relative paths should be ignored

            if (xdgCacheHome.isEmpty())
                xdgCacheHome = QDir::homePath() + "/.cache"_L1;
        }
        if (type == QStandardPaths::CacheLocation)
            appendOrganizationAndApp(xdgCacheHome);
        return xdgCacheHome;
    }
    case StateLocation:
    case GenericStateLocation:
    {
        QString xdgStateHome;
        if (isTestModeEnabled()) {
            xdgStateHome = QDir::homePath() + "/.qttest/state"_L1;
        } else {
            // http://standards.freedesktop.org/basedir-spec/basedir-spec-0.8.html
            xdgStateHome = qEnvironmentVariable("XDG_STATE_HOME");
            if (!xdgStateHome.startsWith(u'/'))
                xdgStateHome.clear(); // spec says relative paths should be ignored

            if (xdgStateHome.isEmpty())
                xdgStateHome = QDir::homePath() + "/.local/state"_L1;
        }
        if (type == QStandardPaths::StateLocation)
            appendOrganizationAndApp(xdgStateHome);
        return xdgStateHome;
    }
    case AppDataLocation:
    case AppLocalDataLocation:
    case GenericDataLocation:
    {
        QString xdgDataHome;
        if (isTestModeEnabled()) {
            xdgDataHome = QDir::homePath() + "/.qttest/share"_L1;
        } else {
            xdgDataHome = qEnvironmentVariable("XDG_DATA_HOME");
            if (!xdgDataHome.startsWith(u'/'))
                xdgDataHome.clear(); // spec says relative paths should be ignored

            if (xdgDataHome.isEmpty())
                xdgDataHome = QDir::homePath() + "/.local/share"_L1;
        }
        if (type == AppDataLocation || type == AppLocalDataLocation)
            appendOrganizationAndApp(xdgDataHome);
        return xdgDataHome;
    }
    case ConfigLocation:
    case GenericConfigLocation:
    case AppConfigLocation:
    {
        QString xdgConfigHome;
        if (isTestModeEnabled()) {
            xdgConfigHome = QDir::homePath() + "/.qttest/config"_L1;
        } else {
            // http://standards.freedesktop.org/basedir-spec/latest/
            xdgConfigHome = qEnvironmentVariable("XDG_CONFIG_HOME");
            if (!xdgConfigHome.startsWith(u'/'))
                xdgConfigHome.clear(); // spec says relative paths should be ignored

            if (xdgConfigHome.isEmpty())
                xdgConfigHome = QDir::homePath() + "/.config"_L1;
        }
        if (type == AppConfigLocation)
            appendOrganizationAndApp(xdgConfigHome);
        return xdgConfigHome;
    }
    case RuntimeLocation:
    {
        QString xdgRuntimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
        if (!xdgRuntimeDir.startsWith(u'/'))
            xdgRuntimeDir.clear(); // spec says relative paths should be ignored

        bool fromEnv = !xdgRuntimeDir.isEmpty();
        if (xdgRuntimeDir.isEmpty() || !checkXdgRuntimeDir(xdgRuntimeDir)) {
            // environment variable not set or is set to something unsuitable
            const uint myUid = uint(geteuid());
            const QString userName = QFileSystemEngine::resolveUserName(myUid);
            xdgRuntimeDir = QDir::tempPath() + "/runtime-"_L1 + userName;

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
    QString xdgConfigHome = qEnvironmentVariable("XDG_CONFIG_HOME");
    if (!xdgConfigHome.startsWith(u'/'))
        xdgConfigHome.clear(); // spec says relative paths should be ignored

    if (xdgConfigHome.isEmpty())
        xdgConfigHome = QDir::homePath() + "/.config"_L1;
    QFile file(xdgConfigHome + "/user-dirs.dirs"_L1);
    const QLatin1StringView key = xdg_key_name(type);
    if (!key.isEmpty() && !isTestModeEnabled() && file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        // Only look for lines like: XDG_DESKTOP_DIR="$HOME/Desktop"
        static const QRegularExpression exp(u"^XDG_(.*)_DIR=(.*)$"_s);
        QString result;
        while (!stream.atEnd()) {
            const QString &line = stream.readLine();
            QRegularExpressionMatch match = exp.match(line);
            if (match.hasMatch() && match.capturedView(1) == key) {
                QStringView value = match.capturedView(2);
                if (value.size() > 2
                    && value.startsWith(u'\"')
                    && value.endsWith(u'\"'))
                    value = value.mid(1, value.size() - 2);
                // value can start with $HOME
                if (value.startsWith("$HOME"_L1))
                    result = QDir::homePath() + value.mid(5);
                else
                    result = value.toString();
                if (result.size() > 1 && result.endsWith(u'/'))
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
        path = QDir::homePath() + "/Desktop"_L1;
        break;
    case DocumentsLocation:
        path = QDir::homePath() + "/Documents"_L1;
       break;
    case PicturesLocation:
        path = QDir::homePath() + "/Pictures"_L1;
        break;

    case FontsLocation:
        path = writableLocation(GenericDataLocation) + "/fonts"_L1;
        break;

    case MusicLocation:
        path = QDir::homePath() + "/Music"_L1;
        break;

    case MoviesLocation:
        path = QDir::homePath() + "/Videos"_L1;
        break;
    case DownloadLocation:
        path = QDir::homePath() + "/Downloads"_L1;
        break;
    case ApplicationsLocation:
        path = writableLocation(GenericDataLocation) + "/applications"_L1;
        break;

    case PublicShareLocation:
        path = QDir::homePath() + "/Public"_L1;
        break;

    case TemplatesLocation:
        path = QDir::homePath() + "/Templates"_L1;
        break;

    default:
        break;
    }

    return path;
}

static QStringList dirsList(const QString &xdgEnvVar)
{
    QStringList dirs;
    // http://standards.freedesktop.org/basedir-spec/latest/
    // Normalize paths, skip relative paths (the spec says relative paths
    // should be ignored)
    for (const auto dir : qTokenize(xdgEnvVar, u':'))
        if (dir.startsWith(u'/'))
            dirs.push_back(QDir::cleanPath(dir.toString()));

    // Remove duplicates from the list, there's no use for duplicated paths
    // in XDG_* env vars - if whatever is being looked for is not found in
    // the given directory the first time, it won't be there the second time.
    // Plus duplicate paths causes problems for example for mimetypes,
    // where duplicate paths here lead to duplicated mime types returned
    // for a file, eg "text/plain,text/plain" instead of "text/plain"
    dirs.removeDuplicates();

    return dirs;
}

static QStringList xdgDataDirs()
{
    // http://standards.freedesktop.org/basedir-spec/latest/
    QString xdgDataDirsEnv = qEnvironmentVariable("XDG_DATA_DIRS");

    QStringList dirs = dirsList(xdgDataDirsEnv);
    if (dirs.isEmpty())
        dirs = QStringList{u"/usr/local/share"_s, u"/usr/share"_s};

    return dirs;
}

static QStringList xdgConfigDirs()
{
    // http://standards.freedesktop.org/basedir-spec/latest/
    const QString xdgConfigDirs = qEnvironmentVariable("XDG_CONFIG_DIRS");

    QStringList dirs = dirsList(xdgConfigDirs);
    if (dirs.isEmpty())
        dirs.push_back(u"/etc/xdg"_s);

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
        for (int i = 0; i < dirs.size(); ++i)
            appendOrganizationAndApp(dirs[i]);
        break;
    case GenericDataLocation:
        dirs = xdgDataDirs();
        break;
    case ApplicationsLocation:
        dirs = xdgDataDirs();
        for (int i = 0; i < dirs.size(); ++i)
            dirs[i].append("/applications"_L1);
        break;
    case AppDataLocation:
    case AppLocalDataLocation:
        dirs = xdgDataDirs();
        for (int i = 0; i < dirs.size(); ++i)
            appendOrganizationAndApp(dirs[i]);
        break;
    case FontsLocation:
        dirs += QDir::homePath() + "/.fonts"_L1;
        dirs += xdgDataDirs();
        for (int i = 1; i < dirs.size(); ++i)
            dirs[i].append("/fonts"_L1);
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
