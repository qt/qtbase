// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef JSON_TEST_PLUGIN_H
#define JSON_TEST_PLUGIN_H

#include <QObject>

class JsonTestPlugin : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "JsonTestPlugin" FILE "jsontestplugin.json")
public:
    JsonTestPlugin(QObject *parent = nullptr);
};

#endif
