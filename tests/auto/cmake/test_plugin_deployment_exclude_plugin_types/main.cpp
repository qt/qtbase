// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui/QImageReader>
#include <QtTest/qtest.h>

class test_plugin_deployment_exclude_plugin_types : public QObject
{
    Q_OBJECT
private slots:
    void doesNotLoadTheExcludedPluginType();
};

void test_plugin_deployment_exclude_plugin_types::doesNotLoadTheExcludedPluginType()
{
    const auto formats = QImageReader::supportedImageFormats();
    if (formats.contains("gif")) {
        qDebug() << "supported imageformats: " << formats;
        QFAIL("The imageformats plugins were deployed, but were excluded.");
    }
}

QTEST_MAIN(test_plugin_deployment_exclude_plugin_types)

#include "main.moc"
