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

#include "qendian.h"

#include "qalgorithms.h"
#include <private/qsimd_p.h>

QT_BEGIN_NAMESPACE

/*!
    \headerfile <QtEndian>
    \title Endian Conversion Functions
    \ingroup funclists
    \brief The <QtEndian> header provides functions to convert between
    little and big endian representations of numbers.
*/

/*!
    \fn template <typename T> T qFromUnaligned(const void *ptr)
    \internal
    \since 5.5

    Loads a \c{T} from address \a ptr, which may be misaligned.

    Use of this function avoids the undefined behavior that the C++ standard
    otherwise attributes to unaligned loads.
*/

/*!
    \fn template <typename T> void qToUnaligned(const T t, void *ptr)
    \internal
    \since 4.5

    Stores \a t to address \a ptr, which may be misaligned.

    Use of this function avoids the undefined behavior that the C++ standard
    otherwise attributes to unaligned stores.
*/


/*!
    \fn template <typename T> T qFromBigEndian(const void *src)
    \since 4.3
    \relates <QtEndian>

    Reads a big-endian number from memory location \a src and returns the number in the
    host byte order representation.
    On CPU architectures where the host byte order is little-endian (such as x86) this
    will swap the byte order; otherwise it will just read from \a src.

    \note Template type \c{T} can either be a quint16, qint16, quint32, qint32,
    quint64, or qint64. Other types of integers, e.g., qlong, are not
    applicable.

    \note Since Qt 5.7, the type of the \a src parameter is a void pointer.

    There are no data alignment constraints for \a src.

    \sa qFromLittleEndian()
    \sa qToBigEndian()
    \sa qToLittleEndian()
*/
/*!
    \fn template <typename T> T qFromBigEndian(T src)
    \since 4.3
    \relates <QtEndian>
    \overload

    Converts \a src from big-endian byte order and returns the number in host byte order
    representation of that number.
    On CPU architectures where the host byte order is little-endian (such as x86) this
    will return \a src with the byte order swapped; otherwise it will return \a src
    unmodified.
*/
/*!
    \fn template <typename T> T qFromBigEndian(const void *src, qsizetype count, void *dest)
    \since 5.12
    \relates <QtEndian>

    Reads \a count big-endian numbers from memory location \a src and stores
    them in the host byte order representation at \a dest. On CPU architectures
    where the host byte order is little-endian (such as x86) this will swap the
    byte order; otherwise it will just perform a \c memcpy from \a src to \a
    dest.

    \note Template type \c{T} can either be a quint16, qint16, quint32, qint32,
    quint64, or qint64. Other types of integers, e.g., qlong, are not
    applicable.

    There are no data alignment constraints for \a src. However, \a dest is
    expected to be naturally aligned for type \c{T}.

    If \a src and \a dest can be the same pointer, this function will perform
    an in-place swap (if necessary). If they are not the same, the memory
    regions must not overlap.

    \sa qFromLittleEndian()
    \sa qToBigEndian()
    \sa qToLittleEndian()
*/
/*!
    \fn template <typename T> inline T qFromLittleEndian(const void *src)
    \since 4.3
    \relates <QtEndian>

    Reads a little-endian number from memory location \a src and returns the number in
    the host byte order representation.
    On CPU architectures where the host byte order is big-endian (such as PowerPC) this
    will swap the byte order; otherwise it will just read from \a src.

    \note Template type \c{T} can either be a quint16, qint16, quint32, qint32,
    quint64, or qint64. Other types of integers, e.g., qlong, are not
    applicable.

    \note Since Qt 5.7, the type of the \a src parameter is a void pointer.

    There are no data alignment constraints for \a src.

    \sa qFromBigEndian()
    \sa qToBigEndian()
    \sa qToLittleEndian()
*/
/*!
    \fn template <typename T> inline T qFromLittleEndian(T src)
    \since 4.3
    \relates <QtEndian>
    \overload

    Converts \a src from little-endian byte order and returns the number in host byte
    order representation of that number.
    On CPU architectures where the host byte order is big-endian (such as PowerPC) this
    will return \a src with the byte order swapped; otherwise it will return \a src
    unmodified.
*/
/*!
    \fn template <typename T> inline T qFromLittleEndian(const void *src, qsizetype count, void *dest)
    \since 5.12
    \relates <QtEndian>

    Reads \a count little-endian numbers from memory location \a src and stores
    them in the host byte order representation at \a dest. On CPU architectures
    where the host byte order is big-endian (such as PowerPC) this will swap the
    byte order; otherwise it will just perform a \c memcpy from \a src to \a
    dest.

    \note Template type \c{T} can either be a quint16, qint16, quint32, qint32,
    quint64, or qint64. Other types of integers, e.g., qlong, are not
    applicable.

    There are no data alignment constraints for \a src. However, \a dest is
    expected to be naturally aligned for type \c{T}.

    If \a src and \a dest can be the same pointer, this function will perform
    an in-place swap (if necessary). If they are not the same, the memory
    regions must not overlap.

    \sa qToBigEndian()
    \sa qToLittleEndian()
*/
/*!
    \fn template <typename T> void qToBigEndian(T src, void *dest)
    \since 4.3
    \relates <QtEndian>

    Writes the number \a src with template type \c{T} to the memory location at \a dest
    in big-endian byte order.

    \note Template type \c{T} can either be a quint16, qint16, quint32, qint32,
    quint64, or qint64. Other types of integers, e.g., qlong, are not
    applicable.

    There are no data alignment constraints for \a dest.

    \note Since Qt 5.7, the type of the \a dest parameter is a void pointer.

    \sa qFromBigEndian()
    \sa qFromLittleEndian()
    \sa qToLittleEndian()
*/
/*!
    \fn template <typename T> T qToBigEndian(T src)
    \since 4.3
    \relates <QtEndian>
    \overload

    Converts \a src from host byte order and returns the number in big-endian byte order
    representation of that number.
    On CPU architectures where the host byte order is little-endian (such as x86) this
    will return \a src with the byte order swapped; otherwise it will return \a src
    unmodified.
*/
/*!
    \fn template <typename T> T qToBigEndian(const void *src, qsizetype count, void *dest)
    \since 5.12
    \relates <QtEndian>

    Reads \a count numbers from memory location \a src in the host byte order
    and stores them in big-endian representation at \a dest. On CPU
    architectures where the host byte order is little-endian (such as x86) this
    will swap the byte order; otherwise it will just perform a \c memcpy from
    \a src to \a dest.

    \note Template type \c{T} can either be a quint16, qint16, quint32, qint32,
    quint64, or qint64. Other types of integers, e.g., qlong, are not
    applicable.

    There are no data alignment constraints for \a dest. However, \a src is
    expected to be naturally aligned for type \c{T}.

    If \a src and \a dest can be the same pointer, this function will perform
    an in-place swap (if necessary). If they are not the same, the memory
    regions must not overlap.

    \sa qFromLittleEndian()
    \sa qToBigEndian()
    \sa qToLittleEndian()
*/
/*!
    \fn template <typename T> void qToLittleEndian(T src, void *dest)
    \since 4.3
    \relates <QtEndian>

    Writes the number \a src with template type \c{T} to the memory location at \a dest
    in little-endian byte order.

    \note Template type \c{T} can either be a quint16, qint16, quint32, qint32,
    quint64, or qint64. Other types of integers, e.g., qlong, are not
    applicable.

    There are no data alignment constraints for \a dest.

    \note Since Qt 5.7, the type of the \a dest parameter is a void pointer.

    \sa qFromBigEndian()
    \sa qFromLittleEndian()
    \sa qToBigEndian()
*/
/*!
    \fn template <typename T> T qToLittleEndian(T src)
    \since 4.3
    \relates <QtEndian>
    \overload

    Converts \a src from host byte order and returns the number in little-endian byte
    order representation of that number.
    On CPU architectures where the host byte order is big-endian (such as PowerPC) this
    will return \a src with the byte order swapped; otherwise it will return \a src
    unmodified.
*/
/*!
    \fn template <typename T> T qToLittleEndian(const void *src, qsizetype count, void *dest)
    \since 5.12
    \relates <QtEndian>

    Reads \a count numbers from memory location \a src in the host byte order
    and stores them in little-endian representation at \a dest. On CPU
    architectures where the host byte order is big-endian (such as PowerPC)
    this will swap the byte order; otherwise it will just perform a \c memcpy
    from \a src to \a dest.

    \note Template type \c{T} can either be a quint16, qint16, quint32, qint32,
    quint64, or qint64. Other types of integers, e.g., qlong, are not
    applicable.

    There are no data alignment constraints for \a dest. However, \a src is
    expected to be naturally aligned for type \c{T}.

    If \a src and \a dest can be the same pointer, this function will perform
    an in-place swap (if necessary). If they are not the same, the memory
    regions must not overlap.

    \sa qFromLittleEndian()
    \sa qToBigEndian()
    \sa qToLittleEndian()
*/

/*!
    \class QLEInteger
    \inmodule QtCore
    \brief The QLEInteger class provides platform-independent little-endian integers.
    \since 5.10

    The template parameter \c T must be a C++ integer type:
    \list
       \li 8-bit: char, signed char, unsigned char, qint8, quint8
       \li 16-bit: short, unsigned short, qint16, quint16, char16_t
       \li 32-bit: int, unsigned int, qint32, quint32, char32_t
       \li 64-bit: long long, unsigned long long, qint64, quint64
       \li platform-specific size: long, unsigned long
       \li pointer size: qintptr, quintptr, qptrdiff
    \endlist

    \note Using this class may be slower than using native integers, so only use it when
    an exact endianness is needed.
*/

/*! \fn template <typename T> QLEInteger<T>::QLEInteger(T value)

    Constructs a QLEInteger with the given \a value.
*/

/*! \fn template <typename T> QLEInteger &QLEInteger<T>::operator=(T i)

    Assigns \a i to this QLEInteger and returns a reference to
    this QLEInteger.
*/

/*!
    \fn template <typename T> QLEInteger<T>::operator T() const

    Returns the value of this QLEInteger as a native integer.
*/

/*!
    \fn template <typename T> bool QLEInteger<T>::operator==(QLEInteger other) const

    Returns \c true if the value of this QLEInteger is equal to the value of \a other.
*/

/*!
    \fn template <typename T> bool QLEInteger<T>::operator!=(QLEInteger other) const

    Returns \c true if the value of this QLEInteger is not equal to the value of \a other.
*/

/*!
    \fn template <typename T> QLEInteger &QLEInteger<T>::operator+=(T i)

    Adds \a i to this QLEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QLEInteger &QLEInteger<T>::operator-=(T i)

    Subtracts \a i from this QLEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QLEInteger &QLEInteger<T>::operator*=(T i)

    Multiplies \a i with this QLEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QLEInteger &QLEInteger<T>::operator/=(T i)

    Divides this QLEInteger with \a i and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QLEInteger &QLEInteger<T>::operator%=(T i)

    Sets this QLEInteger to the remainder of a division by \a i and
    returns a reference to this object.
*/

/*!
    \fn template <typename T> QLEInteger &QLEInteger<T>::operator>>=(T i)

    Performs a left-shift by \a i on this QLEInteger and returns a
    reference to this object.
*/

/*!
    \fn template <typename T> QLEInteger &QLEInteger<T>::operator<<=(T i)

    Performs a right-shift by \a i on this QLEInteger and returns a
    reference to this object.
*/

/*!
    \fn template <typename T> QLEInteger &QLEInteger<T>::operator|=(T i)

    Performs a bitwise OR with \a i onto this QLEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QLEInteger &QLEInteger<T>::operator&=(T i)

    Performs a bitwise AND with \a i onto this QLEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QLEInteger &QLEInteger<T>::operator^=(T i)

    Performs a bitwise XOR with \a i onto this QLEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QLEInteger &QLEInteger<T>::operator++()

    Performs a prefix ++ (increment) on this QLEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QLEInteger QLEInteger<T>::operator++(int)

    Performs a postfix ++ (increment) on this QLEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QLEInteger &QLEInteger<T>::operator--()

    Performs a prefix -- (decrement) on this QLEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QLEInteger QLEInteger<T>::operator--(int)

    Performs a postfix -- (decrement) on this QLEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QLEInteger QLEInteger<T>::max()

    Returns the maximum (finite) value representable by the numeric type T.
*/

/*!
    \fn template <typename T> QLEInteger QLEInteger<T>::min()

    Returns the minimum (finite) value representable by the numeric type T.
*/

/*!
    \class QBEInteger
    \inmodule QtCore
    \brief The QBEInteger class provides platform-independent big-endian integers.
    \since 5.10

    The template parameter \c T must be a C++ integer type:
    \list
       \li 8-bit: char, signed char, unsigned char, qint8, quint8
       \li 16-bit: short, unsigned short, qint16, quint16, char16_t (C++11)
       \li 32-bit: int, unsigned int, qint32, quint32, char32_t (C++11)
       \li 64-bit: long long, unsigned long long, qint64, quint64
       \li platform-specific size: long, unsigned long
       \li pointer size: qintptr, quintptr, qptrdiff
    \endlist

    \note Using this class may be slower than using native integers, so only use it when
    an exact endianness is needed.
*/

/*! \fn template <typename T> QBEInteger<T>::QBEInteger(T value)

    Constructs a QBEInteger with the given \a value.
*/

/*! \fn template <typename T> QBEInteger &QBEInteger<T>::operator=(T i)

    Assigns \a i to this QBEInteger and returns a reference to
    this QBEInteger.
*/

/*!
    \fn template <typename T> QBEInteger<T>::operator T() const

    Returns the value of this QBEInteger as a native integer.
*/

/*!
    \fn template <typename T> bool QBEInteger<T>::operator==(QBEInteger other) const

    Returns \c true if the value of this QBEInteger is equal to the value of \a other.
*/

/*!
    \fn template <typename T> bool QBEInteger<T>::operator!=(QBEInteger other) const

    Returns \c true if the value of this QBEInteger is not equal to the value of \a other.
*/

/*!
    \fn template <typename T> QBEInteger &QBEInteger<T>::operator+=(T i)

    Adds \a i to this QBEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QBEInteger &QBEInteger<T>::operator-=(T i)

    Subtracts \a i from this QBEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QBEInteger &QBEInteger<T>::operator*=(T i)

    Multiplies \a i with this QBEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QBEInteger &QBEInteger<T>::operator/=(T i)

    Divides this QBEInteger with \a i and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QBEInteger &QBEInteger<T>::operator%=(T i)

    Sets this QBEInteger to the remainder of a division by \a i and
    returns a reference to this object.
*/

/*!
    \fn template <typename T> QBEInteger &QBEInteger<T>::operator>>=(T i)

    Performs a left-shift by \a i on this QBEInteger and returns a
    reference to this object.
*/

/*!
    \fn template <typename T> QBEInteger &QBEInteger<T>::operator<<=(T i)

    Performs a right-shift by \a i on this QBEInteger and returns a
    reference to this object.
*/

/*!
    \fn template <typename T> QBEInteger &QBEInteger<T>::operator|=(T i)

    Performs a bitwise OR with \a i onto this QBEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QBEInteger &QBEInteger<T>::operator&=(T i)

    Performs a bitwise AND with \a i onto this QBEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QBEInteger &QBEInteger<T>::operator^=(T i)

    Performs a bitwise XOR with \a i onto this QBEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QBEInteger &QBEInteger<T>::operator++()

    Performs a prefix ++ (increment) on this QBEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QBEInteger QBEInteger<T>::operator++(int)

    Performs a postfix ++ (increment) on this QBEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QBEInteger &QBEInteger<T>::operator--()

    Performs a prefix -- (decrement) on this QBEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QBEInteger QBEInteger<T>::operator--(int)

    Performs a postfix -- (decrement) on this QBEInteger and returns a reference to
    this object.
*/

/*!
    \fn template <typename T> QBEInteger QBEInteger<T>::max()

    Returns the maximum (finite) value representable by the numeric type T.
*/

/*!
    \fn template <typename T> QBEInteger QBEInteger<T>::min()

    Returns the minimum (finite) value representable by the numeric type T.
*/

/*!
    \typedef quint16_le
    \relates <QtEndian>
    \since 5.10

    Typedef for QLEInteger<quint16>. This type is guaranteed to be stored in memory as
    a 16-bit little-endian unsigned integer on all platforms supported by Qt.

    \sa quint16
*/

/*!
    \typedef quint32_le
    \relates <QtEndian>
    \since 5.10

    Typedef for QLEInteger<quint32>. This type is guaranteed to be stored in memory as
    a 32-bit little-endian unsigned integer on all platforms supported by Qt.

    \sa quint32
*/

/*!
    \typedef quint64_le
    \relates <QtEndian>
    \since 5.10

    Typedef for QLEInteger<quint64>. This type is guaranteed to be stored in memory as
    a 64-bit little-endian unsigned integer on all platforms supported by Qt.

    \sa quint64
*/

/*!
    \typedef quint16_be
    \relates <QtEndian>
    \since 5.10

    Typedef for QBEInteger<quint16>. This type is guaranteed to be stored in memory as
    a 16-bit big-endian unsigned integer on all platforms supported by Qt.

    \sa quint16
*/

/*!
    \typedef quint32_be
    \relates <QtEndian>
    \since 5.10

    Typedef for QBEInteger<quint32>. This type is guaranteed to be stored in memory as
    a 32-bit big-endian unsigned integer on all platforms supported by Qt.

    \sa quint32
*/

/*!
    \typedef quint64_be
    \relates <QtEndian>
    \since 5.10

    Typedef for QBEInteger<quint64>. This type is guaranteed to be stored in memory as
    a 64-bit big-endian unsigned integer on all platforms supported by Qt.

    \sa quint64
*/

/*!
    \typedef qint16_le
    \relates <QtEndian>
    \since 5.10

    Typedef for QLEInteger<qint16>. This type is guaranteed to be stored in memory as
    a 16-bit little-endian signed integer on all platforms supported by Qt.

    \sa qint16
*/

/*!
    \typedef qint32_le
    \relates <QtEndian>
    \since 5.10

    Typedef for QLEInteger<qint32>. This type is guaranteed to be stored in memory as
    a 32-bit little-endian signed integer on all platforms supported by Qt.

    \sa qint32
*/

/*!
    \typedef qint64_le
    \relates <QtEndian>
    \since 5.10

    Typedef for QLEInteger<qint64>. This type is guaranteed to be stored in memory as
    a 64-bit little-endian signed integer on all platforms supported by Qt.

    \sa qint64
*/

/*!
    \typedef qint16_be
    \relates <QtEndian>
    \since 5.10

    Typedef for QBEInteger<qint16>. This type is guaranteed to be stored in memory as
    a 16-bit big-endian signed integer on all platforms supported by Qt.

    \sa qint16
*/

/*!
    \typedef qint32_be
    \relates <QtEndian>
    \since 5.10

    Typedef for QBEInteger<qint32>. This type is guaranteed to be stored in memory as
    a 32-bit big-endian signed integer on all platforms supported by Qt.

    \sa qint32
*/

/*!
    \typedef qint64_be
    \relates <QtEndian>
    \since 5.10

    Typedef for QBEInteger<qint64>. This type is guaranteed to be stored in memory as
    a 64-bit big-endian signed integer on all platforms supported by Qt.

    \sa qint64
*/

#if defined(__SSSE3__)
using ShuffleMask = uchar[16];
Q_DECL_ALIGN(16) static const ShuffleMask shuffleMasks[3] = {
    // 16-bit
    {1, 0, 3, 2,  5, 4, 7, 6,  9, 8, 11, 10,  13, 12, 15, 14},
    // 32-bit
    {3, 2, 1, 0,  7, 6, 5, 4,  11, 10, 9, 8,  15, 14, 13, 12},
    // 64-bit
    {7, 6, 5, 4, 3, 2, 1, 0,   15, 14, 13, 12, 11, 10, 9, 8}
};

static size_t sseSwapLoop(const uchar *src, size_t bytes, uchar *dst,
                          const __m128i *shuffleMaskPtr) noexcept
{
    size_t i = 0;
    const __m128i shuffleMask = _mm_load_si128(shuffleMaskPtr);

#  ifdef __AVX2__
    const __m256i shuffleMask256 = _mm256_inserti128_si256(_mm256_castsi128_si256(shuffleMask), shuffleMask, 1);
    for ( ; i + sizeof(__m256i) <= bytes; i += sizeof(__m256i)) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + i));
        data = _mm256_shuffle_epi8(data, shuffleMask256);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst + i), data);
    }
#  else
    for ( ; i + 2 * sizeof(__m128i) <= bytes; i += 2 * sizeof(__m128i)) {
        __m128i data1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i));
        __m128i data2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i) + 1);
        data1 = _mm_shuffle_epi8(data1, shuffleMask);
        data2 = _mm_shuffle_epi8(data2, shuffleMask);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + i), data1);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + i) + 1, data2);
    }
#  endif

    if (i + sizeof(__m128i) <= bytes) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i));
        data = _mm_shuffle_epi8(data, shuffleMask);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + i), data);
        i += sizeof(__m128i);
    }

    return i;
}

template <typename T> static Q_ALWAYS_INLINE
size_t simdSwapLoop(const uchar *src, size_t bytes, uchar *dst) noexcept
{
    auto shuffleMaskPtr = reinterpret_cast<const __m128i *>(shuffleMasks[0]);
    shuffleMaskPtr += qCountTrailingZeroBits(sizeof(T)) - 1;
    size_t i = sseSwapLoop(src, bytes, dst, shuffleMaskPtr);

    // epilogue
    for (size_t _i = 0 ; i < bytes && _i < sizeof(__m128i); i += sizeof(T), _i += sizeof(T))
        qbswap(qFromUnaligned<T>(src + i), dst + i);

    // return the total, so the bswapLoop below does nothing
    return bytes;
}
#elif defined(__SSE2__)
template <typename T> static
size_t simdSwapLoop(const uchar *, size_t, uchar *) noexcept
{
    // no generic version: we can't do 32- and 64-bit swaps easily,
    // so we won't try
    return 0;
}

template <> size_t simdSwapLoop<quint16>(const uchar *src, size_t bytes, uchar *dst) noexcept
{
    auto swapEndian = [](__m128i &data) {
        __m128i lows = _mm_srli_epi16(data, 8);
        __m128i highs = _mm_slli_epi16(data, 8);
        data = _mm_xor_si128(lows, highs);
    };

    size_t i = 0;
    for ( ; i + 2 * sizeof(__m128i) <= bytes; i += 2 * sizeof(__m128i)) {
        __m128i data1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i));
        __m128i data2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i) + 1);
        swapEndian(data1);
        swapEndian(data2);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + i), data1);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + i) + 1, data2);
    }

    if (i + sizeof(__m128i) <= bytes) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i));
        swapEndian(data);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + i), data);
        i += sizeof(__m128i);
    }

    // epilogue
    for (size_t _i = 0 ; i < bytes && _i < sizeof(__m128i); i += sizeof(quint16), _i += sizeof(quint16))
        qbswap(qFromUnaligned<quint16>(src + i), dst + i);

    // return the total, so the bswapLoop below does nothing
    return bytes;
}
#else
template <typename T> static Q_ALWAYS_INLINE
size_t simdSwapLoop(const uchar *, size_t, uchar *) noexcept
{
    return 0;
}
#endif

template <typename T> static Q_ALWAYS_INLINE
void *bswapLoop(const uchar *src, size_t n, uchar *dst) noexcept
{
    // Buffers cannot partially overlap: either they're identical or totally
    // disjoint (note: they can be adjacent).
    if (src != dst) {
        quintptr s = quintptr(src);
        quintptr d = quintptr(dst);
        if (s < d)
            Q_ASSERT(s + n <= d);
        else
            Q_ASSERT(d + n <= s);
    }

    size_t i = simdSwapLoop<T>(src, n, dst);

    for ( ; i < n; i += sizeof(T))
        qbswap(qFromUnaligned<T>(src + i), dst + i);
    return dst + i;
}

template <> void *qbswap<2>(const void *source, qsizetype n, void *dest) noexcept
{
    const uchar *src = reinterpret_cast<const uchar *>(source);
    uchar *dst = reinterpret_cast<uchar *>(dest);

    return bswapLoop<quint16>(src, n << 1, dst);
}

template <> void *qbswap<4>(const void *source, qsizetype n, void *dest) noexcept
{
    const uchar *src = reinterpret_cast<const uchar *>(source);
    uchar *dst = reinterpret_cast<uchar *>(dest);

    return bswapLoop<quint32>(src, n << 2, dst);
}

template <> void *qbswap<8>(const void *source, qsizetype n, void *dest) noexcept
{
    const uchar *src = reinterpret_cast<const uchar *>(source);
    uchar *dst = reinterpret_cast<uchar *>(dest);

    return bswapLoop<quint64>(src, n << 3, dst);
}

QT_END_NAMESPACE
