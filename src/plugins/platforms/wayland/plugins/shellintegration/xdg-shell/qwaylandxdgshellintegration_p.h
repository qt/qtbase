// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "qwayland-xdg-shell.h"

#include <QtWaylandClient/private/qwaylandshellintegration_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandXdgShell;

class Q_WAYLANDCLIENT_EXPORT QWaylandXdgShellIntegration
    : public QWaylandShellIntegrationTemplate<QWaylandXdgShellIntegration>,
      public QtWayland::xdg_wm_base
{
public:
    QWaylandXdgShellIntegration();
    ~QWaylandXdgShellIntegration() override;
    QWaylandShellSurface *createShellSurface(QWaylandWindow *window) override;
    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) override;
    bool initialize(QWaylandDisplay *display) override;

protected:
    void xdg_wm_base_ping(uint32_t serial) override;

private:
    QWaylandDisplay *mDisplay;
    QScopedPointer<QWaylandXdgShell> mXdgShell;
};

}

QT_END_NAMESPACE
