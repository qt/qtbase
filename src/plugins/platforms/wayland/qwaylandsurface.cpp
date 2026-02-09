// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandsurface_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandscreen_p.h"

#include <QtGui/QGuiApplication>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandSurface::QWaylandSurface(QWaylandDisplay *display)
    : wl_surface(display->createSurface(this))
{
    connect(qApp, &QGuiApplication::screenRemoved, this, &QWaylandSurface::handleScreenRemoved);
    connect(qApp, &QGuiApplication::screenAdded, this, &QWaylandSurface::screensChanged);
}

QWaylandSurface::~QWaylandSurface()
{
    destroy();
}

QWaylandScreen *QWaylandSurface::oldestEnteredScreen()
{
    for (auto *screen : std::as_const(m_screens)) {
        // only report valid screens
        // we can have some ouptuts waiting for xdg output information
        // that are valid QPlatformScreens, but not valid QScreens
        if (screen->screen())
            return screen;
    }
    return nullptr;
}

QWaylandSurface *QWaylandSurface::fromWlSurface(::wl_surface *surface)
{
    if (auto *s = QtWayland::wl_surface::fromObject(surface))
        return static_cast<QWaylandSurface *>(s);
    return nullptr;
}

void QWaylandSurface::handleScreenRemoved(QScreen *qScreen)
{
    auto *platformScreen = qScreen->handle();
    if (platformScreen->isPlaceholder())
        return;

    auto *waylandScreen = static_cast<QWaylandScreen *>(qScreen->handle());
    if (m_screens.removeOne(waylandScreen))
        emit screensChanged();
}

void QWaylandSurface::surface_enter(wl_output *output)
{
    auto addedScreen = QWaylandScreen::fromWlOutput(output);

    if (!addedScreen)
        return;

    if (m_screens.contains(addedScreen)) {
        qCWarning(lcQpaWayland)
                << "Ignoring unexpected wl_surface.enter received for output with id:"
                << wl_proxy_get_id(reinterpret_cast<wl_proxy *>(output))
                << "screen name:" << addedScreen->name() << "screen model:" << addedScreen->model()
                << "This is most likely a bug in the compositor.";
        return;
    }

    m_screens.append(addedScreen);
    emit screensChanged();
}

void QWaylandSurface::surface_leave(wl_output *output)
{
    auto *removedScreen = QWaylandScreen::fromWlOutput(output);

    if (!removedScreen)
        return;

    bool wasRemoved = m_screens.removeOne(removedScreen);
    if (!wasRemoved) {
        qCWarning(lcQpaWayland)
                << "Ignoring unexpected wl_surface.leave received for output with id:"
                << wl_proxy_get_id(reinterpret_cast<wl_proxy *>(output))
                << "screen name:" << removedScreen->name()
                << "screen model:" << removedScreen->model()
                << "This is most likely a bug in the compositor.";
        return;
    }
    emit screensChanged();
}

void QWaylandSurface::surface_preferred_buffer_scale(int32_t scale)
{
    if (m_preferredBufferScale == scale)
        return;
    m_preferredBufferScale = scale;
    Q_EMIT preferredBufferScaleChanged();
}

void QWaylandSurface::surface_preferred_buffer_transform(uint32_t transform)
{
    if (m_preferredBufferTransform == transform)
        return;
    m_preferredBufferTransform = static_cast<wl_output_transform>(transform);
    Q_EMIT preferredBufferTransformChanged();
}

} // namespace QtWaylandClient

QT_END_NAMESPACE

#include "moc_qwaylandsurface_p.cpp"
