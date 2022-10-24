// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmbackingstore.h"
#include "qwasmwindow.h"
#include "qwasmcompositor.h"

#include <QtOpenGL/qopengltexture.h>
#include <QtGui/qmatrix4x4.h>
#include <QtGui/qpainter.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformscreen.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qbackingstore.h>

QT_BEGIN_NAMESPACE

QWasmBackingStore::QWasmBackingStore(QWasmCompositor *compositor, QWindow *window)
    : QPlatformBackingStore(window)
    , m_compositor(compositor)
    , m_texture(new QOpenGLTexture(QOpenGLTexture::Target2D))
{
    QWasmWindow *wasmWindow = static_cast<QWasmWindow *>(window->handle());
    if (wasmWindow)
        wasmWindow->setBackingStore(this);
}

QWasmBackingStore::~QWasmBackingStore()
{
    auto window = this->window();
    QWasmIntegration::get()->removeBackingStore(window);
    destroy();
    QWasmWindow *wasmWindow = static_cast<QWasmWindow *>(window->handle());
    if (wasmWindow)
        wasmWindow->setBackingStore(nullptr);
}

void QWasmBackingStore::destroy()
{
    if (m_texture->isCreated()) {
        auto context = m_compositor->context();
        auto currentContext = QOpenGLContext::currentContext();
        if (!currentContext || !QOpenGLContext::areSharing(context, currentContext)) {
            QOffscreenSurface offScreenSurface(m_compositor->screen()->screen());
            offScreenSurface.setFormat(context->format());
            offScreenSurface.create();
            context->makeCurrent(&offScreenSurface);
            m_texture->destroy();
        } else {
            m_texture->destroy();
        }
    }
}

QPaintDevice *QWasmBackingStore::paintDevice()
{
    return &m_image;
}

void QWasmBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    Q_UNUSED(region);
    Q_UNUSED(offset);

    m_dirty |= region;
    m_compositor->handleBackingStoreFlush();
}

void QWasmBackingStore::updateTexture()
{
    if (m_dirty.isNull())
        return;

    if (m_recreateTexture) {
        m_recreateTexture = false;
        destroy();
    }

    if (!m_texture->isCreated()) {
        m_texture->setMinificationFilter(QOpenGLTexture::Nearest);
        m_texture->setMagnificationFilter(QOpenGLTexture::Nearest);
        m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
        m_texture->setData(m_image, QOpenGLTexture::DontGenerateMipMaps);
        m_texture->create();
    }
    m_texture->bind();

    QRegion fixed;
    QRect imageRect = m_image.rect();

    for (const QRect &rect : m_dirty) {

        // Convert device-independent dirty region to device region
        qreal dpr = m_image.devicePixelRatio();
        QRect deviceRect = QRect(rect.topLeft() * dpr, rect.size() * dpr);

        // intersect with image rect to be sure
        QRect r = imageRect & deviceRect;
        // if the rect is wide enough it is cheaper to just extend it instead of doing an image copy
        if (r.width() >= imageRect.width() / 2) {
            r.setX(0);
            r.setWidth(imageRect.width());
        }

        fixed |= r;
    }

    for (const QRect &rect : fixed) {
        // if the sub-rect is full-width we can pass the image data directly to
        // OpenGL instead of copying, since there is no gap between scanlines
        if (rect.width() == imageRect.width()) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                            m_image.constScanLine(rect.y()));
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                            m_image.copy(rect).constBits());
        }
    }
    /* End of code taken from QEGLPlatformBackingStore */

    m_dirty = QRegion();
}

void QWasmBackingStore::beginPaint(const QRegion &region)
{
    m_dirty |= region;
    // Keep backing store device pixel ratio in sync with window
    if (m_image.devicePixelRatio() != window()->handle()->devicePixelRatio())
        resize(backingStore()->size(), backingStore()->staticContents());

    QPainter painter(&m_image);

    if (m_image.hasAlphaChannel()) {
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        const QColor blank = Qt::transparent;
        for (const QRect &rect : region)
            painter.fillRect(rect, blank);
    }
}

void QWasmBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);

    QImage::Format format = window()->format().hasAlpha() ?
        QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32;
    const auto platformScreenDPR = window()->handle()->devicePixelRatio();
    m_image = QImage(size * platformScreenDPR, format);
    m_image.setDevicePixelRatio(platformScreenDPR);
    m_recreateTexture = true;
}

QImage QWasmBackingStore::toImage() const
{
    // used by QPlatformBackingStore::rhiFlush
    return m_image;
}

const QImage &QWasmBackingStore::getImageRef() const
{
    return m_image;
}

const QOpenGLTexture *QWasmBackingStore::getUpdatedTexture()
{
    updateTexture();
    return m_texture.data();
}

QT_END_NAMESPACE
