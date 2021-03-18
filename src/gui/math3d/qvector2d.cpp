/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qvector2d.h"
#include "qvector3d.h"
#include "qvector4d.h"
#include <QtCore/qdatastream.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvariant.h>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_VECTOR2D

Q_STATIC_ASSERT_X(std::is_standard_layout<QVector2D>::value, "QVector2D is supposed to be standard layout");
Q_STATIC_ASSERT_X(sizeof(QVector2D) == sizeof(float) * 2, "QVector2D is not supposed to have padding at the end");

// QVector2D used to be defined as class QVector2D { float x, y; };,
// now instead it is defined as classs QVector2D { float v[2]; };.
// Check that binary compatibility is preserved.
// ### Qt 6: remove all of these checks.

namespace {

struct QVector2DOld
{
    float x, y;
};

struct QVector2DNew
{
    float v[2];
};

Q_STATIC_ASSERT_X(std::is_standard_layout<QVector2DOld>::value, "Binary compatibility break in QVector2D");
Q_STATIC_ASSERT_X(std::is_standard_layout<QVector2DNew>::value, "Binary compatibility break in QVector2D");

Q_STATIC_ASSERT_X(sizeof(QVector2DOld) == sizeof(QVector2DNew), "Binary compatibility break in QVector2D");

// requires a constexpr offsetof
#if !defined(Q_CC_MSVC) || (_MSC_VER >= 1910)
Q_STATIC_ASSERT_X(offsetof(QVector2DOld, x) == offsetof(QVector2DNew, v) + sizeof(QVector2DNew::v[0]) * 0, "Binary compatibility break in QVector2D");
Q_STATIC_ASSERT_X(offsetof(QVector2DOld, y) == offsetof(QVector2DNew, v) + sizeof(QVector2DNew::v[0]) * 1, "Binary compatibility break in QVector2D");
#endif

} // anonymous namespace

/*!
    \class QVector2D
    \brief The QVector2D class represents a vector or vertex in 2D space.
    \since 4.6
    \ingroup painting
    \ingroup painting-3D
    \inmodule QtGui

    The QVector2D class can also be used to represent vertices in 2D space.
    We therefore do not need to provide a separate vertex class.

    \sa QVector3D, QVector4D, QQuaternion
*/

/*!
    \fn QVector2D::QVector2D()

    Constructs a null vector, i.e. with coordinates (0, 0).
*/

/*!
    \fn QVector2D::QVector2D(Qt::Initialization)
    \since 5.5
    \internal

    Constructs a vector without initializing the contents.
*/

/*!
    \fn QVector2D::QVector2D(float xpos, float ypos)

    Constructs a vector with coordinates (\a xpos, \a ypos).
*/

/*!
    \fn QVector2D::QVector2D(const QPoint& point)

    Constructs a vector with x and y coordinates from a 2D \a point.
*/

/*!
    \fn QVector2D::QVector2D(const QPointF& point)

    Constructs a vector with x and y coordinates from a 2D \a point.
*/

#ifndef QT_NO_VECTOR3D

/*!
    Constructs a vector with x and y coordinates from a 3D \a vector.
    The z coordinate of \a vector is dropped.

    \sa toVector3D()
*/
QVector2D::QVector2D(const QVector3D& vector)
{
    v[0] = vector.v[0];
    v[1] = vector.v[1];
}

#endif

#ifndef QT_NO_VECTOR4D

/*!
    Constructs a vector with x and y coordinates from a 3D \a vector.
    The z and w coordinates of \a vector are dropped.

    \sa toVector4D()
*/
QVector2D::QVector2D(const QVector4D& vector)
{
    v[0] = vector.v[0];
    v[1] = vector.v[1];
}

#endif

/*!
    \fn bool QVector2D::isNull() const

    Returns \c true if the x and y coordinates are set to 0.0,
    otherwise returns \c false.
*/

/*!
    \fn float QVector2D::x() const

    Returns the x coordinate of this point.

    \sa setX(), y()
*/

/*!
    \fn float QVector2D::y() const

    Returns the y coordinate of this point.

    \sa setY(), x()
*/

/*!
    \fn void QVector2D::setX(float x)

    Sets the x coordinate of this point to the given \a x coordinate.

    \sa x(), setY()
*/

/*!
    \fn void QVector2D::setY(float y)

    Sets the y coordinate of this point to the given \a y coordinate.

    \sa y(), setX()
*/

/*! \fn float &QVector2D::operator[](int i)
    \since 5.2

    Returns the component of the vector at index position \a i
    as a modifiable reference.

    \a i must be a valid index position in the vector (i.e., 0 <= \a i
    < 2).
*/

/*! \fn float QVector2D::operator[](int i) const
    \since 5.2

    Returns the component of the vector at index position \a i.

    \a i must be a valid index position in the vector (i.e., 0 <= \a i
    < 2).
*/

/*!
    Returns the length of the vector from the origin.

    \sa lengthSquared(), normalized()
*/
float QVector2D::length() const
{
    // Need some extra precision if the length is very small.
    double len = double(v[0]) * double(v[0]) +
                 double(v[1]) * double(v[1]);
    return float(std::sqrt(len));
}

/*!
    Returns the squared length of the vector from the origin.
    This is equivalent to the dot product of the vector with itself.

    \sa length(), dotProduct()
*/
float QVector2D::lengthSquared() const
{
    return v[0] * v[0] + v[1] * v[1];
}

/*!
    Returns the normalized unit vector form of this vector.

    If this vector is null, then a null vector is returned.  If the length
    of the vector is very close to 1, then the vector will be returned as-is.
    Otherwise the normalized form of the vector of length 1 will be returned.

    \sa length(), normalize()
*/
QVector2D QVector2D::normalized() const
{
    // Need some extra precision if the length is very small.
    double len = double(v[0]) * double(v[0]) +
                 double(v[1]) * double(v[1]);
    if (qFuzzyIsNull(len - 1.0f)) {
        return *this;
    } else if (!qFuzzyIsNull(len)) {
        double sqrtLen = std::sqrt(len);
        return QVector2D(float(double(v[0]) / sqrtLen), float(double(v[1]) / sqrtLen));
    } else {
        return QVector2D();
    }
}

/*!
    Normalizes the currect vector in place.  Nothing happens if this
    vector is a null vector or the length of the vector is very close to 1.

    \sa length(), normalized()
*/
void QVector2D::normalize()
{
    // Need some extra precision if the length is very small.
    double len = double(v[0]) * double(v[0]) +
                 double(v[1]) * double(v[1]);
    if (qFuzzyIsNull(len - 1.0f) || qFuzzyIsNull(len))
        return;

    len = std::sqrt(len);

    v[0] = float(double(v[0]) / len);
    v[1] = float(double(v[1]) / len);
}

/*!
    \since 5.1

    Returns the distance from this vertex to a point defined by
    the vertex \a point.

    \sa distanceToLine()
*/
float QVector2D::distanceToPoint(const QVector2D& point) const
{
    return (*this - point).length();
}

/*!
    \since 5.1

    Returns the distance that this vertex is from a line defined
    by \a point and the unit vector \a direction.

    If \a direction is a null vector, then it does not define a line.
    In that case, the distance from \a point to this vertex is returned.

    \sa distanceToPoint()
*/
float QVector2D::distanceToLine
        (const QVector2D& point, const QVector2D& direction) const
{
    if (direction.isNull())
        return (*this - point).length();
    QVector2D p = point + dotProduct(*this - point, direction) * direction;
    return (*this - p).length();
}

/*!
    \fn QVector2D &QVector2D::operator+=(const QVector2D &vector)

    Adds the given \a vector to this vector and returns a reference to
    this vector.

    \sa operator-=()
*/

/*!
    \fn QVector2D &QVector2D::operator-=(const QVector2D &vector)

    Subtracts the given \a vector from this vector and returns a reference to
    this vector.

    \sa operator+=()
*/

/*!
    \fn QVector2D &QVector2D::operator*=(float factor)

    Multiplies this vector's coordinates by the given \a factor, and
    returns a reference to this vector.

    \sa operator/=()
*/

/*!
    \fn QVector2D &QVector2D::operator*=(const QVector2D &vector)

    Multiplies the components of this vector by the corresponding
    components in \a vector.
*/

/*!
    \fn QVector2D &QVector2D::operator/=(float divisor)

    Divides this vector's coordinates by the given \a divisor, and
    returns a reference to this vector.

    \sa operator*=()
*/

/*!
    \fn QVector2D &QVector2D::operator/=(const QVector2D &vector)
    \since 5.5

    Divides the components of this vector by the corresponding
    components in \a vector.

    \sa operator*=()
*/

/*!
    Returns the dot product of \a v1 and \a v2.
*/
float QVector2D::dotProduct(const QVector2D& v1, const QVector2D& v2)
{
    return v1.v[0] * v2.v[0] + v1.v[1] * v2.v[1];
}

/*!
    \fn bool operator==(const QVector2D &v1, const QVector2D &v2)
    \relates QVector2D

    Returns \c true if \a v1 is equal to \a v2; otherwise returns \c false.
    This operator uses an exact floating-point comparison.
*/

/*!
    \fn bool operator!=(const QVector2D &v1, const QVector2D &v2)
    \relates QVector2D

    Returns \c true if \a v1 is not equal to \a v2; otherwise returns \c false.
    This operator uses an exact floating-point comparison.
*/

/*!
    \fn const QVector2D operator+(const QVector2D &v1, const QVector2D &v2)
    \relates QVector2D

    Returns a QVector2D object that is the sum of the given vectors, \a v1
    and \a v2; each component is added separately.

    \sa QVector2D::operator+=()
*/

/*!
    \fn const QVector2D operator-(const QVector2D &v1, const QVector2D &v2)
    \relates QVector2D

    Returns a QVector2D object that is formed by subtracting \a v2 from \a v1;
    each component is subtracted separately.

    \sa QVector2D::operator-=()
*/

/*!
    \fn const QVector2D operator*(float factor, const QVector2D &vector)
    \relates QVector2D

    Returns a copy of the given \a vector,  multiplied by the given \a factor.

    \sa QVector2D::operator*=()
*/

/*!
    \fn const QVector2D operator*(const QVector2D &vector, float factor)
    \relates QVector2D

    Returns a copy of the given \a vector,  multiplied by the given \a factor.

    \sa QVector2D::operator*=()
*/

/*!
    \fn const QVector2D operator*(const QVector2D &v1, const QVector2D &v2)
    \relates QVector2D

    Multiplies the components of \a v1 by the corresponding
    components in \a v2.
*/

/*!
    \fn const QVector2D operator-(const QVector2D &vector)
    \relates QVector2D
    \overload

    Returns a QVector2D object that is formed by changing the sign of
    the components of the given \a vector.

    Equivalent to \c {QVector2D(0,0) - vector}.
*/

/*!
    \fn const QVector2D operator/(const QVector2D &vector, float divisor)
    \relates QVector2D

    Returns the QVector2D object formed by dividing all three components of
    the given \a vector by the given \a divisor.

    \sa QVector2D::operator/=()
*/

/*!
    \fn const QVector2D operator/(const QVector2D &vector, const QVector2D &divisor)
    \relates QVector2D
    \since 5.5

    Returns the QVector2D object formed by dividing components of the given
    \a vector by a respective components of the given \a divisor.

    \sa QVector2D::operator/=()
*/

/*!
    \fn bool qFuzzyCompare(const QVector2D& v1, const QVector2D& v2)
    \relates QVector2D

    Returns \c true if \a v1 and \a v2 are equal, allowing for a small
    fuzziness factor for floating-point comparisons; false otherwise.
*/

#ifndef QT_NO_VECTOR3D

/*!
    Returns the 3D form of this 2D vector, with the z coordinate set to zero.

    \sa toVector4D(), toPoint()
*/
QVector3D QVector2D::toVector3D() const
{
    return QVector3D(v[0], v[1], 0.0f);
}

#endif

#ifndef QT_NO_VECTOR4D

/*!
    Returns the 4D form of this 2D vector, with the z and w coordinates set to zero.

    \sa toVector3D(), toPoint()
*/
QVector4D QVector2D::toVector4D() const
{
    return QVector4D(v[0], v[1], 0.0f, 0.0f);
}

#endif

/*!
    \fn QPoint QVector2D::toPoint() const

    Returns the QPoint form of this 2D vector.

    \sa toPointF(), toVector3D()
*/

/*!
    \fn QPointF QVector2D::toPointF() const

    Returns the QPointF form of this 2D vector.

    \sa toPoint(), toVector3D()
*/

/*!
    Returns the 2D vector as a QVariant.
*/
QVector2D::operator QVariant() const
{
    return QVariant(QMetaType::QVector2D, this);
}

#ifndef QT_NO_DEBUG_STREAM

QDebug operator<<(QDebug dbg, const QVector2D &vector)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QVector2D(" << vector.x() << ", " << vector.y() << ')';
    return dbg;
}

#endif

#ifndef QT_NO_DATASTREAM

/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QVector2D &vector)
    \relates QVector2D

    Writes the given \a vector to the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &stream, const QVector2D &vector)
{
    stream << vector.x() << vector.y();
    return stream;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QVector2D &vector)
    \relates QVector2D

    Reads a 2D vector from the given \a stream into the given \a vector
    and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &stream, QVector2D &vector)
{
    float x, y;
    stream >> x;
    stream >> y;
    vector.setX(x);
    vector.setY(y);
    return stream;
}

#endif // QT_NO_DATASTREAM

#endif // QT_NO_VECTOR2D

QT_END_NAMESPACE
