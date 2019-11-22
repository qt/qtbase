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

#include "qxcbimage.h"
#include <QtCore/QtEndian>
#include <QtGui/QColor>
#include <QtGui/private/qimage_p.h>
#include <QtGui/private/qdrawhelper_p.h>

#include <xcb/render.h>
#include <xcb/xcb_renderutil.h>

#include "qxcbconnection.h"
#include "qxcbintegration.h"

namespace {

QImage::Format imageFormatForMasks(int depth, int bits_per_pixel, int red_mask, int blue_mask)
{
    if (bits_per_pixel == 32) {
        switch (depth) {
        case 32:
            if (red_mask == 0xff0000 && blue_mask == 0xff)
                return QImage::Format_ARGB32_Premultiplied;
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            if (red_mask == 0xff && blue_mask == 0xff0000)
                return QImage::Format_RGBA8888_Premultiplied;
#else
            if (unsigned(red_mask) == unsigned(0xff000000) && blue_mask == 0xff00)
                return QImage::Format_RGBA8888_Premultiplied;
#endif
            if (red_mask == 0x3ff && blue_mask == 0x3ff00000)
                return QImage::Format_A2BGR30_Premultiplied;
            if (red_mask == 0x3ff00000 && blue_mask == 0x3ff)
                return QImage::Format_A2RGB30_Premultiplied;
            break;
        case 30:
            if (red_mask == 0x3ff && blue_mask == 0x3ff00000)
                return QImage::Format_BGR30;
            if (blue_mask == 0x3ff && red_mask == 0x3ff00000)
                return QImage::Format_RGB30;
            break;
        case 24:
            if (red_mask == 0xff0000 && blue_mask == 0xff)
                return QImage::Format_RGB32;
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            if (red_mask == 0xff && blue_mask == 0xff0000)
                return QImage::Format_RGBX8888;
#else
            if (unsigned(red_mask) == unsigned(0xff000000) && blue_mask == 0xff00)
                return QImage::Format_RGBX8888;
#endif
            break;
        }
    } else if (bits_per_pixel == 16) {
        if (depth == 16 && red_mask == 0xf800 && blue_mask == 0x1f)
            return QImage::Format_RGB16;
        if (depth == 15 && red_mask == 0x7c00 && blue_mask == 0x1f)
            return QImage::Format_RGB555;
    }
    return QImage::Format_Invalid;
}

} // namespace

QT_BEGIN_NAMESPACE

bool qt_xcb_imageFormatForVisual(QXcbConnection *connection, uint8_t depth, const xcb_visualtype_t *visual,
                                 QImage::Format *imageFormat, bool *needsRgbSwap)
{
    Q_ASSERT(connection && visual && imageFormat);

    if (needsRgbSwap)
        *needsRgbSwap = false;
    *imageFormat = QImage::Format_Invalid;

    if (depth == 8) {
        if (visual->_class == XCB_VISUAL_CLASS_GRAY_SCALE) {
            *imageFormat = QImage::Format_Grayscale8;
            return true;
        }
#if QT_CONFIG(xcb_native_painting)
        if (QXcbIntegration::instance() && QXcbIntegration::instance()->nativePaintingEnabled()) {
            *imageFormat = QImage::Format_Indexed8;
            return true;
        }
#endif
        return false;
    }

    const xcb_format_t *format = connection->formatForDepth(depth);
    if (!format)
        return false;

    const bool connectionEndianSwap = connection->imageNeedsEndianSwap();
    // We swap the masks and see if we can recognize it as a host format
    const quint32 red_mask = connectionEndianSwap ? qbswap(visual->red_mask) : visual->red_mask;
    const quint32 blue_mask = connectionEndianSwap ? qbswap(visual->blue_mask) : visual->blue_mask;

    *imageFormat = imageFormatForMasks(depth, format->bits_per_pixel, red_mask, blue_mask);
    if (*imageFormat != QImage::Format_Invalid)
        return true;

    if (needsRgbSwap) {
        *imageFormat = imageFormatForMasks(depth, format->bits_per_pixel, blue_mask, red_mask);
        if (*imageFormat != QImage::Format_Invalid) {
            *needsRgbSwap = true;
            return true;
        }
    }

    qWarning("Unsupported screen format: depth: %d, bits_per_pixel: %d, red_mask: %x, blue_mask: %x", depth, format->bits_per_pixel, red_mask, blue_mask);

    return false;
}

QPixmap qt_xcb_pixmapFromXPixmap(QXcbConnection *connection, xcb_pixmap_t pixmap,
                                 int width, int height, int depth,
                                 const xcb_visualtype_t *visual)
{
    xcb_connection_t *conn = connection->xcb_connection();

    auto image_reply = Q_XCB_REPLY_UNCHECKED(xcb_get_image, conn, XCB_IMAGE_FORMAT_Z_PIXMAP, pixmap,
                                             0, 0, width, height, 0xffffffff);
    if (!image_reply) {
        return QPixmap();
    }

    uint8_t *data = xcb_get_image_data(image_reply.get());
    uint32_t length = xcb_get_image_data_length(image_reply.get());

    QPixmap result;

    QImage::Format format;
    bool needsRgbSwap;
    if (qt_xcb_imageFormatForVisual(connection, depth, visual, &format, &needsRgbSwap)) {
        uint32_t bytes_per_line = length / height;
        QImage image(const_cast<uint8_t *>(data), width, height, bytes_per_line, format);

        if (needsRgbSwap)
            image = std::move(image).rgbSwapped();

        // fix-up alpha channel
        if (format == QImage::Format_RGB32 || format == QImage::Format_RGBX8888) {
            QRgb *p = (QRgb *)image.bits();
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x)
                    p[x] |= 0xff000000;
                p += bytes_per_line / 4;
            }
        } else if (format == QImage::Format_BGR30 || format == QImage::Format_RGB30) {
            QRgb *p = (QRgb *)image.bits();
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x)
                    p[x] |= 0xc0000000;
                p += bytes_per_line / 4;
            }
        }

        result = QPixmap::fromImage(image.copy());
    }

    return result;
}

xcb_pixmap_t qt_xcb_XPixmapFromBitmap(QXcbScreen *screen, const QImage &image)
{
    xcb_connection_t *conn = screen->xcb_connection();
    QImage bitmap = image.convertToFormat(QImage::Format_MonoLSB);
    const QRgb c0 = QColor(Qt::black).rgb();
    const QRgb c1 = QColor(Qt::white).rgb();
    if (bitmap.color(0) == c0 && bitmap.color(1) == c1) {
        bitmap.invertPixels();
        bitmap.setColor(0, c1);
        bitmap.setColor(1, c0);
    }
    const int width = bitmap.width();
    const int height = bitmap.height();
    const int bytesPerLine = bitmap.bytesPerLine();
    int destLineSize = width / 8;
    if (width % 8)
        ++destLineSize;
    const uchar *map = bitmap.bits();
    uint8_t *buf = new uint8_t[height * destLineSize];
    for (int i = 0; i < height; i++)
        memcpy(buf + (destLineSize * i), map + (bytesPerLine * i), destLineSize);
    xcb_pixmap_t pm = xcb_create_pixmap_from_bitmap_data(conn, screen->root(), buf,
                                                         width, height, 1, 0, 0, nullptr);
    delete[] buf;
    return pm;
}

xcb_cursor_t qt_xcb_createCursorXRender(QXcbScreen *screen, const QImage &image,
                                        const QPoint &spot)
{
    xcb_connection_t *conn = screen->xcb_connection();
    const int w = image.width();
    const int h = image.height();
    auto formats = Q_XCB_REPLY(xcb_render_query_pict_formats, conn);
    if (!formats) {
        qWarning("qt_xcb_createCursorXRender: query_pict_formats failed");
        return XCB_NONE;
    }
    xcb_render_pictforminfo_t *fmt = xcb_render_util_find_standard_format(formats.get(),
                                                                          XCB_PICT_STANDARD_ARGB_32);
    if (!fmt) {
        qWarning("qt_xcb_createCursorXRender: Failed to find format PICT_STANDARD_ARGB_32");
        return XCB_NONE;
    }

    QImage img = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    xcb_image_t *xi = xcb_image_create(w, h, XCB_IMAGE_FORMAT_Z_PIXMAP,
                                       32, 32, 32, 32,
                                       QSysInfo::ByteOrder == QSysInfo::BigEndian ? XCB_IMAGE_ORDER_MSB_FIRST : XCB_IMAGE_ORDER_LSB_FIRST,
                                       XCB_IMAGE_ORDER_MSB_FIRST,
                                       nullptr, 0, nullptr);
    if (!xi) {
        qWarning("qt_xcb_createCursorXRender: xcb_image_create failed");
        return XCB_NONE;
    }
    xi->data = (uint8_t *) malloc(xi->stride * h);
    if (!xi->data) {
        qWarning("qt_xcb_createCursorXRender: Failed to malloc() image data");
        xcb_image_destroy(xi);
        return XCB_NONE;
    }
    memcpy(xi->data, img.constBits(), img.sizeInBytes());

    xcb_pixmap_t pix = xcb_generate_id(conn);
    xcb_create_pixmap(conn, 32, pix, screen->root(), w, h);

    xcb_render_picture_t pic = xcb_generate_id(conn);
    xcb_render_create_picture(conn, pic, pix, fmt->id, 0, nullptr);

    xcb_gcontext_t gc = xcb_generate_id(conn);
    xcb_create_gc(conn, gc, pix, 0, nullptr);
    xcb_image_put(conn, pix, gc, xi, 0, 0, 0);
    xcb_free_gc(conn, gc);

    xcb_cursor_t cursor = xcb_generate_id(conn);
    xcb_render_create_cursor(conn, cursor, pic, spot.x(), spot.y());

    free(xi->data);
    xcb_image_destroy(xi);
    xcb_render_free_picture(conn, pic);
    xcb_free_pixmap(conn, pix);
    return cursor;
}

QT_END_NAMESPACE
