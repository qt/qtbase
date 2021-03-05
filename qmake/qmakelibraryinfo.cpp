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

#include <qmakeconfig.cpp>

QT_BEGIN_NAMESPACE

QString QMakeLibraryInfo::binaryAbsLocation;
QString QMakeLibraryInfo::qtconfManualPath;

struct QMakeLibrarySettings
{
    QMakeLibrarySettings() { load(); }

    void load();

    QScopedPointer<QSettings> settings;
    bool haveDevicePaths;
    bool haveEffectiveSourcePaths;
    bool haveEffectivePaths;
    bool havePaths;
    bool reloadOnQAppAvailable;
};
Q_GLOBAL_STATIC(QMakeLibrarySettings, qmake_library_settings)

QSettings *QMakeLibraryInfo::findConfiguration()
{
    QString qtconfig = libraryInfoFile();
    if (!qtconfig.isEmpty())
        return new QSettings(qtconfig, QSettings::IniFormat);
    return nullptr; // no luck
}

QSettings *QMakeLibraryInfo::configuration()
{
    QMakeLibrarySettings *ls = qmake_library_settings();
    return ls ? ls->settings.data() : nullptr;
}

void QMakeLibraryInfo::reload()
{
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

void QMakeLibrarySettings::load()
{
    settings.reset(QMakeLibraryInfo::findConfiguration());
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
    if (loc < QMakeLibraryInfo::FirstHostPath || loc > QMakeLibraryInfo::LastHostPath)
        sysrootify(ret);

    return ret;
}

struct LocationInfo
{
    QString key;
    QString defaultValue;
};

static LocationInfo defaultLocationInfo(int loc)
{
    LocationInfo result;

    if (loc == QMakeLibraryInfo::SysrootPath) {
        result.key = QStringLiteral("Sysroot");
    } else if (loc == QMakeLibraryInfo::SysrootifyPrefixPath) {
        result.key = QStringLiteral("SysrootifyPrefix");
    } else if (unsigned(loc) < sizeof(qtConfEntries) / sizeof(qtConfEntries[0])) {
        result.key = QLatin1String(qtConfEntries[loc].key);
        result.defaultValue = QLatin1String(qtConfEntries[loc].value);
    }
#ifndef Q_OS_WIN // On Windows we use the registry
    else if (loc == QLibraryInfo::SettingsPath) {
        result.key = QLatin1String("Settings");
        result.defaultValue = QLatin1String(".");
    }
#endif
    return result;
}

static QString storedPath(int loc)
{
    QString result;

    // "volatile" here is a hack to prevent compilers from doing a
    // compile-time strlen() on "path". The issue is that Qt installers
    // will binary-patch the Qt installation paths -- in such scenarios, Qt
    // will be built with a dummy path, thus the compile-time result of
    // strlen is meaningless.
    const char *volatile path = nullptr;
    if (loc == QLibraryInfo::PrefixPath || loc == QMakeLibraryInfo::HostPrefixPath) {
        result = QLibraryInfo::path(QLibraryInfo::PrefixPath);
    } else if (loc == QMakeLibraryInfo::SysrootPath) {
        // empty result
    } else if (loc == QMakeLibraryInfo::SysrootifyPrefixPath) {
        result = QStringLiteral("false");
    } else if (unsigned(loc)
               <= sizeof(qt_configure_str_offsets) / sizeof(qt_configure_str_offsets[0])) {
        path = qt_configure_strs + qt_configure_str_offsets[loc - 1];
#ifndef Q_OS_WIN // On Windows we use the registry
    } else if (loc == QLibraryInfo::SettingsPath) {
        path = QT_CONFIGURE_SETTINGS_PATH;
#endif
    }

    if (path)
        result = QString::fromLocal8Bit(path);

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
            QSettings *config = QMakeLibraryInfo::configuration();
            config->beginGroup(QLatin1String(group == DevicePaths ? "DevicePaths"
                                                     : group == EffectiveSourcePaths
                                                     ? "EffectiveSourcePaths"
                                                     : group == EffectivePaths ? "EffectivePaths"
                                                                               : "Paths"));

            ret = config->value(locinfo.key, locinfo.defaultValue).toString();

            if (ret.isEmpty()) {
                if (loc == HostPrefixPath) {
                    locinfo = defaultLocationInfo(QLibraryInfo::PrefixPath);
                    ret = config->value(locinfo.key, locinfo.defaultValue).toString();
                } else if (loc == TargetSpecPath || loc == HostSpecPath
                           || loc == SysrootifyPrefixPath) {
                    fromConf = false;
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
            baseDir = QFileInfo(libraryInfoFile()).absolutePath();
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

QString QMakeLibraryInfo::libraryInfoFile()
{
    if (!qtconfManualPath.isEmpty())
        return qtconfManualPath;
    if (!binaryAbsLocation.isEmpty()) {
        QDir dir(QFileInfo(binaryAbsLocation).absolutePath());
        QString qtconfig = dir.filePath("qt" QT_STRINGIFY(QT_VERSION_MAJOR) ".conf");
        if (QFile::exists(qtconfig))
            return qtconfig;
        qtconfig = dir.filePath("qt.conf");
        if (QFile::exists(qtconfig))
            return qtconfig;
    }
    return QString();
}

QT_END_NAMESPACE
