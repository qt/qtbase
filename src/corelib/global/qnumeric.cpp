// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qnumeric.h"
#include "qnumeric_p.h"
#include <string.h>

QT_BEGIN_NAMESPACE

/*!
    Returns \c true if the double \a {d} is equivalent to infinity.
    \relates <QtGlobal>
    \sa qInf()
*/
Q_CORE_EXPORT bool qIsInf(double d) { return qt_is_inf(d); }

/*!
    Returns \c true if the double \a {d} is not a number (NaN).
    \relates <QtGlobal>
*/
Q_CORE_EXPORT bool qIsNaN(double d) { return qt_is_nan(d); }

/*!
    Returns \c true if the double \a {d} is a finite number.
    \relates <QtGlobal>
*/
Q_CORE_EXPORT bool qIsFinite(double d) { return qt_is_finite(d); }

/*!
    Returns \c true if the float \a {f} is equivalent to infinity.
    \relates <QtGlobal>
    \sa qInf()
*/
Q_CORE_EXPORT bool qIsInf(float f) { return qt_is_inf(f); }

/*!
    Returns \c true if the float \a {f} is not a number (NaN).
    \relates <QtGlobal>
*/
Q_CORE_EXPORT bool qIsNaN(float f) { return qt_is_nan(f); }

/*!
    Returns \c true if the float \a {f} is a finite number.
    \relates <QtGlobal>
*/
Q_CORE_EXPORT bool qIsFinite(float f) { return qt_is_finite(f); }

#if QT_CONFIG(signaling_nan)
/*!
    Returns the bit pattern of a signalling NaN as a double.
    \relates <QtGlobal>
*/
Q_CORE_EXPORT double qSNaN() { return qt_snan(); }
#endif

/*!
    Returns the bit pattern of a quiet NaN as a double.
    \relates <QtGlobal>
    \sa qIsNaN()
*/
Q_CORE_EXPORT double qQNaN() { return qt_qnan(); }

/*!
    Returns the bit pattern for an infinite number as a double.
    \relates <QtGlobal>
    \sa qIsInf()
*/
Q_CORE_EXPORT double qInf() { return qt_inf(); }

/*!
    \fn int qFpClassify(double val)
    \fn int qFpClassify(float val)

    \relates <QtGlobal>
    Classifies a floating-point value.

    The return values are defined in \c{<cmath>}: returns one of the following,
    determined by the floating-point class of \a val:
    \list
    \li FP_NAN not a number
    \li FP_INFINITE infinities (positive or negative)
    \li FP_ZERO zero (positive or negative)
    \li FP_NORMAL finite with a full mantissa
    \li FP_SUBNORMAL finite with a reduced mantissa
    \endlist
*/
Q_CORE_EXPORT int qFpClassify(double val) { return qt_fpclassify(val); }
Q_CORE_EXPORT int qFpClassify(float val) { return qt_fpclassify(val); }


/*!
   \internal
 */
static inline quint32 f2i(float f)
{
    quint32 i;
    memcpy(&i, &f, sizeof(f));
    return i;
}

/*!
    Returns the number of representable floating-point numbers between \a a and \a b.

    This function provides an alternative way of doing approximated comparisons of floating-point
    numbers similar to qFuzzyCompare(). However, it returns the distance between two numbers, which
    gives the caller a possibility to choose the accepted error. Errors are relative, so for
    instance the distance between 1.0E-5 and 1.00001E-5 will give 110, while the distance between
    1.0E36 and 1.00001E36 will give 127.

    This function is useful if a floating point comparison requires a certain precision.
    Therefore, if \a a and \a b are equal it will return 0. The maximum value it will return for 32-bit
    floating point numbers is 4,278,190,078. This is the distance between \c{-FLT_MAX} and
    \c{+FLT_MAX}.

    The function does not give meaningful results if any of the arguments are \c Infinite or \c NaN.
    You can check for this by calling qIsFinite().

    The return value can be considered as the "error", so if you for instance want to compare
    two 32-bit floating point numbers and all you need is an approximated 24-bit precision, you can
    use this function like this:

    \snippet code/src_corelib_global_qnumeric.cpp 0

    \sa qFuzzyCompare()
    \since 5.2
    \relates <QtGlobal>
*/
Q_CORE_EXPORT quint32 qFloatDistance(float a, float b)
{
    static const quint32 smallestPositiveFloatAsBits = 0x00000001;  // denormalized, (SMALLEST), (1.4E-45)
    /* Assumes:
       * IEE754 format.
       * Integers and floats have the same endian
    */
    static_assert(sizeof(quint32) == sizeof(float));
    Q_ASSERT(qIsFinite(a) && qIsFinite(b));
    if (a == b)
        return 0;
    if ((a < 0) != (b < 0)) {
        // if they have different signs
        if (a < 0)
            a = -a;
        else /*if (b < 0)*/
            b = -b;
        return qFloatDistance(0.0F, a) + qFloatDistance(0.0F, b);
    }
    if (a < 0) {
        a = -a;
        b = -b;
    }
    // at this point a and b should not be negative

    // 0 is special
    if (!a)
        return f2i(b) - smallestPositiveFloatAsBits + 1;
    if (!b)
        return f2i(a) - smallestPositiveFloatAsBits + 1;

    // finally do the common integer subtraction
    return a > b ? f2i(a) - f2i(b) : f2i(b) - f2i(a);
}


/*!
   \internal
 */
static inline quint64 d2i(double d)
{
    quint64 i;
    memcpy(&i, &d, sizeof(d));
    return i;
}

/*!
    Returns the number of representable floating-point numbers between \a a and \a b.

    This function serves the same purpose as \c{qFloatDistance(float, float)}, but
    returns the distance between two \c double numbers. Since the range is larger
    than for two \c float numbers (\c{[-DBL_MAX,DBL_MAX]}), the return type is quint64.


    \sa qFuzzyCompare()
    \since 5.2
    \relates <QtGlobal>
*/
Q_CORE_EXPORT quint64 qFloatDistance(double a, double b)
{
    static const quint64 smallestPositiveFloatAsBits = 0x1;  // denormalized, (SMALLEST)
    /* Assumes:
       * IEE754 format double precision
       * Integers and floats have the same endian
    */
    static_assert(sizeof(quint64) == sizeof(double));
    Q_ASSERT(qIsFinite(a) && qIsFinite(b));
    if (a == b)
        return 0;
    if ((a < 0) != (b < 0)) {
        // if they have different signs
        if (a < 0)
            a = -a;
        else /*if (b < 0)*/
            b = -b;
        return qFloatDistance(0.0, a) + qFloatDistance(0.0, b);
    }
    if (a < 0) {
        a = -a;
        b = -b;
    }
    // at this point a and b should not be negative

    // 0 is special
    if (!a)
        return d2i(b) - smallestPositiveFloatAsBits + 1;
    if (!b)
        return d2i(a) - smallestPositiveFloatAsBits + 1;

    // finally do the common integer subtraction
    return a > b ? d2i(a) - d2i(b) : d2i(b) - d2i(a);
}

/*!
    \fn template<typename T> bool qAddOverflow(T v1, T v2, T *result)
    \relates <QtGlobal>
    \since 6.1

    Adds two values \a v1 and \a v2, of a numeric type \c T and records the
    value in \a result. If the addition overflows the valid range for type \c T,
    returns \c true, otherwise returns \c false.

    An implementation is guaranteed to be available for 8-, 16-, and 32-bit
    integer types, as well as integer types of the size of a pointer. Overflow
    math for other types, if available, is considered private API.
*/

/*!
    \fn template <typename T, T V2> bool qAddOverflow(T v1, std::integral_constant<T, V2>, T *r)
    \since 6.1
    \internal

    Equivalent to qAddOverflow(v1, v2, r) with \a v1 as first argument, the
    compile time constant \c V2 as second argument, and \a r as third argument.
*/

/*!
    \fn template <auto V2, typename T> bool qAddOverflow(T v1, T *r)
    \since 6.1
    \internal

    Equivalent to qAddOverflow(v1, v2, r) with \a v1 as first argument, the
    compile time constant \c V2 as second argument, and \a r as third argument.
*/

/*!
    \fn template<typename T> bool qSubOverflow(T v1, T v2, T *result)
    \relates <QtGlobal>
    \since 6.1

    Subtracts \a v2 from \a v1 and records the resulting value in \a result. If
    the subtraction overflows the valid range for type \c T, returns \c true,
    otherwise returns \c false.

    An implementation is guaranteed to be available for 8-, 16-, and 32-bit
    integer types, as well as integer types of the size of a pointer. Overflow
    math for other types, if available, is considered private API.
*/

/*!
    \fn template <typename T, T V2> bool qSubOverflow(T v1, std::integral_constant<T, V2>, T *r)
    \since 6.1
    \internal

    Equivalent to qSubOverflow(v1, v2, r) with \a v1 as first argument, the
    compile time constant \c V2 as second argument, and \a r as third argument.
*/

/*!
    \fn template <auto V2, typename T> bool qSubOverflow(T v1, T *r)
    \since 6.1
    \internal

    Equivalent to qSubOverflow(v1, v2, r) with \a v1 as first argument, the
    compile time constant \c V2 as second argument, and \a r as third argument.
*/

/*!
    \fn template<typename T> bool qMulOverflow(T v1, T v2, T *result)
    \relates <QtGlobal>
    \since 6.1

    Multiplies \a v1 and \a v2, and records the resulting value in \a result. If
    the multiplication overflows the valid range for type \c T, returns
    \c true, otherwise returns \c false.

    An implementation is guaranteed to be available for 8-, 16-, and 32-bit
    integer types, as well as integer types of the size of a pointer. Overflow
    math for other types, if available, is considered private API.
*/

/*!
    \fn template <typename T, T V2> bool qMulOverflow(T v1, std::integral_constant<T, V2>, T *r)
    \since 6.1
    \internal

    Equivalent to qMulOverflow(v1, v2, r) with \a v1 as first argument, the
    compile time constant \c V2 as second argument, and \a r as third argument.
    This can be faster than calling the version with only variable arguments.
*/

/*!
    \fn template <auto V2, typename T> bool qMulOverflow(T v1, T *r)
    \since 6.1
    \internal

    Equivalent to qMulOverflow(v1, v2, r) with \a v1 as first argument, the
    compile time constant \c V2 as second argument, and \a r as third argument.
    This can be faster than calling the version with only variable arguments.
*/

template <typename T> static constexpr T max = std::numeric_limits<T>::max();
template <typename T> static constexpr T min = std::numeric_limits<T>::min();

static_assert(qt_saturate<short>(max<unsigned>) == max<short>);
static_assert(qt_saturate<int>(max<unsigned>) == max<int>);
static_assert(qt_saturate<qint64>(max<unsigned>) == qint64(max<unsigned>));

static_assert(qt_saturate<short>(max<int>) == max<short>);
static_assert(qt_saturate<unsigned>(max<int>) == unsigned(max<int>));
static_assert(qt_saturate<qint64>(max<int>) == qint64(max<int>));

static_assert(qt_saturate<short>(max<qint64>) == max<short>);
static_assert(qt_saturate<int>(max<qint64>) == max<int>);
static_assert(qt_saturate<unsigned>(max<qint64>) == max<unsigned>);
static_assert(qt_saturate<quint64>(max<qint64>) == quint64(max<qint64>));

static_assert(qt_saturate<short>(max<quint64>) == max<short>);
static_assert(qt_saturate<int>(max<quint64>) == max<int>);
static_assert(qt_saturate<unsigned>(max<quint64>) == max<unsigned>);
static_assert(qt_saturate<qint64>(max<quint64>) == max<qint64>);

static_assert(qt_saturate<short>(min<int>) == min<short>);
static_assert(qt_saturate<qint64>(min<int>) == qint64(min<int>));
static_assert(qt_saturate<unsigned>(min<int>) == 0);
static_assert(qt_saturate<quint64>(min<int>) == 0);

static_assert(qt_saturate<short>(min<qint64>) == min<short>);
static_assert(qt_saturate<int>(min<qint64>) == min<int>);
static_assert(qt_saturate<unsigned>(min<qint64>) == 0);
static_assert(qt_saturate<quint64>(min<qint64>) == 0);

QT_END_NAMESPACE
