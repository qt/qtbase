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

#include "qwaylandreadbackcglwindow.h"
#include "qwaylandshmbackingstore.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/glext.h>

QWaylandReadbackCGLWindow::QWaylandReadbackCGLWindow(QWindow *window, QWaylandReadbackCGLIntegration *cglIntegration)
    : QWaylandShmWindow(window)
    , m_CglIntegration(cglIntegration)
    , mContext(0)
    , m_buffer(0)
    , m_pixelBuffer(0)
{
}

QWaylandWindow::WindowType QWaylandReadbackCGLWindow::windowType() const
{
    //yeah. this type needs a new name
    return QWaylandWindow::Egl;
}


void QWaylandReadbackCGLWindow::setGeometry(const QRect &rect)
{
    QWaylandShmWindow::setGeometry(rect);

    if (m_buffer) {
        delete m_buffer;
        m_buffer = 0;

        CGLDestroyPBuffer(m_pixelBuffer);
        m_pixelBuffer = 0;
    }
}

CGLPBufferObj QWaylandReadbackCGLWindow::pixelBuffer()
{
    if (!m_pixelBuffer)
        createSurface();

    return m_pixelBuffer;
}

uchar *QWaylandReadbackCGLWindow::buffer()
{
    return m_buffer->image()->bits();
}

void QWaylandReadbackCGLWindow::createSurface()
{
    QSize size(geometry().size());
    if (size.isEmpty()) {
        //QGLWidget wants a context for a window without geometry
        size = QSize(1,1);
    }

    waitForFrameSync();

    CGLCreatePBuffer(size.width(), size.height(), GL_TEXTURE_RECTANGLE_ARB, GL_BGRA, 0, &m_pixelBuffer);

    delete m_buffer;
    m_buffer = new QWaylandShmBuffer(m_CglIntegration->waylandDisplay(),size,QImage::Format_ARGB32);
    attach(m_buffer);
}

