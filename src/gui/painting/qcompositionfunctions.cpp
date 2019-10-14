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

#include <qglobal.h>
#include "qdrawhelper_p.h"
#include "qrgba64_p.h"

QT_BEGIN_NAMESPACE

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

struct Argb32OperationsC
{
    typedef QRgb Type;
    typedef quint8 Scalar;
    typedef QRgb OptimalType;
    typedef quint8 OptimalScalar;

    static const Type clear;
    static bool isOpaque(Type val)
    { return qAlpha(val) == 255; }
    static bool isTransparent(Type val)
    { return qAlpha(val) == 0; }
    static Scalar scalarFrom8bit(uint8_t a)
    { return a; }
    static void memfill(Type *ptr, Type value, qsizetype len)
    { qt_memfill32(ptr, value, len); }
    static void memcpy(Type *Q_DECL_RESTRICT dest, const Type *Q_DECL_RESTRICT src, qsizetype len)
    { ::memcpy(dest, src, len * sizeof(Type)); }

    static OptimalType load(const Type *ptr)
    { return *ptr; }
    static OptimalType convert(const Type &val)
    { return val; }
    static void store(Type *ptr, OptimalType value)
    { *ptr = value; }
    static OptimalType add(OptimalType a, OptimalType b)
    { return a + b; }
    static OptimalScalar add(OptimalScalar a, OptimalScalar b)
    { return a + b; }
    static OptimalType plus(OptimalType a, OptimalType b)
    { return comp_func_Plus_one_pixel(a, b); }
    static OptimalScalar alpha(OptimalType val)
    { return qAlpha(val); }
    static OptimalScalar invAlpha(OptimalScalar c)
    { return 255 - c; }
    static OptimalScalar invAlpha(OptimalType val)
    { return alpha(~val); }
    static OptimalScalar scalar(Scalar v)
    { return v; }
    static OptimalType multiplyAlpha8bit(OptimalType val, uint8_t a)
    { return BYTE_MUL(val, a); }
    static OptimalType interpolate8bit(OptimalType x, uint8_t a1, OptimalType y, uint8_t a2)
    { return INTERPOLATE_PIXEL_255(x, a1, y, a2); }
    static OptimalType multiplyAlpha(OptimalType val, OptimalScalar a)
    { return BYTE_MUL(val, a); }
    static OptimalScalar multiplyAlpha8bit(OptimalScalar val, uint8_t a)
    { return qt_div_255(val * a); }
    static OptimalType interpolate(OptimalType x, OptimalScalar a1, OptimalType y, OptimalScalar a2)
    { return INTERPOLATE_PIXEL_255(x, a1, y, a2); }
};

const Argb32OperationsC::Type Argb32OperationsC::clear = 0;

struct Rgba64OperationsBase
{
    typedef QRgba64 Type;
    typedef quint16 Scalar;

    static const Type clear;

    static bool isOpaque(Type val)
    { return val.isOpaque(); }
    static bool isTransparent(Type val)
    { return val.isTransparent(); }
    static Scalar scalarFrom8bit(uint8_t a)
    { return a * 257; }

    static void memfill(Type *ptr, Type value, qsizetype len)
    { qt_memfill64((quint64*)ptr, value, len); }
    static void memcpy(Type *Q_DECL_RESTRICT dest, const Type *Q_DECL_RESTRICT src, qsizetype len)
    { ::memcpy(dest, src, len * sizeof(Type)); }
};

#if QT_CONFIG(raster_64bit)
const Rgba64OperationsBase::Type Rgba64OperationsBase::clear = QRgba64::fromRgba64(0);

struct Rgba64OperationsC : public Rgba64OperationsBase
{
    typedef QRgba64 OptimalType;
    typedef quint16 OptimalScalar;

    static OptimalType load(const Type *ptr)
    { return *ptr; }
    static OptimalType convert(const Type &val)
    { return val; }
    static void store(Type *ptr, OptimalType value)
    { *ptr = value; }
    static OptimalType add(OptimalType a, OptimalType b)
    { return QRgba64::fromRgba64((quint64)a + (quint64)b); }
    static OptimalScalar add(OptimalScalar a, OptimalScalar b)
    { return a + b; }
    static OptimalType plus(OptimalType a, OptimalType b)
    { return addWithSaturation(a, b); }
    static OptimalScalar alpha(OptimalType val)
    { return val.alpha(); }
    static OptimalScalar invAlpha(Scalar c)
    { return 65535 - c; }
    static OptimalScalar invAlpha(OptimalType val)
    { return 65535 - alpha(val); }
    static OptimalScalar scalar(Scalar v)
    { return v; }
    static OptimalType multiplyAlpha8bit(OptimalType val, uint8_t a)
    { return multiplyAlpha255(val, a); }
    static OptimalScalar multiplyAlpha8bit(OptimalScalar val, uint8_t a)
    { return qt_div_255(val * a); }
    static OptimalType interpolate8bit(OptimalType x, uint8_t a1, OptimalType y, uint8_t a2)
    { return interpolate255(x, a1, y, a2); }
    static OptimalType multiplyAlpha(OptimalType val, OptimalScalar a)
    { return multiplyAlpha65535(val, a); }
    static OptimalType interpolate(OptimalType x, OptimalScalar a1, OptimalType y, OptimalScalar a2)
    { return interpolate65535(x, a1, y, a2); }
};

#if defined(__SSE2__)
struct Rgba64OperationsSSE2 : public Rgba64OperationsBase
{
    typedef __m128i OptimalType;
    typedef __m128i OptimalScalar;

    static OptimalType load(const Type *ptr)
    {
        return _mm_loadl_epi64(reinterpret_cast<const __m128i *>(ptr));
    }
    static OptimalType convert(const Type &value)
    {
#ifdef Q_PROCESSOR_X86_64
        return _mm_cvtsi64_si128(value);
#else
        return load(&value);
#endif
    }
    static void store(Type *ptr, OptimalType value)
    {
        _mm_storel_epi64(reinterpret_cast<__m128i *>(ptr), value);
    }
    static OptimalType add(OptimalType a, OptimalType b)
    {
        return _mm_add_epi16(a, b);
    }
//    same as above:
//    static OptimalScalar add(OptimalScalar a, OptimalScalar b)
    static OptimalType plus(OptimalType a, OptimalType b)
    {
        return _mm_adds_epu16(a, b);
    }
    static OptimalScalar alpha(OptimalType c)
    {
        return _mm_shufflelo_epi16(c, _MM_SHUFFLE(3, 3, 3, 3));
    }
    static OptimalScalar invAlpha(Scalar c)
    {
        return scalar(65535 - c);
    }
    static OptimalScalar invAlpha(OptimalType c)
    {
        return _mm_xor_si128(_mm_set1_epi16(-1), alpha(c));
    }
    static OptimalScalar scalar(Scalar n)
    {
        return _mm_shufflelo_epi16(_mm_cvtsi32_si128(n), _MM_SHUFFLE(0, 0, 0, 0));
    }
    static OptimalType multiplyAlpha8bit(OptimalType val, uint8_t a)
    {
        return multiplyAlpha255(val, a);
    }
//    same as above:
//    static OptimalScalar multiplyAlpha8bit(OptimalScalar a, uint8_t a)
    static OptimalType interpolate8bit(OptimalType x, uint8_t a1, OptimalType y, uint8_t a2)
    {
        return interpolate255(x, a1, y, a2);
    }
    static OptimalType multiplyAlpha(OptimalType val, OptimalScalar a)
    {
        return multiplyAlpha65535(val, a);
    }
    // a2 is const-ref because otherwise MSVC2015@x86 complains that it can't 16-byte align the argument.
    static OptimalType interpolate(OptimalType x, OptimalScalar a1, OptimalType y, const OptimalScalar &a2)
    {
        return interpolate65535(x, a1, y, a2);
    }
};
#endif

#if defined(__ARM_NEON__)
struct Rgba64OperationsNEON : public Rgba64OperationsBase
{
    typedef uint16x4_t OptimalType;
    typedef uint16x4_t OptimalScalar;

    static OptimalType load(const Type *ptr)
    {
        return vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(ptr)));
    }
    static OptimalType convert(const Type &val)
    {
        return vreinterpret_u16_u64(vmov_n_u64(val));
    }
    static void store(Type *ptr, OptimalType value)
    {
        vst1_u64(reinterpret_cast<uint64_t *>(ptr), vreinterpret_u64_u16(value));
    }
    static OptimalType add(OptimalType a, OptimalType b)
    {
        return vadd_u16(a, b);
    }
//    same as above:
//    static OptimalScalar add(OptimalScalar a, OptimalScalar b)
    static OptimalType plus(OptimalType a, OptimalType b)
    {
        return vqadd_u16(a, b);
    }
    static OptimalScalar alpha(OptimalType c)
    {
        return vdup_lane_u16(c, 3);
    }
    static OptimalScalar invAlpha(Scalar c)
    {
        return scalar(65535 - c);
    }
    static OptimalScalar invAlpha(OptimalType c)
    {
        return vmvn_u16(alpha(c));
    }
    static OptimalScalar scalar(Scalar n)
    {
        return vdup_n_u16(n);
    }
    static OptimalType multiplyAlpha8bit(OptimalType val, uint8_t a)
    {
        return multiplyAlpha255(val, a);
    }
//    same as above:
//    static OptimalScalar multiplyAlpha8bit(OptimalScalar a, uint8_t a)
    static OptimalType interpolate8bit(OptimalType x, uint8_t a1, OptimalType y, uint8_t a2)
    {
        return interpolate255(x, a1, y, a2);
    }
    static OptimalType multiplyAlpha(OptimalType val, OptimalScalar a)
    {
        return multiplyAlpha65535(val, a);
    }
    static OptimalType interpolate(OptimalType x, OptimalScalar a1, OptimalType y, OptimalScalar a2)
    {
        return interpolate65535(x, a1, y, a2);
    }
};
#endif

#if defined(__SSE2__)
typedef Rgba64OperationsSSE2 Rgba64Operations;
#elif defined(__ARM_NEON__)
typedef Rgba64OperationsNEON Rgba64Operations;
#else
typedef Rgba64OperationsC Rgba64Operations;
#endif
#endif // QT_CONFIG(raster_64bit)

typedef Argb32OperationsC Argb32Operations;

/*
  result = 0
  d = d * cia
*/
template<class Ops>
inline static void comp_func_Clear_template(typename Ops::Type *dest, int length, uint const_alpha)
{
    if (const_alpha == 255)
        Ops::memfill(dest, Ops::clear, length);
    else {
        uint ialpha = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            Ops::store(&dest[i], Ops::multiplyAlpha8bit(Ops::load(&dest[i]), ialpha));
        }
    }
}

void QT_FASTCALL comp_func_solid_Clear(uint *dest, int length, uint, uint const_alpha)
{
    comp_func_Clear_template<Argb32Operations>(dest, length, const_alpha);
}

void QT_FASTCALL comp_func_Clear(uint *dest, const uint *, int length, uint const_alpha)
{
    comp_func_Clear_template<Argb32Operations>(dest, length, const_alpha);
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_Clear_rgb64(QRgba64 *dest, int length, QRgba64, uint const_alpha)
{
    comp_func_Clear_template<Rgba64Operations>(dest, length, const_alpha);
}

void QT_FASTCALL comp_func_Clear_rgb64(QRgba64 *dest, const QRgba64 *, int length, uint const_alpha)
{
    comp_func_Clear_template<Rgba64Operations>(dest, length, const_alpha);
}
#endif


/*
  result = s
  dest = s * ca + d * cia
*/
template<class Ops>
inline static void comp_func_solid_Source_template(typename Ops::Type *dest, int length, typename Ops::Type color, uint const_alpha)
{
    if (const_alpha == 255)
        Ops::memfill(dest, color, length);
    else {
        const uint ialpha = 255 - const_alpha;
        auto s = Ops::multiplyAlpha8bit(Ops::convert(color), const_alpha);
        for (int i = 0; i < length; ++i) {
            auto d = Ops::multiplyAlpha8bit(Ops::load(&dest[i]), ialpha);
            Ops::store(&dest[i], Ops::add(s, d));
        }
    }
}

template<class Ops>
inline static void comp_func_Source_template(typename Ops::Type *Q_DECL_RESTRICT dest,
                                             const typename Ops::Type *Q_DECL_RESTRICT src,
                                             int length, uint const_alpha)
{
    if (const_alpha == 255)
        Ops::memcpy(dest, src, length);
    else {
        const uint ialpha = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            auto s = Ops::load(src + i);
            auto d = Ops::load(dest + i);
            Ops::store(&dest[i], Ops::interpolate8bit(s, const_alpha, d, ialpha));
        }
    }
}

void QT_FASTCALL comp_func_solid_Source(uint *dest, int length, uint color, uint const_alpha)
{
    comp_func_solid_Source_template<Argb32Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_Source(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_Source_template<Argb32Operations>(dest, src, length, const_alpha);
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_Source_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    comp_func_solid_Source_template<Rgba64Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_Source_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_Source_template<Rgba64Operations>(dest, src, length, const_alpha);
}
#endif

void QT_FASTCALL comp_func_solid_Destination(uint *, int, uint, uint)
{
}

void QT_FASTCALL comp_func_Destination(uint *, const uint *, int, uint)
{
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_Destination_rgb64(QRgba64 *, int, QRgba64, uint)
{
}

void QT_FASTCALL comp_func_Destination_rgb64(QRgba64 *, const QRgba64 *, int, uint)
{
}
#endif

/*
  result = s + d * sia
  dest = (s + d * sia) * ca + d * cia
       = s * ca + d * (sia * ca + cia)
       = s * ca + d * (1 - sa*ca)
*/
template<class Ops>
inline static void comp_func_solid_SourceOver_template(typename Ops::Type *dest, int length, typename Ops::Type color, uint const_alpha)
{
    if (const_alpha == 255 && Ops::isOpaque(color))
        Ops::memfill(dest, color, length);
    else {
        auto c = Ops::convert(color);
        if (const_alpha != 255)
            c = Ops::multiplyAlpha8bit(c, const_alpha);
        auto cAlpha = Ops::invAlpha(c);
        for (int i = 0; i < length; ++i) {
            auto d = Ops::multiplyAlpha(Ops::load(&dest[i]), cAlpha);
            Ops::store(&dest[i], Ops::add(c, d));
        }
    }
}

template<class Ops>
inline static void comp_func_SourceOver_template(typename Ops::Type *Q_DECL_RESTRICT dest,
                                                 const typename Ops::Type *Q_DECL_RESTRICT src,
                                                 int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            auto c = src[i];
            if (Ops::isOpaque(c))
                Ops::store(&dest[i], Ops::convert(c));
            else if (!Ops::isTransparent(c)) {
                auto s = Ops::convert(c);
                auto d = Ops::multiplyAlpha(Ops::load(&dest[i]), Ops::invAlpha(s));
                Ops::store(&dest[i], Ops::add(s, d));
            }
        }
    } else {
        for (int i = 0; i < length; ++i) {
            auto s = Ops::multiplyAlpha8bit(Ops::load(&src[i]), const_alpha);
            auto d = Ops::multiplyAlpha(Ops::load(&dest[i]), Ops::invAlpha(s));
            Ops::store(&dest[i], Ops::add(s, d));
        }
    }
}

void QT_FASTCALL comp_func_solid_SourceOver(uint *dest, int length, uint color, uint const_alpha)
{
    comp_func_solid_SourceOver_template<Argb32Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_SourceOver(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_SourceOver_template<Argb32Operations>(dest, src, length, const_alpha);
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_SourceOver_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    comp_func_solid_SourceOver_template<Rgba64Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_SourceOver_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_SourceOver_template<Rgba64Operations>(dest, src, length, const_alpha);
}
#endif

/*
  result = d + s * dia
  dest = (d + s * dia) * ca + d * cia
       = d + s * dia * ca
*/
template<class Ops>
inline static void comp_func_solid_DestinationOver_template(typename Ops::Type *dest, int length, typename Ops::Type color, uint const_alpha)
{
    auto c = Ops::convert(color);
    if (const_alpha != 255)
        c = Ops::multiplyAlpha8bit(c, const_alpha);
    for (int i = 0; i < length; ++i) {
        auto d = Ops::load(&dest[i]);
        auto s = Ops::multiplyAlpha(c, Ops::invAlpha(d));
        Ops::store(&dest[i], Ops::add(s, d));
    }
}

template<class Ops>
inline static void comp_func_DestinationOver_template(typename Ops::Type *Q_DECL_RESTRICT dest,
                                                      const typename Ops::Type *Q_DECL_RESTRICT src,
                                                      int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            auto d = Ops::load(&dest[i]);
            auto s = Ops::multiplyAlpha(Ops::load(&src[i]), Ops::invAlpha(d));
            Ops::store(&dest[i], Ops::add(s, d));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            auto d = Ops::load(&dest[i]);
            auto s = Ops::multiplyAlpha8bit(Ops::load(&src[i]), const_alpha);
            s = Ops::multiplyAlpha(s, Ops::invAlpha(d));
            Ops::store(&dest[i], Ops::add(s, d));
        }
    }
}

void QT_FASTCALL comp_func_solid_DestinationOver(uint *dest, int length, uint color, uint const_alpha)
{
    comp_func_solid_DestinationOver_template<Argb32Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_DestinationOver(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_DestinationOver_template<Argb32Operations>(dest, src, length, const_alpha);
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_DestinationOver_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    comp_func_solid_DestinationOver_template<Rgba64Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_DestinationOver_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_DestinationOver_template<Rgba64Operations>(dest, src, length, const_alpha);
}
#endif

/*
  result = s * da
  dest = s * da * ca + d * cia
*/
template<class Ops>
inline static void comp_func_solid_SourceIn_template(typename Ops::Type *dest, int length, typename Ops::Type color, uint const_alpha)
{
    if (const_alpha == 255) {
        auto c = Ops::convert(color);
        for (int i = 0; i < length; ++i) {
            Ops::store(&dest[i], Ops::multiplyAlpha(c, Ops::alpha(Ops::load(&dest[i]))));
        }
    } else {
        auto c = Ops::multiplyAlpha8bit(Ops::convert(color), const_alpha);
        auto cia = Ops::invAlpha(Ops::scalarFrom8bit(const_alpha));
        for (int i = 0; i < length; ++i) {
            auto d = Ops::load(&dest[i]);
            Ops::store(&dest[i], Ops::interpolate(c, Ops::alpha(d), d, cia));
        }
    }
}

template<class Ops>
inline static void comp_func_SourceIn_template(typename Ops::Type *Q_DECL_RESTRICT dest,
                                               const typename Ops::Type *Q_DECL_RESTRICT src,
                                               int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            auto s = Ops::load(&src[i]);
            Ops::store(&dest[i], Ops::multiplyAlpha(s, Ops::alpha(Ops::load(&dest[i]))));
        }
    } else {
        auto ca = Ops::scalarFrom8bit(const_alpha);
        auto cia = Ops::invAlpha(ca);
        auto cav = Ops::scalar(ca);
        for (int i = 0; i < length; ++i) {
            auto d = Ops::load(&dest[i]);
            auto s = Ops::multiplyAlpha(Ops::load(&src[i]), cav);
            Ops::store(&dest[i], Ops::interpolate(s, Ops::alpha(d), d, cia));
        }
    }
}

void QT_FASTCALL comp_func_solid_SourceIn(uint *dest, int length, uint color, uint const_alpha)
{
    comp_func_solid_SourceIn_template<Argb32Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_SourceIn(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_SourceIn_template<Argb32Operations>(dest, src, length, const_alpha);
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_SourceIn_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    comp_func_solid_SourceIn_template<Rgba64Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_SourceIn_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_SourceIn_template<Rgba64Operations>(dest, src, length, const_alpha);
}
#endif

/*
  result = d * sa
  dest = d * sa * ca + d * cia
       = d * (sa * ca + cia)
*/
template<class Ops>
inline static void comp_func_solid_DestinationIn_template(typename Ops::Type *dest, int length, typename Ops::Type color, uint const_alpha)
{
    auto sa = Ops::alpha(Ops::convert(color));
    if (const_alpha != 255) {
        sa = Ops::multiplyAlpha8bit(sa, const_alpha);
        sa = Ops::add(sa, Ops::invAlpha(Ops::scalarFrom8bit(const_alpha)));
    }

    for (int i = 0; i < length; ++i) {
        Ops::store(&dest[i], Ops::multiplyAlpha(Ops::load(&dest[i]), sa));
    }
}

template<class Ops>
inline static void comp_func_DestinationIn_template(typename Ops::Type *Q_DECL_RESTRICT dest,
                                                    const typename Ops::Type *Q_DECL_RESTRICT src,
                                                    int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            auto a = Ops::alpha(Ops::load(&src[i]));
            Ops::store(&dest[i], Ops::multiplyAlpha(Ops::load(&dest[i]), a));
        }
    } else {
        auto cia = Ops::invAlpha(Ops::scalarFrom8bit(const_alpha));
        for (int i = 0; i < length; ++i) {
            auto sa = Ops::multiplyAlpha8bit(Ops::alpha(Ops::load(&src[i])), const_alpha);
            sa = Ops::add(sa, cia);
            Ops::store(&dest[i], Ops::multiplyAlpha(Ops::load(&dest[i]), sa));
        }
    }
}

void QT_FASTCALL comp_func_solid_DestinationIn(uint *dest, int length, uint color, uint const_alpha)
{
    comp_func_solid_DestinationIn_template<Argb32Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_DestinationIn(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_DestinationIn_template<Argb32Operations>(dest, src, length, const_alpha);
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_DestinationIn_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    comp_func_solid_DestinationIn_template<Rgba64Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_DestinationIn_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_DestinationIn_template<Rgba64Operations>(dest, src, length, const_alpha);
}
#endif

/*
  result = s * dia
  dest = s * dia * ca + d * cia
*/
template<class Ops>
inline static void comp_func_solid_SourceOut_template(typename Ops::Type *dest, int length, typename Ops::Type color, uint const_alpha)
{
    auto c = Ops::convert(color);
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i)
            Ops::store(&dest[i], Ops::multiplyAlpha(c, Ops::invAlpha(Ops::load(&dest[i]))));
    } else {
        auto cia = Ops::invAlpha(Ops::scalarFrom8bit(const_alpha));
        c = Ops::multiplyAlpha8bit(c, const_alpha);
        for (int i = 0; i < length; ++i) {
            auto d = Ops::load(&dest[i]);
            Ops::store(&dest[i], Ops::interpolate(c, Ops::invAlpha(d), d, cia));
        }
    }
}

template<class Ops>
inline static void comp_func_SourceOut_template(typename Ops::Type *Q_DECL_RESTRICT dest,
                                                const typename Ops::Type *Q_DECL_RESTRICT src,
                                                int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            auto s = Ops::load(&src[i]);
            auto d = Ops::load(&dest[i]);
            Ops::store(&dest[i], Ops::multiplyAlpha(s, Ops::invAlpha(d)));
        }
    } else {
        auto cia = Ops::invAlpha(Ops::scalarFrom8bit(const_alpha));
        for (int i = 0; i < length; ++i) {
            auto s = Ops::multiplyAlpha8bit(Ops::load(&src[i]), const_alpha);
            auto d = Ops::load(&dest[i]);
            Ops::store(&dest[i], Ops::interpolate(s, Ops::invAlpha(d), d, cia));
        }
    }
}

void QT_FASTCALL comp_func_solid_SourceOut(uint *dest, int length, uint color, uint const_alpha)
{
    comp_func_solid_SourceOut_template<Argb32Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_SourceOut(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_SourceOut_template<Argb32Operations>(dest, src, length, const_alpha);
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_SourceOut_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    comp_func_solid_SourceOut_template<Rgba64Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_SourceOut_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_SourceOut_template<Rgba64Operations>(dest, src, length, const_alpha);
}
#endif

/*
  result = d * sia
  dest = d * sia * ca + d * cia
       = d * (sia * ca + cia)
*/
template<class Ops>
inline static void comp_func_solid_DestinationOut_template(typename Ops::Type *dest, int length, typename Ops::Type color, uint const_alpha)
{
    auto sai = Ops::invAlpha(Ops::convert(color));
    if (const_alpha != 255) {
        sai = Ops::multiplyAlpha8bit(sai, const_alpha);
        sai = Ops::add(sai, Ops::invAlpha(Ops::scalarFrom8bit(const_alpha)));
    }

    for (int i = 0; i < length; ++i) {
        Ops::store(&dest[i], Ops::multiplyAlpha(Ops::load(&dest[i]), sai));
    }
}

template<class Ops>
inline static void comp_func_DestinationOut_template(typename Ops::Type *Q_DECL_RESTRICT dest,
                                                     const typename Ops::Type *Q_DECL_RESTRICT src,
                                                     int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            auto sia = Ops::invAlpha(Ops::load(&src[i]));
            Ops::store(&dest[i], Ops::multiplyAlpha(Ops::load(&dest[i]), sia));
        }
    } else {
        auto cia = Ops::invAlpha(Ops::scalarFrom8bit(const_alpha));
        for (int i = 0; i < length; ++i) {
            auto sia = Ops::multiplyAlpha8bit(Ops::invAlpha(Ops::load(&src[i])), const_alpha);
            sia = Ops::add(sia, cia);
            Ops::store(&dest[i], Ops::multiplyAlpha(Ops::load(&dest[i]), sia));
        }
    }
}

void QT_FASTCALL comp_func_solid_DestinationOut(uint *dest, int length, uint color, uint const_alpha)
{
    comp_func_solid_DestinationOut_template<Argb32Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_DestinationOut(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_DestinationOut_template<Argb32Operations>(dest, src, length, const_alpha);
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_DestinationOut_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    comp_func_solid_DestinationOut_template<Rgba64Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_DestinationOut_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_DestinationOut_template<Rgba64Operations>(dest, src, length, const_alpha);
}
#endif

/*
  result = s*da + d*sia
  dest = s*da*ca + d*sia*ca + d *cia
       = s*ca * da + d * (sia*ca + cia)
       = s*ca * da + d * (1 - sa*ca)
*/
template<class Ops>
inline static void comp_func_solid_SourceAtop_template(typename Ops::Type *dest, int length, typename Ops::Type color, uint const_alpha)
{
    auto c = Ops::convert(color);
    if (const_alpha != 255)
        c = Ops::multiplyAlpha8bit(c, const_alpha);
    auto sia = Ops::invAlpha(c);
    for (int i = 0; i < length; ++i) {
        auto d = Ops::load(&dest[i]);
        Ops::store(&dest[i], Ops::interpolate(c, Ops::alpha(d), d, sia));
    }
}

template<class Ops>
inline static void comp_func_SourceAtop_template(typename Ops::Type *Q_DECL_RESTRICT dest,
                                                 const typename Ops::Type *Q_DECL_RESTRICT src,
                                                 int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            auto s = Ops::load(&src[i]);
            auto d = Ops::load(&dest[i]);
            Ops::store(&dest[i], Ops::interpolate(s, Ops::alpha(d), d, Ops::invAlpha(s)));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            auto s = Ops::multiplyAlpha8bit(Ops::load(&src[i]), const_alpha);
            auto d = Ops::load(&dest[i]);
            Ops::store(&dest[i], Ops::interpolate(s, Ops::alpha(d), d, Ops::invAlpha(s)));
        }
    }
}

void QT_FASTCALL comp_func_solid_SourceAtop(uint *dest, int length, uint color, uint const_alpha)
{
    comp_func_solid_SourceAtop_template<Argb32Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_SourceAtop(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_SourceAtop_template<Argb32Operations>(dest, src, length, const_alpha);
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_SourceAtop_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    comp_func_solid_SourceAtop_template<Rgba64Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_SourceAtop_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_SourceAtop_template<Rgba64Operations>(dest, src, length, const_alpha);
}
#endif

/*
  result = d*sa + s*dia
  dest = d*sa*ca + s*dia*ca + d *cia
       = s*ca * dia + d * (sa*ca + cia)
*/
template<class Ops>
inline static void comp_func_solid_DestinationAtop_template(typename Ops::Type *dest, int length, typename Ops::Type color, uint const_alpha)
{
    auto c = Ops::convert(color);
    auto sa = Ops::alpha(c);
    if (const_alpha != 255) {
        c = Ops::multiplyAlpha8bit(c, const_alpha);
        auto cia = Ops::invAlpha(Ops::scalarFrom8bit(const_alpha));
        sa = Ops::add(Ops::alpha(c), cia);
    }

    for (int i = 0; i < length; ++i) {
        auto d = Ops::load(&dest[i]);
        Ops::store(&dest[i], Ops::interpolate(c, Ops::invAlpha(d), d, sa));
    }
}

template<class Ops>
inline static void comp_func_DestinationAtop_template(typename Ops::Type *Q_DECL_RESTRICT dest,
                                                      const typename Ops::Type *Q_DECL_RESTRICT src,
                                                      int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            auto s = Ops::load(&src[i]);
            auto d = Ops::load(&dest[i]);
            Ops::store(&dest[i], Ops::interpolate(s, Ops::invAlpha(d), d, Ops::alpha(s)));
        }
    } else {
        auto cia = Ops::invAlpha(Ops::scalarFrom8bit(const_alpha));
        for (int i = 0; i < length; ++i) {
            auto s = Ops::multiplyAlpha8bit(Ops::load(&src[i]), const_alpha);
            auto d = Ops::load(&dest[i]);
            auto sa = Ops::add(Ops::alpha(s), cia);
            Ops::store(&dest[i], Ops::interpolate(s, Ops::invAlpha(d), d, sa));
        }
    }
}

void QT_FASTCALL comp_func_solid_DestinationAtop(uint *dest, int length, uint color, uint const_alpha)
{
    comp_func_solid_DestinationAtop_template<Argb32Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_DestinationAtop(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_DestinationAtop_template<Argb32Operations>(dest, src, length, const_alpha);
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_DestinationAtop_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    comp_func_solid_DestinationAtop_template<Rgba64Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_DestinationAtop_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_DestinationAtop_template<Rgba64Operations>(dest, src, length, const_alpha);
}
#endif

/*
  result = d*sia + s*dia
  dest = d*sia*ca + s*dia*ca + d *cia
       = s*ca * dia + d * (sia*ca + cia)
       = s*ca * dia + d * (1 - sa*ca)
*/
template<class Ops>
inline static void comp_func_solid_XOR_template(typename Ops::Type *dest, int length, typename Ops::Type color, uint const_alpha)
{
    auto c = Ops::convert(color);
    if (const_alpha != 255)
        c = Ops::multiplyAlpha8bit(c, const_alpha);

    auto sia = Ops::invAlpha(c);
    for (int i = 0; i < length; ++i) {
        auto d = Ops::load(&dest[i]);
        Ops::store(&dest[i], Ops::interpolate(c, Ops::invAlpha(d), d, sia));
    }
}

template<class Ops>
inline static void comp_func_XOR_template(typename Ops::Type *Q_DECL_RESTRICT dest,
                                          const typename Ops::Type *Q_DECL_RESTRICT src,
                                          int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            auto d = Ops::load(&dest[i]);
            auto s = Ops::load(&src[i]);
            Ops::store(&dest[i], Ops::interpolate(s, Ops::invAlpha(d), d, Ops::invAlpha(s)));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            auto d = Ops::load(&dest[i]);
            auto s = Ops::multiplyAlpha8bit(Ops::load(&src[i]), const_alpha);
            Ops::store(&dest[i], Ops::interpolate(s, Ops::invAlpha(d), d, Ops::invAlpha(s)));
        }
    }
}

void QT_FASTCALL comp_func_solid_XOR(uint *dest, int length, uint color, uint const_alpha)
{
    comp_func_solid_XOR_template<Argb32Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_XOR(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_XOR_template<Argb32Operations>(dest, src, length, const_alpha);
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_XOR_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    comp_func_solid_XOR_template<Rgba64Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_XOR_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_XOR_template<Rgba64Operations>(dest, src, length, const_alpha);
}
#endif

struct QFullCoverage {
    inline void store(uint *dest, const uint src) const
    {
        *dest = src;
    }
    inline void store(QRgba64 *dest, const QRgba64 src) const
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
    inline void store(QRgba64 *dest, const QRgba64 src) const
    {
        *dest = interpolate255(src, ca, *dest, ica);
    }

private:
    const uint ca;
    const uint ica;
};

static inline int mix_alpha(int da, int sa)
{
    return 255 - ((255 - sa) * (255 - da) >> 8);
}

static inline uint mix_alpha_rgb64(uint da, uint sa)
{
    return 65535 - ((65535 - sa) * (65535 - da) >> 16);
}

/*
    Dca' = Sca.Da + Dca.Sa + Sca.(1 - Da) + Dca.(1 - Sa)
         = Sca + Dca
*/
template<class Ops>
inline static void comp_func_solid_Plus_template(typename Ops::Type *dest, int length, typename Ops::Type color, uint const_alpha)
{
    auto c = Ops::convert(color);
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            auto d = Ops::load(&dest[i]);
            d = Ops::plus(d, c);
            Ops::store(&dest[i], d);
        }
    } else {
        uint ia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            auto d = Ops::load(&dest[i]);
            d = Ops::interpolate8bit(Ops::plus(d, c), const_alpha, d, ia);
            Ops::store(&dest[i], d);
        }
    }
}

template<class Ops>
inline static void comp_func_Plus_template(typename Ops::Type *Q_DECL_RESTRICT dest,
                                           const typename Ops::Type *Q_DECL_RESTRICT src,
                                           int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            auto d = Ops::load(&dest[i]);
            auto s = Ops::load(&src[i]);
            d = Ops::plus(d, s);
            Ops::store(&dest[i], d);
        }
    } else {
        uint ia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            auto d = Ops::load(&dest[i]);
            auto s = Ops::load(&src[i]);
            d = Ops::interpolate8bit(Ops::plus(d, s), const_alpha, d, ia);
            Ops::store(&dest[i], d);
        }
    }
}

void QT_FASTCALL comp_func_solid_Plus(uint *dest, int length, uint color, uint const_alpha)
{
    comp_func_solid_Plus_template<Argb32Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_Plus(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_Plus_template<Argb32Operations>(dest, src, length, const_alpha);
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_Plus_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    comp_func_solid_Plus_template<Rgba64Operations>(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_Plus_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    comp_func_Plus_template<Rgba64Operations>(dest, src, length, const_alpha);
}
#endif

/*
    Dca' = Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int multiply_op(int dst, int src, int da, int sa)
{
    return qt_div_255(src * dst + src * (255 - da) + dst * (255 - sa));
}

template <typename T>
static inline void comp_func_solid_Multiply_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
static inline uint multiply_op_rgb64(uint dst, uint src, uint da, uint sa)
{
    return qt_div_65535(src * dst + src * (65535 - da) + dst * (65535 - sa));
}

template <typename T>
static inline void comp_func_solid_Multiply_impl(QRgba64 *dest, int length, QRgba64 color, const T &coverage)
{
    uint sa = color.alpha();
    uint sr = color.red();
    uint sg = color.green();
    uint sb = color.blue();

    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        uint da = d.alpha();

#define OP(a, b) multiply_op_rgb64(a, b, da, sa)
        uint r = OP(  d.red(), sr);
        uint b = OP( d.blue(), sb);
        uint g = OP(d.green(), sg);
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Multiply_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Multiply_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Multiply_impl(dest, length, color, QPartialCoverage(const_alpha));
}
#endif

template <typename T>
static inline void comp_func_Multiply_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
template <typename T>
static inline void comp_func_Multiply_impl(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        QRgba64 s = src[i];

        uint da = d.alpha();
        uint sa = s.alpha();

#define OP(a, b) multiply_op_rgb64(a, b, da, sa)
        uint r = OP(  d.red(),   s.red());
        uint b = OP( d.blue(),  s.blue());
        uint g = OP(d.green(), s.green());
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Multiply_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Multiply_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Multiply_impl(dest, src, length, QPartialCoverage(const_alpha));
}
#endif

/*
    Dca' = (Sca.Da + Dca.Sa - Sca.Dca) + Sca.(1 - Da) + Dca.(1 - Sa)
         = Sca + Dca - Sca.Dca
*/
template <typename T>
static inline void comp_func_solid_Screen_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
template <typename T>
static inline void comp_func_solid_Screen_impl(QRgba64 *dest, int length, QRgba64 color, const T &coverage)
{
    uint sa = color.alpha();
    uint sr = color.red();
    uint sg = color.green();
    uint sb = color.blue();

    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        uint da = d.alpha();

#define OP(a, b) 65535 - qt_div_65535((65535-a) * (65535-b))
        uint r = OP(  d.red(), sr);
        uint b = OP( d.blue(), sb);
        uint g = OP(d.green(), sg);
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Screen_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Screen_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Screen_impl(dest, length, color, QPartialCoverage(const_alpha));
}
#endif

template <typename T>
static inline void comp_func_Screen_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
template <typename T>
static inline void comp_func_Screen_impl(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        QRgba64 s = src[i];

        uint da = d.alpha();
        uint sa = s.alpha();

#define OP(a, b) 65535 - (((65535-a) * (65535-b)) >> 16)
        uint r = OP(  d.red(),   s.red());
        uint b = OP( d.blue(),  s.blue());
        uint g = OP(d.green(), s.green());
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Screen_rgb64(QRgba64 *dest, const QRgba64 *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Screen_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Screen_impl(dest, src, length, QPartialCoverage(const_alpha));
}
#endif

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
static inline void comp_func_solid_Overlay_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
static inline uint overlay_op_rgb64(uint dst, uint src, uint da, uint sa)
{
    const uint temp = src * (65535 - da) + dst * (65535 - sa);
    if (2 * dst < da)
        return qt_div_65535(2 * src * dst + temp);
    else
        return qt_div_65535(sa * da - 2 * (da - dst) * (sa - src) + temp);
}

template <typename T>
static inline void comp_func_solid_Overlay_impl(QRgba64 *dest, int length, QRgba64 color, const T &coverage)
{
    uint sa = color.alpha();
    uint sr = color.red();
    uint sg = color.green();
    uint sb = color.blue();

    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        uint da = d.alpha();

#define OP(a, b) overlay_op_rgb64(a, b, da, sa)
        uint r = OP(  d.red(), sr);
        uint b = OP( d.blue(), sb);
        uint g = OP(d.green(), sg);
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Overlay_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Overlay_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Overlay_impl(dest, length, color, QPartialCoverage(const_alpha));
}
#endif

template <typename T>
static inline void comp_func_Overlay_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
template <typename T>
static inline void comp_func_Overlay_impl(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        QRgba64 s = src[i];

        uint da = d.alpha();
        uint sa = s.alpha();

#define OP(a, b) overlay_op_rgb64(a, b, da, sa)
        uint r = OP(  d.red(),   s.red());
        uint b = OP( d.blue(),  s.blue());
        uint g = OP(d.green(), s.green());
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Overlay_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Overlay_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Overlay_impl(dest, src, length, QPartialCoverage(const_alpha));
}
#endif

/*
    Dca' = min(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
    Da'  = Sa + Da - Sa.Da
*/
static inline int darken_op(int dst, int src, int da, int sa)
{
    return qt_div_255(qMin(src * da, dst * sa) + src * (255 - da) + dst * (255 - sa));
}

template <typename T>
static inline void comp_func_solid_Darken_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
static inline uint darken_op_rgb64(uint dst, uint src, uint da, uint sa)
{
    return qt_div_65535(qMin(src * da, dst * sa) + src * (65535 - da) + dst * (65535 - sa));
}

template <typename T>
static inline void comp_func_solid_Darken_impl(QRgba64 *dest, int length, QRgba64 color, const T &coverage)
{
    uint sa = color.alpha();
    uint sr = color.red();
    uint sg = color.green();
    uint sb = color.blue();

    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        uint da = d.alpha();

#define OP(a, b) darken_op_rgb64(a, b, da, sa)
        uint r = OP(  d.red(), sr);
        uint b = OP( d.blue(), sb);
        uint g = OP(d.green(), sg);
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Darken_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Darken_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Darken_impl(dest, length, color, QPartialCoverage(const_alpha));
}
#endif

template <typename T>
static inline void comp_func_Darken_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
template <typename T>
static inline void comp_func_Darken_impl(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        QRgba64 s = src[i];

        uint da = d.alpha();
        uint sa = s.alpha();

#define OP(a, b) darken_op_rgb64(a, b, da, sa)
        uint r = OP(  d.red(),   s.red());
        uint b = OP( d.blue(),  s.blue());
        uint g = OP(d.green(), s.green());
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Darken_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Darken_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Darken_impl(dest, src, length, QPartialCoverage(const_alpha));
}
#endif

/*
   Dca' = max(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
   Da'  = Sa + Da - Sa.Da
*/
static inline int lighten_op(int dst, int src, int da, int sa)
{
    return qt_div_255(qMax(src * da, dst * sa) + src * (255 - da) + dst * (255 - sa));
}

template <typename T>
static inline void comp_func_solid_Lighten_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
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


#if QT_CONFIG(raster_64bit)
static inline uint lighten_op_rgb64(uint dst, uint src, uint da, uint sa)
{
    return qt_div_65535(qMax(src * da, dst * sa) + src * (65535 - da) + dst * (65535 - sa));
}

template <typename T>
static inline void comp_func_solid_Lighten_impl(QRgba64 *dest, int length, QRgba64 color, const T &coverage)
{
    uint sa = color.alpha();
    uint sr = color.red();
    uint sg = color.green();
    uint sb = color.blue();

    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        uint da = d.alpha();

#define OP(a, b) lighten_op_rgb64(a, b, da, sa)
        uint r = OP(  d.red(), sr);
        uint b = OP( d.blue(), sb);
        uint g = OP(d.green(), sg);
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Lighten_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Lighten_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Lighten_impl(dest, length, color, QPartialCoverage(const_alpha));
}
#endif

template <typename T>
static inline void comp_func_Lighten_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
template <typename T>
static inline void comp_func_Lighten_impl(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        QRgba64 s = src[i];

        uint da = d.alpha();
        uint sa = s.alpha();

#define OP(a, b) lighten_op_rgb64(a, b, da, sa)
        uint r = OP(  d.red(),   s.red());
        uint b = OP( d.blue(),  s.blue());
        uint g = OP(d.green(), s.green());
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Lighten_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Lighten_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Lighten_impl(dest, src, length, QPartialCoverage(const_alpha));
}
#endif

/*
   if Sca.Da + Dca.Sa > Sa.Da
       Dca' = Sa.Da + Sca.(1 - Da) + Dca.(1 - Sa)
   else if Sca == Sa
       Dca' = Dca.Sa + Sca.(1 - Da) + Dca.(1 - Sa)
   otherwise
       Dca' = Dca.Sa/(1-Sca/Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int color_dodge_op(int dst, int src, int da, int sa)
{
    const int sa_da = sa * da;
    const int dst_sa = dst * sa;
    const int src_da = src * da;

    const int temp = src * (255 - da) + dst * (255 - sa);
    if (src_da + dst_sa > sa_da)
        return qt_div_255(sa_da + temp);
    else if (src == sa || sa == 0)
        return qt_div_255(temp);
    else
        return qt_div_255(255 * dst_sa / (255 - 255 * src / sa) + temp);
}

template <typename T>
static inline void comp_func_solid_ColorDodge_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
static inline uint color_dodge_op_rgb64(qint64 dst, qint64 src, qint64 da, qint64 sa)
{
    const qint64 sa_da = sa * da;
    const qint64 dst_sa = dst * sa;
    const qint64 src_da = src * da;

    const qint64 temp = src * (65535 - da) + dst * (65535 - sa);
    if (src_da + dst_sa > sa_da)
        return qt_div_65535(sa_da + temp);
    else if (src == sa || sa == 0)
        return qt_div_65535(temp);
    else
        return qt_div_65535(65535 * dst_sa / (65535 - 65535 * src / sa) + temp);
}

template <typename T>
static inline void comp_func_solid_ColorDodge_impl(QRgba64 *dest, int length, QRgba64 color, const T &coverage)
{
    uint sa = color.alpha();
    uint sr = color.red();
    uint sg = color.green();
    uint sb = color.blue();

    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        uint da = d.alpha();

#define OP(a,b) color_dodge_op_rgb64(a, b, da, sa)
        uint r = OP(  d.red(), sr);
        uint b = OP( d.blue(), sb);
        uint g = OP(d.green(), sg);
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_ColorDodge_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_ColorDodge_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_ColorDodge_impl(dest, length, color, QPartialCoverage(const_alpha));
}
#endif

template <typename T>
static inline void comp_func_ColorDodge_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
template <typename T>
static inline void comp_func_ColorDodge_impl(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        QRgba64 s = src[i];

        uint da = d.alpha();
        uint sa = s.alpha();

#define OP(a, b) color_dodge_op_rgb64(a, b, da, sa)
        uint r = OP(  d.red(),   s.red());
        uint b = OP( d.blue(),  s.blue());
        uint g = OP(d.green(), s.green());
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_ColorDodge_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_ColorDodge_impl(dest, src, length, QFullCoverage());
    else
        comp_func_ColorDodge_impl(dest, src, length, QPartialCoverage(const_alpha));
}
#endif

/*
   if Sca.Da + Dca.Sa < Sa.Da
       Dca' = Sca.(1 - Da) + Dca.(1 - Sa)
   else if Sca == 0
       Dca' = Dca.Sa + Sca.(1 - Da) + Dca.(1 - Sa)
   otherwise
       Dca' = Sa.(Sca.Da + Dca.Sa - Sa.Da)/Sca + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int color_burn_op(int dst, int src, int da, int sa)
{
    const int src_da = src * da;
    const int dst_sa = dst * sa;
    const int sa_da = sa * da;

    const int temp = src * (255 - da) + dst * (255 - sa);

    if (src_da + dst_sa < sa_da)
        return qt_div_255(temp);
    else if (src == 0)
        return qt_div_255(dst_sa + temp);
    return qt_div_255(sa * (src_da + dst_sa - sa_da) / src + temp);
}

template <typename T>
static inline void comp_func_solid_ColorBurn_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
static inline uint color_burn_op_rgb64(qint64 dst, qint64 src, qint64 da, qint64 sa)
{
    const qint64 src_da = src * da;
    const qint64 dst_sa = dst * sa;
    const qint64 sa_da = sa * da;

    const qint64 temp = src * (65535 - da) + dst * (65535 - sa);

    if (src_da + dst_sa < sa_da)
        return qt_div_65535(temp);
    else if (src == 0)
        return qt_div_65535(dst_sa + temp);
    return qt_div_65535(sa * (src_da + dst_sa - sa_da) / src + temp);
}

template <typename T>
static inline void comp_func_solid_ColorBurn_impl(QRgba64 *dest, int length, QRgba64 color, const T &coverage)
{
    uint sa = color.alpha();
    uint sr = color.red();
    uint sg = color.green();
    uint sb = color.blue();

    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        uint da = d.alpha();

#define OP(a, b) color_burn_op_rgb64(a, b, da, sa)
        uint r =  OP(  d.red(), sr);
        uint b =  OP( d.blue(), sb);
        uint g =  OP(d.green(), sg);
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_ColorBurn_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_ColorBurn_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_ColorBurn_impl(dest, length, color, QPartialCoverage(const_alpha));
}
#endif

template <typename T>
static inline void comp_func_ColorBurn_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
template <typename T>
static inline void comp_func_ColorBurn_impl(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        QRgba64 s = src[i];

        uint da = d.alpha();
        uint sa = s.alpha();

#define OP(a, b) color_burn_op_rgb64(a, b, da, sa)
        uint r = OP(  d.red(),   s.red());
        uint b = OP( d.blue(),  s.blue());
        uint g = OP(d.green(), s.green());
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_ColorBurn_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_ColorBurn_impl(dest, src, length, QFullCoverage());
    else
        comp_func_ColorBurn_impl(dest, src, length, QPartialCoverage(const_alpha));
}
#endif

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
static inline void comp_func_solid_HardLight_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
static inline uint hardlight_op_rgb64(uint dst, uint src, uint da, uint sa)
{
    const uint temp = src * (65535 - da) + dst * (65535 - sa);

    if (2 * src < sa)
        return qt_div_65535(2 * src * dst + temp);
    else
        return qt_div_65535(sa * da - 2 * (da - dst) * (sa - src) + temp);
}

template <typename T>
static inline void comp_func_solid_HardLight_impl(QRgba64 *dest, int length, QRgba64 color, const T &coverage)
{
    uint sa = color.alpha();
    uint sr = color.red();
    uint sg = color.green();
    uint sb = color.blue();

    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        uint da = d.alpha();

#define OP(a, b) hardlight_op_rgb64(a, b, da, sa)
        uint r =  OP(  d.red(), sr);
        uint b =  OP( d.blue(), sb);
        uint g =  OP(d.green(), sg);
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_HardLight_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_HardLight_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_HardLight_impl(dest, length, color, QPartialCoverage(const_alpha));
}
#endif

template <typename T>
static inline void comp_func_HardLight_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
template <typename T>
static inline void comp_func_HardLight_impl(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        QRgba64 s = src[i];

        uint da = d.alpha();
        uint sa = s.alpha();

#define OP(a, b) hardlight_op_rgb64(a, b, da, sa)
        uint r = OP(  d.red(),   s.red());
        uint b = OP( d.blue(),  s.blue());
        uint g = OP(d.green(), s.green());
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_HardLight_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_HardLight_impl(dest, src, length, QFullCoverage());
    else
        comp_func_HardLight_impl(dest, src, length, QPartialCoverage(const_alpha));
}
#endif

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
static inline void comp_func_solid_SoftLight_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
static inline uint soft_light_op_rgb64(qint64 dst, qint64 src, qint64 da, qint64 sa)
{
    const qint64 src2 = src << 1;
    const qint64 dst_np = da != 0 ? (65535 * dst) / da : 0;
    const qint64 temp = (src * (65535 - da) + dst * (65535 - sa)) * 65535;
    const qint64 factor = qint64(65535) * 65535;

    if (src2 < sa)
        return (dst * (sa * 65535 + (src2 - sa) * (65535 - dst_np)) + temp) / factor;
    else if (4 * dst <= da)
        return (dst * sa * 65535 + da * (src2 - sa) * ((((16 * dst_np - 12 * 65535) * dst_np + 3 * factor) * dst_np) / factor) + temp) / factor;
    else {
        return (dst * sa * 65535 + da * (src2 - sa) * (int(qSqrt(qreal(dst_np * 65535))) - dst_np) + temp) / factor;
    }
}

template <typename T>
static inline void comp_func_solid_SoftLight_impl(QRgba64 *dest, int length, QRgba64 color, const T &coverage)
{
    uint sa = color.alpha();
    uint sr = color.red();
    uint sg = color.green();
    uint sb = color.blue();

    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        uint da = d.alpha();

#define OP(a, b) soft_light_op_rgb64(a, b, da, sa)
        uint r =  OP(  d.red(), sr);
        uint b =  OP( d.blue(), sb);
        uint g =  OP(d.green(), sg);
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}
#endif

void QT_FASTCALL comp_func_solid_SoftLight(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_SoftLight_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_SoftLight_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
static inline void comp_func_SoftLight_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_SoftLight_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_SoftLight_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_SoftLight_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
static inline void comp_func_SoftLight_impl(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        QRgba64 s = src[i];

        uint da = d.alpha();
        uint sa = s.alpha();

#define OP(a, b) soft_light_op_rgb64(a, b, da, sa)
        uint r = OP(  d.red(),   s.red());
        uint b = OP( d.blue(),  s.blue());
        uint g = OP(d.green(), s.green());
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_SoftLight_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_SoftLight_impl(dest, src, length, QFullCoverage());
    else
        comp_func_SoftLight_impl(dest, src, length, QPartialCoverage(const_alpha));
}
#endif

/*
   Dca' = abs(Dca.Sa - Sca.Da) + Sca.(1 - Da) + Dca.(1 - Sa)
        = Sca + Dca - 2.min(Sca.Da, Dca.Sa)
*/
static inline int difference_op(int dst, int src, int da, int sa)
{
    return src + dst - qt_div_255(2 * qMin(src * da, dst * sa));
}

template <typename T>
static inline void comp_func_solid_Difference_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
static inline uint difference_op_rgb64(qint64 dst, qint64 src, qint64 da, qint64 sa)
{
    return src + dst - qt_div_65535(2 * qMin(src * da, dst * sa));
}

template <typename T>
static inline void comp_func_solid_Difference_impl(QRgba64 *dest, int length, QRgba64 color, const T &coverage)
{
    uint sa = color.alpha();
    uint sr = color.red();
    uint sg = color.green();
    uint sb = color.blue();

    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        uint da = d.alpha();

#define OP(a, b) difference_op_rgb64(a, b, da, sa)
        uint r =  OP(  d.red(), sr);
        uint b =  OP( d.blue(), sb);
        uint g =  OP(d.green(), sg);
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Difference_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Difference_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Difference_impl(dest, length, color, QPartialCoverage(const_alpha));
}
#endif

template <typename T>
static inline void comp_func_Difference_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
template <typename T>
static inline void comp_func_Difference_impl(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        QRgba64 s = src[i];

        uint da = d.alpha();
        uint sa = s.alpha();

#define OP(a, b) difference_op_rgb64(a, b, da, sa)
        uint r = OP(  d.red(),   s.red());
        uint b = OP( d.blue(),  s.blue());
        uint g = OP(d.green(), s.green());
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Difference_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Difference_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Difference_impl(dest, src, length, QPartialCoverage(const_alpha));
}
#endif

/*
    Dca' = (Sca.Da + Dca.Sa - 2.Sca.Dca) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
template <typename T>
static inline void QT_FASTCALL comp_func_solid_Exclusion_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
template <typename T>
static inline void QT_FASTCALL comp_func_solid_Exclusion_impl(QRgba64 *dest, int length, QRgba64 color, const T &coverage)
{
    uint sa = color.alpha();
    uint sr = color.red();
    uint sg = color.green();
    uint sb = color.blue();

    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        uint da = d.alpha();

#define OP(a, b) (a + b - qt_div_65535(2*(qint64(a)*b)))
        uint r =  OP(  d.red(), sr);
        uint b =  OP( d.blue(), sb);
        uint g =  OP(d.green(), sg);
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}


void QT_FASTCALL comp_func_solid_Exclusion_rgb64(QRgba64 *dest, int length, QRgba64 color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Exclusion_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Exclusion_impl(dest, length, color, QPartialCoverage(const_alpha));
}
#endif

template <typename T>
static inline void comp_func_Exclusion_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
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

#if QT_CONFIG(raster_64bit)
template <typename T>
static inline void comp_func_Exclusion_impl(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        QRgba64 d = dest[i];
        QRgba64 s = src[i];

        uint da = d.alpha();
        uint sa = s.alpha();

#define OP(a, b)  (a + b - ((qint64(a)*b) >> 15))
        uint r = OP(  d.red(),   s.red());
        uint b = OP( d.blue(),  s.blue());
        uint g = OP(d.green(), s.green());
        uint a = mix_alpha_rgb64(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba64(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Exclusion_rgb64(QRgba64 *Q_DECL_RESTRICT dest, const QRgba64 *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Exclusion_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Exclusion_impl(dest, src, length, QPartialCoverage(const_alpha));
}
#endif

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
#if QT_CONFIG(raster_64bit)
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
        comp_func_solid_Multiply_rgb64,
        comp_func_solid_Screen_rgb64,
        comp_func_solid_Overlay_rgb64,
        comp_func_solid_Darken_rgb64,
        comp_func_solid_Lighten_rgb64,
        comp_func_solid_ColorDodge_rgb64,
        comp_func_solid_ColorBurn_rgb64,
        comp_func_solid_HardLight_rgb64,
        comp_func_solid_SoftLight_rgb64,
        comp_func_solid_Difference_rgb64,
        comp_func_solid_Exclusion_rgb64,
#else
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#endif
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
#if QT_CONFIG(raster_64bit)
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
        comp_func_Multiply_rgb64,
        comp_func_Screen_rgb64,
        comp_func_Overlay_rgb64,
        comp_func_Darken_rgb64,
        comp_func_Lighten_rgb64,
        comp_func_ColorDodge_rgb64,
        comp_func_ColorBurn_rgb64,
        comp_func_HardLight_rgb64,
        comp_func_SoftLight_rgb64,
        comp_func_Difference_rgb64,
        comp_func_Exclusion_rgb64,
#else
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#endif
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
};

QT_END_NAMESPACE
