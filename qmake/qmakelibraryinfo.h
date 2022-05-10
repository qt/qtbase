// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMAKELIBRARYINFO_H
#define QMAKELIBRARYINFO_H

#include <qlibraryinfo.h>
#include <qstring.h>
#include <qstringlist.h>

QT_BEGIN_NAMESPACE

class QSettings;

struct QMakeLibraryInfo
{
    static QString path(int loc);

    /* This enum has to start after the last value in QLibraryInfo::LibraryPath(NOT SettingsPath!).
     * See qconfig.cpp.in and QLibraryInfo for details.
     * When adding enum values between FirstHostPath and LastHostPath, make sure to adjust
     * the hostToTargetPathEnum(int) function.
     */
    enum LibraryPathQMakeExtras {
        HostBinariesPath = QLibraryInfo::TestsPath + 1,
        FirstHostPath = HostBinariesPath,
        HostLibraryExecutablesPath,
        HostLibrariesPath,
        HostDataPath,
        HostPrefixPath,
        LastHostPath = HostPrefixPath,
        TargetSpecPath,
        HostSpecPath,
        SysrootPath,
        SysrootifyPrefixPath
    };
    enum PathGroup { FinalPaths, EffectivePaths, EffectiveSourcePaths, DevicePaths };
    static QString rawLocation(int loc, PathGroup group);
    static void reload();
    static bool haveGroup(PathGroup group);
    static void sysrootify(QString &path);
};

QT_END_NAMESPACE

#endif // QMAKELIBRARYINFO_H
