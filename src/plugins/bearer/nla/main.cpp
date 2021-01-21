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

#include "qnlaengine.h"

#include <QtNetwork/private/qbearerplugin_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

class QNlaEnginePlugin : public QBearerEnginePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QBearerEngineFactoryInterface" FILE "nla.json")

public:
    QNlaEnginePlugin();
    ~QNlaEnginePlugin();

    QBearerEngine *create(const QString &key) const;
};

QNlaEnginePlugin::QNlaEnginePlugin()
{
}

QNlaEnginePlugin::~QNlaEnginePlugin()
{
}

QBearerEngine *QNlaEnginePlugin::create(const QString &key) const
{
    if (key == QLatin1String("nla"))
        return new QNlaEngine;
    else
        return 0;
}

QT_END_NAMESPACE

#include "main.moc"
