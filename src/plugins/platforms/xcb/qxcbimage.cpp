/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxcbimage.h"
#include <QtGui/QColor>
#include <QtGui/private/qimage_p.h>

QT_BEGIN_NAMESPACE

QImage::Format qt_xcb_imageFormatForVisual(QXcbConnection *connection, uint8_t depth,
                                           const xcb_visualtype_t *visual)
{
    const xcb_format_t *format = connection->formatForDepth(depth);

    if (!visual || !format)
        return QImage::Format_Invalid;

    if (depth == 32 && format->bits_per_pixel == 32 && visual->red_mask == 0xff0000
        && visual->green_mask == 0xff00 && visual->blue_mask == 0xff)
        return QImage::Format_ARGB32_Premultiplied;

    if (depth == 24 && format->bits_per_pixel == 32 && visual->red_mask == 0xff0000
        && visual->green_mask == 0xff00 && visual->blue_mask == 0xff)
        return QImage::Format_RGB32;

    if (depth == 16 && format->bits_per_pixel == 16 && visual->red_mask == 0xf800
        && visual->green_mask == 0x7e0 && visual->blue_mask == 0x1f)
        return QImage::Format_RGB16;

    return QImage::Format_Invalid;
}

QPixmap qt_xcb_pixmapFromXPixmap(QXcbConnection *connection, xcb_pixmap_t pixmap,
                                 int width, int height, int depth,
                                 const xcb_visualtype_t *visual)
{
    xcb_connection_t *conn = connection->xcb_connection();
    xcb_generic_error_t *error = 0;

    xcb_get_image_cookie_t get_image_cookie =
        xcb_get_image(conn, XCB_IMAGE_FORMAT_Z_PIXMAP, pixmap,
                      0, 0, width, height, 0xffffffff);

    xcb_get_image_reply_t *image_reply =
        xcb_get_image_reply(conn, get_image_cookie, &error);

    if (!image_reply) {
        if (error) {
            connection->handleXcbError(error);
            free(error);
        }
        return QPixmap();
    }

    uint8_t *data = xcb_get_image_data(image_reply);
    uint32_t length = xcb_get_image_data_length(image_reply);

    QPixmap result;

    QImage::Format format = qt_xcb_imageFormatForVisual(connection, depth, visual);
    if (format != QImage::Format_Invalid) {
        uint32_t bytes_per_line = length / height;
        QImage image(const_cast<uint8_t *>(data), width, height, bytes_per_line, format);
        uint8_t image_byte_order = connection->setup()->image_byte_order;

        // we may have to swap the byte order
        if ((QSysInfo::ByteOrder == QSysInfo::LittleEndian && image_byte_order == XCB_IMAGE_ORDER_MSB_FIRST)
            || (QSysInfo::ByteOrder == QSysInfo::BigEndian && image_byte_order == XCB_IMAGE_ORDER_LSB_FIRST))
        {
            for (int i=0; i < image.height(); i++) {
                switch (format) {
                case QImage::Format_RGB16: {
                    ushort *p = (ushort*)image.scanLine(i);
                    ushort *end = p + image.width();
                    while (p < end) {
                        *p = ((*p << 8) & 0xff00) | ((*p >> 8) & 0x00ff);
                        p++;
                    }
                    break;
                }
                case QImage::Format_RGB32: // fall-through
                case QImage::Format_ARGB32_Premultiplied: {
                    uint *p = (uint*)image.scanLine(i);
                    uint *end = p + image.width();
                    while (p < end) {
                        *p = ((*p << 24) & 0xff000000) | ((*p << 8) & 0x00ff0000)
                            | ((*p >> 8) & 0x0000ff00) | ((*p >> 24) & 0x000000ff);
                        p++;
                    }
                    break;
                }
                default:
                    Q_ASSERT(false);
                }
            }
        }

        // fix-up alpha channel
        if (format == QImage::Format_RGB32) {
            QRgb *p = (QRgb *)image.bits();
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x)
                    p[x] |= 0xff000000;
                p += bytes_per_line / 4;
            }
        }

        result = QPixmap::fromImage(image.copy());
    }

    free(image_reply);
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
                                                         width, height, 1, 0, 0, 0);
    delete[] buf;
    return pm;
}

QT_END_NAMESPACE
