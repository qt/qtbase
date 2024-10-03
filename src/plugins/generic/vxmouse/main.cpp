// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qgenericplugin.h>
#include <QtInputSupport/private/qvxmousemanager_p.h>

QT_BEGIN_NAMESPACE

class QVxMousePlugin : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QGenericPluginFactoryInterface_iid FILE "vxmouse.json")

public:
    QVxMousePlugin();

    QObject* create(const QString &key, const QString &specification) override;
};

QVxMousePlugin::QVxMousePlugin()
    : QGenericPlugin()
{
}

QObject* QVxMousePlugin::create(const QString &key,
                                const QString &specification)
{
    if (!key.compare(QLatin1String("VxMouse"), Qt::CaseInsensitive))
        return new QVxMouseManager(key, specification);

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
