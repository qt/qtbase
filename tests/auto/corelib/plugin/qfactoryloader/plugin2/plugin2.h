// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef THEPLUGIN_H
#define THEPLUGIN_H

#include <QtCore/qobject.h>
#include <QtCore/qplugin.h>
#include "plugininterface2.h"

class Plugin2 : public QObject, public PluginInterface2
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.autotests.plugininterface2")
    Q_INTERFACES(PluginInterface2)

public:
    virtual QString pluginName() const override;
};

#endif // THEPLUGIN_H
