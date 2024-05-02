// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QObject>

class PluginClass : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.tests.moc" FILE "staticplugin.json")
};

#include "main.moc"
