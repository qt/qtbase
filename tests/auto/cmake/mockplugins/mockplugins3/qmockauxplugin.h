// Copyright (C) 2018 Kitware, Inc.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QMOCKAUXPLUGIN_H
#define QMOCKAUXPLUGIN_H

#include <QtCore/QString>
#include <QtCore/QtPlugin>

QT_BEGIN_NAMESPACE

#define QMockAuxPlugin_iid "org.qt-project.Qt.Tests.QMockAuxPlugin"

class QMockAuxPlugin
{
public:
    virtual ~QMockAuxPlugin() {}
    virtual QString pluginName() const = 0;
};

Q_DECLARE_INTERFACE(QMockAuxPlugin, QMockAuxPlugin_iid)

QT_END_NAMESPACE

#endif // QMOCKAUXPLUGIN_H
