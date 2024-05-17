// Copyright (C) 2018 Kitware, Inc.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QMOCKPLUGIN_H
#define QMOCKPLUGIN_H

#include <QtCore/QString>
#include <QtCore/QtPlugin>

QT_BEGIN_NAMESPACE

#define QMockPlugin_iid "org.qt-project.Qt.Tests.QMockPlugin"

class QMockPlugin
{
public:
    virtual ~QMockPlugin() {}
    virtual QString pluginName() const = 0;
};

Q_DECLARE_INTERFACE(QMockPlugin, QMockPlugin_iid)

QT_END_NAMESPACE

#endif // QMOCKPLUGIN_H
