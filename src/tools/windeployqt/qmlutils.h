// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLUTILS_H
#define QMLUTILS_H

#include "utils.h"

#include <QStringList>

QT_BEGIN_NAMESPACE

QString findQmlDirectory(Platform platform, const QString &startDirectoryName);

struct QmlImportScanResult {
    struct Module {
        QString installPath(const QString &root) const;

        QString name;
        QString className;
        QString sourcePath;
        QString relativePath;
    };

    void append(const QmlImportScanResult &other);

    bool ok = false;
    QList<Module> modules;
    QStringList plugins;
};

bool operator==(const QmlImportScanResult::Module &m1, const QmlImportScanResult::Module &m2);

QmlImportScanResult runQmlImportScanner(const QString &directory, const QStringList &qmlImportPaths,
                                        bool usesWidgets, int platform, DebugMatchMode debugMatchMode,
                                        QString *errorMessage, int timeout = 30000);

QT_END_NAMESPACE

#endif // QMLUTILS_H
