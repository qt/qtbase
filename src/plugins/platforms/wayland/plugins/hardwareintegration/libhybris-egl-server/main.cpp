// Copyright (C) 2016 Jolla Ltd, author: <giulio.camuffo@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtWaylandClient/private/qwaylandserverbufferintegrationplugin_p.h>
#include "libhybriseglserverbufferintegration.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class LibHybrisEglServerBufferPlugin : public QWaylandServerBufferIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QWaylandServerBufferIntegrationFactoryInterface_iid FILE "libhybris-egl-server.json")
public:
    QWaylandServerBufferIntegration *create(const QString&, const QStringList&) override;
};

QWaylandServerBufferIntegration *LibHybrisEglServerBufferPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList);
    Q_UNUSED(system);
    return new LibHybrisEglServerBufferIntegration();
}

}

QT_END_NAMESPACE

#include "main.moc"
