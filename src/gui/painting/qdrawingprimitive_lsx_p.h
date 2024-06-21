// Copyright (C) 2024 Loongson Technology Corporation Limited.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDRAWINGPRIMITIVE_LSX_P_H
#define QDRAWINGPRIMITIVE_LSX_P_H

#include <QtGui/private/qtguiglobal_p.h>
#include <private/qsimd_p.h>
#include "qdrawhelper_loongarch64_p.h"
#include "qrgba64_p.h"

#ifdef __loongarch_sx

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

/*
 * Multiply the components of pixelVector by alphaChannel
 * Each 32bits components of alphaChannel must be in the form 0x00AA00AA
 * colorMask must have 0x00ff00ff on each 32 bits component
 * half must have the value 128 (0x80) for each 32 bits component
 */
inline static void Q_DECL_VECTORCALL
BYTE_MUL_LSX(__m128i &pixelVector, __m128i alphaChannel, __m128i colorMask, __m128i half)
{
    /* 1. separate the colors in 2 vectors so each color is on 16 bits
       (in order to be multiplied by the alpha
       each 32 bit of dstVectorAG are in the form 0x00AA00GG
       each 32 bit of dstVectorRB are in the form 0x00RR00BB */
    __m128i pixelVectorAG = __lsx_vsrli_h(pixelVector, 8);
    __m128i pixelVectorRB = __lsx_vand_v(pixelVector, colorMask);

    /* 2. multiply the vectors by the alpha channel */
    pixelVectorAG = __lsx_vmul_h(pixelVectorAG, alphaChannel);
    pixelVectorRB = __lsx_vmul_h(pixelVectorRB, alphaChannel);

    /* 3. divide by 255, that's the tricky part.
       we do it like for BYTE_MUL(), with bit shift: X/255 ~= (X + X/256 + rounding)/256 */
    /** so first (X + X/256 + rounding) */
    pixelVectorRB = __lsx_vadd_h(pixelVectorRB, __lsx_vsrli_h(pixelVectorRB, 8));
    pixelVectorRB = __lsx_vadd_h(pixelVectorRB, half);
    pixelVectorAG = __lsx_vadd_h(pixelVectorAG, __lsx_vsrli_h(pixelVectorAG, 8));
    pixelVectorAG = __lsx_vadd_h(pixelVectorAG, half);

    /** second divide by 256 */
    pixelVectorRB = __lsx_vsrli_h(pixelVectorRB, 8);
    /** for AG, we could >> 8 to divide followed by << 8 to put the
        bytes in the correct position. By masking instead, we execute
        only one instruction */
    pixelVectorAG = __lsx_vandn_v(colorMask, pixelVectorAG);

    /* 4. combine the 2 pairs of colors */
    pixelVector = __lsx_vor_v(pixelVectorAG, pixelVectorRB);
}

/*
 * Each 32bits components of alphaChannel must be in the form 0x00AA00AA
 * oneMinusAlphaChannel must be 255 - alpha for each 32 bits component
 * colorMask must have 0x00ff00ff on each 32 bits component
 * half must have the value 128 (0x80) for each 32 bits component
 */
inline static void Q_DECL_VECTORCALL
INTERPOLATE_PIXEL_255_LSX(__m128i srcVector, __m128i &dstVector, __m128i alphaChannel,
                          __m128i oneMinusAlphaChannel, __m128i colorMask, __m128i half)
{
    /* interpolate AG */
    __m128i srcVectorAG = __lsx_vsrli_h(srcVector, 8);
    __m128i dstVectorAG = __lsx_vsrli_h(dstVector, 8);
    __m128i srcVectorAGalpha = __lsx_vmul_h(srcVectorAG, alphaChannel);
    __m128i dstVectorAGoneMinusAlphalpha = __lsx_vmul_h(dstVectorAG, oneMinusAlphaChannel);
    __m128i finalAG = __lsx_vadd_h(srcVectorAGalpha, dstVectorAGoneMinusAlphalpha);
    finalAG = __lsx_vadd_h(finalAG, __lsx_vsrli_h(finalAG, 8));
    finalAG = __lsx_vadd_h(finalAG, half);
    finalAG = __lsx_vandn_v(colorMask, finalAG);

    /* interpolate RB */
    __m128i srcVectorRB = __lsx_vand_v(srcVector, colorMask);
    __m128i dstVectorRB = __lsx_vand_v(dstVector, colorMask);
    __m128i srcVectorRBalpha = __lsx_vmul_h(srcVectorRB, alphaChannel);
    __m128i dstVectorRBoneMinusAlphalpha = __lsx_vmul_h(dstVectorRB, oneMinusAlphaChannel);
    __m128i finalRB = __lsx_vadd_h(srcVectorRBalpha, dstVectorRBoneMinusAlphalpha);
    finalRB = __lsx_vadd_h(finalRB, __lsx_vsrli_h(finalRB, 8));
    finalRB = __lsx_vadd_h(finalRB, half);
    finalRB = __lsx_vsrli_h(finalRB, 8);

    /* combine */
    dstVector = __lsx_vor_v(finalAG, finalRB);
}

// same as BLEND_SOURCE_OVER_ARGB32_LSX, but for one vector srcVector
inline static void Q_DECL_VECTORCALL
BLEND_SOURCE_OVER_ARGB32_LSX_helper(quint32 *dst, int x, __m128i srcVector,
                                    __m128i nullVector, __m128i half, __m128i one,
                                    __m128i colorMask, __m128i alphaMask)
{
    const __m128i srcVectorAlpha = __lsx_vand_v(srcVector, alphaMask);
    __m128i vseq = __lsx_vseq_w(srcVectorAlpha, alphaMask);
    v4i32 vseq_res = (v4i32)__lsx_vmsknz_b(vseq);
    if (vseq_res[0] == (0x0000ffff)) {
        /* all opaque */
        __lsx_vst(srcVector, &dst[x], 0);
    } else {
        __m128i vseq_n = __lsx_vseq_w(srcVectorAlpha, nullVector);
        v4i32 vseq_n_res = (v4i32)__lsx_vmsknz_b(vseq_n);
        if (vseq_n_res[0] != (0x0000ffff)) {
            /* not fully transparent */
            /* extract the alpha channel on 2 x 16 bits */
            /* so we have room for the multiplication */
            /* each 32 bits will be in the form 0x00AA00AA */
            /* with A being the 1 - alpha */
            __m128i alphaChannel = __lsx_vsrli_w(srcVector, 24);
            alphaChannel = __lsx_vor_v(alphaChannel, __lsx_vslli_w(alphaChannel, 16));
            alphaChannel = __lsx_vsub_h(one, alphaChannel);

            __m128i dstVector = __lsx_vld(&dst[x], 0);
            BYTE_MUL_LSX(dstVector, alphaChannel, colorMask, half);

            /* result = s + d * (1-alpha) */
            const __m128i result = __lsx_vadd_b(srcVector, dstVector);
            __lsx_vst(result, &dst[x], 0);
        }
    }
}

// Basically blend src over dst with the const alpha defined as constAlphaVector.
// nullVector, half, one, colorMask are constant across the whole image/texture, and should be defined as:
//const __m128i nullVector = __lsx_vreplgr2vr_w(0);
//const __m128i half = __lsx_vreplgr2vr_h(0x80);
//const __m128i one = __lsx_vreplgr2vr_h(0xff);
//const __m128i colorMask = __lsx_vreplgr2vr_w(0x00ff00ff);
//const __m128i alphaMask = __lsx_vreplgr2vr_w(0xff000000);
//
// The computation being done is:
// result = s + d * (1-alpha)
// with shortcuts if fully opaque or fully transparent.
inline static void Q_DECL_VECTORCALL
BLEND_SOURCE_OVER_ARGB32_LSX(quint32 *dst, const quint32 *src, int length)
{
    int x = 0;

    /* First, get dst aligned. */
    ALIGNMENT_PROLOGUE_16BYTES(dst, x, length) {
        blend_pixel(dst[x], src[x]);
    }

    const __m128i alphaMask = __lsx_vreplgr2vr_w(0xff000000);
    const __m128i nullVector = __lsx_vreplgr2vr_w(0);
    const __m128i half = __lsx_vreplgr2vr_h(0x80);
    const __m128i one = __lsx_vreplgr2vr_h(0xff);
    const __m128i colorMask = __lsx_vreplgr2vr_w(0x00ff00ff);

    for (; x < length-3; x += 4) {
        const __m128i srcVector = __lsx_vld((const __m128i *)&src[x], 0);
        BLEND_SOURCE_OVER_ARGB32_LSX_helper(dst, x, srcVector, nullVector, half, one, colorMask, alphaMask);
    }
    SIMD_EPILOGUE(x, length, 3) {
        blend_pixel(dst[x], src[x]);
    }
}

// Basically blend src over dst with the const alpha defined as constAlphaVector.
// The computation being done is:
// dest = (s + d * sia) * ca + d * cia
//      = s * ca + d * (sia * ca + cia)
//      = s * ca + d * (1 - sa*ca)
inline static void Q_DECL_VECTORCALL
BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_LSX(quint32 *dst, const quint32 *src, int length, uint const_alpha)
{
    int x = 0;

    ALIGNMENT_PROLOGUE_16BYTES(dst, x, length) {
        blend_pixel(dst[x], src[x], const_alpha);
    }

    const __m128i nullVector = __lsx_vreplgr2vr_w(0);
    const __m128i half = __lsx_vreplgr2vr_h(0x80);
    const __m128i one = __lsx_vreplgr2vr_h(0xff);
    const __m128i colorMask = __lsx_vreplgr2vr_w(0x00ff00ff);
    const __m128i constAlphaVector = __lsx_vreplgr2vr_h(const_alpha);

    for (; x < length-3; x += 4) {
        __m128i srcVector = __lsx_vld((const __m128i *)&src[x], 0);
        __m128i vseq = __lsx_vseq_w(srcVector, nullVector);
        v4i32 vseq_res = (v4i32)__lsx_vmsknz_b(vseq);
        if (vseq_res[0] != 0x0000ffff) {
            BYTE_MUL_LSX(srcVector, constAlphaVector, colorMask, half);

            __m128i alphaChannel = __lsx_vsrli_w(srcVector, 24);
            alphaChannel = __lsx_vor_v(alphaChannel, __lsx_vslli_w(alphaChannel, 16));
            alphaChannel = __lsx_vsub_h(one, alphaChannel);

            __m128i dstVector = __lsx_vld((__m128i *)&dst[x], 0);
            BYTE_MUL_LSX(dstVector, alphaChannel, colorMask, half);

            const __m128i result = __lsx_vadd_b(srcVector, dstVector);
            __lsx_vst(result, &dst[x], 0);
        }
    }
    SIMD_EPILOGUE(x, length, 3) {
        blend_pixel(dst[x], src[x], const_alpha);
    }
}

typedef union
{
    int i;
    float f;
} FloatInt;

/* float type data load instructions */
static __m128 __lsx_vreplfr2vr_s(float val)
{
    FloatInt fi_tmpval = {.f = val};
    return (__m128)__lsx_vreplgr2vr_w(fi_tmpval.i);
}

QT_END_NAMESPACE

#endif // __loongarch_sx

#endif // QDRAWINGPRIMITIVE_LSX_P_H
