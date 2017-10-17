/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/QOpenGLContext>
#include <QtGui/QWindow>
#include <QtGui/QPainter>
#include <QtGui/QOffscreenSurface>
#include <qpa/qplatformbackingstore.h>
#include <private/qwindow_p.h>

#include "qopenglcompositorbackingstore_p.h"
#include "qopenglcompositor_p.h"

#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH              0x0CF2
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QOpenGLCompositorBackingStore
    \brief A backing store implementation for OpenGL
    \since 5.4
    \internal
    \ingroup qpa

    This implementation uploads raster-rendered widget windows into
    textures. It is meant to be used with QOpenGLCompositor that
    composites the textures onto a single native window using OpenGL.
    This means that multiple top-level widgets are supported without
    creating actual native windows for each of them.

    \note It is important to call notifyComposited() from the
    corresponding platform window's endCompositing() callback
    (inherited from QOpenGLCompositorWindow).

    \note When implementing QOpenGLCompositorWindow::textures() for
    windows of type RasterSurface or RasterGLSurface, simply return
    the list provided by this class' textures().
*/

QOpenGLCompositorBackingStore::QOpenGLCompositorBackingStore(QWindow *window)
    : QPlatformBackingStore(window),
      m_window(window),
      m_bsTexture(0),
      m_bsTextureContext(0),
      m_textures(new QPlatformTextureList),
      m_lockedWidgetTextures(0)
{
}

QOpenGLCompositorBackingStore::~QOpenGLCompositorBackingStore()
{
    if (m_bsTexture) {
        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        // With render-to-texture-widgets QWidget makes sure the TLW's shareContext() is
        // made current before destroying backingstores. That is however not the case for
        // windows with regular widgets only.
        QScopedPointer<QOffscreenSurface> tempSurface;
        if (!ctx) {
            ctx = QOpenGLCompositor::instance()->context();
            tempSurface.reset(new QOffscreenSurface);
            tempSurface->setFormat(ctx->format());
            tempSurface->create();
            ctx->makeCurrent(tempSurface.data());
        }

        if (m_bsTextureContext && ctx->shareGroup() == m_bsTextureContext->shareGroup())
            glDeleteTextures(1, &m_bsTexture);
        else
            qWarning("QOpenGLCompositorBackingStore: Texture is not valid in the current context");

        if (tempSurface)
            ctx->doneCurrent();
    }

    delete m_textures; // this does not actually own any GL resources
}

QPaintDevice *QOpenGLCompositorBackingStore::paintDevice()
{
    return &m_image;
}

void QOpenGLCompositorBackingStore::updateTexture()
{
    if (!m_bsTexture) {
        m_bsTextureContext = QOpenGLContext::currentContext();
        Q_ASSERT(m_bsTextureContext);
        glGenTextures(1, &m_bsTexture);
        glBindTexture(GL_TEXTURE_2D, m_bsTexture);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_image.width(), m_image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    } else {
        glBindTexture(GL_TEXTURE_2D, m_bsTexture);
    }

    if (!m_dirty.isNull()) {
        QRegion fixed;
        QRect imageRect = m_image.rect();

        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        if (!ctx->isOpenGLES() || ctx->format().majorVersion() >= 3) {
            for (const QRect &rect : m_dirty) {
                QRect r = imageRect & rect;
                glPixelStorei(GL_UNPACK_ROW_LENGTH, m_image.width());
                glTexSubImage2D(GL_TEXTURE_2D, 0, r.x(), r.y(), r.width(), r.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                                m_image.constScanLine(r.y()) + r.x() * 4);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            }
        } else {
            for (const QRect &rect : m_dirty) {
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
            for (const QRect &rect : fixed) {
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
        }

        m_dirty = QRegion();
    }
}

void QOpenGLCompositorBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    // Called for ordinary raster windows.

    Q_UNUSED(region);
    Q_UNUSED(offset);

    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    QOpenGLContext *dstCtx = compositor->context();
    Q_ASSERT(dstCtx);

    QWindow *dstWin = compositor->targetWindow();
    if (!dstWin)
        return;

    dstCtx->makeCurrent(dstWin);
    updateTexture();
    m_textures->clear();
    m_textures->appendTexture(nullptr, m_bsTexture, window->geometry());

    compositor->update();
}

void QOpenGLCompositorBackingStore::composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                                               QPlatformTextureList *textures,
                                               bool translucentBackground)
{
    // QOpenGLWidget/QQuickWidget content provided as textures. The raster content goes on top.

    Q_UNUSED(region);
    Q_UNUSED(offset);
    Q_UNUSED(translucentBackground);

    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    QOpenGLContext *dstCtx = compositor->context();
    Q_ASSERT(dstCtx); // setTarget() must have been called before, e.g. from QEGLFSWindow

    // The compositor's context and the context to which QOpenGLWidget/QQuickWidget
    // textures belong are not the same. They share resources, though.
    Q_ASSERT(qt_window_private(window)->shareContext()->shareGroup() == dstCtx->shareGroup());

    QWindow *dstWin = compositor->targetWindow();
    if (!dstWin)
        return;

    dstCtx->makeCurrent(dstWin);

    QWindowPrivate::get(window)->lastComposeTime.start();

    m_textures->clear();
    for (int i = 0; i < textures->count(); ++i)
        m_textures->appendTexture(textures->source(i), textures->textureId(i), textures->geometry(i),
                                  textures->clipRect(i), textures->flags(i));

    updateTexture();
    m_textures->appendTexture(nullptr, m_bsTexture, window->geometry());

    textures->lock(true);
    m_lockedWidgetTextures = textures;

    compositor->update();
}

void QOpenGLCompositorBackingStore::notifyComposited()
{
    if (m_lockedWidgetTextures) {
        QPlatformTextureList *textureList = m_lockedWidgetTextures;
        m_lockedWidgetTextures = 0; // may reenter so null before unlocking
        textureList->lock(false);
    }
}

void QOpenGLCompositorBackingStore::beginPaint(const QRegion &region)
{
    m_dirty |= region;

    if (m_image.hasAlphaChannel()) {
        QPainter p(&m_image);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        for (const QRect &r : region)
            p.fillRect(r, Qt::transparent);
    }
}

void QOpenGLCompositorBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);

    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    QOpenGLContext *dstCtx = compositor->context();
    QWindow *dstWin = compositor->targetWindow();
    if (!dstWin)
        return;

    m_image = QImage(size, QImage::Format_RGBA8888);

    m_window->create();

    dstCtx->makeCurrent(dstWin);
    if (m_bsTexture) {
        glDeleteTextures(1, &m_bsTexture);
        m_bsTexture = 0;
        m_bsTextureContext = nullptr;
    }
}

QImage QOpenGLCompositorBackingStore::toImage() const
{
    return m_image;
}

QT_END_NAMESPACE
