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

QT_END_NAMESPACE

#endif // QT_COMPILER_SUPPORTS_LSX
