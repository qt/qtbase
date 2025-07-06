// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/QOpenGLContext>
#include <QtGui/QWindow>
#include <QtGui/QPainter>
#include <qpa/qplatformbackingstore.h>
#include <private/qwindow_p.h>
#include <rhi/qrhi.h>

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
      m_bsTextureWrapper(nullptr),
      m_bsTextureContext(0),
      m_textures(new QPlatformTextureList),
      m_lockedWidgetTextures(0),
      m_rhi(nullptr)
{
}

QOpenGLCompositorBackingStore::~QOpenGLCompositorBackingStore()
{
    if (m_bsTexture && m_rhi) {
        delete m_bsTextureWrapper;
        // Contexts are sharing resources, won't matter which one is
        // current here, use the rhi's shortcut.
        m_rhi->makeThreadLocalNativeContextCurrent();
        glDeleteTextures(1, &m_bsTexture);
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

    if (!m_bsTextureWrapper) {
        m_bsTextureWrapper = m_rhi->newTexture(QRhiTexture::RGBA8, m_image.size());
        m_bsTextureWrapper->createFrom({m_bsTexture, 0});
    }
}

void QOpenGLCompositorBackingStore::flush(QWindow *flushedWindow, const QRegion &region, const QPoint &offset)
{
    // Called for ordinary raster windows.

    Q_UNUSED(region);
    Q_UNUSED(offset);

    QOpenGLCompositorWindow *handle = dynamic_cast<QOpenGLCompositorWindow *>(flushedWindow->handle());
    if (handle && !handle->backingStore())
        handle->setBackingStore(this);

    if (!rhi(flushedWindow)) {
        QPlatformBackingStoreRhiConfig rhiConfig;
        rhiConfig.setApi(QPlatformBackingStoreRhiConfig::OpenGL);
        rhiConfig.setEnabled(true);
        createRhi(flushedWindow, rhiConfig);
    }

    static QPlatformTextureList emptyTextureList;
    bool translucentBackground = m_image.hasAlphaChannel();
    rhiFlush(flushedWindow, flushedWindow->devicePixelRatio(),
        region, offset, &emptyTextureList, translucentBackground);
}

QPlatformBackingStore::FlushResult QOpenGLCompositorBackingStore::rhiFlush(QWindow *window,
                                                                           qreal sourceDevicePixelRatio,
                                                                           const QRegion &region,
                                                                           const QPoint &offset,
                                                                           QPlatformTextureList *textures,
                                                                           bool translucentBackground,
                                                                           qreal sourceTransformFactor)
{
    // QOpenGLWidget/QQuickWidget content provided as textures. The raster content goes on top.

    Q_UNUSED(region);
    Q_UNUSED(offset);
    Q_UNUSED(translucentBackground);
    Q_UNUSED(sourceDevicePixelRatio);
    Q_UNUSED(sourceTransformFactor);

    m_rhi = rhi(window);
    Q_ASSERT(m_rhi);

    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    QOpenGLContext *dstCtx = compositor->context();
    if (!dstCtx)
        return FlushFailed;

    QWindow *dstWin = compositor->targetWindow();
    if (!dstWin)
        return FlushFailed;

    if (!dstCtx->makeCurrent(dstWin))
        return FlushFailed;

    QWindowPrivate::get(window)->lastComposeTime.start();

    m_textures->clear();
    for (int i = 0; i < textures->count(); ++i) {
        m_textures->appendTexture(textures->source(i), textures->texture(i), textures->geometry(i),
                                  textures->clipRect(i), textures->flags(i));
    }

    updateTexture();
    m_textures->appendTexture(nullptr, m_bsTextureWrapper, window->geometry());

    textures->lock(true);
    m_lockedWidgetTextures = textures;

    compositor->update();

    return FlushSuccess;
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
    if (!dstCtx)
        return;
    QWindow *dstWin = compositor->targetWindow();
    if (!dstWin)
        return;

    m_image = QImage(size, QImage::Format_RGBA8888);

    m_window->create();

    dstCtx->makeCurrent(dstWin);
    if (m_bsTexture) {
        delete m_bsTextureWrapper;
        m_bsTextureWrapper = nullptr;
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
