// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "xdgshell.h"

namespace MockCompositor {

XdgWmBase::XdgWmBase(CoreCompositor *compositor, int version)
    : QtWaylandServer::xdg_wm_base(compositor->m_display, version)
    , m_compositor(compositor)
{
}

XdgToplevel *XdgWmBase::toplevel(int i)
{
    int j = 0;
    for (auto *xdgSurface : std::as_const(m_xdgSurfaces)) {
        if (auto *toplevel = xdgSurface->m_toplevel) {
            if (j == i)
                return toplevel;
            ++j;
        }
    }
    return nullptr;
}

XdgPopup *XdgWmBase::popup(int i)
{
    int j = 0;
    for (auto *xdgSurface : std::as_const(m_xdgSurfaces)) {
        if (auto *popup = xdgSurface->m_popup) {
            if (j == i)
                return popup;
            ++j;
        }
    }
    return nullptr;
}

void XdgWmBase::xdg_wm_base_get_xdg_surface(Resource *resource, uint32_t id, wl_resource *surface)
{
    auto *s = fromResource<Surface>(surface);
    auto *xdgSurface = new XdgSurface(this, s, resource->client(), id, resource->version());
    m_xdgSurfaces << xdgSurface;
    emit xdgSurfaceCreated(xdgSurface);
}

void XdgWmBase::xdg_wm_base_pong(Resource *resource, uint32_t serial)
{
    Q_UNUSED(resource);
    emit pong(serial);
}

XdgSurface::XdgSurface(XdgWmBase *xdgWmBase, Surface *surface, wl_client *client, int id, int version)
    : QtWaylandServer::xdg_surface(client, id, version)
    , m_xdgWmBase(xdgWmBase)
    , m_surface(surface)
{
    QVERIFY(!surface->m_pending.buffer);
    QVERIFY(!surface->m_committed.buffer);
    connect(this, &XdgSurface::toplevelCreated, xdgWmBase, &XdgWmBase::toplevelCreated, Qt::DirectConnection);
    connect(surface, &Surface::attach, this, [this]  (void *buffer) {
        if (buffer)
            verifyConfigured();
    });
    connect(surface, &Surface::commit, this, [this] {
        m_committed = m_pending;

        if (m_ackedConfigureSerial != m_committedConfigureSerial) {
            m_committedConfigureSerial = m_ackedConfigureSerial;
            emit configureCommitted(m_committedConfigureSerial);
        }
    });
    connect(surface, &Surface::bufferCommitted, this, [this] {
        if (m_surface->m_committed.buffer && (m_toplevel || m_popup)) {
            m_surface->map();
        } else {
            m_surface->unmap();
        }
    });
}

void XdgSurface::sendConfigure(uint serial)
{
    Q_ASSERT(serial);
    m_pendingConfigureSerials.append(serial);
    m_configureSent = true;
    xdg_surface::send_configure(serial);
}

uint XdgSurface::sendConfigure()
{
    const uint serial = m_xdgWmBase->m_compositor->nextSerial();
    sendConfigure(serial);
    return serial;
}

void XdgSurface::xdg_surface_get_toplevel(Resource *resource, uint32_t id)
{
    QVERIFY(!m_toplevel);
    QVERIFY(!m_popup);
    if (!m_surface->m_role) {
        m_surface->m_role = new XdgToplevelRole;
    } else if (!qobject_cast<XdgToplevelRole *>(m_surface->m_role)) {
        QFAIL(QByteArrayLiteral("surface already has role") + m_surface->m_role->metaObject()->className());
        return;
    }
    m_toplevel = new XdgToplevel(this, id, resource->version());
    emit toplevelCreated(m_toplevel);
}

void XdgSurface::xdg_surface_get_popup(Resource *resource, uint32_t id, wl_resource *parent, wl_resource *positioner)
{
    QVERIFY(!m_toplevel);
    QVERIFY(!m_popup);
    if (!m_surface->m_role) {
        m_surface->m_role = new XdgPopupRole;
    } else if (!qobject_cast<XdgPopupRole *>(m_surface->m_role)) {
        qWarning() << "surface already has role" << m_surface->m_role->metaObject()->className();
        return;
    }
    auto *p = fromResource<XdgSurface>(parent);
    XdgPositionerState positionerState;
    if (auto *pos = fromResource<XdgPositioner>(positioner))
        positionerState = pos->m_state;
    m_popup = new XdgPopup(this, p, positionerState, id, resource->version());
}

void XdgSurface::xdg_surface_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    bool removed = m_xdgWmBase->m_xdgSurfaces.removeOne(this);
    Q_ASSERT(removed);
    delete this;
}

void XdgSurface::xdg_surface_destroy(QtWaylandServer::xdg_surface::Resource *resource)
{
    QVERIFY(m_popups.empty());
    wl_resource_destroy(resource->handle);
}

void XdgSurface::xdg_surface_set_window_geometry(Resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
    Q_UNUSED(resource);
    QRect rect(x, y, width, height);
    QVERIFY(rect.isValid());
    m_pending.windowGeometry = rect;
}

void XdgSurface::xdg_surface_ack_configure(Resource *resource, uint32_t serial)
{
    Q_UNUSED(resource);
    QVERIFY2(m_pendingConfigureSerials.contains(serial), qPrintable(QString::number(serial)));
    m_ackedConfigureSerial = serial;
    while (!m_pendingConfigureSerials.empty()) {
        uint s = m_pendingConfigureSerials.takeFirst();
        if (s == serial)
            return;
    }
}

XdgToplevel::XdgToplevel(XdgSurface *xdgSurface, int id, int version)
    : QtWaylandServer::xdg_toplevel(xdgSurface->resource()->client(), id, version)
    , m_xdgSurface(xdgSurface)
{
    connect(surface(), &Surface::commit, this, [this] { m_committed = m_pending; });
}

void XdgToplevel::sendConfigureBounds(const QSize &size)
{
    send_configure_bounds(size.width(), size.height());
}

void XdgToplevel::sendConfigure(const QSize &size, const QList<uint> &states)
{
    send_configure(size.width(), size.height(), toByteArray(states));
}

uint XdgToplevel::sendCompleteConfigure(const QSize &size, const QList<uint> &states)
{
    sendConfigure(size, states);
    return m_xdgSurface->sendConfigure();
}

void XdgToplevel::xdg_toplevel_set_max_size(Resource *resource, int32_t width, int32_t height)
{
    Q_UNUSED(resource);
    QSize size(width, height);
    QVERIFY(size.isValid());
    m_pending.maxSize = size;
}

void XdgToplevel::xdg_toplevel_set_min_size(Resource *resource, int32_t width, int32_t height)
{
    Q_UNUSED(resource);
    QSize size(width, height);
    QVERIFY(size.isValid());
    m_pending.minSize = size;
}

QPoint XdgPositionerState::unconstrainedPosition() const
{
    int x = 0;
    switch (anchor) {
    case QtWaylandServer::xdg_positioner::anchor_left:
    case QtWaylandServer::xdg_positioner::anchor_top_left:
    case QtWaylandServer::xdg_positioner::anchor_bottom_left:
        x = anchorRect.x();
        break;
    case QtWaylandServer::xdg_positioner::anchor_right:
    case QtWaylandServer::xdg_positioner::anchor_top_right:
    case QtWaylandServer::xdg_positioner::anchor_bottom_right:
        x = anchorRect.x() + anchorRect.width();
        break;
    default:
        x = anchorRect.x() + anchorRect.width() / 2;
        break;
    }

    int y = 0;
    switch (anchor) {
    case QtWaylandServer::xdg_positioner::anchor_top:
    case QtWaylandServer::xdg_positioner::anchor_top_left:
    case QtWaylandServer::xdg_positioner::anchor_top_right:
        y = anchorRect.y();
        break;
    case QtWaylandServer::xdg_positioner::anchor_bottom:
    case QtWaylandServer::xdg_positioner::anchor_bottom_left:
    case QtWaylandServer::xdg_positioner::anchor_bottom_right:
        y = anchorRect.y() + anchorRect.height();
        break;
    default:
        y = anchorRect.y() + anchorRect.height() / 2;
        break;
    }

    // The gravity says in which direction the popup grows away from the anchor point.
    switch (gravity) {
    case QtWaylandServer::xdg_positioner::gravity_left:
    case QtWaylandServer::xdg_positioner::gravity_top_left:
    case QtWaylandServer::xdg_positioner::gravity_bottom_left:
        x -= size.width();
        break;
    case QtWaylandServer::xdg_positioner::gravity_right:
    case QtWaylandServer::xdg_positioner::gravity_top_right:
    case QtWaylandServer::xdg_positioner::gravity_bottom_right:
        break;
    default:
        x -= size.width() / 2;
        break;
    }

    switch (gravity) {
    case QtWaylandServer::xdg_positioner::gravity_top:
    case QtWaylandServer::xdg_positioner::gravity_top_left:
    case QtWaylandServer::xdg_positioner::gravity_top_right:
        y -= size.height();
        break;
    case QtWaylandServer::xdg_positioner::gravity_bottom:
    case QtWaylandServer::xdg_positioner::gravity_bottom_left:
    case QtWaylandServer::xdg_positioner::gravity_bottom_right:
        break;
    default:
        y -= size.height() / 2;
        break;
    }

    return QPoint(x, y) + offset;
}

XdgPopup::XdgPopup(XdgSurface *xdgSurface, XdgSurface *parent, const XdgPositionerState &positioner,
                   int id, int version)
    : QtWaylandServer::xdg_popup(xdgSurface->resource()->client(), id, version)
    , m_xdgSurface(xdgSurface)
    , m_parentXdgSurface(parent)
    , m_positioner(positioner)
{
    Q_ASSERT(m_xdgSurface);
    Q_ASSERT(m_parentXdgSurface);
    m_parentXdgSurface->m_popups << this;
}

void XdgPopup::sendConfigure(const QRect &geometry)
{
    send_configure(geometry.x(), geometry.y(), geometry.width(), geometry.height());
}

uint XdgPopup::sendCompleteConfigure(const QRect &geometry)
{
    sendConfigure(geometry);
    return m_xdgSurface->sendConfigure();
}

void XdgPopup::xdg_popup_grab(QtWaylandServer::xdg_popup::Resource *resource, wl_resource *seat, uint32_t serial)
{
    Q_UNUSED(resource);
    Q_UNUSED(seat); // TODO: verify correct seat as well
    QVERIFY(!m_grabbed);
    QVERIFY(m_parentXdgSurface->isValidPopupGrabParent());
    m_xdgSurface->m_xdgWmBase->m_topmostGrabbingPopup = this;
    m_grabbed = true;
    m_grabSerial = serial;
}

void XdgPopup::xdg_popup_reposition(Resource *resource, wl_resource *positioner, uint32_t token)
{
    Q_UNUSED(resource);
    if (auto *pos = fromResource<XdgPositioner>(positioner))
        m_positioner = pos->m_state;
    m_repositionToken = token;
    emit repositioned();
}

void XdgPopup::xdg_popup_destroy(Resource *resource) {
    Q_UNUSED(resource);
    if (m_grabbed) {
        auto *base = m_xdgSurface->m_xdgWmBase;
        QCOMPARE(base->m_topmostGrabbingPopup, this);
        base->m_topmostGrabbingPopup = this->m_parentXdgSurface->m_popup;
    }
    m_xdgSurface->m_popup = nullptr;
    m_parentXdgSurface->m_popups.removeAll(this);
    emit destroyRequested();
}

} // namespace MockCompositor
