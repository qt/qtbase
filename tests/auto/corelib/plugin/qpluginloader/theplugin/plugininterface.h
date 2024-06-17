// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QtCore/QtGlobal>

struct PluginInterface {
    virtual ~PluginInterface() {}
    virtual QString pluginName() const = 0;
    virtual const char *architectureName() const { return ""; };
};

QT_BEGIN_NAMESPACE

#define PluginInterface_iid "org.qt-project.Qt.autotests.plugininterface"

Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

QT_END_NAMESPACE

#endif // PLUGININTERFACE_H

