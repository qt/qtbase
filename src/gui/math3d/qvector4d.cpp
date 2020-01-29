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

#include "qvector4d.h"
#include "qvector3d.h"
#include "qvector2d.h"
#include <QtCore/qdatastream.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvariant.h>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_VECTOR4D

Q_STATIC_ASSERT_X(std::is_standard_layout<QVector4D>::value, "QVector4D is supposed to be standard layout");
Q_STATIC_ASSERT_X(sizeof(QVector4D) == sizeof(float) * 4, "QVector4D is not supposed to have padding at the end");

// QVector4D used to be defined as class QVector4D { float x, y, z, w; };,
// now instead it is defined as classs QVector4D { float v[4]; };.
// Check that binary compatibility is preserved.
// ### Qt 6: remove all of these checks.

namespace {

struct QVector4DOld
{
    float x, y, z, w;
};

struct QVector4DNew
{
    float v[4];
};

Q_STATIC_ASSERT_X(std::is_standard_layout<QVector4DOld>::value, "Binary compatibility break in QVector4D");
Q_STATIC_ASSERT_X(std::is_standard_layout<QVector4DNew>::value, "Binary compatibility break in QVector4D");

Q_STATIC_ASSERT_X(sizeof(QVector4DOld) == sizeof(QVector4DNew), "Binary compatibility break in QVector4D");

// requires a constexpr offsetof
#if !defined(Q_CC_MSVC) || (_MSC_VER >= 1910)
Q_STATIC_ASSERT_X(offsetof(QVector4DOld, x) == offsetof(QVector4DNew, v) + sizeof(QVector4DNew::v[0]) * 0, "Binary compatibility break in QVector4D");
Q_STATIC_ASSERT_X(offsetof(QVector4DOld, y) == offsetof(QVector4DNew, v) + sizeof(QVector4DNew::v[0]) * 1, "Binary compatibility break in QVector4D");
Q_STATIC_ASSERT_X(offsetof(QVector4DOld, z) == offsetof(QVector4DNew, v) + sizeof(QVector4DNew::v[0]) * 2, "Binary compatibility break in QVector4D");
Q_STATIC_ASSERT_X(offsetof(QVector4DOld, w) == offsetof(QVector4DNew, v) + sizeof(QVector4DNew::v[0]) * 3, "Binary compatibility break in QVector4D");
#endif


} // anonymous namespace

/*!
    \class QVector4D
    \brief The QVector4D class represents a vector or vertex in 4D space.
    \since 4.6
    \ingroup painting-3D
    \inmodule QtGui

    The QVector4D class can also be used to represent vertices in 4D space.
    We therefore do not need to provide a separate vertex class.

    \sa QQuaternion, QVector2D, QVector3D
*/

/*!
    \fn QVector4D::QVector4D()

    Constructs a null vector, i.e. with coordinates (0, 0, 0, 0).
*/

/*!
    \fn QVector4D::QVector4D(Qt::Initialization)
    \since 5.5
    \internal

    Constructs a vector without initializing the contents.
*/

/*!
    \fn QVector4D::QVector4D(float xpos, float ypos, float zpos, float wpos)

    Constructs a vector with coordinates (\a xpos, \a ypos, \a zpos, \a wpos).
*/

/*!
    \fn QVector4D::QVector4D(const QPoint& point)

    Constructs a vector with x and y coordinates from a 2D \a point, and
    z and w coordinates of 0.
*/

/*!
    \fn QVector4D::QVector4D(const QPointF& point)

    Constructs a vector with x and y coordinates from a 2D \a point, and
    z and w coordinates of 0.
*/

#ifndef QT_NO_VECTOR2D

/*!
    Constructs a 4D vector from the specified 2D \a vector.  The z
    and w coordinates are set to zero.

    \sa toVector2D()
*/
QVector4D::QVector4D(const QVector2D& vector)
{
    v[0] = vector.v[0];
    v[1] = vector.v[1];
    v[2] = 0.0f;
    v[3] = 0.0f;
}

/*!
    Constructs a 4D vector from the specified 2D \a vector.  The z
    and w coordinates are set to \a zpos and \a wpos respectively.

    \sa toVector2D()
*/
QVector4D::QVector4D(const QVector2D& vector, float zpos, float wpos)
{
    v[0] = vector.v[0];
    v[1] = vector.v[1];
    v[2] = zpos;
    v[3] = wpos;
}

#endif

#ifndef QT_NO_VECTOR3D

/*!
    Constructs a 4D vector from the specified 3D \a vector.  The w
    coordinate is set to zero.

    \sa toVector3D()
*/
QVector4D::QVector4D(const QVector3D& vector)
{
    v[0] = vector.v[0];
    v[1] = vector.v[1];
    v[2] = vector.v[2];
    v[3] = 0.0f;
}

/*!
    Constructs a 4D vector from the specified 3D \a vector.  The w
    coordinate is set to \a wpos.

    \sa toVector3D()
*/
QVector4D::QVector4D(const QVector3D& vector, float wpos)
{
    v[0] = vector.v[0];
    v[1] = vector.v[1];
    v[2] = vector.v[2];
    v[3] = wpos;
}

#endif

/*!
    \fn bool QVector4D::isNull() const

    Returns \c true if the x, y, z, and w coordinates are set to 0.0,
    otherwise returns \c false.
*/

/*!
    \fn float QVector4D::x() const

    Returns the x coordinate of this point.

    \sa setX(), y(), z(), w()
*/

/*!
    \fn float QVector4D::y() const

    Returns the y coordinate of this point.

    \sa setY(), x(), z(), w()
*/

/*!
    \fn float QVector4D::z() const

    Returns the z coordinate of this point.

    \sa setZ(), x(), y(), w()
*/

/*!
    \fn float QVector4D::w() const

    Returns the w coordinate of this point.

    \sa setW(), x(), y(), z()
*/

/*!
    \fn void QVector4D::setX(float x)

    Sets the x coordinate of this point to the given \a x coordinate.

    \sa x(), setY(), setZ(), setW()
*/

/*!
    \fn void QVector4D::setY(float y)

    Sets the y coordinate of this point to the given \a y coordinate.

    \sa y(), setX(), setZ(), setW()
*/

/*!
    \fn void QVector4D::setZ(float z)

    Sets the z coordinate of this point to the given \a z coordinate.

    \sa z(), setX(), setY(), setW()
*/

/*!
    \fn void QVector4D::setW(float w)

    Sets the w coordinate of this point to the given \a w coordinate.

    \sa w(), setX(), setY(), setZ()
*/

/*! \fn float &QVector4D::operator[](int i)
    \since 5.2

    Returns the component of the vector at index position \a i
    as a modifiable reference.

    \a i must be a valid index position in the vector (i.e., 0 <= \a i
    < 4).
*/

/*! \fn float QVector4D::operator[](int i) const
    \since 5.2

    Returns the component of the vector at index position \a i.

    \a i must be a valid index position in the vector (i.e., 0 <= \a i
    < 4).
*/

/*!
    Returns the length of the vector from the origin.

    \sa lengthSquared(), normalized()
*/
float QVector4D::length() const
{
    // Need some extra precision if the length is very small.
    double len = double(v[0]) * double(v[0]) +
                 double(v[1]) * double(v[1]) +
                 double(v[2]) * double(v[2]) +
                 double(v[3]) * double(v[3]);
    return float(std::sqrt(len));
}

/*!
    Returns the squared length of the vector from the origin.
    This is equivalent to the dot product of the vector with itself.

    \sa length(), dotProduct()
*/
float QVector4D::lengthSquared() const
{
    return v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3];
}

/*!
    Returns the normalized unit vector form of this vector.

    If this vector is null, then a null vector is returned.  If the length
    of the vector is very close to 1, then the vector will be returned as-is.
    Otherwise the normalized form of the vector of length 1 will be returned.

    \sa length(), normalize()
*/
QVector4D QVector4D::normalized() const
{
    // Need some extra precision if the length is very small.
    double len = double(v[0]) * double(v[0]) +
                 double(v[1]) * double(v[1]) +
                 double(v[2]) * double(v[2]) +
                 double(v[3]) * double(v[3]);
    if (qFuzzyIsNull(len - 1.0f)) {
        return *this;
    } else if (!qFuzzyIsNull(len)) {
        double sqrtLen = std::sqrt(len);
        return QVector4D(float(double(v[0]) / sqrtLen),
                         float(double(v[1]) / sqrtLen),
                         float(double(v[2]) / sqrtLen),
                         float(double(v[3]) / sqrtLen));
    } else {
        return QVector4D();
    }
}

/*!
    Normalizes the currect vector in place.  Nothing happens if this
    vector is a null vector or the length of the vector is very close to 1.

    \sa length(), normalized()
*/
void QVector4D::normalize()
{
    // Need some extra precision if the length is very small.
    double len = double(v[0]) * double(v[0]) +
                 double(v[1]) * double(v[1]) +
                 double(v[2]) * double(v[2]) +
                 double(v[3]) * double(v[3]);
    if (qFuzzyIsNull(len - 1.0f) || qFuzzyIsNull(len))
        return;

    len = std::sqrt(len);

    v[0] = float(double(v[0]) / len);
    v[1] = float(double(v[1]) / len);
    v[2] = float(double(v[2]) / len);
    v[3] = float(double(v[3]) / len);
}

/*!
    \fn QVector4D &QVector4D::operator+=(const QVector4D &vector)

    Adds the given \a vector to this vector and returns a reference to
    this vector.

    \sa operator-=()
*/

/*!
    \fn QVector4D &QVector4D::operator-=(const QVector4D &vector)

    Subtracts the given \a vector from this vector and returns a reference to
    this vector.

    \sa operator+=()
*/

/*!
    \fn QVector4D &QVector4D::operator*=(float factor)

    Multiplies this vector's coordinates by the given \a factor, and
    returns a reference to this vector.

    \sa operator/=()
*/

/*!
    \fn QVector4D &QVector4D::operator*=(const QVector4D &vector)

    Multiplies the components of this vector by the corresponding
    components in \a vector.
*/

/*!
    \fn QVector4D &QVector4D::operator/=(float divisor)

    Divides this vector's coordinates by the given \a divisor, and
    returns a reference to this vector.

    \sa operator*=()
*/

/*!
    \fn QVector4D &QVector4D::operator/=(const QVector4D &vector)
    \since 5.5

    Divides the components of this vector by the corresponding
    components in \a vector.

    \sa operator*=()
*/

/*!
    Returns the dot product of \a v1 and \a v2.
*/
float QVector4D::dotProduct(const QVector4D& v1, const QVector4D& v2)
{
    return v1.v[0] * v2.v[0] + v1.v[1] * v2.v[1] + v1.v[2] * v2.v[2] + v1.v[3] * v2.v[3];
}

/*!
    \fn bool operator==(const QVector4D &v1, const QVector4D &v2)
    \relates QVector4D

    Returns \c true if \a v1 is equal to \a v2; otherwise returns \c false.
    This operator uses an exact floating-point comparison.
*/

/*!
    \fn bool operator!=(const QVector4D &v1, const QVector4D &v2)
    \relates QVector4D

    Returns \c true if \a v1 is not equal to \a v2; otherwise returns \c false.
    This operator uses an exact floating-point comparison.
*/

/*!
    \fn const QVector4D operator+(const QVector4D &v1, const QVector4D &v2)
    \relates QVector4D

    Returns a QVector4D object that is the sum of the given vectors, \a v1
    and \a v2; each component is added separately.

    \sa QVector4D::operator+=()
*/

/*!
    \fn const QVector4D operator-(const QVector4D &v1, const QVector4D &v2)
    \relates QVector4D

    Returns a QVector4D object that is formed by subtracting \a v2 from \a v1;
    each component is subtracted separately.

    \sa QVector4D::operator-=()
*/

/*!
    \fn const QVector4D operator*(float factor, const QVector4D &vector)
    \relates QVector4D

    Returns a copy of the given \a vector,  multiplied by the given \a factor.

    \sa QVector4D::operator*=()
*/

/*!
    \fn const QVector4D operator*(const QVector4D &vector, float factor)
    \relates QVector4D

    Returns a copy of the given \a vector,  multiplied by the given \a factor.

    \sa QVector4D::operator*=()
*/

/*!
    \fn const QVector4D operator*(const QVector4D &v1, const QVector4D& v2)
    \relates QVector4D

    Returns the vector consisting of the multiplication of the
    components from \a v1 and \a v2.

    \sa QVector4D::operator*=()
*/

/*!
    \fn const QVector4D operator-(const QVector4D &vector)
    \relates QVector4D
    \overload

    Returns a QVector4D object that is formed by changing the sign of
    all three components of the given \a vector.

    Equivalent to \c {QVector4D(0,0,0,0) - vector}.
*/

/*!
    \fn const QVector4D operator/(const QVector4D &vector, float divisor)
    \relates QVector4D

    Returns the QVector4D object formed by dividing all four components of
    the given \a vector by the given \a divisor.

    \sa QVector4D::operator/=()
*/

/*!
    \fn const QVector4D operator/(const QVector4D &vector, const QVector4D &divisor)
    \relates QVector4D
    \since 5.5

    Returns the QVector4D object formed by dividing components of the given
    \a vector by a respective components of the given \a divisor.

    \sa QVector4D::operator/=()
*/

/*!
    \fn bool qFuzzyCompare(const QVector4D& v1, const QVector4D& v2)
    \relates QVector4D

    Returns \c true if \a v1 and \a v2 are equal, allowing for a small
    fuzziness factor for floating-point comparisons; false otherwise.
*/

#ifndef QT_NO_VECTOR2D

/*!
    Returns the 2D vector form of this 4D vector, dropping the z and w coordinates.

    \sa toVector2DAffine(), toVector3D(), toPoint()
*/
QVector2D QVector4D::toVector2D() const
{
    return QVector2D(v[0], v[1]);
}

/*!
    Returns the 2D vector form of this 4D vector, dividing the x and y
    coordinates by the w coordinate and dropping the z coordinate.
    Returns a null vector if w is zero.

    \sa toVector2D(), toVector3DAffine(), toPoint()
*/
QVector2D QVector4D::toVector2DAffine() const
{
    if (qIsNull(v[3]))
        return QVector2D();
    return QVector2D(v[0] / v[3], v[1] / v[3]);
}

#endif

#ifndef QT_NO_VECTOR3D

/*!
    Returns the 3D vector form of this 4D vector, dropping the w coordinate.

    \sa toVector3DAffine(), toVector2D(), toPoint()
*/
QVector3D QVector4D::toVector3D() const
{
    return QVector3D(v[0], v[1], v[2]);
}

/*!
    Returns the 3D vector form of this 4D vector, dividing the x, y, and
    z coordinates by the w coordinate.  Returns a null vector if w is zero.

    \sa toVector3D(), toVector2DAffine(), toPoint()
*/
QVector3D QVector4D::toVector3DAffine() const
{
    if (qIsNull(v[3]))
        return QVector3D();
    return QVector3D(v[0] / v[3], v[1] / v[3], v[2] / v[3]);
}

#endif

/*!
    \fn QPoint QVector4D::toPoint() const

    Returns the QPoint form of this 4D vector. The z and w coordinates
    are dropped.

    \sa toPointF(), toVector2D()
*/

/*!
    \fn QPointF QVector4D::toPointF() const

    Returns the QPointF form of this 4D vector. The z and w coordinates
    are dropped.

    \sa toPoint(), toVector2D()
*/

/*!
    Returns the 4D vector as a QVariant.
*/
QVector4D::operator QVariant() const
{
    return QVariant(QMetaType::QVector4D, this);
}

#ifndef QT_NO_DEBUG_STREAM

QDebug operator<<(QDebug dbg, const QVector4D &vector)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QVector4D("
        << vector.x() << ", " << vector.y() << ", "
        << vector.z() << ", " << vector.w() << ')';
    return dbg;
}

#endif

#ifndef QT_NO_DATASTREAM

/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QVector4D &vector)
    \relates QVector4D

    Writes the given \a vector to the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &stream, const QVector4D &vector)
{
    stream << vector.x() << vector.y()
           << vector.z() << vector.w();
    return stream;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QVector4D &vector)
    \relates QVector4D

    Reads a 4D vector from the given \a stream into the given \a vector
    and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &stream, QVector4D &vector)
{
    float x, y, z, w;
    stream >> x;
    stream >> y;
    stream >> z;
    stream >> w;
    vector.setX(x);
    vector.setY(y);
    vector.setZ(z);
    vector.setW(w);
    return stream;
}

#endif // QT_NO_DATASTREAM

#endif // QT_NO_VECTOR4D

QT_END_NAMESPACE
