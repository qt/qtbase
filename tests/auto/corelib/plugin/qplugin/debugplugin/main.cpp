// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtPlugin>
#include <QObject>

class DebugPlugin : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "SomeIID")
public:
    DebugPlugin() {}
};

#include "main.moc"
