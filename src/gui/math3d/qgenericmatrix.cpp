// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qgenericmatrix.h"

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QMatrix2x2)
QT_IMPL_METATYPE_EXTERN(QMatrix2x3)
QT_IMPL_METATYPE_EXTERN(QMatrix2x4)
QT_IMPL_METATYPE_EXTERN(QMatrix3x2)
QT_IMPL_METATYPE_EXTERN(QMatrix3x3)
QT_IMPL_METATYPE_EXTERN(QMatrix3x4)
QT_IMPL_METATYPE_EXTERN(QMatrix4x2)
QT_IMPL_METATYPE_EXTERN(QMatrix4x3)

/*!
    \class QGenericMatrix
    \brief The QGenericMatrix class is a template class that represents an N x M transformation matrix with N columns and M rows.
    \since 4.6
    \ingroup painting
    \ingroup painting-3D
    \inmodule QtGui

    QGenericMatrix\<N, M, T\> is a template class where \a N is the number of
    columns, \a M is the number of rows, and \a T is the element type that is
    visible to users of the class.

    \sa QMatrix4x4
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T>::QGenericMatrix()

    Constructs an \a N x \a M identity matrix.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T>::QGenericMatrix(Qt::Initialization)
    \since 5.5
    \internal

    Constructs an \a N x \a M matrix without initializing the contents.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T>::QGenericMatrix(const T *values)

    Constructs a matrix from the given \a N * \a M floating-point \a values.
    The contents of the array \a values is assumed to be in
    row-major order.

    \sa copyDataTo()
*/

/*!
    \fn template <int N, int M, typename T> const T& QGenericMatrix<N, M, T>::operator()(int row, int column) const

    Returns a constant reference to the element at position
    (\a row, \a column) in this matrix.
*/

/*!
    \fn template <int N, int M, typename T> T& QGenericMatrix<N, M, T>::operator()(int row, int column)

    Returns a reference to the element at position (\a row, \a column)
    in this matrix so that the element can be assigned to.
*/

/*!
    \fn template <int N, int M, typename T> bool QGenericMatrix<N, M, T>::isIdentity() const

    Returns \c true if this matrix is the identity; false otherwise.

    \sa setToIdentity()
*/

/*!
    \fn template <int N, int M, typename T> void QGenericMatrix<N, M, T>::setToIdentity()

    Sets this matrix to the identity.

    \sa isIdentity()
*/

/*!
    \fn template <int N, int M, typename T> void QGenericMatrix<N, M, T>::fill(T value)

    Fills all elements of this matrix with \a value.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<M, N> QGenericMatrix<N, M, T>::transposed() const

    Returns this matrix, transposed about its diagonal.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T>& QGenericMatrix<N, M, T>::operator+=(const QGenericMatrix<N, M, T>& other)

    Adds the contents of \a other to this matrix.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T>& QGenericMatrix<N, M, T>::operator-=(const QGenericMatrix<N, M, T>& other)

    Subtracts the contents of \a other from this matrix.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T>& QGenericMatrix<N, M, T>::operator*=(T factor)

    Multiplies all elements of this matrix by \a factor.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T>& QGenericMatrix<N, M, T>::operator/=(T divisor)

    Divides all elements of this matrix by \a divisor.
*/

/*!
    \fn template <int N, int M, typename T> bool QGenericMatrix<N, M, T>::operator==(const QGenericMatrix<N, M, T>& other) const

    Returns \c true if this matrix is identical to \a other; false otherwise.
*/

/*!
    \fn template <int N, int M, typename T> bool QGenericMatrix<N, M, T>::operator!=(const QGenericMatrix<N, M, T>& other) const

    Returns \c true if this matrix is not identical to \a other; false otherwise.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T> operator+(const QGenericMatrix<N, M, T>& m1, const QGenericMatrix<N, M, T>& m2)
    \relates QGenericMatrix

    Returns the sum of the \a N x \a M matrices \a m1 and \a m2.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T> operator-(const QGenericMatrix<N, M, T>& m1, const QGenericMatrix<N, M, T>& m2)
    \relates QGenericMatrix

    Returns the difference of the \a N x \a M matrices \a m1 and \a m2.
*/

/*!
    \fn template <int N, int M, typename T> template<int NN, int M1, int M2, typename TT> QGenericMatrix<M1, M2, TT> QGenericMatrix<N, M, T>::operator*(const QGenericMatrix<NN, M2, TT>& m1, const QGenericMatrix<M1, NN, TT>& m2)

    Returns the product of the \a NN x \a M2 matrix \a m1 and the \a M1 x \a NN
    matrix \a m2 to produce an \a M1 x \a M2 matrix result.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T> operator-(const QGenericMatrix<N, M, T>& matrix)
    \overload
    \relates QGenericMatrix

    Returns the negation of \a matrix.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T> operator*(T factor, const QGenericMatrix<N, M, T>& matrix)
    \relates QGenericMatrix

    Returns the result of multiplying all elements of the \a N x \a M
    \a matrix by \a factor.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T> operator*(const QGenericMatrix<N, M, T>& matrix, T factor)
    \relates QGenericMatrix

    Returns the result of multiplying all elements of \a matrix by \a factor.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T> operator/(const QGenericMatrix<N, M, T>& matrix, T divisor)
    \relates QGenericMatrix

    Returns the result of dividing all elements of the \a N x \a M
    \a matrix by \a divisor.
*/

/*!
    \fn template <int N, int M, typename T> void QGenericMatrix<N, M, T>::copyDataTo(T *values) const

    Retrieves the \a N * \a M items in this matrix and copies them to \a values
    in row-major order.
*/

/*!
    \fn template <int N, int M, typename T> T *QGenericMatrix<N, M, T>::data()

    Returns a pointer to the raw data of this matrix.

    \sa constData()
*/

/*!
    \fn template <int N, int M, typename T> const T *QGenericMatrix<N, M, T>::data() const

    Returns a constant pointer to the raw data of this matrix.

    \sa constData()
*/

/*!
    \fn template <int N, int M, typename T> const T *QGenericMatrix<N, M, T>::constData() const

    Returns a constant pointer to the raw data of this matrix.

    \sa data()
*/

#ifndef QT_NO_DATASTREAM

/*!
    \fn template <int N, int M, typename T> QDataStream &operator<<(QDataStream &stream, const QGenericMatrix<N, M, T> &matrix)
    \relates QGenericMatrix

    Writes the given \a N x \a M \a matrix to the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

/*!
    \fn template <int N, int M, typename T> QDataStream &operator>>(QDataStream &stream, QGenericMatrix<N, M, T> &matrix)
    \relates QGenericMatrix

    Reads an \a N x \a M matrix from the given \a stream into the given \a matrix
    and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

#endif

/*!
    \typedef QMatrix2x2
    \relates QGenericMatrix

    The QMatrix2x2 type defines a convenient instantiation of the
    QGenericMatrix template for 2 columns, 2 rows, and float as
    the element type.
*/

/*!
    \typedef QMatrix2x3
    \relates QGenericMatrix

    The QMatrix2x3 type defines a convenient instantiation of the
    QGenericMatrix template for 2 columns, 3 rows, and float as
    the element type.
*/

/*!
    \typedef QMatrix2x4
    \relates QGenericMatrix

    The QMatrix2x4 type defines a convenient instantiation of the
    QGenericMatrix template for 2 columns, 4 rows, and float as
    the element type.
*/

/*!
    \typedef QMatrix3x2
    \relates QGenericMatrix

    The QMatrix3x2 type defines a convenient instantiation of the
    QGenericMatrix template for 3 columns, 2 rows, and float as
    the element type.
*/

/*!
    \typedef QMatrix3x3
    \relates QGenericMatrix

    The QMatrix3x3 type defines a convenient instantiation of the
    QGenericMatrix template for 3 columns, 3 rows, and float as
    the element type.
*/

/*!
    \typedef QMatrix3x4
    \relates QGenericMatrix

    The QMatrix3x4 type defines a convenient instantiation of the
    QGenericMatrix template for 3 columns, 4 rows, and float as
    the element type.
*/

/*!
    \typedef QMatrix4x2
    \relates QGenericMatrix

    The QMatrix4x2 type defines a convenient instantiation of the
    QGenericMatrix template for 4 columns, 2 rows, and float as
    the element type.
*/

/*!
    \typedef QMatrix4x3
    \relates QGenericMatrix

    The QMatrix4x3 type defines a convenient instantiation of the
    QGenericMatrix template for 4 columns, 3 rows, and float as
    the element type.
*/

QT_END_NAMESPACE
