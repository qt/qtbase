// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmbackingstore.h"
#include "qwasmwindow.h"
#include "qwasmcompositor.h"

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
    Q_UNUSED(region);
    Q_UNUSED(offset);

    m_dirty |= region;
    m_compositor->handleBackingStoreFlush(window);
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

    for (const QRect &dirtyRect : clippedDpiScaledRegion) {
        constexpr int BytesPerColor = 4;
        if (dirtyRect.width() == imageRect.width()) {
            // Copy a contiguous chunk of memory
            // ...............
            // OOOOOOOOOOOOOOO
            // OOOOOOOOOOOOOOO -> image data
            // OOOOOOOOOOOOOOO
            // ...............
            auto imageMemory = emscripten::typed_memory_view(dirtyRect.width() * dirtyRect.height()
                                                                     * BytesPerColor,
                                                             m_image.constScanLine(dirtyRect.y()));
            m_webImageDataArray["data"].call<void>("set", imageMemory,
                                                   dirtyRect.y() * m_image.width() * BytesPerColor);
        } else {
            // Go through the scanlines manually to set the individual lines in bulk. This is
            // marginally less performant than the above.
            // ...............
            // ...OOOOOOOOO... r = 0  -> image data
            // ...OOOOOOOOO... r = 1  -> image data
            // ...OOOOOOOOO... r = 2  -> image data
            // ...............
            for (int r = 0; r < dirtyRect.height(); ++r) {
                auto scanlineMemory = emscripten::typed_memory_view(
                        dirtyRect.width() * BytesPerColor,
                        m_image.constScanLine(r + dirtyRect.y()) + BytesPerColor * dirtyRect.x());
                m_webImageDataArray["data"].call<void>("set", scanlineMemory,
                                                       (dirtyRect.y() + r) * m_image.width()
                                                                       * BytesPerColor
                                                               + dirtyRect.x() * BytesPerColor);
            }
        }
    }

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
