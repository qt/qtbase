// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mylib/mylib.h"

#include <QtTest/qtest.h>

class test_plugin_deployment : public QObject
{
    Q_OBJECT
private slots:
    void loadsTheRightPlugins();
};

void test_plugin_deployment::loadsTheRightPlugins()
{
    const auto formats = getSupportedImageFormats();
    if (!formats.contains("gif")) {
        qDebug() << "supported imageformats: " << formats;
        QFAIL("Cannot load the qgif plugin. Plugin deployment failed.");
    }
}

QTEST_MAIN(test_plugin_deployment)

#include "main.moc"
