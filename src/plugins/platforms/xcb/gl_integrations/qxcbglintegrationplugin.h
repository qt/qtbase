/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QXCBGLINTEGRATIONPLUGIN_H
#define QXCBGLINTEGRATIONPLUGIN_H

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

#endif //QXCBGLINTEGRATIONPLUGIN_H
