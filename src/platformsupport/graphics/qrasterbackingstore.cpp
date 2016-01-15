/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qrasterbackingstore_p.h"

#include <QtGui/qpainter.h>

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

    int windowDevicePixelRatio = window()->devicePixelRatio();
    QSize effectiveBufferSize = size * windowDevicePixelRatio;

    if (m_image.size() == effectiveBufferSize)
        return;

    QImage::Format format = window()->format().hasAlpha() ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32;
    m_image = QImage(effectiveBufferSize, format);
    m_image.setDevicePixelRatio(windowDevicePixelRatio);
    if (format == QImage::Format_ARGB32_Premultiplied)
        m_image.fill(Qt::transparent);
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
    if (window()->surfaceType() != QSurface::RasterSurface)
        return false;

    extern void qt_scrollRectInImage(QImage &, const QRect &, const QPoint &);

    const qreal devicePixelRatio = m_image.devicePixelRatio();
    const QPoint delta(dx * devicePixelRatio, dy * devicePixelRatio);

    foreach (const QRect &rect, region.rects())
        qt_scrollRectInImage(m_image, QRect(rect.topLeft() * devicePixelRatio, rect.size() * devicePixelRatio), delta);

    return true;
}

void QRasterBackingStore::beginPaint(const QRegion &region)
{
    if (!m_image.hasAlphaChannel())
        return;

    QPainter painter(&m_image);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    const QColor blank = Qt::transparent;
    foreach (const QRect &rect, region.rects())
        painter.fillRect(rect, blank);
}

QT_END_NAMESPACE
