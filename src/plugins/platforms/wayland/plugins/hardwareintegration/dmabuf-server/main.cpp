// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtWaylandClient/private/qwaylandserverbufferintegrationplugin_p.h>
#include "dmabufserverbufferintegration.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class DmaBufServerBufferPlugin : public QWaylandServerBufferIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QWaylandServerBufferIntegrationFactoryInterface_iid FILE "dmabuf-server.json")
public:
    QWaylandServerBufferIntegration *create(const QString&, const QStringList&) override;
};

QWaylandServerBufferIntegration *DmaBufServerBufferPlugin::create(const QString&, const QStringList&)
{
    return new DmaBufServerBufferIntegration();
}

}

QT_END_NAMESPACE

#include "main.moc"
