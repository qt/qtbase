// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef PLUGININTERFACE1_H
#define PLUGININTERFACE1_H

#include <QtCore/QtGlobal>

struct PluginInterface1 {
    virtual ~PluginInterface1() {}
    virtual QString pluginName() const = 0;
};

QT_BEGIN_NAMESPACE

#define PluginInterface1_iid "org.qt-project.Qt.autotests.plugininterface1"

Q_DECLARE_INTERFACE(PluginInterface1, PluginInterface1_iid)

QT_END_NAMESPACE

#endif // PLUGININTERFACE1_H
