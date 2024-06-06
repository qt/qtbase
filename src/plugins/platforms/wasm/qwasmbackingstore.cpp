// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmbackingstore.h"
#include "qwasmwindow.h"
#include "qwasmcompositor.h"
#include "qwasmdom.h"

#include <QtGui/qpainter.h>
#include <QtGui/qbackingstore.h>

#include <emscripten.h>
#include <emscripten/wire.h>

QT_BEGIN_NAMESPACE

QWasmBackingStore::QWasmBackingStore(QWasmCompositor *compositor, QWindow *window)
    : QPlatformBackingStore(window), m_compositor(compositor)
{
    QWasmWindow *wasmWindow = static_cast<QWasmWindow *>(window->handle());
    if (wasmWindow)
        wasmWindow->setBackingStore(this);
}

QWasmBackingStore::~QWasmBackingStore()
{
    auto window = this->window();
    QWasmIntegration::get()->removeBackingStore(window);
    QWasmWindow *wasmWindow = static_cast<QWasmWindow *>(window->handle());
    if (wasmWindow)
        wasmWindow->setBackingStore(nullptr);
}

QPaintDevice *QWasmBackingStore::paintDevice()
{
    return &m_image;
}

void QWasmBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    m_dirty |= region;

    QRect updateRect = region.boundingRect();
    updateRect.translate(offset);

    m_compositor->handleBackingStoreFlush(this->window(), updateRect);
}

void QWasmBackingStore::updateTexture(QWasmWindow *window)
{
    if (m_dirty.isNull())
        return;

    if (m_webImageDataArray.isUndefined()) {
        m_webImageDataArray = window->context2d().call<emscripten::val>(
                "createImageData", emscripten::val(m_image.width()),
                emscripten::val(m_image.height()));
    }

    QRegion clippedDpiScaledRegion;
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

        clippedDpiScaledRegion |= r;
    }

    for (const QRect &dirtyRect : clippedDpiScaledRegion)
        dom::drawImageToWebImageDataArray(m_image, m_webImageDataArray, dirtyRect);

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

    QImage::Format format = QImage::Format_RGBA8888;
    const auto platformScreenDPR = window()->handle()->devicePixelRatio();
    m_image = QImage(size * platformScreenDPR, format);
    m_image.setDevicePixelRatio(platformScreenDPR);
    m_webImageDataArray = emscripten::val::undefined();
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

emscripten::val QWasmBackingStore::getUpdatedWebImage(QWasmWindow *window)
{
    updateTexture(window);
    return m_webImageDataArray;
}

QT_END_NAMESPACE
