// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDXDGDECORATIONV1_P_H
#define QWAYLANDXDGDECORATIONV1_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qwayland-xdg-decoration-unstable-v1.h"

#include <QtWaylandClient/qtwaylandclientglobal.h>

QT_BEGIN_NAMESPACE

class QWindow;

namespace QtWaylandClient {

class QWaylandXdgToplevel;
class QWaylandXdgToplevelDecorationV1;

class Q_WAYLANDCLIENT_EXPORT QWaylandXdgDecorationManagerV1 : public QtWayland::zxdg_decoration_manager_v1
{
public:
    QWaylandXdgDecorationManagerV1(struct ::wl_registry *registry, uint32_t id, uint32_t availableVersion);
    ~QWaylandXdgDecorationManagerV1() override;
    QWaylandXdgToplevelDecorationV1 *createToplevelDecoration(::xdg_toplevel *toplevel);
};

class Q_WAYLANDCLIENT_EXPORT QWaylandXdgToplevelDecorationV1 : public QtWayland::zxdg_toplevel_decoration_v1
{
public:
    QWaylandXdgToplevelDecorationV1(::zxdg_toplevel_decoration_v1 *decoration);
    ~QWaylandXdgToplevelDecorationV1() override;
    void requestMode(mode mode);
    void unsetMode();
    mode pending() const;
    bool isConfigured() const;

protected:
    void zxdg_toplevel_decoration_v1_configure(uint32_t mode) override;

private:
    mode m_pending = mode_client_side;
    mode m_requested = mode_client_side;
    bool m_modeSet = false;
    bool m_configured = false;
};

} // namespace QtWaylandClient

QT_END_NAMESPACE
#endif // QWAYLANDXDGDECORATIONV1_P_H
