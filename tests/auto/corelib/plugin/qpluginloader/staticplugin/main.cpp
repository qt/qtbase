// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtPlugin>
#include <QObject>

class StaticPlugin : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "SomeIID" URI "qt.test.pluginloader.staticplugin")
public:
    StaticPlugin() {}
};

#include "main.moc"
