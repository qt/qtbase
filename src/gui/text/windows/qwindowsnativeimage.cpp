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

#include "qwindowsnativeimage_p.h"

#include <QtGui/private/qpaintengine_p.h>
#include <QtGui/private/qpaintengine_raster_p.h>

QT_BEGIN_NAMESPACE

typedef struct {
    BITMAPINFOHEADER bmiHeader;
    DWORD redMask;
    DWORD greenMask;
    DWORD blueMask;
} BITMAPINFO_MASK;

/*!
    \class QWindowsNativeImage
    \brief Windows Native image

    Note that size can be 0 (widget autotests with zero size), which
    causes CreateDIBSection() to fail.

    \sa QWindowsBackingStore
    \internal
*/

static inline HDC createDC()
{
    HDC display_dc = GetDC(0);
    HDC hdc = CreateCompatibleDC(display_dc);
    ReleaseDC(0, display_dc);
    Q_ASSERT(hdc);
    return hdc;
}

static inline HBITMAP createDIB(HDC hdc, int width, int height,
                                QImage::Format format,
                                uchar **bitsIn)
{
    BITMAPINFO_MASK bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = width;
    bmi.bmiHeader.biHeight      = -height; // top-down.
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biSizeImage   = 0;

    if (format == QImage::Format_RGB16) {
        bmi.bmiHeader.biBitCount = 16;
        bmi.bmiHeader.biCompression = BI_BITFIELDS;
        bmi.redMask = 0xF800;
        bmi.greenMask = 0x07E0;
        bmi.blueMask = 0x001F;
    } else {
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.redMask = 0;
        bmi.greenMask = 0;
        bmi.blueMask = 0;
    }

    uchar *bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(hdc, reinterpret_cast<BITMAPINFO *>(&bmi),
                                      DIB_RGB_COLORS, reinterpret_cast<void **>(&bits), 0, 0);
    if (Q_UNLIKELY(!bitmap || !bits)) {
        qFatal("%s: CreateDIBSection failed (%dx%d, format: %d)", __FUNCTION__,
               width, height, int(format));
    }

    *bitsIn = bits;
    return bitmap;
}

QWindowsNativeImage::QWindowsNativeImage(int width, int height,
                                         QImage::Format format) :
    m_hdc(createDC())
{
    if (width != 0 && height != 0) {
        uchar *bits;
        m_bitmap = createDIB(m_hdc, width, height, format, &bits);
        m_null_bitmap = static_cast<HBITMAP>(SelectObject(m_hdc, m_bitmap));
        m_image = QImage(bits, width, height, format);
        Q_ASSERT(m_image.paintEngine()->type() == QPaintEngine::Raster);
        static_cast<QRasterPaintEngine *>(m_image.paintEngine())->setDC(m_hdc);
    } else {
        m_image = QImage(width, height, format);
    }

    GdiFlush();
}

QWindowsNativeImage::~QWindowsNativeImage()
{
    if (m_hdc) {
        if (m_bitmap) {
            if (m_null_bitmap)
                SelectObject(m_hdc, m_null_bitmap);
            DeleteObject(m_bitmap);
        }
        DeleteDC(m_hdc);
    }
}

QImage::Format QWindowsNativeImage::systemFormat()
{
    static int depth = -1;
    if (depth == -1) {
        if (HDC defaultDC = GetDC(0)) {
            depth = GetDeviceCaps(defaultDC, BITSPIXEL);
            ReleaseDC(0, defaultDC);
        } else {
            // FIXME Same remark as in QWindowsFontDatabase::defaultVerticalDPI()
            // BONUS FIXME: Is 32 too generous/optimistic?
            depth = 32;
        }
    }
    return depth == 16 ? QImage::Format_RGB16 : QImage::Format_RGB32;
}

QT_END_NAMESPACE
