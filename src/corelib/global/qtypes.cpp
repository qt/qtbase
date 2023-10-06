// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtypes.h"

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qsystemdetection.h>
#include <QtCore/qprocessordetection.h>

#include <climits>
#include <limits>
#include <type_traits>

QT_BEGIN_NAMESPACE

/*!
    \headerfile <QtTypes>
    \inmodule QtCore
    \title Qt Type Declarations

    \brief The <QtTypes> header file includes Qt fundamental type declarations.

    The header file declares several type definitions that guarantee a
    specified bit-size on all platforms supported by Qt for various
    basic types, for example \l qint8 which is a signed char
    guaranteed to be 8-bit on all platforms supported by Qt. The
    header file also declares the \l qlonglong type definition for
    \c {long long int}.

    Several convenience type definitions are declared: \l qreal for \c
    double or \c float, \l uchar for \c {unsigned char}, \l uint for
    \c {unsigned int}, \l ulong for \c {unsigned long} and \l ushort
    for \c {unsigned short}.

    The header also provides series of macros that make it possible to print
    some Qt type aliases (qsizetype, qintptr, etc.) via a formatted output
    facility such as printf() or qDebug() without raising formatting warnings
    and without the need of a type cast.
*/

/*!
    \typedef qreal
    \relates <QtTypes>

    Typedef for \c double unless Qt is configured with the
    \c{-qreal float} option.
*/

/*! \typedef uchar
    \relates <QtTypes>

    Convenience typedef for \c{unsigned char}.
*/

/*! \typedef ushort
    \relates <QtTypes>

    Convenience typedef for \c{unsigned short}.
*/

/*! \typedef uint
    \relates <QtTypes>

    Convenience typedef for \c{unsigned int}.
*/

/*! \typedef ulong
    \relates <QtTypes>

    Convenience typedef for \c{unsigned long}.
*/

/*! \typedef qint8
    \relates <QtTypes>

    Typedef for \c{signed char}. This type is guaranteed to be 8-bit
    on all platforms supported by Qt.
*/

/*!
    \typedef quint8
    \relates <QtTypes>

    Typedef for \c{unsigned char}. This type is guaranteed to
    be 8-bit on all platforms supported by Qt.
*/

/*! \typedef qint16
    \relates <QtTypes>

    Typedef for \c{signed short}. This type is guaranteed to be
    16-bit on all platforms supported by Qt.
*/

/*!
    \typedef quint16
    \relates <QtTypes>

    Typedef for \c{unsigned short}. This type is guaranteed to
    be 16-bit on all platforms supported by Qt.
*/

/*! \typedef qint32
    \relates <QtTypes>

    Typedef for \c{signed int}. This type is guaranteed to be 32-bit
    on all platforms supported by Qt.
*/

/*!
    \typedef quint32
    \relates <QtTypes>

    Typedef for \c{unsigned int}. This type is guaranteed to
    be 32-bit on all platforms supported by Qt.
*/

/*! \typedef qint64
    \relates <QtTypes>

    Typedef for \c{long long int}. This type is guaranteed to be 64-bit
    on all platforms supported by Qt.

    Literals of this type can be created using the Q_INT64_C() macro:

    \snippet code/src_corelib_global_qglobal.cpp 5

    \sa Q_INT64_C(), quint64, qlonglong
*/

/*!
    \typedef quint64
    \relates <QtTypes>

    Typedef for \c{unsigned long long int}. This type is guaranteed to
    be 64-bit on all platforms supported by Qt.

    Literals of this type can be created using the Q_UINT64_C()
    macro:

    \snippet code/src_corelib_global_qglobal.cpp 6

    \sa Q_UINT64_C(), qint64, qulonglong
*/

/*!
    \typedef qint128
    \relates <QtTypes>
    \since 6.6

    Typedef for \c{__int128} on platforms that support it (Qt defines the macro
    \l QT_SUPPORTS_INT128 if this is the case).

    Literals of this type can be created using the Q_INT128_C() macro.

    \sa Q_INT128_C(), Q_INT128_MIN, Q_INT128_MAX, quint128, QT_SUPPORTS_INT128
*/

/*!
    \typedef quint128
    \relates <QtTypes>
    \since 6.6

    Typedef for \c{unsigned __int128} on platforms that support it (Qt defines
    the macro \l QT_SUPPORTS_INT128 if this is the case).

    Literals of this type can be created using the Q_UINT128_C() macro.

    \sa Q_UINT128_C(), Q_UINT128_MAX, qint128, QT_SUPPORTS_INT128
*/

/*!
    \macro QT_SUPPORTS_INT128
    \relates <QtTypes>
    \since 6.6

    Qt defines this macro as well as the \l qint128 and \l quint128 types if
    the platform has support for 128-bit integer types.

    \sa qint128, quint128, Q_INT128_C(), Q_UINT128_C(), Q_INT128_MIN, Q_INT128_MAX, Q_UINT128_MAX
*/

/*!
    \typedef qintptr
    \relates <QtTypes>

    Integral type for representing pointers in a signed integer (useful for
    hashing, etc.).

    Typedef for either qint32 or qint64. This type is guaranteed to
    be the same size as a pointer on all platforms supported by Qt. On
    a system with 32-bit pointers, qintptr is a typedef for qint32;
    on a system with 64-bit pointers, qintptr is a typedef for
    qint64.

    Note that qintptr is signed. Use quintptr for unsigned values.

    In order to print values of this type by using formatted-output
    facilities such as \c{printf()}, qDebug(), QString::asprintf() and
    so on, you can use the \c{PRIdQINTPTR} and \c{PRIiQINTPTR}
    macros as format specifiers. They will both print the value as a
    base 10 number.

    \code
    qintptr p = 123;
    printf("The pointer is %" PRIdQINTPTR "\n", p);
    \endcode

    \sa qptrdiff, qint32, qint64
*/

/*! \typedef qlonglong
    \relates <QtTypes>

    Typedef for \c{long long int} (\c __int64 on Windows). This is
    the same as \l qint64.

    \sa qulonglong, qint64
*/

/*!
    \typedef qulonglong
    \relates <QtTypes>

    Typedef for \c{unsigned long long int} (\c{unsigned __int64} on
    Windows). This is the same as \l quint64.

    \sa quint64, qlonglong
*/

/*!
    \macro PRIdQINTPTR
    \macro PRIiQINTPTR
    \since 6.2
    \relates <QtTypes>

    See \l qintptr.
*/

/*!
    \typedef quintptr
    \relates <QtTypes>

    Integral type for representing pointers in an unsigned integer (useful for
    hashing, etc.).

    Typedef for either quint32 or quint64. This type is guaranteed to
    be the same size as a pointer on all platforms supported by Qt. On
    a system with 32-bit pointers, quintptr is a typedef for quint32;
    on a system with 64-bit pointers, quintptr is a typedef for
    quint64.

    Note that quintptr is unsigned. Use qptrdiff for signed values.

    In order to print values of this type by using formatted-output
    facilities such as \c{printf()}, qDebug(), QString::asprintf() and
    so on, you can use the following macros as format specifiers:

    \list
    \li \c{PRIuQUINTPTR}: prints the value as a base 10 number.
    \li \c{PRIoQUINTPTR}: prints the value as a base 8 number.
    \li \c{PRIxQUINTPTR}: prints the value as a base 16 number, using lowercase \c{a-f} letters.
    \li \c{PRIXQUINTPTR}: prints the value as a base 16 number, using uppercase \c{A-F} letters.
    \endlist

    \code
    quintptr p = 123u;
    printf("The pointer value is 0x%" PRIXQUINTPTR "\n", p);
    \endcode

    \sa qptrdiff, quint32, quint64
*/

/*!
    \macro PRIoQUINTPTR
    \macro PRIuQUINTPTR
    \macro PRIxQUINTPTR
    \macro PRIXQUINTPTR
    \since 6.2
    \relates <QtTypes>

    See quintptr.
*/

/*!
    \typedef qptrdiff
    \relates <QtTypes>

    Integral type for representing pointer differences.

    Typedef for either qint32 or qint64. This type is guaranteed to be
    the same size as a pointer on all platforms supported by Qt. On a
    system with 32-bit pointers, quintptr is a typedef for quint32; on
    a system with 64-bit pointers, quintptr is a typedef for quint64.

    Note that qptrdiff is signed. Use quintptr for unsigned values.

    In order to print values of this type by using formatted-output
    facilities such as \c{printf()}, qDebug(), QString::asprintf() and
    so on, you can use the \c{PRIdQPTRDIFF} and \c{PRIiQPTRDIFF}
    macros as format specifiers. They will both print the value as a
    base 10 number.

    \code
    qptrdiff d = 123;
    printf("The difference is %" PRIdQPTRDIFF "\n", d);
    \endcode

    \sa quintptr, qint32, qint64
*/

/*!
    \macro PRIdQPTRDIFF
    \macro PRIiQPTRDIFF
    \since 6.2
    \relates <QtTypes>

    See qptrdiff.
*/

/*!
    \typedef qsizetype
    \relates <QtTypes>
    \since 5.10

    Integral type providing Posix' \c ssize_t for all platforms.

    This type is guaranteed to be the same size as a \c size_t on all
    platforms supported by Qt.

    Note that qsizetype is signed. Use \c size_t for unsigned values.

    In order to print values of this type by using formatted-output
    facilities such as \c{printf()}, qDebug(), QString::asprintf() and
    so on, you can use the \c{PRIdQSIZETYPE} and \c{PRIiQSIZETYPE}
    macros as format specifiers. They will both print the value as a
    base 10 number.

    \code
    qsizetype s = 123;
    printf("The size is %" PRIdQSIZETYPE "\n", s);
    \endcode

    \sa qptrdiff
*/

/*!
    \macro PRIdQSIZETYPE
    \macro PRIiQSIZETYPE
    \since 6.2
    \relates <QtTypes>

    See qsizetype.
*/

/*! \macro qint64 Q_INT64_C(literal)
    \relates <QtTypes>

    Wraps the signed 64-bit integer \a literal in a
    platform-independent way.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 8

    \sa qint64, Q_UINT64_C(), Q_INT128_C()
*/

/*! \macro quint64 Q_UINT64_C(literal)
    \relates <QtTypes>

    Wraps the unsigned 64-bit integer \a literal in a
    platform-independent way.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 9

    \sa quint64, Q_INT64_C(), Q_UINT128_C()
*/

/*!
    \macro qint128 Q_INT128_C(literal)
    \relates <QtTypes>
    \since 6.6

    Wraps the signed 128-bit integer \a literal in a
    platform-independent way.

    \note Unlike Q_INT64_C(), this macro is only available in C++, not in C.
    This is because compilers do not provide these literals as built-ins and C
    does not have support for user-defined literals.

    \sa qint128, Q_UINT128_C(), Q_INT128_MIN, Q_INT128_MAX, Q_INT64_C(), QT_SUPPORTS_INT128
*/

/*!
    \macro quint128 Q_UINT128_C(literal)
    \relates <QtTypes>
    \since 6.6

    Wraps the unsigned 128-bit integer \a literal in a
    platform-independent way.

    \note Unlike Q_UINT64_C(), this macro is only available in C++, not in C.
    This is because compilers do not provide these literals as built-ins and C
    does not have support for user-defined literals.

    \sa quint128, Q_INT128_C(), Q_UINT128_MAX, Q_UINT64_C(), QT_SUPPORTS_INT128
*/

/*!
    \macro Q_UINT128_MAX
    \relates <QtTypes>
    \since 6.6

    This macro expands to a compile-time constant representing the
    maximum value representable in a \l quint128.

    This macro is available in both C++ and C modes.

    The minimum of \l quint128 is 0 (zero), so a \c{Q_UINT128_MIN} is neither
    needed nor provided.

    \sa Q_INT128_MAX, quint128, Q_UINT128_C, QT_SUPPORTS_INT128
*/

/*!
    \macro Q_INT128_MIN
    \relates <QtTypes>
    \since 6.6

    This macro expands to a compile-time constant representing the
    minimum value representable in a \l qint128.

    This macro is available in both C++ and C modes.

    \sa Q_INT128_MAX, qint128, Q_INT128_C, QT_SUPPORTS_INT128
*/

/*!
    \macro Q_INT128_MAX
    \relates <QtTypes>
    \since 6.6

    This macro expands to a compile-time constant representing the
    maximum value representable in a \l qint128.

    This macro is available in both C++ and C modes.

    \sa Q_INT128_MIN, Q_UINT128_MAX, qint128, Q_INT128_C, QT_SUPPORTS_INT128
*/

// Statically check assumptions about the environment we're running
// in. The idea here is to error or warn if otherwise implicit Qt
// assumptions are not fulfilled on new hardware or compilers
// (if this list becomes too long, consider factoring into a separate file)
static_assert(UCHAR_MAX == 255, "Qt assumes that char is 8 bits");
static_assert(sizeof(int) == 4, "Qt assumes that int is 32 bits");
static_assert(QT_POINTER_SIZE == sizeof(void *), "QT_POINTER_SIZE defined incorrectly");
static_assert(sizeof(float) == 4, "Qt assumes that float is 32 bits");
static_assert(sizeof(char16_t) == 2, "Qt assumes that char16_t is 16 bits");
static_assert(sizeof(char32_t) == 4, "Qt assumes that char32_t is 32 bits");
#if defined(Q_OS_WIN)
static_assert(sizeof(wchar_t) == sizeof(char16_t));
#endif
static_assert(std::numeric_limits<int>::radix == 2,
                  "Qt assumes binary integers");
static_assert((std::numeric_limits<int>::max() + std::numeric_limits<int>::lowest()) == -1,
                  "Qt assumes two's complement integers");
static_assert(sizeof(wchar_t) == sizeof(char32_t) || sizeof(wchar_t) == sizeof(char16_t),
              "Qt assumes wchar_t is compatible with either char32_t or char16_t");

// While we'd like to check for __STDC_IEC_559__, as per ISO/IEC 9899:2011
// Annex F (C11, normative for C++11), there are a few corner cases regarding
// denormals where GHS compiler is relying hardware behavior that is not IEC
// 559 compliant. So split the check in several subchecks.

// On GHS the compiler reports std::numeric_limits<float>::is_iec559 as false.
// This is all right according to our needs.
#if !defined(Q_CC_GHS)
static_assert(std::numeric_limits<float>::is_iec559,
                  "Qt assumes IEEE 754 floating point");
#endif

// Technically, presence of NaN and infinities are implied from the above check,
// but double checking our environment doesn't hurt...
static_assert(std::numeric_limits<float>::has_infinity &&
                  std::numeric_limits<float>::has_quiet_NaN,
                  "Qt assumes IEEE 754 floating point");

// is_iec559 checks for ISO/IEC/IEEE 60559:2011 (aka IEEE 754-2008) compliance,
// but that allows for a non-binary radix. We need to recheck that.
// Note how __STDC_IEC_559__ would instead check for IEC 60559:1989, aka
// ANSI/IEEE 754−1985, which specifically implies binary floating point numbers.
static_assert(std::numeric_limits<float>::radix == 2,
                  "Qt assumes binary IEEE 754 floating point");

// not required by the definition of size_t, but we depend on this
static_assert(sizeof(size_t) == sizeof(void *), "size_t and a pointer don't have the same size");
static_assert(sizeof(size_t) == sizeof(qsizetype)); // implied by the definition
static_assert((std::is_same<qsizetype, qptrdiff>::value));
static_assert(std::is_same_v<std::size_t, size_t>);

// Check that our own typedefs are not broken.
static_assert(sizeof(qint8) == 1, "Internal error, qint8 is misdefined");
static_assert(sizeof(qint16)== 2, "Internal error, qint16 is misdefined");
static_assert(sizeof(qint32) == 4, "Internal error, qint32 is misdefined");
static_assert(sizeof(qint64) == 8, "Internal error, qint64 is misdefined");
#ifdef QT_SUPPORTS_INT128
static_assert(sizeof(qint128) == 16, "Internal error, qint128 is misdefined");
#endif

#ifdef QT_SUPPORTS_INT128
// Standard Library supports for 128-bit integers:
//  Implementation      | Version | Note
// ---------------------|---------|------
//  GNU libstdc++       | 11.1.0  |
//  LLVM libc++         | 3.5     | May change if compiler has __is_integral()
//  MS STL              | none    |

#  if defined(_LIBCPP_VERSION) || (defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE >= 11)
static_assert(std::numeric_limits<quint128>::max() == Q_UINT128_MAX);
#  endif
#endif

QT_END_NAMESPACE
