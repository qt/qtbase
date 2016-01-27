/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2015 Intel Corporation.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

#include "QtCore/qglobal.h"

#include <limits>

#if defined(Q_CC_MSVC) && !defined(Q_OS_WINCE)
#  include <intrin.h>
#elif defined(Q_CC_INTEL)
#  include <immintrin.h>    // for _addcarry_u<nn>
#endif

#ifndef __has_builtin
#  define __has_builtin(x)  0
#endif

QT_BEGIN_NAMESPACE

#if !defined(Q_CC_MIPS)

static const union { unsigned char c[8]; double d; } qt_be_inf_bytes = { { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 } };
static const union { unsigned char c[8]; double d; } qt_le_inf_bytes = { { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f } };
static inline double qt_inf()
{
    return (QSysInfo::ByteOrder == QSysInfo::BigEndian
            ? qt_be_inf_bytes.d
            : qt_le_inf_bytes.d);
}

// Signaling NAN
static const union { unsigned char c[8]; double d; } qt_be_snan_bytes = { { 0x7f, 0xf8, 0, 0, 0, 0, 0, 0 } };
static const union { unsigned char c[8]; double d; } qt_le_snan_bytes = { { 0, 0, 0, 0, 0, 0, 0xf8, 0x7f } };
static inline double qt_snan()
{
    return (QSysInfo::ByteOrder == QSysInfo::BigEndian
            ? qt_be_snan_bytes.d
            : qt_le_snan_bytes.d);
}

// Quiet NAN
static const union { unsigned char c[8]; double d; } qt_be_qnan_bytes = { { 0xff, 0xf8, 0, 0, 0, 0, 0, 0 } };
static const union { unsigned char c[8]; double d; } qt_le_qnan_bytes = { { 0, 0, 0, 0, 0, 0, 0xf8, 0xff } };
static inline double qt_qnan()
{
    return (QSysInfo::ByteOrder == QSysInfo::BigEndian
            ? qt_be_qnan_bytes.d
            : qt_le_qnan_bytes.d);
}

#else // Q_CC_MIPS

static const unsigned char qt_be_inf_bytes[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
static const unsigned char qt_le_inf_bytes[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
static inline double qt_inf()
{
    const unsigned char *bytes;
    bytes = (QSysInfo::ByteOrder == QSysInfo::BigEndian
             ? qt_be_inf_bytes
             : qt_le_inf_bytes);

    union { unsigned char c[8]; double d; } returnValue;
    memcpy(returnValue.c, bytes, sizeof(returnValue.c));
    return returnValue.d;
}

// Signaling NAN
static const unsigned char qt_be_snan_bytes[] = { 0x7f, 0xf8, 0, 0, 0, 0, 0, 0 };
static const unsigned char qt_le_snan_bytes[] = { 0, 0, 0, 0, 0, 0, 0xf8, 0x7f };
static inline double qt_snan()
{
    const unsigned char *bytes;
    bytes = (QSysInfo::ByteOrder == QSysInfo::BigEndian
             ? qt_be_snan_bytes
             : qt_le_snan_bytes);

    union { unsigned char c[8]; double d; } returnValue;
    memcpy(returnValue.c, bytes, sizeof(returnValue.c));
    return returnValue.d;
}

// Quiet NAN
static const unsigned char qt_be_qnan_bytes[] = { 0xff, 0xf8, 0, 0, 0, 0, 0, 0 };
static const unsigned char qt_le_qnan_bytes[] = { 0, 0, 0, 0, 0, 0, 0xf8, 0xff };
static inline double qt_qnan()
{
    const unsigned char *bytes;
    bytes = (QSysInfo::ByteOrder == QSysInfo::BigEndian
             ? qt_be_qnan_bytes
             : qt_le_qnan_bytes);

    union { unsigned char c[8]; double d; } returnValue;
    memcpy(returnValue.c, bytes, sizeof(returnValue.c));
    return returnValue.d;
}

#endif // Q_CC_MIPS

static inline bool qt_is_inf(double d)
{
    uchar *ch = (uchar *)&d;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        return (ch[0] & 0x7f) == 0x7f && ch[1] == 0xf0;
    } else {
        return (ch[7] & 0x7f) == 0x7f && ch[6] == 0xf0;
    }
}

static inline bool qt_is_nan(double d)
{
    uchar *ch = (uchar *)&d;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        return (ch[0] & 0x7f) == 0x7f && ch[1] > 0xf0;
    } else {
        return (ch[7] & 0x7f) == 0x7f && ch[6] > 0xf0;
    }
}

static inline bool qt_is_finite(double d)
{
    uchar *ch = (uchar *)&d;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        return (ch[0] & 0x7f) != 0x7f || (ch[1] & 0xf0) != 0xf0;
    } else {
        return (ch[7] & 0x7f) != 0x7f || (ch[6] & 0xf0) != 0xf0;
    }
}

static inline bool qt_is_inf(float d)
{
    uchar *ch = (uchar *)&d;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        return (ch[0] & 0x7f) == 0x7f && ch[1] == 0x80;
    } else {
        return (ch[3] & 0x7f) == 0x7f && ch[2] == 0x80;
    }
}

static inline bool qt_is_nan(float d)
{
    uchar *ch = (uchar *)&d;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        return (ch[0] & 0x7f) == 0x7f && ch[1] > 0x80;
    } else {
        return (ch[3] & 0x7f) == 0x7f && ch[2] > 0x80;
    }
}

static inline bool qt_is_finite(float d)
{
    uchar *ch = (uchar *)&d;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        return (ch[0] & 0x7f) != 0x7f || (ch[1] & 0x80) != 0x80;
    } else {
        return (ch[3] & 0x7f) != 0x7f || (ch[2] & 0x80) != 0x80;
    }
}

//
// Overflow math
//
namespace {
template <typename T> inline typename QtPrivate::QEnableIf<QtPrivate::is_unsigned<T>::value, bool>::Type
add_overflow(T v1, T v2, T *r)
{
    // unsigned additions are well-defined
    *r = v1 + v2;
    return v1 > T(v1 + v2);
}

template <typename T> inline typename QtPrivate::QEnableIf<QtPrivate::is_unsigned<T>::value, bool>::Type
mul_overflow(T v1, T v2, T *r)
{
    // use the next biggest type
    // Note: for 64-bit systems where __int128 isn't supported, this will cause an error.
    // A fallback is present below.
    typedef typename QIntegerForSize<sizeof(T) * 2>::Unsigned Larger;
    Larger lr = Larger(v1) * Larger(v2);
    *r = T(lr);
    return lr > std::numeric_limits<T>::max();
}

#if defined(__SIZEOF_INT128__)
#  define HAVE_MUL64_OVERFLOW
#endif

// GCC 5 and Clang have builtins to detect overflows
#if (defined(Q_CC_GNU) && !defined(Q_CC_INTEL) && Q_CC_GNU >= 500) || __has_builtin(__builtin_uadd_overflow)
template <> inline bool add_overflow(unsigned v1, unsigned v2, unsigned *r)
{ return __builtin_uadd_overflow(v1, v2, r); }
#endif
#if (defined(Q_CC_GNU) && !defined(Q_CC_INTEL) && Q_CC_GNU >= 500) || __has_builtin(__builtin_uaddl_overflow)
template <> inline bool add_overflow(unsigned long v1, unsigned long v2, unsigned long *r)
{ return __builtin_uaddl_overflow(v1, v2, r); }
#endif
#if (defined(Q_CC_GNU) && !defined(Q_CC_INTEL) && Q_CC_GNU >= 500) || __has_builtin(__builtin_uaddll_overflow)
template <> inline bool add_overflow(unsigned long long v1, unsigned long long v2, unsigned long long *r)
{ return __builtin_uaddll_overflow(v1, v2, r); }
#endif

#if (defined(Q_CC_GNU) && !defined(Q_CC_INTEL) && Q_CC_GNU >= 500) || __has_builtin(__builtin_umul_overflow)
template <> inline bool mul_overflow(unsigned v1, unsigned v2, unsigned *r)
{ return __builtin_umul_overflow(v1, v2, r); }
#endif
#if (defined(Q_CC_GNU) && !defined(Q_CC_INTEL) && Q_CC_GNU >= 500) || __has_builtin(__builtin_umull_overflow)
template <> inline bool mul_overflow(unsigned long v1, unsigned long v2, unsigned long *r)
{ return __builtin_umull_overflow(v1, v2, r); }
#endif
#if (defined(Q_CC_GNU) && !defined(Q_CC_INTEL) && Q_CC_GNU >= 500) || __has_builtin(__builtin_umulll_overflow)
template <> inline bool mul_overflow(unsigned long long v1, unsigned long long v2, unsigned long long *r)
{ return __builtin_umulll_overflow(v1, v2, r); }
#  define HAVE_MUL64_OVERFLOW
#endif

#if ((defined(Q_CC_MSVC) && _MSC_VER >= 1800) || defined(Q_CC_INTEL)) && defined(Q_PROCESSOR_X86)
template <> inline bool add_overflow(unsigned v1, unsigned v2, unsigned *r)
{ return _addcarry_u32(0, v1, v2, r); }
#  ifdef Q_CC_MSVC      // longs are 32-bit
template <> inline bool add_overflow(unsigned long v1, unsigned long v2, unsigned long *r)
{ return _addcarry_u32(0, v1, v2, reinterpret_cast<unsigned *>(r)); }
#  endif
#endif
#if ((defined(Q_CC_MSVC) && _MSC_VER >= 1800) || defined(Q_CC_INTEL)) && defined(Q_PROCESSOR_X86_64)
template <> inline bool add_overflow(quint64 v1, quint64 v2, quint64 *r)
{ return _addcarry_u64(0, v1, v2, reinterpret_cast<unsigned __int64 *>(r)); }
#  ifndef Q_CC_MSVC      // longs are 64-bit
template <> inline bool add_overflow(unsigned long v1, unsigned long v2, unsigned long *r)
{ return _addcarry_u64(0, v1, v2, reinterpret_cast<unsigned __int64 *>(r)); }
#  endif
#endif

#if defined(Q_CC_MSVC) && (defined(Q_PROCESSOR_X86_64) || defined(Q_PROCESSOR_IA64))
#pragma intrinsic(_umul128)
template <> inline bool mul_overflow(quint64 v1, quint64 v2, quint64 *r)
{
    // use 128-bit multiplication with the _umul128 intrinsic
    // https://msdn.microsoft.com/en-us/library/3dayytw9.aspx
    quint64 high;
    *r = _umul128(v1, v2, &high);
    return high;
}
#  define HAVE_MUL64_OVERFLOW
#endif

#if !defined(HAVE_MUL64_OVERFLOW) && defined(__LP64__)
// no 128-bit multiplication, we need to figure out with a slow division
template <> inline bool mul_overflow(quint64 v1, quint64 v2, quint64 *r)
{
    if (v2 && v1 > std::numeric_limits<quint64>::max() / v2)
        return true;
    *r = v1 * v2;
    return false;
}
template <> inline bool mul_overflow(unsigned long v1, unsigned long v2, unsigned long *r)
{
    return mul_overflow<quint64>(v1, v2, reinterpret_cast<quint64 *>(r));
}
#else
#  undef HAVE_MUL64_OVERFLOW
#endif
}

QT_END_NAMESPACE

#endif // QNUMERIC_P_H
