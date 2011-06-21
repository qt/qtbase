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

#include "qwaylandreadbackglxcontext.h"

#include "qwaylandshmbackingstore.h"
#include "qwaylandreadbackglxwindow.h"

#include <QtGui/QGuiGLContext>
#include <QtCore/QDebug>

static inline void qgl_byteSwapImage(QImage &img, GLenum pixel_type)
{
    const int width = img.width();
    const int height = img.height();

    if (pixel_type == GL_UNSIGNED_INT_8_8_8_8_REV
        || (pixel_type == GL_UNSIGNED_BYTE && QSysInfo::ByteOrder == QSysInfo::LittleEndian))
    {
        for (int i = 0; i < height; ++i) {
            uint *p = (uint *) img.scanLine(i);
            for (int x = 0; x < width; ++x)
                p[x] = ((p[x] << 16) & 0xff0000) | ((p[x] >> 16) & 0xff) | (p[x] & 0xff00ff00);
        }
    } else {
        for (int i = 0; i < height; ++i) {
            uint *p = (uint *) img.scanLine(i);
            for (int x = 0; x < width; ++x)
                p[x] = (p[x] << 8) | ((p[x] >> 24) & 0xff);
        }
    }
}

QWaylandReadbackGlxContext::QWaylandReadbackGlxContext(const QSurfaceFormat &format,
        QPlatformGLContext *share, Display *display, int screen)
    : m_display(display)
{
    GLXFBConfig config = qglx_findConfig(display, screen, format, GLX_PIXMAP_BIT);

    GLXContext shareContext = share ? static_cast<QWaylandReadbackGlxContext *>(share)->m_context : 0;

    XVisualInfo *visualInfo = glXGetVisualFromFBConfig(display, config);
    m_context = glXCreateContext(display, visualInfo, shareContext, TRUE);
    m_format = qglx_surfaceFormatFromGLXFBConfig(display, config, m_context);
}

QSurfaceFormat QWaylandReadbackGlxContext::format() const
{
    return m_format;
}

bool QWaylandReadbackGlxContext::makeCurrent(QPlatformSurface *surface)
{
    GLXPixmap glxPixmap = static_cast<QWaylandReadbackGlxWindow *>(surface)->glxPixmap();

    return glXMakeCurrent(m_display, glxPixmap, m_context);
}

void QWaylandReadbackGlxContext::doneCurrent()
{
    glXMakeCurrent(m_display, 0, 0);
}

void QWaylandReadbackGlxContext::swapBuffers(QPlatformSurface *surface)
{
    // #### makeCurrent() directly on the platform context doesn't update QGuiGLContext::currentContext()
    if (QGuiGLContext::currentContext()->handle() != this)
        makeCurrent(surface);

    QWaylandReadbackGlxWindow *w = static_cast<QWaylandReadbackGlxWindow *>(surface);

    QSize size = w->geometry().size();

    QImage img(size, QImage::Format_ARGB32);
    const uchar *constBits = img.bits();
    void *pixels = const_cast<uchar *>(constBits);

    glReadPixels(0, 0, size.width(), size.height(), GL_RGBA,GL_UNSIGNED_BYTE, pixels);

    img = img.mirrored();
    qgl_byteSwapImage(img, GL_UNSIGNED_INT_8_8_8_8_REV);
    constBits = img.bits();

    const uchar *constDstBits = w->buffer();
    uchar *dstBits = const_cast<uchar *>(constDstBits);
    memcpy(dstBits, constBits, (img.width() * 4) * img.height());

    w->damage(QRect(QPoint(), size));

    w->waitForFrameSync();
}

void (*QWaylandReadbackGlxContext::getProcAddress(const QByteArray &procName)) ()
{
    return glXGetProcAddress(reinterpret_cast<const GLubyte *>(procName.constData()));
}
