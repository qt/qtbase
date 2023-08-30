// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXCBIMAGE_H
#define QXCBIMAGE_H

#include "qxcbscreen.h"
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <xcb/xcb_image.h>

QT_BEGIN_NAMESPACE

bool qt_xcb_imageFormatForVisual(QXcbConnection *connection, uint8_t depth, const xcb_visualtype_t *visual,
                                 QImage::Format *imageFormat, bool *needsRgbSwap = nullptr);
QPixmap qt_xcb_pixmapFromXPixmap(QXcbConnection *connection, xcb_pixmap_t pixmap,
                                 int width, int height, int depth,
                                 const xcb_visualtype_t *visual);
xcb_pixmap_t qt_xcb_XPixmapFromBitmap(QXcbScreen *screen, const QImage &image);
xcb_cursor_t qt_xcb_createCursorXRender(QXcbScreen *screen, const QImage &image,
                                        const QPoint &spot);

QT_END_NAMESPACE

#endif
