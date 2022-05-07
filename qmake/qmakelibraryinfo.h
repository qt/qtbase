/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
