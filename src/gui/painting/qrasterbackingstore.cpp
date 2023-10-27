// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrasterbackingstore_p.h"

#include <QtGui/qbackingstore.h>
#include <QtGui/qpainter.h>

#include <private/qhighdpiscaling_p.h>
#include <qpa/qplatformwindow.h>

QT_BEGIN_NAMESPACE

QRasterBackingStore::QRasterBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
}

QRasterBackingStore::~QRasterBackingStore()
{
}

void QRasterBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);
    m_requestedSize = size;
}

QImage::Format QRasterBackingStore::format() const
{
    if (window()->format().hasAlpha())
        return QImage::Format_ARGB32_Premultiplied;
    else
        return QImage::Format_RGB32;
}

QPaintDevice *QRasterBackingStore::paintDevice()
{
    return &m_image;
}

QImage QRasterBackingStore::toImage() const
{
    return m_image;
}

bool QRasterBackingStore::scroll(const QRegion &region, int dx, int dy)
{
    if (!QPlatformSurface::isRasterSurface(window()))
        return false;

    extern void qt_scrollRectInImage(QImage &, const QRect &, const QPoint &);

    const qreal devicePixelRatio = m_image.devicePixelRatio();
    const QPoint delta(dx * devicePixelRatio, dy * devicePixelRatio);

    const QRect rect = region.boundingRect();
    qt_scrollRectInImage(m_image, QRect(rect.topLeft() * devicePixelRatio, rect.size() * devicePixelRatio), delta);

    return true;
}

void QRasterBackingStore::beginPaint(const QRegion &region)
{
    qreal nativeWindowDevicePixelRatio = window()->handle()->devicePixelRatio();
    QSize effectiveBufferSize = m_requestedSize * nativeWindowDevicePixelRatio;
    if (m_image.devicePixelRatio() != nativeWindowDevicePixelRatio || m_image.size() != effectiveBufferSize) {
        m_image = QImage(effectiveBufferSize, format());
        m_image.setDevicePixelRatio(nativeWindowDevicePixelRatio);
        if (m_image.hasAlphaChannel())
            m_image.fill(Qt::transparent);
    }

    if (!m_image.hasAlphaChannel())
        return;

    QPainter painter(&m_image);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    for (const QRect &rect : region)
        painter.fillRect(rect, Qt::transparent);
}

QT_END_NAMESPACE
