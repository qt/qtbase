// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLIBRARYINFO_P_H
#define QLIBRARYINFO_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qlibraryinfo.h"
#include "QtCore/private/qglobal_p.h"

#if QT_CONFIG(settings)
#    include "QtCore/qsettings.h"
#endif
#include "QtCore/qstring.h"

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QLibraryInfoPrivate final
{
public:
#if QT_CONFIG(settings)
    static QSettings *configuration();
    static void setQtconfManualPath(const QString *qtconfManualPath);
    // Expands $(VAR) placeholders with the value of the VAR environment
    // variable, and converts native separators to '/'.
    static QString expandEnvVariables(QString path);
#endif

    static void reload();

    struct LocationInfo
    {
        QString key;
        QString defaultValue;
        QString fallbackKey;
    };

    static LocationInfo locationInfo(QLibraryInfo::LibraryPath loc);
    static QString path(QLibraryInfo::LibraryPath p);
    static QStringList paths(QLibraryInfo::LibraryPath p);

private:
    static QStringList qtConfPaths(QLibraryInfo::LibraryPath location);
    static QStringList appPaths(QLibraryInfo::LibraryPath location);
    static QStringList qtPaths(QLibraryInfo::LibraryPath location);
};

QT_END_NAMESPACE

#endif // QLIBRARYINFO_P_H
