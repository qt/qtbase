// Copyright (C) 2018 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wayland-client.h>
#include <QtWaylandClient/private/qwayland-wayland.h>
#include <QtWaylandClient/private/qwaylandshellintegration_p.h>

#include "qwayland-fullscreen-shell-unstable-v1.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class Q_WAYLANDCLIENT_EXPORT QWaylandFullScreenShellV1Integration
    : public QWaylandShellIntegrationTemplate<QWaylandFullScreenShellV1Integration>,
      public QtWayland::zwp_fullscreen_shell_v1
{
public:
    QWaylandFullScreenShellV1Integration();
    ~QWaylandFullScreenShellV1Integration() override;
    QWaylandShellSurface *createShellSurface(QWaylandWindow *window) override;
};

} // namespace QtWaylandClient

QT_END_NAMESPACE
