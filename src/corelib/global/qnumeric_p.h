/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2018 Intel Corporation.
** Contact: https://www.qt.io/licensing/
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

#ifndef QNUMERIC_P_H
#define QNUMERIC_P_H

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

#include "QtCore/private/qglobal_p.h"
#include <cmath>
#include <limits>

#if defined(Q_CC_MSVC)
#  include <intrin.h>
#endif

#if defined(Q_CC_MSVC)
#include <float.h>
#endif

#if !defined(Q_CC_MSVC) && (defined(Q_OS_QNX) || defined(Q_CC_INTEL))
#  include <math.h>
#  ifdef isnan
#    define QT_MATH_H_DEFINES_MACROS
QT_BEGIN_NAMESPACE
namespace qnumeric_std_wrapper {
// the 'using namespace std' below is cases where the stdlib already put the math.h functions in the std namespace and undefined the macros.
Q_DECL_CONST_FUNCTION static inline bool math_h_isnan(double d) { using namespace std; return isnan(d); }
Q_DECL_CONST_FUNCTION static inline bool math_h_isinf(double d) { using namespace std; return isinf(d); }
Q_DECL_CONST_FUNCTION static inline bool math_h_isfinite(double d) { using namespace std; return isfinite(d); }
Q_DECL_CONST_FUNCTION static inline bool math_h_isnan(float f) { using namespace std; return isnan(f); }
Q_DECL_CONST_FUNCTION static inline bool math_h_isinf(float f) { using namespace std; return isinf(f); }
Q_DECL_CONST_FUNCTION static inline bool math_h_isfinite(float f) { using namespace std; return isfinite(f); }
}
QT_END_NAMESPACE
// These macros from math.h conflict with the real functions in the std namespace.
#    undef signbit
#    undef isnan
#    undef isinf
#    undef isfinite
#  endif // defined(isnan)
#endif

QT_BEGIN_NAMESPACE

namespace qnumeric_std_wrapper {
#if defined(QT_MATH_H_DEFINES_MACROS)
#  undef QT_MATH_H_DEFINES_MACROS
Q_DECL_CONST_FUNCTION static inline bool isnan(double d) { return math_h_isnan(d); }
Q_DECL_CONST_FUNCTION static inline bool isinf(double d) { return math_h_isinf(d); }
Q_DECL_CONST_FUNCTION static inline bool isfinite(double d) { return math_h_isfinite(d); }
Q_DECL_CONST_FUNCTION static inline bool isnan(float f) { return math_h_isnan(f); }
Q_DECL_CONST_FUNCTION static inline bool isinf(float f) { return math_h_isinf(f); }
Q_DECL_CONST_FUNCTION static inline bool isfinite(float f) { return math_h_isfinite(f); }
#else
Q_DECL_CONST_FUNCTION static inline bool isnan(double d) { return std::isnan(d); }
Q_DECL_CONST_FUNCTION static inline bool isinf(double d) { return std::isinf(d); }
Q_DECL_CONST_FUNCTION static inline bool isfinite(double d) { return std::isfinite(d); }
Q_DECL_CONST_FUNCTION static inline bool isnan(float f) { return std::isnan(f); }
Q_DECL_CONST_FUNCTION static inline bool isinf(float f) { return std::isinf(f); }
Q_DECL_CONST_FUNCTION static inline bool isfinite(float f) { return std::isfinite(f); }
#endif
}

Q_DECL_CONSTEXPR Q_DECL_CONST_FUNCTION static inline double qt_inf() Q_DECL_NOEXCEPT
{
    Q_STATIC_ASSERT_X(std::numeric_limits<double>::has_infinity,
                      "platform has no definition for infinity for type double");
    return std::numeric_limits<double>::infinity();
}

// Signaling NaN
Q_DECL_CONSTEXPR Q_DECL_CONST_FUNCTION static inline double qt_snan() Q_DECL_NOEXCEPT
{
    Q_STATIC_ASSERT_X(std::numeric_limits<double>::has_signaling_NaN,
                      "platform has no definition for signaling NaN for type double");
    return std::numeric_limits<double>::signaling_NaN();
}

// Quiet NaN
Q_DECL_CONSTEXPR Q_DECL_CONST_FUNCTION static inline double qt_qnan() Q_DECL_NOEXCEPT
{
    Q_STATIC_ASSERT_X(std::numeric_limits<double>::has_quiet_NaN,
                      "platform has no definition for quiet NaN for type double");
    return std::numeric_limits<double>::quiet_NaN();
}

Q_DECL_CONST_FUNCTION static inline bool qt_is_inf(double d)
{
    return qnumeric_std_wrapper::isinf(d);
}

Q_DECL_CONST_FUNCTION static inline bool qt_is_nan(double d)
{
    return qnumeric_std_wrapper::isnan(d);
}

Q_DECL_CONST_FUNCTION static inline bool qt_is_finite(double d)
{
    return qnumeric_std_wrapper::isfinite(d);
}

Q_DECL_CONST_FUNCTION static inline bool qt_is_inf(float f)
{
    return qnumeric_std_wrapper::isinf(f);
}

Q_DECL_CONST_FUNCTION static inline bool qt_is_nan(float f)
{
    return qnumeric_std_wrapper::isnan(f);
}

Q_DECL_CONST_FUNCTION static inline bool qt_is_finite(float f)
{
    return qnumeric_std_wrapper::isfinite(f);
}

#ifndef Q_CLANG_QDOC
namespace {
// Overflow math.
// This provides efficient implementations for int, unsigned, qsizetype and
// size_t. Implementations for 8- and 16-bit types will work but may not be as
// efficient. Implementations for 64-bit may be missing on 32-bit platforms.

#if (defined(Q_CC_GNU) && (Q_CC_GNU >= 500) || (defined(Q_CC_INTEL) && !defined(Q_OS_WIN))) || QT_HAS_BUILTIN(__builtin_add_overflowx)
// GCC 5, ICC 18, and Clang 3.8 have builtins to detect overflows

template <typename T> inline
typename std::enable_if<std::is_unsigned<T>::value || std::is_signed<T>::value, bool>::type
add_overflow(T v1, T v2, T *r)
{ return __builtin_add_overflow(v1, v2, r); }

template <typename T> inline
typename std::enable_if<std::is_unsigned<T>::value || std::is_signed<T>::value, bool>::type
sub_overflow(T v1, T v2, T *r)
{ return __builtin_sub_overflow(v1, v2, r); }

template <typename T> inline
typename std::enable_if<std::is_unsigned<T>::value || std::is_signed<T>::value, bool>::type
mul_overflow(T v1, T v2, T *r)
{ return __builtin_mul_overflow(v1, v2, r); }

#else
// Generic implementations

template <typename T> inline typename std::enable_if<std::is_unsigned<T>::value, bool>::type
add_overflow(T v1, T v2, T *r)
{
    // unsigned additions are well-defined
    *r = v1 + v2;
    return v1 > T(v1 + v2);
}

template <typename T> inline typename std::enable_if<std::is_signed<T>::value, bool>::type
add_overflow(T v1, T v2, T *r)
{
    // Here's how we calculate the overflow:
    // 1) unsigned addition is well-defined, so we can always execute it
    // 2) conversion from unsigned back to signed is implementation-
    //    defined and in the implementations we use, it's a no-op.
    // 3) signed integer overflow happens if the sign of the two input operands
    //    is the same but the sign of the result is different. In other words,
    //    the sign of the result must be the same as the sign of either
    //    operand.

    using U = typename std::make_unsigned<T>::type;
    *r = T(U(v1) + U(v2));

    // If int is two's complement, assume all integer types are too.
    if (std::is_same<int32_t, int>::value) {
        // Two's complement equivalent (generates slightly shorter code):
        //  x ^ y             is negative if x and y have different signs
        //  x & y             is negative if x and y are negative
        // (x ^ z) & (y ^ z)  is negative if x and z have different signs
        //                    AND y and z have different signs
        return ((v1 ^ *r) & (v2 ^ *r)) < 0;
    }

    bool s1 = (v1 < 0);
    bool s2 = (v2 < 0);
    bool sr = (*r < 0);
    return s1 != sr && s2 != sr;
    // also: return s1 == s2 && s1 != sr;
}

template <typename T> inline typename std::enable_if<std::is_unsigned<T>::value, bool>::type
sub_overflow(T v1, T v2, T *r)
{
    // unsigned subtractions are well-defined
    *r = v1 - v2;
    return v1 < v2;
}

template <typename T> inline typename std::enable_if<std::is_signed<T>::value, bool>::type
sub_overflow(T v1, T v2, T *r)
{
    // See above for explanation. This is the same with some signs reversed.
    // We can't use add_overflow(v1, -v2, r) because it would be UB if
    // v2 == std::numeric_limits<T>::min().

    using U = typename std::make_unsigned<T>::type;
    *r = T(U(v1) - U(v2));

    if (std::is_same<int32_t, int>::value)
        return ((v1 ^ *r) & (~v2 ^ *r)) < 0;

    bool s1 = (v1 < 0);
    bool s2 = !(v2 < 0);
    bool sr = (*r < 0);
    return s1 != sr && s2 != sr;
    // also: return s1 == s2 && s1 != sr;
}

template <typename T> inline
typename std::enable_if<std::is_unsigned<T>::value || std::is_signed<T>::value, bool>::type
mul_overflow(T v1, T v2, T *r)
{
    // use the next biggest type
    // Note: for 64-bit systems where __int128 isn't supported, this will cause an error.
    using LargerInt = QIntegerForSize<sizeof(T) * 2>;
    using Larger = typename std::conditional<std::is_signed<T>::value,
            typename LargerInt::Signed, typename LargerInt::Unsigned>::type;
    Larger lr = Larger(v1) * Larger(v2);
    *r = T(lr);
    return lr > std::numeric_limits<T>::max() || lr < std::numeric_limits<T>::min();
}

#  if defined(Q_CC_MSVC) && defined(Q_PROCESSOR_X86)
// We can use intrinsics for the unsigned operations with MSVC
template <> inline bool add_overflow(unsigned v1, unsigned v2, unsigned *r)
{ return _addcarry_u32(0, v1, v2, r); }

// 32-bit mul_overflow is fine with the generic code above

#    if defined(Q_PROCESSOR_X86_64)
template <> inline bool add_overflow(quint64 v1, quint64 v2, quint64 *r)
{ return _addcarry_u64(0, v1, v2, reinterpret_cast<unsigned __int64 *>(r)); }

#    pragma intrinsic(_umul128)
template <> inline bool mul_overflow(quint64 v1, quint64 v2, quint64 *r)
{
    // use 128-bit multiplication with the _umul128 intrinsic
    // https://msdn.microsoft.com/en-us/library/3dayytw9.aspx
    quint64 high;
    *r = _umul128(v1, v2, &high);
    return high;
}

#    pragma intrinsic(_mul128)
template <> inline bool mul_overflow(qint64 v1, qint64 v2, qint64 *r)
{
    // Use 128-bit multiplication with the _mul128 intrinsic
    // https://msdn.microsoft.com/en-us/library/82cxdw50.aspx

    // This is slightly more complex than the unsigned case above: the sign bit
    // of 'low' must be replicated as the entire 'high', so the only valid
    // values for 'high' are 0 and -1.

    qint64 high;
    *r = _mul128(v1, v2, &high);
    if (high == 0)
        return *r < 0;
    if (high == -1)
        return *r >= 0;
    return true;
}
#    endif // x86-64
#  endif // MSVC x86
#endif // !GCC
}
#endif // Q_CLANG_QDOC

QT_END_NAMESPACE

#endif // QNUMERIC_P_H
