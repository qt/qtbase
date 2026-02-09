// Copyright (C) 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandxdgactivationv1_p.h"
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandinputdevice_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandXdgActivationV1::QWaylandXdgActivationV1(wl_registry *registry, uint32_t id,
                                                 uint32_t availableVersion)
    : QtWayland::xdg_activation_v1(registry, id, qMin(availableVersion, 1u))
{
}

QWaylandXdgActivationV1::~QWaylandXdgActivationV1()
{
    Q_ASSERT(isInitialized());
    destroy();
}

QWaylandXdgActivationTokenV1 *
QWaylandXdgActivationV1::requestXdgActivationToken(QWaylandDisplay *display,
                                                   struct ::wl_surface *surface,
                                                   std::optional<uint32_t> serial,
                                                   const QString &app_id)
{
    auto wl = get_activation_token();
    auto provider = new QWaylandXdgActivationTokenV1;
    provider->init(wl);

    if (surface)
        provider->set_surface(surface);

    if (!app_id.isEmpty())
        provider->set_app_id(app_id);

    if (serial && display->lastInputDevice())
        provider->set_serial(*serial, display->lastInputDevice()->wl_seat());
    provider->commit();
    return provider;
}

QWaylandXdgActivationTokenV1::~QWaylandXdgActivationTokenV1()
{
    destroy();
}
}

QT_END_NAMESPACE

#include "moc_qwaylandxdgactivationv1_p.cpp"
