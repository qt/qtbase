/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QGENERICPLUGIN_H
#define QGENERICPLUGIN_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_NAMESPACE

#define QGenericPluginFactoryInterface_iid "org.qt-project.Qt.QGenericPluginFactoryInterface"

class Q_GUI_EXPORT QGenericPlugin : public QObject
{
    Q_OBJECT
public:
    explicit QGenericPlugin(QObject *parent = nullptr);
    ~QGenericPlugin();

    virtual QObject* create(const QString& name, const QString &spec) = 0;
};

QT_END_NAMESPACE

#endif // QGENERICPLUGIN_H
