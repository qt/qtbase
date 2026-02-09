// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#include "qwaylandshellintegration_p.h"
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandShellIntegration::~QWaylandShellIntegration()
    = default; // MUST stay empty until Qt 7 (was inline in Qt < 6.9)

wl_surface *QWaylandShellIntegration::wlSurfaceForWindow(QWaylandWindow *window)
{
    return window->wlSurface();
}

}

QT_END_NAMESPACE
