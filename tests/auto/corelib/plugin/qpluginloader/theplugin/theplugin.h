// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef THEPLUGIN_H
#define THEPLUGIN_H

#include <QObject>
#include <QtPlugin>
#include "plugininterface.h"

class ThePlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.autotests.plugininterface" FILE "../utf8_data.json")
    Q_INTERFACES(PluginInterface)

public:
    virtual QString pluginName() const override;
};

#endif // THEPLUGIN_H

