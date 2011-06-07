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

#include "qwaylandxcompositeglxwindow.h"
#include "qwaylandxcompositebuffer.h"

#include <QtCore/QDebug>

#include "wayland-xcomposite-client-protocol.h"
#include <QtGui/QRegion>

#include <X11/extensions/Xcomposite.h>


QWaylandXCompositeGLXWindow::QWaylandXCompositeGLXWindow(QWindow *window, QWaylandXCompositeGLXIntegration *glxIntegration)
    : QWaylandWindow(window)
    , m_glxIntegration(glxIntegration)
    , m_xWindow(0)
    , m_config(qglx_findConfig(glxIntegration->xDisplay(), glxIntegration->screen(), window->glFormat()))
    , m_buffer(0)
    , m_waitingForSync(false)
{
}

QWaylandWindow::WindowType QWaylandXCompositeGLXWindow::windowType() const
{
    //yeah. this type needs a new name
    return QWaylandWindow::Egl;
}

QPlatformGLSurface *QWaylandXCompositeGLXWindow::createGLSurface() const
{
    return new QWaylandXCompositeGLXSurface(const_cast<QWaylandXCompositeGLXWindow *>(this));
}

void QWaylandXCompositeGLXWindow::setGeometry(const QRect &rect)
{
    QWaylandWindow::setGeometry(rect);

    if (m_xWindow) {
        delete m_buffer;

        XDestroyWindow(m_glxIntegration->xDisplay(), m_xWindow);
        m_xWindow = 0;
    }
}

Window QWaylandXCompositeGLXWindow::xWindow() const
{
    if (!m_xWindow)
        const_cast<QWaylandXCompositeGLXWindow *>(this)->createSurface();

    return m_xWindow;
}

void QWaylandXCompositeGLXWindow::waitForSync()
{
    wl_display_sync_callback(m_glxIntegration->waylandDisplay()->wl_display(),
                             QWaylandXCompositeGLXWindow::sync_function,
                             this);
    m_waitingForSync= true;
    wl_display_sync(m_glxIntegration->waylandDisplay()->wl_display(), 0);
    m_glxIntegration->waylandDisplay()->flushRequests();
    while (m_waitingForSync)
        m_glxIntegration->waylandDisplay()->readEvents();
}


void QWaylandXCompositeGLXWindow::createSurface()
{
    QSize size(geometry().size());
    if (size.isEmpty()) {
        //QGLWidget wants a context for a window without geometry
        size = QSize(1,1);
    }

    XVisualInfo *visualInfo = glXGetVisualFromFBConfig(m_glxIntegration->xDisplay(), m_config);
    Colormap cmap = XCreateColormap(m_glxIntegration->xDisplay(), m_glxIntegration->rootWindow(),
            visualInfo->visual, AllocNone);

    XSetWindowAttributes a;
    a.background_pixel = WhitePixel(m_glxIntegration->xDisplay(), m_glxIntegration->screen());
    a.border_pixel = BlackPixel(m_glxIntegration->xDisplay(), m_glxIntegration->screen());
    a.colormap = cmap;
    m_xWindow = XCreateWindow(m_glxIntegration->xDisplay(), m_glxIntegration->rootWindow(),0, 0, size.width(), size.height(),
                              0, visualInfo->depth, InputOutput, visualInfo->visual,
                              CWBackPixel|CWBorderPixel|CWColormap, &a);

    XCompositeRedirectWindow(m_glxIntegration->xDisplay(), m_xWindow, CompositeRedirectManual);
    XMapWindow(m_glxIntegration->xDisplay(), m_xWindow);

    XSync(m_glxIntegration->xDisplay(), False);
    m_buffer = new QWaylandXCompositeBuffer(m_glxIntegration->waylandXComposite(),
                                            (uint32_t)m_xWindow,
                                            size,
                                            m_glxIntegration->waylandDisplay()->argbVisual());
    attach(m_buffer);
    waitForSync();
}

void QWaylandXCompositeGLXWindow::sync_function(void *data)
{
    QWaylandXCompositeGLXWindow *that = static_cast<QWaylandXCompositeGLXWindow *>(data);
    that->m_waitingForSync = false;
}

