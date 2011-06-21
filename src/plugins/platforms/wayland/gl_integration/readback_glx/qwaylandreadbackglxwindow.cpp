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

#include <QtDebug>

#include "qwaylandreadbackglxwindow.h"
#include "qwaylandshmbackingstore.h"

QWaylandReadbackGlxWindow::QWaylandReadbackGlxWindow(QWindow *window, QWaylandReadbackGlxIntegration *glxIntegration)
    : QWaylandShmWindow(window)
    , m_glxIntegration(glxIntegration)
    , m_buffer(0)
    , m_pixmap(0)
    , m_config(0)
    , m_glxPixmap(0)
    , m_window(window)
{
}

QWaylandWindow::WindowType QWaylandReadbackGlxWindow::windowType() const
{
    //yeah. this type needs a new name
    return QWaylandWindow::Egl;
}

void QWaylandReadbackGlxWindow::setGeometry(const QRect &rect)
{
    QWaylandShmWindow::setGeometry(rect);

    if (m_pixmap) {
        delete mBuffer;
        //XFreePixmap deletes the glxPixmap as well
        XFreePixmap(m_glxIntegration->xDisplay(), m_pixmap);
        m_pixmap = 0;
    }
}

GLXPixmap QWaylandReadbackGlxWindow::glxPixmap() const
{
    if (!m_pixmap)
        const_cast<QWaylandReadbackGlxWindow *>(this)->createSurface();

    return m_glxPixmap;
}

uchar *QWaylandReadbackGlxWindow::buffer()
{
    return m_buffer->image()->bits();
}

QPlatformGLSurface *QWaylandReadbackGlxWindow::createGLSurface() const
{
    return new QWaylandReadbackGlxSurface(const_cast<QWaylandReadbackGlxWindow *>(this));
}

void QWaylandReadbackGlxWindow::createSurface()
{
    QSize size(geometry().size());
    if (size.isEmpty()) {
        //QGLWidget wants a context for a window without geometry
        size = QSize(1,1);
    }

    waitForFrameSync();

    m_buffer = new QWaylandShmBuffer(m_glxIntegration->waylandDisplay(), size, QImage::Format_ARGB32);
    attach(m_buffer);

    int depth = XDefaultDepth(m_glxIntegration->xDisplay(), m_glxIntegration->screen());
    m_pixmap = XCreatePixmap(m_glxIntegration->xDisplay(), m_glxIntegration->rootWindow(), size.width(), size.height(), depth);
    XSync(m_glxIntegration->xDisplay(), False);

    if (!m_config)
        m_config = qglx_findConfig(m_glxIntegration->xDisplay(), m_glxIntegration->screen(), m_window->glFormat());

    m_glxPixmap = glXCreatePixmap(m_glxIntegration->xDisplay(), m_config, m_pixmap,0);

    if (!m_glxPixmap)
        qDebug() << "Could not make glx surface out of pixmap :(";
}

