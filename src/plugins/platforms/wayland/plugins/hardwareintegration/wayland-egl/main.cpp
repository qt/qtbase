// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtWaylandClient/private/qwaylandclientbufferintegrationplugin_p.h>
#include "qwaylandeglclientbufferintegration_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandEglClientBufferPlugin : public QWaylandClientBufferIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QWaylandClientBufferIntegrationFactoryInterface_iid FILE "wayland-egl.json")
public:
    QWaylandClientBufferIntegration *create(const QString&, const QStringList&) override;
};

QWaylandClientBufferIntegration *QWaylandEglClientBufferPlugin::create(const QString&, const QStringList&)
{
    return new QWaylandEglClientBufferIntegration();
}

}

QT_END_NAMESPACE

#include "main.moc"
