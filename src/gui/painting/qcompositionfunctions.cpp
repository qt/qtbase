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

#include <qglobal.h>
#include <private/qdrawhelper_p.h>
#include <private/qrgba64_p.h>

QT_BEGIN_NAMESPACE

#    define PRELOAD_INIT(x)
#    define PRELOAD_INIT2(x,y)
#    define PRELOAD_COND(x)
#    define PRELOAD_COND2(x,y)

/* The constant alpha factor describes an alpha factor that gets applied
   to the result of the composition operation combining it with the destination.

   The intent is that if const_alpha == 0. we get back dest, and if const_alpha == 1.
   we get the unmodified operation

   result = src op dest
   dest = result * const_alpha + dest * (1. - const_alpha)

   This means that in the comments below, the first line is the const_alpha==255 case, the
   second line the general one.

   In the lines below:
   s == src, sa == alpha(src), sia = 1 - alpha(src)
   d == dest, da == alpha(dest), dia = 1 - alpha(dest)
   ca = const_alpha, cia = 1 - const_alpha

   The methods exist in two variants. One where we have a constant source, the other
   where the source is an array of pixels.
*/

/*
  result = 0
  d = d * cia
*/
#define comp_func_Clear_impl(dest, length, const_alpha)\
{\
    if (const_alpha == 255) {\
        QT_MEMFILL_UINT(dest, length, 0);\
    } else {\
        int ialpha = 255 - const_alpha;\
        PRELOAD_INIT(dest)\
        for (int i = 0; i < length; ++i) {\
            PRELOAD_COND(dest)\
            dest[i] = BYTE_MUL(dest[i], ialpha);\
        }\
    }\
}

void QT_FASTCALL comp_func_solid_Clear(uint *dest, int length, uint, uint const_alpha)
{
    comp_func_Clear_impl(dest, length, const_alpha);
}

void QT_FASTCALL comp_func_solid_Clear_rgb64(QRgba64 *dest, int length, QRgba64, uint const_alpha)
{
    if (const_alpha == 255)
        qt_memfill64((quint64*)dest, 0, length);
    else {
        int ialpha = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            dest[i] = multiplyAlpha255(dest[i], ialpha);
        }
    }
}

void QT_FASTCALL comp_func_Clear(uint *dest, const uint *, int length, uint const_alpha)
{
    comp_func_Clear_impl(dest, length, const_alpha);
}

void QT_FASTCALL comp_func_Clear_rgb64(QRgba64 *dest, const QRgba64 *, int length, uint const_alpha)
{
    if (const_alpha == 255)
        qt_memfill64((quint64*)dest, 0, length);
    else {
        int ialpha = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            dest[i] = multiplyAlpha255(dest[i], ialpha);
        }
    }
}

/*
  result = s
  dest = s * ca + d * cia
*/
void QT_FASTCALL comp_func_solid_Source(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255) {
        QT_MEMFILL_UINT(dest, length, color);
    } else {
        int ialpha = 255 - const_alpha;
        color = BYTE_MUL(color, const_alpha);
        PRELOAD_INIT(dest)
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            dest[i] = color + BYTE_MUL(dest[i], ialpha);
        }
    }
}

void QT_FASTCALL comp_func_solid_Source_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255)
        qt_memfill64((quint64*)dest, color, length);
    else {
        int ialpha = 255 - const_alpha;
        color = multiplyAlpha255(color, const_alpha);
        for (int i = 0; i < length; ++i) {
            dest[i] = color + multiplyAlpha255(dest[i], ialpha);
        }
    }
}

void QT_FASTCALL comp_func_Source(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        ::memcpy(dest, src, length * sizeof(uint));
    } else {
        int ialpha = 255 - const_alpha;
        PRELOAD_INIT2(dest, src)
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            dest[i] = INTERPOLATE_PIXEL_255(src[i], const_alpha, dest[i], ialpha);
        }
    }
}

void QT_FASTCALL comp_func_Source_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        ::memcpy(dest, src, length * sizeof(quint64));
    else {
        int ialpha = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            dest[i] = interpolate255(src[i], const_alpha, dest[i], ialpha);
        }
    }
}

void QT_FASTCALL comp_func_solid_Destination(uint *, int, uint, uint)
{
}

void QT_FASTCALL comp_func_solid_Destination_rgb64(QRgba64 *, int, QRgba64, uint)
{
}

void QT_FASTCALL comp_func_Destination(uint *, const uint *, int, uint)
{
}

void QT_FASTCALL comp_func_Destination_rgb64(QRgba64 *, const QRgba64 *, int, uint)
{
}

/*
  result = s + d * sia
  dest = (s + d * sia) * ca + d * cia
       = s * ca + d * (sia * ca + cia)
       = s * ca + d * (1 - sa*ca)
*/
void QT_FASTCALL comp_func_solid_SourceOver(uint *dest, int length, uint color, uint const_alpha)
{
    if ((const_alpha & qAlpha(color)) == 255) {
        QT_MEMFILL_UINT(dest, length, color);
    } else {
        if (const_alpha != 255)
            color = BYTE_MUL(color, const_alpha);
        PRELOAD_INIT(dest)
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            dest[i] = color + BYTE_MUL(dest[i], qAlpha(~color));
        }
    }
}

void QT_FASTCALL comp_func_solid_SourceOver_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255 && color.isOpaque()) {
        qt_memfill64((quint64*)dest, color, length);
    } else {
        if (const_alpha != 255)
            color = multiplyAlpha255(color, const_alpha);
        for (int i = 0; i < length; ++i) {
            dest[i] = color + multiplyAlpha65535(dest[i], 65535 - color.alpha());
        }
    }
}

void QT_FASTCALL comp_func_SourceOver(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = src[i];
            if (s >= 0xff000000)
                dest[i] = s;
            else if (s != 0)
                dest[i] = s + BYTE_MUL(dest[i], qAlpha(~s));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = s + BYTE_MUL(dest[i], qAlpha(~s));
        }
    }
}

void QT_FASTCALL comp_func_SourceOver_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            QRgba64 s = src[i];
            if (s.isOpaque())
                dest[i] = s;
            else if (!s.isTransparent())
                dest[i] = s + multiplyAlpha65535(dest[i], 65535 - s.alpha());
        }
    } else {
        for (int i = 0; i < length; ++i) {
            QRgba64 s = multiplyAlpha255(src[i], const_alpha);
            dest[i] = s + multiplyAlpha65535(dest[i], 65535 - s.alpha());
        }
    }
}

/*
  result = d + s * dia
  dest = (d + s * dia) * ca + d * cia
       = d + s * dia * ca
*/
void QT_FASTCALL comp_func_solid_DestinationOver(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255)
        color = BYTE_MUL(color, const_alpha);
    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        dest[i] = d + BYTE_MUL(color, qAlpha(~d));
    }
}

void QT_FASTCALL comp_func_solid_DestinationOver_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha != 255)
        color = multiplyAlpha255(color, const_alpha);
    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        dest[i] = d + multiplyAlpha65535(color, 65535 - d.alpha());
    }
}

void QT_FASTCALL comp_func_DestinationOver(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint d = dest[i];
            dest[i] = d + BYTE_MUL(src[i], qAlpha(~d));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint d = dest[i];
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = d + BYTE_MUL(s, qAlpha(~d));
        }
    }
}

void QT_FASTCALL comp_func_DestinationOver_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            QRgba64 d = dest[i];
            dest[i] = d + multiplyAlpha65535(src[i], 65535 - d.alpha());
        }
    } else {
        for (int i = 0; i < length; ++i) {
            QRgba64 d = dest[i];
            QRgba64 s = multiplyAlpha255(src[i], const_alpha);
            dest[i] = d + multiplyAlpha65535(s, 65535 - d.alpha());
        }
    }
}

/*
  result = s * da
  dest = s * da * ca + d * cia
*/
void QT_FASTCALL comp_func_solid_SourceIn(uint *dest, int length, uint color, uint const_alpha)
{
    PRELOAD_INIT(dest)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            dest[i] = BYTE_MUL(color, qAlpha(dest[i]));
        }
    } else {
        color = BYTE_MUL(color, const_alpha);
        uint cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(d), d, cia);
        }
    }
}

void QT_FASTCALL comp_func_solid_SourceIn_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            dest[i] =  multiplyAlpha65535(color, dest[i].alpha());
        }
    } else {
        uint ca = const_alpha * 257;
        uint cia = 65535 - ca;
        color = multiplyAlpha65535(color, ca);
        for (int i = 0; i < length; ++i) {
            QRgba64 d = dest[i];
            dest[i] = interpolate65535(color, d.alpha(), d, cia);
        }
    }
}

void QT_FASTCALL comp_func_SourceIn(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            dest[i] = BYTE_MUL(src[i], qAlpha(dest[i]));
        }
    } else {
        uint cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint d = dest[i];
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(d), d, cia);
        }
    }
}

void QT_FASTCALL comp_func_SourceIn_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            dest[i] = multiplyAlpha65535(src[i], dest[i].alpha());
        }
    } else {
        uint ca = const_alpha * 257;
        uint cia = 65535 - ca;
        for (int i = 0; i < length; ++i) {
            QRgba64 d = dest[i];
            QRgba64 s = multiplyAlpha65535(src[i], ca);
            dest[i] = interpolate65535(s, d.alpha(), d, cia);
        }
    }
}

/*
  result = d * sa
  dest = d * sa * ca + d * cia
       = d * (sa * ca + cia)
*/
void QT_FASTCALL comp_func_solid_DestinationIn(uint *dest, int length, uint color, uint const_alpha)
{
    uint a = qAlpha(color);
    if (const_alpha != 255) {
        a = BYTE_MUL(a, const_alpha) + 255 - const_alpha;
    }
    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        dest[i] = BYTE_MUL(dest[i], a);
    }
}

void QT_FASTCALL comp_func_solid_DestinationIn_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    uint a = color.alpha();
    uint ca64k = const_alpha * 257;
    if (const_alpha != 255)
        a = qt_div_65535(a * ca64k) + 65535 - ca64k;
    for (int i = 0; i < length; ++i) {
        dest[i] = multiplyAlpha65535(dest[i], a);
    }
}

void QT_FASTCALL comp_func_DestinationIn(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            dest[i] = BYTE_MUL(dest[i], qAlpha(src[i]));
        }
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint a = BYTE_MUL(qAlpha(src[i]), const_alpha) + cia;
            dest[i] = BYTE_MUL(dest[i], a);
        }
    }
}

void QT_FASTCALL comp_func_DestinationIn_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            dest[i] = multiplyAlpha65535(dest[i], src[i].alpha());
        }
    } else {
        uint ca = const_alpha * 257;
        uint cia = 65535 - ca;
        for (int i = 0; i < length; ++i) {
            uint a = qt_div_65535(src[i].alpha() * ca) + cia;
            dest[i] = multiplyAlpha65535(dest[i], a);
        }
    }
}

/*
  result = s * dia
  dest = s * dia * ca + d * cia
*/

void QT_FASTCALL comp_func_solid_SourceOut(uint *dest, int length, uint color, uint const_alpha)
{
    PRELOAD_INIT(dest)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            dest[i] = BYTE_MUL(color, qAlpha(~dest[i]));
        }
    } else {
        color = BYTE_MUL(color, const_alpha);
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(~d), d, cia);
        }
    }
}

void QT_FASTCALL comp_func_solid_SourceOut_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            dest[i] =  multiplyAlpha65535(color, 65535 - dest[i].alpha());
        }
    } else {
        uint ca = const_alpha * 257;
        uint cia = 65535 - ca;
        color = multiplyAlpha65535(color, ca);
        for (int i = 0; i < length; ++i) {
            QRgba64 d = dest[i];
            dest[i] = interpolate65535(color, 65535 - d.alpha(), d, cia);
        }
    }
}

void QT_FASTCALL comp_func_SourceOut(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            dest[i] = BYTE_MUL(src[i], qAlpha(~dest[i]));
        }
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = BYTE_MUL(src[i], const_alpha);
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(~d), d, cia);
        }
    }
}

void QT_FASTCALL comp_func_SourceOut_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            dest[i] = multiplyAlpha65535(src[i], 65535 - dest[i].alpha());
        }
    } else {
        uint ca = const_alpha * 257;
        uint cia = 65535 - ca;
        for (int i = 0; i < length; ++i) {
            QRgba64 d = dest[i];
            QRgba64 s = multiplyAlpha65535(src[i], ca);
            dest[i] = interpolate65535(s, 65535 - d.alpha(), d, cia);
        }
    }
}

/*
  result = d * sia
  dest = d * sia * ca + d * cia
       = d * (sia * ca + cia)
*/
void QT_FASTCALL comp_func_solid_DestinationOut(uint *dest, int length, uint color, uint const_alpha)
{
    uint a = qAlpha(~color);
    if (const_alpha != 255)
        a = BYTE_MUL(a, const_alpha) + 255 - const_alpha;
    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        dest[i] = BYTE_MUL(dest[i], a);
    }
}

void QT_FASTCALL comp_func_solid_DestinationOut_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    uint a = 65535 - color.alpha();
    uint ca64k = const_alpha * 257;
    if (const_alpha != 255)
        a = qt_div_65535(a * ca64k) + 65535 - ca64k;
    for (int i = 0; i < length; ++i) {
        dest[i] = multiplyAlpha65535(dest[i], a);
    }
}

void QT_FASTCALL comp_func_DestinationOut(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            dest[i] = BYTE_MUL(dest[i], qAlpha(~src[i]));
        }
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint sia = BYTE_MUL(qAlpha(~src[i]), const_alpha) + cia;
            dest[i] = BYTE_MUL(dest[i], sia);
        }
    }
}

void QT_FASTCALL comp_func_DestinationOut_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            dest[i] = multiplyAlpha65535(dest[i], 65535 - src[i].alpha());
        }
    } else {
        uint ca = const_alpha * 257;
        uint cia = 65535 - ca;
        for (int i = 0; i < length; ++i) {
            uint a = qt_div_65535((65535 - src[i].alpha()) * ca) + cia;
            dest[i] = multiplyAlpha65535(dest[i], a);
        }
    }
}

/*
  result = s*da + d*sia
  dest = s*da*ca + d*sia*ca + d *cia
       = s*ca * da + d * (sia*ca + cia)
       = s*ca * da + d * (1 - sa*ca)
*/
void QT_FASTCALL comp_func_solid_SourceAtop(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255) {
        color = BYTE_MUL(color, const_alpha);
    }
    uint sia = qAlpha(~color);
    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(dest[i]), dest[i], sia);
    }
}

void QT_FASTCALL comp_func_solid_SourceAtop_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha != 255)
        color = multiplyAlpha255(color, const_alpha);
    uint sia = 65535 - color.alpha();
    for (int i = 0; i < length; ++i) {
        dest[i] = interpolate65535(color, dest[i].alpha(), dest[i], sia);
    }
}

void QT_FASTCALL comp_func_SourceAtop(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = src[i];
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(d), d, qAlpha(~s));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = BYTE_MUL(src[i], const_alpha);
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(d), d, qAlpha(~s));
        }
    }
}

void QT_FASTCALL comp_func_SourceAtop_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            QRgba64 s = src[i];
            QRgba64 d = dest[i];
            dest[i] = interpolate65535(s, d.alpha(), d, 65535 - s.alpha());
        }
    } else {
        for (int i = 0; i < length; ++i) {
            QRgba64 s = multiplyAlpha255(src[i], const_alpha);
            QRgba64 d = dest[i];
            dest[i] = interpolate65535(s, d.alpha(), d, 65535 - s.alpha());
        }
    }
}

/*
  result = d*sa + s*dia
  dest = d*sa*ca + s*dia*ca + d *cia
       = s*ca * dia + d * (sa*ca + cia)
*/
void QT_FASTCALL comp_func_solid_DestinationAtop(uint *dest, int length, uint color, uint const_alpha)
{
    uint a = qAlpha(color);
    if (const_alpha != 255) {
        color = BYTE_MUL(color, const_alpha);
        a = qAlpha(color) + 255 - const_alpha;
    }
    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        dest[i] = INTERPOLATE_PIXEL_255(d, a, color, qAlpha(~d));
    }
}

void QT_FASTCALL comp_func_solid_DestinationAtop_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    uint a = color.alpha();
    if (const_alpha != 255) {
        color = multiplyAlpha255(color, const_alpha);
        a = color.alpha() + 65535 - (const_alpha * 257);
    }
    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        dest[i] = interpolate65535(d, a, color, 65535 - d.alpha());
    }
}

void QT_FASTCALL comp_func_DestinationAtop(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = src[i];
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(d, qAlpha(s), s, qAlpha(~d));
        }
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = BYTE_MUL(src[i], const_alpha);
            uint d = dest[i];
            uint a = qAlpha(s) + cia;
            dest[i] = INTERPOLATE_PIXEL_255(d, a, s, qAlpha(~d));
        }
    }
}

void QT_FASTCALL comp_func_DestinationAtop_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            QRgba64 s = src[i];
            QRgba64 d = dest[i];
            dest[i] = interpolate65535(d, s.alpha(), s, 65535 - d.alpha());
        }
    } else {
        int ca = const_alpha * 257;
        int cia = 65535 - ca;
        for (int i = 0; i < length; ++i) {
            QRgba64 s = multiplyAlpha65535(src[i], ca);
            QRgba64 d = dest[i];
            uint a = s.alpha() + cia;
            dest[i] = interpolate65535(d, a, s, 65535 - d.alpha());
        }
    }
}

/*
  result = d*sia + s*dia
  dest = d*sia*ca + s*dia*ca + d *cia
       = s*ca * dia + d * (sia*ca + cia)
       = s*ca * dia + d * (1 - sa*ca)
*/
void QT_FASTCALL comp_func_solid_XOR(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255)
        color = BYTE_MUL(color, const_alpha);
    uint sia = qAlpha(~color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(~d), d, sia);
    }
}

void QT_FASTCALL comp_func_solid_XOR_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha != 255)
        color = multiplyAlpha255(color, const_alpha);
    uint sia = 65535 - color.alpha();
    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        dest[i] = interpolate65535(color, 65535 - d.alpha(), d, sia);
    }
}

void QT_FASTCALL comp_func_XOR(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint d = dest[i];
            uint s = src[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(~d), d, qAlpha(~s));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint d = dest[i];
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(~d), d, qAlpha(~s));
        }
    }
}

void QT_FASTCALL comp_func_XOR_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            QRgba64 d = dest[i];
            QRgba64 s = src[i];
            dest[i] = interpolate65535(s, 65535 - d.alpha(), d, 65535 - s.alpha());
        }
    } else {
        for (int i = 0; i < length; ++i) {
            QRgba64 d = dest[i];
            QRgba64 s = multiplyAlpha255(src[i], const_alpha);
            dest[i] = interpolate65535(s, 65535 - d.alpha(), d, 65535 - s.alpha());
        }
    }
}

struct QFullCoverage {
    inline void store(uint *dest, const uint src) const
    {
        *dest = src;
    }
};

struct QPartialCoverage {
    inline QPartialCoverage(uint const_alpha)
        : ca(const_alpha)
        , ica(255 - const_alpha)
    {
    }

    inline void store(uint *dest, const uint src) const
    {
        *dest = INTERPOLATE_PIXEL_255(src, ca, *dest, ica);
    }

private:
    const uint ca;
    const uint ica;
};

static inline int mix_alpha(int da, int sa)
{
    return 255 - ((255 - sa) * (255 - da) >> 8);
}

/*
    Dca' = Sca.Da + Dca.Sa + Sca.(1 - Da) + Dca.(1 - Sa)
         = Sca + Dca
*/
template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Plus_impl(uint *dest, int length, uint color, const T &coverage)
{
    uint s = color;

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        d = comp_func_Plus_one_pixel(d, s);
        coverage.store(&dest[i], d);
    }
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Plus_impl_rgb64(QRgba64 *dest, int length, QRgba64 color, const T &coverage)
{
    QRgba64 s = color;
    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        d = comp_func_Plus_one_pixel(d, s);
        coverage.store(&dest[i], d);
    }
}

void QT_FASTCALL comp_func_solid_Plus(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Plus_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Plus_impl(dest, length, color, QPartialCoverage(const_alpha));
}

void QT_FASTCALL comp_func_solid_Plus_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            dest[i] = addWithSaturation(dest[i], color);
        }
    } else {
        for (int i = 0; i < length; ++i) {
            QRgba64 d = addWithSaturation(dest[i], color);
            dest[i] = interpolate255(d, const_alpha, dest[i], 255 - const_alpha);
        }
    }
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Plus_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        d = comp_func_Plus_one_pixel(d, s);

        coverage.store(&dest[i], d);
    }
}

void QT_FASTCALL comp_func_Plus(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Plus_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Plus_impl(dest, src, length, QPartialCoverage(const_alpha));
}

void QT_FASTCALL comp_func_Plus_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            dest[i] = addWithSaturation(dest[i], src[i]);
        }
    } else {
        for (int i = 0; i < length; ++i) {
            QRgba64 d = addWithSaturation(dest[i], src[i]);
            dest[i] = interpolate255(d, const_alpha, dest[i], 255 - const_alpha);
        }
    }
}

/*
    Dca' = Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int multiply_op(int dst, int src, int da, int sa)
{
    return qt_div_255(src * dst + src * (255 - da) + dst * (255 - sa));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Multiply_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) multiply_op(a, b, da, sa)
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Multiply(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Multiply_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Multiply_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Multiply_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) multiply_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Multiply(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Multiply_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Multiply_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    Dca' = (Sca.Da + Dca.Sa - Sca.Dca) + Sca.(1 - Da) + Dca.(1 - Sa)
         = Sca + Dca - Sca.Dca
*/
template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Screen_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) 255 - qt_div_255((255-a) * (255-b))
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Screen(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Screen_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Screen_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Screen_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) 255 - (((255-a) * (255-b)) >> 8)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Screen(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Screen_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Screen_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    if 2.Dca < Da
        Dca' = 2.Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
    otherwise
        Dca' = Sa.Da - 2.(Da - Dca).(Sa - Sca) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int overlay_op(int dst, int src, int da, int sa)
{
    const int temp = src * (255 - da) + dst * (255 - sa);
    if (2 * dst < da)
        return qt_div_255(2 * src * dst + temp);
    else
        return qt_div_255(sa * da - 2 * (da - dst) * (sa - src) + temp);
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Overlay_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) overlay_op(a, b, da, sa)
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Overlay(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Overlay_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Overlay_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Overlay_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) overlay_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Overlay(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Overlay_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Overlay_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    Dca' = min(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
    Da'  = Sa + Da - Sa.Da
*/
static inline int darken_op(int dst, int src, int da, int sa)
{
    return qt_div_255(qMin(src * da, dst * sa) + src * (255 - da) + dst * (255 - sa));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Darken_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) darken_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Darken(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Darken_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Darken_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Darken_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) darken_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Darken(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Darken_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Darken_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
   Dca' = max(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
   Da'  = Sa + Da - Sa.Da
*/
static inline int lighten_op(int dst, int src, int da, int sa)
{
    return qt_div_255(qMax(src * da, dst * sa) + src * (255 - da) + dst * (255 - sa));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Lighten_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) lighten_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Lighten(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Lighten_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Lighten_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Lighten_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) lighten_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Lighten(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Lighten_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Lighten_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
   if Sca.Da + Dca.Sa >= Sa.Da
       Dca' = Sa.Da + Sca.(1 - Da) + Dca.(1 - Sa)
   otherwise
       Dca' = Dca.Sa/(1-Sca/Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int color_dodge_op(int dst, int src, int da, int sa)
{
    const int sa_da = sa * da;
    const int dst_sa = dst * sa;
    const int src_da = src * da;

    const int temp = src * (255 - da) + dst * (255 - sa);
    if (src_da + dst_sa >= sa_da)
        return qt_div_255(sa_da + temp);
    else
        return qt_div_255(255 * dst_sa / (255 - 255 * src / sa) + temp);
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_ColorDodge_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a,b) color_dodge_op(a, b, da, sa)
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_ColorDodge(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_ColorDodge_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_ColorDodge_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_ColorDodge_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) color_dodge_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_ColorDodge(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_ColorDodge_impl(dest, src, length, QFullCoverage());
    else
        comp_func_ColorDodge_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
   if Sca.Da + Dca.Sa <= Sa.Da
       Dca' = Sca.(1 - Da) + Dca.(1 - Sa)
   otherwise
       Dca' = Sa.(Sca.Da + Dca.Sa - Sa.Da)/Sca + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int color_burn_op(int dst, int src, int da, int sa)
{
    const int src_da = src * da;
    const int dst_sa = dst * sa;
    const int sa_da = sa * da;

    const int temp = src * (255 - da) + dst * (255 - sa);

    if (src == 0 || src_da + dst_sa <= sa_da)
        return qt_div_255(temp);
    return qt_div_255(sa * (src_da + dst_sa - sa_da) / src + temp);
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_ColorBurn_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) color_burn_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_ColorBurn(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_ColorBurn_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_ColorBurn_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_ColorBurn_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) color_burn_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_ColorBurn(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_ColorBurn_impl(dest, src, length, QFullCoverage());
    else
        comp_func_ColorBurn_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    if 2.Sca < Sa
        Dca' = 2.Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
    otherwise
        Dca' = Sa.Da - 2.(Da - Dca).(Sa - Sca) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline uint hardlight_op(int dst, int src, int da, int sa)
{
    const uint temp = src * (255 - da) + dst * (255 - sa);

    if (2 * src < sa)
        return qt_div_255(2 * src * dst + temp);
    else
        return qt_div_255(sa * da - 2 * (da - dst) * (sa - src) + temp);
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_HardLight_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) hardlight_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_HardLight(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_HardLight_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_HardLight_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_HardLight_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) hardlight_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_HardLight(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_HardLight_impl(dest, src, length, QFullCoverage());
    else
        comp_func_HardLight_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    if 2.Sca <= Sa
        Dca' = Dca.(Sa + (2.Sca - Sa).(1 - Dca/Da)) + Sca.(1 - Da) + Dca.(1 - Sa)
    otherwise if 2.Sca > Sa and 4.Dca <= Da
        Dca' = Dca.Sa + Da.(2.Sca - Sa).(4.Dca/Da.(4.Dca/Da + 1).(Dca/Da - 1) + 7.Dca/Da) + Sca.(1 - Da) + Dca.(1 - Sa)
    otherwise if 2.Sca > Sa and 4.Dca > Da
        Dca' = Dca.Sa + Da.(2.Sca - Sa).((Dca/Da)^0.5 - Dca/Da) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int soft_light_op(int dst, int src, int da, int sa)
{
    const int src2 = src << 1;
    const int dst_np = da != 0 ? (255 * dst) / da : 0;
    const int temp = (src * (255 - da) + dst * (255 - sa)) * 255;

    if (src2 < sa)
        return (dst * (sa * 255 + (src2 - sa) * (255 - dst_np)) + temp) / 65025;
    else if (4 * dst <= da)
        return (dst * sa * 255 + da * (src2 - sa) * ((((16 * dst_np - 12 * 255) * dst_np + 3 * 65025) * dst_np) / 65025) + temp) / 65025;
    else {
        return (dst * sa * 255 + da * (src2 - sa) * (int(qSqrt(qreal(dst_np * 255))) - dst_np) + temp) / 65025;
    }
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_SoftLight_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) soft_light_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_SoftLight(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_SoftLight_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_SoftLight_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_SoftLight_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) soft_light_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_SoftLight(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_SoftLight_impl(dest, src, length, QFullCoverage());
    else
        comp_func_SoftLight_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
   Dca' = abs(Dca.Sa - Sca.Da) + Sca.(1 - Da) + Dca.(1 - Sa)
        = Sca + Dca - 2.min(Sca.Da, Dca.Sa)
*/
static inline int difference_op(int dst, int src, int da, int sa)
{
    return src + dst - qt_div_255(2 * qMin(src * da, dst * sa));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Difference_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) difference_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Difference(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Difference_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Difference_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Difference_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) difference_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Difference(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Difference_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Difference_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    Dca' = (Sca.Da + Dca.Sa - 2.Sca.Dca) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void QT_FASTCALL comp_func_solid_Exclusion_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) (a + b - qt_div_255(2*(a*b)))
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Exclusion(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Exclusion_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Exclusion_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Exclusion_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) (a + b - ((a*b) >> 7))
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Exclusion(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Exclusion_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Exclusion_impl(dest, src, length, QPartialCoverage(const_alpha));
}

void QT_FASTCALL rasterop_solid_SourceOrDestination(uint *dest,
                                                    int length,
                                                    uint color,
                                                    uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--)
        *dest++ |= color;
}

void QT_FASTCALL rasterop_SourceOrDestination(uint *Q_DECL_RESTRICT dest,
                                              const uint *Q_DECL_RESTRICT src,
                                              int length,
                                              uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--)
        *dest++ |= *src++;
}

void QT_FASTCALL rasterop_solid_SourceAndDestination(uint *dest,
                                                     int length,
                                                     uint color,
                                                     uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color |= 0xff000000;
    while (length--)
        *dest++ &= color;
}

void QT_FASTCALL rasterop_SourceAndDestination(uint *Q_DECL_RESTRICT dest,
                                               const uint *Q_DECL_RESTRICT src,
                                               int length,
                                               uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (*src & *dest) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_SourceXorDestination(uint *dest,
                                                     int length,
                                                     uint color,
                                                     uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color &= 0x00ffffff;
    while (length--)
        *dest++ ^= color;
}

void QT_FASTCALL rasterop_SourceXorDestination(uint *Q_DECL_RESTRICT dest,
                                               const uint *Q_DECL_RESTRICT src,
                                               int length,
                                               uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (*src ^ *dest) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_NotSourceAndNotDestination(uint *dest,
                                                           int length,
                                                           uint color,
                                                           uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color = ~color;
    while (length--) {
        *dest = (color & ~(*dest)) | 0xff000000;
        ++dest;
    }
}

void QT_FASTCALL rasterop_NotSourceAndNotDestination(uint *Q_DECL_RESTRICT dest,
                                                     const uint *Q_DECL_RESTRICT src,
                                                     int length,
                                                     uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (~(*src) & ~(*dest)) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_NotSourceOrNotDestination(uint *dest,
                                                          int length,
                                                          uint color,
                                                          uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color = ~color | 0xff000000;
    while (length--) {
        *dest = color | ~(*dest);
        ++dest;
    }
}

void QT_FASTCALL rasterop_NotSourceOrNotDestination(uint *Q_DECL_RESTRICT dest,
                                                    const uint *Q_DECL_RESTRICT src,
                                                    int length,
                                                    uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = ~(*src) | ~(*dest) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_NotSourceXorDestination(uint *dest,
                                                        int length,
                                                        uint color,
                                                        uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color = ~color & 0x00ffffff;
    while (length--) {
        *dest = color ^ (*dest);
        ++dest;
    }
}

void QT_FASTCALL rasterop_NotSourceXorDestination(uint *Q_DECL_RESTRICT dest,
                                                  const uint *Q_DECL_RESTRICT src,
                                                  int length,
                                                  uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = ((~(*src)) ^ (*dest)) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_NotSource(uint *dest, int length,
                                          uint color, uint const_alpha)
{
    Q_UNUSED(const_alpha);
    qt_memfill(dest, ~color | 0xff000000, length);
}

void QT_FASTCALL rasterop_NotSource(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src,
                                    int length, uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--)
        *dest++ = ~(*src++) | 0xff000000;
}

void QT_FASTCALL rasterop_solid_NotSourceAndDestination(uint *dest,
                                                        int length,
                                                        uint color,
                                                        uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color = ~color | 0xff000000;
    while (length--) {
        *dest = color & *dest;
        ++dest;
    }
}

void QT_FASTCALL rasterop_NotSourceAndDestination(uint *Q_DECL_RESTRICT dest,
                                                  const uint *Q_DECL_RESTRICT src,
                                                  int length,
                                                  uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (~(*src) & *dest) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_SourceAndNotDestination(uint *dest,
                                                        int length,
                                                        uint color,
                                                        uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (color & ~(*dest)) | 0xff000000;
        ++dest;
    }
}

void QT_FASTCALL rasterop_SourceAndNotDestination(uint *Q_DECL_RESTRICT dest,
                                                  const uint *Q_DECL_RESTRICT src,
                                                  int length,
                                                  uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (*src & ~(*dest)) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_NotSourceOrDestination(uint *Q_DECL_RESTRICT dest,
                                                 const uint *Q_DECL_RESTRICT src,
                                                 int length,
                                                 uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (~(*src) | *dest) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_NotSourceOrDestination(uint *Q_DECL_RESTRICT dest,
                                                       int length,
                                                       uint color,
                                                       uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color = ~color | 0xff000000;
    while (length--)
        *dest++ |= color;
}

void QT_FASTCALL rasterop_SourceOrNotDestination(uint *Q_DECL_RESTRICT dest,
                                                 const uint *Q_DECL_RESTRICT src,
                                                 int length,
                                                 uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (*src | ~(*dest)) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_SourceOrNotDestination(uint *Q_DECL_RESTRICT dest,
                                                       int length,
                                                       uint color,
                                                       uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (color | ~(*dest)) | 0xff000000;
        ++dest;
    }
}

void QT_FASTCALL rasterop_ClearDestination(uint *Q_DECL_RESTRICT dest,
                                           const uint *Q_DECL_RESTRICT src,
                                           int length,
                                           uint const_alpha)
{
    Q_UNUSED(src);
    comp_func_solid_SourceOver (dest, length, 0xff000000, const_alpha);
}

void QT_FASTCALL rasterop_solid_ClearDestination(uint *Q_DECL_RESTRICT dest,
                                                 int length,
                                                 uint color,
                                                 uint const_alpha)
{
    Q_UNUSED(color);
    comp_func_solid_SourceOver (dest, length, 0xff000000, const_alpha);
}

void QT_FASTCALL rasterop_SetDestination(uint *Q_DECL_RESTRICT dest,
                                         const uint *Q_DECL_RESTRICT src,
                                         int length,
                                         uint const_alpha)
{
    Q_UNUSED(src);
    comp_func_solid_SourceOver (dest, length, 0xffffffff, const_alpha);
}

void QT_FASTCALL rasterop_solid_SetDestination(uint *Q_DECL_RESTRICT dest,
                                               int length,
                                               uint color,
                                               uint const_alpha)
{
    Q_UNUSED(color);
    comp_func_solid_SourceOver (dest, length, 0xffffffff, const_alpha);
}

void QT_FASTCALL rasterop_NotDestination(uint *Q_DECL_RESTRICT dest,
                                         const uint *Q_DECL_RESTRICT src,
                                         int length,
                                         uint const_alpha)
{
    Q_UNUSED(src);
    rasterop_solid_SourceXorDestination (dest, length, 0x00ffffff, const_alpha);
}

void QT_FASTCALL rasterop_solid_NotDestination(uint *Q_DECL_RESTRICT dest,
                                               int length,
                                               uint color,
                                               uint const_alpha)
{
    Q_UNUSED(color);
    rasterop_solid_SourceXorDestination (dest, length, 0x00ffffff, const_alpha);
}

CompositionFunctionSolid qt_functionForModeSolid_C[] = {
        comp_func_solid_SourceOver,
        comp_func_solid_DestinationOver,
        comp_func_solid_Clear,
        comp_func_solid_Source,
        comp_func_solid_Destination,
        comp_func_solid_SourceIn,
        comp_func_solid_DestinationIn,
        comp_func_solid_SourceOut,
        comp_func_solid_DestinationOut,
        comp_func_solid_SourceAtop,
        comp_func_solid_DestinationAtop,
        comp_func_solid_XOR,
        comp_func_solid_Plus,
        comp_func_solid_Multiply,
        comp_func_solid_Screen,
        comp_func_solid_Overlay,
        comp_func_solid_Darken,
        comp_func_solid_Lighten,
        comp_func_solid_ColorDodge,
        comp_func_solid_ColorBurn,
        comp_func_solid_HardLight,
        comp_func_solid_SoftLight,
        comp_func_solid_Difference,
        comp_func_solid_Exclusion,
        rasterop_solid_SourceOrDestination,
        rasterop_solid_SourceAndDestination,
        rasterop_solid_SourceXorDestination,
        rasterop_solid_NotSourceAndNotDestination,
        rasterop_solid_NotSourceOrNotDestination,
        rasterop_solid_NotSourceXorDestination,
        rasterop_solid_NotSource,
        rasterop_solid_NotSourceAndDestination,
        rasterop_solid_SourceAndNotDestination,
        rasterop_solid_NotSourceOrDestination,
        rasterop_solid_SourceOrNotDestination,
        rasterop_solid_ClearDestination,
        rasterop_solid_SetDestination,
        rasterop_solid_NotDestination
};

CompositionFunctionSolid64 qt_functionForModeSolid64_C[] = {
        comp_func_solid_SourceOver_rgb64,
        comp_func_solid_DestinationOver_rgb64,
        comp_func_solid_Clear_rgb64,
        comp_func_solid_Source_rgb64,
        comp_func_solid_Destination_rgb64,
        comp_func_solid_SourceIn_rgb64,
        comp_func_solid_DestinationIn_rgb64,
        comp_func_solid_SourceOut_rgb64,
        comp_func_solid_DestinationOut_rgb64,
        comp_func_solid_SourceAtop_rgb64,
        comp_func_solid_DestinationAtop_rgb64,
        comp_func_solid_XOR_rgb64,
        comp_func_solid_Plus_rgb64,
        0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
};

CompositionFunction qt_functionForMode_C[] = {
        comp_func_SourceOver,
        comp_func_DestinationOver,
        comp_func_Clear,
        comp_func_Source,
        comp_func_Destination,
        comp_func_SourceIn,
        comp_func_DestinationIn,
        comp_func_SourceOut,
        comp_func_DestinationOut,
        comp_func_SourceAtop,
        comp_func_DestinationAtop,
        comp_func_XOR,
        comp_func_Plus,
        comp_func_Multiply,
        comp_func_Screen,
        comp_func_Overlay,
        comp_func_Darken,
        comp_func_Lighten,
        comp_func_ColorDodge,
        comp_func_ColorBurn,
        comp_func_HardLight,
        comp_func_SoftLight,
        comp_func_Difference,
        comp_func_Exclusion,
        rasterop_SourceOrDestination,
        rasterop_SourceAndDestination,
        rasterop_SourceXorDestination,
        rasterop_NotSourceAndNotDestination,
        rasterop_NotSourceOrNotDestination,
        rasterop_NotSourceXorDestination,
        rasterop_NotSource,
        rasterop_NotSourceAndDestination,
        rasterop_SourceAndNotDestination,
        rasterop_NotSourceOrDestination,
        rasterop_SourceOrNotDestination,
        rasterop_ClearDestination,
        rasterop_SetDestination,
        rasterop_NotDestination
};

CompositionFunction64 qt_functionForMode64_C[] = {
        comp_func_SourceOver_rgb64,
        comp_func_DestinationOver_rgb64,
        comp_func_Clear_rgb64,
        comp_func_Source_rgb64,
        comp_func_Destination_rgb64,
        comp_func_SourceIn_rgb64,
        comp_func_DestinationIn_rgb64,
        comp_func_SourceOut_rgb64,
        comp_func_DestinationOut_rgb64,
        comp_func_SourceAtop_rgb64,
        comp_func_DestinationAtop_rgb64,
        comp_func_XOR_rgb64,
        comp_func_Plus_rgb64,
        0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
};

QT_END_NAMESPACE
