// Copyright (C) 2026 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef FUTUREPLUGIN_H
#define FUTUREPLUGIN_H

#include <QtCore/qobject.h>
#include <QtCore/qplugin.h>
#include "../plugin2/plugininterface2.h"

// A plugin advertising the same IID as plugin2, but whose metadata claims a
// far-future Qt version (see futureplugin.cpp).
class FuturePlugin : public QObject, public PluginInterface2
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.autotests.plugininterface2")
    Q_INTERFACES(PluginInterface2)

public:
    QString pluginName() const override;
};

#endif // FUTUREPLUGIN_H
