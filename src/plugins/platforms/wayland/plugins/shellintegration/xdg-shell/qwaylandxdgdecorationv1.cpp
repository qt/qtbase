// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandxdgdecorationv1_p.h"
#include "qwaylandxdgshell_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandXdgDecorationManagerV1::QWaylandXdgDecorationManagerV1(wl_registry *registry, uint32_t id, uint32_t availableVersion)
    : QtWayland::zxdg_decoration_manager_v1(registry, id, qMin(availableVersion, 2u))
{
}

QWaylandXdgDecorationManagerV1::~QWaylandXdgDecorationManagerV1()
{
    Q_ASSERT(isInitialized());
    destroy();
}

QWaylandXdgToplevelDecorationV1 *QWaylandXdgDecorationManagerV1::createToplevelDecoration(::xdg_toplevel *toplevel)
{
    Q_ASSERT(toplevel);
    return new QWaylandXdgToplevelDecorationV1(get_toplevel_decoration(toplevel));
}

QWaylandXdgToplevelDecorationV1::QWaylandXdgToplevelDecorationV1(::zxdg_toplevel_decoration_v1 *decoration)
    : QtWayland::zxdg_toplevel_decoration_v1(decoration)
{
}

QWaylandXdgToplevelDecorationV1::~QWaylandXdgToplevelDecorationV1()
{
    Q_ASSERT(isInitialized());
    destroy();
}

void QWaylandXdgToplevelDecorationV1::requestMode(QtWayland::zxdg_toplevel_decoration_v1::mode mode)
{
    // According to the spec the client is responsible for not requesting a mode repeatedly.
    if (m_modeSet && m_requested == mode)
        return;

    set_mode(mode);
    m_requested = mode;
    m_modeSet = true;
}

void QWaylandXdgToplevelDecorationV1::unsetMode()
{
    unset_mode();
    m_modeSet = false;
    m_requested = mode_client_side;
}

QWaylandXdgToplevelDecorationV1::mode QWaylandXdgToplevelDecorationV1::pending() const
{
    return m_pending;
}

bool QWaylandXdgToplevelDecorationV1::isConfigured() const
{
    return m_configured;
}

void QtWaylandClient::QWaylandXdgToplevelDecorationV1::zxdg_toplevel_decoration_v1_configure(uint32_t mode)
{
    m_pending = zxdg_toplevel_decoration_v1::mode(mode);
    m_configured = true;
}

}

QT_END_NAMESPACE
