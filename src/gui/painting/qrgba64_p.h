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

#ifndef QRGBA64_P_H
#define QRGBA64_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qrgba64.h>
#include <QtGui/private/qdrawhelper_p.h>
#include <private/qsimd_p.h>

QT_BEGIN_NAMESPACE

inline QRgba64 combineAlpha256(QRgba64 rgba64, uint alpha256)
{
    return QRgba64::fromRgba64(rgba64.red(), rgba64.green(), rgba64.blue(), (rgba64.alpha() * alpha256) >> 8);
}

inline QRgba64 multiplyAlpha256(QRgba64 rgba64, uint alpha256)
{
    return QRgba64::fromRgba64((rgba64.red()   * alpha256) >> 8,
                               (rgba64.green() * alpha256) >> 8,
                               (rgba64.blue()  * alpha256) >> 8,
                               (rgba64.alpha() * alpha256) >> 8);
}

inline QRgba64 multiplyAlpha65535(QRgba64 rgba64, uint alpha65535)
{
    return QRgba64::fromRgba64(qt_div_65535(rgba64.red()   * alpha65535),
                               qt_div_65535(rgba64.green() * alpha65535),
                               qt_div_65535(rgba64.blue()  * alpha65535),
                               qt_div_65535(rgba64.alpha() * alpha65535));
}

#ifdef __SSE2__
Q_ALWAYS_INLINE __m128i multiplyAlpha65535(__m128i rgba64, __m128i va)
{
    __m128i vs = rgba64;
    vs = _mm_unpacklo_epi16(_mm_mullo_epi16(vs, va), _mm_mulhi_epu16(vs, va));
    vs = _mm_add_epi32(vs, _mm_srli_epi32(vs, 16));
    vs = _mm_add_epi32(vs, _mm_set1_epi32(0x8000));
    vs = _mm_srai_epi32(vs, 16);
    vs = _mm_packs_epi32(vs, _mm_setzero_si128());
    return vs;
}
Q_ALWAYS_INLINE __m128i multiplyAlpha65535(__m128i rgba64, uint alpha65535)
{
    const __m128i va = _mm_shufflelo_epi16(_mm_cvtsi32_si128(alpha65535), _MM_SHUFFLE(0, 0, 0, 0));
    return multiplyAlpha65535(rgba64, va);
}
#endif

#if defined(__ARM_NEON__)
Q_ALWAYS_INLINE uint16x4_t multiplyAlpha65535(uint16x4_t rgba64, uint16x4_t alpha65535)
{
    uint32x4_t vs32 = vmull_u16(rgba64, alpha65535); // vs = vs * alpha
    vs32 = vsraq_n_u32(vs32, vs32, 16); // vs = vs + (vs >> 16)
    return vrshrn_n_u32(vs32, 16); // vs = (vs + 0x8000) >> 16
}
Q_ALWAYS_INLINE uint16x4_t multiplyAlpha65535(uint16x4_t rgba64, uint alpha65535)
{
    uint32x4_t vs32 = vmull_n_u16(rgba64, alpha65535); // vs = vs * alpha
    vs32 = vsraq_n_u32(vs32, vs32, 16); // vs = vs + (vs >> 16)
    return vrshrn_n_u32(vs32, 16); // vs = (vs + 0x8000) >> 16
}
#endif

template<typename T>
inline T multiplyAlpha255(T rgba64, uint alpha255)
{
#if defined(__SSE2__) || defined(__ARM_NEON__)
    return multiplyAlpha65535(rgba64, alpha255 * 257);
#else
    return QRgba64::fromRgba64(qt_div_255(rgba64.red()   * alpha255),
                               qt_div_255(rgba64.green() * alpha255),
                               qt_div_255(rgba64.blue()  * alpha255),
                               qt_div_255(rgba64.alpha() * alpha255));
#endif
}

inline QRgba64 interpolate256(QRgba64 x, uint alpha1, QRgba64 y, uint alpha2)
{
    return QRgba64::fromRgba64(multiplyAlpha256(x, alpha1) + multiplyAlpha256(y, alpha2));
}

inline QRgba64 interpolate255(QRgba64 x, uint alpha1, QRgba64 y, uint alpha2)
{
    return QRgba64::fromRgba64(multiplyAlpha255(x, alpha1) + multiplyAlpha255(y, alpha2));
}

#if defined __SSE2__
Q_ALWAYS_INLINE __m128i interpolate255(__m128i x, uint alpha1, __m128i y, uint alpha2)
{
    return _mm_add_epi32(multiplyAlpha255(x, alpha1), multiplyAlpha255(y, alpha2));
}
#endif

#if defined __ARM_NEON__
Q_ALWAYS_INLINE uint16x4_t interpolate255(uint16x4_t x, uint alpha1, uint16x4_t y, uint alpha2)
{
    return vadd_u16(multiplyAlpha255(x, alpha1), multiplyAlpha255(y, alpha2));
}
#endif

inline QRgba64 interpolate65535(QRgba64 x, uint alpha1, QRgba64 y, uint alpha2)
{
    return QRgba64::fromRgba64(multiplyAlpha65535(x, alpha1) + multiplyAlpha65535(y, alpha2));
}

#if defined __SSE2__
Q_ALWAYS_INLINE __m128i interpolate65535(__m128i x, uint alpha1, __m128i y, uint alpha2)
{
    return _mm_add_epi32(multiplyAlpha65535(x, alpha1), multiplyAlpha65535(y, alpha2));
}
// alpha2 below is const-ref because otherwise MSVC2013 complains that it can't 16-byte align the argument.
Q_ALWAYS_INLINE __m128i interpolate65535(__m128i x, __m128i alpha1, __m128i y, const __m128i &alpha2)
{
    return _mm_add_epi32(multiplyAlpha65535(x, alpha1), multiplyAlpha65535(y, alpha2));
}
#endif

#if defined __ARM_NEON__
Q_ALWAYS_INLINE uint16x4_t interpolate65535(uint16x4_t x, uint alpha1, uint16x4_t y, uint alpha2)
{
    return vadd_u16(multiplyAlpha65535(x, alpha1), multiplyAlpha65535(y, alpha2));
}
Q_ALWAYS_INLINE uint16x4_t interpolate65535(uint16x4_t x, uint16x4_t alpha1, uint16x4_t y, uint16x4_t alpha2)
{
    return vadd_u16(multiplyAlpha65535(x, alpha1), multiplyAlpha65535(y, alpha2));
}
#endif

inline QRgba64 addWithSaturation(QRgba64 a, QRgba64 b)
{
    return QRgba64::fromRgba64(qMin(a.red() + b.red(), 65535),
                               qMin(a.green() + b.green(), 65535),
                               qMin(a.blue() + b.blue(), 65535),
                               qMin(a.alpha() + b.alpha(), 65535));
}

#if defined(__SSE2__)
Q_ALWAYS_INLINE __m128i addWithSaturation(__m128i a, __m128i b)
{
    return _mm_adds_epu16(a, b);
}
#endif

#if defined(__ARM_NEON__)
Q_ALWAYS_INLINE uint16x4_t addWithSaturation(uint16x4_t a, uint16x4_t b)
{
    return vqmovn_u32(vaddl_u16(a, b));
}
#endif

QT_END_NAMESPACE

#endif // QRGBA64_P_H
