// Copyright (C) 2024 Loongson Technology Corporation Limited.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdrawhelper_p.h"
#include "qdrawhelper_loongarch64_p.h"
#include "qdrawingprimitive_lsx_p.h"
#include "qrgba64_p.h"

#if defined(QT_COMPILER_SUPPORTS_LASX)

QT_BEGIN_NAMESPACE

enum {
    FixedScale = 1 << 16,
    HalfPoint = 1 << 15
};

#ifdef Q_CC_CLANG
#define VREGS_PREFIX "$vr"
#define XREGS_PREFIX "$xr"
#else // GCC
#define VREGS_PREFIX "$f"
#define XREGS_PREFIX "$f"
#endif
#define __ALL_REGS "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31"

// Convert two __m128i to __m256i
static inline __m256i lasx_set_q(__m128i inhi, __m128i inlo)
{
    __m256i out;
    __asm__ volatile (
        ".irp i," __ALL_REGS                "\n\t"
        " .ifc %[hi], " VREGS_PREFIX "\\i    \n\t"
        "  .irp j," __ALL_REGS              "\n\t"
        "   .ifc %[lo], " VREGS_PREFIX "\\j  \n\t"
        "    xvpermi.q $xr\\i, $xr\\j, 0x20  \n\t"
        "   .endif                           \n\t"
        "  .endr                             \n\t"
        " .endif                             \n\t"
        ".endr                               \n\t"
        ".ifnc %[out], %[hi]                 \n\t"
        ".irp i," __ALL_REGS                "\n\t"
        " .ifc %[out], " XREGS_PREFIX "\\i   \n\t"
        "  .irp j," __ALL_REGS              "\n\t"
        "   .ifc %[hi], " VREGS_PREFIX "\\j  \n\t"
        "    xvori.b $xr\\i, $xr\\j, 0       \n\t"
        "   .endif                           \n\t"
        "  .endr                             \n\t"
        " .endif                             \n\t"
        ".endr                               \n\t"
        ".endif                              \n\t"
        : [out] "=f" (out), [hi] "+f" (inhi)
        : [lo] "f" (inlo)
    );
    return out;
}

// Convert __m256i low part to __m128i
static inline __m128i lasx_extracti128_lo(__m256i in)
{
    __m128i out;
    __asm__ volatile (
        ".ifnc %[out], %[in]                 \n\t"
        ".irp i," __ALL_REGS                "\n\t"
        " .ifc %[out], " VREGS_PREFIX "\\i   \n\t"
        "  .irp j," __ALL_REGS              "\n\t"
        "   .ifc %[in], " XREGS_PREFIX "\\j  \n\t"
        "    vori.b $vr\\i, $vr\\j, 0        \n\t"
        "   .endif                           \n\t"
        "  .endr                             \n\t"
        " .endif                             \n\t"
        ".endr                               \n\t"
        ".endif                              \n\t"
        : [out] "=f" (out) : [in] "f" (in)
    );
    return out;
}

// Convert __m256i high part to __m128i
static inline __m128i lasx_extracti128_hi(__m256i in)
{
    __m128i out;
    __asm__ volatile (
        ".irp i," __ALL_REGS                "\n\t"
        " .ifc %[out], " VREGS_PREFIX "\\i   \n\t"
        "  .irp j," __ALL_REGS              "\n\t"
        "   .ifc %[in], " XREGS_PREFIX "\\j  \n\t"
        "    xvpermi.q $xr\\i, $xr\\j, 0x11  \n\t"
        "   .endif                           \n\t"
        "  .endr                             \n\t"
        " .endif                             \n\t"
        ".endr                               \n\t"
        : [out] "=f" (out) : [in] "f" (in)
    );
    return out;
}

// Vectorized blend functions:

// See BYTE_MUL_LSX for details.
inline static void Q_DECL_VECTORCALL
BYTE_MUL_LASX(__m256i &pixelVector, __m256i alphaChannel, __m256i colorMask, __m256i half)
{
    __m256i pixelVectorAG = __lasx_xvsrli_h(pixelVector, 8);
    __m256i pixelVectorRB = __lasx_xvand_v(pixelVector, colorMask);

    pixelVectorAG = __lasx_xvmul_h(pixelVectorAG, alphaChannel);
    pixelVectorRB = __lasx_xvmul_h(pixelVectorRB, alphaChannel);

    pixelVectorRB = __lasx_xvadd_h(pixelVectorRB, __lasx_xvsrli_h(pixelVectorRB, 8));
    pixelVectorRB = __lasx_xvadd_h(pixelVectorRB, half);
    pixelVectorAG = __lasx_xvadd_h(pixelVectorAG, __lasx_xvsrli_h(pixelVectorAG, 8));
    pixelVectorAG = __lasx_xvadd_h(pixelVectorAG, half);

    pixelVectorRB = __lasx_xvsrli_h(pixelVectorRB, 8);
    pixelVectorAG = __lasx_xvandn_v(colorMask, pixelVectorAG);

    pixelVector = __lasx_xvor_v(pixelVectorAG, pixelVectorRB);
}

inline static void Q_DECL_VECTORCALL
BYTE_MUL_RGB64_LASX(__m256i &pixelVector, __m256i alphaChannel, __m256i colorMask, __m256i half)
{
    __m256i pixelVectorAG = __lasx_xvsrli_w(pixelVector, 16);
    __m256i pixelVectorRB = __lasx_xvand_v(pixelVector, colorMask);

    pixelVectorAG = __lasx_xvmul_w(pixelVectorAG, alphaChannel);
    pixelVectorRB = __lasx_xvmul_w(pixelVectorRB, alphaChannel);

    pixelVectorRB = __lasx_xvadd_w(pixelVectorRB, __lasx_xvsrli_w(pixelVectorRB, 16));
    pixelVectorAG = __lasx_xvadd_w(pixelVectorAG, __lasx_xvsrli_w(pixelVectorAG, 16));
    pixelVectorRB = __lasx_xvadd_w(pixelVectorRB, half);
    pixelVectorAG = __lasx_xvadd_w(pixelVectorAG, half);

    pixelVectorRB = __lasx_xvsrli_w(pixelVectorRB, 16);
    pixelVectorAG = __lasx_xvandn_v(colorMask, pixelVectorAG);

    pixelVector = __lasx_xvor_v(pixelVectorAG, pixelVectorRB);
}

// See INTERPOLATE_PIXEL_255_LSX for details.
inline static void Q_DECL_VECTORCALL
INTERPOLATE_PIXEL_255_LASX(__m256i srcVector, __m256i &dstVector, __m256i alphaChannel,
                           __m256i oneMinusAlphaChannel, __m256i colorMask, __m256i half)
{
    const __m256i srcVectorAG = __lasx_xvsrli_h(srcVector, 8);
    const __m256i dstVectorAG = __lasx_xvsrli_h(dstVector, 8);
    const __m256i srcVectorRB = __lasx_xvand_v(srcVector, colorMask);
    const __m256i dstVectorRB = __lasx_xvand_v(dstVector, colorMask);
    const __m256i srcVectorAGalpha = __lasx_xvmul_h(srcVectorAG, alphaChannel);
    const __m256i srcVectorRBalpha = __lasx_xvmul_h(srcVectorRB, alphaChannel);
    const __m256i dstVectorAGoneMinusAlpha = __lasx_xvmul_h(dstVectorAG, oneMinusAlphaChannel);
    const __m256i dstVectorRBoneMinusAlpha = __lasx_xvmul_h(dstVectorRB, oneMinusAlphaChannel);
    __m256i finalAG = __lasx_xvadd_h(srcVectorAGalpha, dstVectorAGoneMinusAlpha);
    __m256i finalRB = __lasx_xvadd_h(srcVectorRBalpha, dstVectorRBoneMinusAlpha);
    finalAG = __lasx_xvadd_h(finalAG, __lasx_xvsrli_h(finalAG, 8));
    finalRB = __lasx_xvadd_h(finalRB, __lasx_xvsrli_h(finalRB, 8));
    finalAG = __lasx_xvadd_h(finalAG, half);
    finalRB = __lasx_xvadd_h(finalRB, half);
    finalAG = __lasx_xvandn_v(colorMask, finalAG);
    finalRB = __lasx_xvsrli_h(finalRB, 8);

    dstVector = __lasx_xvor_v(finalAG, finalRB);
}

inline static void Q_DECL_VECTORCALL
INTERPOLATE_PIXEL_RGB64_LASX(__m256i srcVector, __m256i &dstVector, __m256i alphaChannel,
                             __m256i oneMinusAlphaChannel, __m256i colorMask, __m256i half)
{
    const __m256i srcVectorAG = __lasx_xvsrli_w(srcVector, 16);
    const __m256i dstVectorAG = __lasx_xvsrli_w(dstVector, 16);
    const __m256i srcVectorRB = __lasx_xvand_v(srcVector, colorMask);
    const __m256i dstVectorRB = __lasx_xvand_v(dstVector, colorMask);
    const __m256i srcVectorAGalpha = __lasx_xvmul_w(srcVectorAG, alphaChannel);
    const __m256i srcVectorRBalpha = __lasx_xvmul_w(srcVectorRB, alphaChannel);
    const __m256i dstVectorAGoneMinusAlpha = __lasx_xvmul_w(dstVectorAG, oneMinusAlphaChannel);
    const __m256i dstVectorRBoneMinusAlpha = __lasx_xvmul_w(dstVectorRB, oneMinusAlphaChannel);
    __m256i finalAG = __lasx_xvadd_w(srcVectorAGalpha, dstVectorAGoneMinusAlpha);
    __m256i finalRB = __lasx_xvadd_w(srcVectorRBalpha, dstVectorRBoneMinusAlpha);
    finalAG = __lasx_xvadd_w(finalAG, __lasx_xvsrli_w(finalAG, 16));
    finalRB = __lasx_xvadd_w(finalRB, __lasx_xvsrli_w(finalRB, 16));
    finalAG = __lasx_xvadd_w(finalAG, half);
    finalRB = __lasx_xvadd_w(finalRB, half);
    finalAG = __lasx_xvandn_v(colorMask, finalAG);
    finalRB = __lasx_xvsrli_w(finalRB, 16);

    dstVector = __lasx_xvor_v(finalAG, finalRB);
}

// See BLEND_SOURCE_OVER_ARGB32_LSX for details.
inline static void Q_DECL_VECTORCALL BLEND_SOURCE_OVER_ARGB32_LASX(quint32 *dst, const quint32 *src, const int length)
{
    const __m256i half = __lasx_xvreplgr2vr_h(0x80);
    const __m256i one = __lasx_xvreplgr2vr_h(0xff);
    const __m256i colorMask = __lasx_xvreplgr2vr_w(0x00ff00ff);
    const __m256i alphaMask = __lasx_xvreplgr2vr_w(0xff000000);
    const __m256i offsetMask = (__m256i)(v8i32){0, 1, 2, 3, 4, 5, 6, 7};
    const __m256i offsetMaskr = (__m256i)(v8i32){7, 6, 5, 4, 3, 2, 1, 0};
    const __m256i alphaShuffleMask = (__m256i)(v32u8){3, 0xff, 3, 0xff, 7, 0xff, 7, 0xff, 11, 0xff, 11, 0xff, 15, 0xff, 15, 0xff,
                                                      3, 0xff, 3, 0xff, 7, 0xff, 7, 0xff, 11, 0xff, 11, 0xff, 15, 0xff, 15, 0xff};

    const int minusOffsetToAlignDstOn32Bytes = (reinterpret_cast<quintptr>(dst) >> 2) & 0x7;

    int x = 0;
    // Prologue to handle all pixels until dst is 32-byte aligned in one step.
    if (minusOffsetToAlignDstOn32Bytes != 0 && x < (length - 7)) {
        const __m256i prologueMask = __lasx_xvsub_w(__lasx_xvreplgr2vr_w(minusOffsetToAlignDstOn32Bytes - 1), offsetMaskr);
        const __m256i prologueMask1 = __lasx_xvslti_w(prologueMask, 0);
        const __m256i srcVector = __lasx_xvbitsel_v(__lasx_xvldi(0),
                                                    __lasx_xvld((const int *)&src[x], 0),
                                                    prologueMask1);
        const __m256i prologueMask2 = __lasx_xvslti_b(prologueMask, 0);
        const __m256i prologueAlphaMask = __lasx_xvbitsel_v(__lasx_xvldi(0),
                                                            alphaMask,
                                                            prologueMask2);
        const v8i32 testz1 = (v8i32)__lasx_xvmsknz_b(__lasx_xvand_v(srcVector, prologueAlphaMask));

        if (testz1[0]!=0 || testz1[4]!=0) {
            const v8i32 testc1 = (v8i32)__lasx_xvmsknz_b(__lasx_xvandn_v(srcVector,
                                                                         prologueAlphaMask));
            __m256i dstVector = __lasx_xvld((int *)&dst[x], 0);
            if (testc1[0]==0 && testc1[4]==0) {
                __lasx_xvst(__lasx_xvbitsel_v(dstVector, srcVector, prologueMask1), (int *)&dst[x], 0);
            } else {
                __m256i alphaChannel = __lasx_xvshuf_b(__lasx_xvldi(0),
                                                       srcVector,
                                                       alphaShuffleMask);
                alphaChannel = __lasx_xvsub_h(one, alphaChannel);
                __m256i dstV = dstVector;
                BYTE_MUL_LASX(dstVector, alphaChannel, colorMask, half);
                dstVector = __lasx_xvadd_b(dstVector, srcVector);
                __lasx_xvst(__lasx_xvbitsel_v(dstV, dstVector, prologueMask1), (int *)&dst[x], 0);
            }
        }
        x += (8 - minusOffsetToAlignDstOn32Bytes);
    }

    for (; x < (length - 7); x += 8) {
        const __m256i srcVector = __lasx_xvld((const __m256i *)&src[x], 0);
        const v8i32 testz2 = (v8i32)__lasx_xvmsknz_b(__lasx_xvand_v(srcVector, alphaMask));
        if (testz2[0]!=0 || testz2[4]!=0) {
            const v8i32 testc2 = (v8i32)__lasx_xvmsknz_b(__lasx_xvandn_v(srcVector, alphaMask));
            if (testc2[0]==0 && testc2[4]==0) {
                __lasx_xvst(srcVector, (__m256i *)&dst[x], 0);
            } else {
                __m256i alphaChannel = __lasx_xvshuf_b(__lasx_xvldi(0), srcVector, alphaShuffleMask);
                alphaChannel = __lasx_xvsub_h(one, alphaChannel);
                __m256i dstVector = __lasx_xvld((__m256i *)&dst[x], 0);
                 BYTE_MUL_LASX(dstVector, alphaChannel, colorMask, half);
                dstVector = __lasx_xvadd_b(dstVector, srcVector);
                __lasx_xvst(dstVector, (__m256i *)&dst[x], 0);
            }
        }
    }

    // Epilogue to handle all remaining pixels in one step.
    if (x < length) {
        const __m256i epilogueMask = __lasx_xvadd_w(offsetMask, __lasx_xvreplgr2vr_w(x - length));
        const __m256i epilogueMask1 = __lasx_xvslti_w(epilogueMask, 0);
        const __m256i srcVector =  __lasx_xvbitsel_v(__lasx_xvldi(0),
                                                     __lasx_xvld((const int *)&src[x], 0),
                                                     epilogueMask1);
        const __m256i epilogueMask2 = __lasx_xvslti_b(epilogueMask,0);
        const __m256i epilogueAlphaMask = __lasx_xvbitsel_v(__lasx_xvldi(0),
                                                            alphaMask,
                                                            epilogueMask2);
        const v8i32 testz3 = (v8i32)__lasx_xvmsknz_b(__lasx_xvand_v(srcVector, epilogueAlphaMask));

        if (testz3[0]!=0 || testz3[4]!=0) {
            const v8i32 testc3 = (v8i32)__lasx_xvmsknz_b(__lasx_xvandn_v(srcVector,
                                                                         epilogueAlphaMask));
            if (testc3[0]==0 && testc3[4]==0) {
                __m256i srcV = __lasx_xvld((int *)&dst[x], 0);
                __lasx_xvst(__lasx_xvbitsel_v(srcV, srcVector, epilogueMask1), (int *)&dst[x], 0);
            } else {
                __m256i alphaChannel = __lasx_xvshuf_b(__lasx_xvldi(0), srcVector, alphaShuffleMask);
                alphaChannel = __lasx_xvsub_h(one, alphaChannel);
                __m256i dstVector = __lasx_xvbitsel_v(__lasx_xvldi(0),
                                                      __lasx_xvld((int *)&dst[x], 0),
                                                      epilogueMask1);
                BYTE_MUL_LASX(dstVector, alphaChannel, colorMask, half);
                dstVector = __lasx_xvadd_b(dstVector, srcVector);
                __m256i dstV = __lasx_xvld((int *)&dst[x], 0);
                __lasx_xvst(__lasx_xvbitsel_v(dstV, dstVector, epilogueMask1), (int *)&dst[x], 0);
            }
        }
    }
}

// See BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_LSX for details.
inline static void Q_DECL_VECTORCALL
BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_LASX(quint32 *dst, const quint32 *src, const int length, const int const_alpha)
{
    int x = 0;

    ALIGNMENT_PROLOGUE_32BYTES(dst, x, length)
        blend_pixel(dst[x], src[x], const_alpha);

    const __m256i half = __lasx_xvreplgr2vr_h(0x80);
    const __m256i one = __lasx_xvreplgr2vr_h(0xff);
    const __m256i colorMask = __lasx_xvreplgr2vr_w(0x00ff00ff);
    const __m256i alphaMask = __lasx_xvreplgr2vr_w(0xff000000);
    const __m256i alphaShuffleMask = (__m256i)(v32i8){3,char(0xff),3,char(0xff),7,char(0xff),7,char(0xff),11,char(0xff),11,char(0xff),15,char(0xff),15,char(0xff),
                                                      3,char(0xff),3,char(0xff),7,char(0xff),7,char(0xff),11,char(0xff),11,char(0xff),15,char(0xff),15,char(0xff)};
    const __m256i constAlphaVector = __lasx_xvreplgr2vr_h(const_alpha);
    for (; x < (length - 7); x += 8) {
        __m256i srcVector = __lasx_xvld((const __m256i *)&src[x], 0);
        const v8i32 testz = (v8i32)__lasx_xvmsknz_b(__lasx_xvand_v(srcVector, alphaMask));
        if (testz[0]!=0 || testz[4]!=0) {
            BYTE_MUL_LASX(srcVector, constAlphaVector, colorMask, half);

            __m256i alphaChannel = __lasx_xvshuf_b(__lasx_xvldi(0), srcVector, alphaShuffleMask);
            alphaChannel = __lasx_xvsub_h(one, alphaChannel);
            __m256i dstVector = __lasx_xvld((__m256i *)&dst[x], 0);
            BYTE_MUL_LASX(dstVector, alphaChannel, colorMask, half);
            dstVector = __lasx_xvadd_b(dstVector, srcVector);
            __lasx_xvst(dstVector, (__m256i *)&dst[x], 0);
        }
    }
    SIMD_EPILOGUE(x, length, 7)
        blend_pixel(dst[x], src[x], const_alpha);
}

void qt_blend_argb32_on_argb32_lasx(uchar *destPixels, int dbpl,
                                    const uchar *srcPixels, int sbpl,
                                    int w, int h,
                                    int const_alpha)
{
    if (const_alpha == 256) {
        for (int y = 0; y < h; ++y) {
            const quint32 *src = reinterpret_cast<const quint32 *>(srcPixels);
            quint32 *dst = reinterpret_cast<quint32 *>(destPixels);
            BLEND_SOURCE_OVER_ARGB32_LASX(dst, src, w);
            destPixels += dbpl;
            srcPixels += sbpl;
        }
    } else if (const_alpha != 0) {
        const_alpha = (const_alpha * 255) >> 8;
        for (int y = 0; y < h; ++y) {
            const quint32 *src = reinterpret_cast<const quint32 *>(srcPixels);
            quint32 *dst = reinterpret_cast<quint32 *>(destPixels);
            BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_LASX(dst, src, w, const_alpha);
            destPixels += dbpl;
            srcPixels += sbpl;
        }
    }
}

void qt_blend_rgb32_on_rgb32_lasx(uchar *destPixels, int dbpl,
                                  const uchar *srcPixels, int sbpl,
                                  int w, int h,
                                  int const_alpha)
{
    if (const_alpha == 256) {
        for (int y = 0; y < h; ++y) {
            const quint32 *src = reinterpret_cast<const quint32 *>(srcPixels);
            quint32 *dst = reinterpret_cast<quint32 *>(destPixels);
            ::memcpy(dst, src, w * sizeof(uint));
            srcPixels += sbpl;
            destPixels += dbpl;
        }
        return;
    }
    if (const_alpha == 0)
        return;

    const __m256i half = __lasx_xvreplgr2vr_h(0x80);
    const __m256i colorMask = __lasx_xvreplgr2vr_w(0x00ff00ff);

    const_alpha = (const_alpha * 255) >> 8;
    int one_minus_const_alpha = 255 - const_alpha;
    const __m256i constAlphaVector = __lasx_xvreplgr2vr_h(const_alpha);
    const __m256i oneMinusConstAlpha =  __lasx_xvreplgr2vr_h(one_minus_const_alpha);
    for (int y = 0; y < h; ++y) {
        const quint32 *src = reinterpret_cast<const quint32 *>(srcPixels);
        quint32 *dst = reinterpret_cast<quint32 *>(destPixels);
        int x = 0;

        // First, align dest to 32 bytes:
        ALIGNMENT_PROLOGUE_32BYTES(dst, x, w)
            dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], one_minus_const_alpha);

        // 2) interpolate pixels with LASX
        for (; x < (w - 7); x += 8) {
            const __m256i srcVector = __lasx_xvld((const __m256i *)&src[x], 0);
            __m256i dstVector = __lasx_xvld((__m256i *)&dst[x], 0);
            INTERPOLATE_PIXEL_255_LASX(srcVector, dstVector, constAlphaVector, oneMinusConstAlpha, colorMask, half);
            __lasx_xvst(dstVector, (__m256i *)&dst[x], 0);
        }

        // 3) Epilogue
        SIMD_EPILOGUE(x, w, 7)
            dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], one_minus_const_alpha);

        srcPixels += sbpl;
        destPixels += dbpl;
    }
}

static Q_NEVER_INLINE
void Q_DECL_VECTORCALL qt_memfillXX_lasx(uchar *dest, __m256i value256, qsizetype bytes)
{
    __m128i value128 = *(__m128i*)(&value256);

    // main body
    __m256i *dst256 = reinterpret_cast<__m256i *>(dest);
    uchar *end = dest + bytes;
    while (reinterpret_cast<uchar *>(dst256 + 4) <= end) {
        __lasx_xvst(value256, dst256 + 0, 0);
        __lasx_xvst(value256, dst256 + 1, 0);
        __lasx_xvst(value256, dst256 + 2, 0);
        __lasx_xvst(value256, dst256 + 3, 0);
        dst256 += 4;
    }

    // first epilogue: fewer than 128 bytes / 32 entries
    bytes = end - reinterpret_cast<uchar *>(dst256);
    switch (bytes / sizeof(value256)) {
    case 3: __lasx_xvst(value256, dst256++, 0); Q_FALLTHROUGH();
    case 2: __lasx_xvst(value256, dst256++, 0); Q_FALLTHROUGH();
    case 1: __lasx_xvst(value256, dst256++, 0);
    }

    // second epilogue: fewer than 32 bytes
    __m128i *dst128 = reinterpret_cast<__m128i *>(dst256);
    if (bytes & sizeof(value128))
        __lsx_vst(value128, dst128++, 0);

    // third epilogue: fewer than 16 bytes
    if (bytes & 8)
        __lasx_xvstelm_d(value256, reinterpret_cast<__m128i *>(end - 8), 0, 0);
}

void qt_memfill64_lasx(quint64 *dest, quint64 value, qsizetype count)
{
    __m256i value256 = __lasx_xvreplgr2vr_d(value);

    qt_memfillXX_lasx(reinterpret_cast<uchar *>(dest), value256, count * sizeof(quint64));
}

void qt_memfill32_lasx(quint32 *dest, quint32 value, qsizetype count)
{
    if (count % 2) {
        // odd number of pixels, round to even
        *dest++ = value;
        --count;
    }
    qt_memfillXX_lasx(reinterpret_cast<uchar *>(dest), __lasx_xvreplgr2vr_w(value), count * sizeof(quint32));
}

void QT_FASTCALL comp_func_SourceOver_lasx(uint *destPixels, const uint *srcPixels,
                                           int length, uint const_alpha)
{
    Q_ASSERT(const_alpha < 256);

    const quint32 *src = (const quint32 *) srcPixels;
    quint32 *dst = (quint32 *) destPixels;

    if (const_alpha == 255)
        BLEND_SOURCE_OVER_ARGB32_LASX(dst, src, length);
    else
        BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_LASX(dst, src, length, const_alpha);
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_SourceOver_rgb64_lasx(QRgba64 *dst, const QRgba64 *src,
                                                 int length, uint const_alpha)
{
    Q_ASSERT(const_alpha < 256); // const_alpha is in [0-255]
    const __m256i half = __lasx_xvreplgr2vr_w(0x8000);
    const __m256i one  = __lasx_xvreplgr2vr_w(0xffff);
    const __m256i colorMask = __lasx_xvreplgr2vr_w(0x0000ffff);
    __m256i alphaMask = __lasx_xvreplgr2vr_w(0xff000000);
    alphaMask = __lasx_xvilvl_b(alphaMask, alphaMask);
    const __m256i alphaShuffleMask = (__m256i)(v32i8){6,7,char(0xff),char(0xff),6,7,char(0xff),char(0xff),14,15,char(0xff),char(0xff),14,15,char(0xff),char(0xff),
                                                      6,7,char(0xff),char(0xff),6,7,char(0xff),char(0xff),14,15,char(0xff),char(0xff),14,15,char(0xff),char(0xff)};

    if (const_alpha == 255) {
        int x = 0;
        for (; x < length && (quintptr(dst + x) & 31); ++x)
            blend_pixel(dst[x], src[x]);
        for (; x < length - 3; x += 4) {
            const __m256i srcVector = __lasx_xvld((const __m256i *)&src[x], 0);
            const v8i32 testz1 = (v8i32)__lasx_xvmsknz_b(__lasx_xvand_v(srcVector, alphaMask));
            if (testz1[0]!=0 || testz1[4]!=0){
                const v8i32 testc1 = (v8i32)__lasx_xvmsknz_b(__lasx_xvandn_v(srcVector, alphaMask));
                if (testc1[0]==0 && testc1[4]==0){
                    __lasx_xvst(srcVector, &dst[x], 0);
                } else {
                    __m256i alphaChannel = __lasx_xvshuf_b(__lasx_xvldi(0), srcVector, alphaShuffleMask);
                    alphaChannel = __lasx_xvsub_w(one, alphaChannel);
                    __m256i dstVector = __lasx_xvld(&dst[x], 0);
                    BYTE_MUL_RGB64_LASX(dstVector, alphaChannel, colorMask, half);
                    dstVector = __lasx_xvadd_h(dstVector, srcVector);
                    __lasx_xvst(dstVector, (__m256i *)&dst[x], 0);
                }
            }
        }
        SIMD_EPILOGUE(x, length, 3)
            blend_pixel(dst[x], src[x]);
    } else {
        const __m256i constAlphaVector = __lasx_xvreplgr2vr_w(const_alpha | (const_alpha << 8));
        int x = 0;
        for (; x < length && (quintptr(dst + x) & 31); ++x)
            blend_pixel(dst[x], src[x], const_alpha);
        for (; x < length - 3; x += 4) {
            __m256i srcVector = __lasx_xvld((const __m256i *)&src[x], 0);
            const v8i32 testz = (v8i32)__lasx_xvmsknz_b(__lasx_xvand_v(srcVector, alphaMask));
            if (testz[0]!=0 || testz[4]!=0){
                // Not all transparent
                BYTE_MUL_RGB64_LASX(srcVector, constAlphaVector, colorMask, half);
                __m256i alphaChannel = __lasx_xvshuf_b(__lasx_xvldi(0), srcVector, alphaShuffleMask);
                alphaChannel = __lasx_xvsub_w(one, alphaChannel);
                __m256i dstVector = __lasx_xvld((__m256i *)&dst[x], 0);
                BYTE_MUL_RGB64_LASX(dstVector, alphaChannel, colorMask, half);
                dstVector = __lasx_xvadd_h(dstVector, srcVector);
                __lasx_xvst(dstVector, (__m256i *)&dst[x], 0);
            }
        }
        SIMD_EPILOGUE(x, length, 3)
            blend_pixel(dst[x], src[x], const_alpha);
    }
}
#endif

void QT_FASTCALL comp_func_Source_lasx(uint *dst, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        ::memcpy(dst, src, length * sizeof(uint));
    } else {
        const int ialpha = 255 - const_alpha;

        int x = 0;

        // 1) prologue, align on 32 bytes
        ALIGNMENT_PROLOGUE_32BYTES(dst, x, length)
            dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], ialpha);

        // 2) interpolate pixels with LASX
        const __m256i half = __lasx_xvreplgr2vr_h(0x80);
        const __m256i colorMask = __lasx_xvreplgr2vr_w(0x00ff00ff);
        const __m256i constAlphaVector = __lasx_xvreplgr2vr_h(const_alpha);
        const __m256i oneMinusConstAlpha =  __lasx_xvreplgr2vr_h(ialpha);
        for (; x < length - 7; x += 8) {
            const __m256i srcVector = __lasx_xvld((const __m256i *)&src[x], 0);
            __m256i dstVector = __lasx_xvld((__m256i *)&dst[x], 0);
            INTERPOLATE_PIXEL_255_LASX(srcVector, dstVector, constAlphaVector, oneMinusConstAlpha, colorMask, half);
            __lasx_xvst(dstVector, (__m256i *)&dst[x], 0);
        }

        // 3) Epilogue
        SIMD_EPILOGUE(x, length, 7)
            dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], ialpha);
    }
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_Source_rgb64_lasx(QRgba64 *dst, const QRgba64 *src,
                                             int length, uint const_alpha)
{
    Q_ASSERT(const_alpha < 256); // const_alpha is in [0-255]
    if (const_alpha == 255) {
        ::memcpy(dst, src, length * sizeof(QRgba64));
    } else {
        const uint ca = const_alpha | (const_alpha << 8); // adjust to [0-65535]
        const uint cia = 65535 - ca;

        int x = 0;

        // 1) prologue, align on 32 bytes
        for (; x < length && (quintptr(dst + x) & 31); ++x)
            dst[x] = interpolate65535(src[x], ca, dst[x], cia);

        // 2) interpolate pixels with AVX2
        const __m256i half = __lasx_xvreplgr2vr_w(0x8000);
        const __m256i colorMask = __lasx_xvreplgr2vr_w(0x0000ffff);
        const __m256i constAlphaVector = __lasx_xvreplgr2vr_w(ca);
        const __m256i oneMinusConstAlpha =  __lasx_xvreplgr2vr_w(cia);
        for (; x < length - 3; x += 4) {
            const __m256i srcVector = __lasx_xvld((const __m256i *)&src[x], 0);
            __m256i dstVector = __lasx_xvld((__m256i *)&dst[x], 0);
            INTERPOLATE_PIXEL_RGB64_LASX(srcVector, dstVector, constAlphaVector,
                                         oneMinusConstAlpha, colorMask, half);
            __lasx_xvst(dstVector, &dst[x], 0);
        }

        // 3) Epilogue
        SIMD_EPILOGUE(x, length, 3)
            dst[x] = interpolate65535(src[x], ca, dst[x], cia);
    }
}
#endif

void QT_FASTCALL comp_func_solid_SourceOver_lasx(uint *destPixels, int length,
                                                 uint color, uint const_alpha)
{
    if ((const_alpha & qAlpha(color)) == 255) {
        qt_memfill32(destPixels, color, length);
    } else {
        if (const_alpha != 255)
            color = BYTE_MUL(color, const_alpha);

        const quint32 minusAlphaOfColor = qAlpha(~color);
        int x = 0;

        quint32 *dst = (quint32 *) destPixels;
        const __m256i colorVector = __lasx_xvreplgr2vr_w(color);
        const __m256i colorMask = __lasx_xvreplgr2vr_w(0x00ff00ff);
        const __m256i half = __lasx_xvreplgr2vr_h(0x80);
        const __m256i minusAlphaOfColorVector = __lasx_xvreplgr2vr_h(minusAlphaOfColor);

        ALIGNMENT_PROLOGUE_32BYTES(dst, x, length)
            destPixels[x] = color + BYTE_MUL(destPixels[x], minusAlphaOfColor);

        for (; x < length - 7; x += 8) {
            __m256i dstVector = __lasx_xvld(&dst[x], 0);
            BYTE_MUL_LASX(dstVector, minusAlphaOfColorVector, colorMask, half);
            dstVector = __lasx_xvadd_b(colorVector, dstVector);
            __lasx_xvst(dstVector, &dst[x], 0);
        }
        SIMD_EPILOGUE(x, length, 7)
            destPixels[x] = color + BYTE_MUL(destPixels[x], minusAlphaOfColor);
    }
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_SourceOver_rgb64_lasx(QRgba64 *destPixels, int length,
                                                       QRgba64 color, uint const_alpha)
{
    Q_ASSERT(const_alpha < 256); // const_alpha is in [0-255]
    if (const_alpha == 255 && color.isOpaque()) {
        qt_memfill64((quint64*)destPixels, color, length);
    } else {
        if (const_alpha != 255)
            color = multiplyAlpha255(color, const_alpha);

        const uint minusAlphaOfColor = 65535 - color.alpha();
        int x = 0;
        quint64 *dst = (quint64 *) destPixels;
        const __m256i colorVector = __lasx_xvreplgr2vr_d(color);
        const __m256i colorMask = __lasx_xvreplgr2vr_w(0x0000ffff);
        const __m256i half = __lasx_xvreplgr2vr_w(0x8000);
        const __m256i minusAlphaOfColorVector = __lasx_xvreplgr2vr_w(minusAlphaOfColor);

        for (; x < length && (quintptr(dst + x) & 31); ++x)
            destPixels[x] = color + multiplyAlpha65535(destPixels[x], minusAlphaOfColor);

        for (; x < length - 3; x += 4) {
            __m256i dstVector = __lasx_xvld(&dst[x], 0);
            BYTE_MUL_RGB64_LASX(dstVector, minusAlphaOfColorVector, colorMask, half);
            dstVector = __lasx_xvadd_h(colorVector, dstVector);
            __lasx_xvst(dstVector, &dst[x], 0);
        }
        SIMD_EPILOGUE(x, length, 3)
            destPixels[x] = color + multiplyAlpha65535(destPixels[x], minusAlphaOfColor);
    }
}
#endif

static inline void interpolate_4_pixels_16_lasx(const __m256i tlr1, const __m256i tlr2, const __m256i blr1,
                                                const __m256i blr2, __m256i distx, __m256i disty, uint *b)
{
    const __m256i colorMask = __lasx_xvreplgr2vr_w(0x00ff00ff);
    const __m256i v_256 = __lasx_xvreplgr2vr_h(256);

    /* Correct for later unpack */
    const __m256i vdistx = __lasx_xvpermi_d(distx, 0b11011000);
    const __m256i vdisty = __lasx_xvpermi_d(disty, 0b11011000);

    __m256i dxdy = __lasx_xvmul_h(vdistx, vdisty);
    const __m256i distx_ = __lasx_xvslli_h(vdistx, 4);
    const __m256i disty_ = __lasx_xvslli_h(vdisty, 4);
    __m256i idxidy =  __lasx_xvadd_h(dxdy, __lasx_xvsub_h(v_256, __lasx_xvadd_h(distx_, disty_)));
    __m256i dxidy =  __lasx_xvsub_h(distx_, dxdy);
    __m256i idxdy =  __lasx_xvsub_h(disty_, dxdy);

    __m256i tlr1AG = __lasx_xvsrli_h(tlr1, 8);
    __m256i tlr1RB = __lasx_xvand_v(tlr1, colorMask);
    __m256i tlr2AG = __lasx_xvsrli_h(tlr2, 8);
    __m256i tlr2RB = __lasx_xvand_v(tlr2, colorMask);
    __m256i blr1AG = __lasx_xvsrli_h(blr1, 8);
    __m256i blr1RB = __lasx_xvand_v(blr1, colorMask);
    __m256i blr2AG = __lasx_xvsrli_h(blr2, 8);
    __m256i blr2RB = __lasx_xvand_v(blr2, colorMask);

    __m256i odxidy1 = __lasx_xvilvl_w(dxidy, idxidy);
    __m256i odxidy2 = __lasx_xvilvh_w(dxidy, idxidy);
    tlr1AG = __lasx_xvmul_h(tlr1AG, odxidy1);
    tlr1RB = __lasx_xvmul_h(tlr1RB, odxidy1);
    tlr2AG = __lasx_xvmul_h(tlr2AG, odxidy2);
    tlr2RB = __lasx_xvmul_h(tlr2RB, odxidy2);
    __m256i odxdy1 = __lasx_xvilvl_w(dxdy, idxdy);
    __m256i odxdy2 = __lasx_xvilvh_w(dxdy, idxdy);
    blr1AG = __lasx_xvmul_h(blr1AG, odxdy1);
    blr1RB = __lasx_xvmul_h(blr1RB, odxdy1);
    blr2AG = __lasx_xvmul_h(blr2AG, odxdy2);
    blr2RB = __lasx_xvmul_h(blr2RB, odxdy2);

    /* Add the values, and shift to only keep 8 significant bits per colors */
    tlr1AG = __lasx_xvadd_w(tlr1AG, __lasx_xvbsrl_v(tlr1AG, 0b100));
    tlr2AG = __lasx_xvadd_w(tlr2AG, __lasx_xvbsrl_v(tlr2AG, 0b100));
    __m256i topAG = __lasx_xvpermi_w(tlr2AG, tlr1AG, 0b10001000);
    tlr1RB = __lasx_xvadd_w(tlr1RB, __lasx_xvbsrl_v(tlr1RB, 0b100));
    tlr2RB = __lasx_xvadd_w(tlr2RB, __lasx_xvbsrl_v(tlr2RB, 0b100));
    __m256i topRB = __lasx_xvpermi_w(tlr2RB, tlr1RB, 0b10001000);
    blr1AG = __lasx_xvadd_w(blr1AG, __lasx_xvbsrl_v(blr1AG, 0b100));
    blr2AG = __lasx_xvadd_w(blr2AG, __lasx_xvbsrl_v(blr2AG, 0b100));
    __m256i botAG = __lasx_xvpermi_w(blr2AG, blr1AG, 0b10001000);
    blr1RB = __lasx_xvadd_w(blr1RB, __lasx_xvbsrl_v(blr1RB, 0b100));
    blr2RB = __lasx_xvadd_w(blr2RB, __lasx_xvbsrl_v(blr2RB, 0b100));
    __m256i botRB = __lasx_xvpermi_w(blr2RB, blr1RB, 0b10001000);
    __m256i rAG = __lasx_xvadd_h(topAG, botAG);
    __m256i rRB = __lasx_xvadd_h(topRB, botRB);
    rRB = __lasx_xvsrli_h(rRB, 8);
    /* Correct for hadd */
    rAG = __lasx_xvpermi_d(rAG, 0b11011000);
    rRB = __lasx_xvpermi_d(rRB, 0b11011000);
    __m256i colorMask1 = __lasx_xvslti_b(colorMask, 0);
    __lasx_xvst(__lasx_xvbitsel_v(rAG, rRB, colorMask1), b, 0);
}

inline void fetchTransformedBilinear_pixelBounds(int, int l1, int l2, int &v1, int &v2)
{
    if (v1 < l1)
        v2 = v1 = l1;
    else if (v1 >= l2)
        v2 = v1 = l2;
    else
        v2 = v1 + 1;
    Q_ASSERT(v1 >= l1 && v1 <= l2);
    Q_ASSERT(v2 >= l1 && v2 <= l2);
}

void QT_FASTCALL intermediate_adder_lasx(uint *b, uint *end,
                                         const IntermediateBuffer &intermediate,
                                         int offset, int &fx, int fdx);

void QT_FASTCALL fetchTransformedBilinearARGB32PM_simple_scale_helper_lasx(uint *b, uint *end, const QTextureData &image,
                                                                           int &fx, int &fy, int fdx, int /*fdy*/)
{
    int y1 = (fy >> 16);
    int y2;
    fetchTransformedBilinear_pixelBounds(image.height, image.y1, image.y2 - 1, y1, y2);
    const uint *s1 = (const uint *)image.scanLine(y1);
    const uint *s2 = (const uint *)image.scanLine(y2);

    const int disty = (fy & 0x0000ffff) >> 8;
    const int idisty = 256 - disty;
    const int length = end - b;

    // The intermediate buffer is generated in the positive direction
    const int adjust = (fdx < 0) ? fdx * length : 0;
    const int offset = (fx + adjust) >> 16;
    int x = offset;

    IntermediateBuffer intermediate;
    // count is the size used in the intermediate_buffer.
    int count = (qint64(length) * qAbs(fdx) + FixedScale - 1) / FixedScale + 2;
    // length is supposed to be <= BufferSize either because data->m11 < 1 or
    // data->m11 < 2, and any larger buffers split
    Q_ASSERT(count <= BufferSize + 2);
    int f = 0;
    int lim = qMin(count, image.x2 - x);
    if (x < image.x1) {
        Q_ASSERT(x < image.x2);
        uint t = s1[image.x1];
        uint b = s2[image.x1];
        quint32 rb = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
        quint32 ag = ((((t>>8) & 0xff00ff) * idisty + ((b>>8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
        do {
            intermediate.buffer_rb[f] = rb;
            intermediate.buffer_ag[f] = ag;
            f++;
            x++;
        } while (x < image.x1 && f < lim);
    }

    const __m256i disty_ = __lasx_xvreplgr2vr_h(disty);
    const __m256i idisty_ = __lasx_xvreplgr2vr_h(idisty);
    const __m256i colorMask = __lasx_xvreplgr2vr_w(0x00ff00ff);

    lim -= 7;
    for (; f < lim; x += 8, f += 8) {
        // Load 8 pixels from s1, and split the alpha-green and red-blue component
        __m256i top = __lasx_xvld((s1+x), 0);
        __m256i topAG = __lasx_xvsrli_h(top, 8);
        __m256i topRB = __lasx_xvand_v(top, colorMask);
        // Multiplies each color component by idisty
        topAG = __lasx_xvmul_h(topAG, idisty_);
        topRB = __lasx_xvmul_h(topRB, idisty_);

        // Same for the s2 vector
        __m256i bottom = __lasx_xvld((s2+x), 0);
        __m256i bottomAG = __lasx_xvsrli_h(bottom, 8);
        __m256i bottomRB = __lasx_xvand_v(bottom, colorMask);
        bottomAG = __lasx_xvmul_h(bottomAG, disty_);
        bottomRB = __lasx_xvmul_h(bottomRB, disty_);

        // Add the values, and shift to only keep 8 significant bits per colors
        __m256i rAG = __lasx_xvadd_h(topAG, bottomAG);
        rAG = __lasx_xvsrli_h(rAG, 8);
        __lasx_xvst(rAG, (&intermediate.buffer_ag[f]), 0);
        __m256i rRB = __lasx_xvadd_h(topRB, bottomRB);
        rRB = __lasx_xvsrli_h(rRB, 8);
        __lasx_xvst(rRB, (&intermediate.buffer_rb[f]), 0);
    }

    for (; f < count; f++) { // Same as above but without simd
        x = qMin(x, image.x2 - 1);

        uint t = s1[x];
        uint b = s2[x];

        intermediate.buffer_rb[f] = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
        intermediate.buffer_ag[f] = ((((t>>8) & 0xff00ff) * idisty + ((b>>8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
        x++;
    }

    // Now interpolate the values from the intermediate_buffer to get the final result.
    intermediate_adder_lasx(b, end, intermediate, offset, fx, fdx);
}

void QT_FASTCALL intermediate_adder_lasx(uint *b, uint *end,
                                         const IntermediateBuffer &intermediate,
                                         int offset, int &fx, int fdx)
{
    fx -= offset * FixedScale;

    const __m128i v_fdx = __lsx_vreplgr2vr_w(fdx * 4);
    const __m128i v_blend = __lsx_vreplgr2vr_w(0x00ff00ff);
    const __m128i vdx_shuffle = (__m128i)(v16i8){1, char(0xff), 1, char(0xff), 5, char(0xff), 5, char(0xff),
                                                 9, char(0xff), 9, char(0xff), 13, char(0xff), 13, char(0xff)};
    __m128i v_fx = (__m128i)(v4i32){fx, fx + fdx, fx + fdx + fdx, fx + fdx + fdx + fdx};

    while (b < end - 3) {
        v4i32 offset = (v4i32)__lsx_vsrli_w(v_fx, 16);

        __m256i vrb = (__m256i)(v4i64){*(const long long *)(intermediate.buffer_rb + offset[0]),
                                       *(const long long *)(intermediate.buffer_rb + offset[1]),
                                       *(const long long *)(intermediate.buffer_rb + offset[2]),
                                       *(const long long *)(intermediate.buffer_rb + offset[3])};
        __m256i vag = (__m256i)(v4i64){*(const long long *)(intermediate.buffer_ag + offset[0]),
                                       *(const long long *)(intermediate.buffer_ag + offset[1]),
                                       *(const long long *)(intermediate.buffer_ag + offset[2]),
                                       *(const long long *)(intermediate.buffer_ag + offset[3])};

        __m128i vdx = __lsx_vshuf_b(__lsx_vldi(0), v_fx, vdx_shuffle);
        __m128i vidx = __lsx_vsub_h(__lsx_vreplgr2vr_h(256), vdx);
        v2i64 vl = __lsx_vilvl_w(vdx, vidx);
        v2i64 vh = __lsx_vilvh_w(vdx, vidx);
        __m256i vmulx = lasx_set_q(vh, vl);

        vrb = __lasx_xvmul_h(vrb, vmulx);
        vag = __lasx_xvmul_h(vag, vmulx);
        vrb = __lasx_xvadd_w(vrb, __lasx_xvbsrl_v(vrb, 0b100));
        vag = __lasx_xvadd_w(vag, __lasx_xvbsrl_v(vag, 0b100));
        __m256i vrbag = __lasx_xvpickev_w(vag, vrb);
        vrbag = (v4i64)__lasx_xvpermi_d(vrbag, 0b11011000);

        __m128i rb = lasx_extracti128_lo(vrbag);
        __m128i ag = lasx_extracti128_hi(vrbag);

        rb = __lsx_vsrli_h(rb, 8);
        __lsx_vst(__lsx_vbitsel_v(ag, rb, v_blend), (__m128i*)b, 0);
        b += 4;
        v_fx = __lsx_vadd_w(v_fx, v_fdx);
    }
    fx = __lsx_vpickve2gr_w(v_fx, 0);
    while (b < end) {
        const int x = (fx >> 16);

        const uint distx = (fx & 0x0000ffff) >> 8;
        const uint idistx = 256 - distx;
        const uint rb = (intermediate.buffer_rb[x] * idistx + intermediate.buffer_rb[x + 1] * distx) & 0xff00ff00;
        const uint ag = (intermediate.buffer_ag[x] * idistx + intermediate.buffer_ag[x + 1] * distx) & 0xff00ff00;
        *b = (rb >> 8) | ag;
        b++;
        fx += fdx;
    }
    fx += offset * FixedScale;
}

void QT_FASTCALL fetchTransformedBilinearARGB32PM_downscale_helper_lasx(uint *b, uint *end, const QTextureData &image,
                                                                        int &fx, int &fy, int fdx, int /*fdy*/)
{
    int y1 = (fy >> 16);
    int y2;
    fetchTransformedBilinear_pixelBounds(image.height, image.y1, image.y2 - 1, y1, y2);
    const uint *s1 = (const uint *)image.scanLine(y1);
    const uint *s2 = (const uint *)image.scanLine(y2);
    const int disty8 = (fy & 0x0000ffff) >> 8;
    const int disty4 = (disty8 + 0x08) >> 4;

    const qint64 min_fx = qint64(image.x1) * FixedScale;
    const qint64 max_fx = qint64(image.x2 - 1) * FixedScale;
    while (b < end) {
        int x1 = (fx >> 16);
        int x2;
        fetchTransformedBilinear_pixelBounds(image.width, image.x1, image.x2 - 1, x1, x2);
        if (x1 != x2)
            break;
        uint top = s1[x1];
        uint bot = s2[x1];
        *b = INTERPOLATE_PIXEL_256(top, 256 - disty8, bot, disty8);
        fx += fdx;
        ++b;
    }
    uint *boundedEnd = end;
    if (fdx > 0)
        boundedEnd = qMin(boundedEnd, b + (max_fx - fx) / fdx);
    else if (fdx < 0)
        boundedEnd = qMin(boundedEnd, b + (min_fx - fx) / fdx);

    // A fast middle part without boundary checks
    const __m256i vdistShuffle = (__m256i)(v32i8){0, char(0xff), 0, char(0xff), 4, char(0xff), 4, char(0xff),
                                                  8, char(0xff), 8, char(0xff), 12, char(0xff), 12, char(0xff),
                                                  0, char(0xff), 0, char(0xff), 4, char(0xff), 4, char(0xff),
                                                  8, char(0xff), 8, char(0xff), 12, char(0xff), 12, char(0xff)};
    const __m256i v_disty = __lasx_xvreplgr2vr_h(disty4);
    const __m256i v_fdx = __lasx_xvreplgr2vr_w(fdx * 8);
    const __m256i v_fx_r = __lasx_xvreplgr2vr_w(0x08);
    const __m256i v_index = (__m256i)(v8i32){0, 1, 2, 3, 4, 5, 6, 7};
    __m256i v_fx = __lasx_xvreplgr2vr_w(fx);
    v_fx = __lasx_xvadd_w(v_fx, __lasx_xvmul_w(__lasx_xvreplgr2vr_w(fdx), v_index));

    while (b < boundedEnd - 7) {
        const v8i32 offset = (v8i32)__lasx_xvsrli_w(v_fx, 16);

        const __m256i toplo = (__m256i)(v4i64){*(const long long *)(s1 + offset[0]), *(const long long *)(s1 + offset[1]),
                                               *(const long long *)(s1 + offset[2]), *(const long long *)(s1 + offset[3])};
        const __m256i tophi = (__m256i)(v4i64){*(const long long *)(s1 + offset[4]), *(const long long *)(s1 + offset[5]),
                                               *(const long long *)(s1 + offset[6]), *(const long long *)(s1 + offset[7])};
        const __m256i botlo = (__m256i)(v4i64){*(const long long *)(s2 + offset[0]), *(const long long *)(s2 + offset[1]),
                                               *(const long long *)(s2 + offset[2]), *(const long long *)(s2 + offset[3])};
        const __m256i bothi = (__m256i)(v4i64){*(const long long *)(s2 + offset[4]), *(const long long *)(s2 + offset[5]),
                                               *(const long long *)(s2 + offset[6]), *(const long long *)(s2 + offset[7])};

        __m256i v_distx = __lasx_xvsrli_h(v_fx, 8);
        v_distx = __lasx_xvsrli_h(__lasx_xvadd_w(v_distx, v_fx_r), 4);
        v_distx = __lasx_xvshuf_b(__lasx_xvldi(0), v_distx, vdistShuffle);

        interpolate_4_pixels_16_lasx(toplo, tophi, botlo, bothi, v_distx, v_disty, b);
        b += 8;
        v_fx = __lasx_xvadd_w(v_fx, v_fdx);
    }
    fx = __lasx_xvpickve2gr_w(v_fx, 0);

    while (b < boundedEnd) {
        int x = (fx >> 16);
        int distx8 = (fx & 0x0000ffff) >> 8;
        *b = interpolate_4_pixels(s1 + x, s2 + x, distx8, disty8);
        fx += fdx;
        ++b;
    }

    while (b < end) {
        int x1 = (fx >> 16);
        int x2;
        fetchTransformedBilinear_pixelBounds(image.width, image.x1, image.x2 - 1, x1, x2);
        uint tl = s1[x1];
        uint tr = s1[x2];
        uint bl = s2[x1];
        uint br = s2[x2];
        int distx8 = (fx & 0x0000ffff) >> 8;
        *b = interpolate_4_pixels(tl, tr, bl, br, distx8, disty8);
        fx += fdx;
        ++b;
    }
}

void QT_FASTCALL fetchTransformedBilinearARGB32PM_fast_rotate_helper_lasx(uint *b, uint *end, const QTextureData &image,
                                                                          int &fx, int &fy, int fdx, int fdy)
{
    const qint64 min_fx = qint64(image.x1) * FixedScale;
    const qint64 max_fx = qint64(image.x2 - 1) * FixedScale;
    const qint64 min_fy = qint64(image.y1) * FixedScale;
    const qint64 max_fy = qint64(image.y2 - 1) * FixedScale;
    // first handle the possibly bounded part in the beginning
    while (b < end) {
        int x1 = (fx >> 16);
        int x2;
        int y1 = (fy >> 16);
        int y2;
        fetchTransformedBilinear_pixelBounds(image.width, image.x1, image.x2 - 1, x1, x2);
        fetchTransformedBilinear_pixelBounds(image.height, image.y1, image.y2 - 1, y1, y2);
        if (x1 != x2 && y1 != y2)
            break;
        const uint *s1 = (const uint *)image.scanLine(y1);
        const uint *s2 = (const uint *)image.scanLine(y2);
        uint tl = s1[x1];
        uint tr = s1[x2];
        uint bl = s2[x1];
        uint br = s2[x2];
        int distx = (fx & 0x0000ffff) >> 8;
        int disty = (fy & 0x0000ffff) >> 8;
        *b = interpolate_4_pixels(tl, tr, bl, br, distx, disty);
        fx += fdx;
        fy += fdy;
        ++b;
    }
    uint *boundedEnd = end;
    if (fdx > 0)
        boundedEnd = qMin(boundedEnd, b + (max_fx - fx) / fdx);
    else if (fdx < 0)
        boundedEnd = qMin(boundedEnd, b + (min_fx - fx) / fdx);
    if (fdy > 0)
        boundedEnd = qMin(boundedEnd, b + (max_fy - fy) / fdy);
    else if (fdy < 0)
        boundedEnd = qMin(boundedEnd, b + (min_fy - fy) / fdy);

    // until boundedEnd we can now have a fast middle part without boundary checks
    const __m256i vdistShuffle = (__m256i)(v32i8){0, char(0xff), 0, char(0xff), 4, char(0xff), 4, char(0xff), 8, char(0xff), 8, char(0xff), 12, char(0xff), 12, char(0xff),
                                                  0, char(0xff), 0, char(0xff), 4, char(0xff), 4, char(0xff), 8, char(0xff), 8, char(0xff), 12, char(0xff), 12, char(0xff)};
    const __m256i v_fdx = __lasx_xvreplgr2vr_w(fdx * 8);
    const __m256i v_fdy = __lasx_xvreplgr2vr_w(fdy * 8);
    const __m256i v_fxy_r = __lasx_xvreplgr2vr_w(0x08);
    const __m256i v_index = (__m256i)(v8i32){0, 1, 2, 3, 4, 5, 6, 7};
    __m256i v_fx = __lasx_xvreplgr2vr_w(fx);
    __m256i v_fy = __lasx_xvreplgr2vr_w(fy);
    v_fx = __lasx_xvadd_w(v_fx, __lasx_xvmul_w(__lasx_xvreplgr2vr_w(fdx), v_index));
    v_fy = __lasx_xvadd_w(v_fy, __lasx_xvmul_w(__lasx_xvreplgr2vr_w(fdy), v_index));

    const uchar *textureData = image.imageData;
    const qsizetype bytesPerLine = image.bytesPerLine;
    const __m256i vbpl = __lasx_xvreplgr2vr_h(bytesPerLine/4);

    while (b < boundedEnd - 7) {
        const __m256i vy = __lasx_xvpickev_h(__lasx_xvldi(0),
                                             __lasx_xvsat_w(__lasx_xvsrli_w(v_fy, 16), 15));
        // 8x16bit * 8x16bit -> 8x32bit
        __m256i offset = __lasx_xvilvl_h(__lasx_xvmuh_h(vy, vbpl), __lasx_xvmul_h(vy, vbpl));
        offset = __lasx_xvadd_w(offset, __lasx_xvsrli_w(v_fx, 16));

        const uint *s1 = (const uint *)(textureData);
        const uint *s2 = (const uint *)(textureData + bytesPerLine);
        const __m256i toplo = (__m256i)(v4i64){*(const long long *)(s1+((v8i32)offset)[0]), *(const long long *)(s1+((v8i32)offset)[1]),
                                               *(const long long *)(s1+((v8i32)offset)[2]), *(const long long *)(s1+((v8i32)offset)[3])};
        const __m256i tophi = (__m256i)(v4i64){*(const long long *)(s1+((v8i32)offset)[4]), *(const long long *)(s1+((v8i32)offset)[5]),
                                               *(const long long *)(s1+((v8i32)offset)[6]), *(const long long *)(s1+((v8i32)offset)[7])};
        const __m256i botlo = (__m256i)(v4i64){*(const long long *)(s2+((v8i32)offset)[0]), *(const long long *)(s2+((v8i32)offset)[1]),
                                               *(const long long *)(s2+((v8i32)offset)[2]), *(const long long *)(s2+((v8i32)offset)[3])};
        const __m256i bothi = (__m256i)(v4i64){*(const long long *)(s2+((v8i32)offset)[4]), *(const long long *)(s2+((v8i32)offset)[5]),
                                               *(const long long *)(s2+((v8i32)offset)[6]), *(const long long *)(s2+((v8i32)offset)[7])};

        __m256i v_distx = __lasx_xvsrli_h(v_fx, 8);
        __m256i v_disty = __lasx_xvsrli_h(v_fy, 8);
        v_distx = __lasx_xvsrli_h(__lasx_xvadd_w(v_distx, v_fxy_r), 4);
        v_disty = __lasx_xvsrli_h(__lasx_xvadd_w(v_disty, v_fxy_r), 4);
        v_distx = __lasx_xvshuf_b(__lasx_xvldi(0), v_distx, vdistShuffle);
        v_disty = __lasx_xvshuf_b(__lasx_xvldi(0), v_disty, vdistShuffle);

        interpolate_4_pixels_16_lasx(toplo, tophi, botlo, bothi, v_distx, v_disty, b);
        b += 8;
        v_fx = __lasx_xvadd_w(v_fx, v_fdx);
        v_fy = __lasx_xvadd_w(v_fy, v_fdy);
    }
    fx = __lasx_xvpickve2gr_w(v_fx, 0);
    fy = __lasx_xvpickve2gr_w(v_fy, 0);

    while (b < boundedEnd) {
        int x = (fx >> 16);
        int y = (fy >> 16);

        const uint *s1 = (const uint *)image.scanLine(y);
        const uint *s2 = (const uint *)image.scanLine(y + 1);

        int distx = (fx & 0x0000ffff) >> 8;
        int disty = (fy & 0x0000ffff) >> 8;
        *b = interpolate_4_pixels(s1 + x, s2 + x, distx, disty);

        fx += fdx;
        fy += fdy;
        ++b;
    }

    while (b < end) {
        int x1 = (fx >> 16);
        int x2;
        int y1 = (fy >> 16);
        int y2;

        fetchTransformedBilinear_pixelBounds(image.width, image.x1, image.x2 - 1, x1, x2);
        fetchTransformedBilinear_pixelBounds(image.height, image.y1, image.y2 - 1, y1, y2);

        const uint *s1 = (const uint *)image.scanLine(y1);
        const uint *s2 = (const uint *)image.scanLine(y2);

        uint tl = s1[x1];
        uint tr = s1[x2];
        uint bl = s2[x1];
        uint br = s2[x2];

        int distx = (fx & 0x0000ffff) >> 8;
        int disty = (fy & 0x0000ffff) >> 8;
        *b = interpolate_4_pixels(tl, tr, bl, br, distx, disty);

        fx += fdx;
        fy += fdy;
        ++b;
    }
}

static inline __m256i epilogueMaskFromCount(qsizetype count)
{
    Q_ASSERT(count > 0);
    static const __m256i offsetMask = (__m256i)(v8i32){0, 1, 2, 3, 4, 5, 6, 7};
    return __lasx_xvadd_w(offsetMask, __lasx_xvreplgr2vr_w(-count));
}

template<bool RGBA>
static void convertARGBToARGB32PM_lasx(uint *buffer, const uint *src, qsizetype count)
{
    qsizetype i = 0;
    const __m256i alphaMask = __lasx_xvreplgr2vr_w(0xff000000);
    const __m256i rgbaMask = (__m256i)(v32i8){2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15,
                                              2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15};
    const __m256i shuffleMask = (__m256i)(v32i8){6, 7, 6, 7, 6, 7, 6, 7, 14, 15, 14, 15, 14, 15, 14, 15,
                                                 6, 7, 6, 7, 6, 7, 6, 7, 14, 15, 14, 15, 14, 15, 14, 15};
    const __m256i half = __lasx_xvreplgr2vr_h(0x0080);
    const __m256i zero = __lasx_xvldi(0);

    for (; i < count - 7; i += 8) {
        __m256i srcVector = __lasx_xvld(reinterpret_cast<const __m256i *>(src + i), 0);
        const v8i32 testz = (v8i32)__lasx_xvmsknz_b(__lasx_xvand_v(srcVector, alphaMask));
        if (testz[0]!=0 || testz[4]!=0){
            const v8i32 testc = (v8i32)__lasx_xvmsknz_b(__lasx_xvandn_v(srcVector, alphaMask));
            bool cf = testc[0]==0 && testc[4]==0;
            if (RGBA)
                srcVector = __lasx_xvshuf_b(zero, srcVector, rgbaMask);
            if (!cf) {
                __m256i src1 = __lasx_xvilvl_b(zero, srcVector);
                __m256i src2 = __lasx_xvilvh_b(zero, srcVector);
                __m256i alpha1 = __lasx_xvshuf_b(zero, src1, shuffleMask);
                __m256i alpha2 = __lasx_xvshuf_b(zero, src2, shuffleMask);
                __m256i blendMask = (__m256i)(v16i16){0, 1, 2, 11, 4, 5, 6, 15, 0, 1, 2, 11, 4, 5, 6, 15};
                src1 = __lasx_xvmul_h(src1, alpha1);
                src2 = __lasx_xvmul_h(src2, alpha2);
                src1 = __lasx_xvadd_h(src1, __lasx_xvsrli_h(src1, 8));
                src2 = __lasx_xvadd_h(src2, __lasx_xvsrli_h(src2, 8));
                src1 = __lasx_xvadd_h(src1, half);
                src2 = __lasx_xvadd_h(src2, half);
                src1 = __lasx_xvsrli_h(src1, 8);
                src2 = __lasx_xvsrli_h(src2, 8);
                src1 = __lasx_xvshuf_h(blendMask, alpha1, src1);
                src2 = __lasx_xvshuf_h(blendMask, alpha2, src2);
                src1 = __lasx_xvmaxi_h(src1, 0);
                src2 = __lasx_xvmaxi_h(src2, 0);
                srcVector = __lasx_xvpickev_b(__lasx_xvsat_hu(src2, 7), __lasx_xvsat_hu(src1, 7));
                __lasx_xvst(srcVector, reinterpret_cast<__m256i *>(buffer + i), 0);
            } else {
                if (buffer != src || RGBA)
                    __lasx_xvst(srcVector, reinterpret_cast<__m256i *>(buffer + i), 0);
            }
        } else {
            __lasx_xvst(zero, reinterpret_cast<__m256i *>(buffer + i), 0);
        }
    }

    if (i < count) {
        const __m256i epilogueMask = epilogueMaskFromCount(count - i);
        const __m256i epilogueMask1 = __lasx_xvslti_w(epilogueMask, 0);
        __m256i srcVector = __lasx_xvbitsel_v(__lasx_xvldi(0),
                                              __lasx_xvld(reinterpret_cast<const int *>(src + i), 0),
                                              epilogueMask1);
        const __m256i epilogueMask2 = __lasx_xvslti_b(epilogueMask, 0);
        const __m256i epilogueAlphaMask = __lasx_xvbitsel_v(__lasx_xvldi(0), alphaMask, epilogueMask2);

        const v8i32 testz1 = (v8i32)__lasx_xvmsknz_b(__lasx_xvand_v(srcVector, epilogueAlphaMask));
        if (testz1[0]!=0 || testz1[4]!=0){
            const v8i32 testc1 = (v8i32)__lasx_xvmsknz_b(__lasx_xvandn_v(srcVector, epilogueAlphaMask));
            bool cf = testc1[0]==0 && testc1[4]==0;
            if (RGBA)
                srcVector = __lasx_xvshuf_b(zero, srcVector, rgbaMask);
            if (!cf) {
                __m256i src1 = __lasx_xvilvl_b(zero, srcVector);
                __m256i src2 = __lasx_xvilvh_b(zero, srcVector);
                __m256i alpha1 = __lasx_xvshuf_b(zero, src1, shuffleMask);
                __m256i alpha2 = __lasx_xvshuf_b(zero, src2, shuffleMask);
                __m256i blendMask = (__m256i)(v16i16){0, 1, 2, 11, 4, 5, 6, 15, 0, 1, 2, 11, 4, 5, 6, 15};
                src1 = __lasx_xvmul_h(src1, alpha1);
                src2 = __lasx_xvmul_h(src2, alpha2);
                src1 = __lasx_xvadd_h(src1, __lasx_xvsrli_h(src1, 8));
                src2 = __lasx_xvadd_h(src2, __lasx_xvsrli_h(src2, 8));
                src1 = __lasx_xvadd_h(src1, half);
                src2 = __lasx_xvadd_h(src2, half);
                src1 = __lasx_xvsrli_h(src1, 8);
                src2 = __lasx_xvsrli_h(src2, 8);
                src1 = __lasx_xvshuf_h(blendMask, alpha1, src1);
                src2 = __lasx_xvshuf_h(blendMask, alpha2, src2);
                src1 = __lasx_xvmaxi_h(src1, 0);
                src2 = __lasx_xvmaxi_h(src2, 0);
                srcVector = __lasx_xvpickev_b(__lasx_xvsat_hu(src2, 7), __lasx_xvsat_hu(src1, 7));
                __m256i srcV = __lasx_xvld(reinterpret_cast<int *>(buffer + i), 0);
                srcV = __lasx_xvbitsel_v(srcV, srcVector, epilogueMask1);
                __lasx_xvst(srcV, reinterpret_cast<int *>(buffer + i), 0);
            } else {
                if (buffer != src || RGBA) {
                    __m256i srcV = __lasx_xvld(reinterpret_cast<int *>(buffer + i), 0);
                    srcV = __lasx_xvbitsel_v(srcV, srcVector, epilogueMask1);
                    __lasx_xvst(srcV, reinterpret_cast<int *>(buffer + i), 0);
                }
            }
        } else {
            __m256i srcV = __lasx_xvld(reinterpret_cast<int *>(buffer + i), 0);
            srcV = __lasx_xvbitsel_v(srcV, zero, epilogueMask1);
            __lasx_xvst(srcV, reinterpret_cast<int *>(buffer + i), 0);
        }
    }
}

void QT_FASTCALL convertARGB32ToARGB32PM_lasx(uint *buffer, int count, const QList<QRgb> *)
{
    convertARGBToARGB32PM_lasx<false>(buffer, buffer, count);
}

void QT_FASTCALL convertRGBA8888ToARGB32PM_lasx(uint *buffer, int count, const QList<QRgb> *)
{
    convertARGBToARGB32PM_lasx<true>(buffer, buffer, count);
}

const uint *QT_FASTCALL fetchARGB32ToARGB32PM_lasx(uint *buffer, const uchar *src, int index,
                                                   int count, const QList<QRgb> *, QDitherInfo *)
{
    convertARGBToARGB32PM_lasx<false>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}

const uint *QT_FASTCALL fetchRGBA8888ToARGB32PM_lasx(uint *buffer, const uchar *src, int index, int count,
                                                     const QList<QRgb> *, QDitherInfo *)
{
    convertARGBToARGB32PM_lasx<true>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}

template<bool RGBA>
static void convertARGBToRGBA64PM_lasx(QRgba64 *buffer, const uint *src, qsizetype count)
{
    qsizetype i = 0;
    const __m256i alphaMask = __lasx_xvreplgr2vr_w(0xff000000);
    const __m256i rgbaMask = (__m256i)(v32i8){2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15,
                                              2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15};
    const __m256i shuffleMask = (__m256i)(v32i8){6, 7, 6, 7, 6, 7, 6, 7, 14, 15, 14, 15, 14, 15, 14, 15,
                                                 6, 7, 6, 7, 6, 7, 6, 7, 14, 15, 14, 15, 14, 15, 14, 15};
    const __m256i zero = __lasx_xvldi(0);

    for (; i < count - 7; i += 8) {
        __m256i dst1, dst2;
        __m256i srcVector = __lasx_xvld(reinterpret_cast<const __m256i *>(src + i), 0);
        const v8i32 testz = (v8i32)__lasx_xvmsknz_b(__lasx_xvand_v(srcVector, alphaMask));
        if (testz[0]!=0 || testz[4]!=0){
            const v8i32 testc = (v8i32)__lasx_xvmsknz_b(__lasx_xvandn_v(srcVector, alphaMask));
            bool cf = testc[0]==0 && testc[4]==0;
            if (!RGBA)
                srcVector = __lasx_xvshuf_b(zero, srcVector, rgbaMask);

            // The two unpack instructions unpack the low and upper halves of
            // each 128-bit half of the 256-bit register. Here's the tracking
            // of what's where: (p is 32-bit, P is 64-bit)
            //  as loaded:        [ p1, p2, p3, p4; p5, p6, p7, p8 ]
            //  after xvpermi_d   [ p1, p2, p5, p6; p3, p4, p7, p8 ]
            //  after xvilvl/h    [ P1, P2; P3, P4 ] [ P5, P6; P7, P8 ]
            srcVector = __lasx_xvpermi_d(srcVector, 0b11011000);
            const __m256i src1 = __lasx_xvilvl_b(srcVector, srcVector);
            const __m256i src2 = __lasx_xvilvh_b(srcVector, srcVector);
            if (!cf) {
                const __m256i alpha1 = __lasx_xvshuf_b(zero, src1, shuffleMask);
                const __m256i alpha2 = __lasx_xvshuf_b(zero, src2, shuffleMask);
                __m256i blendMask = (__m256i)(v16i16){0, 1, 2, 11, 4, 5, 6, 15, 0, 1, 2, 11, 4, 5, 6, 15};
                dst1 = __lasx_xvmuh_hu(src1, alpha1);
                dst2 = __lasx_xvmuh_hu(src2, alpha2);
                dst1 = __lasx_xvadd_h(dst1, __lasx_xvsrli_h(dst1, 15));
                dst2 = __lasx_xvadd_h(dst2, __lasx_xvsrli_h(dst2, 15));
                dst1 = __lasx_xvshuf_h(blendMask, src1, dst1);
                dst2 = __lasx_xvshuf_h(blendMask, src2, dst2);
            } else {
                dst1 = src1;
                dst2 = src2;
            }
        } else {
            dst1 = dst2 = zero;
        }
        __lasx_xvst(dst1, reinterpret_cast<__m256i *>(buffer + i), 0);
        __lasx_xvst(dst2, reinterpret_cast<__m256i *>(buffer + i) + 1, 0);
    }

    if (i < count) {
        __m256i epilogueMask = epilogueMaskFromCount(count - i);
        const __m256i epilogueMask1 = __lasx_xvslti_w(epilogueMask,0);
        __m256i srcVector = __lasx_xvbitsel_v(__lasx_xvldi(0),
                                              __lasx_xvld(reinterpret_cast<const int *>(src + i), 0),
                                              epilogueMask1);
        __m256i dst1, dst2;
        const __m256i epilogueMask2 = __lasx_xvslti_b(epilogueMask, 0);
        const __m256i epilogueAlphaMask = __lasx_xvbitsel_v(__lasx_xvldi(0),
                                                            alphaMask,
                                                            epilogueMask2);

        const v8i32 testz1 = (v8i32)__lasx_xvmsknz_b(__lasx_xvand_v(srcVector, epilogueAlphaMask));
        if (testz1[0]!=0 || testz1[4]!=0){
            const v8i32 testc1 = (v8i32)__lasx_xvmsknz_b(__lasx_xvandn_v(srcVector, epilogueAlphaMask));
            bool cf = testc1[0]==0 && testc1[4]==0;

            if (!RGBA)
                srcVector = __lasx_xvshuf_b(zero, srcVector, rgbaMask);
            srcVector = __lasx_xvpermi_d(srcVector, 0b11011000);
            const __m256i src1 = __lasx_xvilvl_b(srcVector, srcVector);
            const __m256i src2 = __lasx_xvilvh_b(srcVector, srcVector);
            if (!cf) {
                const __m256i alpha1 = __lasx_xvshuf_b(zero, src1, shuffleMask);
                const __m256i alpha2 = __lasx_xvshuf_b(zero, src2, shuffleMask);
                const __m256i blendMask = (__m256i)(v16i16){0, 1, 2, 11, 4, 5, 6, 15,
                                                            0, 1, 2, 11, 4, 5, 6, 15};
                dst1 = __lasx_xvmuh_hu(src1, alpha1);
                dst2 = __lasx_xvmuh_hu(src2, alpha2);
                dst1 = __lasx_xvadd_h(dst1, __lasx_xvsrli_h(dst1, 15));
                dst2 = __lasx_xvadd_h(dst2, __lasx_xvsrli_h(dst2, 15));
                dst1 = __lasx_xvshuf_h(blendMask, src1, dst1);
                dst2 = __lasx_xvshuf_h(blendMask, src2, dst2);
            } else {
                dst1 = src1;
                dst2 = src2;
            }
        } else {
            dst1 = dst2 = zero;
        }
        epilogueMask = __lasx_xvpermi_d(epilogueMask, 0b11011000);
        __m256i epilogueMaskl = __lasx_xvslti_d(__lasx_xvilvl_w(epilogueMask, epilogueMask), 0);
        __m256i epilogueMaskh = __lasx_xvslti_d(__lasx_xvilvh_w(epilogueMask, epilogueMask), 0);
        __m256i dst1V = __lasx_xvld(reinterpret_cast<qint64 *>(buffer + i), 0);
        dst1V = __lasx_xvbitsel_v(dst1V, dst1, epilogueMaskl);
        __lasx_xvst(dst1V, reinterpret_cast<qint64 *>(buffer + i), 0);
        __m256i dst2V = __lasx_xvld(reinterpret_cast<qint64 *>(buffer + i + 4), 0);
        dst2V = __lasx_xvbitsel_v(dst2V, dst2, epilogueMaskh);
        __lasx_xvst(dst2V, reinterpret_cast<qint64 *>(buffer + i + 4), 0);
    }
}

const QRgba64 * QT_FASTCALL convertARGB32ToRGBA64PM_lasx(QRgba64 *buffer, const uint *src, int count,
                                                         const QList<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_lasx<false>(buffer, src, count);
    return buffer;
}

const QRgba64 * QT_FASTCALL convertRGBA8888ToRGBA64PM_lasx(QRgba64 *buffer, const uint *src, int count,
                                                           const QList<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_lasx<true>(buffer, src, count);
    return buffer;
}

const QRgba64 *QT_FASTCALL fetchARGB32ToRGBA64PM_lasx(QRgba64 *buffer, const uchar *src, int index, int count,
                                                      const QList<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_lasx<false>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}

const QRgba64 *QT_FASTCALL fetchRGBA8888ToRGBA64PM_lasx(QRgba64 *buffer, const uchar *src, int index, int count,
                                                        const QList<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_lasx<true>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}

QT_END_NAMESPACE

#endif
