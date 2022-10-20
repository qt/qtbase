/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2020 Intel Corporation.
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
#include "QtCore/qnumeric.h"
#include <cmath>
#include <limits>
#include <type_traits>

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
Q_DECL_CONST_FUNCTION static inline int math_h_fpclassify(double d) { using namespace std; return fpclassify(d); }
Q_DECL_CONST_FUNCTION static inline bool math_h_isnan(float f) { using namespace std; return isnan(f); }
Q_DECL_CONST_FUNCTION static inline bool math_h_isinf(float f) { using namespace std; return isinf(f); }
Q_DECL_CONST_FUNCTION static inline bool math_h_isfinite(float f) { using namespace std; return isfinite(f); }
Q_DECL_CONST_FUNCTION static inline int math_h_fpclassify(float f) { using namespace std; return fpclassify(f); }
}
QT_END_NAMESPACE
// These macros from math.h conflict with the real functions in the std namespace.
#    undef signbit
#    undef isnan
#    undef isinf
#    undef isfinite
#    undef fpclassify
#  endif // defined(isnan)
#endif

QT_BEGIN_NAMESPACE

namespace qnumeric_std_wrapper {
#if defined(QT_MATH_H_DEFINES_MACROS)
#  undef QT_MATH_H_DEFINES_MACROS
Q_DECL_CONST_FUNCTION static inline bool isnan(double d) { return math_h_isnan(d); }
Q_DECL_CONST_FUNCTION static inline bool isinf(double d) { return math_h_isinf(d); }
Q_DECL_CONST_FUNCTION static inline bool isfinite(double d) { return math_h_isfinite(d); }
Q_DECL_CONST_FUNCTION static inline int fpclassify(double d) { return math_h_fpclassify(d); }
Q_DECL_CONST_FUNCTION static inline bool isnan(float f) { return math_h_isnan(f); }
Q_DECL_CONST_FUNCTION static inline bool isinf(float f) { return math_h_isinf(f); }
Q_DECL_CONST_FUNCTION static inline bool isfinite(float f) { return math_h_isfinite(f); }
Q_DECL_CONST_FUNCTION static inline int fpclassify(float f) { return math_h_fpclassify(f); }
#else
Q_DECL_CONST_FUNCTION static inline bool isnan(double d) { return std::isnan(d); }
Q_DECL_CONST_FUNCTION static inline bool isinf(double d) { return std::isinf(d); }
Q_DECL_CONST_FUNCTION static inline bool isfinite(double d) { return std::isfinite(d); }
Q_DECL_CONST_FUNCTION static inline int fpclassify(double d) { return std::fpclassify(d); }
Q_DECL_CONST_FUNCTION static inline bool isnan(float f) { return std::isnan(f); }
Q_DECL_CONST_FUNCTION static inline bool isinf(float f) { return std::isinf(f); }
Q_DECL_CONST_FUNCTION static inline bool isfinite(float f) { return std::isfinite(f); }
Q_DECL_CONST_FUNCTION static inline int fpclassify(float f) { return std::fpclassify(f); }
#endif
}

constexpr Q_DECL_CONST_FUNCTION static inline double qt_inf() noexcept
{
    static_assert(std::numeric_limits<double>::has_infinity,
                  "platform has no definition for infinity for type double");
    return std::numeric_limits<double>::infinity();
}

#if QT_CONFIG(signaling_nan)
constexpr Q_DECL_CONST_FUNCTION static inline double qt_snan() noexcept
{
    static_assert(std::numeric_limits<double>::has_signaling_NaN,
                  "platform has no definition for signaling NaN for type double");
    return std::numeric_limits<double>::signaling_NaN();
}
#endif

// Quiet NaN
constexpr Q_DECL_CONST_FUNCTION static inline double qt_qnan() noexcept
{
    static_assert(std::numeric_limits<double>::has_quiet_NaN,
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

Q_DECL_CONST_FUNCTION static inline int qt_fpclassify(double d)
{
    return qnumeric_std_wrapper::fpclassify(d);
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

Q_DECL_CONST_FUNCTION static inline int qt_fpclassify(float f)
{
    return qnumeric_std_wrapper::fpclassify(f);
}

#ifndef Q_CLANG_QDOC
namespace {
/*!
    Returns true if the double \a v can be converted to type \c T, false if
    it's out of range. If the conversion is successful, the converted value is
    stored in \a value; if it was not successful, \a value will contain the
    minimum or maximum of T, depending on the sign of \a d. If \c T is
    unsigned, then \a value contains the absolute value of \a v.

    This function works for v containing infinities, but not NaN. It's the
    caller's responsibility to exclude that possibility before calling it.
*/
template<typename T>
static inline bool convertDoubleTo(double v, T *value, bool allow_precision_upgrade = true)
{
    static_assert(std::numeric_limits<T>::is_integer);

    // The [conv.fpint] (7.10 Floating-integral conversions) section of the C++
    // standard says only exact conversions are guaranteed. Converting
    // integrals to floating-point with loss of precision has implementation-
    // defined behavior whether the next higher or next lower is returned;
    // converting FP to integral is UB if it can't be represented.
    //
    // That means we can't write UINT64_MAX+1. Writing ldexp(1, 64) would be
    // correct, but Clang, ICC and MSVC don't realize that it's a constant and
    // the math call stays in the compiled code.

    double supremum;
    if (std::numeric_limits<T>::is_signed) {
        supremum = -1.0 * std::numeric_limits<T>::min();    // -1 * (-2^63) = 2^63, exact (for T = qint64)
        *value = std::numeric_limits<T>::min();
        if (v < std::numeric_limits<T>::min())
            return false;
    } else {
        using ST = typename std::make_signed<T>::type;
        supremum = -2.0 * std::numeric_limits<ST>::min();   // -2 * (-2^63) = 2^64, exact (for T = quint64)
        v = fabs(v);
    }
    if (std::is_integral<T>::value && sizeof(T) > 4 && !allow_precision_upgrade) {
        if (v > double(Q_INT64_C(1)<<53) || v < double(-((Q_INT64_C(1)<<53) + 1)))
            return false;
    }

    *value = std::numeric_limits<T>::max();
    if (v >= supremum)
        return false;

    // Now we can convert, these two conversions cannot be UB
    *value = T(v);

QT_WARNING_PUSH
QT_WARNING_DISABLE_FLOAT_COMPARE

    return *value == v;

QT_WARNING_POP
}

template <typename T> inline bool add_overflow(T v1, T v2, T *r) { return qAddOverflow(v1, v2, r); }
template <typename T> inline bool sub_overflow(T v1, T v2, T *r) { return qSubOverflow(v1, v2, r); }
template <typename T> inline bool mul_overflow(T v1, T v2, T *r) { return qMulOverflow(v1, v2, r); }

template <typename T, T V2> bool add_overflow(T v1, std::integral_constant<T, V2>, T *r)
{
    return qAddOverflow<T, V2>(v1, std::integral_constant<T, V2>{}, r);
}

template <auto V2, typename T> bool add_overflow(T v1, T *r)
{
    return qAddOverflow<V2, T>(v1, r);
}

template <typename T, T V2> bool sub_overflow(T v1, std::integral_constant<T, V2>, T *r)
{
    return qSubOverflow<T, V2>(v1, std::integral_constant<T, V2>{}, r);
}

template <auto V2, typename T> bool sub_overflow(T v1, T *r)
{
    return qSubOverflow<V2, T>(v1, r);
}

template <typename T, T V2> bool mul_overflow(T v1, std::integral_constant<T, V2>, T *r)
{
    return qMulOverflow<T, V2>(v1, std::integral_constant<T, V2>{}, r);
}

template <auto V2, typename T> bool mul_overflow(T v1, T *r)
{
    return qMulOverflow<V2, T>(v1, r);
}
}
#endif // Q_CLANG_QDOC

QT_END_NAMESPACE

#endif // QNUMERIC_P_H
