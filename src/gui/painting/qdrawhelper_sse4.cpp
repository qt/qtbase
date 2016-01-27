/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <private/qdrawhelper_p.h>
#include <private/qdrawingprimitive_sse2_p.h>

#if defined(QT_COMPILER_SUPPORTS_SSE4_1)

QT_BEGIN_NAMESPACE

const uint *QT_FASTCALL convertARGB32ToARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                     const QPixelLayout *, const QRgb *)
{
    return qt_convertARGB32ToARGB32PM(buffer, src, count);
}

const uint *QT_FASTCALL convertRGBA8888ToARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                       const QPixelLayout *, const QRgb *)
{
    return qt_convertRGBA8888ToARGB32PM(buffer, src, count);
}

const uint *QT_FASTCALL convertARGB32FromARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                       const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qUnpremultiply_sse4(src[i]);
    return buffer;
}

const uint *QT_FASTCALL convertRGBA8888FromARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                         const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = ARGB2RGBA(qUnpremultiply_sse4(src[i]));
    return buffer;
}

const uint *QT_FASTCALL convertRGBXFromARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                     const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = ARGB2RGBA(0xff000000 | qUnpremultiply_sse4(src[i]));
    return buffer;
}

template<QtPixelOrder PixelOrder>
const uint *QT_FASTCALL convertA2RGB30PMFromARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                          const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qConvertArgb32ToA2rgb30_sse4<PixelOrder>(src[i]);
    return buffer;
}

template
const uint *QT_FASTCALL convertA2RGB30PMFromARGB32PM_sse4<PixelOrderBGR>(uint *buffer, const uint *src, int count,
                                                                         const QPixelLayout *, const QRgb *);
template
const uint *QT_FASTCALL convertA2RGB30PMFromARGB32PM_sse4<PixelOrderRGB>(uint *buffer, const uint *src, int count,
                                                                         const QPixelLayout *, const QRgb *);

QT_END_NAMESPACE

#endif
