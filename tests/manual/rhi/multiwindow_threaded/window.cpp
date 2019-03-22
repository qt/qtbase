/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "window.h"
#include <QPlatformSurfaceEvent>

#ifndef QT_NO_OPENGL
#include <QtGui/private/qrhigles2_p.h>
#endif

#if QT_CONFIG(vulkan)
extern QVulkanInstance *instance;
#endif

Window::Window(const QString &title, GraphicsApi api)
{
    switch (api) {
    case OpenGL:
#if QT_CONFIG(opengl)
        setSurfaceType(OpenGLSurface);
        setFormat(QRhiGles2InitParams::adjustedFormat());
#endif
        break;
    case Vulkan:
#if QT_CONFIG(vulkan)
        setSurfaceType(VulkanSurface);
        setVulkanInstance(instance);
#endif
        break;
    case D3D11:
        setSurfaceType(OpenGLSurface); // not a typo
        break;
    case Metal:
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
        setSurfaceType(MetalSurface);
#endif
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
