// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2016 by Southwest Research Institute (R)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfloat16.h"
#include "private/qsimd_p.h"
#include <cmath> // for fpclassify()'s return values

#include <QtCore/qdatastream.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qtextstream.h>

QT_DECL_METATYPE_EXTERN(qfloat16, Q_CORE_EXPORT)
QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(qfloat16)

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
    \fn qfloat16::qfloat16(Qt::Initialization)
    \since 6.1

    Constructs a qfloat16 without initializing the value.
*/

/*!
    \fn bool qIsInf(qfloat16 f)
    \relates qfloat16
    \overload qIsInf(float)

    Returns true if the \c qfloat16 \a {f} is equivalent to infinity.
*/

/*!
    \fn bool qIsNaN(qfloat16 f)
    \relates qfloat16
    \overload qIsNaN(float)

    Returns true if the \c qfloat16 \a {f} is not a number (NaN).
*/

/*!
    \fn bool qIsFinite(qfloat16 f)
    \relates qfloat16
    \overload qIsFinite(float)

    Returns true if the \c qfloat16 \a {f} is a finite number.
*/

/*!
    \internal
    \since 5.14
    \fn bool qfloat16::isInf() const noexcept

    Tests whether this \c qfloat16 value is an infinity.
*/

/*!
    \internal
    \since 5.14
    \fn bool qfloat16::isNaN() const noexcept

    Tests whether this \c qfloat16 value is "not a number".
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
*/

/*!
    \since 5.15
    \fn qfloat16 qfloat16::copySign(qfloat16 sign) const noexcept

    Returns a qfloat16 with the sign of \a sign but the rest of its value taken
    from this qfloat16. Serves as qfloat16's equivalent of std::copysign().
*/

/*!
    \fn int qFpClassify(qfloat16 val)
    \relates qfloat16
    \since 5.14
    \overload qFpClassify(float)

    Returns the floating-point class of \a val.
*/

/*!
    \internal
    \since 5.14
    Implements qFpClassify() for qfloat16.
*/
int qfloat16::fpClassify() const noexcept
{
    return isInf() ? FP_INFINITE : isNaN() ? FP_NAN
        : !(b16 & 0x7fff) ? FP_ZERO : isNormal() ? FP_NORMAL : FP_SUBNORMAL;
}

/*! \fn int qRound(qfloat16 value)
    \relates qfloat16
    \overload qRound(float)

    Rounds \a value to the nearest integer.
*/

/*! \fn qint64 qRound64(qfloat16 value)
    \relates qfloat16
    \overload qRound64(float)

    Rounds \a value to the nearest 64-bit integer.
*/

/*! \fn bool qFuzzyCompare(qfloat16 p1, qfloat16 p2)
    \relates qfloat16
    \overload qFuzzyCompare(float, float)

    Compares the floating point value \a p1 and \a p2 and
    returns \c true if they are considered equal, otherwise \c false.

    The two numbers are compared in a relative way, where the
    exactness is stronger the smaller the numbers are.
 */

#if QT_COMPILER_SUPPORTS_HERE(F16C)
static inline bool hasFastF16()
{
    // qsimd.cpp:detectProcessorFeatures() turns off this feature if AVX
    // state-saving is not enabled by the OS
    return qCpuHasFeature(F16C);
}

#if QT_COMPILER_SUPPORTS_HERE(AVX512VL) && QT_COMPILER_SUPPORTS_HERE(AVX512BW)
static bool hasFastF16Avx256()
{
    // 256-bit AVX512 don't have a performance penalty (see qstring.cpp for more info)
    return qCpuHasFeature(ArchSkylakeAvx512);
}

static QT_FUNCTION_TARGET(ARCH_SKYLAKE_AVX512)
void qFloatToFloat16_tail_avx256(quint16 *out, const float *in, qsizetype len) noexcept
{
    __mmask16 mask = _bzhi_u32(-1, len);
    __m256 f32 = _mm256_maskz_loadu_ps(mask, in );
    __m128i f16 = _mm256_maskz_cvtps_ph(mask, f32, _MM_FROUND_TO_NEAREST_INT);
    _mm_mask_storeu_epi16(out, mask, f16);
};

static QT_FUNCTION_TARGET(ARCH_SKYLAKE_AVX512)
void qFloatFromFloat16_tail_avx256(float *out, const quint16 *in, qsizetype len) noexcept
{
    __mmask16 mask = _bzhi_u32(-1, len);
    __m128i f16 = _mm_maskz_loadu_epi16(mask, in);
    __m256 f32 = _mm256_cvtph_ps(f16);
    _mm256_mask_storeu_ps(out, mask, f32);
};
#endif

QT_FUNCTION_TARGET(F16C)
static void qFloatToFloat16_fast(quint16 *out, const float *in, qsizetype len) noexcept
{
    constexpr qsizetype Step = sizeof(__m256i) / sizeof(float);
    constexpr qsizetype HalfStep = sizeof(__m128i) / sizeof(float);
    qsizetype i = 0;

    if (len >= Step) {
        auto convertOneChunk = [=](qsizetype offset) QT_FUNCTION_TARGET(F16C) {
            __m256 f32 = _mm256_loadu_ps(in + offset);
            __m128i f16 = _mm256_cvtps_ph(f32, _MM_FROUND_TO_NEAREST_INT);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(out + offset), f16);
        };

        // main loop: convert Step (8) floats per iteration
        for ( ; i + Step < len; i += Step)
            convertOneChunk(i);

        // epilogue: convert the last chunk, possibly overlapping with the last
        // iteration of the loop
        return convertOneChunk(len - Step);
    }

#if QT_COMPILER_SUPPORTS_HERE(AVX512VL) && QT_COMPILER_SUPPORTS_HERE(AVX512BW)
    if (hasFastF16Avx256())
        return qFloatToFloat16_tail_avx256(out, in, len);
#endif

    if (len >= HalfStep) {
        auto convertOneChunk = [=](qsizetype offset) QT_FUNCTION_TARGET(F16C) {
            __m128 f32 = _mm_loadu_ps(in + offset);
            __m128i f16 = _mm_cvtps_ph(f32, _MM_FROUND_TO_NEAREST_INT);
            _mm_storel_epi64(reinterpret_cast<__m128i *>(out + offset), f16);
        };

        // two conversions, possibly overlapping
        convertOneChunk(0);
        return convertOneChunk(len - HalfStep);
    }

    // Inlining "qfloat16::qfloat16(float f)":
    for ( ; i < len; ++i)
        out[i] = _mm_extract_epi16(_mm_cvtps_ph(_mm_set_ss(in[i]), 0), 0);
}

QT_FUNCTION_TARGET(F16C)
static void qFloatFromFloat16_fast(float *out, const quint16 *in, qsizetype len) noexcept
{
    constexpr qsizetype Step = sizeof(__m256i) / sizeof(float);
    constexpr qsizetype HalfStep = sizeof(__m128i) / sizeof(float);
    qsizetype i = 0;

    if (len >= Step) {
        auto convertOneChunk = [=](qsizetype offset) QT_FUNCTION_TARGET(F16C) {
            __m128i f16 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(in + offset));
            __m256 f32 = _mm256_cvtph_ps(f16);
            _mm256_storeu_ps(out + offset, f32);
        };

        // main loop: convert Step (8) floats per iteration
        for ( ; i + Step < len; i += Step)
            convertOneChunk(i);

        // epilogue: convert the last chunk, possibly overlapping with the last
        // iteration of the loop
        return convertOneChunk(len - Step);
    }

#if QT_COMPILER_SUPPORTS_HERE(AVX512VL) && QT_COMPILER_SUPPORTS_HERE(AVX512BW)
    if (hasFastF16Avx256())
        return qFloatFromFloat16_tail_avx256(out, in, len);
#endif

    if (len >= HalfStep) {
        auto convertOneChunk = [=](qsizetype offset) QT_FUNCTION_TARGET(F16C) {
            __m128i f16 = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(in + offset));
            __m128 f32 = _mm_cvtph_ps(f16);
            _mm_storeu_ps(out + offset, f32);
        };

        // two conversions, possibly overlapping
        convertOneChunk(0);
        return convertOneChunk(len - HalfStep);
    }

    // Inlining "qfloat16::operator float()":
    for ( ; i < len; ++i)
        out[i] = _mm_cvtss_f32(_mm_cvtph_ps(_mm_cvtsi32_si128(in[i])));
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

/*!
    \fn size_t qfloat16::qHash(qfloat16 key, size_t seed)
    \since 6.5.3

    Returns the hash value for the \a key, using \a seed to seed the
    calculation.

    \note In Qt versions before 6.5, this operation was provided by the
    qHash(float) overload. In Qt versions 6.5.0 to 6.5.2, this functionality
    was broken in various ways. In Qt versions 6.5.3 and 6.6 onwards, this
    overload restores the Qt 6.4 behavior.
*/

#ifndef QT_NO_DATASTREAM
/*!
    \fn qfloat16::operator<<(QDataStream &ds, qfloat16 f)
    \relates QDataStream
    \since 5.9

    Writes a floating point number, \a f, to the stream \a ds using
    the standard IEEE 754 format. Returns a reference to the stream.

    \note In Qt versions prior to 6.3, this was a member function on
    QDataStream.
*/
QDataStream &operator<<(QDataStream &ds, qfloat16 f)
{
    return ds << f.b16;
}

/*!
    \fn qfloat16::operator>>(QDataStream &ds, qfloat16 &f)
    \relates QDataStream
    \since 5.9

    Reads a floating point number from the stream \a ds into \a f,
    using the standard IEEE 754 format. Returns a reference to the
    stream.

    \note In Qt versions prior to 6.3, this was a member function on
    QDataStream.
*/
QDataStream &operator>>(QDataStream &ds, qfloat16 &f)
{
    return ds >> f.b16;
}
#endif

QTextStream &operator>>(QTextStream &ts, qfloat16 &f16)
{
    float f;
    ts >> f;
    f16 = qfloat16(f);
    return ts;
}

QTextStream &operator<<(QTextStream &ts, qfloat16 f)
{
    return ts << float(f);
}

QT_END_NAMESPACE

#include "qfloat16tables.cpp"
