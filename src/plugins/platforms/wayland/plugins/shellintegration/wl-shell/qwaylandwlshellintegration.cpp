// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandwlshellintegration_p.h"
#include "qwaylandwlshellsurface_p.h"

#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include <QtWaylandClient/private/qwaylanddisplay_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandWlShellIntegration::QWaylandWlShellIntegration(QObject *parent) : QWaylandShellIntegrationTemplate(1, parent)
{
    qCWarning(lcQpaWayland) << "\"wl-shell\" is a deprecated shell extension, prefer using"
                            << "\"xdg-shell\" if supported by the compositor"
                            << "by setting the environment variable QT_WAYLAND_SHELL_INTEGRATION";
}

QWaylandWlShellIntegration::~QWaylandWlShellIntegration()
{
    if (object())
        wl_shell_destroy(object());
}

QWaylandShellSurface *QWaylandWlShellIntegration::createShellSurface(QWaylandWindow *window)
{
    return new QWaylandWlShellSurface(get_shell_surface(window->wlSurface()), window);
}

void *QWaylandWlShellIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    QByteArray lowerCaseResource = resource.toLower();
    if (lowerCaseResource == "wl_shell_surface") {
        if (auto waylandWindow = static_cast<QWaylandWindow *>(window->handle())) {
            if (auto shellSurface = qobject_cast<QWaylandWlShellSurface *>(waylandWindow->shellSurface())) {
                return shellSurface->object();
            }
        }
    }
    return nullptr;
}

} // namespace QtWaylandClient

QT_END_NAMESPACE
