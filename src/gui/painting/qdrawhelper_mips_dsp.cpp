/****************************************************************************
**
** Copyright (C) 2012 MIPS Technologies, www.mips.com, author Damir Tatalovic <dtatalovic@mips.com>
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <private/qdrawhelper_p.h>
#include <private/qdrawhelper_mips_dsp_p.h>
#include <private/qpaintengine_raster_p.h>

QT_BEGIN_NAMESPACE

#if defined(QT_COMPILER_SUPPORTS_MIPS_DSP)

extern "C" uint INTERPOLATE_PIXEL_255_asm_mips_dsp(uint x, uint a, uint y, uint b);

extern "C"  uint BYTE_MUL_asm_mips_dsp(uint x, uint a);

extern "C" uint * destfetchARGB32_asm_mips_dsp(uint *buffer, const uint *data, int length);

extern "C" uint * qt_destStoreARGB32_asm_mips_dsp(uint *buffer, const uint *data, int length);

#if defined(QT_COMPILER_SUPPORTS_MIPS_DSPR2)

extern "C" uint INTERPOLATE_PIXEL_255_asm_mips_dspr2(uint x, uint a, uint y, uint b);

extern "C" uint BYTE_MUL_asm_mips_dspr2(uint x, uint a);

#endif // QT_COMPILER_SUPPORTS_MIPS_DSPR2

void qt_blend_argb32_on_argb32_mips_dsp(uchar *destPixels, int dbpl,
                                      const uchar *srcPixels, int sbpl,
                                      int w, int h,
                                      int const_alpha)

{
#ifdef QT_DEBUG_DRAW
    fprintf(stdout,
            "qt_blend_argb32_on_argb32: dst=(%p, %d), src=(%p, %d), dim=(%d, %d) alpha=%d\n",
            destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
    fflush(stdout);
#endif

    const uint *src = (const uint *) srcPixels;
    uint *dst = (uint *) destPixels;
    if (const_alpha == 256) {
        for (int y=0; y<h; ++y) {
            for (int x=0; x<w; ++x) {
                uint s = src[x];
                if (s >= 0xff000000)
                    dst[x] = s;
                else if (s != 0)
#if !defined(QT_COMPILER_SUPPORTS_MIPS_DSPR2)
                    dst[x] = s + BYTE_MUL_asm_mips_dsp(dst[x], qAlpha(~s));
#else
                    dst[x] = s + BYTE_MUL_asm_mips_dspr2(dst[x], qAlpha(~s));
#endif
            }
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    } else if (const_alpha != 0) {
        const_alpha = (const_alpha * 255) >> 8;
        for (int y=0; y<h; ++y) {
            for (int x=0; x<w; ++x) {
#if !defined(QT_COMPILER_SUPPORTS_MIPS_DSPR2)
                uint s = BYTE_MUL_asm_mips_dsp(src[x], const_alpha);
                dst[x] = s + BYTE_MUL_asm_mips_dsp(dst[x], qAlpha(~s));
#else
                uint s = BYTE_MUL_asm_mips_dspr2(src[x], const_alpha);
                dst[x] = s + BYTE_MUL_asm_mips_dspr2(dst[x], qAlpha(~s));
#endif
            }
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    }
}

void qt_blend_rgb32_on_rgb32_mips_dsp(uchar *destPixels, int dbpl,
                                    const uchar *srcPixels, int sbpl,
                                    int w, int h,
                                    int const_alpha)
{
#ifdef QT_DEBUG_DRAW
    fprintf(stdout,
            "qt_blend_rgb32_on_rgb32: dst=(%p, %d), src=(%p, %d), dim=(%d, %d) alpha=%d\n",
            destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
    fflush(stdout);
#endif

    if (const_alpha != 256) {
        qt_blend_argb32_on_argb32_mips_dsp(destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
        return;
    }

    const uint *src = (const uint *) srcPixels;
    uint *dst = (uint *) destPixels;
    int len = w * 4;
    for (int y=0; y<h; ++y) {
        memcpy(dst, src, len);
        dst = (quint32 *)(((uchar *) dst) + dbpl);
        src = (const quint32 *)(((const uchar *) src) + sbpl);
    }
}

void comp_func_Source_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        ::memcpy(dest, src, length * sizeof(uint));
    } else {
        int ialpha = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
#if !defined(QT_COMPILER_SUPPORTS_MIPS_DSPR2)
            dest[i] = INTERPOLATE_PIXEL_255_asm_mips_dsp(src[i], const_alpha, dest[i], ialpha);
#else
            dest[i] = INTERPOLATE_PIXEL_255_asm_mips_dspr2(src[i], const_alpha, dest[i], ialpha);
#endif
        }
    }
}

uint * QT_FASTCALL qt_destFetchARGB32_mips_dsp(uint *buffer,
                                          QRasterBuffer *rasterBuffer,
                                          int x, int y, int length)
{
    const uint *data = (const uint *)rasterBuffer->scanLine(y) + x;
    buffer = destfetchARGB32_asm_mips_dsp(buffer, data, length);
    return buffer;
}

void QT_FASTCALL qt_destStoreARGB32_mips_dsp(QRasterBuffer *rasterBuffer, int x, int y,
                                             const uint *buffer, int length)
{
    uint *data = (uint *)rasterBuffer->scanLine(y) + x;
    qt_destStoreARGB32_asm_mips_dsp(data, buffer, length);
}

#endif // QT_COMPILER_SUPPORTS_MIPS_DSP

QT_END_NAMESPACE
