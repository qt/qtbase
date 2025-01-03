// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcolortransform.h"
#include "qcolortransform_p.h"

#include "qcolormatrix_p.h"
#include "qcolorspace_p.h"
#include "qcolortrc_p.h"
#include "qcolortrclut_p.h"

#include <QtCore/qatomic.h>
#include <QtCore/qmath.h>
#include <QtGui/qcolor.h>
#include <QtGui/qimage.h>
#include <QtGui/qtransform.h>
#include <QtCore/private/qsimd_p.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

std::shared_ptr<QColorTrcLut> lutFromTrc(const QColorTrc &trc)
{
    if (trc.m_type == QColorTrc::Type::Table)
        return QColorTrcLut::fromTransferTable(trc.m_table);
    if (trc.m_type == QColorTrc::Type::Function)
        return QColorTrcLut::fromTransferFunction(trc.m_fun);
    qWarning() << "TRC uninitialized";
    return nullptr;
}

void QColorTransformPrivate::updateLutsIn() const
{
    if (colorSpaceIn->lut.generated.loadAcquire())
        return;
    QMutexLocker lock(&QColorSpacePrivate::s_lutWriteLock);
    if (colorSpaceIn->lut.generated.loadRelaxed())
        return;

    for (int i = 0; i < 3; ++i) {
        if (!colorSpaceIn->trc[i].isValid())
            return;
    }

    if (colorSpaceIn->trc[0] == colorSpaceIn->trc[1] && colorSpaceIn->trc[0] == colorSpaceIn->trc[2]) {
        colorSpaceIn->lut[0] = lutFromTrc(colorSpaceIn->trc[0]);
        colorSpaceIn->lut[1] = colorSpaceIn->lut[0];
        colorSpaceIn->lut[2] = colorSpaceIn->lut[0];
    } else {
        for (int i = 0; i < 3; ++i)
            colorSpaceIn->lut[i] = lutFromTrc(colorSpaceIn->trc[i]);
    }

    colorSpaceIn->lut.generated.storeRelease(1);
}

void QColorTransformPrivate::updateLutsOut() const
{
    if (colorSpaceOut->lut.generated.loadAcquire())
        return;
    QMutexLocker lock(&QColorSpacePrivate::s_lutWriteLock);
    if (colorSpaceOut->lut.generated.loadRelaxed())
        return;
    for (int i = 0; i < 3; ++i) {
        if (!colorSpaceOut->trc[i].isValid())
            return;
    }

    if (colorSpaceOut->trc[0] == colorSpaceOut->trc[1] && colorSpaceOut->trc[0] == colorSpaceOut->trc[2]) {
        colorSpaceOut->lut[0] = lutFromTrc(colorSpaceOut->trc[0]);
        colorSpaceOut->lut[1] = colorSpaceOut->lut[0];
        colorSpaceOut->lut[2] = colorSpaceOut->lut[0];
    } else {
        for (int i = 0; i < 3; ++i)
            colorSpaceOut->lut[i] = lutFromTrc(colorSpaceOut->trc[i]);
    }

    colorSpaceOut->lut.generated.storeRelease(1);
}

/*!
    \class QColorTransform
    \brief The QColorTransform class is a transformation between color spaces.
    \since 5.14

    \ingroup painting
    \ingroup appearance
    \inmodule QtGui

    QColorTransform is an instantiation of a transformation between color spaces.
    It can be applied on color and pixels to convert them from one color space to
    another.

    Setting up a QColorTransform takes some preprocessing, so keeping around
    QColorTransforms that you need often is recommended, instead of generating
    them on the fly.
*/


QColorTransform::QColorTransform(const QColorTransform &colorTransform) noexcept = default;

QColorTransform::~QColorTransform() = default;

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QColorTransformPrivate)

/*!
    \since 6.4
    Returns true if the color transform is the identity transform.
*/
bool QColorTransform::isIdentity() const noexcept
{
    return !d || d->isIdentity();
}

/*!
    \fn bool QColorTransform::operator==(const QColorTransform &ct1, const QColorTransform &ct2)
    \since 6.4
    Returns true if \a ct1 defines the same color transformation as \a ct2.
*/

/*!
    \fn bool QColorTransform::operator!=(const QColorTransform &ct1, const QColorTransform &ct2)
    \since 6.4
    Returns true if \a ct1 does not define the same transformation as \a ct2.
*/

/*! \internal
*/
bool QColorTransform::compare(const QColorTransform &other) const
{
    if (d == other.d)
        return true;
    if (bool(d) != bool(other.d))
        return d ? d->isIdentity() : other.d->isIdentity();
    if (d->colorMatrix != other.d->colorMatrix)
        return false;
    if (bool(d->colorSpaceIn) != bool(other.d->colorSpaceIn))
        return false;
    if (bool(d->colorSpaceOut) != bool(other.d->colorSpaceOut))
        return false;
    for (int i = 0; i < 3; ++i) {
        if (d->colorSpaceIn && d->colorSpaceIn->trc[i] != other.d->colorSpaceIn->trc[i])
            return false;
        if (d->colorSpaceOut && d->colorSpaceOut->trc[i] != other.d->colorSpaceOut->trc[i])
            return false;
    }
    return true;
}

/*!
    Applies the color transformation on the QRgb value \a argb.

    The input should be opaque or unpremultiplied.
*/
QRgb QColorTransform::map(QRgb argb) const
{
    if (!d)
        return argb;
    constexpr float f = 1.0f / 255.0f;
    QColorVector c = { qRed(argb) * f, qGreen(argb) * f, qBlue(argb) * f };
    if (d->colorSpaceIn->lut.generated.loadAcquire()) {
        c.x = d->colorSpaceIn->lut[0]->toLinear(c.x);
        c.y = d->colorSpaceIn->lut[1]->toLinear(c.y);
        c.z = d->colorSpaceIn->lut[2]->toLinear(c.z);
    } else {
        c.x = d->colorSpaceIn->trc[0].apply(c.x);
        c.y = d->colorSpaceIn->trc[1].apply(c.y);
        c.z = d->colorSpaceIn->trc[2].apply(c.z);
    }
    c = d->colorMatrix.map(c);
    c.x = std::max(0.0f, std::min(1.0f, c.x));
    c.y = std::max(0.0f, std::min(1.0f, c.y));
    c.z = std::max(0.0f, std::min(1.0f, c.z));
    if (d->colorSpaceOut->lut.generated.loadAcquire()) {
        c.x = d->colorSpaceOut->lut[0]->fromLinear(c.x);
        c.y = d->colorSpaceOut->lut[1]->fromLinear(c.y);
        c.z = d->colorSpaceOut->lut[2]->fromLinear(c.z);
    } else {
        c.x = d->colorSpaceOut->trc[0].applyInverse(c.x);
        c.y = d->colorSpaceOut->trc[1].applyInverse(c.y);
        c.z = d->colorSpaceOut->trc[2].applyInverse(c.z);
    }

    return qRgba(c.x * 255 + 0.5f, c.y * 255 + 0.5f, c.z * 255 + 0.5f, qAlpha(argb));
}

/*!
    Applies the color transformation on the QRgba64 value \a rgba64.

    The input should be opaque or unpremultiplied.
*/
QRgba64 QColorTransform::map(QRgba64 rgba64) const
{
    if (!d)
        return rgba64;
    constexpr float f = 1.0f / 65535.0f;
    QColorVector c = { rgba64.red() * f, rgba64.green() * f, rgba64.blue() * f };
    if (d->colorSpaceIn->lut.generated.loadAcquire()) {
        c.x = d->colorSpaceIn->lut[0]->toLinear(c.x);
        c.y = d->colorSpaceIn->lut[1]->toLinear(c.y);
        c.z = d->colorSpaceIn->lut[2]->toLinear(c.z);
    } else {
        c.x = d->colorSpaceIn->trc[0].apply(c.x);
        c.y = d->colorSpaceIn->trc[1].apply(c.y);
        c.z = d->colorSpaceIn->trc[2].apply(c.z);
    }
    c = d->colorMatrix.map(c);
    c.x = std::max(0.0f, std::min(1.0f, c.x));
    c.y = std::max(0.0f, std::min(1.0f, c.y));
    c.z = std::max(0.0f, std::min(1.0f, c.z));
    if (d->colorSpaceOut->lut.generated.loadAcquire()) {
        c.x = d->colorSpaceOut->lut[0]->fromLinear(c.x);
        c.y = d->colorSpaceOut->lut[1]->fromLinear(c.y);
        c.z = d->colorSpaceOut->lut[2]->fromLinear(c.z);
    } else {
        c.x = d->colorSpaceOut->trc[0].applyInverse(c.x);
        c.y = d->colorSpaceOut->trc[1].applyInverse(c.y);
        c.z = d->colorSpaceOut->trc[2].applyInverse(c.z);
    }

    return QRgba64::fromRgba64(c.x * 65535.f + 0.5f, c.y * 65535.f + 0.5f, c.z * 65535.f + 0.5f, rgba64.alpha());
}

/*!
    Applies the color transformation on the QRgbaFloat16 value \a rgbafp16.

    The input should be opaque or unpremultiplied.
    \since 6.4
*/
QRgbaFloat16 QColorTransform::map(QRgbaFloat16 rgbafp16) const
{
    if (!d)
        return rgbafp16;
    QColorVector c;
    c.x = d->colorSpaceIn->trc[0].applyExtended(rgbafp16.r);
    c.y = d->colorSpaceIn->trc[1].applyExtended(rgbafp16.g);
    c.z = d->colorSpaceIn->trc[2].applyExtended(rgbafp16.b);
    c = d->colorMatrix.map(c);
    rgbafp16.r = qfloat16(d->colorSpaceOut->trc[0].applyInverseExtended(c.x));
    rgbafp16.g = qfloat16(d->colorSpaceOut->trc[1].applyInverseExtended(c.y));
    rgbafp16.b = qfloat16(d->colorSpaceOut->trc[2].applyInverseExtended(c.z));
    return rgbafp16;
}

/*!
    Applies the color transformation on the QRgbaFloat32 value \a rgbafp32.

    The input should be opaque or unpremultiplied.
    \since 6.4
*/
QRgbaFloat32 QColorTransform::map(QRgbaFloat32 rgbafp32) const
{
    if (!d)
        return rgbafp32;
    QColorVector c;
    c.x = d->colorSpaceIn->trc[0].applyExtended(rgbafp32.r);
    c.y = d->colorSpaceIn->trc[1].applyExtended(rgbafp32.g);
    c.z = d->colorSpaceIn->trc[2].applyExtended(rgbafp32.b);
    c = d->colorMatrix.map(c);
    rgbafp32.r = d->colorSpaceOut->trc[0].applyInverseExtended(c.x);
    rgbafp32.g = d->colorSpaceOut->trc[1].applyInverseExtended(c.y);
    rgbafp32.b = d->colorSpaceOut->trc[2].applyInverseExtended(c.z);
    return rgbafp32;
}

/*!
    Applies the color transformation on the QColor value \a color.

*/
QColor QColorTransform::map(const QColor &color) const
{
    if (!d)
        return color;
    QColor clr = color;
    if (color.spec() != QColor::ExtendedRgb || color.spec() != QColor::Rgb)
        clr = clr.toRgb();

    QColorVector c = { (float)clr.redF(), (float)clr.greenF(), (float)clr.blueF() };
    if (clr.spec() == QColor::ExtendedRgb) {
        c.x = d->colorSpaceIn->trc[0].applyExtended(c.x);
        c.y = d->colorSpaceIn->trc[1].applyExtended(c.y);
        c.z = d->colorSpaceIn->trc[2].applyExtended(c.z);
    } else {
        c.x = d->colorSpaceIn->trc[0].apply(c.x);
        c.y = d->colorSpaceIn->trc[1].apply(c.y);
        c.z = d->colorSpaceIn->trc[2].apply(c.z);
    }
    c = d->colorMatrix.map(c);
    bool inGamut = c.x >= 0.0f && c.x <= 1.0f && c.y >= 0.0f && c.y <= 1.0f && c.z >= 0.0f && c.z <= 1.0f;
    if (inGamut) {
        if (d->colorSpaceOut->lut.generated.loadAcquire()) {
            c.x = d->colorSpaceOut->lut[0]->fromLinear(c.x);
            c.y = d->colorSpaceOut->lut[1]->fromLinear(c.y);
            c.z = d->colorSpaceOut->lut[2]->fromLinear(c.z);
        } else {
            c.x = d->colorSpaceOut->trc[0].applyInverse(c.x);
            c.y = d->colorSpaceOut->trc[1].applyInverse(c.y);
            c.z = d->colorSpaceOut->trc[2].applyInverse(c.z);
        }
    } else {
        c.x = d->colorSpaceOut->trc[0].applyInverseExtended(c.x);
        c.y = d->colorSpaceOut->trc[1].applyInverseExtended(c.y);
        c.z = d->colorSpaceOut->trc[2].applyInverseExtended(c.z);
    }
    QColor out;
    out.setRgbF(c.x, c.y, c.z, color.alphaF());
    return out;
}

// Optimized sub-routines for fast block based conversion:

template<bool DoClamp = true>
static void applyMatrix(QColorVector *buffer, const qsizetype len, const QColorMatrix &colorMatrix)
{
#if defined(__SSE2__)
    const __m128 minV = _mm_set1_ps(0.0f);
    const __m128 maxV = _mm_set1_ps(1.0f);
    const __m128 xMat = _mm_loadu_ps(&colorMatrix.r.x);
    const __m128 yMat = _mm_loadu_ps(&colorMatrix.g.x);
    const __m128 zMat = _mm_loadu_ps(&colorMatrix.b.x);
    for (qsizetype j = 0; j < len; ++j) {
        __m128 c = _mm_loadu_ps(&buffer[j].x);
        __m128 cx = _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 cy = _mm_shuffle_ps(c, c, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 cz = _mm_shuffle_ps(c, c, _MM_SHUFFLE(2, 2, 2, 2));
        cx = _mm_mul_ps(cx, xMat);
        cy = _mm_mul_ps(cy, yMat);
        cz = _mm_mul_ps(cz, zMat);
        cx = _mm_add_ps(cx, cy);
        cx = _mm_add_ps(cx, cz);
        // Clamp:
        if (DoClamp) {
            cx = _mm_min_ps(cx, maxV);
            cx = _mm_max_ps(cx, minV);
        }
        _mm_storeu_ps(&buffer[j].x, cx);
    }
#elif defined(__ARM_NEON__)
    const float32x4_t minV = vdupq_n_f32(0.0f);
    const float32x4_t maxV = vdupq_n_f32(1.0f);
    const float32x4_t xMat = vld1q_f32(&colorMatrix.r.x);
    const float32x4_t yMat = vld1q_f32(&colorMatrix.g.x);
    const float32x4_t zMat = vld1q_f32(&colorMatrix.b.x);
    for (qsizetype j = 0; j < len; ++j) {
        float32x4_t c = vld1q_f32(&buffer[j].x);
        float32x4_t cx = vmulq_n_f32(xMat, vgetq_lane_f32(c, 0));
        float32x4_t cy = vmulq_n_f32(yMat, vgetq_lane_f32(c, 1));
        float32x4_t cz = vmulq_n_f32(zMat, vgetq_lane_f32(c, 2));
        cx = vaddq_f32(cx, cy);
        cx = vaddq_f32(cx, cz);
        // Clamp:
        if (DoClamp) {
            cx = vminq_f32(cx, maxV);
            cx = vmaxq_f32(cx, minV);
        }
        vst1q_f32(&buffer[j].x, cx);
    }
#else
    for (int j = 0; j < len; ++j) {
        const QColorVector cv = colorMatrix.map(buffer[j]);
        if (DoClamp) {
            buffer[j].x = std::max(0.0f, std::min(1.0f, cv.x));
            buffer[j].y = std::max(0.0f, std::min(1.0f, cv.y));
            buffer[j].z = std::max(0.0f, std::min(1.0f, cv.z));
        } else {
            buffer[j] = cv;
        }
    }
#endif
}

#if defined(__SSE2__) || defined(__ARM_NEON__)
template<typename T>
static constexpr inline bool isArgb();
template<>
constexpr inline bool isArgb<QRgb>() { return true; }
template<>
constexpr inline bool isArgb<QRgba64>() { return false; }

template<typename T>
static inline int getAlpha(const T &p);
template<>
inline int getAlpha<QRgb>(const QRgb &p)
{   return qAlpha(p); }
template<>
inline int getAlpha<QRgba64>(const QRgba64 &p)
{    return p.alpha(); }
#endif

template<typename T>
static void loadPremultiplied(QColorVector *buffer, const T *src, const qsizetype len, const QColorTransformPrivate *d_ptr);
template<typename T>
static void loadUnpremultiplied(QColorVector *buffer, const T *src, const qsizetype len, const QColorTransformPrivate *d_ptr);

#if defined(__SSE2__)
// Load to [0-alpha] in 4x32 SIMD
template<typename T>
static inline void loadP(const T &p, __m128i &v);

template<>
inline void loadP<QRgb>(const QRgb &p, __m128i &v)
{
    v = _mm_cvtsi32_si128(p);
#if defined(__SSE4_1__)
    v = _mm_cvtepu8_epi32(v);
#else
    v = _mm_unpacklo_epi8(v, _mm_setzero_si128());
    v = _mm_unpacklo_epi16(v, _mm_setzero_si128());
#endif
}

template<>
inline void loadP<QRgba64>(const QRgba64 &p, __m128i &v)
{
    v = _mm_loadl_epi64((const __m128i *)&p);
#if defined(__SSE4_1__)
    v = _mm_cvtepu16_epi32(v);
#else
    v = _mm_unpacklo_epi16(v, _mm_setzero_si128());
#endif
}

template<typename T>
static void loadPremultiplied(QColorVector *buffer, const T *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    const __m128 v4080 = _mm_set1_ps(4080.f);
    const __m128 iFF00 = _mm_set1_ps(1.0f / (255 * 256));
    constexpr bool isARGB = isArgb<T>();
    for (qsizetype i = 0; i < len; ++i) {
        __m128i v;
        loadP<T>(src[i], v);
        __m128 vf = _mm_cvtepi32_ps(v);
        // Approximate 1/a:
        __m128 va = _mm_shuffle_ps(vf, vf, _MM_SHUFFLE(3, 3, 3, 3));
        __m128 via = _mm_rcp_ps(va);
        via = _mm_sub_ps(_mm_add_ps(via, via), _mm_mul_ps(via, _mm_mul_ps(via, va)));
        // v * (1/a)
        vf = _mm_mul_ps(vf, via);

        // Handle zero alpha
        __m128 vAlphaMask = _mm_cmpeq_ps(va, _mm_set1_ps(0.0f));
        vf = _mm_andnot_ps(vAlphaMask, vf);

        // LUT
        v = _mm_cvtps_epi32(_mm_mul_ps(vf, v4080));
        const int ridx = isARGB ? _mm_extract_epi16(v, 4) : _mm_extract_epi16(v, 0);
        const int gidx = _mm_extract_epi16(v, 2);
        const int bidx = isARGB ? _mm_extract_epi16(v, 0) : _mm_extract_epi16(v, 4);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx], 0);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx], 2);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx], 4);
        vf = _mm_mul_ps(_mm_cvtepi32_ps(v), iFF00);

        _mm_storeu_ps(&buffer[i].x, vf);
    }
}

template<>
void loadPremultiplied<QRgbaFloat32>(QColorVector *buffer, const QRgbaFloat32 *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    const __m128 v4080 = _mm_set1_ps(4080.f);
    const __m128 viFF00 = _mm_set1_ps(1.0f / (255 * 256));
    const __m128 vZero = _mm_set1_ps(0.0f);
    const __m128 vOne  = _mm_set1_ps(1.0f);
    for (qsizetype i = 0; i < len; ++i) {
        __m128 vf = _mm_loadu_ps(&src[i].r);
        // Approximate 1/a:
        __m128 va = _mm_shuffle_ps(vf, vf, _MM_SHUFFLE(3, 3, 3, 3));
        __m128 via = _mm_rcp_ps(va);
        via = _mm_sub_ps(_mm_add_ps(via, via), _mm_mul_ps(via, _mm_mul_ps(via, va)));
        // v * (1/a)
        vf = _mm_mul_ps(vf, via);

        // Handle zero alpha
        __m128 vAlphaMask = _mm_cmpeq_ps(va, vZero);
        vf = _mm_andnot_ps(vAlphaMask, vf);

        // LUT
        const __m128 under = _mm_cmplt_ps(vf, vZero);
        const __m128 over = _mm_cmpgt_ps(vf, vOne);
        if (_mm_movemask_ps(_mm_or_ps(under, over)) == 0) {
            // Within gamut
            __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, v4080));
            const int ridx = _mm_extract_epi16(v, 0);
            const int gidx = _mm_extract_epi16(v, 2);
            const int bidx = _mm_extract_epi16(v, 4);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx], 0);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx], 2);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx], 4);
            vf = _mm_mul_ps(_mm_cvtepi32_ps(v), viFF00);
            _mm_storeu_ps(&buffer[i].x, vf);
        } else {
            // Outside 0.0->1.0 gamut
            _mm_storeu_ps(&buffer[i].x, vf);
            buffer[i].x = d_ptr->colorSpaceIn->trc[0].applyExtended(buffer[i].x);
            buffer[i].y = d_ptr->colorSpaceIn->trc[1].applyExtended(buffer[i].y);
            buffer[i].z = d_ptr->colorSpaceIn->trc[2].applyExtended(buffer[i].z);
        }
    }
}

// Load to [0-4080] in 4x32 SIMD
template<typename T>
static inline void loadPU(const T &p, __m128i &v);

template<>
inline void loadPU<QRgb>(const QRgb &p, __m128i &v)
{
    v = _mm_cvtsi32_si128(p);
#if defined(__SSE4_1__)
    v = _mm_cvtepu8_epi32(v);
#else
    v = _mm_unpacklo_epi8(v, _mm_setzero_si128());
    v = _mm_unpacklo_epi16(v, _mm_setzero_si128());
#endif
    v = _mm_slli_epi32(v, 4);
}

template<>
inline void loadPU<QRgba64>(const QRgba64 &p, __m128i &v)
{
    v = _mm_loadl_epi64((const __m128i *)&p);
    v = _mm_sub_epi16(v, _mm_srli_epi16(v, 8));
#if defined(__SSE4_1__)
    v = _mm_cvtepu16_epi32(v);
#else
    v = _mm_unpacklo_epi16(v, _mm_setzero_si128());
#endif
    v = _mm_srli_epi32(v, 4);
}

template<typename T>
void loadUnpremultiplied(QColorVector *buffer, const T *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    constexpr bool isARGB = isArgb<T>();
    const __m128 iFF00 = _mm_set1_ps(1.0f / (255 * 256));
    for (qsizetype i = 0; i < len; ++i) {
        __m128i v;
        loadPU<T>(src[i], v);
        const int ridx = isARGB ? _mm_extract_epi16(v, 4) : _mm_extract_epi16(v, 0);
        const int gidx = _mm_extract_epi16(v, 2);
        const int bidx = isARGB ? _mm_extract_epi16(v, 0) : _mm_extract_epi16(v, 4);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx], 0);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx], 2);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx], 4);
        __m128 vf = _mm_mul_ps(_mm_cvtepi32_ps(v), iFF00);
        _mm_storeu_ps(&buffer[i].x, vf);
    }
}

template<>
void loadUnpremultiplied<QRgbaFloat32>(QColorVector *buffer, const QRgbaFloat32 *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    const __m128 v4080 = _mm_set1_ps(4080.f);
    const __m128 iFF00 = _mm_set1_ps(1.0f / (255 * 256));
    const __m128 vZero = _mm_set1_ps(0.0f);
    const __m128 vOne  = _mm_set1_ps(1.0f);
    for (qsizetype i = 0; i < len; ++i) {
        __m128 vf = _mm_loadu_ps(&src[i].r);
        const __m128 under = _mm_cmplt_ps(vf, vZero);
        const __m128 over = _mm_cmpgt_ps(vf, vOne);
        if (_mm_movemask_ps(_mm_or_ps(under, over)) == 0) {
            // Within gamut
            __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, v4080));
            const int ridx = _mm_extract_epi16(v, 0);
            const int gidx = _mm_extract_epi16(v, 2);
            const int bidx = _mm_extract_epi16(v, 4);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx], 0);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx], 2);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx], 4);
            vf = _mm_mul_ps(_mm_cvtepi32_ps(v), iFF00);
            _mm_storeu_ps(&buffer[i].x, vf);
        } else {
            // Outside 0.0->1.0 gamut
            buffer[i].x = d_ptr->colorSpaceIn->trc[0].applyExtended(src[i].r);
            buffer[i].y = d_ptr->colorSpaceIn->trc[1].applyExtended(src[i].g);
            buffer[i].z = d_ptr->colorSpaceIn->trc[2].applyExtended(src[i].b);
        }
    }
}

#elif defined(__ARM_NEON__)
// Load to [0-alpha] in 4x32 SIMD
template<typename T>
static inline void loadP(const T &p, uint32x4_t &v);

template<>
inline void loadP<QRgb>(const QRgb &p, uint32x4_t &v)
{
    v = vmovl_u16(vget_low_u16(vmovl_u8(vreinterpret_u8_u32(vmov_n_u32(p)))));
}

template<>
inline void loadP<QRgba64>(const QRgba64 &p, uint32x4_t &v)
{
    v = vmovl_u16(vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&p))));
}

template<typename T>
static void loadPremultiplied(QColorVector *buffer, const T *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    constexpr bool isARGB = isArgb<T>();
    const float iFF00 = 1.0f / (255 * 256);
    for (qsizetype i = 0; i < len; ++i) {
        uint32x4_t v;
        loadP<T>(src[i], v);
        float32x4_t vf = vcvtq_f32_u32(v);
        // Approximate 1/a:
        float32x4_t va = vdupq_n_f32(vgetq_lane_f32(vf, 3));
        float32x4_t via = vrecpeq_f32(va); // estimate 1/a
        via = vmulq_f32(vrecpsq_f32(va, via), via);

        // v * (1/a)
        vf = vmulq_f32(vf, via);

        // Handle zero alpha
#if defined(Q_PROCESSOR_ARM_64)
        uint32x4_t vAlphaMask = vceqzq_f32(va);
#else
        uint32x4_t vAlphaMask = vceqq_f32(va, vdupq_n_f32(0.0));
#endif
        vf = vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(vf), vAlphaMask));

        // LUT
        v = vcvtq_u32_f32(vaddq_f32(vmulq_n_f32(vf, 4080.f), vdupq_n_f32(0.5f)));
        const int ridx = isARGB ? vgetq_lane_u32(v, 2) : vgetq_lane_u32(v, 0);
        const int gidx = vgetq_lane_u32(v, 1);
        const int bidx = isARGB ? vgetq_lane_u32(v, 0) : vgetq_lane_u32(v, 2);
        v = vsetq_lane_u32(d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx], v, 0);
        v = vsetq_lane_u32(d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx], v, 1);
        v = vsetq_lane_u32(d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx], v, 2);
        vf = vmulq_n_f32(vcvtq_f32_u32(v), iFF00);

        vst1q_f32(&buffer[i].x, vf);
    }
}

// Load to [0-4080] in 4x32 SIMD
template<typename T>
static inline void loadPU(const T &p, uint32x4_t &v);

template<>
inline void loadPU<QRgb>(const QRgb &p, uint32x4_t &v)
{
    v = vmovl_u16(vget_low_u16(vmovl_u8(vreinterpret_u8_u32(vmov_n_u32(p)))));
    v = vshlq_n_u32(v, 4);
}

template<>
inline void loadPU<QRgba64>(const QRgba64 &p, uint32x4_t &v)
{
    uint16x4_t v16 = vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&p)));
    v16 = vsub_u16(v16, vshr_n_u16(v16, 8));
    v = vmovl_u16(v16);
    v = vshrq_n_u32(v, 4);
}

template<typename T>
void loadUnpremultiplied(QColorVector *buffer, const T *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    constexpr bool isARGB = isArgb<T>();
    const float iFF00 = 1.0f / (255 * 256);
    for (qsizetype i = 0; i < len; ++i) {
        uint32x4_t v;
        loadPU<T>(src[i], v);
        const int ridx = isARGB ? vgetq_lane_u32(v, 2) : vgetq_lane_u32(v, 0);
        const int gidx = vgetq_lane_u32(v, 1);
        const int bidx = isARGB ? vgetq_lane_u32(v, 0) : vgetq_lane_u32(v, 2);
        v = vsetq_lane_u32(d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx], v, 0);
        v = vsetq_lane_u32(d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx], v, 1);
        v = vsetq_lane_u32(d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx], v, 2);
        float32x4_t vf = vmulq_n_f32(vcvtq_f32_u32(v), iFF00);
        vst1q_f32(&buffer[i].x, vf);
    }
}
#else
template<>
void loadPremultiplied<QRgb>(QColorVector *buffer, const QRgb *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const uint p = src[i];
        const int a = qAlpha(p);
        if (a) {
            const float ia = 4080.0f / a;
            const int ridx = int(qRed(p)   * ia + 0.5f);
            const int gidx = int(qGreen(p) * ia + 0.5f);
            const int bidx = int(qBlue(p)  * ia + 0.5f);
            buffer[i].x = d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx] * (1.0f / (255 * 256));
            buffer[i].y = d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx] * (1.0f / (255 * 256));
            buffer[i].z = d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx] * (1.0f / (255 * 256));
        } else {
            buffer[i].x = buffer[i].y = buffer[i].z = 0.0f;
        }
    }
}

template<>
void loadPremultiplied<QRgba64>(QColorVector *buffer, const QRgba64 *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const QRgba64 &p = src[i];
        const int a = p.alpha();
        if (a) {
            const float ia = 4080.0f / a;
            const int ridx = int(p.red()   * ia + 0.5f);
            const int gidx = int(p.green() * ia + 0.5f);
            const int bidx = int(p.blue()  * ia + 0.5f);
            buffer[i].x = d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx] * (1.0f / (255 * 256));
            buffer[i].y = d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx] * (1.0f / (255 * 256));
            buffer[i].z = d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx] * (1.0f / (255 * 256));
        } else {
            buffer[i].x = buffer[i].y = buffer[i].z = 0.0f;
        }
    }
}

template<>
void loadUnpremultiplied<QRgb>(QColorVector *buffer, const QRgb *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const uint p = src[i];
        buffer[i].x = d_ptr->colorSpaceIn->lut[0]->u8ToLinearF32(qRed(p));
        buffer[i].y = d_ptr->colorSpaceIn->lut[1]->u8ToLinearF32(qGreen(p));
        buffer[i].z = d_ptr->colorSpaceIn->lut[2]->u8ToLinearF32(qBlue(p));
    }
}

template<>
void loadUnpremultiplied<QRgba64>(QColorVector *buffer, const QRgba64 *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const QRgba64 &p = src[i];
        buffer[i].x = d_ptr->colorSpaceIn->lut[0]->u16ToLinearF32(p.red());
        buffer[i].y = d_ptr->colorSpaceIn->lut[1]->u16ToLinearF32(p.green());
        buffer[i].z = d_ptr->colorSpaceIn->lut[2]->u16ToLinearF32(p.blue());
    }
}
#endif
#if !defined(__SSE2__)
template<>
void loadPremultiplied<QRgbaFloat32>(QColorVector *buffer, const QRgbaFloat32 *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const QRgbaFloat32 &p = src[i];
        const float a = p.a;
        if (a) {
            const float ia = 1.0f / a;
            buffer[i].x = d_ptr->colorSpaceIn->trc[0].applyExtended(p.r * ia);
            buffer[i].y = d_ptr->colorSpaceIn->trc[1].applyExtended(p.g * ia);
            buffer[i].z = d_ptr->colorSpaceIn->trc[2].applyExtended(p.b * ia);
        } else {
            buffer[i].x = buffer[i].y = buffer[i].z = 0.0f;
        }
    }
}

template<>
void loadUnpremultiplied<QRgbaFloat32>(QColorVector *buffer, const QRgbaFloat32 *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const QRgbaFloat32 &p = src[i];
        buffer[i].x = d_ptr->colorSpaceIn->trc[0].applyExtended(p.r);
        buffer[i].y = d_ptr->colorSpaceIn->trc[1].applyExtended(p.g);
        buffer[i].z = d_ptr->colorSpaceIn->trc[2].applyExtended(p.b);
   }
}
#endif

#if defined(__SSE2__)
template<typename T>
static inline void storeP(T &p, __m128i &v, int a);
template<>
inline void storeP<QRgb>(QRgb &p, __m128i &v, int a)
{
    v = _mm_packs_epi32(v, v);
    v = _mm_insert_epi16(v, a, 3);
    p = _mm_cvtsi128_si32(_mm_packus_epi16(v, v));
}
template<>
inline void storeP<QRgba64>(QRgba64 &p, __m128i &v, int a)
{
#if defined(__SSE4_1__)
    v = _mm_packus_epi32(v, v);
    v = _mm_insert_epi16(v, a, 3);
    _mm_storel_epi64((__m128i *)&p, v);
#else
    const int r = _mm_extract_epi16(v, 0);
    const int g = _mm_extract_epi16(v, 2);
    const int b = _mm_extract_epi16(v, 4);
    p = qRgba64(r, g, b, a);
#endif
}

template<typename T>
static void storePremultiplied(T *dst, const T *src, const QColorVector *buffer, const qsizetype len,
                               const QColorTransformPrivate *d_ptr)
{
    const __m128 v4080 = _mm_set1_ps(4080.f);
    const __m128 iFF00 = _mm_set1_ps(1.0f / (255 * 256));
    constexpr bool isARGB = isArgb<T>();
    for (qsizetype i = 0; i < len; ++i) {
        const int a = getAlpha<T>(src[i]);
        __m128 vf = _mm_loadu_ps(&buffer[i].x);
        __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, v4080));
        __m128 va = _mm_mul_ps(_mm_set1_ps(a), iFF00);
        const int ridx = _mm_extract_epi16(v, 0);
        const int gidx = _mm_extract_epi16(v, 2);
        const int bidx = _mm_extract_epi16(v, 4);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[0]->m_fromLinear[ridx], isARGB ? 4 : 0);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[1]->m_fromLinear[gidx], 2);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[2]->m_fromLinear[bidx], isARGB ? 0 : 4);
        vf = _mm_cvtepi32_ps(v);
        vf = _mm_mul_ps(vf, va);
        v = _mm_cvtps_epi32(vf);
        storeP<T>(dst[i], v, a);
    }
}

template<>
void storePremultiplied<QRgbaFloat32>(QRgbaFloat32 *dst, const QRgbaFloat32 *src,
                                      const QColorVector *buffer, const qsizetype len,
                                      const QColorTransformPrivate *d_ptr)
{
    const __m128 v4080 = _mm_set1_ps(4080.f);
    const __m128 vZero = _mm_set1_ps(0.0f);
    const __m128 vOne  = _mm_set1_ps(1.0f);
    const __m128 viFF00 = _mm_set1_ps(1.0f / (255 * 256));
    for (qsizetype i = 0; i < len; ++i) {
        const float a = src[i].a;
        __m128 va = _mm_set1_ps(a);
        __m128 vf = _mm_loadu_ps(&buffer[i].x);
        const __m128 under = _mm_cmplt_ps(vf, vZero);
        const __m128 over = _mm_cmpgt_ps(vf, vOne);
        if (_mm_movemask_ps(_mm_or_ps(under, over)) == 0) {
            // Within gamut
            va = _mm_mul_ps(va, viFF00);
            __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, v4080));
            const int ridx = _mm_extract_epi16(v, 0);
            const int gidx = _mm_extract_epi16(v, 2);
            const int bidx = _mm_extract_epi16(v, 4);
            v = _mm_setzero_si128();
            v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[0]->m_fromLinear[ridx], 0);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[1]->m_fromLinear[gidx], 2);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[2]->m_fromLinear[bidx], 4);
            vf = _mm_mul_ps(_mm_cvtepi32_ps(v), va);
            _mm_store_ps(&dst[i].r, vf);
        } else {
            dst[i].r = d_ptr->colorSpaceOut->trc[0].applyInverseExtended(buffer[i].x);
            dst[i].g = d_ptr->colorSpaceOut->trc[1].applyInverseExtended(buffer[i].y);
            dst[i].b = d_ptr->colorSpaceOut->trc[2].applyInverseExtended(buffer[i].z);
            vf = _mm_mul_ps(_mm_load_ps(&dst[i].r), va);
            _mm_store_ps(&dst[i].r, vf);
        }
        dst[i].a = a;
    }
}

template<typename T>
static inline void storePU(T &p, __m128i &v, int a);
template<>
inline void storePU<QRgb>(QRgb &p, __m128i &v, int a)
{
    v = _mm_add_epi16(v, _mm_set1_epi16(0x80));
    v = _mm_srli_epi16(v, 8);
    v = _mm_insert_epi16(v, a, 3);
    p = _mm_cvtsi128_si32(_mm_packus_epi16(v, v));
}
template<>
inline void storePU<QRgba64>(QRgba64 &p, __m128i &v, int a)
{
    v = _mm_add_epi16(v, _mm_srli_epi16(v, 8));
    v = _mm_insert_epi16(v, a, 3);
    _mm_storel_epi64((__m128i *)&p, v);
}

template<typename T>
static void storeUnpremultiplied(T *dst, const T *src, const QColorVector *buffer, const qsizetype len,
                                 const QColorTransformPrivate *d_ptr)
{
    const __m128 v4080 = _mm_set1_ps(4080.f);
    constexpr bool isARGB = isArgb<T>();
    for (qsizetype i = 0; i < len; ++i) {
        const int a = getAlpha<T>(src[i]);
        __m128 vf = _mm_loadu_ps(&buffer[i].x);
        __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, v4080));
        const int ridx = _mm_extract_epi16(v, 0);
        const int gidx = _mm_extract_epi16(v, 2);
        const int bidx = _mm_extract_epi16(v, 4);
        v = _mm_setzero_si128();
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[0]->m_fromLinear[ridx], isARGB ? 2 : 0);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[1]->m_fromLinear[gidx], 1);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[2]->m_fromLinear[bidx], isARGB ? 0 : 2);
        storePU<T>(dst[i], v, a);
    }
}

template<>
void storeUnpremultiplied<QRgbaFloat32>(QRgbaFloat32 *dst, const QRgbaFloat32 *src,
                                        const QColorVector *buffer, const qsizetype len,
                                        const QColorTransformPrivate *d_ptr)
{
    const __m128 v4080 = _mm_set1_ps(4080.f);
    const __m128 vZero = _mm_set1_ps(0.0f);
    const __m128 vOne  = _mm_set1_ps(1.0f);
    const __m128 viFF00 = _mm_set1_ps(1.0f / (255 * 256));
    for (qsizetype i = 0; i < len; ++i) {
        const float a = src[i].a;
        __m128 vf = _mm_loadu_ps(&buffer[i].x);
        const __m128 under = _mm_cmplt_ps(vf, vZero);
        const __m128 over = _mm_cmpgt_ps(vf, vOne);
        if (_mm_movemask_ps(_mm_or_ps(under, over)) == 0) {
            // Within gamut
            __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, v4080));
            const int ridx = _mm_extract_epi16(v, 0);
            const int gidx = _mm_extract_epi16(v, 2);
            const int bidx = _mm_extract_epi16(v, 4);
            v = _mm_setzero_si128();
            v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[0]->m_fromLinear[ridx], 0);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[1]->m_fromLinear[gidx], 2);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[2]->m_fromLinear[bidx], 4);
            vf = _mm_mul_ps(_mm_cvtepi32_ps(v), viFF00);
            _mm_storeu_ps(&dst[i].r, vf);
        } else {
            dst[i].r = d_ptr->colorSpaceOut->trc[0].applyInverseExtended(buffer[i].x);
            dst[i].g = d_ptr->colorSpaceOut->trc[1].applyInverseExtended(buffer[i].y);
            dst[i].b = d_ptr->colorSpaceOut->trc[2].applyInverseExtended(buffer[i].z);
        }
        dst[i].a = a;
    }
}

template<typename T>
static void storeOpaque(T *dst, const T *src, const QColorVector *buffer, const qsizetype len,
                        const QColorTransformPrivate *d_ptr)
{
    Q_UNUSED(src);
    const __m128 v4080 = _mm_set1_ps(4080.f);
    constexpr bool isARGB = isArgb<T>();
    for (qsizetype i = 0; i < len; ++i) {
        __m128 vf = _mm_loadu_ps(&buffer[i].x);
        __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, v4080));
        const int ridx = _mm_extract_epi16(v, 0);
        const int gidx = _mm_extract_epi16(v, 2);
        const int bidx = _mm_extract_epi16(v, 4);
        v = _mm_setzero_si128();
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[0]->m_fromLinear[ridx], isARGB ? 2 : 0);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[1]->m_fromLinear[gidx], 1);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[2]->m_fromLinear[bidx], isARGB ? 0 : 2);
        storePU<T>(dst[i], v, isARGB ? 255 : 0xffff);
    }
}

template<>
void storeOpaque<QRgbaFloat32>(QRgbaFloat32 *dst, const QRgbaFloat32 *src,
                               const QColorVector *buffer, const qsizetype len,
                               const QColorTransformPrivate *d_ptr)
{
    Q_UNUSED(src);
    const __m128 v4080 = _mm_set1_ps(4080.f);
    const __m128 vZero = _mm_set1_ps(0.0f);
    const __m128 vOne  = _mm_set1_ps(1.0f);
    const __m128 viFF00 = _mm_set1_ps(1.0f / (255 * 256));
    for (qsizetype i = 0; i < len; ++i) {
        __m128 vf = _mm_loadu_ps(&buffer[i].x);
        const __m128 under = _mm_cmplt_ps(vf, vZero);
        const __m128 over = _mm_cmpgt_ps(vf, vOne);
        if (_mm_movemask_ps(_mm_or_ps(under, over)) == 0) {
            // Within gamut
            __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, v4080));
            const int ridx = _mm_extract_epi16(v, 0);
            const int gidx = _mm_extract_epi16(v, 2);
            const int bidx = _mm_extract_epi16(v, 4);
            v = _mm_setzero_si128();
            v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[0]->m_fromLinear[ridx], 0);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[1]->m_fromLinear[gidx], 2);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[2]->m_fromLinear[bidx], 4);
            vf = _mm_mul_ps(_mm_cvtepi32_ps(v), viFF00);
            _mm_store_ps(&dst[i].r, vf);
        } else {
            dst[i].r = d_ptr->colorSpaceOut->trc[0].applyInverseExtended(buffer[i].x);
            dst[i].g = d_ptr->colorSpaceOut->trc[1].applyInverseExtended(buffer[i].y);
            dst[i].b = d_ptr->colorSpaceOut->trc[2].applyInverseExtended(buffer[i].z);
        }
        dst[i].a = 1.0f;
    }
}

#elif defined(__ARM_NEON__)
template<typename T>
static inline void storeP(T &p, const uint16x4_t &v);
template<>
inline void storeP<QRgb>(QRgb &p, const uint16x4_t &v)
{
    p = vget_lane_u32(vreinterpret_u32_u8(vmovn_u16(vcombine_u16(v, v))), 0);
}
template<>
inline void storeP<QRgba64>(QRgba64 &p, const uint16x4_t &v)
{
    vst1_u16((uint16_t *)&p, v);
}

template<typename T>
static void storePremultiplied(T *dst, const T *src, const QColorVector *buffer, const qsizetype len,
                               const QColorTransformPrivate *d_ptr)
{
    const float iFF00 = 1.0f / (255 * 256);
    constexpr bool isARGB = isArgb<T>();
    for (qsizetype i = 0; i < len; ++i) {
        const int a = getAlpha<T>(src[i]);
        float32x4_t vf = vld1q_f32(&buffer[i].x);
        uint32x4_t v = vcvtq_u32_f32(vaddq_f32(vmulq_n_f32(vf, 4080.f), vdupq_n_f32(0.5f)));
        const int ridx = vgetq_lane_u32(v, 0);
        const int gidx = vgetq_lane_u32(v, 1);
        const int bidx = vgetq_lane_u32(v, 2);
        v = vsetq_lane_u32(d_ptr->colorSpaceOut->lut[0]->m_fromLinear[ridx], v, isARGB ? 2 : 0);
        v = vsetq_lane_u32(d_ptr->colorSpaceOut->lut[1]->m_fromLinear[gidx], v, 1);
        v = vsetq_lane_u32(d_ptr->colorSpaceOut->lut[2]->m_fromLinear[bidx], v, isARGB ? 0 : 2);
        vf = vcvtq_f32_u32(v);
        vf = vmulq_n_f32(vf, a * iFF00);
        vf = vaddq_f32(vf, vdupq_n_f32(0.5f));
        v = vcvtq_u32_f32(vf);
        uint16x4_t v16 = vmovn_u32(v);
        v16 = vset_lane_u16(a, v16, 3);
        storeP<T>(dst[i], v16);
    }
}

template<typename T>
static inline void storePU(T &p, uint16x4_t &v, int a);
template<>
inline void storePU<QRgb>(QRgb &p, uint16x4_t &v, int a)
{
    v = vadd_u16(v, vdup_n_u16(0x80));
    v = vshr_n_u16(v, 8);
    v = vset_lane_u16(a, v, 3);
    p = vget_lane_u32(vreinterpret_u32_u8(vmovn_u16(vcombine_u16(v, v))), 0);
}
template<>
inline void storePU<QRgba64>(QRgba64 &p, uint16x4_t &v, int a)
{
    v = vadd_u16(v, vshr_n_u16(v, 8));
    v = vset_lane_u16(a, v, 3);
    vst1_u16((uint16_t *)&p, v);
}

template<typename T>
static void storeUnpremultiplied(T *dst, const T *src, const QColorVector *buffer, const qsizetype len,
                                 const QColorTransformPrivate *d_ptr)
{
    constexpr bool isARGB = isArgb<T>();
    for (qsizetype i = 0; i < len; ++i) {
        const int a = getAlpha<T>(src[i]);
        float32x4_t vf = vld1q_f32(&buffer[i].x);
        uint16x4_t v = vmovn_u32(vcvtq_u32_f32(vaddq_f32(vmulq_n_f32(vf, 4080.f), vdupq_n_f32(0.5f))));
        const int ridx = vget_lane_u16(v, 0);
        const int gidx = vget_lane_u16(v, 1);
        const int bidx = vget_lane_u16(v, 2);
        v = vset_lane_u16(d_ptr->colorSpaceOut->lut[0]->m_fromLinear[ridx], v, isARGB ? 2 : 0);
        v = vset_lane_u16(d_ptr->colorSpaceOut->lut[1]->m_fromLinear[gidx], v, 1);
        v = vset_lane_u16(d_ptr->colorSpaceOut->lut[2]->m_fromLinear[bidx], v, isARGB ? 0 : 2);
        storePU<T>(dst[i], v, a);
    }
}

template<typename T>
static void storeOpaque(T *dst, const T *src, const QColorVector *buffer, const qsizetype len,
                        const QColorTransformPrivate *d_ptr)
{
    Q_UNUSED(src);
    constexpr bool isARGB = isArgb<T>();
    for (qsizetype i = 0; i < len; ++i) {
        float32x4_t vf = vld1q_f32(&buffer[i].x);
        uint16x4_t v = vmovn_u32(vcvtq_u32_f32(vaddq_f32(vmulq_n_f32(vf, 4080.f), vdupq_n_f32(0.5f))));
        const int ridx = vget_lane_u16(v, 0);
        const int gidx = vget_lane_u16(v, 1);
        const int bidx = vget_lane_u16(v, 2);
        v = vset_lane_u16(d_ptr->colorSpaceOut->lut[0]->m_fromLinear[ridx], v, isARGB ? 2 : 0);
        v = vset_lane_u16(d_ptr->colorSpaceOut->lut[1]->m_fromLinear[gidx], v, 1);
        v = vset_lane_u16(d_ptr->colorSpaceOut->lut[2]->m_fromLinear[bidx], v, isARGB ? 0 : 2);
        storePU<T>(dst[i], v, isARGB ? 255 : 0xffff);
    }
}
#else
static void storePremultiplied(QRgb *dst, const QRgb *src, const QColorVector *buffer, const qsizetype len,
                               const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int a = qAlpha(src[i]);
        const float fa = a / (255.0f * 256.0f);
        const float r = d_ptr->colorSpaceOut->lut[0]->m_fromLinear[int(buffer[i].x * 4080.0f + 0.5f)];
        const float g = d_ptr->colorSpaceOut->lut[1]->m_fromLinear[int(buffer[i].y * 4080.0f + 0.5f)];
        const float b = d_ptr->colorSpaceOut->lut[2]->m_fromLinear[int(buffer[i].z * 4080.0f + 0.5f)];
        dst[i] = qRgba(r * fa + 0.5f, g * fa + 0.5f, b * fa + 0.5f, a);
    }
}

static void storeUnpremultiplied(QRgb *dst, const QRgb *src, const QColorVector *buffer, const qsizetype len,
                                 const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int r = d_ptr->colorSpaceOut->lut[0]->u8FromLinearF32(buffer[i].x);
        const int g = d_ptr->colorSpaceOut->lut[1]->u8FromLinearF32(buffer[i].y);
        const int b = d_ptr->colorSpaceOut->lut[2]->u8FromLinearF32(buffer[i].z);
        dst[i] = (src[i] & 0xff000000) | (r << 16) | (g << 8) | (b << 0);
    }
}

static void storeOpaque(QRgb *dst, const QRgb *src, const QColorVector *buffer, const qsizetype len,
                        const QColorTransformPrivate *d_ptr)
{
    Q_UNUSED(src);
    for (qsizetype i = 0; i < len; ++i) {
        const int r = d_ptr->colorSpaceOut->lut[0]->u8FromLinearF32(buffer[i].x);
        const int g = d_ptr->colorSpaceOut->lut[1]->u8FromLinearF32(buffer[i].y);
        const int b = d_ptr->colorSpaceOut->lut[2]->u8FromLinearF32(buffer[i].z);
        dst[i] = 0xff000000 | (r << 16) | (g << 8) | (b << 0);
    }
}

static void storePremultiplied(QRgba64 *dst, const QRgba64 *src, const QColorVector *buffer, const qsizetype len,
                               const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int a = src[i].alpha();
        const float fa = a / (255.0f * 256.0f);
        const float r = d_ptr->colorSpaceOut->lut[0]->m_fromLinear[int(buffer[i].x * 4080.0f + 0.5f)];
        const float g = d_ptr->colorSpaceOut->lut[1]->m_fromLinear[int(buffer[i].y * 4080.0f + 0.5f)];
        const float b = d_ptr->colorSpaceOut->lut[2]->m_fromLinear[int(buffer[i].z * 4080.0f + 0.5f)];
        dst[i] = qRgba64(r * fa + 0.5f, g * fa + 0.5f, b * fa + 0.5f, a);
    }
}

static void storeUnpremultiplied(QRgba64 *dst, const QRgba64 *src, const QColorVector *buffer, const qsizetype len,
                                 const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
         const int r = d_ptr->colorSpaceOut->lut[0]->u16FromLinearF32(buffer[i].x);
         const int g = d_ptr->colorSpaceOut->lut[1]->u16FromLinearF32(buffer[i].y);
         const int b = d_ptr->colorSpaceOut->lut[2]->u16FromLinearF32(buffer[i].z);
         dst[i] = qRgba64(r, g, b, src[i].alpha());
    }
}

static void storeOpaque(QRgba64 *dst, const QRgba64 *src, const QColorVector *buffer, const qsizetype len,
                        const QColorTransformPrivate *d_ptr)
{
    Q_UNUSED(src);
    for (qsizetype i = 0; i < len; ++i) {
        const int r = d_ptr->colorSpaceOut->lut[0]->u16FromLinearF32(buffer[i].x);
        const int g = d_ptr->colorSpaceOut->lut[1]->u16FromLinearF32(buffer[i].y);
        const int b = d_ptr->colorSpaceOut->lut[2]->u16FromLinearF32(buffer[i].z);
        dst[i] = qRgba64(r, g, b, 0xFFFF);
    }
}
#endif
#if !defined(__SSE2__)
static void storePremultiplied(QRgbaFloat32 *dst, const QRgbaFloat32 *src, const QColorVector *buffer,
                               const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const float a = src[i].a;
        dst[i].r = d_ptr->colorSpaceOut->trc[0].applyInverseExtended(buffer[i].x) * a;
        dst[i].g = d_ptr->colorSpaceOut->trc[1].applyInverseExtended(buffer[i].y) * a;
        dst[i].b = d_ptr->colorSpaceOut->trc[2].applyInverseExtended(buffer[i].z) * a;
        dst[i].a = a;
    }
}

static void storeUnpremultiplied(QRgbaFloat32 *dst, const QRgbaFloat32 *src, const QColorVector *buffer,
                                 const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const float a = src[i].a;
        dst[i].r = d_ptr->colorSpaceOut->trc[0].applyInverseExtended(buffer[i].x);
        dst[i].g = d_ptr->colorSpaceOut->trc[1].applyInverseExtended(buffer[i].y);
        dst[i].b = d_ptr->colorSpaceOut->trc[2].applyInverseExtended(buffer[i].z);
        dst[i].a = a;
    }
}

static void storeOpaque(QRgbaFloat32 *dst, const QRgbaFloat32 *src, const QColorVector *buffer, const qsizetype len,
                        const QColorTransformPrivate *d_ptr)
{
    Q_UNUSED(src);
    for (qsizetype i = 0; i < len; ++i) {
        dst[i].r = d_ptr->colorSpaceOut->trc[0].applyInverseExtended(buffer[i].x);
        dst[i].g = d_ptr->colorSpaceOut->trc[1].applyInverseExtended(buffer[i].y);
        dst[i].b = d_ptr->colorSpaceOut->trc[2].applyInverseExtended(buffer[i].z);
        dst[i].a = 1.0f;
    }
}
#endif
static void storeGray(quint8 *dst, const QRgb *src, const QColorVector *buffer, const qsizetype len,
                      const QColorTransformPrivate *d_ptr)
{
    Q_UNUSED(src);
    for (qsizetype i = 0; i < len; ++i)
        dst[i] = d_ptr->colorSpaceOut->lut[1]->u8FromLinearF32(buffer[i].y);
}

static void storeGray(quint16 *dst, const QRgba64 *src, const QColorVector *buffer, const qsizetype len,
                      const QColorTransformPrivate *d_ptr)
{
    Q_UNUSED(src);
    for (qsizetype i = 0; i < len; ++i)
        dst[i] = d_ptr->colorSpaceOut->lut[1]->u16FromLinearF32(buffer[i].y);
}

static constexpr qsizetype WorkBlockSize = 256;

template <typename T, int Count = 1>
class QUninitialized
{
public:
    operator T*() { return reinterpret_cast<T *>(this); }
private:
    alignas(T) char data[sizeof(T) * Count];
};

template<typename T>
void QColorTransformPrivate::apply(T *dst, const T *src, qsizetype count, TransformFlags flags) const
{
    if (!colorMatrix.isValid())
        return;

    updateLutsIn();
    updateLutsOut();

    bool doApplyMatrix = !colorMatrix.isIdentity();
    constexpr bool DoClip = !std::is_same_v<T, QRgbaFloat16> && !std::is_same_v<T, QRgbaFloat32>;

    QUninitialized<QColorVector, WorkBlockSize> buffer;

    qsizetype i = 0;
    while (i < count) {
        const qsizetype len = qMin(count - i, WorkBlockSize);
        if (flags & InputPremultiplied)
            loadPremultiplied(buffer, src + i, len, this);
        else
            loadUnpremultiplied(buffer, src + i, len, this);

        if (doApplyMatrix)
            applyMatrix<DoClip>(buffer, len, colorMatrix);

        if (flags & InputOpaque)
            storeOpaque(dst + i, src + i, buffer, len, this);
        else if (flags & OutputPremultiplied)
            storePremultiplied(dst + i, src + i, buffer, len, this);
        else
            storeUnpremultiplied(dst + i, src + i, buffer, len, this);

        i += len;
    }
}

template<typename D, typename S>
void QColorTransformPrivate::applyReturnGray(D *dst, const S *src, qsizetype count, TransformFlags flags) const
{
    if (!colorMatrix.isValid())
        return;

    updateLutsIn();
    updateLutsOut();

    QUninitialized<QColorVector, WorkBlockSize> buffer;

    qsizetype i = 0;
    while (i < count) {
        const qsizetype len = qMin(count - i, WorkBlockSize);
        if (flags & InputPremultiplied)
            loadPremultiplied(buffer, src + i, len, this);
        else
            loadUnpremultiplied(buffer, src + i, len, this);

        applyMatrix(buffer, len, colorMatrix);

        storeGray(dst + i, src + i, buffer, len, this);

        i += len;
    }
}

/*!
    \internal
    \enum QColorTransformPrivate::TransformFlag

    Defines how the transform is to be applied.

    \value Unpremultiplied The input and output should both be unpremultiplied.
    \value InputOpaque The input is guaranteed to be opaque.
    \value InputPremultiplied The input is premultiplied.
    \value OutputPremultiplied The output should be premultiplied.
    \value Premultiplied Both input and output should both be premultiplied.
*/

/*!
    \internal
    Prepares a color transformation for fast application. You do not need to
    call this explicitly as it will be called implicitly on the first transforms, but
    if you want predictable performance on the first transforms, you can perform it
    in advance.

    \sa QColorTransform::map(), apply()
*/
void QColorTransformPrivate::prepare()
{
    updateLutsIn();
    updateLutsOut();
}

/*!
    \internal
    Applies the color transformation on \a count QRgb pixels starting from
    \a src and stores the result in \a dst.

    Thread-safe if prepare() has been called first.

    Assumes unpremultiplied data by default. Set \a flags to change defaults.

    \sa prepare()
*/
void QColorTransformPrivate::apply(QRgb *dst, const QRgb *src, qsizetype count, TransformFlags flags) const
{
    apply<QRgb>(dst, src, count, flags);
}

/*!
    \internal
    Applies the color transformation on \a count QRgba64 pixels starting from
    \a src and stores the result in \a dst.

    Thread-safe if prepare() has been called first.

    Assumes unpremultiplied data by default. Set \a flags to change defaults.

    \sa prepare()
*/
void QColorTransformPrivate::apply(QRgba64 *dst, const QRgba64 *src, qsizetype count, TransformFlags flags) const
{
    apply<QRgba64>(dst, src, count, flags);
}

/*!
    \internal
    Applies the color transformation on \a count QRgbaFloat32 pixels starting from
    \a src and stores the result in \a dst.

    Thread-safe if prepare() has been called first.

    Assumes unpremultiplied data by default. Set \a flags to change defaults.

    \sa prepare()
*/
void QColorTransformPrivate::apply(QRgbaFloat32 *dst, const QRgbaFloat32 *src, qsizetype count,
                                   TransformFlags flags) const
{
    apply<QRgbaFloat32>(dst, src, count, flags);
}

/*!
    \internal
    Is to be called on a color-transform to XYZ, returns only luminance values.

*/
void QColorTransformPrivate::apply(quint8 *dst, const QRgb *src, qsizetype count, TransformFlags flags) const
{
    applyReturnGray<quint8, QRgb>(dst, src, count, flags);
}

/*!
    \internal
    Is to be called on a color-transform to XYZ, returns only luminance values.

*/
void QColorTransformPrivate::apply(quint16 *dst, const QRgba64 *src, qsizetype count, TransformFlags flags) const
{
    applyReturnGray<quint16, QRgba64>(dst, src, count, flags);
}


/*!
    \internal
*/
bool QColorTransformPrivate::isIdentity() const
{
    if (!colorMatrix.isIdentity())
        return false;
    if (colorSpaceIn && colorSpaceOut) {
        if (colorSpaceIn->transferFunction != colorSpaceOut->transferFunction)
            return false;
        if (colorSpaceIn->transferFunction == QColorSpace::TransferFunction::Custom) {
            return colorSpaceIn->trc[0] == colorSpaceOut->trc[0]
                && colorSpaceIn->trc[1] == colorSpaceOut->trc[1]
                && colorSpaceIn->trc[2] == colorSpaceOut->trc[2];
        }
    } else {
        if (colorSpaceIn && colorSpaceIn->transferFunction != QColorSpace::TransferFunction::Linear)
            return false;
        if (colorSpaceOut && colorSpaceOut->transferFunction != QColorSpace::TransferFunction::Linear)
            return false;
    }
    return true;
}

QT_END_NAMESPACE
