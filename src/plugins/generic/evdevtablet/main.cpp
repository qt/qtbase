// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qgenericplugin.h>
#include <QtInputSupport/private/qevdevtabletmanager_p.h>

QT_BEGIN_NAMESPACE

class QEvdevTabletPlugin : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QGenericPluginFactoryInterface_iid FILE "evdevtablet.json")

public:
    QEvdevTabletPlugin();

    QObject* create(const QString &key, const QString &specification) override;
};

QEvdevTabletPlugin::QEvdevTabletPlugin()
{
}

QObject* QEvdevTabletPlugin::create(const QString &key,
                                    const QString &spec)
{
    if (!key.compare(QLatin1String("EvdevTablet"), Qt::CaseInsensitive))
        return new QEvdevTabletManager(key, spec);

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
