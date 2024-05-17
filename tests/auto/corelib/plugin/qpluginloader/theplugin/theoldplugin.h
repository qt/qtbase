// Copyright (C) 2021 Intel Corportaion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef THEOLDPLUGIN_H
#define THEOLDPLUGIN_H

#include <QObject>
#include <QtPlugin>
#include "plugininterface.h"

class TheOldPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    // Q_PLUGIN_METADATA intentionally missing
    Q_INTERFACES(PluginInterface)

public:
    virtual QString pluginName() const override;
};

#endif // THEOLDPLUGIN_H

