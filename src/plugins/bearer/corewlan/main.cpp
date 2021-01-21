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

#include "qcorewlanengine.h"

#include <QtNetwork/private/qbearerplugin_p.h>

#include <QtCore/qdebug.h>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

class QCoreWlanEnginePlugin : public QBearerEnginePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QBearerEngineFactoryInterface" FILE "corewlan.json")

public:
    QCoreWlanEnginePlugin();
    ~QCoreWlanEnginePlugin();

    QBearerEngine *create(const QString &key) const;
};

QCoreWlanEnginePlugin::QCoreWlanEnginePlugin()
{
}

QCoreWlanEnginePlugin::~QCoreWlanEnginePlugin()
{
}

QBearerEngine *QCoreWlanEnginePlugin::create(const QString &key) const
{
    if (key == QLatin1String("corewlan"))
        return new QCoreWlanEngine;
    else
        return 0;
}

QT_END_NAMESPACE

#include "main.moc"

#endif // QT_NO_BEARERMANAGEMENT
