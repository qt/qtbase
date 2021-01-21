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

#include <QtGui/qgenericplugin.h>
#include <QtInputSupport/private/qevdevtouchmanager_p.h>

QT_BEGIN_NAMESPACE

class QEvdevTouchScreenPlugin : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QGenericPluginFactoryInterface_iid FILE "evdevtouch.json")

public:
    QEvdevTouchScreenPlugin();

    QObject* create(const QString &key, const QString &specification) override;
};

QEvdevTouchScreenPlugin::QEvdevTouchScreenPlugin()
{
}

QObject* QEvdevTouchScreenPlugin::create(const QString &key,
                                         const QString &spec)
{
    if (!key.compare(QLatin1String("EvdevTouch"), Qt::CaseInsensitive))
        return new QEvdevTouchManager(key, spec);

    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
