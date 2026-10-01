// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui/QImageReader>
#include <QtTest/qtest.h>

class test_plugin_deployment_exclude_plugins : public QObject
{
    Q_OBJECT
private slots:
    void doesNotLoadTheExcludedPlugin();
};

void test_plugin_deployment_exclude_plugins::doesNotLoadTheExcludedPlugin()
{
    const auto formats = QImageReader::supportedImageFormats();
    if (formats.contains("gif")) {
        qDebug() << "supported imageformats: " << formats;
        QFAIL("The qgif plugin was deployed, but was excluded.");
    }
}

QTEST_MAIN(test_plugin_deployment_exclude_plugins)

#include "main.moc"
