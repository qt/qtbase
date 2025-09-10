// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcolortransform.h"
#include "qcolortransform_p.h"

#include "qcmyk_p.h"
#include "qcolorclut_p.h"
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
        colorSpaceIn->lut[0] = QColorTrcLut::fromTrc(colorSpaceIn->trc[0]);
        colorSpaceIn->lut[1] = colorSpaceIn->lut[0];
        colorSpaceIn->lut[2] = colorSpaceIn->lut[0];
    } else {
        for (int i = 0; i < 3; ++i)
            colorSpaceIn->lut[i] = QColorTrcLut::fromTrc(colorSpaceIn->trc[i]);
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
        colorSpaceOut->lut[0] = QColorTrcLut::fromTrc(colorSpaceOut->trc[0]);
        colorSpaceOut->lut[1] = colorSpaceOut->lut[0];
        colorSpaceOut->lut[2] = colorSpaceOut->lut[0];
    } else {
        for (int i = 0; i < 3; ++i)
            colorSpaceOut->lut[i] = QColorTrcLut::fromTrc(colorSpaceOut->trc[i]);
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

    To create a QColorTransform, use QColorSpace::transformationToColorSpace():

    \code
    QColorSpace sourceColorSpace(QColorSpace::SRgb);
    QColorSpace targetColorSpace(QColorSpace::DisplayP3);
    QColorTransform srgbToP3Transform = sourceColorSpace.transformationToColorSpace(targetColorSpace);
    \endcode

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
    if (d->colorSpaceIn) {
        if (d->colorSpaceIn->transformModel != other.d->colorSpaceIn->transformModel)
            return false;
        if (d->colorSpaceIn->isThreeComponentMatrix()) {
            for (int i = 0; i < 3; ++i) {
                if (d->colorSpaceIn && d->colorSpaceIn->trc[i] != other.d->colorSpaceIn->trc[i])
                    return false;
            }
        } else {
            if (!d->colorSpaceIn->equals(other.d->colorSpaceIn.constData()))
                return false;
        }
    }
    if (d->colorSpaceOut) {
        if (d->colorSpaceOut->transformModel != other.d->colorSpaceOut->transformModel)
            return false;
        if (d->colorSpaceOut->isThreeComponentMatrix()) {
            for (int i = 0; i < 3; ++i) {
                if (d->colorSpaceOut && d->colorSpaceOut->trc[i] != other.d->colorSpaceOut->trc[i])
                    return false;
            }
        } else {
            if (!d->colorSpaceOut->equals(other.d->colorSpaceOut.constData()))
                return false;
        }
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
    c = d->map(c);
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
    c = d->map(c);
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
    QColorVector c(rgbafp16.r, rgbafp16.g, rgbafp16.b);
    c = d->mapExtended(c);
    rgbafp16.r = qfloat16(c.x);
    rgbafp16.g = qfloat16(c.y);
    rgbafp16.b = qfloat16(c.z);
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
    QColorVector c(rgbafp32.r, rgbafp32.g, rgbafp32.b);
    c = d->mapExtended(c);
    rgbafp32.r = c.x;
    rgbafp32.g = c.y;
    rgbafp32.b = c.z;
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
    if (d->colorSpaceIn->colorModel == QColorSpace::ColorModel::Rgb) {
        if (color.spec() != QColor::ExtendedRgb && color.spec() != QColor::Rgb)
            clr = clr.toRgb();
    } else if (d->colorSpaceIn->colorModel == QColorSpace::ColorModel::Cmyk) {
        if (color.spec() != QColor::Cmyk)
            clr = clr.toCmyk();
    }

    QColorVector c =
            (clr.spec() == QColor::Cmyk)
                    ? QColorVector(clr.cyanF(), clr.magentaF(), clr.yellowF(), clr.blackF())
                    : QColorVector(clr.redF(), clr.greenF(), clr.blueF());

    c = d->mapExtended(c);

    QColor out;
    if (d->colorSpaceOut->colorModel == QColorSpace::ColorModel::Cmyk) {
        c.x = std::clamp(c.x, 0.f, 1.f);
        c.y = std::clamp(c.y, 0.f, 1.f);
        c.z = std::clamp(c.z, 0.f, 1.f);
        c.w = std::clamp(c.w, 0.f, 1.f);
        out.setCmykF(c.x, c.y, c.z, c.w, color.alphaF());
    } else {
        out.setRgbF(c.x, c.y, c.z, color.alphaF());
    }
    return out;
}

// Optimized sub-routines for fast block based conversion:

enum ApplyMatrixForm {
    DoNotClamp = 0,
    DoClamp = 1
};

template<ApplyMatrixForm doClamp = DoClamp>
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
        if (doClamp) {
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
        if (doClamp) {
            cx = vminq_f32(cx, maxV);
            cx = vmaxq_f32(cx, minV);
        }
        vst1q_f32(&buffer[j].x, cx);
    }
#else
    for (qsizetype j = 0; j < len; ++j) {
        const QColorVector cv = colorMatrix.map(buffer[j]);
        if (doClamp) {
            buffer[j].x = std::clamp(cv.x, 0.f, 1.f);
            buffer[j].y = std::clamp(cv.y, 0.f, 1.f);
            buffer[j].z = std::clamp(cv.z, 0.f, 1.f);
        } else {
            buffer[j] = cv;
        }
    }
#endif
}

template<ApplyMatrixForm doClamp = DoClamp>
static void clampIfNeeded(QColorVector *buffer, const qsizetype len)
{
    if constexpr (doClamp != DoClamp)
        return;
#if defined(__SSE2__)
    const __m128 minV = _mm_set1_ps(0.0f);
    const __m128 maxV = _mm_set1_ps(1.0f);
    for (qsizetype j = 0; j < len; ++j) {
        __m128 c = _mm_loadu_ps(&buffer[j].x);
        c = _mm_min_ps(c, maxV);
        c = _mm_max_ps(c, minV);
        _mm_storeu_ps(&buffer[j].x, c);
    }
#elif defined(__ARM_NEON__)
    const float32x4_t minV = vdupq_n_f32(0.0f);
    const float32x4_t maxV = vdupq_n_f32(1.0f);
    for (qsizetype j = 0; j < len; ++j) {
        float32x4_t c = vld1q_f32(&buffer[j].x);
        c = vminq_f32(c, maxV);
        c = vmaxq_f32(c, minV);
        vst1q_f32(&buffer[j].x, c);
    }
#else
    for (qsizetype j = 0; j < len; ++j) {
        const QColorVector cv = buffer[j];
        buffer[j].x = std::clamp(cv.x, 0.f, 1.f);
        buffer[j].y = std::clamp(cv.y, 0.f, 1.f);
        buffer[j].z = std::clamp(cv.z, 0.f, 1.f);
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

template<typename T>
static inline constexpr int getFactor();
template<>
inline constexpr int getFactor<QRgb>()
{   return 255; }
template<>
inline constexpr int getFactor<QRgba64>()
{   return 65535; }
#endif

template<typename T>
static float getAlphaF(const T &);
template<> float getAlphaF(const QRgb &r)
{
    return qAlpha(r) * (1.f / 255.f);
}
template<> float getAlphaF(const QCmyk32 &)
{
    return 1.f;
}
template<> float getAlphaF(const QRgba64 &r)
{
    return r.alpha() * (1.f / 65535.f);
}
template<> float getAlphaF(const QRgbaFloat32 &r)
{
    return r.a;
}

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
    const __m128 vTrcRes = _mm_set1_ps(float(QColorTrcLut::Resolution));
    const __m128 iFF00 = _mm_set1_ps(1.0f / (255 * 256));
    constexpr bool isARGB = isArgb<T>();
    const __m128i vRangeMax = _mm_setr_epi32(isARGB ? d_ptr->colorSpaceIn->lut[2]->m_unclampedToLinear
                                                    : d_ptr->colorSpaceIn->lut[0]->m_unclampedToLinear,
                                             d_ptr->colorSpaceIn->lut[1]->m_unclampedToLinear,
                                             isARGB ? d_ptr->colorSpaceIn->lut[0]->m_unclampedToLinear
                                                    : d_ptr->colorSpaceIn->lut[2]->m_unclampedToLinear,
                                             QColorTrcLut::Resolution);
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
        v = _mm_cvtps_epi32(_mm_mul_ps(vf, vTrcRes));
        const int ridx = isARGB ? _mm_extract_epi16(v, 4) : _mm_extract_epi16(v, 0);
        const int gidx = _mm_extract_epi16(v, 2);
        const int bidx = isARGB ? _mm_extract_epi16(v, 0) : _mm_extract_epi16(v, 4);
        if (_mm_movemask_epi8(_mm_cmpgt_epi32(v, vRangeMax)) == 0) {
            v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx], 0);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx], 2);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx], 4);
            vf = _mm_mul_ps(_mm_cvtepi32_ps(v), iFF00);

            _mm_storeu_ps(&buffer[i].x, vf);
        } else {
            constexpr float f = 1.f / QColorTrcLut::Resolution;
            buffer[i].x = d_ptr->colorSpaceIn->trc[0].applyExtended(ridx * f);
            buffer[i].y = d_ptr->colorSpaceIn->trc[1].applyExtended(gidx * f);
            buffer[i].z = d_ptr->colorSpaceIn->trc[2].applyExtended(bidx * f);
        }
    }
}

template<>
void loadPremultiplied<QRgbaFloat32>(QColorVector *buffer, const QRgbaFloat32 *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    const __m128 vTrcRes = _mm_set1_ps(float(QColorTrcLut::Resolution));
    const __m128 viFF00 = _mm_set1_ps(1.0f / (255 * 256));
    const __m128 vZero = _mm_set1_ps(0.0f);
    const float factor = 1.f / float(QColorTrcLut::Resolution);
    const __m128 vRangeMax = _mm_setr_ps(d_ptr->colorSpaceIn->lut[0]->m_unclampedToLinear * factor,
                                         d_ptr->colorSpaceIn->lut[1]->m_unclampedToLinear * factor,
                                         d_ptr->colorSpaceIn->lut[2]->m_unclampedToLinear * factor,
                                         INFINITY);
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
        const __m128 over = _mm_cmpgt_ps(vf, vRangeMax);
        if (_mm_movemask_ps(_mm_or_ps(under, over)) == 0) {
            // Within gamut
            __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, vTrcRes));
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

// Load to [0->TrcResolution] in 4x32 SIMD
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
    v = _mm_slli_epi32(v, QColorTrcLut::ShiftUp);
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
    v = _mm_srli_epi32(v, QColorTrcLut::ShiftDown);
}

template<typename T>
void loadUnpremultiplied(QColorVector *buffer, const T *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    constexpr bool isARGB = isArgb<T>();
    const __m128 iFF00 = _mm_set1_ps(1.0f / (255 * 256));
    const __m128i vRangeMax = _mm_setr_epi32(isARGB ? d_ptr->colorSpaceIn->lut[2]->m_unclampedToLinear
                                                    : d_ptr->colorSpaceIn->lut[0]->m_unclampedToLinear,
                                             d_ptr->colorSpaceIn->lut[1]->m_unclampedToLinear,
                                             isARGB ? d_ptr->colorSpaceIn->lut[0]->m_unclampedToLinear
                                                    : d_ptr->colorSpaceIn->lut[2]->m_unclampedToLinear,
                                             QColorTrcLut::Resolution);
    for (qsizetype i = 0; i < len; ++i) {
        __m128i v;
        loadPU<T>(src[i], v);
        const int ridx = isARGB ? _mm_extract_epi16(v, 4) : _mm_extract_epi16(v, 0);
        const int gidx = _mm_extract_epi16(v, 2);
        const int bidx = isARGB ? _mm_extract_epi16(v, 0) : _mm_extract_epi16(v, 4);
        if (_mm_movemask_epi8(_mm_cmpgt_epi32(v, vRangeMax)) == 0) {
            v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx], 0);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx], 2);
            v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx], 4);
            __m128 vf = _mm_mul_ps(_mm_cvtepi32_ps(v), iFF00);
            _mm_storeu_ps(&buffer[i].x, vf);
        } else {
            constexpr float f = 1.f / QColorTrcLut::Resolution;
            buffer[i].x = d_ptr->colorSpaceIn->trc[0].applyExtended(ridx * f);
            buffer[i].y = d_ptr->colorSpaceIn->trc[1].applyExtended(gidx * f);
            buffer[i].z = d_ptr->colorSpaceIn->trc[2].applyExtended(bidx * f);
        }
    }
}

template<>
void loadUnpremultiplied<QRgbaFloat32>(QColorVector *buffer, const QRgbaFloat32 *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    const __m128 vTrcRes = _mm_set1_ps(float(QColorTrcLut::Resolution));
    const __m128 iFF00 = _mm_set1_ps(1.0f / (255 * 256));
    const __m128 vZero = _mm_set1_ps(0.0f);
    const float factor = 1.f / float(QColorTrcLut::Resolution);
    const __m128 vRangeMax = _mm_setr_ps(d_ptr->colorSpaceIn->lut[0]->m_unclampedToLinear * factor,
                                         d_ptr->colorSpaceIn->lut[1]->m_unclampedToLinear * factor,
                                         d_ptr->colorSpaceIn->lut[2]->m_unclampedToLinear * factor,
                                         INFINITY);
    for (qsizetype i = 0; i < len; ++i) {
        __m128 vf = _mm_loadu_ps(&src[i].r);
        const __m128 under = _mm_cmplt_ps(vf, vZero);
        const __m128 over = _mm_cmpgt_ps(vf, vRangeMax);
        if (_mm_movemask_ps(_mm_or_ps(under, over)) == 0) {
            // Within gamut
            __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, vTrcRes));
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

static inline bool test_all_zero(uint32x4_t p)
{
#if defined(Q_PROCESSOR_ARM_64)
    return vaddvq_u32(p) == 0;
#else
    const uint32x2_t tmp = vpadd_u32(vget_low_u32(p), vget_high_u32(p));
    return vget_lane_u32(vpadd_u32(tmp, tmp), 0) == 0;
#endif
}

template<typename T>
static void loadPremultiplied(QColorVector *buffer, const T *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    constexpr bool isARGB = isArgb<T>();
    const float iFF00 = 1.0f / (255 * 256);
    const uint32x4_t vRangeMax = qvsetq_n_u32(
            isARGB ? d_ptr->colorSpaceIn->lut[2]->m_unclampedToLinear
                   : d_ptr->colorSpaceIn->lut[0]->m_unclampedToLinear,
            d_ptr->colorSpaceIn->lut[1]->m_unclampedToLinear,
            isARGB ? d_ptr->colorSpaceIn->lut[0]->m_unclampedToLinear
                   : d_ptr->colorSpaceIn->lut[2]->m_unclampedToLinear,
            QColorTrcLut::Resolution);
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
        v = vcvtq_u32_f32(vaddq_f32(vmulq_n_f32(vf, float(QColorTrcLut::Resolution)), vdupq_n_f32(0.5f)));
        const int ridx = isARGB ? vgetq_lane_u32(v, 2) : vgetq_lane_u32(v, 0);
        const int gidx = vgetq_lane_u32(v, 1);
        const int bidx = isARGB ? vgetq_lane_u32(v, 0) : vgetq_lane_u32(v, 2);
        if (test_all_zero(vcgtq_u32(v, vRangeMax))) {
            v = vsetq_lane_u32(d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx], v, 0);
            v = vsetq_lane_u32(d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx], v, 1);
            v = vsetq_lane_u32(d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx], v, 2);
            vf = vmulq_n_f32(vcvtq_f32_u32(v), iFF00);

            vst1q_f32(&buffer[i].x, vf);
        } else {
            constexpr float f = 1.f / QColorTrcLut::Resolution;
            buffer[i].x = d_ptr->colorSpaceIn->trc[0].applyExtended(ridx * f);
            buffer[i].y = d_ptr->colorSpaceIn->trc[1].applyExtended(gidx * f);
            buffer[i].z = d_ptr->colorSpaceIn->trc[2].applyExtended(bidx * f);
        }
    }
}

// Load to [0->TrcResultion] in 4x32 SIMD
template<typename T>
static inline void loadPU(const T &p, uint32x4_t &v);

template<>
inline void loadPU<QRgb>(const QRgb &p, uint32x4_t &v)
{
    v = vmovl_u16(vget_low_u16(vmovl_u8(vreinterpret_u8_u32(vmov_n_u32(p)))));
    v = vshlq_n_u32(v, QColorTrcLut::ShiftUp);
}

template<>
inline void loadPU<QRgba64>(const QRgba64 &p, uint32x4_t &v)
{
    uint16x4_t v16 = vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&p)));
    v16 = vsub_u16(v16, vshr_n_u16(v16, 8));
    v = vmovl_u16(v16);
    v = vshrq_n_u32(v, QColorTrcLut::ShiftDown);
}

template<typename T>
void loadUnpremultiplied(QColorVector *buffer, const T *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    constexpr bool isARGB = isArgb<T>();
    const float iFF00 = 1.0f / (255 * 256);
    const uint32x4_t vRangeMax = qvsetq_n_u32(
            isARGB ? d_ptr->colorSpaceIn->lut[2]->m_unclampedToLinear
                   : d_ptr->colorSpaceIn->lut[0]->m_unclampedToLinear,
            d_ptr->colorSpaceIn->lut[1]->m_unclampedToLinear,
            isARGB ? d_ptr->colorSpaceIn->lut[0]->m_unclampedToLinear
                   : d_ptr->colorSpaceIn->lut[2]->m_unclampedToLinear,
            QColorTrcLut::Resolution);
    for (qsizetype i = 0; i < len; ++i) {
        uint32x4_t v;
        loadPU<T>(src[i], v);
        const int ridx = isARGB ? vgetq_lane_u32(v, 2) : vgetq_lane_u32(v, 0);
        const int gidx = vgetq_lane_u32(v, 1);
        const int bidx = isARGB ? vgetq_lane_u32(v, 0) : vgetq_lane_u32(v, 2);
        if (test_all_zero(vcgtq_u32(v, vRangeMax))) {
            v = vsetq_lane_u32(d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx], v, 0);
            v = vsetq_lane_u32(d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx], v, 1);
            v = vsetq_lane_u32(d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx], v, 2);
            float32x4_t vf = vmulq_n_f32(vcvtq_f32_u32(v), iFF00);
            vst1q_f32(&buffer[i].x, vf);
        } else {
            constexpr float f = 1.f / QColorTrcLut::Resolution;
            buffer[i].x = d_ptr->colorSpaceIn->trc[0].applyExtended(ridx * f);
            buffer[i].y = d_ptr->colorSpaceIn->trc[1].applyExtended(gidx * f);
            buffer[i].z = d_ptr->colorSpaceIn->trc[2].applyExtended(bidx * f);
        }
    }
}
#else
template<>
void loadPremultiplied<QRgb>(QColorVector *buffer, const QRgb *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    const int rangeMaxR = d_ptr->colorSpaceIn->lut[0]->m_unclampedToLinear;
    const int rangeMaxG = d_ptr->colorSpaceIn->lut[1]->m_unclampedToLinear;
    const int rangeMaxB = d_ptr->colorSpaceIn->lut[2]->m_unclampedToLinear;
    for (qsizetype i = 0; i < len; ++i) {
        const uint p = src[i];
        const int a = qAlpha(p);
        if (a) {
            const float ia = float(QColorTrcLut::Resolution) / a;
            const int ridx = int(qRed(p)   * ia + 0.5f);
            const int gidx = int(qGreen(p) * ia + 0.5f);
            const int bidx = int(qBlue(p)  * ia + 0.5f);
            if (ridx <= rangeMaxR && gidx <= rangeMaxG && bidx <= rangeMaxB) {
                buffer[i].x = d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx] * (1.0f / (255 * 256));
                buffer[i].y = d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx] * (1.0f / (255 * 256));
                buffer[i].z = d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx] * (1.0f / (255 * 256));
            } else {
                constexpr float f = 1.f / QColorTrcLut::Resolution;
                buffer[i].x = d_ptr->colorSpaceIn->trc[0].applyExtended(ridx * f);
                buffer[i].y = d_ptr->colorSpaceIn->trc[1].applyExtended(gidx * f);
                buffer[i].z = d_ptr->colorSpaceIn->trc[2].applyExtended(bidx * f);
            }
        } else {
            buffer[i].x = buffer[i].y = buffer[i].z = 0.0f;
        }
    }
}

template<>
void loadPremultiplied<QRgba64>(QColorVector *buffer, const QRgba64 *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    const int rangeMaxR = d_ptr->colorSpaceIn->lut[0]->m_unclampedToLinear;
    const int rangeMaxG = d_ptr->colorSpaceIn->lut[1]->m_unclampedToLinear;
    const int rangeMaxB = d_ptr->colorSpaceIn->lut[2]->m_unclampedToLinear;
    for (qsizetype i = 0; i < len; ++i) {
        const QRgba64 &p = src[i];
        const int a = p.alpha();
        if (a) {
            const float ia = float(QColorTrcLut::Resolution) / a;
            const int ridx = int(p.red()   * ia + 0.5f);
            const int gidx = int(p.green() * ia + 0.5f);
            const int bidx = int(p.blue()  * ia + 0.5f);
            if (ridx <= rangeMaxR && gidx <= rangeMaxG && bidx <= rangeMaxB) {
                buffer[i].x = d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx] * (1.0f / (255 * 256));
                buffer[i].y = d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx] * (1.0f / (255 * 256));
                buffer[i].z = d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx] * (1.0f / (255 * 256));
            } else {
                constexpr float f = 1.f / QColorTrcLut::Resolution;
                buffer[i].x = d_ptr->colorSpaceIn->trc[0].applyExtended(ridx * f);
                buffer[i].y = d_ptr->colorSpaceIn->trc[1].applyExtended(gidx * f);
                buffer[i].z = d_ptr->colorSpaceIn->trc[2].applyExtended(bidx * f);
            }
        } else {
            buffer[i].x = buffer[i].y = buffer[i].z = 0.0f;
        }
    }
}

template<>
void loadUnpremultiplied<QRgb>(QColorVector *buffer, const QRgb *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    const int rangeMaxR = d_ptr->colorSpaceIn->lut[0]->m_unclampedToLinear;
    const int rangeMaxG = d_ptr->colorSpaceIn->lut[1]->m_unclampedToLinear;
    const int rangeMaxB = d_ptr->colorSpaceIn->lut[2]->m_unclampedToLinear;
    for (qsizetype i = 0; i < len; ++i) {
        const uint p = src[i];
        const int ridx = qRed(p)   << QColorTrcLut::ShiftUp;
        const int gidx = qGreen(p) << QColorTrcLut::ShiftUp;
        const int bidx = qBlue(p)  << QColorTrcLut::ShiftUp;
        if (ridx <= rangeMaxR && gidx <= rangeMaxG && bidx <= rangeMaxB) {
            buffer[i].x = d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx] * (1.0f / (255 * 256));
            buffer[i].y = d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx] * (1.0f / (255 * 256));
            buffer[i].z = d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx] * (1.0f / (255 * 256));
        } else {
            constexpr float f = 1.f / QColorTrcLut::Resolution;
            buffer[i].x = d_ptr->colorSpaceIn->trc[0].applyExtended(ridx * f);
            buffer[i].y = d_ptr->colorSpaceIn->trc[1].applyExtended(gidx * f);
            buffer[i].z = d_ptr->colorSpaceIn->trc[2].applyExtended(bidx * f);
        }
    }
}

static int u16toidx(int c)
{
    c -= c >> 8;
    return c >> QColorTrcLut::ShiftDown;
}

template<>
void loadUnpremultiplied<QRgba64>(QColorVector *buffer, const QRgba64 *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    const int rangeMaxR = d_ptr->colorSpaceIn->lut[0]->m_unclampedToLinear;
    const int rangeMaxG = d_ptr->colorSpaceIn->lut[1]->m_unclampedToLinear;
    const int rangeMaxB = d_ptr->colorSpaceIn->lut[2]->m_unclampedToLinear;
    for (qsizetype i = 0; i < len; ++i) {
        const QRgba64 &p = src[i];
        const int ridx = u16toidx(p.red());
        const int gidx = u16toidx(p.green());
        const int bidx = u16toidx(p.blue());
        if (ridx <= rangeMaxR && gidx <= rangeMaxG && bidx <= rangeMaxB) {
            buffer[i].x = d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx] * (1.0f / (255 * 256));
            buffer[i].y = d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx] * (1.0f / (255 * 256));
            buffer[i].z = d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx] * (1.0f / (255 * 256));
        } else {
            constexpr float f = 1.f / QColorTrcLut::Resolution;
            buffer[i].x = d_ptr->colorSpaceIn->trc[0].applyExtended(ridx * f);
            buffer[i].y = d_ptr->colorSpaceIn->trc[1].applyExtended(gidx * f);
            buffer[i].z = d_ptr->colorSpaceIn->trc[2].applyExtended(bidx * f);
        }
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

template<typename D, typename S,
         typename = std::enable_if_t<!std::is_same_v<D, QRgbaFloat32>, void>>
static void storePremultiplied(D *dst, const S *src, const QColorVector *buffer, const qsizetype len,
                               const QColorTransformPrivate *d_ptr)
{
    const __m128 vTrcRes = _mm_set1_ps(float(QColorTrcLut::Resolution));
    const __m128 iFF00 = _mm_set1_ps(1.0f / (255 * 256));
    constexpr bool isARGB = isArgb<D>();
    static_assert(getFactor<D>() >= getFactor<S>());
    for (qsizetype i = 0; i < len; ++i) {
        const int a = getAlpha<S>(src[i]) * (getFactor<D>() / getFactor<S>());
        __m128 vf = _mm_loadu_ps(&buffer[i].x);
        __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, vTrcRes));
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
        storeP<D>(dst[i], v, a);
    }
}

template<typename S>
static void storePremultiplied(QRgbaFloat32 *dst, const S *src,
                               const QColorVector *buffer, const qsizetype len,
                               const QColorTransformPrivate *d_ptr)
{
    const __m128 vTrcRes = _mm_set1_ps(float(QColorTrcLut::Resolution));
    const __m128 vZero = _mm_set1_ps(0.0f);
    const __m128 vOne  = _mm_set1_ps(1.0f);
    const __m128 viFF00 = _mm_set1_ps(1.0f / (255 * 256));
    for (qsizetype i = 0; i < len; ++i) {
        const float a = getAlphaF<S>(src[i]);
        __m128 va = _mm_set1_ps(a);
        __m128 vf = _mm_loadu_ps(&buffer[i].x);
        const __m128 under = _mm_cmplt_ps(vf, vZero);
        const __m128 over = _mm_cmpgt_ps(vf, vOne);
        if (_mm_movemask_ps(_mm_or_ps(under, over)) == 0) {
            // Within gamut
            va = _mm_mul_ps(va, viFF00);
            __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, vTrcRes));
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

template<typename D, typename S,
         typename = std::enable_if_t<!std::is_same_v<D, QRgbaFloat32>, void>>
static void storeUnpremultiplied(D *dst, const S *src, const QColorVector *buffer, const qsizetype len,
                                 const QColorTransformPrivate *d_ptr)
{
    const __m128 vTrcRes = _mm_set1_ps(float(QColorTrcLut::Resolution));
    constexpr bool isARGB = isArgb<D>();
    static_assert(getFactor<D>() >= getFactor<S>());
    for (qsizetype i = 0; i < len; ++i) {
        const int a = getAlpha<S>(src[i]) * (getFactor<D>() / getFactor<S>());
        __m128 vf = _mm_loadu_ps(&buffer[i].x);
        __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, vTrcRes));
        const int ridx = _mm_extract_epi16(v, 0);
        const int gidx = _mm_extract_epi16(v, 2);
        const int bidx = _mm_extract_epi16(v, 4);
        v = _mm_setzero_si128();
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[0]->m_fromLinear[ridx], isARGB ? 2 : 0);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[1]->m_fromLinear[gidx], 1);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[2]->m_fromLinear[bidx], isARGB ? 0 : 2);
        storePU<D>(dst[i], v, a);
    }
}

template<typename S>
void storeUnpremultiplied(QRgbaFloat32 *dst, const S *src,
                          const QColorVector *buffer, const qsizetype len,
                          const QColorTransformPrivate *d_ptr)
{
    const __m128 vTrcRes = _mm_set1_ps(float(QColorTrcLut::Resolution));
    const __m128 vZero = _mm_set1_ps(0.0f);
    const __m128 vOne  = _mm_set1_ps(1.0f);
    const __m128 viFF00 = _mm_set1_ps(1.0f / (255 * 256));
    for (qsizetype i = 0; i < len; ++i) {
        const float a = getAlphaF<S>(src[i]);
        __m128 vf = _mm_loadu_ps(&buffer[i].x);
        const __m128 under = _mm_cmplt_ps(vf, vZero);
        const __m128 over = _mm_cmpgt_ps(vf, vOne);
        if (_mm_movemask_ps(_mm_or_ps(under, over)) == 0) {
            // Within gamut
            __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, vTrcRes));
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
static void storeOpaque(T *dst, const QColorVector *buffer, const qsizetype len,
                        const QColorTransformPrivate *d_ptr)
{
    const __m128 vTrcRes = _mm_set1_ps(float(QColorTrcLut::Resolution));
    constexpr bool isARGB = isArgb<T>();
    for (qsizetype i = 0; i < len; ++i) {
        __m128 vf = _mm_loadu_ps(&buffer[i].x);
        __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, vTrcRes));
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
void storeOpaque(QRgbaFloat32 *dst, const QColorVector *buffer, const qsizetype len,
                 const QColorTransformPrivate *d_ptr)
{
    const __m128 vTrcRes = _mm_set1_ps(float(QColorTrcLut::Resolution));
    const __m128 vZero = _mm_set1_ps(0.0f);
    const __m128 vOne  = _mm_set1_ps(1.0f);
    const __m128 viFF00 = _mm_set1_ps(1.0f / (255 * 256));
    for (qsizetype i = 0; i < len; ++i) {
        __m128 vf = _mm_loadu_ps(&buffer[i].x);
        const __m128 under = _mm_cmplt_ps(vf, vZero);
        const __m128 over = _mm_cmpgt_ps(vf, vOne);
        if (_mm_movemask_ps(_mm_or_ps(under, over)) == 0) {
            // Within gamut
            __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, vTrcRes));
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

template<typename D, typename S,
         typename = std::enable_if_t<!std::is_same_v<D, QRgbaFloat32>, void>>
static void storePremultiplied(D *dst, const S *src, const QColorVector *buffer, const qsizetype len,
                               const QColorTransformPrivate *d_ptr)
{
    const float iFF00 = 1.0f / (255 * 256);
    constexpr bool isARGB = isArgb<D>();
    static_assert(getFactor<D>() >= getFactor<S>());
    for (qsizetype i = 0; i < len; ++i) {
        const int a = getAlpha<S>(src[i]) * (getFactor<D>() / getFactor<S>());
        float32x4_t vf = vld1q_f32(&buffer[i].x);
        uint32x4_t v = vcvtq_u32_f32(vaddq_f32(vmulq_n_f32(vf, float(QColorTrcLut::Resolution)), vdupq_n_f32(0.5f)));
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
        storeP<D>(dst[i], v16);
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

template<typename D, typename S,
         typename = std::enable_if_t<!std::is_same_v<D, QRgbaFloat32>, void>>
static void storeUnpremultiplied(D *dst, const S *src, const QColorVector *buffer, const qsizetype len,
                                 const QColorTransformPrivate *d_ptr)
{
    constexpr bool isARGB = isArgb<D>();
    static_assert(getFactor<D>() >= getFactor<S>());
    for (qsizetype i = 0; i < len; ++i) {
        const int a = getAlpha<S>(src[i]) * (getFactor<D>() / getFactor<S>());
        float32x4_t vf = vld1q_f32(&buffer[i].x);
        uint16x4_t v = vmovn_u32(vcvtq_u32_f32(vaddq_f32(vmulq_n_f32(vf, float(QColorTrcLut::Resolution)), vdupq_n_f32(0.5f))));
        const int ridx = vget_lane_u16(v, 0);
        const int gidx = vget_lane_u16(v, 1);
        const int bidx = vget_lane_u16(v, 2);
        v = vset_lane_u16(d_ptr->colorSpaceOut->lut[0]->m_fromLinear[ridx], v, isARGB ? 2 : 0);
        v = vset_lane_u16(d_ptr->colorSpaceOut->lut[1]->m_fromLinear[gidx], v, 1);
        v = vset_lane_u16(d_ptr->colorSpaceOut->lut[2]->m_fromLinear[bidx], v, isARGB ? 0 : 2);
        storePU<D>(dst[i], v, a);
    }
}

template<typename T>
static void storeOpaque(T *dst, const QColorVector *buffer, const qsizetype len,
                        const QColorTransformPrivate *d_ptr)
{
    constexpr bool isARGB = isArgb<T>();
    for (qsizetype i = 0; i < len; ++i) {
        float32x4_t vf = vld1q_f32(&buffer[i].x);
        uint16x4_t v = vmovn_u32(vcvtq_u32_f32(vaddq_f32(vmulq_n_f32(vf, float(QColorTrcLut::Resolution)), vdupq_n_f32(0.5f))));
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
        const float r = d_ptr->colorSpaceOut->lut[0]->m_fromLinear[int(buffer[i].x * float(QColorTrcLut::Resolution) + 0.5f)];
        const float g = d_ptr->colorSpaceOut->lut[1]->m_fromLinear[int(buffer[i].y * float(QColorTrcLut::Resolution) + 0.5f)];
        const float b = d_ptr->colorSpaceOut->lut[2]->m_fromLinear[int(buffer[i].z * float(QColorTrcLut::Resolution) + 0.5f)];
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

static void storeOpaque(QRgb *dst, const QColorVector *buffer, const qsizetype len,
                        const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int r = d_ptr->colorSpaceOut->lut[0]->u8FromLinearF32(buffer[i].x);
        const int g = d_ptr->colorSpaceOut->lut[1]->u8FromLinearF32(buffer[i].y);
        const int b = d_ptr->colorSpaceOut->lut[2]->u8FromLinearF32(buffer[i].z);
        dst[i] = 0xff000000 | (r << 16) | (g << 8) | (b << 0);
    }
}

template<typename S>
static void storePremultiplied(QRgba64 *dst, const S *src, const QColorVector *buffer, const qsizetype len,
                               const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int a = getAlphaF(src[i]) * 65535.f;
        const float fa = a / (255.0f * 256.0f);
        const float r = d_ptr->colorSpaceOut->lut[0]->m_fromLinear[int(buffer[i].x * float(QColorTrcLut::Resolution) + 0.5f)];
        const float g = d_ptr->colorSpaceOut->lut[1]->m_fromLinear[int(buffer[i].y * float(QColorTrcLut::Resolution) + 0.5f)];
        const float b = d_ptr->colorSpaceOut->lut[2]->m_fromLinear[int(buffer[i].z * float(QColorTrcLut::Resolution) + 0.5f)];
        dst[i] = qRgba64(r * fa + 0.5f, g * fa + 0.5f, b * fa + 0.5f, a);
    }
}

template<typename S>
static void storeUnpremultiplied(QRgba64 *dst, const S *src, const QColorVector *buffer, const qsizetype len,
                                 const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
         const int a = getAlphaF(src[i]) * 65535.f;
         const int r = d_ptr->colorSpaceOut->lut[0]->u16FromLinearF32(buffer[i].x);
         const int g = d_ptr->colorSpaceOut->lut[1]->u16FromLinearF32(buffer[i].y);
         const int b = d_ptr->colorSpaceOut->lut[2]->u16FromLinearF32(buffer[i].z);
         dst[i] = qRgba64(r, g, b, a);
    }
}

static void storeOpaque(QRgba64 *dst, const QColorVector *buffer, const qsizetype len,
                        const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int r = d_ptr->colorSpaceOut->lut[0]->u16FromLinearF32(buffer[i].x);
        const int g = d_ptr->colorSpaceOut->lut[1]->u16FromLinearF32(buffer[i].y);
        const int b = d_ptr->colorSpaceOut->lut[2]->u16FromLinearF32(buffer[i].z);
        dst[i] = qRgba64(r, g, b, 0xFFFF);
    }
}
#endif
#if !defined(__SSE2__)
template<typename S>
static void storePremultiplied(QRgbaFloat32 *dst, const S *src, const QColorVector *buffer,
                               const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const float a = getAlphaF(src[i]);
        dst[i].r = d_ptr->colorSpaceOut->trc[0].applyInverseExtended(buffer[i].x) * a;
        dst[i].g = d_ptr->colorSpaceOut->trc[1].applyInverseExtended(buffer[i].y) * a;
        dst[i].b = d_ptr->colorSpaceOut->trc[2].applyInverseExtended(buffer[i].z) * a;
        dst[i].a = a;
    }
}

template<typename S>
static void storeUnpremultiplied(QRgbaFloat32 *dst, const S *src, const QColorVector *buffer,
                                 const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        const float a = getAlphaF(src[i]);
        dst[i].r = d_ptr->colorSpaceOut->trc[0].applyInverseExtended(buffer[i].x);
        dst[i].g = d_ptr->colorSpaceOut->trc[1].applyInverseExtended(buffer[i].y);
        dst[i].b = d_ptr->colorSpaceOut->trc[2].applyInverseExtended(buffer[i].z);
        dst[i].a = a;
    }
}

static void storeOpaque(QRgbaFloat32 *dst, const QColorVector *buffer, const qsizetype len,
                        const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i) {
        dst[i].r = d_ptr->colorSpaceOut->trc[0].applyInverseExtended(buffer[i].x);
        dst[i].g = d_ptr->colorSpaceOut->trc[1].applyInverseExtended(buffer[i].y);
        dst[i].b = d_ptr->colorSpaceOut->trc[2].applyInverseExtended(buffer[i].z);
        dst[i].a = 1.0f;
    }
}
#endif

static void loadGray(QColorVector *buffer, const quint8 *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    if (d_ptr->colorSpaceIn->colorModel == QColorSpace::ColorModel::Gray ||
        (d_ptr->colorSpaceIn->lut[0] == d_ptr->colorSpaceIn->lut[1] &&
         d_ptr->colorSpaceIn->lut[0] == d_ptr->colorSpaceIn->lut[2])) {
        for (qsizetype i = 0; i < len; ++i) {
            const float y = d_ptr->colorSpaceIn->lut[0]->u8ToLinearF32(src[i]);
            buffer[i] = d_ptr->colorSpaceIn->whitePoint * y;
        }
    } else {
        for (qsizetype i = 0; i < len; ++i) {
            QColorVector v;
            v.x = d_ptr->colorSpaceIn->lut[0]->u8ToLinearF32(src[i]);
            v.y = d_ptr->colorSpaceIn->lut[1]->u8ToLinearF32(src[i]);
            v.z = d_ptr->colorSpaceIn->lut[2]->u8ToLinearF32(src[i]);
            buffer[i] = d_ptr->colorSpaceIn->toXyz.map(v);
        }
    }
}

static void loadGray(QColorVector *buffer, const quint16 *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    if (d_ptr->colorSpaceIn->colorModel == QColorSpace::ColorModel::Gray ||
        (d_ptr->colorSpaceIn->lut[0] == d_ptr->colorSpaceIn->lut[1] &&
         d_ptr->colorSpaceIn->lut[0] == d_ptr->colorSpaceIn->lut[2])) {
        for (qsizetype i = 0; i < len; ++i) {
            const float y = d_ptr->colorSpaceIn->lut[0]->u16ToLinearF32(src[i]);
            buffer[i] = d_ptr->colorSpaceIn->whitePoint * y;
        }
    } else {
        for (qsizetype i = 0; i < len; ++i) {
            QColorVector v;
            v.x = d_ptr->colorSpaceIn->lut[0]->u16ToLinearF32(src[i]);
            v.y = d_ptr->colorSpaceIn->lut[1]->u16ToLinearF32(src[i]);
            v.z = d_ptr->colorSpaceIn->lut[2]->u16ToLinearF32(src[i]);
            buffer[i] = d_ptr->colorSpaceIn->toXyz.map(v);
        }
    }
}

static void storeOpaque(quint8 *dst, const QColorVector *buffer, const qsizetype len,
                      const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i)
        dst[i] = d_ptr->colorSpaceOut->lut[0]->u8FromLinearF32(buffer[i].y);
}

static void storeOpaque(quint16 *dst, const QColorVector *buffer, const qsizetype len,
                      const QColorTransformPrivate *d_ptr)
{
    for (qsizetype i = 0; i < len; ++i)
        dst[i] = d_ptr->colorSpaceOut->lut[0]->u16FromLinearF32(buffer[i].y);
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

void loadUnpremultipliedLUT(QColorVector *buffer, const uchar *src, const qsizetype len)
{
    const float f = 1.0f / 255.f;
    for (qsizetype i = 0; i < len; ++i) {
        const float p = src[i] * f;
        buffer[i].x = p;
        buffer[i].y = p;
        buffer[i].z = p;
    }
}

void loadUnpremultipliedLUT(QColorVector *buffer, const quint16 *src, const qsizetype len)
{
    const float f = 1.0f / 65535.f;
    for (qsizetype i = 0; i < len; ++i) {
        const float p = src[i] * f;
        buffer[i].x = p;
        buffer[i].y = p;
        buffer[i].z = p;
    }
}

void loadUnpremultipliedLUT(QColorVector *buffer, const QRgb *src, const qsizetype len)
{
    const float f = 1.0f / 255.f;
    for (qsizetype i = 0; i < len; ++i) {
        const uint p = src[i];
        buffer[i].x = qRed(p) * f;
        buffer[i].y = qGreen(p) * f;
        buffer[i].z = qBlue(p) * f;
    }
}

void loadUnpremultipliedLUT(QColorVector *buffer, const QCmyk32 *src, const qsizetype len)
{
    const float f = 1.0f / 255.f;
    for (qsizetype i = 0; i < len; ++i) {
        const QCmyk32 p = src[i];
        buffer[i].x = (p.cyan() * f);
        buffer[i].y = (p.magenta() * f);
        buffer[i].z = (p.yellow() * f);
        buffer[i].w = (p.black() * f);
    }
}

void loadUnpremultipliedLUT(QColorVector *buffer, const QRgba64 *src, const qsizetype len)
{
    const float f = 1.0f / 65535.f;
    for (qsizetype i = 0; i < len; ++i) {
        buffer[i].x = src[i].red() * f;
        buffer[i].y = src[i].green() * f;
        buffer[i].z = src[i].blue() * f;
    }
}

void loadUnpremultipliedLUT(QColorVector *buffer, const QRgbaFloat32 *src, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        buffer[i].x = src[i].r;
        buffer[i].y = src[i].g;
        buffer[i].z = src[i].b;
    }
}

void loadPremultipliedLUT(QColorVector *, const uchar *, const qsizetype)
{
    Q_UNREACHABLE();
}

void loadPremultipliedLUT(QColorVector *, const quint16 *, const qsizetype)
{
    Q_UNREACHABLE();
}

void loadPremultipliedLUT(QColorVector *buffer, const QRgb *src, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const uint p = src[i];
        const float f = 1.0f / qAlpha(p);
        buffer[i].x = (qRed(p) * f);
        buffer[i].y = (qGreen(p) * f);
        buffer[i].z = (qBlue(p) * f);
    }
}

void loadPremultipliedLUT(QColorVector *, const QCmyk32 *, const qsizetype)
{
    Q_UNREACHABLE();
}

void loadPremultipliedLUT(QColorVector *buffer, const QRgba64 *src, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const float f = 1.0f / src[i].alpha();
        buffer[i].x = (src[i].red() * f);
        buffer[i].y = (src[i].green() * f);
        buffer[i].z = (src[i].blue() * f);
    }
}

void loadPremultipliedLUT(QColorVector *buffer, const QRgbaFloat32 *src, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const float f = 1.0f / src[i].a;
        buffer[i].x = src[i].r * f;
        buffer[i].y = src[i].g * f;
        buffer[i].z = src[i].b * f;
    }
}
template<typename T>
static void storeUnpremultipliedLUT(QRgb *dst, const T *, const QColorVector *buffer, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int r = buffer[i].x * 255.f;
        const int g = buffer[i].y * 255.f;
        const int b = buffer[i].z * 255.f;
        dst[i] = 0xff000000 | (r << 16) | (g << 8) | (b << 0);
    }
}

template<>
void storeUnpremultipliedLUT(QRgb *dst, const QRgb *src, const QColorVector *buffer, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int r = buffer[i].x * 255.f;
        const int g = buffer[i].y * 255.f;
        const int b = buffer[i].z * 255.f;
        dst[i] = (src[i] & 0xff000000) | (r << 16) | (g << 8) | (b << 0);
    }
}


template<typename T>
void storeUnpremultipliedLUT(QCmyk32 *dst, const T *, const QColorVector *buffer, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int c = buffer[i].x * 255.f;
        const int m = buffer[i].y * 255.f;
        const int y = buffer[i].z * 255.f;
        const int k = buffer[i].w * 255.f;
        dst[i] = QCmyk32(c, m, y, k);
    }
}

template<typename T>
static void storeUnpremultipliedLUT(QRgba64 *dst, const T *,
                                    const QColorVector *buffer, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int r = buffer[i].x * 65535.f;
        const int g = buffer[i].y * 65535.f;
        const int b = buffer[i].z * 65535.f;
        dst[i] = qRgba64(r, g, b, 65535);
    }
}

template<>
void storeUnpremultipliedLUT(QRgba64 *dst, const QRgb *src,
                             const QColorVector *buffer, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int a = qAlpha(src[i]) * 257;
        const int r = buffer[i].x * 65535.f;
        const int g = buffer[i].y * 65535.f;
        const int b = buffer[i].z * 65535.f;
        dst[i] = qRgba64(r, g, b, a);
    }
}

template<>
void storeUnpremultipliedLUT(QRgba64 *dst, const QRgba64 *src,
                             const QColorVector *buffer, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int r = buffer[i].x * 65535.f;
        const int g = buffer[i].y * 65535.f;
        const int b = buffer[i].z * 65535.f;
        dst[i] = qRgba64(r, g, b, src[i].alpha());
    }
}

template<typename T>
static void storeUnpremultipliedLUT(QRgbaFloat32 *dst, const T *src,
                                    const QColorVector *buffer, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const float r = buffer[i].x;
        const float g = buffer[i].y;
        const float b = buffer[i].z;
        dst[i] = QRgbaFloat32{r, g, b, getAlphaF(src[i])};
    }
}

template<typename T>
static void storePremultipliedLUT(QRgb *dst, const T *, const QColorVector *buffer, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int r = buffer[i].x * 255.f;
        const int g = buffer[i].y * 255.f;
        const int b = buffer[i].z * 255.f;
        dst[i] = 0xff000000 | (r << 16) | (g << 8) | (b << 0);
    }
}

template<>
void storePremultipliedLUT(QRgb *dst, const QRgb *src, const QColorVector *buffer, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int a = qAlpha(src[i]);
        const int r = buffer[i].x * a;
        const int g = buffer[i].y * a;
        const int b = buffer[i].z * a;
        dst[i] = (src[i] & 0xff000000) | (r << 16) | (g << 8) | (b << 0);
    }
}

template<typename T>
static void storePremultipliedLUT(QCmyk32 *dst, const T *src, const QColorVector *buffer, const qsizetype len)
{
    storeUnpremultipliedLUT(dst, src, buffer, len);
}

template<typename T>
static void storePremultipliedLUT(QRgba64 *dst, const T *, const QColorVector *buffer, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int r = buffer[i].x * 65535.f;
        const int g = buffer[i].y * 65535.f;
        const int b = buffer[i].z * 65535.f;
        dst[i] = qRgba64(r, g, b, 65535);
    }
}

template<>
void storePremultipliedLUT(QRgba64 *dst, const QRgb *src, const QColorVector *buffer, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int a = qAlpha(src[i]) * 257;
        const int r = buffer[i].x * a;
        const int g = buffer[i].y * a;
        const int b = buffer[i].z * a;
        dst[i] = qRgba64(r, g, b, a);
    }
}

template<>
void storePremultipliedLUT(QRgba64 *dst, const QRgba64 *src, const QColorVector *buffer, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const int a = src[i].alpha();
        const int r = buffer[i].x * a;
        const int g = buffer[i].y * a;
        const int b = buffer[i].z * a;
        dst[i] = qRgba64(r, g, b, a);
    }
}

template<typename T>
static void storePremultipliedLUT(QRgbaFloat32 *dst, const T *src, const QColorVector *buffer, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i) {
        const float a = getAlphaF(src[i]);
        const float r = buffer[i].x * a;
        const float g = buffer[i].y * a;
        const float b = buffer[i].z * a;
        dst[i] = QRgbaFloat32{r, g, b, a};
    }
}

static void visitElement(const QColorSpacePrivate::TransferElement &element, QColorVector *buffer, const qsizetype len)
{
    const bool doW = element.trc[3].isValid();
    for (qsizetype i = 0; i < len; ++i) {
        buffer[i].x = element.trc[0].apply(buffer[i].x);
        buffer[i].y = element.trc[1].apply(buffer[i].y);
        buffer[i].z = element.trc[2].apply(buffer[i].z);
        if (doW)
            buffer[i].w = element.trc[3].apply(buffer[i].w);
    }
}

static void visitElement(const QColorMatrix &element, QColorVector *buffer, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i)
        buffer[i] = element.map(buffer[i]);
}

static void visitElement(const QColorVector &offset, QColorVector *buffer, const qsizetype len)
{
    for (qsizetype i = 0; i < len; ++i)
        buffer[i] += offset;
}

static void visitElement(const QColorCLUT &element, QColorVector *buffer, const qsizetype len)
{
    if (element.isEmpty())
        return;
    for (qsizetype i = 0; i < len; ++i)
        buffer[i] = element.apply(buffer[i]);
}

/*!
    \internal
*/
QColorVector QColorTransformPrivate::map(QColorVector c) const
{
    if (colorSpaceIn->isThreeComponentMatrix()) {
        if (colorSpaceIn->lut.generated.loadAcquire()) {
            c.x = colorSpaceIn->lut[0]->toLinear(c.x);
            c.y = colorSpaceIn->lut[1]->toLinear(c.y);
            c.z = colorSpaceIn->lut[2]->toLinear(c.z);
        } else {
            c.x = colorSpaceIn->trc[0].apply(c.x);
            c.y = colorSpaceIn->trc[1].apply(c.y);
            c.z = colorSpaceIn->trc[2].apply(c.z);
        }
        c = colorMatrix.map(c);
    } else {
        // Do element based conversion
        for (auto &&element : colorSpaceIn->mAB)
            std::visit([&c](auto &&elm) { visitElement(elm, &c, 1); }, element);
    }
    c.x = std::clamp(c.x, 0.0f, 1.0f);
    c.y = std::clamp(c.y, 0.0f, 1.0f);
    c.z = std::clamp(c.z, 0.0f, 1.0f);

    // Match Profile Connection Spaces (PCS):
    if (colorSpaceOut->isPcsLab && !colorSpaceIn->isPcsLab)
        c = c.xyzToLab();
    else if (colorSpaceIn->isPcsLab && !colorSpaceOut->isPcsLab)
        c = c.labToXyz();

    if (colorSpaceOut->isThreeComponentMatrix()) {
        if (!colorSpaceIn->isThreeComponentMatrix()) {
            c = colorMatrix.map(c);
            c.x = std::clamp(c.x, 0.0f, 1.0f);
            c.y = std::clamp(c.y, 0.0f, 1.0f);
            c.z = std::clamp(c.z, 0.0f, 1.0f);
        }
        if (colorSpaceOut->lut.generated.loadAcquire()) {
            c.x = colorSpaceOut->lut[0]->fromLinear(c.x);
            c.y = colorSpaceOut->lut[1]->fromLinear(c.y);
            c.z = colorSpaceOut->lut[2]->fromLinear(c.z);
        } else {
            c.x = colorSpaceOut->trc[0].applyInverse(c.x);
            c.y = colorSpaceOut->trc[1].applyInverse(c.y);
            c.z = colorSpaceOut->trc[2].applyInverse(c.z);
        }
    } else {
        // Do element based conversion
        for (auto &&element : colorSpaceOut->mBA)
            std::visit([&c](auto &&elm) { visitElement(elm, &c, 1); }, element);
        c.x = std::clamp(c.x, 0.0f, 1.0f);
        c.y = std::clamp(c.y, 0.0f, 1.0f);
        c.z = std::clamp(c.z, 0.0f, 1.0f);
    }
    return c;
}

/*!
    \internal
*/
QColorVector QColorTransformPrivate::mapExtended(QColorVector c) const
{
    if (colorSpaceIn->isThreeComponentMatrix()) {
        c.x = colorSpaceIn->trc[0].applyExtended(c.x);
        c.y = colorSpaceIn->trc[1].applyExtended(c.y);
        c.z = colorSpaceIn->trc[2].applyExtended(c.z);
        c = colorMatrix.map(c);
    } else {
        // Do element based conversion
        for (auto &&element : colorSpaceIn->mAB)
            std::visit([&c](auto &&elm) { visitElement(elm, &c, 1); }, element);
    }

    // Match Profile Connection Spaces (PCS):
    if (colorSpaceOut->isPcsLab && !colorSpaceIn->isPcsLab)
        c = c.xyzToLab();
    else if (colorSpaceIn->isPcsLab && !colorSpaceOut->isPcsLab)
        c = c.labToXyz();

    if (colorSpaceOut->isThreeComponentMatrix()) {
        if (!colorSpaceIn->isThreeComponentMatrix())
            c = colorMatrix.map(c);
        c.x = colorSpaceOut->trc[0].applyInverseExtended(c.x);
        c.y = colorSpaceOut->trc[1].applyInverseExtended(c.y);
        c.z = colorSpaceOut->trc[2].applyInverseExtended(c.z);
    } else {
        // Do element based conversion
        for (auto &&element : colorSpaceOut->mBA)
            std::visit([&c](auto &&elm) { visitElement(elm, &c, 1); }, element);
    }
    return c;
}

template<typename T>
constexpr bool IsGrayscale = std::is_same_v<T, uchar> || std::is_same_v<T, quint16>;
template<typename T>
constexpr bool IsAlwaysOpaque = std::is_same_v<T, QCmyk32> || IsGrayscale<T>;
template<typename T>
constexpr bool CanUseThreeComponent = !std::is_same_v<T, QCmyk32>;
template<typename T>
constexpr bool UnclampedValues = std::is_same_v<T, QRgbaFloat16> || std::is_same_v<T, QRgbaFloat32>;

// Possible combos for data and color spaces:
//  DataCM     ColorSpaceCM    ColorSpacePM     Notes
//   Gray       Gray            ThreeMatrix
//   Gray       Rgb             ThreeMatrix     Invalid colorMatrix
//   Rgb        Rgb             ThreeMatrix
//   Rgb        Rgb             ElementProc
//   Gray       Rgb             ElementProc     Only possible for input data
//   Cmyk       Cmyk            ElementProc
//
// Gray data can be uchar, quint16, and is always Opaque
// Rgb data can be QRgb, QRgba64, or QRgbaFloat32, and is Unpremultiplied, Premultiplied, or Opaque
// Cmyk data can be Cmyk32, and is always Opaque
//
// colorMatrix as setup for Gray on Gray or Rgb on Rgb, but not Gray data on Rgb colorspace.

template<typename S>
void QColorTransformPrivate::applyConvertIn(const S *src, QColorVector *buffer, qsizetype len, TransformFlags flags) const
{
    if constexpr (IsGrayscale<S>) {
        if (colorSpaceIn->isThreeComponentMatrix()) {
            loadGray(buffer, src, len, this);
            if (!colorSpaceOut->isThreeComponentMatrix() || colorSpaceIn->colorModel != QColorSpace::ColorModel::Gray) {
                if (!colorSpaceIn->chad.isNull())
                    applyMatrix<DoClamp>(buffer, len, colorSpaceIn->chad);
            }
            return;
        }
    } else if constexpr (CanUseThreeComponent<S>) {
        if (colorSpaceIn->isThreeComponentMatrix()) {
            if (flags & InputPremultiplied)
                loadPremultiplied(buffer, src, len, this);
            else
                loadUnpremultiplied(buffer, src, len, this);

            if (!colorSpaceOut->isThreeComponentMatrix())
                applyMatrix<DoClamp>(buffer, len, colorMatrix);
            return;
        }
    }
    Q_ASSERT(!colorSpaceIn->isThreeComponentMatrix());

    if (flags & InputPremultiplied)
        loadPremultipliedLUT(buffer, src, len);
    else
        loadUnpremultipliedLUT(buffer, src, len);

    // Do element based conversion
    for (auto &&element : colorSpaceIn->mAB)
        std::visit([&buffer, len](auto &&elm) { visitElement(elm, buffer, len); }, element);
}

template<typename D, typename S>
void QColorTransformPrivate::applyConvertOut(D *dst, const S *src, QColorVector *buffer, qsizetype len, TransformFlags flags) const
{
    constexpr ApplyMatrixForm doClamp = UnclampedValues<D> ? DoNotClamp : DoClamp;
    if constexpr (IsGrayscale<D>) {
        Q_UNUSED(src); // dealing with buggy warnings in gcc 9
        Q_UNUSED(flags);
        // Calculate the matrix for grayscale conversion
        QColorMatrix grayMatrix;
        if (colorSpaceIn == colorSpaceOut ||
                (colorSpaceIn->colorModel == QColorSpace::ColorModel::Gray &&
                 colorSpaceOut->colorModel == QColorSpace::ColorModel::Gray)) {
            // colorMatrix already has the right form
            grayMatrix = colorMatrix;
        } else {
            if constexpr (IsGrayscale<S>) {
                if (colorSpaceIn->colorModel == QColorSpace::ColorModel::Gray)
                    grayMatrix = colorSpaceIn->chad;
                else
                    grayMatrix = QColorMatrix::identity(); // Otherwise already handled in applyConvertIn
            } else {
                if (colorSpaceIn->isThreeComponentMatrix())
                    grayMatrix = colorSpaceIn->toXyz;
                else
                    grayMatrix = QColorMatrix::identity();
            }
            if (!colorSpaceOut->chad.isNull())
                grayMatrix = colorSpaceOut->chad.inverted() * grayMatrix;
        }

        applyMatrix<doClamp>(buffer, len, grayMatrix);
        storeOpaque(dst, buffer, len, this);
        return;
    } else if constexpr (CanUseThreeComponent<D>) {
        if (colorSpaceOut->isThreeComponentMatrix()) {
            if (IsGrayscale<S> && colorSpaceIn->colorModel != QColorSpace::ColorModel::Gray)
                applyMatrix<doClamp>(buffer, len, colorSpaceOut->toXyz.inverted()); // colorMatrix wasnt prepared for gray input
            else
                applyMatrix<doClamp>(buffer, len, colorMatrix);

            if constexpr (IsAlwaysOpaque<S>) {
                storeOpaque(dst, buffer, len, this);
            } else {
                if (flags & InputOpaque)
                    storeOpaque(dst, buffer, len, this);
                else if (flags & OutputPremultiplied)
                    storePremultiplied(dst, src, buffer, len, this);
                else
                    storeUnpremultiplied(dst, src, buffer, len, this);
            }
            return;
        }
    }
    if constexpr (!IsGrayscale<D>) {
        Q_ASSERT(!colorSpaceOut->isThreeComponentMatrix());

        // Do element based conversion
        for (auto &&element : colorSpaceOut->mBA)
            std::visit([&buffer, len](auto &&elm) { visitElement(elm, buffer, len); }, element);

        clampIfNeeded<doClamp>(buffer, len);

        if (flags & OutputPremultiplied)
            storePremultipliedLUT(dst, src, buffer, len);
        else
            storeUnpremultipliedLUT(dst, src, buffer, len);
    } else {
        Q_UNREACHABLE();
    }
}

/*!
    \internal
    Adapt Profile Connection Spaces.
*/
void QColorTransformPrivate::pcsAdapt(QColorVector *buffer, qsizetype count) const
{
    // Match Profile Connection Spaces (PCS):
    if (colorSpaceOut->isPcsLab && !colorSpaceIn->isPcsLab) {
        for (qsizetype j = 0; j < count; ++j)
            buffer[j] = buffer[j].xyzToLab();
    } else if (colorSpaceIn->isPcsLab && !colorSpaceOut->isPcsLab) {
        for (qsizetype j = 0; j < count; ++j)
            buffer[j] = buffer[j].labToXyz();
    }
}

/*!
    \internal
    Applies the color transformation on \a count S pixels starting from
    \a src and stores the result in \a dst as D pixels .

    Assumes unpremultiplied data by default. Set \a flags to change defaults.

    \sa prepare()
*/
template<typename D, typename S>
void QColorTransformPrivate::apply(D *dst, const S *src, qsizetype count, TransformFlags flags) const
{
    if (colorSpaceIn->isThreeComponentMatrix())
        updateLutsIn();
    if (colorSpaceOut->isThreeComponentMatrix())
        updateLutsOut();

    Q_DECL_UNINITIALIZED QUninitialized<QColorVector, WorkBlockSize> buffer;
    qsizetype i = 0;
    while (i < count) {
        const qsizetype len = qMin(count - i, WorkBlockSize);

        applyConvertIn(src + i, buffer, len, flags);

        pcsAdapt(buffer, len);

        applyConvertOut(dst + i, src + i, buffer, len, flags);

        i += len;
    }
}

/*!
    \internal
    \enum QColorTransformPrivate::TransformFlag

    Defines how the transform should handle alpha values.

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

// Only some versions increasing precision 14/36 combos
template void QColorTransformPrivate::apply<quint8, quint8>(quint8 *dst, const quint8 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<quint8, QRgb>(quint8 *dst, const QRgb *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<quint8, QCmyk32>(quint8 *dst, const QCmyk32 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<quint16, quint8>(quint16 *dst, const quint8 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<quint16, quint16>(quint16 *dst, const quint16 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<quint16, QCmyk32>(quint16 *dst, const QCmyk32 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<quint16, QRgba64>(quint16 *dst, const QRgba64 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QRgb, quint8>(QRgb *dst, const quint8 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QRgb, QRgb>(QRgb *dst, const QRgb *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QRgb, QCmyk32>(QRgb *dst, const QCmyk32 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QCmyk32, quint8>(QCmyk32 *dst, const quint8 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QCmyk32, quint16>(QCmyk32 *dst, const quint16 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QCmyk32, QRgb>(QCmyk32 *dst, const QRgb *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QCmyk32, QCmyk32>(QCmyk32 *dst, const QCmyk32 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QCmyk32, QRgba64>(QCmyk32 *dst, const QRgba64 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QCmyk32, QRgbaFloat32>(QCmyk32 *dst, const QRgbaFloat32 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QRgba64, quint16>(QRgba64 *dst, const quint16 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QRgba64, QRgb>(QRgba64 *dst, const QRgb *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QRgba64, QCmyk32>(QRgba64 *dst, const QCmyk32 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QRgba64, QRgba64>(QRgba64 *dst, const QRgba64 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QRgbaFloat32, QRgb>(QRgbaFloat32 *dst, const QRgb *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QRgbaFloat32, QCmyk32>(QRgbaFloat32 *dst, const QCmyk32 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QRgbaFloat32, QRgba64>(QRgbaFloat32 *dst, const QRgba64 *src, qsizetype count, TransformFlags flags) const;
template void QColorTransformPrivate::apply<QRgbaFloat32, QRgbaFloat32>(QRgbaFloat32 *dst, const QRgbaFloat32 *src, qsizetype count, TransformFlags flags) const;

/*!
    \internal
*/
bool QColorTransformPrivate::isIdentity() const
{
    if (colorSpaceIn == colorSpaceOut)
        return true;
    if (!colorMatrix.isIdentity())
        return false;
    if (colorSpaceIn && colorSpaceOut) {
        if (colorSpaceIn->equals(colorSpaceOut.constData()))
            return true;
        if (!colorSpaceIn->isThreeComponentMatrix() || !colorSpaceOut->isThreeComponentMatrix())
            return false;
        if (colorSpaceIn->transferFunction != colorSpaceOut->transferFunction)
            return false;
        if (colorSpaceIn->transferFunction == QColorSpace::TransferFunction::Custom) {
            return colorSpaceIn->trc[0] == colorSpaceOut->trc[0]
                && colorSpaceIn->trc[1] == colorSpaceOut->trc[1]
                && colorSpaceIn->trc[2] == colorSpaceOut->trc[2];
        }
    } else {
        if (colorSpaceIn && !colorSpaceIn->isThreeComponentMatrix())
            return false;
        if (colorSpaceOut && !colorSpaceOut->isThreeComponentMatrix())
            return false;
        if (colorSpaceIn && colorSpaceIn->transferFunction != QColorSpace::TransferFunction::Linear)
            return false;
        if (colorSpaceOut && colorSpaceOut->transferFunction != QColorSpace::TransferFunction::Linear)
            return false;
    }
    return true;
}

QT_END_NAMESPACE
