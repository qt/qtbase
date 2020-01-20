/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwasmbackingstore.h"
#include "qwasmwindow.h"
#include "qwasmcompositor.h"

#include <QtGui/qopengltexture.h>
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
    m_compositor->requestRedraw();
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
    if (m_image.devicePixelRatio() != window()->devicePixelRatio())
        resize(backingStore()->size(), backingStore()->staticContents());

    QPainter painter(&m_image);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    const QColor blank = Qt::transparent;
    for (const QRect &rect : region)
        painter.fillRect(rect, blank);
}

void QWasmBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents)

    m_image = QImage(size * window()->devicePixelRatio(), QImage::Format_RGB32);
    m_image.setDevicePixelRatio(window()->devicePixelRatio());
    m_recreateTexture = true;
}

QImage QWasmBackingStore::toImage() const
{
    // used by QPlatformBackingStore::composeAndFlush
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
