// Copyright (C) 2024 Loongson Technology Corporation Limited.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qdrawhelper_loongarch64_p.h>

#ifdef QT_COMPILER_SUPPORTS_LSX

#include <private/qdrawingprimitive_lsx_p.h>
#include <private/qpaintengine_raster_p.h>

QT_BEGIN_NAMESPACE

void qt_blend_argb32_on_argb32_lsx(uchar *destPixels, int dbpl,
                                   const uchar *srcPixels, int sbpl,
                                   int w, int h,
                                   int const_alpha)
{
    const quint32 *src = (const quint32 *) srcPixels;
    quint32 *dst = (quint32 *) destPixels;
    if (const_alpha == 256) {
        for (int y = 0; y < h; ++y) {
            BLEND_SOURCE_OVER_ARGB32_LSX(dst, src, w);
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    } else if (const_alpha != 0) {
        // dest = (s + d * sia) * ca + d * cia
        //      = s * ca + d * (sia * ca + cia)
        //      = s * ca + d * (1 - sa*ca)
        const_alpha = (const_alpha * 255) >> 8;

        for (int y = 0; y < h; ++y) {
            BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_LSX(dst, src, w, const_alpha);
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    }
}

// qblendfunctions.cpp
void qt_blend_rgb32_on_rgb32(uchar *destPixels, int dbpl,
                             const uchar *srcPixels, int sbpl,
                             int w, int h,
                             int const_alpha);

void qt_blend_rgb32_on_rgb32_lsx(uchar *destPixels, int dbpl,
                                 const uchar *srcPixels, int sbpl,
                                 int w, int h,
                                 int const_alpha)
{
    const quint32 *src = (const quint32 *) srcPixels;
    quint32 *dst = (quint32 *) destPixels;
    if (const_alpha != 256) {
        if (const_alpha != 0) {
            const __m128i half = __lsx_vreplgr2vr_h(0x80);
            const __m128i colorMask = __lsx_vreplgr2vr_w(0x00ff00ff);

            const_alpha = (const_alpha * 255) >> 8;
            int one_minus_const_alpha = 255 - const_alpha;
            const __m128i constAlphaVector = __lsx_vreplgr2vr_h(const_alpha);
            const __m128i oneMinusConstAlpha =  __lsx_vreplgr2vr_h(one_minus_const_alpha);
            for (int y = 0; y < h; ++y) {
                int x = 0;

                // First, align dest to 16 bytes:
                ALIGNMENT_PROLOGUE_16BYTES(dst, x, w) {
                    dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha,
                                                   dst[x], one_minus_const_alpha);
                }

                for (; x < w-3; x += 4) {
                    __m128i srcVector = __lsx_vld(&src[x], 0);
                    __m128i dstVector = __lsx_vld(&dst[x], 0);
                    INTERPOLATE_PIXEL_255_LSX(srcVector, dstVector, constAlphaVector,
                                              oneMinusConstAlpha, colorMask, half);
                    __lsx_vst(dstVector, &dst[x], 0);
                }
                SIMD_EPILOGUE(x, w, 3)
                    dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha,
                                                   dst[x], one_minus_const_alpha);
                dst = (quint32 *)(((uchar *) dst) + dbpl);
                src = (const quint32 *)(((const uchar *) src) + sbpl);
            }
        }
    } else {
        qt_blend_rgb32_on_rgb32(destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
    }
}

void QT_FASTCALL comp_func_SourceOver_lsx(uint *destPixels, const uint *srcPixels,
                                          int length, uint const_alpha)
{
    Q_ASSERT(const_alpha < 256);

    const quint32 *src = (const quint32 *) srcPixels;
    quint32 *dst = (quint32 *) destPixels;

    if (const_alpha == 255) {
        BLEND_SOURCE_OVER_ARGB32_LSX(dst, src, length);
    } else {
        BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_LSX(dst, src, length, const_alpha);
    }
}

void QT_FASTCALL comp_func_Plus_lsx(uint *dst, const uint *src, int length, uint const_alpha)
{
    int x = 0;

    if (const_alpha == 255) {
        // 1) Prologue: align destination on 16 bytes
        ALIGNMENT_PROLOGUE_16BYTES(dst, x, length)
            dst[x] = comp_func_Plus_one_pixel(dst[x], src[x]);

        // 2) composition with LSX
        for (; x < length - 3; x += 4) {
            const __m128i srcVector = __lsx_vld(&src[x], 0);
            const __m128i dstVector = __lsx_vld(&dst[x], 0);

            const __m128i result = __lsx_vsadd_bu(srcVector, dstVector);
            __lsx_vst(result, &dst[x], 0);
        }

        // 3) Epilogue:
        SIMD_EPILOGUE(x, length, 3)
            dst[x] = comp_func_Plus_one_pixel(dst[x], src[x]);
    } else {
        const int one_minus_const_alpha = 255 - const_alpha;
        const __m128i constAlphaVector = __lsx_vreplgr2vr_h(const_alpha);
        const __m128i oneMinusConstAlpha =  __lsx_vreplgr2vr_h(one_minus_const_alpha);

        // 1) Prologue: align destination on 16 bytes
        ALIGNMENT_PROLOGUE_16BYTES(dst, x, length)
            dst[x] = comp_func_Plus_one_pixel_const_alpha(dst[x], src[x],
                                                          const_alpha,
                                                          one_minus_const_alpha);

        const __m128i half = __lsx_vreplgr2vr_h(0x80);
        const __m128i colorMask = __lsx_vreplgr2vr_w(0x00ff00ff);
        // 2) composition with LSX
        for (; x < length - 3; x += 4) {
            const __m128i srcVector = __lsx_vld(&src[x], 0);
            __m128i dstVector = __lsx_vld(&dst[x], 0);
            __m128i result = __lsx_vsadd_bu(srcVector, dstVector);
            INTERPOLATE_PIXEL_255_LSX(result, dstVector, constAlphaVector,
                                      oneMinusConstAlpha, colorMask, half);
            __lsx_vst(dstVector, &dst[x], 0);
        }

        // 3) Epilogue:
        SIMD_EPILOGUE(x, length, 3)
            dst[x] = comp_func_Plus_one_pixel_const_alpha(dst[x], src[x],
                                                          const_alpha, one_minus_const_alpha);
    }
}

void QT_FASTCALL comp_func_Source_lsx(uint *dst, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        ::memcpy(dst, src, length * sizeof(uint));
    } else {
        const int ialpha = 255 - const_alpha;

        int x = 0;

        // 1) prologue, align on 16 bytes
        ALIGNMENT_PROLOGUE_16BYTES(dst, x, length)
            dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], ialpha);

        // 2) interpolate pixels with LSX
        const __m128i half = __lsx_vreplgr2vr_h(0x80);
        const __m128i colorMask = __lsx_vreplgr2vr_w(0x00ff00ff);

        const __m128i constAlphaVector = __lsx_vreplgr2vr_h(const_alpha);
        const __m128i oneMinusConstAlpha =  __lsx_vreplgr2vr_h(ialpha);
        for (; x < length - 3; x += 4) {
            const __m128i srcVector = __lsx_vld(&src[x], 0);
            __m128i dstVector = __lsx_vld(&dst[x], 0);
            INTERPOLATE_PIXEL_255_LSX(srcVector, dstVector, constAlphaVector,
                                      oneMinusConstAlpha, colorMask, half);
            __lsx_vst(dstVector, &dst[x], 0);
        }

        // 3) Epilogue
        SIMD_EPILOGUE(x, length, 3)
            dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], ialpha);
    }
}

static Q_NEVER_INLINE
void Q_DECL_VECTORCALL qt_memfillXX_aligned(void *dest, __m128i value128, quintptr bytecount)
{
    __m128i *dst128 = reinterpret_cast<__m128i *>(dest);
    __m128i *end128 = reinterpret_cast<__m128i *>(static_cast<uchar *>(dest) + bytecount);

    while (dst128 + 4 <= end128) {
        __lsx_vst(value128, dst128 + 0, 0);
        __lsx_vst(value128, dst128 + 1, 0);
        __lsx_vst(value128, dst128 + 2, 0);
        __lsx_vst(value128, dst128 + 3, 0);
        dst128 += 4;
    }

    bytecount %= 4 * sizeof(__m128i);
    switch (bytecount / sizeof(__m128i)) {
    case 3: __lsx_vst(value128, dst128++, 0); Q_FALLTHROUGH();
    case 2: __lsx_vst(value128, dst128++, 0); Q_FALLTHROUGH();
    case 1: __lsx_vst(value128, dst128++, 0);
    }
}

void qt_memfill64_lsx(quint64 *dest, quint64 value, qsizetype count)
{
    quintptr misaligned = quintptr(dest) % sizeof(__m128i);
    if (misaligned && count) {
        *dest++ = value;
        --count;
    }

    if (count % 2) {
        dest[count - 1] = value;
        --count;
    }

    qt_memfillXX_aligned(dest, __lsx_vreplgr2vr_d(value), count * sizeof(quint64));
}

void qt_memfill32_lsx(quint32 *dest, quint32 value, qsizetype count)
{
    if (count < 4) {
        // this simplifies the code below: the first switch can fall through
        // without checking the value of count
        switch (count) {
        case 3: *dest++ = value; Q_FALLTHROUGH();
        case 2: *dest++ = value; Q_FALLTHROUGH();
        case 1: *dest   = value;
        }
        return;
    }

    const int align = (quintptr)(dest) & 0xf;
    switch (align) {
    case 4:  *dest++ = value; --count; Q_FALLTHROUGH();
    case 8:  *dest++ = value; --count; Q_FALLTHROUGH();
    case 12: *dest++ = value; --count;
    }

    const int rest = count & 0x3;
    if (rest) {
        switch (rest) {
        case 3: dest[count - 3] = value; Q_FALLTHROUGH();
        case 2: dest[count - 2] = value; Q_FALLTHROUGH();
        case 1: dest[count - 1] = value;
        }
    }

    qt_memfillXX_aligned(dest, __lsx_vreplgr2vr_w(value), count * sizeof(quint32));
}

void QT_FASTCALL comp_func_solid_Source_lsx(uint *destPixels, int length,
                                            uint color, uint const_alpha)
{
    if (const_alpha == 255) {
        qt_memfill32(destPixels, color, length);
    } else {
        const quint32 ialpha = 255 - const_alpha;
        color = BYTE_MUL(color, const_alpha);
        int x = 0;

        quint32 *dst = (quint32 *) destPixels;
        const __m128i colorVector = __lsx_vreplgr2vr_w(color);
        const __m128i colorMask = __lsx_vreplgr2vr_w(0x00ff00ff);
        const __m128i half = __lsx_vreplgr2vr_h(0x80);
        const __m128i iAlphaVector = __lsx_vreplgr2vr_h(ialpha);

        ALIGNMENT_PROLOGUE_16BYTES(dst, x, length)
            destPixels[x] = color + BYTE_MUL(destPixels[x], ialpha);

        for (; x < length-3; x += 4) {
            __m128i dstVector = __lsx_vld(&dst[x], 0);
            BYTE_MUL_LSX(dstVector, iAlphaVector, colorMask, half);
            dstVector = __lsx_vadd_b(colorVector, dstVector);
            __lsx_vst(dstVector, &dst[x], 0);
        }
        SIMD_EPILOGUE(x, length, 3)
            destPixels[x] = color + BYTE_MUL(destPixels[x], ialpha);
    }
}

void QT_FASTCALL comp_func_solid_SourceOver_lsx(uint *destPixels, int length,
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
        const __m128i colorVector = __lsx_vreplgr2vr_w(color);
        const __m128i colorMask = __lsx_vreplgr2vr_w(0x00ff00ff);
        const __m128i half = __lsx_vreplgr2vr_h(0x80);
        const __m128i minusAlphaOfColorVector = __lsx_vreplgr2vr_h(minusAlphaOfColor);

        ALIGNMENT_PROLOGUE_16BYTES(dst, x, length)
            destPixels[x] = color + BYTE_MUL(destPixels[x], minusAlphaOfColor);

        for (; x < length-3; x += 4) {
            __m128i dstVector = __lsx_vld(&dst[x], 0);
            BYTE_MUL_LSX(dstVector, minusAlphaOfColorVector, colorMask, half);
            dstVector = __lsx_vadd_b(colorVector, dstVector);
            __lsx_vst(dstVector, &dst[x], 0);
        }
        SIMD_EPILOGUE(x, length, 3)
            destPixels[x] = color + BYTE_MUL(destPixels[x], minusAlphaOfColor);
    }
}

void qt_bitmapblit32_lsx_base(QRasterBuffer *rasterBuffer, int x, int y,
                              quint32 color,
                              const uchar *src, int width, int height, int stride)
{
    quint32 *dest = reinterpret_cast<quint32*>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->stride<quint32>();

    const __m128i c128 = __lsx_vreplgr2vr_w(color);
    const __m128i maskmask1 = (__m128i)(v4u32){0x80808080, 0x40404040,
                                               0x20202020, 0x10101010};
    const __m128i maskadd1 = (__m128i)(v4i32){0x00000000, 0x40404040,
                                              0x60606060, 0x70707070};

    if (width > 4) {
        const __m128i maskmask2 = (__m128i)(v4i32){0x08080808, 0x04040404,
                                                   0x02020202, 0x01010101};
        const __m128i maskadd2 = (__m128i)(v4i32){0x78787878, 0x7c7c7c7c,
                                                  0x7e7e7e7e, 0x7f7f7f7f};
        while (height--) {
            for (int x = 0; x < width; x += 8) {
                const quint8 s = src[x >> 3];
                if (!s)
                    continue;
                __m128i mask1 = __lsx_vreplgr2vr_b(s);
                __m128i mask2 = mask1;

                mask1 = __lsx_vand_v(mask1, maskmask1);
                mask1 = __lsx_vadd_b(mask1, maskadd1);

                __m128i destSrc1 = __lsx_vld((char*)(dest + x), 0);

                mask1 = __lsx_vslti_b(mask1,0);
                destSrc1 = __lsx_vbitsel_v(destSrc1, c128, mask1);
                __lsx_vst(destSrc1, (char*)(dest + x), 0);

                __m128i destSrc2 = __lsx_vld((char*)(dest + x + 4), 0);

                mask2 = __lsx_vand_v(mask2, maskmask2);
                mask2 = __lsx_vadd_b(mask2, maskadd2);

                mask2 = __lsx_vslti_b(mask2,0);
                destSrc2 = __lsx_vbitsel_v(destSrc2, c128, mask2);
                __lsx_vst(destSrc2, (char*)(dest + x + 4), 0);
            }
            dest += destStride;
            src += stride;
        }
    } else {
        while (height--) {
            const quint8 s = *src;
            if (s) {
                __m128i mask1 = __lsx_vreplgr2vr_b(s);

                __m128i destSrc1 = __lsx_vld((char*)(dest), 0);
                mask1 = __lsx_vand_v(mask1, maskmask1);
                mask1 = __lsx_vadd_b(mask1, maskadd1);

                mask1 = __lsx_vslti_b(mask1, 0);
                destSrc1 = __lsx_vbitsel_v(destSrc1, c128, mask1);
                __lsx_vst(destSrc1, (char*)(dest), 0);
            }
            dest += destStride;
            src += stride;
        }
    }
}

void qt_bitmapblit32_lsx(QRasterBuffer *rasterBuffer, int x, int y,
                         const QRgba64 &color,
                         const uchar *src, int width, int height, int stride)
{
    qt_bitmapblit32_lsx_base(rasterBuffer, x, y, color.toArgb32(), src, width, height, stride);
}

void qt_bitmapblit8888_lsx(QRasterBuffer *rasterBuffer, int x, int y,
                           const QRgba64 &color,
                           const uchar *src, int width, int height, int stride)
{
    qt_bitmapblit32_lsx_base(rasterBuffer, x, y, ARGB2RGBA(color.toArgb32()), src, width, height, stride);
}

void qt_bitmapblit16_lsx(QRasterBuffer *rasterBuffer, int x, int y,
                         const QRgba64 &color,
                         const uchar *src, int width, int height, int stride)
{
    const quint16 c = qConvertRgb32To16(color.toArgb32());
    quint16 *dest = reinterpret_cast<quint16*>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->stride<quint32>();

    const __m128i c128 = __lsx_vreplgr2vr_h(c);
    const __m128i maskmask = (__m128i)(v8u16){0x8080, 0x4040, 0x2020, 0x1010,
                                              0x0808, 0x0404, 0x0202, 0x0101};

    const __m128i maskadd = (__m128i)(v8i16){0x0000, 0x4040, 0x6060, 0x7070,
                                             0x7878, 0x7c7c, 0x7e7e, 0x7f7f};
    while (--height >= 0) {
        for (int x = 0; x < width; x += 8) {
            const quint8 s = src[x >> 3];
            if (!s)
                continue;
            __m128i mask = __lsx_vreplgr2vr_b(s);
            __m128i destSrc = __lsx_vld((char*)(dest + x), 0);
            mask = __lsx_vand_v(mask, maskmask);
            mask = __lsx_vadd_b(mask, maskadd);
            mask = __lsx_vslti_b(mask, 0);
            destSrc = __lsx_vbitsel_v(destSrc, c128, mask);
            __lsx_vst(destSrc, (char*)(dest + x), 0);
        }
        dest += destStride;
        src += stride;
    }
}

class QSimdLsx
{
public:
    typedef __m128i Int32x4;
    typedef __m128 Float32x4;

    union Vect_buffer_i { Int32x4 v; int i[4]; };
    union Vect_buffer_f { Float32x4 v; float f[4]; };

    static inline Float32x4 Q_DECL_VECTORCALL v_dup(float x) { return __lsx_vreplfr2vr_s(x); }
    static inline Float32x4 Q_DECL_VECTORCALL v_dup(double x) { return __lsx_vreplfr2vr_s(x); }
    static inline Int32x4 Q_DECL_VECTORCALL v_dup(int x) { return __lsx_vreplgr2vr_w(x); }
    static inline Int32x4 Q_DECL_VECTORCALL v_dup(uint x) { return __lsx_vreplgr2vr_w(x); }

    static inline Float32x4 Q_DECL_VECTORCALL v_add(Float32x4 a, Float32x4 b) { return __lsx_vfadd_s(a, b); }
    static inline Int32x4 Q_DECL_VECTORCALL v_add(Int32x4 a, Int32x4 b) { return __lsx_vadd_w(a, b); }

    static inline Float32x4 Q_DECL_VECTORCALL v_max(Float32x4 a, Float32x4 b) { return __lsx_vfmax_s(a, b); }
    static inline Float32x4 Q_DECL_VECTORCALL v_min(Float32x4 a, Float32x4 b) { return __lsx_vfmin_s(a, b); }
    static inline Int32x4 Q_DECL_VECTORCALL v_min_16(Int32x4 a, Int32x4 b) { return __lsx_vmin_h(a, b); }

    static inline Int32x4 Q_DECL_VECTORCALL v_and(Int32x4 a, Int32x4 b) { return __lsx_vand_v(a, b); }

    static inline Float32x4 Q_DECL_VECTORCALL v_sub(Float32x4 a, Float32x4 b) { return __lsx_vfsub_s(a, b); }
    static inline Int32x4 Q_DECL_VECTORCALL v_sub(Int32x4 a, Int32x4 b) { return __lsx_vsub_w(a, b); }

    static inline Float32x4 Q_DECL_VECTORCALL v_mul(Float32x4 a, Float32x4 b) { return __lsx_vfmul_s(a, b); }

    static inline Float32x4 Q_DECL_VECTORCALL v_sqrt(Float32x4 x) { return __lsx_vfsqrt_s(x); }

    static inline Int32x4 Q_DECL_VECTORCALL v_toInt(Float32x4 x) { return __lsx_vftintrz_w_s(x); }

    static inline Int32x4 Q_DECL_VECTORCALL v_greaterOrEqual(Float32x4 a, Float32x4 b) { return __lsx_vfcmp_clt_s(b, a); }
};

const uint * QT_FASTCALL qt_fetch_radial_gradient_lsx(uint *buffer, const Operator *op,
                                                      const QSpanData *data,
                                                      int y, int x, int length)
{
    return qt_fetch_radial_gradient_template<QRadialFetchSimd<QSimdLsx>,uint>(buffer, op, data, y, x, length);
}

void qt_scale_image_argb32_on_argb32_lsx(uchar *destPixels, int dbpl,
                                         const uchar *srcPixels, int sbpl, int srch,
                                         const QRectF &targetRect,
                                         const QRectF &sourceRect,
                                         const QRect &clip,
                                         int const_alpha)
{
    if (const_alpha != 256) {
        // from qblendfunctions.cpp
        extern void qt_scale_image_argb32_on_argb32(uchar *destPixels, int dbpl,
                                                    const uchar *srcPixels, int sbpl, int srch,
                                                    const QRectF &targetRect,
                                                    const QRectF &sourceRect,
                                                    const QRect &clip,
                                                    int const_alpha);
        return qt_scale_image_argb32_on_argb32(destPixels, dbpl, srcPixels, sbpl, srch,
                                               targetRect, sourceRect, clip, const_alpha);
    }

    qreal sx = sourceRect.width() / (qreal)targetRect.width();
    qreal sy = sourceRect.height() / (qreal)targetRect.height();


    const int ix = 0x00010000 * sx;
    const int iy = 0x00010000 * sy;

    QRect tr = targetRect.normalized().toRect();
    tr = tr.intersected(clip);
    if (tr.isEmpty())
        return;
    const int tx1 = tr.left();
    const int ty1 = tr.top();
    int h = tr.height();
    int w = tr.width();

    quint32 basex;
    quint32 srcy;

    if (sx < 0) {
        int dstx = qFloor((tx1 + qreal(0.5) - targetRect.right()) * sx * 65536) + 1;
        basex = quint32(sourceRect.right() * 65536) + dstx;
    } else {
        int dstx = qCeil((tx1 + qreal(0.5) - targetRect.left()) * sx * 65536) - 1;
        basex = quint32(sourceRect.left() * 65536) + dstx;
    }
    if (sy < 0) {
        int dsty = qFloor((ty1 + qreal(0.5) - targetRect.bottom()) * sy * 65536) + 1;
        srcy = quint32(sourceRect.bottom() * 65536) + dsty;
    } else {
        int dsty = qCeil((ty1 + qreal(0.5) - targetRect.top()) * sy * 65536) - 1;
        srcy = quint32(sourceRect.top() * 65536) + dsty;
    }

    quint32 *dst = ((quint32 *) (destPixels + ty1 * dbpl)) + tx1;

    const __m128i nullVector = __lsx_vreplgr2vr_w(0);
    const __m128i half = __lsx_vreplgr2vr_h(0x80);
    const __m128i one = __lsx_vreplgr2vr_h(0xff);
    const __m128i colorMask = __lsx_vreplgr2vr_w(0x00ff00ff);
    const __m128i alphaMask = __lsx_vreplgr2vr_w(0xff000000);
    const __m128i ixVector = __lsx_vreplgr2vr_w(4*ix);

    // this bounds check here is required as floating point rounding above might in some cases lead to
    // w/h values that are one pixel too large, falling outside of the valid image area.
    const int ystart = srcy >> 16;
    if (ystart >= srch && iy < 0) {
        srcy += iy;
        --h;
    }
    const int xstart = basex >> 16;
    if (xstart >=  (int)(sbpl/sizeof(quint32)) && ix < 0) {
        basex += ix;
        --w;
    }
    int yend = (srcy + iy * (h - 1)) >> 16;
    if (yend < 0 || yend >= srch)
        --h;
    int xend = (basex + ix * (w - 1)) >> 16;
    if (xend < 0 || xend >= (int)(sbpl/sizeof(quint32)))
        --w;

    while (--h >= 0) {
        const uint *src = (const quint32 *) (srcPixels + (srcy >> 16) * sbpl);
        int srcx = basex;
        int x = 0;

        ALIGNMENT_PROLOGUE_16BYTES(dst, x, w) {
            uint s = src[srcx >> 16];
            dst[x] = s + BYTE_MUL(dst[x], qAlpha(~s));
            srcx += ix;
        }

        __m128i srcxVector = (__m128i)(v4i32){srcx + ix + ix + ix, srcx + ix + ix, srcx + ix, srcx};

        for (; x < (w - 3); x += 4) {
            const int idx0 = __lsx_vpickve2gr_h(srcxVector, 1);
            const int idx1 = __lsx_vpickve2gr_h(srcxVector, 3);
            const int idx2 = __lsx_vpickve2gr_h(srcxVector, 5);
            const int idx3 = __lsx_vpickve2gr_h(srcxVector, 7);
            srcxVector = __lsx_vadd_w(srcxVector, ixVector);

            const __m128i srcVector = (__m128i)((v4u32){src[idx3], src[idx2], src[idx1], src[idx0]});

            BLEND_SOURCE_OVER_ARGB32_LSX_helper(dst, x, srcVector, nullVector, half, one, colorMask, alphaMask);
        }

        SIMD_EPILOGUE(x, w, 3) {
            uint s = src[(basex + x*ix) >> 16];
            dst[x] = s + BYTE_MUL(dst[x], qAlpha(~s));
        }
        dst = (quint32 *)(((uchar *) dst) + dbpl);
        srcy += iy;
    }
}

const uint *QT_FASTCALL fetchPixelsBPP24_lsx(uint *buffer, const uchar *src, int index, int count)
{
    const quint24 *s = reinterpret_cast<const quint24 *>(src);
    for (int i = 0; i < count; ++i)
        buffer[i] = s[index + i];
    return buffer;
}

const uint * QT_FASTCALL qt_fetchUntransformed_888_lsx(uint *buffer, const Operator *,
                                                       const QSpanData *data,
                                                       int y, int x, int length)
{
    const uchar *line = data->texture.scanLine(y) + x * 3;
    // from image/qimage_lsx.cpp
    extern void QT_FASTCALL qt_convert_rgb888_to_rgb32_lsx(quint32 *dst, const uchar *src, int len);
    qt_convert_rgb888_to_rgb32_lsx(buffer, line, length);
    return buffer;
}

void qt_memfill24_lsx(quint24 *dest, quint24 color, qsizetype count)
{
    // LCM of 12 and 16 bytes is 48 bytes (16 px)
    quint32 v = color;
    __m128i m = __lsx_vinsgr2vr_w(__lsx_vldi(0), v, 0);
    quint24 *end = dest + count;

    constexpr uchar x = 2, y = 1, z = 0;
    alignas(__m128i) static const uchar
    shuffleMask[16 + 1] = { x, y, z, x,  y, z, x, y,  z, x, y, z,  x, y, z, x,  y };
    __m128i indexMask = (__m128i)(v16i8){2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};

    __m128i mval1 = __lsx_vshuf_b(m, m, __lsx_vld(reinterpret_cast<const __m128i *>(shuffleMask), 0));
    __m128i mval2 = __lsx_vshuf_b(m, m, __lsx_vld(reinterpret_cast<const __m128i *>(shuffleMask + 1), 0));
    __m128i mval3 = __lsx_vshuf_b(mval2, mval1, indexMask);

    for ( ; dest + 16 <= end; dest += 16) {
        __lsx_vst(mval1, reinterpret_cast<__m128i *>(dest) + 0, 0);
        __lsx_vst(mval2, reinterpret_cast<__m128i *>(dest) + 1, 0);
        __lsx_vst(mval3, reinterpret_cast<__m128i *>(dest) + 2, 0);
    }

    if (count < 3) {
        if (count > 1)
            end[-2] = v;
        if (count)
            end[-1] = v;
        return;
    }

    // less than 16px/48B left
    uchar *ptr = reinterpret_cast<uchar *>(dest);
    uchar *ptr_end = reinterpret_cast<uchar *>(end);
    qptrdiff left = ptr_end - ptr;
    if (left >= 24) {
        // 8px/24B or more left
        __lsx_vst(mval1, reinterpret_cast<__m128i *>(ptr) + 0, 0);
        __lsx_vstelm_d(mval2, reinterpret_cast<__m128i *>(ptr) + 1, 0, 0);
        ptr += 24;
        left -= 24;
    }

    // less than 8px/24B left

    if (left >= 16) {
        // but more than 5px/15B left
        __lsx_vst(mval1, reinterpret_cast<__m128i *>(ptr) , 0);
    } else if (left >= 8) {
        // but more than 2px/6B left
        __lsx_vstelm_d(mval1, reinterpret_cast<__m128i *>(ptr), 0, 0);
    }

    if (left) {
        // 1 or 2px left
        // store 8 bytes ending with the right values (will overwrite a bit)
        __lsx_vstelm_d(mval2, reinterpret_cast<__m128i *>(ptr_end - 8), 0, 0);
    }
}

void QT_FASTCALL rbSwap_888_lsx(uchar *dst, const uchar *src, int count)
{
    int i = 0;
    const static __m128i shuffleMask1 = (__m128i)(v16i8){2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9, 14, 13, 12, 15};
    const static __m128i shuffleMask2 = (__m128i)(v16i8){0, 1, 4, 3, 2, 7, 6, 5, 10, 9, 8, 13, 12, 11, 14, 15};
    const static __m128i shuffleMask3 = (__m128i)(v16i8){0, 3, 2, 1, 6, 5, 4, 9, 8, 7, 12, 11, 10, 15, 14, 13};

    for (; i + 15 < count; i += 16) {
        __m128i s1 = __lsx_vld(src, 0);
        __m128i s2 = __lsx_vld((src + 16), 0);
        __m128i s3 = __lsx_vld((src + 32), 0);
        s1 = __lsx_vshuf_b(s1, s1, shuffleMask1);
        s2 = __lsx_vshuf_b(s2, s2, shuffleMask2);
        s3 = __lsx_vshuf_b(s3, s3, shuffleMask3);
        __lsx_vst(s1, dst, 0);
        __lsx_vst(s2, (dst + 16), 0);
        __lsx_vst(s3, (dst + 32), 0);

        // Now fix the last four misplaced values
        std::swap(dst[15], dst[17]);
        std::swap(dst[30], dst[32]);

        src += 48;
        dst += 48;
    }

    if (src != dst) {
        SIMD_EPILOGUE(i, count, 15) {
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
            dst += 3;
            src += 3;
        }
    } else {
        SIMD_EPILOGUE(i, count, 15) {
            std::swap(dst[0], dst[2]);
            dst += 3;
        }
    }
}

template<bool RGBA>
static void convertARGBToARGB32PM_lsx(uint *buffer, const uint *src, int count)
{
    int i = 0;
    const __m128i alphaMask = __lsx_vreplgr2vr_w(0xff000000);
    const __m128i rgbaMask = (__m128i)(v16i8){2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15};
    const __m128i shuffleMask = (__m128i)(v16i8){6, 7, 6, 7, 6, 7, 6, 7, 14, 15, 14, 15, 14, 15, 14, 15};
    const __m128i half = __lsx_vreplgr2vr_h(0x0080);
    const __m128i zero = __lsx_vldi(0);

    for (; i < count - 3; i += 4) {
        __m128i srcVector = __lsx_vld(&src[i], 0);
        const v4i32 testz = (v4i32)__lsx_vmsknz_b(__lsx_vand_v(srcVector, alphaMask));
        if (testz[0]!=0) {
            const v4i32 testc = (v4i32)__lsx_vmsknz_b(__lsx_vandn_v(srcVector, alphaMask));
            if (testc[0]!=0) {
                if (RGBA)
                    srcVector = __lsx_vshuf_b(zero, srcVector, rgbaMask);
                __m128i src1 = __lsx_vilvl_b(zero, srcVector);
                __m128i src2 = __lsx_vilvh_b(zero, srcVector);
                __m128i alpha1 = __lsx_vshuf_b(zero, src1, shuffleMask);
                __m128i alpha2 = __lsx_vshuf_b(zero, src2, shuffleMask);
                src1 = __lsx_vmul_h(src1, alpha1);
                src2 = __lsx_vmul_h(src2, alpha2);
                src1 = __lsx_vadd_h(src1, __lsx_vsrli_h(src1, 8));
                src2 = __lsx_vadd_h(src2, __lsx_vsrli_h(src2, 8));
                src1 = __lsx_vadd_h(src1, half);
                src2 = __lsx_vadd_h(src2, half);
                src1 = __lsx_vsrli_h(src1, 8);
                src2 = __lsx_vsrli_h(src2, 8);
                __m128i blendMask = (__m128i)(v8i16){0, 1, 2, 11, 4, 5, 6, 15};
                src1 = __lsx_vshuf_h(blendMask, alpha1, src1);
                src2 = __lsx_vshuf_h(blendMask, alpha2, src2);
                src1 = __lsx_vmaxi_h(src1, 0);
                src2 = __lsx_vmaxi_h(src2, 0);
                srcVector = __lsx_vpickev_b(__lsx_vsat_hu(src2, 7), __lsx_vsat_hu(src1, 7));
                __lsx_vst(srcVector, &buffer[i], 0);
            } else {
                if (RGBA)
                    __lsx_vst(__lsx_vshuf_b(zero, srcVector, rgbaMask), &buffer[i], 0);
                else if (buffer != src)
                    __lsx_vst(srcVector, &buffer[i], 0);
            }
        } else {
            __lsx_vst(zero, &buffer[i], 0);
        }
    }

    SIMD_EPILOGUE(i, count, 3) {
        uint v = qPremultiply(src[i]);
        buffer[i] = RGBA ? RGBA2ARGB(v) : v;
    }
}

template<bool RGBA>
static void convertARGBToRGBA64PM_lsx(QRgba64 *buffer, const uint *src, int count)
{
    int i = 0;
    const __m128i alphaMask = __lsx_vreplgr2vr_w(0xff000000);
    const __m128i rgbaMask = (__m128i)(v16i8){2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15};
    const __m128i shuffleMask = (__m128i)(v16i8){6, 7, 6, 7, 6, 7, 6, 7, 14, 15, 14, 15, 14, 15, 14, 15};
    const __m128i zero = __lsx_vldi(0);

    for (; i < count - 3; i += 4) {
        __m128i srcVector = __lsx_vld(&src[i], 0);
        const v4i32 testz = (v4i32)__lsx_vmsknz_b(__lsx_vand_v(srcVector, alphaMask));
        if (testz[0]!=0) {
            const v4i32 testc = (v4i32)__lsx_vmsknz_b(__lsx_vandn_v(srcVector, alphaMask));
            if (!RGBA)
                srcVector = __lsx_vshuf_b(zero, srcVector, rgbaMask);
            const __m128i src1 = __lsx_vilvl_b(srcVector, srcVector);
            const __m128i src2 = __lsx_vilvh_b(srcVector, srcVector);
            if (testc[0]!=0) {
                __m128i alpha1 = __lsx_vshuf_b(zero, src1, shuffleMask);
                __m128i alpha2 = __lsx_vshuf_b(zero, src2, shuffleMask);
                __m128i dst1 = __lsx_vmuh_hu(src1, alpha1);
                __m128i dst2 = __lsx_vmuh_hu(src2, alpha2);
                // Map 0->0xfffe to 0->0xffff
                dst1 = __lsx_vadd_h(dst1, __lsx_vsrli_h(dst1, 15));
                dst2 = __lsx_vadd_h(dst2, __lsx_vsrli_h(dst2, 15));
                // correct alpha value:
                const __m128i blendMask = (__m128i)(v8i16){0, 1, 2, 11, 4, 5, 6, 15};
                dst1 = __lsx_vshuf_h(blendMask, src1, dst1);
                dst2 = __lsx_vshuf_h(blendMask, src2, dst2);
                __lsx_vst(dst1, &buffer[i], 0);
                __lsx_vst(dst2, &buffer[i + 2], 0);
            } else {
                __lsx_vst(src1, &buffer[i], 0);
                __lsx_vst(src2, &buffer[i + 2], 0);
            }
        } else {
            __lsx_vst(zero, &buffer[i], 0);
            __lsx_vst(zero, &buffer[i + 2], 0);
        }
    }

    SIMD_EPILOGUE(i, count, 3) {
        const uint s = RGBA ? RGBA2ARGB(src[i]) : src[i];
        buffer[i] = QRgba64::fromArgb32(s).premultiplied();
    }
}

template<bool RGBA, bool RGBx>
static inline void convertARGBFromARGB32PM_lsx(uint *buffer, const uint *src, int count)
{
    int i = 0;
    const __m128i alphaMask = __lsx_vreplgr2vr_w(0xff000000);
    const __m128i rgbaMask = (__m128i)(v16i8){2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15};
    const __m128i zero = __lsx_vldi(0);

    for (; i < count - 3; i += 4) {
        __m128i srcVector = __lsx_vld(&src[i], 0);
        const v4i32 testz = (v4i32)__lsx_vmsknz_b(__lsx_vand_v(srcVector, alphaMask));
        if (testz[0]!=0) {
            const v4i32 testc = (v4i32)__lsx_vmsknz_b(__lsx_vandn_v(srcVector, alphaMask));
            if (testc[0]!=0) {
                __m128i srcVectorAlpha = __lsx_vsrli_w(srcVector, 24);
                if (RGBA)
                    srcVector = __lsx_vshuf_b(zero, srcVector, rgbaMask);
                const __m128 a = __lsx_vffint_s_w(srcVectorAlpha);
                const __m128 ia = reciprocal_mul_ps(a, 255.0f);
                __m128i src1 = __lsx_vilvl_b(zero, srcVector);
                __m128i src3 = __lsx_vilvh_b(zero, srcVector);
                __m128i src2 = __lsx_vilvh_h(zero, src1);
                __m128i src4 = __lsx_vilvh_h(zero, src3);
                src1 = __lsx_vilvl_h(zero, src1);
                src3 = __lsx_vilvl_h(zero, src3);
                __m128 ia1 = (__m128)__lsx_vreplvei_w(ia, 0);
                __m128 ia2 = (__m128)__lsx_vreplvei_w(ia, 1);
                __m128 ia3 = (__m128)__lsx_vreplvei_w(ia, 2);
                __m128 ia4 = (__m128)__lsx_vreplvei_w(ia, 3);
                src1 = __lsx_vftintrne_w_s(__lsx_vfmul_s(__lsx_vffint_s_w(src1), ia1));
                src2 = __lsx_vftintrne_w_s(__lsx_vfmul_s(__lsx_vffint_s_w(src2), ia2));
                src3 = __lsx_vftintrne_w_s(__lsx_vfmul_s(__lsx_vffint_s_w(src3), ia3));
                src4 = __lsx_vftintrne_w_s(__lsx_vfmul_s(__lsx_vffint_s_w(src4), ia4));
                src1 = __lsx_vpickev_h(__lsx_vsat_wu(src2, 15), __lsx_vsat_wu(src1, 15));
                src3 = __lsx_vpickev_h(__lsx_vsat_wu(src4, 15), __lsx_vsat_wu(src3, 15));
                src1 = __lsx_vmaxi_h(src1, 0);
                src3 = __lsx_vmaxi_h(src3, 0);
                src1 = __lsx_vpickev_b(__lsx_vsat_hu(src3, 7), __lsx_vsat_hu(src1, 7));
                // Handle potential alpha == 0 values:
                __m128i srcVectorAlphaMask = __lsx_vseq_w(srcVectorAlpha, zero);
                src1 = __lsx_vandn_v(srcVectorAlphaMask, src1);
                // Fixup alpha values:
                if (RGBx)
                    srcVector = __lsx_vor_v(src1, alphaMask);
                else
                    srcVector = __lsx_vbitsel_v(src1, srcVector, __lsx_vslti_b(alphaMask, 0));
                __lsx_vst(srcVector, &buffer[i], 0);
            } else {
                if (RGBA)
                    __lsx_vst(__lsx_vshuf_b(zero, srcVector, rgbaMask), &buffer[i], 0);
                else if (buffer != src)
                    __lsx_vst(srcVector, &buffer[i], 0);
            }
        } else {
            if (RGBx)
                __lsx_vst(alphaMask, &buffer[i], 0);
            else
                __lsx_vst(zero, &buffer[i], 0);
        }
    }

    SIMD_EPILOGUE(i, count, 3) {
        uint v = qUnpremultiply_lsx(src[i]);
        if (RGBx)
            v = 0xff000000 | v;
        if (RGBA)
            v = ARGB2RGBA(v);
        buffer[i] = v;
    }
}

template<bool RGBA>
static inline void convertARGBFromRGBA64PM_lsx(uint *buffer, const QRgba64 *src, int count)
{
    int i = 0;
    const __m128i alphaMask = __lsx_vreplgr2vr_d(qint64(Q_UINT64_C(0xffff) << 48));
    const __m128i alphaMask32 = __lsx_vreplgr2vr_w(0xff000000);
    const __m128i rgbaMask = (__m128i)(v16i8){2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15};
    const __m128i zero = __lsx_vldi(0);

    for (; i < count - 3; i += 4) {
        __m128i srcVector1 = __lsx_vld(&src[i], 0);
        __m128i srcVector2 = __lsx_vld(&src[i + 2], 0);
        const v4i32 testz1 = (v4i32)__lsx_vmsknz_b(__lsx_vand_v(srcVector1, alphaMask));
        bool transparent1 = testz1[0]==0;
        const v4i32 testc1 = (v4i32)__lsx_vmsknz_b(__lsx_vandn_v(srcVector1, alphaMask));
        bool opaque1 = testc1[0]==0;
        const v4i32 testz2 = (v4i32)__lsx_vmsknz_b(__lsx_vand_v(srcVector2, alphaMask));
        bool transparent2 = testz2[0]==0;
        const v4i32 testc2 = (v4i32)__lsx_vmsknz_b(__lsx_vandn_v(srcVector2, alphaMask));
        bool opaque2 = testc2[0]==0;

        if (!(transparent1 && transparent2)) {
            if (!(opaque1 && opaque2)) {
                __m128i srcVector1Alpha = __lsx_vsrli_d(srcVector1, 48);
                __m128i srcVector2Alpha = __lsx_vsrli_d(srcVector2, 48);
                __m128i srcVectorAlpha = __lsx_vpickev_h(__lsx_vsat_wu(srcVector2Alpha, 15),
                                                         __lsx_vsat_wu(srcVector1Alpha, 15));
                const __m128 a = __lsx_vffint_s_w(srcVectorAlpha);
                // Convert srcVectorAlpha to final 8-bit alpha channel
                srcVectorAlpha = __lsx_vadd_w(srcVectorAlpha, __lsx_vreplgr2vr_w(128));
                srcVectorAlpha = __lsx_vsub_w(srcVectorAlpha, __lsx_vsrli_w(srcVectorAlpha, 8));
                srcVectorAlpha = __lsx_vsrli_w(srcVectorAlpha, 8);
                srcVectorAlpha = __lsx_vslli_w(srcVectorAlpha, 24);
                const __m128 ia = reciprocal_mul_ps(a, 255.0f);
                __m128i src1 = __lsx_vilvl_h(zero, srcVector1);
                __m128i src2 = __lsx_vilvh_h(zero, srcVector1);
                __m128i src3 = __lsx_vilvl_h(zero, srcVector2);
                __m128i src4 = __lsx_vilvh_h(zero, srcVector2);
                __m128 ia1 = (__m128)__lsx_vreplvei_w(ia, 0);
                __m128 ia2 = (__m128)__lsx_vreplvei_w(ia, 1);
                __m128 ia3 = (__m128)__lsx_vreplvei_w(ia, 2);
                __m128 ia4 = (__m128)__lsx_vreplvei_w(ia, 3);
                src1 = __lsx_vftintrne_w_s(__lsx_vfmul_s(__lsx_vffint_s_w(src1), ia1));
                src2 = __lsx_vftintrne_w_s(__lsx_vfmul_s(__lsx_vffint_s_w(src2), ia2));
                src3 = __lsx_vftintrne_w_s(__lsx_vfmul_s(__lsx_vffint_s_w(src3), ia3));
                src4 = __lsx_vftintrne_w_s(__lsx_vfmul_s(__lsx_vffint_s_w(src4), ia4));
                src1 = __lsx_vpickev_h(__lsx_vsat_wu(src2, 15), __lsx_vsat_wu(src1, 15));
                src3 = __lsx_vpickev_h(__lsx_vsat_wu(src4, 15), __lsx_vsat_wu(src3, 15));
                // Handle potential alpha == 0 values:
                __m128i srcVector1AlphaMask = __lsx_vseq_d(srcVector1Alpha, zero);
                __m128i srcVector2AlphaMask = __lsx_vseq_d(srcVector2Alpha, zero);
                src1 = __lsx_vandn_v(srcVector1AlphaMask, src1);
                src3 = __lsx_vandn_v(srcVector2AlphaMask, src3);
                src1 = __lsx_vmaxi_h(src1, 0);
                src3 = __lsx_vmaxi_h(src3, 0);
                src1 = __lsx_vpickev_b(__lsx_vsat_hu(src3, 7), __lsx_vsat_hu(src1, 7));
                // Fixup alpha values:
                src1 = __lsx_vbitsel_v(src1, srcVectorAlpha, __lsx_vslti_b(alphaMask32, 0));
                // Fix RGB order
                if (!RGBA){
                    src1 = __lsx_vshuf_b(zero, src1, rgbaMask);}
                __lsx_vst(src1, (__m128i *)&buffer[i], 0);
            } else {
                __m128i src1 = __lsx_vilvl_h(zero, srcVector1);
                __m128i src2 = __lsx_vilvh_h(zero, srcVector1);
                __m128i src3 = __lsx_vilvl_h(zero, srcVector2);
                __m128i src4 = __lsx_vilvh_h(zero, srcVector2);
                src1 = __lsx_vadd_w(src1, __lsx_vreplgr2vr_w(128));
                src2 = __lsx_vadd_w(src2, __lsx_vreplgr2vr_w(128));
                src3 = __lsx_vadd_w(src3, __lsx_vreplgr2vr_w(128));
                src4 = __lsx_vadd_w(src4, __lsx_vreplgr2vr_w(128));
                src1 = __lsx_vsub_w(src1, __lsx_vsrli_w(src1, 8));
                src2 = __lsx_vsub_w(src2, __lsx_vsrli_w(src2, 8));
                src3 = __lsx_vsub_w(src3, __lsx_vsrli_w(src3, 8));
                src4 = __lsx_vsub_w(src4, __lsx_vsrli_w(src4, 8));
                src1 = __lsx_vsrli_w(src1, 8);
                src2 = __lsx_vsrli_w(src2, 8);
                src3 = __lsx_vsrli_w(src3, 8);
                src4 = __lsx_vsrli_w(src4, 8);
                src1 = __lsx_vpickev_h(__lsx_vsat_wu(src2, 15), __lsx_vsat_wu(src1, 15));
                src3 = __lsx_vpickev_h(__lsx_vsat_wu(src4, 15), __lsx_vsat_wu(src3, 15));
                src1 = __lsx_vmaxi_h(src1, 0);
                src3 = __lsx_vmaxi_h(src3, 0);
                src1 = __lsx_vpickev_b(__lsx_vsat_hu(src3, 7), __lsx_vsat_hu(src1, 15));
                if (!RGBA){
                    src1 = __lsx_vshuf_b(zero, src1, rgbaMask);}
                __lsx_vst(src1, &buffer[i], 0);
            }
        } else {
            __lsx_vst(zero, &buffer[i], 0);
        }
    }

    SIMD_EPILOGUE(i, count, 3) {
        buffer[i] = qConvertRgba64ToRgb32_lsx<RGBA ? PixelOrderRGB : PixelOrderBGR>(src[i]);
    }
}

template<bool mask>
static inline void convertRGBA64FromRGBA64PM_lsx(QRgba64 *buffer, const QRgba64 *src, int count)
{
    int i = 0;
    const __m128i alphaMask = __lsx_vreplgr2vr_d(qint64(Q_UINT64_C(0xffff) << 48));
    const __m128i zero = __lsx_vldi(0);

    for (; i < count - 3; i += 4) {
        __m128i srcVector1 = __lsx_vld(&src[i + 0], 0);
        __m128i srcVector2 = __lsx_vld(&src[i + 2], 0);
        const v4i32 testz1 = (v4i32)__lsx_vmsknz_b(__lsx_vand_v(srcVector1, alphaMask));
        bool transparent1 = testz1[0]==0;
        const v4i32 testc1 = (v4i32)__lsx_vmsknz_b(__lsx_vandn_v(srcVector1, alphaMask));
        bool opaque1 = testc1[0]==0;
        const v4i32 testz2 = (v4i32)__lsx_vmsknz_b(__lsx_vand_v(srcVector2, alphaMask));
        bool transparent2 = testz2[0]==0;
        const v4i32 testc2 = (v4i32)__lsx_vmsknz_b(__lsx_vandn_v(srcVector2, alphaMask));
        bool opaque2 = testc2[0]==0;

        if (!(transparent1 && transparent2)) {
            if (!(opaque1 && opaque2)) {
                __m128i srcVector1Alpha = __lsx_vsrli_d(srcVector1, 48);
                __m128i srcVector2Alpha = __lsx_vsrli_d(srcVector2, 48);
                __m128i srcVectorAlpha = __lsx_vpickev_h(__lsx_vsat_wu(srcVector2Alpha, 15),
                                                         __lsx_vsat_wu(srcVector1Alpha, 15));
                const __m128 a = __lsx_vffint_s_w(srcVectorAlpha);
                const __m128 ia = reciprocal_mul_ps(a, 65535.0f);
                __m128i src1 = __lsx_vilvl_h(zero, srcVector1);
                __m128i src2 = __lsx_vilvh_h(zero, srcVector1);
                __m128i src3 = __lsx_vilvl_h(zero, srcVector2);
                __m128i src4 = __lsx_vilvh_h(zero, srcVector2);
                __m128 ia1 = (__m128)__lsx_vreplvei_w(ia, 0);
                __m128 ia2 = (__m128)__lsx_vreplvei_w(ia, 1);
                __m128 ia3 = (__m128)__lsx_vreplvei_w(ia, 2);
                __m128 ia4 = (__m128)__lsx_vreplvei_w(ia, 3);
                src1 = __lsx_vftintrne_w_s(__lsx_vfmul_s(__lsx_vffint_s_w(src1), ia1));
                src2 = __lsx_vftintrne_w_s(__lsx_vfmul_s(__lsx_vffint_s_w(src2), ia2));
                src3 = __lsx_vftintrne_w_s(__lsx_vfmul_s(__lsx_vffint_s_w(src3), ia3));
                src4 = __lsx_vftintrne_w_s(__lsx_vfmul_s(__lsx_vffint_s_w(src4), ia4));
                src1 = __lsx_vpickev_h(__lsx_vsat_wu(src2, 15), __lsx_vsat_wu(src1, 15));
                src3 = __lsx_vpickev_h(__lsx_vsat_wu(src4, 15), __lsx_vsat_wu(src3, 15));
                // Handle potential alpha == 0 values:
                __m128i srcVector1AlphaMask = __lsx_vseq_d(srcVector1Alpha, zero);
                __m128i srcVector2AlphaMask = __lsx_vseq_d(srcVector2Alpha, zero);
                src1 = __lsx_vandn_v(srcVector1AlphaMask, src1);
                src3 = __lsx_vandn_v(srcVector2AlphaMask, src3);
                // Fixup alpha values:
                if (mask) {
                    src1 = __lsx_vor_v(src1, alphaMask);
                    src3 = __lsx_vor_v(src3, alphaMask);
                } else {
                    src1 = __lsx_vbitsel_v(src1, srcVector1, __lsx_vslti_b(alphaMask, 0));
                    src3 = __lsx_vbitsel_v(src3, srcVector2, __lsx_vslti_b(alphaMask, 0));
                }
                __lsx_vst(src1, &buffer[i + 0], 0);
                __lsx_vst(src3, &buffer[i + 2], 0);
            } else {
                if (mask) {
                    srcVector1 = __lsx_vor_v(srcVector1, alphaMask);
                    srcVector2 = __lsx_vor_v(srcVector2, alphaMask);
                }
                if (mask || src != buffer) {
                    __lsx_vst(srcVector1, &buffer[i + 0], 0);
                    __lsx_vst(srcVector2, &buffer[i + 2], 0);
                }
            }
        } else {
            __lsx_vst(zero, &buffer[i + 0], 0);
            __lsx_vst(zero, &buffer[i + 2], 0);
        }
    }

    SIMD_EPILOGUE(i, count, 3) {
        QRgba64 v = src[i].unpremultiplied();
        if (mask)
            v.setAlpha(65535);
        buffer[i] = v;
    }
}

void QT_FASTCALL convertARGB32ToARGB32PM_lsx(uint *buffer, int count, const QList<QRgb> *)
{
    convertARGBToARGB32PM_lsx<false>(buffer, buffer, count);
}

void QT_FASTCALL convertRGBA8888ToARGB32PM_lsx(uint *buffer, int count, const QList<QRgb> *)
{
    convertARGBToARGB32PM_lsx<true>(buffer, buffer, count);
}

const QRgba64 * QT_FASTCALL convertARGB32ToRGBA64PM_lsx(QRgba64 *buffer, const uint *src, int count,
                                                        const QList<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_lsx<false>(buffer, src, count);
    return buffer;
}

const QRgba64 * QT_FASTCALL convertRGBA8888ToRGBA64PM_lsx(QRgba64 *buffer, const uint *src, int count,
                                                          const QList<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_lsx<true>(buffer, src, count);
    return buffer;
}

const uint *QT_FASTCALL fetchARGB32ToARGB32PM_lsx(uint *buffer, const uchar *src, int index, int count,
                                                  const QList<QRgb> *, QDitherInfo *)
{
    convertARGBToARGB32PM_lsx<false>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}

const uint *QT_FASTCALL fetchRGBA8888ToARGB32PM_lsx(uint *buffer, const uchar *src, int index, int count,
                                                    const QList<QRgb> *, QDitherInfo *)
{
    convertARGBToARGB32PM_lsx<true>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}

const QRgba64 *QT_FASTCALL fetchARGB32ToRGBA64PM_lsx(QRgba64 *buffer, const uchar *src, int index, int count,
                                                     const QList<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_lsx<false>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}

const QRgba64 *QT_FASTCALL fetchRGBA8888ToRGBA64PM_lsx(QRgba64 *buffer, const uchar *src, int index, int count,
                                                        const QList<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_lsx<true>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}

void QT_FASTCALL storeRGB32FromARGB32PM_lsx(uchar *dest, const uint *src, int index, int count,
                                            const QList<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    convertARGBFromARGB32PM_lsx<false,true>(d, src, count);
}

void QT_FASTCALL storeARGB32FromARGB32PM_lsx(uchar *dest, const uint *src, int index, int count,
                                             const QList<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    convertARGBFromARGB32PM_lsx<false,false>(d, src, count);
}

void QT_FASTCALL storeRGBA8888FromARGB32PM_lsx(uchar *dest, const uint *src, int index, int count,
                                               const QList<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    convertARGBFromARGB32PM_lsx<true,false>(d, src, count);
}

void QT_FASTCALL storeRGBXFromARGB32PM_lsx(uchar *dest, const uint *src, int index, int count,
                                           const QList<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    convertARGBFromARGB32PM_lsx<true,true>(d, src, count);
}

template<QtPixelOrder PixelOrder>
void QT_FASTCALL storeA2RGB30PMFromARGB32PM_lsx(uchar *dest, const uint *src, int index, int count,
                                                const QList<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    for (int i = 0; i < count; ++i)
        d[i] = qConvertArgb32ToA2rgb30_lsx<PixelOrder>(src[i]);
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL destStore64ARGB32_lsx(QRasterBuffer *rasterBuffer, int x,
                                       int y, const QRgba64 *buffer, int length)
{
    uint *dest = (uint*)rasterBuffer->scanLine(y) + x;
    convertARGBFromRGBA64PM_lsx<false>(dest, buffer, length);
}

void QT_FASTCALL destStore64RGBA8888_lsx(QRasterBuffer *rasterBuffer, int x,
                                         int y, const QRgba64 *buffer, int length)
{
    uint *dest = (uint*)rasterBuffer->scanLine(y) + x;
    convertARGBFromRGBA64PM_lsx<true>(dest, buffer, length);
}
#endif

void QT_FASTCALL storeARGB32FromRGBA64PM_lsx(uchar *dest, const QRgba64 *src, int index, int count,
                                             const QList<QRgb> *, QDitherInfo *)
{
    uint *d = (uint*)dest + index;
    convertARGBFromRGBA64PM_lsx<false>(d, src, count);
}

void QT_FASTCALL storeRGBA8888FromRGBA64PM_lsx(uchar *dest, const QRgba64 *src, int index, int count,
                                               const QList<QRgb> *, QDitherInfo *)
{
    uint *d = (uint*)dest + index;
    convertARGBFromRGBA64PM_lsx<true>(d, src, count);
}

template
void QT_FASTCALL storeA2RGB30PMFromARGB32PM_lsx<PixelOrderBGR>(uchar *dest, const uint *src, int index, int count,
                                                               const QList<QRgb> *, QDitherInfo *);
template
void QT_FASTCALL storeA2RGB30PMFromARGB32PM_lsx<PixelOrderRGB>(uchar *dest, const uint *src, int index, int count,
                                                               const QList<QRgb> *, QDitherInfo *);

void QT_FASTCALL storeRGBA64FromRGBA64PM_lsx(uchar *dest, const QRgba64 *src, int index, int count,
                                             const QList<QRgb> *, QDitherInfo *)
{
    QRgba64 *d = (QRgba64 *)dest + index;
    convertRGBA64FromRGBA64PM_lsx<false>(d, src, count);
}

void QT_FASTCALL storeRGBx64FromRGBA64PM_lsx(uchar *dest, const QRgba64 *src, int index, int count,
                                             const QList<QRgb> *, QDitherInfo *)
{
    QRgba64 *d = (QRgba64 *)dest + index;
    convertRGBA64FromRGBA64PM_lsx<true>(d, src, count);
}

#if QT_CONFIG(raster_fp)
const QRgbaFloat32 *QT_FASTCALL fetchRGBA32FToRGBA32F_lsx(QRgbaFloat32 *buffer, const uchar *src,
                                                          int index, int count,
                                                          const QList<QRgb> *, QDitherInfo *)
{
    const QRgbaFloat32 *s = reinterpret_cast<const QRgbaFloat32 *>(src) + index;
    for (int i = 0; i < count; ++i) {
        __m128 vsf = (__m128)__lsx_vld(reinterpret_cast<const float *>(s + i), 0);
        __m128 vsa = (__m128)__lsx_vreplvei_w(vsf, 3);
        vsf = __lsx_vfmul_s(vsf, vsa);
        vsf = (__m128)__lsx_vextrins_w(vsf, vsa, 0x30);
        __lsx_vst(vsf, reinterpret_cast<float *>(buffer + i), 0);
    }
    return buffer;
}

void QT_FASTCALL storeRGBX32FFromRGBA32F_lsx(uchar *dest, const QRgbaFloat32 *src,
                                             int index, int count,
                                             const QList<QRgb> *, QDitherInfo *)
{
    QRgbaFloat32 *d = reinterpret_cast<QRgbaFloat32 *>(dest) + index;
    const __m128 zero = (__m128)(v4f32){0.0f, 0.0f, 0.0f, 1.0f};
    for (int i = 0; i < count; ++i) {
        __m128 vsf = (__m128)__lsx_vld(reinterpret_cast<const float *>(src + i), 0);
        const __m128 vsa = (__m128)__lsx_vreplvei_w(vsf, 3);
        FloatInt a;
        a.i = __lsx_vpickve2gr_w(vsa, 0);
        if (a.f == 1.0f)
        { }
        else if (a.f == 0.0f)
            vsf = zero;
        else {
            __m128 vsr = __lsx_vfrecip_s(vsa);
            vsr = __lsx_vfsub_s(__lsx_vfadd_s(vsr, vsr),
                                __lsx_vfmul_s(vsr, __lsx_vfmul_s(vsr, vsa)));
            vsf = __lsx_vfmul_s(vsf, vsr);
            FloatInt b = {.f = 1.0f};
            vsf = (__m128)__lsx_vinsgr2vr_w(vsf, b.i, 3);
        }
        __lsx_vst(vsf, reinterpret_cast<float *>(d + i), 0);
    }
}

void QT_FASTCALL storeRGBA32FFromRGBA32F_lsx(uchar *dest, const QRgbaFloat32 *src,
                                             int index, int count,
                                             const QList<QRgb> *, QDitherInfo *)
{
    QRgbaFloat32 *d = reinterpret_cast<QRgbaFloat32 *>(dest) + index;
    const __m128 zero = (__m128)__lsx_vldi(0);
    for (int i = 0; i < count; ++i) {
        __m128 vsf = (__m128)__lsx_vld(reinterpret_cast<const float *>(src + i), 0);
        const __m128 vsa = (__m128)__lsx_vreplvei_w(vsf, 3);
        FloatInt a;
        a.i = __lsx_vpickve2gr_w(vsa, 0);
        if (a.f == 1.0f)
        { }
        else if (a.f == 0.0f)
            vsf = zero;
        else {
            __m128 vsr = __lsx_vfrecip_s(vsa);
            vsr = __lsx_vfsub_s(__lsx_vfadd_s(vsr, vsr),
                                __lsx_vfmul_s(vsr, __lsx_vfmul_s(vsr, vsa)));
            FloatInt b = {.f = 1.0f};
            vsr = (__m128)__lsx_vinsgr2vr_w(vsr, b.i, 3);
            vsf = __lsx_vfmul_s(vsf, vsr);
        }
        __lsx_vst(vsf, reinterpret_cast<float *>(d + i), 0);
    }
}
#endif

QT_END_NAMESPACE

#endif // QT_COMPILER_SUPPORTS_LSX
