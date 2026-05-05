// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "qxcbexport.h"
#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_NAMESPACE

#define QXcbGlIntegrationFactoryInterface_iid "org.qt-project.Qt.QPA.Xcb.QXcbGlIntegrationFactoryInterface.5.5"

class QXcbGlIntegration;

class Q_XCB_EXPORT QXcbGlIntegrationPlugin : public QObject
{
    Q_OBJECT
public:
        explicit QXcbGlIntegrationPlugin(QObject *parent = nullptr)
            : QObject(parent)
        { }

        virtual QXcbGlIntegration *create() = 0;
    // the pattern expected by qLoadPlugin calls for a QString argument.
    // we don't need it, so don't bother subclasses with it:
    QXcbGlIntegration *create(const QString &) { return create(); }
};
QT_END_NAMESPACE
