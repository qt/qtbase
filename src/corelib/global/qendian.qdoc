/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:FDL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Free Documentation License Usage
** Alternatively, this file may be used under the terms of the GNU Free
** Documentation License version 1.3 as published by the Free Software
** Foundation and appearing in the file included in the packaging of
** this file. Please review the following information to ensure
** the GNU Free Documentation License version 1.3 requirements
** will be met: https://www.gnu.org/licenses/fdl-1.3.html.
** $QT_END_LICENSE$
**
****************************************************************************/

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

    \note Template type \c{T} can either be a qint16, qint32 or qint64. Other types of
    integers, e.g., qlong, are not applicable.

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
    \fn template <typename T> T qFromLittleEndian(const void *src)
    \since 4.3
    \relates <QtEndian>

    Reads a little-endian number from memory location \a src and returns the number in
    the host byte order representation.
    On CPU architectures where the host byte order is big-endian (such as PowerPC) this
    will swap the byte order; otherwise it will just read from \a src.

    \note Template type \c{T} can either be a qint16, qint32 or qint64. Other types of
    integers, e.g., qlong, are not applicable.

    \note Since Qt 5.7, the type of the \a src parameter is a void pointer.

    There are no data alignment constraints for \a src.

    \sa qFromBigEndian()
    \sa qToBigEndian()
    \sa qToLittleEndian()
*/
/*!
    \fn template <typename T> T qFromLittleEndian(T src)
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
    \fn template <typename T> void qToBigEndian(T src, void *dest)
    \since 4.3
    \relates <QtEndian>

    Writes the number \a src with template type \c{T} to the memory location at \a dest
    in big-endian byte order.

    Note that template type \c{T} can only be an integer data type (signed or unsigned).

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
    \fn template <typename T> void qToLittleEndian(T src, void *dest)
    \since 4.3
    \relates <QtEndian>

    Writes the number \a src with template type \c{T} to the memory location at \a dest
    in little-endian byte order.

    Note that template type \c{T} can only be an integer data type (signed or unsigned).

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
    an exact endian is needed.
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
    an exact endian is needed.
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
