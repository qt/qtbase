// Copyright (C) 2018 Kitware, Inc.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef QMOCK6PLUGIN_H
#define QMOCK6PLUGIN_H

#include <QObject>
#include <QtMockPlugins3/QMockAuxPlugin>

QT_BEGIN_NAMESPACE

class QMock6Plugin : public QObject, public QMockAuxPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QMockAuxPlugin_iid FILE "mock6plugin.json")
    Q_INTERFACES(QMockAuxPlugin)
public:
    QString pluginName() const override;
};

QT_END_NAMESPACE

#endif // QMOCK6PLUGIN_H
