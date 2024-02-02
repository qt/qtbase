// Copyright (C) 2018 Kitware, Inc.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QMOCK4PLUGIN_H
#define QMOCK4PLUGIN_H

#include <QObject>
#include <QtMockPlugins1/QMockPlugin>

QT_BEGIN_NAMESPACE

class QMock4Plugin : public QObject, public QMockPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QMockPlugin_iid FILE "mock4plugin.json")
    Q_INTERFACES(QMockPlugin)
public:
    QString pluginName() const override;
};

QT_END_NAMESPACE

#endif // QMOCK4PLUGIN_H
