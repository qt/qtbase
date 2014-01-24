/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLContext>

#include "qeglplatformbackingstore_p.h"
#include "qeglcompositor_p.h"
#include "qeglplatformwindow_p.h"
#include "qeglplatformscreen_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QEGLPlatformBackingStore
    \brief A backing store implementation for EGL and GLES.
    \since 5.2
    \internal
    \ingroup qpa

    This implementation uploads raster-rendered widget windows into
    textures and composites them onto a single native window using
    QEGLCompositor. This means that multiple top-level widgets are
    supported without creating actual native windows for each of them.

    The class is ready to be used as-is, the default
    QEGLPlatformIntegration::createPlatformBackingStore()
    implementation creates an instance which is ready to be used
    without further customization.

    If QEGLCompositor is not suitable, this backing store
    implementation can also be used without it. In this case a
    subclass must reimplement composite() and schedule an update in
    its custom compositor when this function is called. The textures
    are accessible via QEGLPlatformWindow::texture().
*/

QEGLPlatformBackingStore::QEGLPlatformBackingStore(QWindow *window)
    : QPlatformBackingStore(window),
      m_window(static_cast<QEGLPlatformWindow *>(window->handle())),
      m_texture(0)
{
    m_window->setBackingStore(this);
}

QPaintDevice *QEGLPlatformBackingStore::paintDevice()
{
    return &m_image;
}

void QEGLPlatformBackingStore::updateTexture()
{
    glBindTexture(GL_TEXTURE_2D, m_texture);

    if (!m_dirty.isNull()) {
        QRegion fixed;
        QRect imageRect = m_image.rect();

        foreach (const QRect &rect, m_dirty.rects()) {
            // intersect with image rect to be sure
            QRect r = imageRect & rect;

            // if the rect is wide enough it's cheaper to just
            // extend it instead of doing an image copy
            if (r.width() >= imageRect.width() / 2) {
                r.setX(0);
                r.setWidth(imageRect.width());
            }

            fixed |= r;
        }

        foreach (const QRect &rect, fixed.rects()) {
            // if the sub-rect is full-width we can pass the image data directly to
            // OpenGL instead of copying, since there's no gap between scanlines
            if (rect.width() == imageRect.width()) {
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                                m_image.constScanLine(rect.y()));
            } else {
                glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                    m_image.copy(rect).constBits());
            }
        }

        m_dirty = QRegion();
    }
}

void QEGLPlatformBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    Q_UNUSED(region);
    Q_UNUSED(offset);

#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglBackingStore::flush %p", window);
#endif

    QEGLPlatformScreen *screen = static_cast<QEGLPlatformScreen *>(m_window->screen());
    QEGLPlatformWindow *dstWin = screen->compositingWindow();
    if (!dstWin || !dstWin->isRaster())
        return;

    m_window->create();
    QOpenGLContext *context = screen->compositingContext();
    context->makeCurrent(dstWin->window());
    updateTexture();
    composite(context, dstWin);
}

void QEGLPlatformBackingStore::composite(QOpenGLContext *context, QEGLPlatformWindow *window)
{
    QEGLCompositor::instance()->schedule(context, window);
}

void QEGLPlatformBackingStore::beginPaint(const QRegion &rgn)
{
    m_dirty |= rgn;
}

void QEGLPlatformBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);

    QEGLPlatformScreen *screen = static_cast<QEGLPlatformScreen *>(m_window->screen());
    QEGLPlatformWindow *dstWin = screen->compositingWindow();
    if (!dstWin || !dstWin->isRaster())
        return;

    m_image = QImage(size, QImage::Format_RGB32);
    m_window->create();

    screen->compositingContext()->makeCurrent(dstWin->window());

    if (m_texture)
        glDeleteTextures(1, &m_texture);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width(), size.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
}

QT_END_NAMESPACE
