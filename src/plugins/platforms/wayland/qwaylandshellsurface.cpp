// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandshellsurface_p.h"
#include "qwaylandwindow_p.h"
#include "qwaylandinputdevice_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandShellSurface::QWaylandShellSurface(QWaylandWindow *window)
                    : m_window(window)
{
}

void QWaylandShellSurface::setWindowFlags(Qt::WindowFlags flags)
{
    Q_UNUSED(flags);
}

void QWaylandShellSurface::sendProperty(const QString &name, const QVariant &value)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
}

QPlatformWindow *QWaylandShellSurface::platformWindow()
{
    return m_window;
}

wl_surface *QWaylandShellSurface::wlSurface()
{
    return m_window ? m_window->wlSurface() : nullptr;
}

void QWaylandShellSurface::setWindowGeometry(const QRect &rect)
{
    setWindowPosition(rect.topLeft());
    setWindowSize(rect.size());
}

void QWaylandShellSurface::resizeFromApplyConfigure(const QSize &sizeWithMargins, const QPoint &offset)
{
    m_window->resizeFromApplyConfigure(sizeWithMargins, offset);
}

void QWaylandShellSurface::repositionFromApplyConfigure(const QPoint &position)
{
    m_window->repositionFromApplyConfigure(position);
}

void QWaylandShellSurface::setGeometryFromApplyConfigure(const QPoint &globalPosition, const QSize &sizeWithMargins)
{
    m_window->setGeometryFromApplyConfigure(globalPosition, sizeWithMargins);
}

void QWaylandShellSurface::applyConfigureWhenPossible()
{
    m_window->applyConfigureWhenPossible();
}

void QWaylandShellSurface::handleActivationChanged(bool activated)
{
    if (activated)
        m_window->display()->handleWindowActivated(m_window);
    else
        m_window->display()->handleWindowDeactivated(m_window);
}

uint32_t QWaylandShellSurface::getSerial(QWaylandInputDevice *inputDevice)
{
    return inputDevice->serial();
}

void QWaylandShellSurface::setXdgActivationToken(const QString &token)
{
    Q_UNUSED(token);
    qCWarning(lcQpaWayland) << "setXdgActivationToken not implemented" << token;
}

void QWaylandShellSurface::requestXdgActivationToken(quint32 serial)
{
    Q_UNUSED(serial);
    Q_EMIT m_window->xdgActivationTokenCreated({});
}

/*!
    \internal
    Determines whether the client should commit the surface with no buffer
    after creating the role and performing initial setup
*/
bool QWaylandShellSurface::commitSurfaceRole() const
{
    return true;
}

}

QT_END_NAMESPACE

#include "moc_qwaylandshellsurface_p.cpp"
