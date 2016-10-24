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
#include <qdir.h>
#include <qfile.h>
#include <qhash.h>
#include <qtextstream.h>
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
        const uid_t myUid = geteuid();
        // http://standards.freedesktop.org/basedir-spec/latest/
        QFileInfo fileInfo;
        QString xdgRuntimeDir = QFile::decodeName(qgetenv("XDG_RUNTIME_DIR"));
        if (xdgRuntimeDir.isEmpty()) {
            const QString userName = QFileSystemEngine::resolveUserName(myUid);
            xdgRuntimeDir = QDir::tempPath() + QLatin1String("/runtime-") + userName;
            fileInfo.setFile(xdgRuntimeDir);
            if (!fileInfo.isDir()) {
                if (!QDir().mkdir(xdgRuntimeDir)) {
                    qWarning("QStandardPaths: error creating runtime directory %s: %s", qPrintable(xdgRuntimeDir), qPrintable(qt_error_string(errno)));
                    return QString();
                }
            }
            qWarning("QStandardPaths: XDG_RUNTIME_DIR not set, defaulting to '%s'", qPrintable(xdgRuntimeDir));
        } else {
            fileInfo.setFile(xdgRuntimeDir);
            if (!fileInfo.exists()) {
                qWarning("QStandardPaths: XDG_RUNTIME_DIR points to non-existing path '%s', "
                         "please create it with 0700 permissions.", qPrintable(xdgRuntimeDir));
                return QString();
            }
            if (!fileInfo.isDir()) {
                qWarning("QStandardPaths: XDG_RUNTIME_DIR points to '%s' which is not a directory",
                         qPrintable(xdgRuntimeDir));
                return QString();
            }
        }
        // "The directory MUST be owned by the user"
        if (fileInfo.ownerId() != myUid) {
            qWarning("QStandardPaths: wrong ownership on runtime directory %s, %d instead of %d", qPrintable(xdgRuntimeDir),
                     fileInfo.ownerId(), myUid);
            return QString();
        }
        // "and he MUST be the only one having read and write access to it. Its Unix access mode MUST be 0700."
        // since the current user is the owner, set both xxxUser and xxxOwner
        const QFile::Permissions wantedPerms = QFile::ReadUser | QFile::WriteUser | QFile::ExeUser
                                               | QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner;
        if (fileInfo.permissions() != wantedPerms) {
            QFile file(xdgRuntimeDir);
            if (!file.setPermissions(wantedPerms)) {
                qWarning("QStandardPaths: could not set correct permissions on runtime directory %s: %s",
                         qPrintable(xdgRuntimeDir), qPrintable(file.errorString()));
                return QString();
            }
        }
        return xdgRuntimeDir;
    }
    default:
        break;
    }

#ifndef QT_BOOTSTRAPPED
    // http://www.freedesktop.org/wiki/Software/xdg-user-dirs
    QString xdgConfigHome = QFile::decodeName(qgetenv("XDG_CONFIG_HOME"));
    if (xdgConfigHome.isEmpty())
        xdgConfigHome = QDir::homePath() + QLatin1String("/.config");
    QFile file(xdgConfigHome + QLatin1String("/user-dirs.dirs"));
    if (!isTestModeEnabled() && file.open(QIODevice::ReadOnly)) {
        QHash<QString, QString> lines;
        QTextStream stream(&file);
        // Only look for lines like: XDG_DESKTOP_DIR="$HOME/Desktop"
        QRegExp exp(QLatin1String("^XDG_(.*)_DIR=(.*)$"));
        while (!stream.atEnd()) {
            const QString &line = stream.readLine();
            if (exp.indexIn(line) != -1) {
                const QStringList lst = exp.capturedTexts();
                const QString key = lst.at(1);
                QString value = lst.at(2);
                if (value.length() > 2
                    && value.startsWith(QLatin1Char('\"'))
                    && value.endsWith(QLatin1Char('\"')))
                    value = value.mid(1, value.length() - 2);
                // Store the key and value: "DESKTOP", "$HOME/Desktop"
                lines[key] = value;
            }
        }

        QString key;
        switch (type) {
        case DesktopLocation:
            key = QLatin1String("DESKTOP");
            break;
        case DocumentsLocation:
            key = QLatin1String("DOCUMENTS");
            break;
        case PicturesLocation:
            key = QLatin1String("PICTURES");
            break;
        case MusicLocation:
            key = QLatin1String("MUSIC");
            break;
        case MoviesLocation:
            key = QLatin1String("VIDEOS");
            break;
        case DownloadLocation:
            key = QLatin1String("DOWNLOAD");
            break;
        default:
            break;
        }
        if (!key.isEmpty()) {
            QString value = lines.value(key);
            if (!value.isEmpty()) {
                // value can start with $HOME
                if (value.startsWith(QLatin1String("$HOME")))
                    value = QDir::homePath() + value.midRef(5);
                if (value.length() > 1 && value.endsWith(QLatin1Char('/')))
                    value.chop(1);
                return value;
            }
        }
    }
#endif

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
        dirs = xdgDataDirsEnv.split(QLatin1Char(':'), QString::SkipEmptyParts);

        // Normalize paths, skip relative paths
        QMutableListIterator<QString> it(dirs);
        while (it.hasNext()) {
            const QString dir = it.next();
            if (!dir.startsWith(QLatin1Char('/')))
                it.remove();
            else
                it.setValue(QDir::cleanPath(dir));
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
