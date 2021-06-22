/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include "qmakelibraryinfo.h"

#include <qdir.h>
#include <qfile.h>
#include <qglobalstatic.h>
#include <qsettings.h>
#include <qscopedpointer.h>
#include <qstringlist.h>
#include <private/qlibraryinfo_p.h>

#include <utility>

QT_BEGIN_NAMESPACE

struct QMakeLibrarySettings
{
    QMakeLibrarySettings() { load(); }
    void load();

    bool haveDevicePaths;
    bool haveEffectiveSourcePaths;
    bool haveEffectivePaths;
    bool havePaths;
};
Q_GLOBAL_STATIC(QMakeLibrarySettings, qmake_library_settings)

void QMakeLibrarySettings::load()
{
    QSettings *settings = QLibraryInfoPrivate::configuration();
    if (settings) {
        QStringList children = settings->childGroups();
        haveDevicePaths = children.contains(QLatin1String("DevicePaths"));
        haveEffectiveSourcePaths = children.contains(QLatin1String("EffectiveSourcePaths"));
        haveEffectivePaths =
                haveEffectiveSourcePaths || children.contains(QLatin1String("EffectivePaths"));
        // Backwards compat: an existing but empty file is claimed to contain the Paths section.
        havePaths = (!haveDevicePaths && !haveEffectivePaths
                     && !children.contains(QLatin1String("Platforms")))
                || children.contains(QLatin1String("Paths"));
    } else {
        haveDevicePaths = false;
        haveEffectiveSourcePaths = false;
        haveEffectivePaths = false;
        havePaths = false;
    }
}

void QMakeLibraryInfo::reload()
{
    QLibraryInfoPrivate::reload();
    if (qmake_library_settings.exists())
        qmake_library_settings->load();
}

bool QMakeLibraryInfo::haveGroup(PathGroup group)
{
    QMakeLibrarySettings *ls = qmake_library_settings();
    return ls
            && (group == EffectiveSourcePaths     ? ls->haveEffectiveSourcePaths
                        : group == EffectivePaths ? ls->haveEffectivePaths
                        : group == DevicePaths    ? ls->haveDevicePaths
                                                  : ls->havePaths);
}

void QMakeLibraryInfo::sysrootify(QString &path)
{
    // Acceptable values for SysrootifyPrefixPath are "true" and "false"
    if (!QVariant::fromValue(rawLocation(SysrootifyPrefixPath, FinalPaths)).toBool())
        return;

    const QString sysroot = rawLocation(SysrootPath, FinalPaths);
    if (sysroot.isEmpty())
        return;

    if (path.length() > 2 && path.at(1) == QLatin1Char(':')
        && (path.at(2) == QLatin1Char('/') || path.at(2) == QLatin1Char('\\'))) {
        path.replace(0, 2, sysroot); // Strip out the drive on Windows targets
    } else {
        path.prepend(sysroot);
    }
}

QString QMakeLibraryInfo::path(int loc)
{
    QString ret = rawLocation(loc, QMakeLibraryInfo::FinalPaths);

    // Automatically prepend the sysroot to target paths
    if (loc < QMakeLibraryInfo::FirstHostPath)
        sysrootify(ret);

    return ret;
}

static QLibraryInfo::LibraryPath hostToTargetPathEnum(int loc)
{
    static std::pair<int, QLibraryInfo::LibraryPath> mapping[] = {
        { QMakeLibraryInfo::HostBinariesPath, QLibraryInfo::BinariesPath },
        { QMakeLibraryInfo::HostLibraryExecutablesPath, QLibraryInfo::LibraryExecutablesPath },
        { QMakeLibraryInfo::HostLibrariesPath, QLibraryInfo::LibrariesPath },
        { QMakeLibraryInfo::HostDataPath, QLibraryInfo::DataPath },
        { QMakeLibraryInfo::HostPrefixPath, QLibraryInfo::PrefixPath }
    };
    for (size_t i = 0; i < sizeof(mapping) / sizeof(mapping[0]); ++i) {
        if (mapping[i].first == loc)
            return mapping[i].second;
    }
    qFatal("Unhandled host path %d in hostToTargetPathEnum.", loc);
    Q_UNREACHABLE();
}

struct LocationInfo
{
    QString key;
    QString defaultValue;
};

static LocationInfo defaultLocationInfo(int loc)
{
    LocationInfo result;

    if (loc < QMakeLibraryInfo::FirstHostPath) {
        QLibraryInfoPrivate::keyAndDefault(static_cast<QLibraryInfo::LibraryPath>(loc),
                                   &result.key, &result.defaultValue);
    } else if (loc <= QMakeLibraryInfo::LastHostPath) {
        QLibraryInfoPrivate::keyAndDefault(hostToTargetPathEnum(loc), &result.key, &result.defaultValue);
        result.key.prepend(QStringLiteral("Host"));
    } else if (loc == QMakeLibraryInfo::SysrootPath) {
        result.key = QStringLiteral("Sysroot");
    } else if (loc == QMakeLibraryInfo::SysrootifyPrefixPath) {
        result.key = QStringLiteral("SysrootifyPrefix");
    } else if (loc == QMakeLibraryInfo::TargetSpecPath) {
        result.key = QStringLiteral("TargetSpec");
    } else if (loc == QMakeLibraryInfo::HostSpecPath) {
        result.key = QStringLiteral("HostSpec");
    }
    return result;
}

static QString storedPath(int loc)
{
    QString result;
    if (loc < QMakeLibraryInfo::FirstHostPath) {
        result = QLibraryInfo::path(static_cast<QLibraryInfo::LibraryPath>(loc));
    } else if (loc <= QMakeLibraryInfo::LastHostPath) {
        if (loc == QMakeLibraryInfo::HostDataPath) {
            // Handle QT_HOST_DATADIR specially. It is not necessarily equal to QT_INSTALL_DATA.
            result = QT_HOST_DATADIR;
        } else {
            result = QLibraryInfo::path(hostToTargetPathEnum(loc));
        }
    } else if (loc == QMakeLibraryInfo::SysrootPath) {
        // empty result
    } else if (loc == QMakeLibraryInfo::SysrootifyPrefixPath) {
        result = QStringLiteral("false");
    } else if (loc == QMakeLibraryInfo::TargetSpecPath) {
        result = QT_TARGET_MKSPEC;
    } else if (loc == QMakeLibraryInfo::HostSpecPath) {
        result = QT_HOST_MKSPEC;
    }
    return result;
}

QString QMakeLibraryInfo::rawLocation(int loc, QMakeLibraryInfo::PathGroup group)
{
    QString ret;
    bool fromConf = false;
    // Logic for choosing the right data source: if EffectivePaths are requested
    // and qt.conf with that section is present, use it, otherwise fall back to
    // FinalPaths. For FinalPaths, use qt.conf if present and contains not only
    // [EffectivePaths], otherwise fall back to builtins.
    // EffectiveSourcePaths falls back to EffectivePaths.
    // DevicePaths falls back to FinalPaths.
    PathGroup orig_group = group;
    if (QMakeLibraryInfo::haveGroup(group)
        || (group == EffectiveSourcePaths
            && (group = EffectivePaths, QMakeLibraryInfo::haveGroup(group)))
        || ((group == EffectivePaths || group == DevicePaths)
            && (group = FinalPaths, QMakeLibraryInfo::haveGroup(group)))
        || (group = orig_group, false)) {
        fromConf = true;

        LocationInfo locinfo = defaultLocationInfo(loc);
        if (!locinfo.key.isNull()) {
            QSettings *config = QLibraryInfoPrivate::configuration();
            Q_ASSERT(config != nullptr);
            config->beginGroup(QLatin1String(group == DevicePaths ? "DevicePaths"
                                                     : group == EffectiveSourcePaths
                                                     ? "EffectiveSourcePaths"
                                                     : group == EffectivePaths ? "EffectivePaths"
                                                                               : "Paths"));

            ret = config->value(locinfo.key).toString();
            if (ret.isEmpty()) {
                if (loc == HostPrefixPath || loc == TargetSpecPath || loc == HostSpecPath
                           || loc == SysrootifyPrefixPath || loc == QLibraryInfo::PrefixPath) {
                    fromConf = false;
                } else {
                    ret = locinfo.defaultValue;
                }
                // The last case here is SysrootPath, which can be legitimately empty.
                // All other keys have non-empty fallbacks to start with.
            }

            // TODO: Might be replaced by common for qmake and qtcore function
            int startIndex = 0;
            forever {
                startIndex = ret.indexOf(QLatin1Char('$'), startIndex);
                if (startIndex < 0)
                    break;
                if (ret.length() < startIndex + 3)
                    break;
                if (ret.at(startIndex + 1) != QLatin1Char('(')) {
                    startIndex++;
                    continue;
                }
                int endIndex = ret.indexOf(QLatin1Char(')'), startIndex + 2);
                if (endIndex < 0)
                    break;
                auto envVarName =
                        QStringView { ret }.mid(startIndex + 2, endIndex - startIndex - 2);
                QString value =
                        QString::fromLocal8Bit(qgetenv(envVarName.toLocal8Bit().constData()));
                ret.replace(startIndex, endIndex - startIndex + 1, value);
                startIndex += value.length();
            }
            config->endGroup();

            ret = QDir::fromNativeSeparators(ret);
        }
    }

    if (!fromConf)
        ret = storedPath(loc);

    // These values aren't actually paths and thus need to be returned verbatim.
    if (loc == TargetSpecPath || loc == HostSpecPath || loc == SysrootifyPrefixPath)
        return ret;

    if (!ret.isEmpty() && QDir::isRelativePath(ret)) {
        QString baseDir;
        if (loc == HostPrefixPath || loc == QLibraryInfo::PrefixPath || loc == SysrootPath) {
            // We make the prefix/sysroot path absolute to the executable's directory.
            // loc == PrefixPath while a sysroot is set would make no sense here.
            // loc == SysrootPath only makes sense if qmake lives inside the sysroot itself.
            QSettings *config = QLibraryInfoPrivate::configuration();
            if (config != nullptr) {
                baseDir = QFileInfo(config->fileName()).absolutePath();
            }
        } else if (loc >= FirstHostPath && loc <= LastHostPath) {
            // We make any other host path absolute to the host prefix directory.
            baseDir = rawLocation(HostPrefixPath, group);
        } else {
            // we make any other path absolute to the prefix directory
            baseDir = rawLocation(QLibraryInfo::PrefixPath, group);
            if (group == EffectivePaths)
                sysrootify(baseDir);
        }
        ret = QDir::cleanPath(baseDir + QLatin1Char('/') + ret);
    }
    return ret;
}

QT_END_NAMESPACE
