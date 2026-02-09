// Copyright (C) 2018 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandfullscreenshellv1integration.h"
#include "qwaylandfullscreenshellv1surface.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandFullScreenShellV1Integration::QWaylandFullScreenShellV1Integration()
    : QWaylandShellIntegrationTemplate(1)
{
}

QWaylandFullScreenShellV1Integration::~QWaylandFullScreenShellV1Integration()
{
    if (isActive())
        release();
}

QWaylandShellSurface *QWaylandFullScreenShellV1Integration::createShellSurface(QWaylandWindow *window)
{
    return new QWaylandFullScreenShellV1Surface(this, window);
}

} // namespace QtWaylandClient

QT_END_NAMESPACE
