// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <qpa/qplatformintegrationplugin.h>
#include <QtWaylandClient/private/qwaylandintegration_p.h>
#include <QtWaylandClient/private/qtwaylandclientglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "qwayland.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&) override;
};

QPlatformIntegration *QWaylandIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList)
#if !QT_CONFIG(wayland_egl)
    if (system == "wayland-egl")
        return nullptr;
#endif
#if !QT_CONFIG(wayland_brcm)
    if (system == "wayland-brcm")
        return nullptr;
#endif
    auto *integration = new QWaylandIntegration(system);

    if (!integration->init()) {
        delete integration;
        integration = nullptr;
    }

    return integration;
}

} // namespace QtWaylandClient

QT_END_NAMESPACE

#include "main.moc"
