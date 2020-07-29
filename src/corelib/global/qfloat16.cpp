/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Copyright (C) 2016 by Southwest Research Institute (R)
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qfloat16.h"
#include "private/qsimd_p.h"
#include <cmath> // for fpclassify()'s return values

QT_BEGIN_NAMESPACE

/*!
    \class qfloat16
    \keyword 16-bit Floating Point Support
    \ingroup funclists
    \inmodule QtCore
    \inheaderfile QFloat16
    \brief Provides 16-bit floating point support.

    The \c qfloat16 class provides support for half-precision (16-bit) floating
    point data.  It is fully compliant with IEEE 754 as a storage type.  This
    implies that any arithmetic operation on a \c qfloat16 instance results in
    the value first being converted to a \c float.  This conversion to and from
    \c float is performed by hardware when possible, but on processors that do
    not natively support half-precision, the conversion is performed through a
    sequence of lookup table operations.

    \c qfloat16 should be treated as if it were a POD (plain old data) type.
    Consequently, none of the supported operations need any elaboration beyond
    stating that it supports all arithmetic operators incident to floating point
    types.

    \note On x86 and x86-64 that to get hardware accelerated conversions you must
    compile with F16C or AVX2 enabled, or use qFloatToFloat16() and qFloatFromFloat16()
    which will detect F16C at runtime.

    \since 5.9
*/

/*!
    \macro QT_NO_FLOAT16_OPERATORS
    \relates qfloat16
    \since 5.12.4

    Defining this macro disables the arithmetic operators for qfloat16.

    This is only necessary on Visual Studio 2017 (and earlier) when including
    \c {<QFloat16>} and \c{<bitset>} in the same translation unit, which would
    otherwise cause a compilation error due to a toolchain bug (see
    [QTBUG-72073]).
*/

/*!
    \fn bool qIsInf(qfloat16 f)
    \relates qfloat16

    Returns true if the \c qfloat16 \a {f} is equivalent to infinity.

    \sa qIsInf
*/

/*!
    \fn bool qIsNaN(qfloat16 f)
    \relates qfloat16

    Returns true if the \c qfloat16 \a {f} is not a number (NaN).

    \sa qIsNaN
*/

/*!
    \fn bool qIsFinite(qfloat16 f)
    \relates qfloat16

    Returns true if the \c qfloat16 \a {f} is a finite number.

    \sa qIsFinite
*/

/*!
    \internal
    \since 5.14
    \fn bool qfloat16::isInf() const noexcept

    Tests whether this \c qfloat16 value is an infinity.

    \sa qIsInf()
*/

/*!
    \internal
    \since 5.14
    \fn bool qfloat16::isNaN() const noexcept

    Tests whether this \c qfloat16 value is "not a number".

    \sa qIsNaN()
*/

/*!
    \since 5.14
    \fn bool qfloat16::isNormal() const noexcept

    Returns \c true if this \c qfloat16 value is finite and in normal form.

    \sa qFpClassify()
*/

/*!
    \internal
    \since 5.14
    \fn bool qfloat16::isFinite() const noexcept

    Tests whether this \c qfloat16 value is finite.

    \sa qIsFinite()
*/

/*!
    \since 5.15
    \fn qfloat16 qfloat16::copySign(qfloat16 sign) const noexcept

    Returns a qfloat16 with the sign of \a sign but the rest of its value taken
    from this qfloat16. Serves as qfloat16's equivalent of std::copysign().
*/

/*!
    \internal
    \since 5.14
    Implements qFpClassify() for qfloat16.

    \sa qFpClassify()
*/
int qfloat16::fpClassify() const noexcept
{
    return isInf() ? FP_INFINITE : isNaN() ? FP_NAN
        : !(b16 & 0x7fff) ? FP_ZERO : isNormal() ? FP_NORMAL : FP_SUBNORMAL;
}

/*! \fn int qRound(qfloat16 value)
    \relates qfloat16

    Rounds \a value to the nearest integer.

    \sa qRound
*/

/*! \fn qint64 qRound64(qfloat16 value)
    \relates qfloat16

    Rounds \a value to the nearest 64-bit integer.

    \sa qRound64
*/

/*! \fn bool qFuzzyCompare(qfloat16 p1, qfloat16 p2)
    \relates qfloat16

    Compares the floating point value \a p1 and \a p2 and
    returns \c true if they are considered equal, otherwise \c false.

    The two numbers are compared in a relative way, where the
    exactness is stronger the smaller the numbers are.
 */

#if QT_COMPILER_SUPPORTS(F16C)
static inline bool hasFastF16()
{
    // All processors with F16C also support AVX, but YMM registers
    // might not be supported by the OS, or they might be disabled.
    return qCpuHasFeature(F16C) && qCpuHasFeature(AVX);
}

extern "C" {
#ifdef QFLOAT16_INCLUDE_FAST
#  define f16cextern    static
#else
#  define f16cextern    extern
#endif

f16cextern void qFloatToFloat16_fast(quint16 *out, const float *in, qsizetype len) noexcept;
f16cextern void qFloatFromFloat16_fast(float *out, const quint16 *in, qsizetype len) noexcept;

#undef f16cextern
}

#elif defined(__ARM_FP16_FORMAT_IEEE) && defined(__ARM_NEON__) && (__ARM_FP & 2)
static inline bool hasFastF16()
{
    return true;
}

static void qFloatToFloat16_fast(quint16 *out, const float *in, qsizetype len) noexcept
{
    __fp16 *out_f16 = reinterpret_cast<__fp16 *>(out);
    qsizetype i = 0;
    for (; i < len - 3; i += 4)
        vst1_f16(out_f16 + i, vcvt_f16_f32(vld1q_f32(in + i)));
    SIMD_EPILOGUE(i, len, 3)
        out_f16[i] = __fp16(in[i]);
}

static void qFloatFromFloat16_fast(float *out, const quint16 *in, qsizetype len) noexcept
{
    const __fp16 *in_f16 = reinterpret_cast<const __fp16 *>(in);
    qsizetype i = 0;
    for (; i < len - 3; i += 4)
        vst1q_f32(out + i, vcvt_f32_f16(vld1_f16(in_f16 + i)));
    SIMD_EPILOGUE(i, len, 3)
        out[i] = float(in_f16[i]);
}
#else
static inline bool hasFastF16()
{
    return false;
}

static void qFloatToFloat16_fast(quint16 *, const float *, qsizetype) noexcept
{
    Q_UNREACHABLE();
}

static void qFloatFromFloat16_fast(float *, const quint16 *, qsizetype) noexcept
{
    Q_UNREACHABLE();
}
#endif
/*!
    \since 5.11
    \relates qfloat16

    Converts \a len floats from \a in to qfloat16 and stores them in \a out.
    Both \a in and \a out must have \a len allocated entries.

    This function is faster than converting values one by one, and will do runtime
    F16C detection on x86 and x86-64 hardware.
*/
Q_CORE_EXPORT void qFloatToFloat16(qfloat16 *out, const float *in, qsizetype len) noexcept
{
    if (hasFastF16())
        return qFloatToFloat16_fast(reinterpret_cast<quint16 *>(out), in, len);

    for (qsizetype i = 0; i < len; ++i)
        out[i] = qfloat16(in[i]);
}

/*!
    \since 5.11
    \relates qfloat16

    Converts \a len qfloat16 from \a in to floats and stores them in \a out.
    Both \a in and \a out must have \a len allocated entries.

    This function is faster than converting values one by one, and will do runtime
    F16C detection on x86 and x86-64 hardware.
*/
Q_CORE_EXPORT void qFloatFromFloat16(float *out, const qfloat16 *in, qsizetype len) noexcept
{
    if (hasFastF16())
        return qFloatFromFloat16_fast(out, reinterpret_cast<const quint16 *>(in), len);

    for (qsizetype i = 0; i < len; ++i)
        out[i] = float(in[i]);
}

QT_END_NAMESPACE

#include "qfloat16tables.cpp"
#ifdef QFLOAT16_INCLUDE_FAST
#  include "qfloat16_f16c.c"
#endif
