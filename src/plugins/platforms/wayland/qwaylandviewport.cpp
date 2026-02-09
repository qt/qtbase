// Copyright (C) 2022 David Edmundson <davidedmundson@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandviewport_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandViewport::QWaylandViewport(::wp_viewport *viewport)
    : QtWayland::wp_viewport(viewport)
{
}

QWaylandViewport::~QWaylandViewport()
{
    destroy();
}

void QWaylandViewport::setSource(const QRectF &source)
{
    set_source(wl_fixed_from_double(source.x()),
               wl_fixed_from_double(source.y()),
               wl_fixed_from_double(source.width()),
               wl_fixed_from_double(source.height()));
}

void QWaylandViewport::setDestination(const QSize &destination)
{
    set_destination(destination.width(), destination.height());
}

}

QT_END_NAMESPACE
