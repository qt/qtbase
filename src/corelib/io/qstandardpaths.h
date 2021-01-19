/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QSTANDARDPATHS_H
#define QSTANDARDPATHS_H

#include <QtCore/qstringlist.h>
#include <QtCore/qobjectdefs.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_STANDARDPATHS

class Q_CORE_EXPORT QStandardPaths
{
    Q_GADGET

public:
    // Do not re-order, must match QDesktopServices
    enum StandardLocation {
        DesktopLocation,
        DocumentsLocation,
        FontsLocation,
        ApplicationsLocation,
        MusicLocation,
        MoviesLocation,
        PicturesLocation,
        TempLocation,
        HomeLocation,
        DataLocation,
        CacheLocation,
        GenericDataLocation,
        RuntimeLocation,
        ConfigLocation,
        DownloadLocation,
        GenericCacheLocation,
        GenericConfigLocation,
        AppDataLocation,
        AppConfigLocation,
        AppLocalDataLocation = DataLocation
    };
    Q_ENUM(StandardLocation)

    static QString writableLocation(StandardLocation type);
    static QStringList standardLocations(StandardLocation type);

    enum LocateOption {
        LocateFile = 0x0,
        LocateDirectory = 0x1
    };
    Q_DECLARE_FLAGS(LocateOptions, LocateOption)
    Q_FLAG(LocateOptions)

    static QString locate(StandardLocation type, const QString &fileName, LocateOptions options = LocateFile);
    static QStringList locateAll(StandardLocation type, const QString &fileName, LocateOptions options = LocateFile);
#ifndef QT_BOOTSTRAPPED
    static QString displayName(StandardLocation type);
#endif

    static QString findExecutable(const QString &executableName, const QStringList &paths = QStringList());

#if QT_DEPRECATED_SINCE(5, 2)
    static QT_DEPRECATED void enableTestMode(bool testMode);
#endif
    static void setTestModeEnabled(bool testMode);
    static bool isTestModeEnabled();

private:
    // prevent construction
    QStandardPaths();
    ~QStandardPaths();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QStandardPaths::LocateOptions)

#endif // QT_NO_STANDARDPATHS

QT_END_NAMESPACE

#endif // QSTANDARDPATHS_H
