/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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


#include "qcolortransform.h"
#include "qcolortransform_p.h"

#include "qcolormatrix_p.h"
#include "qcolorspace_p.h"
#include "qcolortrc_p.h"
#include "qcolortrclut_p.h"

#include <QtCore/qatomic.h>
#include <QtCore/qmath.h>
#include <QtGui/qcolor.h>
#include <QtGui/qtransform.h>
#include <QtCore/private/qsimd_p.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

QColorTrcLut *lutFromTrc(const QColorTrc &trc)
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
        colorSpaceIn->lut[0].reset(lutFromTrc(colorSpaceIn->trc[0]));
        colorSpaceIn->lut[1] = colorSpaceIn->lut[0];
        colorSpaceIn->lut[2] = colorSpaceIn->lut[0];
    } else {
        for (int i = 0; i < 3; ++i)
            colorSpaceIn->lut[i].reset(lutFromTrc(colorSpaceIn->trc[i]));
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
        colorSpaceOut->lut[0].reset(lutFromTrc(colorSpaceOut->trc[0]));
        colorSpaceOut->lut[1] = colorSpaceOut->lut[0];
        colorSpaceOut->lut[2] = colorSpaceOut->lut[0];
    } else {
        for (int i = 0; i < 3; ++i)
            colorSpaceOut->lut[i].reset(lutFromTrc(colorSpaceOut->trc[i]));
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


QColorTransform::QColorTransform(const QColorTransform &colorTransform) noexcept
    : d(colorTransform.d)
{
    if (d)
        d->ref.ref();
}


QColorTransform::~QColorTransform()
{
    if (d && !d->ref.deref())
        delete d;
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
    c.x = d->colorSpaceIn->trc[0].apply(c.x);
    c.y = d->colorSpaceIn->trc[1].apply(c.y);
    c.z = d->colorSpaceIn->trc[2].apply(c.z);
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
    c.x = d->colorSpaceIn->trc[0].apply(c.x);
    c.y = d->colorSpaceIn->trc[1].apply(c.y);
    c.z = d->colorSpaceIn->trc[2].apply(c.z);
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

    return QRgba64::fromRgba64(c.x * 65535, c.y * 65535, c.z * 65535, rgba64.alpha());
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
        cx = _mm_min_ps(cx, maxV);
        cx = _mm_max_ps(cx, minV);
        _mm_storeu_ps(&buffer[j].x, cx);
    }
#else
    for (int j = 0; j < len; ++j) {
        const QColorVector cv = colorMatrix.map(buffer[j]);
        buffer[j].x = std::max(0.0f, std::min(1.0f, cv.x));
        buffer[j].y = std::max(0.0f, std::min(1.0f, cv.y));
        buffer[j].z = std::max(0.0f, std::min(1.0f, cv.z));
    }
#endif
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
    // Shuffle to ARGB as the template below expects it
    v = _mm_shuffle_epi32(v, _MM_SHUFFLE(3, 0, 1, 2));
}

template<typename T>
static void loadPremultiplied(QColorVector *buffer, const T *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    const __m128 v4080 = _mm_set1_ps(4080.f);
    const __m128 iFF00 = _mm_set1_ps(1.0f / (255 * 256));
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
        const int ridx = _mm_extract_epi16(v, 4);
        const int gidx = _mm_extract_epi16(v, 2);
        const int bidx = _mm_extract_epi16(v, 0);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx], 0);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx], 2);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx], 4);
        vf = _mm_mul_ps(_mm_cvtepi32_ps(v), iFF00);

        _mm_storeu_ps(&buffer[i].x, vf);
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
    // Shuffle to ARGB as the template below expects it
    v = _mm_shuffle_epi32(v, _MM_SHUFFLE(3, 0, 1, 2));
}

template<typename T>
void loadUnpremultiplied(QColorVector *buffer, const T *src, const qsizetype len, const QColorTransformPrivate *d_ptr)
{
    const __m128 iFF00 = _mm_set1_ps(1.0f / (255 * 256));
    for (qsizetype i = 0; i < len; ++i) {
        __m128i v;
        loadPU<T>(src[i], v);
        const int ridx = _mm_extract_epi16(v, 4);
        const int gidx = _mm_extract_epi16(v, 2);
        const int bidx = _mm_extract_epi16(v, 0);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[0]->m_toLinear[ridx], 0);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[1]->m_toLinear[gidx], 2);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceIn->lut[2]->m_toLinear[bidx], 4);
        __m128 vf = _mm_mul_ps(_mm_cvtepi32_ps(v), iFF00);
        _mm_storeu_ps(&buffer[i].x, vf);
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

static void storePremultiplied(QRgb *dst, const QRgb *src, const QColorVector *buffer, const qsizetype len,
                               const QColorTransformPrivate *d_ptr)
{
#if defined(__SSE2__)
    const __m128 v4080 = _mm_set1_ps(4080.f);
    const __m128 iFF00 = _mm_set1_ps(1.0f / (255 * 256));
    for (qsizetype i = 0; i < len; ++i) {
        const int a = qAlpha(src[i]);
        __m128 vf = _mm_loadu_ps(&buffer[i].x);
        __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, v4080));
        __m128 va = _mm_set1_ps(a);
        va = _mm_mul_ps(va, iFF00);
        const int ridx = _mm_extract_epi16(v, 0);
        const int gidx = _mm_extract_epi16(v, 2);
        const int bidx = _mm_extract_epi16(v, 4);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[0]->m_fromLinear[ridx], 4);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[1]->m_fromLinear[gidx], 2);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[2]->m_fromLinear[bidx], 0);
        vf = _mm_cvtepi32_ps(v);
        vf = _mm_mul_ps(vf, va);
        v = _mm_cvtps_epi32(vf);
        v = _mm_packs_epi32(v, v);
        v = _mm_insert_epi16(v, a, 3);
        v = _mm_packus_epi16(v, v);
        dst[i] = _mm_cvtsi128_si32(v);
    }
#else
    for (qsizetype i = 0; i < len; ++i) {
        const int a = qAlpha(src[i]);
        const float fa = a / (255.0f * 256.0f);
        const float r = d_ptr->colorSpaceOut->lut[0]->m_fromLinear[int(buffer[i].x * 4080.0f + 0.5f)];
        const float g = d_ptr->colorSpaceOut->lut[1]->m_fromLinear[int(buffer[i].y * 4080.0f + 0.5f)];
        const float b = d_ptr->colorSpaceOut->lut[2]->m_fromLinear[int(buffer[i].z * 4080.0f + 0.5f)];
        dst[i] = qRgba(r * fa + 0.5f, g * fa + 0.5f, b * fa + 0.5f, a);
    }
#endif
}

static void storeUnpremultiplied(QRgb *dst, const QRgb *src, const QColorVector *buffer, const qsizetype len,
                                 const QColorTransformPrivate *d_ptr)
{
#if defined(__SSE2__)
    const __m128 v4080 = _mm_set1_ps(4080.f);
    for (qsizetype i = 0; i < len; ++i) {
        const int a = qAlpha(src[i]);
        __m128 vf = _mm_loadu_ps(&buffer[i].x);
        __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, v4080));
        const int ridx = _mm_extract_epi16(v, 0);
        const int gidx = _mm_extract_epi16(v, 2);
        const int bidx = _mm_extract_epi16(v, 4);
        v = _mm_setzero_si128();
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[0]->m_fromLinear[ridx], 2);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[1]->m_fromLinear[gidx], 1);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[2]->m_fromLinear[bidx], 0);
        v = _mm_add_epi16(v, _mm_set1_epi16(0x80));
        v = _mm_srli_epi16(v, 8);
        v = _mm_insert_epi16(v, a, 3);
        v = _mm_packus_epi16(v, v);
        dst[i] = _mm_cvtsi128_si32(v);
    }
#else
    for (qsizetype i = 0; i < len; ++i) {
        const int r = d_ptr->colorSpaceOut->lut[0]->u8FromLinearF32(buffer[i].x);
        const int g = d_ptr->colorSpaceOut->lut[1]->u8FromLinearF32(buffer[i].y);
        const int b = d_ptr->colorSpaceOut->lut[2]->u8FromLinearF32(buffer[i].z);
        dst[i] = (src[i] & 0xff000000) | (r << 16) | (g << 8) | (b << 0);
    }
#endif
}

static void storeOpaque(QRgb *dst, const QRgb *src, const QColorVector *buffer, const qsizetype len,
                        const QColorTransformPrivate *d_ptr)
{
    Q_UNUSED(src);
#if defined(__SSE2__)
    const __m128 v4080 = _mm_set1_ps(4080.f);
    for (qsizetype i = 0; i < len; ++i) {
        __m128 vf = _mm_loadu_ps(&buffer[i].x);
        __m128i v = _mm_cvtps_epi32(_mm_mul_ps(vf, v4080));
        const int ridx = _mm_extract_epi16(v, 0);
        const int gidx = _mm_extract_epi16(v, 2);
        const int bidx = _mm_extract_epi16(v, 4);
        v = _mm_setzero_si128();
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[0]->m_fromLinear[ridx], 2);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[1]->m_fromLinear[gidx], 1);
        v = _mm_insert_epi16(v, d_ptr->colorSpaceOut->lut[2]->m_fromLinear[bidx], 0);
        v = _mm_add_epi16(v, _mm_set1_epi16(0x80));
        v = _mm_srli_epi16(v, 8);
        v = _mm_insert_epi16(v, 255, 3);
        v = _mm_packus_epi16(v, v);
        dst[i] = _mm_cvtsi128_si32(v);
    }
#else
    for (qsizetype i = 0; i < len; ++i) {
        const int r = d_ptr->colorSpaceOut->lut[0]->u8FromLinearF32(buffer[i].x);
        const int g = d_ptr->colorSpaceOut->lut[1]->u8FromLinearF32(buffer[i].y);
        const int b = d_ptr->colorSpaceOut->lut[2]->u8FromLinearF32(buffer[i].z);
        dst[i] = 0xff000000 | (r << 16) | (g << 8) | (b << 0);
    }
#endif
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

    bool doApplyMatrix = (colorMatrix != QColorMatrix::identity());

    QUninitialized<QColorVector, WorkBlockSize> buffer;

    qsizetype i = 0;
    while (i < count) {
        const qsizetype len = qMin(count - i, WorkBlockSize);
        if (flags & InputPremultiplied)
            loadPremultiplied(buffer, src + i, len, this);
        else
            loadUnpremultiplied(buffer, src + i, len, this);

        if (doApplyMatrix)
            applyMatrix(buffer, len, colorMatrix);

        if (flags & InputOpaque)
            storeOpaque(dst + i, src + i, buffer, len, this);
        else if (flags & OutputPremultiplied)
            storePremultiplied(dst + i, src + i, buffer, len, this);
        else
            storeUnpremultiplied(dst + i, src + i, buffer, len, this);

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


QT_END_NAMESPACE
