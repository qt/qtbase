/****************************************************************************
**
** Copyright (C) 2014 Robin Burchell <robin.burchell@viroteck.net>
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <QtGui/qgenericplugin.h>
#include <QCoreApplication>

#include "qtuiohandler_p.h"

QT_BEGIN_NAMESPACE

class QTuioTouchPlugin : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QGenericPluginFactoryInterface_iid FILE "tuiotouch.json")

public:
    QTuioTouchPlugin();

    QObject* create(const QString &key, const QString &specification);
};

QTuioTouchPlugin::QTuioTouchPlugin()
{
}

QObject* QTuioTouchPlugin::create(const QString &key,
                                         const QString &spec)
{
    if (!key.compare(QLatin1String("TuioTouch"), Qt::CaseInsensitive))
        return new QTuioHandler(spec);

    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
