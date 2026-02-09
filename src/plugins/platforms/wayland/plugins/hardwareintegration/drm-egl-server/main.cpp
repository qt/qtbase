// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtWaylandClient/private/qwaylandserverbufferintegrationplugin_p.h>
#include "drmeglserverbufferintegration.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class DrmEglServerBufferPlugin : public QWaylandServerBufferIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QWaylandServerBufferIntegrationFactoryInterface_iid FILE "drm-egl-server.json")
public:
    QWaylandServerBufferIntegration *create(const QString&, const QStringList&) override;
};

QWaylandServerBufferIntegration *DrmEglServerBufferPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList);
    Q_UNUSED(system);
    return new DrmEglServerBufferIntegration();
}

}

QT_END_NAMESPACE

#include "main.moc"
