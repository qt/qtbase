// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef ALMOSTPLUGIN_H
#define ALMOSTPLUGIN_H

#include <QObject>
#include <QtPlugin>
#include "../theplugin/plugininterface.h"

class AlmostPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.autotests.plugininterface" FILE "../empty.json")
    Q_INTERFACES(PluginInterface)

public:
    QString pluginName() const override;
    void unresolvedSymbol() const;
};

#endif // ALMOSTPLUGIN_H
