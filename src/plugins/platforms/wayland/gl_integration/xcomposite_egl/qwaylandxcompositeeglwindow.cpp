/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwaylandxcompositeeglwindow.h"
#include "qwaylandxcompositebuffer.h"

#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <QtPlatformSupport/private/qxlibeglintegration_p.h>

#include "wayland-xcomposite-client-protocol.h"

#include <X11/extensions/Xcomposite.h>

#include <QtCore/QDebug>

QWaylandXCompositeEGLWindow::QWaylandXCompositeEGLWindow(QWindow *window, QWaylandXCompositeEGLIntegration *glxIntegration)
    : QWaylandWindow(window)
    , m_glxIntegration(glxIntegration)
    , m_context(0)
    , m_buffer(0)
    , m_xWindow(0)
    , m_config(q_configFromGLFormat(glxIntegration->eglDisplay(), window->glFormat(), true))
    , m_surface(0)
    , m_waitingForSync(false)
{
}

QWaylandWindow::WindowType QWaylandXCompositeEGLWindow::windowType() const
{
    //yeah. this type needs a new name
    return QWaylandWindow::Egl;
}

QPlatformGLSurface *QWaylandXCompositeEGLWindow::createGLSurface() const
{
    return new QWaylandXCompositeEGLSurface(const_cast<QWaylandXCompositeEGLWindow *>(this));
}

void QWaylandXCompositeEGLWindow::setGeometry(const QRect &rect)
{
    QWaylandWindow::setGeometry(rect);

    if (m_surface) {
        eglDestroySurface(m_glxIntegration->eglDisplay(), m_surface);
        m_surface = 0;
    }
}

EGLSurface QWaylandXCompositeEGLWindow::eglSurface() const
{
    if (!m_surface)
        const_cast<QWaylandXCompositeEGLWindow *>(this)->createEglSurface();
    return m_surface;
}

void QWaylandXCompositeEGLWindow::createEglSurface()
{
    QSize size(geometry().size());
    if (size.isEmpty()) {
        // QGLWidget wants a context for a window without geometry
        size = QSize(1,1);
    }

    delete m_buffer;
    //XFreePixmap deletes the glxPixmap as well
    if (m_xWindow) {
        XDestroyWindow(m_glxIntegration->xDisplay(), m_xWindow);
    }

    VisualID visualId = QXlibEglIntegration::getCompatibleVisualId(m_glxIntegration->xDisplay(), m_glxIntegration->eglDisplay(), m_config);

    XVisualInfo visualInfoTemplate;
    memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));
    visualInfoTemplate.visualid = visualId;

    int matchingCount = 0;
    XVisualInfo *visualInfo = XGetVisualInfo(m_glxIntegration->xDisplay(), VisualIDMask, &visualInfoTemplate, &matchingCount);

    Colormap cmap = XCreateColormap(m_glxIntegration->xDisplay(),m_glxIntegration->rootWindow(),visualInfo->visual,AllocNone);

    XSetWindowAttributes a;
    a.colormap = cmap;
    m_xWindow = XCreateWindow(m_glxIntegration->xDisplay(), m_glxIntegration->rootWindow(),0, 0, size.width(), size.height(),
                             0, visualInfo->depth, InputOutput, visualInfo->visual,
                             CWColormap, &a);

    XCompositeRedirectWindow(m_glxIntegration->xDisplay(), m_xWindow, CompositeRedirectManual);
    XMapWindow(m_glxIntegration->xDisplay(), m_xWindow);

    m_surface = eglCreateWindowSurface(m_glxIntegration->eglDisplay(), m_config, m_xWindow,0);
    if (m_surface == EGL_NO_SURFACE) {
        qFatal("Could not make eglsurface");
    }

    XSync(m_glxIntegration->xDisplay(),False);
    mBuffer = new QWaylandXCompositeBuffer(m_glxIntegration->waylandXComposite(),
                                           (uint32_t)m_xWindow,
                                           size,
                                           m_glxIntegration->waylandDisplay()->argbVisual());
    attach(m_buffer);
    wl_display_sync_callback(m_glxIntegration->waylandDisplay()->wl_display(),
                             QWaylandXCompositeEGLWindow::sync_function,
                             this);

    m_waitingForSync = true;
    wl_display_sync(m_glxIntegration->waylandDisplay()->wl_display(),0);
    m_glxIntegration->waylandDisplay()->flushRequests();
    while (m_waitingForSync)
        m_glxIntegration->waylandDisplay()->readEvents();
}

void QWaylandXCompositeEGLWindow::sync_function(void *data)
{
    QWaylandXCompositeEGLWindow *that = static_cast<QWaylandXCompositeEGLWindow *>(data);
    that->m_waitingForSync = false;
}
