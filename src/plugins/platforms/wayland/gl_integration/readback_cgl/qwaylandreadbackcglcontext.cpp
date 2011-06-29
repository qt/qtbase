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

#include "qwaylandreadbackcglcontext.h"

#include "qwaylandshmbackingstore.h"
#include "qwaylandreadbackcglwindow.h"

#include <QtGui/QGuiGLContext>
#include <QtCore/QDebug>

#include <OpenGL/OpenGL.h>
#include <OpenGL/glext.h>
#include <OpenGL/glu.h>

#include <QtPlatformSupport/private/cglconvenience_p.h>

QWaylandReadbackCGLContext::QWaylandReadbackCGLContext(QPlatformGLContext *share)
    : QPlatformGLContext()
{
    Q_UNUSED(share);
    m_glContext = qcgl_createGlContext();
}

QSurfaceFormat QWaylandReadbackCGLContext::format() const
{
    return qcgl_surfaceFormat();
}

bool QWaylandReadbackCGLContext::makeCurrent(QPlatformSurface *surface)
{
    QWaylandReadbackCGLWindow *window = static_cast<QWaylandReadbackCGLWindow *>(surface);
    CGLSetPBuffer(m_glContext, window->pixelBuffer(), 0, 0, 0);
    CGLSetCurrentContext(m_glContext);
    return true;
}

void QWaylandReadbackCGLContext::doneCurrent()
{
    CGLSetCurrentContext(0);
}

void QWaylandReadbackCGLContext::swapBuffers(QPlatformSurface *surface)
{
    Q_UNUSED(surface);

    if (QGuiGLContext::currentContext()->handle() != this) {
        makeCurrent(surface);
    }
    CGLFlushDrawable(m_glContext);

    QWaylandReadbackCGLWindow *window = static_cast<QWaylandReadbackCGLWindow *>(surface);
    QSize size = window->geometry().size();

    uchar *dstBits = const_cast<uchar *>(window->buffer());
    glReadPixels(0,0, size.width(), size.height(), GL_BGRA,GL_UNSIGNED_BYTE, dstBits);

    window->damage(QRect(QPoint(0,0),size));

    // ### Should sync here but this call deadlocks with the server.
    //window->waitForFrameSync();
}

void (*QWaylandReadbackCGLContext::getProcAddress(const QByteArray &procName)) ()
{
    return qcgl_getProcAddress(procName);
}

