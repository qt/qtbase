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

#ifndef QLIBRARYINFO_H
#define QLIBRARYINFO_H

#include <QtCore/qstring.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QLibraryInfo
{
public:
    static const char *build() noexcept;

    static bool isDebugBuild();

#ifndef QT_BOOTSTRAPPED
    static QVersionNumber version() noexcept Q_DECL_CONST_FUNCTION;
#endif

    enum LibraryPath {
        PrefixPath = 0,
        DocumentationPath,
        HeadersPath,
        LibrariesPath,
        LibraryExecutablesPath,
        BinariesPath,
        PluginsPath,
        QmlImportsPath,
        Qml2ImportsPath = QmlImportsPath,
        ArchDataPath,
        DataPath,
        TranslationsPath,
        ExamplesPath,
        TestsPath,
        // Insert new values above this line
        // Please read the comments in qconfig.cpp.in before adding
        SettingsPath = 100
    };
    static QString path(LibraryPath p);
#if QT_DEPRECATED_SINCE(6, 0)
    using LibraryLocation = LibraryPath;
    QT_DEPRECATED_VERSION_X_6_0("Use path()")
    static QString location(LibraryLocation location)
    { return path(location); }
#endif
    static QStringList platformPluginArguments(const QString &platformName);

private:
    QLibraryInfo();
};

QT_END_NAMESPACE

#endif // QLIBRARYINFO_H
