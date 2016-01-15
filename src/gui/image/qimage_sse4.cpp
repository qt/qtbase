/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <qimage.h>
#include <private/qdrawhelper_p.h>
#include <private/qdrawingprimitive_sse2_p.h>
#include <private/qimage_p.h>
#include <private/qsimd_p.h>

#ifdef QT_COMPILER_SUPPORTS_SSE4_1

QT_BEGIN_NAMESPACE

const uint *QT_FASTCALL convertRGB32FromARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                      const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = 0xff000000 | qUnpremultiply_sse4(src[i]);
    return buffer;
}

void convert_ARGB_to_ARGB_PM_sse4(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_ARGB32 || src->format == QImage::Format_RGBA8888);
    Q_ASSERT(dest->format == QImage::Format_ARGB32_Premultiplied || dest->format == QImage::Format_RGBA8888_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const uint *src_data = (uint *) src->data;
    uint *dest_data = (uint *) dest->data;
    for (int i = 0; i < src->height; ++i) {
        qt_convertARGB32ToARGB32PM(dest_data, src_data, src->width);
        src_data += src->bytes_per_line >> 2;
        dest_data += dest->bytes_per_line >> 2;
    }
}

QT_END_NAMESPACE

#endif // QT_COMPILER_SUPPORTS_SSE4_1
