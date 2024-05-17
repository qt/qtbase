// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "window.h"
#include <QPlatformSurfaceEvent>

#if QT_CONFIG(vulkan)
extern QVulkanInstance *instance;
#endif

Window::Window(const QString &title, GraphicsApi api)
{
    switch (api) {
    case OpenGL:
        setSurfaceType(OpenGLSurface);
        break;
    case Vulkan:
        setSurfaceType(VulkanSurface);
#if QT_CONFIG(vulkan)
        setVulkanInstance(instance);
#endif
        break;
    case D3D11:
    case D3D12:
        setSurfaceType(Direct3DSurface);
        break;
    case Metal:
        setSurfaceType(MetalSurface);
        break;
    default:
        break;
    }

    resize(800, 600);
    setTitle(title);
}

Window::~Window()
{
}

void Window::exposeEvent(QExposeEvent *)
{
    if (isExposed()) {
        if (!m_running) {
            // initialize and start rendering when the window becomes usable for graphics purposes
            m_running = true;
            m_notExposed = false;
            emit initRequested();
            emit renderRequested(true);
        } else {
            // continue when exposed again
            if (m_notExposed) {
                m_notExposed = false;
                emit renderRequested(true);
            } else {
                // resize generates exposes - this is very important here (unlike in a single-threaded renderer)
                emit syncSurfaceSizeRequested();
            }
        }
    } else {
        // stop pushing frames when not exposed (on some platforms this is essential, optional on others)
        if (m_running)
            m_notExposed = true;
    }
}

bool Window::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::UpdateRequest:
        if (!m_notExposed)
            emit renderRequested(false);
        break;

    case QEvent::PlatformSurface:
        // this is the proper time to tear down the swapchain (while the native window and surface are still around)
        if (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            emit surfaceGoingAway();
        break;

    default:
        break;
    }

    return QWindow::event(e);
}
