// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qgenericplugin.h>
#include <QtInputSupport/private/qevdevmousemanager_p.h>

QT_BEGIN_NAMESPACE

class QEvdevMousePlugin : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QGenericPluginFactoryInterface_iid FILE "evdevmouse.json")

public:
    QEvdevMousePlugin();

    QObject* create(const QString &key, const QString &specification) override;
};

QEvdevMousePlugin::QEvdevMousePlugin()
    : QGenericPlugin()
{
}

QObject* QEvdevMousePlugin::create(const QString &key,
                                   const QString &specification)
{
    if (!key.compare(QLatin1String("EvdevMouse"), Qt::CaseInsensitive))
        return new QEvdevMouseManager(key, specification);

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
