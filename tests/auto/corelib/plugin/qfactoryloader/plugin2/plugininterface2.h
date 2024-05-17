// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef PLUGININTERFACE2_H
#define PLUGININTERFACE2_H

#include <QtCore/QtGlobal>

struct PluginInterface2 {
    virtual ~PluginInterface2() {}
    virtual QString pluginName() const = 0;
};

QT_BEGIN_NAMESPACE

#define PluginInterface2_iid "org.qt-project.Qt.autotests.plugininterface2"

Q_DECLARE_INTERFACE(PluginInterface2, PluginInterface2_iid)

QT_END_NAMESPACE

#endif // PLUGININTERFACE2_H
