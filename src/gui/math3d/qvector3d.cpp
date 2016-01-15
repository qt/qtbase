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

#include "qvector3d.h"
#include "qvector2d.h"
#include "qvector4d.h"
#include "qmatrix4x4.h"
#include <QtCore/qdatastream.h>
#include <QtCore/qmath.h>
#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>
#include <QtCore/qrect.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_VECTOR3D

/*!
    \class QVector3D
    \brief The QVector3D class represents a vector or vertex in 3D space.
    \since 4.6
    \ingroup painting-3D
    \inmodule QtGui

    Vectors are one of the main building blocks of 3D representation and
    drawing.  They consist of three coordinates, traditionally called
    x, y, and z.

    The QVector3D class can also be used to represent vertices in 3D space.
    We therefore do not need to provide a separate vertex class.

    \sa QVector2D, QVector4D, QQuaternion
*/

/*!
    \fn QVector3D::QVector3D()

    Constructs a null vector, i.e. with coordinates (0, 0, 0).
*/

/*!
    \fn QVector3D::QVector3D(Qt::Initialization)
    \since 5.5
    \internal

    Constructs a vector without initializing the contents.
*/

/*!
    \fn QVector3D::QVector3D(float xpos, float ypos, float zpos)

    Constructs a vector with coordinates (\a xpos, \a ypos, \a zpos).
*/

/*!
    \fn QVector3D::QVector3D(const QPoint& point)

    Constructs a vector with x and y coordinates from a 2D \a point, and a
    z coordinate of 0.
*/

/*!
    \fn QVector3D::QVector3D(const QPointF& point)

    Constructs a vector with x and y coordinates from a 2D \a point, and a
    z coordinate of 0.
*/

#ifndef QT_NO_VECTOR2D

/*!
    Constructs a 3D vector from the specified 2D \a vector.  The z
    coordinate is set to zero.

    \sa toVector2D()
*/
QVector3D::QVector3D(const QVector2D& vector)
{
    xp = vector.xp;
    yp = vector.yp;
    zp = 0.0f;
}

/*!
    Constructs a 3D vector from the specified 2D \a vector.  The z
    coordinate is set to \a zpos.

    \sa toVector2D()
*/
QVector3D::QVector3D(const QVector2D& vector, float zpos)
{
    xp = vector.xp;
    yp = vector.yp;
    zp = zpos;
}

#endif

#ifndef QT_NO_VECTOR4D

/*!
    Constructs a 3D vector from the specified 4D \a vector.  The w
    coordinate is dropped.

    \sa toVector4D()
*/
QVector3D::QVector3D(const QVector4D& vector)
{
    xp = vector.xp;
    yp = vector.yp;
    zp = vector.zp;
}

#endif

/*!
    \fn bool QVector3D::isNull() const

    Returns \c true if the x, y, and z coordinates are set to 0.0,
    otherwise returns \c false.
*/

/*!
    \fn float QVector3D::x() const

    Returns the x coordinate of this point.

    \sa setX(), y(), z()
*/

/*!
    \fn float QVector3D::y() const

    Returns the y coordinate of this point.

    \sa setY(), x(), z()
*/

/*!
    \fn float QVector3D::z() const

    Returns the z coordinate of this point.

    \sa setZ(), x(), y()
*/

/*!
    \fn void QVector3D::setX(float x)

    Sets the x coordinate of this point to the given \a x coordinate.

    \sa x(), setY(), setZ()
*/

/*!
    \fn void QVector3D::setY(float y)

    Sets the y coordinate of this point to the given \a y coordinate.

    \sa y(), setX(), setZ()
*/

/*!
    \fn void QVector3D::setZ(float z)

    Sets the z coordinate of this point to the given \a z coordinate.

    \sa z(), setX(), setY()
*/

/*! \fn float &QVector3D::operator[](int i)
    \since 5.2

    Returns the component of the vector at index position \a i
    as a modifiable reference.

    \a i must be a valid index position in the vector (i.e., 0 <= \a i
    < 3).
*/

/*! \fn float QVector3D::operator[](int i) const
    \since 5.2

    Returns the component of the vector at index position \a i.

    \a i must be a valid index position in the vector (i.e., 0 <= \a i
    < 3).
*/

/*!
    Returns the normalized unit vector form of this vector.

    If this vector is null, then a null vector is returned.  If the length
    of the vector is very close to 1, then the vector will be returned as-is.
    Otherwise the normalized form of the vector of length 1 will be returned.

    \sa length(), normalize()
*/
QVector3D QVector3D::normalized() const
{
    // Need some extra precision if the length is very small.
    double len = double(xp) * double(xp) +
                 double(yp) * double(yp) +
                 double(zp) * double(zp);
    if (qFuzzyIsNull(len - 1.0f)) {
        return *this;
    } else if (!qFuzzyIsNull(len)) {
        double sqrtLen = std::sqrt(len);
        return QVector3D(float(double(xp) / sqrtLen),
                         float(double(yp) / sqrtLen),
                         float(double(zp) / sqrtLen));
    } else {
        return QVector3D();
    }
}

/*!
    Normalizes the currect vector in place.  Nothing happens if this
    vector is a null vector or the length of the vector is very close to 1.

    \sa length(), normalized()
*/
void QVector3D::normalize()
{
    // Need some extra precision if the length is very small.
    double len = double(xp) * double(xp) +
                 double(yp) * double(yp) +
                 double(zp) * double(zp);
    if (qFuzzyIsNull(len - 1.0f) || qFuzzyIsNull(len))
        return;

    len = std::sqrt(len);

    xp = float(double(xp) / len);
    yp = float(double(yp) / len);
    zp = float(double(zp) / len);
}

/*!
    \fn QVector3D &QVector3D::operator+=(const QVector3D &vector)

    Adds the given \a vector to this vector and returns a reference to
    this vector.

    \sa operator-=()
*/

/*!
    \fn QVector3D &QVector3D::operator-=(const QVector3D &vector)

    Subtracts the given \a vector from this vector and returns a reference to
    this vector.

    \sa operator+=()
*/

/*!
    \fn QVector3D &QVector3D::operator*=(float factor)

    Multiplies this vector's coordinates by the given \a factor, and
    returns a reference to this vector.

    \sa operator/=()
*/

/*!
    \fn QVector3D &QVector3D::operator*=(const QVector3D& vector)
    \overload

    Multiplies the components of this vector by the corresponding
    components in \a vector.

    Note: this is not the same as the crossProduct() of this
    vector and \a vector.

    \sa crossProduct()
*/

/*!
    \fn QVector3D &QVector3D::operator/=(float divisor)

    Divides this vector's coordinates by the given \a divisor, and
    returns a reference to this vector.

    \sa operator*=()
*/

/*!
    \fn QVector3D &QVector3D::operator/=(const QVector3D &vector)
    \since 5.5

    Divides the components of this vector by the corresponding
    components in \a vector.

    \sa operator*=()
*/

/*!
    Returns the dot product of \a v1 and \a v2.
*/
float QVector3D::dotProduct(const QVector3D& v1, const QVector3D& v2)
{
    return v1.xp * v2.xp + v1.yp * v2.yp + v1.zp * v2.zp;
}

/*!
    Returns the cross-product of vectors \a v1 and \a v2, which corresponds
    to the normal vector of a plane defined by \a v1 and \a v2.

    \sa normal()
*/
QVector3D QVector3D::crossProduct(const QVector3D& v1, const QVector3D& v2)
{
    return QVector3D(v1.yp * v2.zp - v1.zp * v2.yp,
                     v1.zp * v2.xp - v1.xp * v2.zp,
                     v1.xp * v2.yp - v1.yp * v2.xp);
}

/*!
    Returns the normal vector of a plane defined by vectors \a v1 and \a v2,
    normalized to be a unit vector.

    Use crossProduct() to compute the cross-product of \a v1 and \a v2 if you
    do not need the result to be normalized to a unit vector.

    \sa crossProduct(), distanceToPlane()
*/
QVector3D QVector3D::normal(const QVector3D& v1, const QVector3D& v2)
{
    return crossProduct(v1, v2).normalized();
}

/*!
    \overload

    Returns the normal vector of a plane defined by vectors
    \a v2 - \a v1 and \a v3 - \a v1, normalized to be a unit vector.

    Use crossProduct() to compute the cross-product of \a v2 - \a v1 and
    \a v3 - \a v1 if you do not need the result to be normalized to a
    unit vector.

    \sa crossProduct(), distanceToPlane()
*/
QVector3D QVector3D::normal
        (const QVector3D& v1, const QVector3D& v2, const QVector3D& v3)
{
    return crossProduct((v2 - v1), (v3 - v1)).normalized();
}

/*!
    \since 5.5

    Returns the window coordinates of this vector initially in object/model
    coordinates using the model view matrix \a modelView, the projection matrix
    \a projection and the viewport dimensions \a viewport.

    When transforming from clip to normalized space, a division by the w
    component on the vector components takes place. To prevent dividing by 0 if
    w equals to 0, it is set to 1.

    \note the returned y coordinates are in OpenGL orientation. OpenGL expects
    the bottom to be 0 whereas for Qt top is 0.

    \sa unproject()
 */
QVector3D QVector3D::project(const QMatrix4x4 &modelView, const QMatrix4x4 &projection, const QRect &viewport) const
{
    QVector4D tmp(*this, 1.0f);
    tmp = projection * modelView * tmp;
    if (qFuzzyIsNull(tmp.w()))
        tmp.setW(1.0f);
    tmp /= tmp.w();

    tmp = tmp * 0.5f + QVector4D(0.5f, 0.5f, 0.5f, 0.5f);
    tmp.setX(tmp.x() * viewport.width() + viewport.x());
    tmp.setY(tmp.y() * viewport.height() + viewport.y());

    return tmp.toVector3D();
}

/*!
    \since 5.5

    Returns the object/model coordinates of this vector initially in window
    coordinates using the model view matrix \a modelView, the projection matrix
    \a projection and the viewport dimensions \a viewport.

    When transforming from clip to normalized space, a division by the w
    component of the vector components takes place. To prevent dividing by 0 if
    w equals to 0, it is set to 1.

    \note y coordinates in \a viewport should use OpenGL orientation. OpenGL
    expects the bottom to be 0 whereas for Qt top is 0.

    \sa project()
 */
QVector3D QVector3D::unproject(const QMatrix4x4 &modelView, const QMatrix4x4 &projection, const QRect &viewport) const
{
    QMatrix4x4 inverse = QMatrix4x4( projection * modelView ).inverted();

    QVector4D tmp(*this, 1.0f);
    tmp.setX((tmp.x() - float(viewport.x())) / float(viewport.width()));
    tmp.setY((tmp.y() - float(viewport.y())) / float(viewport.height()));
    tmp = tmp * 2.0f - QVector4D(1.0f, 1.0f, 1.0f, 1.0f);

    QVector4D obj = inverse * tmp;
    if (qFuzzyIsNull(obj.w()))
        obj.setW(1.0f);
    obj /= obj.w();
    return obj.toVector3D();
}

/*!
    \since 5.1

    Returns the distance from this vertex to a point defined by
    the vertex \a point.

    \sa distanceToPlane(), distanceToLine()
*/
float QVector3D::distanceToPoint(const QVector3D& point) const
{
    return (*this - point).length();
}

/*!
    Returns the distance from this vertex to a plane defined by
    the vertex \a plane and a \a normal unit vector.  The \a normal
    parameter is assumed to have been normalized to a unit vector.

    The return value will be negative if the vertex is below the plane,
    or zero if it is on the plane.

    \sa normal(), distanceToLine()
*/
float QVector3D::distanceToPlane
        (const QVector3D& plane, const QVector3D& normal) const
{
    return dotProduct(*this - plane, normal);
}

/*!
    \overload

    Returns the distance from this vertex a plane defined by
    the vertices \a plane1, \a plane2 and \a plane3.

    The return value will be negative if the vertex is below the plane,
    or zero if it is on the plane.

    The two vectors that define the plane are \a plane2 - \a plane1
    and \a plane3 - \a plane1.

    \sa normal(), distanceToLine()
*/
float QVector3D::distanceToPlane
    (const QVector3D& plane1, const QVector3D& plane2, const QVector3D& plane3) const
{
    QVector3D n = normal(plane2 - plane1, plane3 - plane1);
    return dotProduct(*this - plane1, n);
}

/*!
    Returns the distance that this vertex is from a line defined
    by \a point and the unit vector \a direction.

    If \a direction is a null vector, then it does not define a line.
    In that case, the distance from \a point to this vertex is returned.

    \sa distanceToPlane()
*/
float QVector3D::distanceToLine
        (const QVector3D& point, const QVector3D& direction) const
{
    if (direction.isNull())
        return (*this - point).length();
    QVector3D p = point + dotProduct(*this - point, direction) * direction;
    return (*this - p).length();
}

/*!
    \fn bool operator==(const QVector3D &v1, const QVector3D &v2)
    \relates QVector3D

    Returns \c true if \a v1 is equal to \a v2; otherwise returns \c false.
    This operator uses an exact floating-point comparison.
*/

/*!
    \fn bool operator!=(const QVector3D &v1, const QVector3D &v2)
    \relates QVector3D

    Returns \c true if \a v1 is not equal to \a v2; otherwise returns \c false.
    This operator uses an exact floating-point comparison.
*/

/*!
    \fn const QVector3D operator+(const QVector3D &v1, const QVector3D &v2)
    \relates QVector3D

    Returns a QVector3D object that is the sum of the given vectors, \a v1
    and \a v2; each component is added separately.

    \sa QVector3D::operator+=()
*/

/*!
    \fn const QVector3D operator-(const QVector3D &v1, const QVector3D &v2)
    \relates QVector3D

    Returns a QVector3D object that is formed by subtracting \a v2 from \a v1;
    each component is subtracted separately.

    \sa QVector3D::operator-=()
*/

/*!
    \fn const QVector3D operator*(float factor, const QVector3D &vector)
    \relates QVector3D

    Returns a copy of the given \a vector,  multiplied by the given \a factor.

    \sa QVector3D::operator*=()
*/

/*!
    \fn const QVector3D operator*(const QVector3D &vector, float factor)
    \relates QVector3D

    Returns a copy of the given \a vector,  multiplied by the given \a factor.

    \sa QVector3D::operator*=()
*/

/*!
    \fn const QVector3D operator*(const QVector3D &v1, const QVector3D& v2)
    \relates QVector3D

    Multiplies the components of \a v1 by the corresponding components in \a v2.

    Note: this is not the same as the crossProduct() of \a v1 and \a v2.

    \sa QVector3D::crossProduct()
*/

/*!
    \fn const QVector3D operator-(const QVector3D &vector)
    \relates QVector3D
    \overload

    Returns a QVector3D object that is formed by changing the sign of
    all three components of the given \a vector.

    Equivalent to \c {QVector3D(0,0,0) - vector}.
*/

/*!
    \fn const QVector3D operator/(const QVector3D &vector, float divisor)
    \relates QVector3D

    Returns the QVector3D object formed by dividing all three components of
    the given \a vector by the given \a divisor.

    \sa QVector3D::operator/=()
*/

/*!
    \fn const QVector3D operator/(const QVector3D &vector, const QVector3D &divisor)
    \relates QVector3D
    \since 5.5

    Returns the QVector3D object formed by dividing components of the given
    \a vector by a respective components of the given \a divisor.

    \sa QVector3D::operator/=()
*/

/*!
    \fn bool qFuzzyCompare(const QVector3D& v1, const QVector3D& v2)
    \relates QVector3D

    Returns \c true if \a v1 and \a v2 are equal, allowing for a small
    fuzziness factor for floating-point comparisons; false otherwise.
*/

#ifndef QT_NO_VECTOR2D

/*!
    Returns the 2D vector form of this 3D vector, dropping the z coordinate.

    \sa toVector4D(), toPoint()
*/
QVector2D QVector3D::toVector2D() const
{
    return QVector2D(xp, yp);
}

#endif

#ifndef QT_NO_VECTOR4D

/*!
    Returns the 4D form of this 3D vector, with the w coordinate set to zero.

    \sa toVector2D(), toPoint()
*/
QVector4D QVector3D::toVector4D() const
{
    return QVector4D(xp, yp, zp, 0.0f);
}

#endif

/*!
    \fn QPoint QVector3D::toPoint() const

    Returns the QPoint form of this 3D vector. The z coordinate
    is dropped.

    \sa toPointF(), toVector2D()
*/

/*!
    \fn QPointF QVector3D::toPointF() const

    Returns the QPointF form of this 3D vector. The z coordinate
    is dropped.

    \sa toPoint(), toVector2D()
*/

/*!
    Returns the 3D vector as a QVariant.
*/
QVector3D::operator QVariant() const
{
    return QVariant(QVariant::Vector3D, this);
}

/*!
    Returns the length of the vector from the origin.

    \sa lengthSquared(), normalized()
*/
float QVector3D::length() const
{
    // Need some extra precision if the length is very small.
    double len = double(xp) * double(xp) +
                 double(yp) * double(yp) +
                 double(zp) * double(zp);
    return float(std::sqrt(len));
}

/*!
    Returns the squared length of the vector from the origin.
    This is equivalent to the dot product of the vector with itself.

    \sa length(), dotProduct()
*/
float QVector3D::lengthSquared() const
{
    return xp * xp + yp * yp + zp * zp;
}

#ifndef QT_NO_DEBUG_STREAM

QDebug operator<<(QDebug dbg, const QVector3D &vector)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QVector3D("
        << vector.x() << ", " << vector.y() << ", " << vector.z() << ')';
    return dbg;
}

#endif

#ifndef QT_NO_DATASTREAM

/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QVector3D &vector)
    \relates QVector3D

    Writes the given \a vector to the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &stream, const QVector3D &vector)
{
    stream << vector.x() << vector.y() << vector.z();
    return stream;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QVector3D &vector)
    \relates QVector3D

    Reads a 3D vector from the given \a stream into the given \a vector
    and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &stream, QVector3D &vector)
{
    float x, y, z;
    stream >> x;
    stream >> y;
    stream >> z;
    vector.setX(x);
    vector.setY(y);
    vector.setZ(z);
    return stream;
}

#endif // QT_NO_DATASTREAM

#endif // QT_NO_VECTOR3D

QT_END_NAMESPACE
