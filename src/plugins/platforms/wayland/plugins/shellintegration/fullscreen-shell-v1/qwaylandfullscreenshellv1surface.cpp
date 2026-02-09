// Copyright (C) 2018 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtWaylandClient/private/qwaylandscreen_p.h>

#include "qwaylandfullscreenshellv1surface.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandFullScreenShellV1Surface::QWaylandFullScreenShellV1Surface(QtWayland::zwp_fullscreen_shell_v1 *shell, QWaylandWindow *window)
    : QWaylandShellSurface(window)
    , m_shell(shell)
    , m_window(window)
{
    auto *screen = m_window->waylandScreen();
    auto *output = screen ? screen->output() : nullptr;
    m_shell->present_surface(m_window->wlSurface(),
                             QtWayland::zwp_fullscreen_shell_v1::present_method_default,
                             output);
}

} // namespace QtWaylandClient

QT_END_NAMESPACE
