// Copyright (C) 2018 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <QtWaylandClient/qtwaylandclientglobal.h>
#include <QtWaylandClient/private/qwaylandshellsurface_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

#include "qwayland-fullscreen-shell-unstable-v1.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class Q_WAYLANDCLIENT_EXPORT QWaylandFullScreenShellV1Surface : public QWaylandShellSurface
{
public:
    QWaylandFullScreenShellV1Surface(QtWayland::zwp_fullscreen_shell_v1 *shell, QWaylandWindow *window);
    std::any surfaceRole() const override { return m_shell->object(); }

private:
    QtWayland::zwp_fullscreen_shell_v1 *m_shell = nullptr;
    QWaylandWindow *m_window = nullptr;
};

} // namespace QtWaylandClient

QT_END_NAMESPACE
