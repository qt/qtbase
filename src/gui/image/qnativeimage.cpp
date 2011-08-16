/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <qdebug.h>
#include "qnativeimage_p.h"
#include "private/qguiapplication_p.h"
#include "qscreen.h"

#include "private/qpaintengine_raster_p.h"

#include "private/qguiapplication_p.h"

#if defined(Q_WS_X11) && !defined(QT_NO_MITSHM)
#include <qx11info_x11.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#ifdef Q_WS_MAC
#include <private/qpaintengine_mac_p.h>
#endif

QT_BEGIN_NAMESPACE

#ifdef Q_WS_WIN
typedef struct {
    BITMAPINFOHEADER bmiHeader;
    DWORD redMask;
    DWORD greenMask;
    DWORD blueMask;
} BITMAPINFO_MASK;


QNativeImage::QNativeImage(int width, int height, QImage::Format format, bool isTextBuffer, QWindow *)
{
#ifndef Q_WS_WINCE
    Q_UNUSED(isTextBuffer);
#endif
    BITMAPINFO_MASK bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = width;
    bmi.bmiHeader.biHeight      = -height;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biSizeImage   = 0;

    if (format == QImage::Format_RGB16) {
        bmi.bmiHeader.biBitCount = 16;
#ifdef Q_WS_WINCE
        if (isTextBuffer) {
            bmi.bmiHeader.biCompression = BI_RGB;
            bmi.redMask = 0;
            bmi.greenMask = 0;
            bmi.blueMask = 0;
        } else
#endif
        {
            bmi.bmiHeader.biCompression = BI_BITFIELDS;
            bmi.redMask = 0xF800;
            bmi.greenMask = 0x07E0;
            bmi.blueMask = 0x001F;
        }
    } else {
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.redMask = 0;
        bmi.greenMask = 0;
        bmi.blueMask = 0;
    }

    HDC display_dc = GetDC(0);
    hdc = CreateCompatibleDC(display_dc);
    ReleaseDC(0, display_dc);
    Q_ASSERT(hdc);

    uchar *bits = 0;
    bitmap = CreateDIBSection(hdc, reinterpret_cast<BITMAPINFO *>(&bmi), DIB_RGB_COLORS, (void**) &bits, 0, 0);
    Q_ASSERT(bitmap);
    Q_ASSERT(bits);

    null_bitmap = (HBITMAP)SelectObject(hdc, bitmap);
    image = QImage(bits, width, height, format);

    Q_ASSERT(image.paintEngine()->type() == QPaintEngine::Raster);
    static_cast<QRasterPaintEngine *>(image.paintEngine())->setDC(hdc);

#ifndef Q_WS_WINCE
    GdiFlush();
#endif
}

QNativeImage::~QNativeImage()
{
    if (bitmap || hdc) {
        Q_ASSERT(hdc);
        Q_ASSERT(bitmap);
        if (null_bitmap)
            SelectObject(hdc, null_bitmap);
        DeleteDC(hdc);
        DeleteObject(bitmap);
    }
}

QImage::Format QNativeImage::systemFormat()
{
    if (QGuiApplication::primaryScreen()->depth() == 16)
        return QImage::Format_RGB16;
    return QImage::Format_RGB32;
}

#elif defined(Q_WS_MAC)

QNativeImage::QNativeImage(int width, int height, QImage::Format format, bool /* isTextBuffer */, QWindow *)
    : image(width, height, format)
{

    uint cgflags = kCGImageAlphaNoneSkipFirst;
    switch (format) {
    case QImage::Format_ARGB32:
        cgflags = kCGImageAlphaFirst;
        break;
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB4444_Premultiplied:
        cgflags = kCGImageAlphaPremultipliedFirst;
        break;
    default:
        break;
    }

#ifdef kCGBitmapByteOrder32Host //only needed because CGImage.h added symbols in the minor version
    cgflags |= kCGBitmapByteOrder32Host;
#endif

    cg = CGBitmapContextCreate(image.bits(), width, height, 8, image.bytesPerLine(),
                               QCoreGraphicsPaintEngine::macDisplayColorSpace(0), cgflags);
    CGContextTranslateCTM(cg, 0, height);
    CGContextScaleCTM(cg, 1, -1);

    Q_ASSERT(image.paintEngine()->type() == QPaintEngine::Raster);
    static_cast<QRasterPaintEngine *>(image.paintEngine())->setCGContext(cg);
}


QNativeImage::~QNativeImage()
{
    CGContextRelease(cg);
}

QImage::Format QNativeImage::systemFormat()
{
    return QImage::Format_RGB32;
}


#else // other platforms...

QNativeImage::QNativeImage(int width, int height, QImage::Format format,  bool /* isTextBuffer */, QWindow *)
    : image(width, height, format)
{

}


QNativeImage::~QNativeImage()
{
}

QImage::Format QNativeImage::systemFormat()
{
    return QGuiApplication::primaryScreen()->handle()->format();
}

#endif // platforms

QT_END_NAMESPACE

