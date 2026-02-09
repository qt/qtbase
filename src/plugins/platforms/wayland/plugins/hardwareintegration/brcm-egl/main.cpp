// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtWaylandClient/private/qwaylandclientbufferintegrationplugin_p.h>
#include "qwaylandbrcmeglintegration.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandBrcmEglClientBufferPlugin : public QWaylandClientBufferIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QWaylandClientBufferIntegrationFactoryInterface_iid FILE "brcm-egl.json")
public:
    QWaylandClientBufferIntegration *create(const QString&, const QStringList&) override;
};

QWaylandClientBufferIntegration *QWaylandBrcmEglClientBufferPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList);
    Q_UNUSED(system);
    return new QWaylandBrcmEglIntegration();
}

}

QT_END_NAMESPACE

#include "main.moc"
