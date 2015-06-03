/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qquaternion.h"
#include <QtCore/qdatastream.h>
#include <QtCore/qmath.h>
#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>

#include <cmath>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_QUATERNION

/*!
    \class QQuaternion
    \brief The QQuaternion class represents a quaternion consisting of a vector and scalar.
    \since 4.6
    \ingroup painting-3D
    \inmodule QtGui

    Quaternions are used to represent rotations in 3D space, and
    consist of a 3D rotation axis specified by the x, y, and z
    coordinates, and a scalar representing the rotation angle.
*/

/*!
    \fn QQuaternion::QQuaternion()

    Constructs an identity quaternion (1, 0, 0, 0), i.e. with the vector (0, 0, 0)
    and scalar 1.
*/

/*!
    \fn QQuaternion::QQuaternion(Qt::Initialization)
    \since 5.5
    \internal

    Constructs a quaternion without initializing the contents.
*/

/*!
    \fn QQuaternion::QQuaternion(float scalar, float xpos, float ypos, float zpos)

    Constructs a quaternion with the vector (\a xpos, \a ypos, \a zpos)
    and \a scalar.
*/

#ifndef QT_NO_VECTOR3D

/*!
    \fn QQuaternion::QQuaternion(float scalar, const QVector3D& vector)

    Constructs a quaternion vector from the specified \a vector and
    \a scalar.

    \sa vector(), scalar()
*/

/*!
    \fn QVector3D QQuaternion::vector() const

    Returns the vector component of this quaternion.

    \sa setVector(), scalar()
*/

/*!
    \fn void QQuaternion::setVector(const QVector3D& vector)

    Sets the vector component of this quaternion to \a vector.

    \sa vector(), setScalar()
*/

#endif

/*!
    \fn void QQuaternion::setVector(float x, float y, float z)

    Sets the vector component of this quaternion to (\a x, \a y, \a z).

    \sa vector(), setScalar()
*/

#ifndef QT_NO_VECTOR4D

/*!
    \fn QQuaternion::QQuaternion(const QVector4D& vector)

    Constructs a quaternion from the components of \a vector.
*/

/*!
    \fn QVector4D QQuaternion::toVector4D() const

    Returns this quaternion as a 4D vector.
*/

#endif

/*!
    \fn bool QQuaternion::isNull() const

    Returns \c true if the x, y, z, and scalar components of this
    quaternion are set to 0.0; otherwise returns \c false.
*/

/*!
    \fn bool QQuaternion::isIdentity() const

    Returns \c true if the x, y, and z components of this
    quaternion are set to 0.0, and the scalar component is set
    to 1.0; otherwise returns \c false.
*/

/*!
    \fn float QQuaternion::x() const

    Returns the x coordinate of this quaternion's vector.

    \sa setX(), y(), z(), scalar()
*/

/*!
    \fn float QQuaternion::y() const

    Returns the y coordinate of this quaternion's vector.

    \sa setY(), x(), z(), scalar()
*/

/*!
    \fn float QQuaternion::z() const

    Returns the z coordinate of this quaternion's vector.

    \sa setZ(), x(), y(), scalar()
*/

/*!
    \fn float QQuaternion::scalar() const

    Returns the scalar component of this quaternion.

    \sa setScalar(), x(), y(), z()
*/

/*!
    \fn void QQuaternion::setX(float x)

    Sets the x coordinate of this quaternion's vector to the given
    \a x coordinate.

    \sa x(), setY(), setZ(), setScalar()
*/

/*!
    \fn void QQuaternion::setY(float y)

    Sets the y coordinate of this quaternion's vector to the given
    \a y coordinate.

    \sa y(), setX(), setZ(), setScalar()
*/

/*!
    \fn void QQuaternion::setZ(float z)

    Sets the z coordinate of this quaternion's vector to the given
    \a z coordinate.

    \sa z(), setX(), setY(), setScalar()
*/

/*!
    \fn void QQuaternion::setScalar(float scalar)

    Sets the scalar component of this quaternion to \a scalar.

    \sa scalar(), setX(), setY(), setZ()
*/

/*!
    \fn float QQuaternion::dotProduct(const QQuaternion &q1, const QQuaternion &q2)
    \since 5.5

    Returns the dot product of \a q1 and \a q2.

    \sa length()
*/

/*!
    Returns the length of the quaternion.  This is also called the "norm".

    \sa lengthSquared(), normalized(), dotProduct()
*/
float QQuaternion::length() const
{
    return std::sqrt(xp * xp + yp * yp + zp * zp + wp * wp);
}

/*!
    Returns the squared length of the quaternion.

    \sa length(), dotProduct()
*/
float QQuaternion::lengthSquared() const
{
    return xp * xp + yp * yp + zp * zp + wp * wp;
}

/*!
    Returns the normalized unit form of this quaternion.

    If this quaternion is null, then a null quaternion is returned.
    If the length of the quaternion is very close to 1, then the quaternion
    will be returned as-is.  Otherwise the normalized form of the
    quaternion of length 1 will be returned.

    \sa normalize(), length(), dotProduct()
*/
QQuaternion QQuaternion::normalized() const
{
    // Need some extra precision if the length is very small.
    double len = double(xp) * double(xp) +
                 double(yp) * double(yp) +
                 double(zp) * double(zp) +
                 double(wp) * double(wp);
    if (qFuzzyIsNull(len - 1.0f))
        return *this;
    else if (!qFuzzyIsNull(len))
        return *this / std::sqrt(len);
    else
        return QQuaternion(0.0f, 0.0f, 0.0f, 0.0f);
}

/*!
    Normalizes the current quaternion in place.  Nothing happens if this
    is a null quaternion or the length of the quaternion is very close to 1.

    \sa length(), normalized()
*/
void QQuaternion::normalize()
{
    // Need some extra precision if the length is very small.
    double len = double(xp) * double(xp) +
                 double(yp) * double(yp) +
                 double(zp) * double(zp) +
                 double(wp) * double(wp);
    if (qFuzzyIsNull(len - 1.0f) || qFuzzyIsNull(len))
        return;

    len = std::sqrt(len);

    xp /= len;
    yp /= len;
    zp /= len;
    wp /= len;
}

/*!
    \fn QQuaternion QQuaternion::inverted() const
    \since 5.5

    Returns the inverse of this quaternion.
    If this quaternion is null, then a null quaternion is returned.

    \sa isNull(), length()
*/

/*!
    \fn QQuaternion QQuaternion::conjugated() const
    \since 5.5

    Returns the conjugate of this quaternion, which is
    (-x, -y, -z, scalar).
*/

/*!
    \fn QQuaternion QQuaternion::conjugate() const
    \obsolete

    Use conjugated() instead.
*/

/*!
    Rotates \a vector with this quaternion to produce a new vector
    in 3D space.  The following code:

    \code
    QVector3D result = q.rotatedVector(vector);
    \endcode

    is equivalent to the following:

    \code
    QVector3D result = (q * QQuaternion(0, vector) * q.conjugated()).vector();
    \endcode
*/
QVector3D QQuaternion::rotatedVector(const QVector3D& vector) const
{
    return (*this * QQuaternion(0, vector) * conjugated()).vector();
}

/*!
    \fn QQuaternion &QQuaternion::operator+=(const QQuaternion &quaternion)

    Adds the given \a quaternion to this quaternion and returns a reference to
    this quaternion.

    \sa operator-=()
*/

/*!
    \fn QQuaternion &QQuaternion::operator-=(const QQuaternion &quaternion)

    Subtracts the given \a quaternion from this quaternion and returns a
    reference to this quaternion.

    \sa operator+=()
*/

/*!
    \fn QQuaternion &QQuaternion::operator*=(float factor)

    Multiplies this quaternion's components by the given \a factor, and
    returns a reference to this quaternion.

    \sa operator/=()
*/

/*!
    \fn QQuaternion &QQuaternion::operator*=(const QQuaternion &quaternion)

    Multiplies this quaternion by \a quaternion and returns a reference
    to this quaternion.
*/

/*!
    \fn QQuaternion &QQuaternion::operator/=(float divisor)

    Divides this quaternion's components by the given \a divisor, and
    returns a reference to this quaternion.

    \sa operator*=()
*/

#ifndef QT_NO_VECTOR3D

/*!
    \fn void QQuaternion::getAxisAndAngle(QVector3D *axis, float *angle) const
    \since 5.5
    \overload

    Extracts a 3D axis \a axis and a rotating angle \a angle (in degrees)
    that corresponds to this quaternion.

    \sa fromAxisAndAngle()
*/

/*!
    Creates a normalized quaternion that corresponds to rotating through
    \a angle degrees about the specified 3D \a axis.

    \sa getAxisAndAngle()
*/
QQuaternion QQuaternion::fromAxisAndAngle(const QVector3D& axis, float angle)
{
    // Algorithm from:
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q56
    // We normalize the result just in case the values are close
    // to zero, as suggested in the above FAQ.
    float a = (angle / 2.0f) * M_PI / 180.0f;
    float s = std::sin(a);
    float c = std::cos(a);
    QVector3D ax = axis.normalized();
    return QQuaternion(c, ax.x() * s, ax.y() * s, ax.z() * s).normalized();
}

#endif

/*!
    \since 5.5

    Extracts a 3D axis (\a x, \a y, \a z) and a rotating angle \a angle (in degrees)
    that corresponds to this quaternion.

    \sa fromAxisAndAngle()
*/
void QQuaternion::getAxisAndAngle(float *x, float *y, float *z, float *angle) const
{
    Q_ASSERT(x && y && z && angle);

    // The quaternion representing the rotation is
    //   q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)

    float length = xp * xp + yp * yp + zp * zp;
    if (!qFuzzyIsNull(length)) {
        *x = xp;
        *y = yp;
        *z = zp;
        if (!qFuzzyIsNull(length - 1.0f)) {
            length = std::sqrt(length);
            *x /= length;
            *y /= length;
            *z /= length;
        }
        *angle = 2.0f * std::acos(wp);
    } else {
        // angle is 0 (mod 2*pi), so any axis will fit
        *x = *y = *z = *angle = 0.0f;
    }

    *angle = qRadiansToDegrees(*angle);
}

/*!
    Creates a normalized quaternion that corresponds to rotating through
    \a angle degrees about the 3D axis (\a x, \a y, \a z).

    \sa getAxisAndAngle()
*/
QQuaternion QQuaternion::fromAxisAndAngle
        (float x, float y, float z, float angle)
{
    float length = std::sqrt(x * x + y * y + z * z);
    if (!qFuzzyIsNull(length - 1.0f) && !qFuzzyIsNull(length)) {
        x /= length;
        y /= length;
        z /= length;
    }
    float a = (angle / 2.0f) * M_PI / 180.0f;
    float s = std::sin(a);
    float c = std::cos(a);
    return QQuaternion(c, x * s, y * s, z * s).normalized();
}

#ifndef QT_NO_VECTOR3D

/*!
    \fn QVector3D QQuaternion::toEulerAngles() const
    \since 5.5
    \overload

    Calculates roll, pitch, and yaw Euler angles (in degrees)
    that corresponds to this quaternion.

    \sa fromEulerAngles()
*/

/*!
    \fn QQuaternion QQuaternion::fromEulerAngles(const QVector3D &eulerAngles)
    \since 5.5
    \overload

    Creates a quaternion that corresponds to a rotation of \a eulerAngles:
    eulerAngles.z() degrees around the z axis, eulerAngles.x() degrees around the x axis,
    and eulerAngles.y() degrees around the y axis (in that order).

    \sa toEulerAngles()
*/

#endif // QT_NO_VECTOR3D

/*!
    \since 5.5

    Calculates \a roll, \a pitch, and \a yaw Euler angles (in degrees)
    that corresponds to this quaternion.

    \sa fromEulerAngles()
*/
void QQuaternion::getEulerAngles(float *pitch, float *yaw, float *roll) const
{
    Q_ASSERT(pitch && yaw && roll);

    // Algorithm from:
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q37

    float xx = xp * xp;
    float xy = xp * yp;
    float xz = xp * zp;
    float xw = xp * wp;
    float yy = yp * yp;
    float yz = yp * zp;
    float yw = yp * wp;
    float zz = zp * zp;
    float zw = zp * wp;

    const float lengthSquared = xx + yy + zz + wp * wp;
    if (!qFuzzyIsNull(lengthSquared - 1.0f) && !qFuzzyIsNull(lengthSquared)) {
        xx /= lengthSquared;
        xy /= lengthSquared; // same as (xp / length) * (yp / length)
        xz /= lengthSquared;
        xw /= lengthSquared;
        yy /= lengthSquared;
        yz /= lengthSquared;
        yw /= lengthSquared;
        zz /= lengthSquared;
        zw /= lengthSquared;
    }

    *pitch = std::asin(-2.0f * (yz - xw));
    if (*pitch < M_PI_2) {
        if (*pitch > -M_PI_2) {
            *yaw = std::atan2(2.0f * (xz + yw), 1.0f - 2.0f * (xx + yy));
            *roll = std::atan2(2.0f * (xy + zw), 1.0f - 2.0f * (xx + zz));
        } else {
            // not a unique solution
            *roll = 0.0f;
            *yaw = -std::atan2(-2.0f * (xy - zw), 1.0f - 2.0f * (yy + zz));
        }
    } else {
        // not a unique solution
        *roll = 0.0f;
        *yaw = std::atan2(-2.0f * (xy - zw), 1.0f - 2.0f * (yy + zz));
    }

    *pitch = qRadiansToDegrees(*pitch);
    *yaw = qRadiansToDegrees(*yaw);
    *roll = qRadiansToDegrees(*roll);
}

/*!
    \since 5.5

    Creates a quaternion that corresponds to a rotation of
    \a roll degrees around the z axis, \a pitch degrees around the x axis,
    and \a yaw degrees around the y axis (in that order).

    \sa getEulerAngles()
*/
QQuaternion QQuaternion::fromEulerAngles(float pitch, float yaw, float roll)
{
    // Algorithm from:
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q60

    pitch = qDegreesToRadians(pitch);
    yaw = qDegreesToRadians(yaw);
    roll = qDegreesToRadians(roll);

    pitch *= 0.5f;
    yaw *= 0.5f;
    roll *= 0.5f;

    const float c1 = std::cos(yaw);
    const float s1 = std::sin(yaw);
    const float c2 = std::cos(roll);
    const float s2 = std::sin(roll);
    const float c3 = std::cos(pitch);
    const float s3 = std::sin(pitch);
    const float c1c2 = c1 * c2;
    const float s1s2 = s1 * s2;

    const float w = c1c2 * c3 + s1s2 * s3;
    const float x = c1c2 * s3 + s1s2 * c3;
    const float y = s1 * c2 * c3 - c1 * s2 * s3;
    const float z = c1 * s2 * c3 - s1 * c2 * s3;

    return QQuaternion(w, x, y, z);
}

/*!
    \since 5.5

    Creates a rotation matrix that corresponds to this quaternion.

    \note If this quaternion is not normalized,
    the resulting rotation matrix will contain scaling information.

    \sa fromRotationMatrix(), getAxes()
*/
QMatrix3x3 QQuaternion::toRotationMatrix() const
{
    // Algorithm from:
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q54

    QMatrix3x3 rot3x3(Qt::Uninitialized);

    const float f2x = xp + xp;
    const float f2y = yp + yp;
    const float f2z = zp + zp;
    const float f2xw = f2x * wp;
    const float f2yw = f2y * wp;
    const float f2zw = f2z * wp;
    const float f2xx = f2x * xp;
    const float f2xy = f2x * yp;
    const float f2xz = f2x * zp;
    const float f2yy = f2y * yp;
    const float f2yz = f2y * zp;
    const float f2zz = f2z * zp;

    rot3x3(0, 0) = 1.0f - (f2yy + f2zz);
    rot3x3(0, 1) =         f2xy - f2zw;
    rot3x3(0, 2) =         f2xz + f2yw;
    rot3x3(1, 0) =         f2xy + f2zw;
    rot3x3(1, 1) = 1.0f - (f2xx + f2zz);
    rot3x3(1, 2) =         f2yz - f2xw;
    rot3x3(2, 0) =         f2xz - f2yw;
    rot3x3(2, 1) =         f2yz + f2xw;
    rot3x3(2, 2) = 1.0f - (f2xx + f2yy);

    return rot3x3;
}

/*!
    \since 5.5

    Creates a quaternion that corresponds to a rotation matrix \a rot3x3.

    \note If a given rotation matrix is not normalized,
    the resulting quaternion will contain scaling information.

    \sa toRotationMatrix(), fromAxes()
*/
QQuaternion QQuaternion::fromRotationMatrix(const QMatrix3x3 &rot3x3)
{
    // Algorithm from:
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q55

    float scalar;
    float axis[3];

    const float trace = rot3x3(0, 0) + rot3x3(1, 1) + rot3x3(2, 2);
    if (trace > 0.00000001f) {
        const float s = 2.0f * std::sqrt(trace + 1.0f);
        scalar = 0.25f * s;
        axis[0] = (rot3x3(2, 1) - rot3x3(1, 2)) / s;
        axis[1] = (rot3x3(0, 2) - rot3x3(2, 0)) / s;
        axis[2] = (rot3x3(1, 0) - rot3x3(0, 1)) / s;
    } else {
        static int s_next[3] = { 1, 2, 0 };
        int i = 0;
        if (rot3x3(1, 1) > rot3x3(0, 0))
            i = 1;
        if (rot3x3(2, 2) > rot3x3(i, i))
            i = 2;
        int j = s_next[i];
        int k = s_next[j];

        const float s = 2.0f * std::sqrt(rot3x3(i, i) - rot3x3(j, j) - rot3x3(k, k) + 1.0f);
        axis[i] = 0.25f * s;
        scalar = (rot3x3(k, j) - rot3x3(j, k)) / s;
        axis[j] = (rot3x3(j, i) + rot3x3(i, j)) / s;
        axis[k] = (rot3x3(k, i) + rot3x3(i, k)) / s;
    }

    return QQuaternion(scalar, axis[0], axis[1], axis[2]);
}

#ifndef QT_NO_VECTOR3D

/*!
    \since 5.5

    Returns the 3 orthonormal axes (\a xAxis, \a yAxis, \a zAxis) defining the quaternion.

    \sa fromAxes(), toRotationMatrix()
*/
void QQuaternion::getAxes(QVector3D *xAxis, QVector3D *yAxis, QVector3D *zAxis) const
{
    Q_ASSERT(xAxis && yAxis && zAxis);

    const QMatrix3x3 rot3x3(toRotationMatrix());

    *xAxis = QVector3D(rot3x3(0, 0), rot3x3(1, 0), rot3x3(2, 0));
    *yAxis = QVector3D(rot3x3(0, 1), rot3x3(1, 1), rot3x3(2, 1));
    *zAxis = QVector3D(rot3x3(0, 2), rot3x3(1, 2), rot3x3(2, 2));
}

/*!
    \since 5.5

    Constructs the quaternion using 3 axes (\a xAxis, \a yAxis, \a zAxis).

    \note The axes are assumed to be orthonormal.

    \sa getAxes(), fromRotationMatrix()
*/
QQuaternion QQuaternion::fromAxes(const QVector3D &xAxis, const QVector3D &yAxis, const QVector3D &zAxis)
{
    QMatrix3x3 rot3x3(Qt::Uninitialized);
    rot3x3(0, 0) = xAxis.x();
    rot3x3(1, 0) = xAxis.y();
    rot3x3(2, 0) = xAxis.z();
    rot3x3(0, 1) = yAxis.x();
    rot3x3(1, 1) = yAxis.y();
    rot3x3(2, 1) = yAxis.z();
    rot3x3(0, 2) = zAxis.x();
    rot3x3(1, 2) = zAxis.y();
    rot3x3(2, 2) = zAxis.z();

    return QQuaternion::fromRotationMatrix(rot3x3);
}

/*!
    \since 5.5

    Constructs the quaternion using specified forward direction \a direction
    and upward direction \a up.
    If the upward direction was not specified or the forward and upward
    vectors are collinear, a new orthonormal upward direction will be generated.

    \sa fromAxes(), rotationTo()
*/
QQuaternion QQuaternion::fromDirection(const QVector3D &direction, const QVector3D &up)
{
    if (qFuzzyIsNull(direction.x()) && qFuzzyIsNull(direction.y()) && qFuzzyIsNull(direction.z()))
        return QQuaternion();

    const QVector3D zAxis(direction.normalized());
    QVector3D xAxis(QVector3D::crossProduct(up, zAxis));
    if (qFuzzyIsNull(xAxis.lengthSquared())) {
        // collinear or invalid up vector; derive shortest arc to new direction
        return QQuaternion::rotationTo(QVector3D(0.0f, 0.0f, 1.0f), zAxis);
    }

    xAxis.normalize();
    const QVector3D yAxis(QVector3D::crossProduct(zAxis, xAxis));

    return QQuaternion::fromAxes(xAxis, yAxis, zAxis);
}

/*!
    \since 5.5

    Returns the shortest arc quaternion to rotate from the direction described by the vector \a from
    to the direction described by the vector \a to.

    \sa fromDirection()
*/
QQuaternion QQuaternion::rotationTo(const QVector3D &from, const QVector3D &to)
{
    // Based on Stan Melax's article in Game Programming Gems

    const QVector3D v0(from.normalized());
    const QVector3D v1(to.normalized());

    float d = QVector3D::dotProduct(v0, v1) + 1.0f;

    // if dest vector is close to the inverse of source vector, ANY axis of rotation is valid
    if (qFuzzyIsNull(d)) {
        QVector3D axis = QVector3D::crossProduct(QVector3D(1.0f, 0.0f, 0.0f), v0);
        if (qFuzzyIsNull(axis.lengthSquared()))
            axis = QVector3D::crossProduct(QVector3D(0.0f, 1.0f, 0.0f), v0);
        axis.normalize();

        // same as QQuaternion::fromAxisAndAngle(axis, 180.0f)
        return QQuaternion(0.0f, axis.x(), axis.y(), axis.z());
    }

    d = std::sqrt(2.0f * d);
    const QVector3D axis(QVector3D::crossProduct(v0, v1) / d);

    return QQuaternion(d * 0.5f, axis).normalized();
}

#endif // QT_NO_VECTOR3D

/*!
    \fn bool operator==(const QQuaternion &q1, const QQuaternion &q2)
    \relates QQuaternion

    Returns \c true if \a q1 is equal to \a q2; otherwise returns \c false.
    This operator uses an exact floating-point comparison.
*/

/*!
    \fn bool operator!=(const QQuaternion &q1, const QQuaternion &q2)
    \relates QQuaternion

    Returns \c true if \a q1 is not equal to \a q2; otherwise returns \c false.
    This operator uses an exact floating-point comparison.
*/

/*!
    \fn const QQuaternion operator+(const QQuaternion &q1, const QQuaternion &q2)
    \relates QQuaternion

    Returns a QQuaternion object that is the sum of the given quaternions,
    \a q1 and \a q2; each component is added separately.

    \sa QQuaternion::operator+=()
*/

/*!
    \fn const QQuaternion operator-(const QQuaternion &q1, const QQuaternion &q2)
    \relates QQuaternion

    Returns a QQuaternion object that is formed by subtracting
    \a q2 from \a q1; each component is subtracted separately.

    \sa QQuaternion::operator-=()
*/

/*!
    \fn const QQuaternion operator*(float factor, const QQuaternion &quaternion)
    \relates QQuaternion

    Returns a copy of the given \a quaternion,  multiplied by the
    given \a factor.

    \sa QQuaternion::operator*=()
*/

/*!
    \fn const QQuaternion operator*(const QQuaternion &quaternion, float factor)
    \relates QQuaternion

    Returns a copy of the given \a quaternion,  multiplied by the
    given \a factor.

    \sa QQuaternion::operator*=()
*/

/*!
    \fn const QQuaternion operator*(const QQuaternion &q1, const QQuaternion& q2)
    \relates QQuaternion

    Multiplies \a q1 and \a q2 using quaternion multiplication.
    The result corresponds to applying both of the rotations specified
    by \a q1 and \a q2.

    \sa QQuaternion::operator*=()
*/

/*!
    \fn const QQuaternion operator-(const QQuaternion &quaternion)
    \relates QQuaternion
    \overload

    Returns a QQuaternion object that is formed by changing the sign of
    all three components of the given \a quaternion.

    Equivalent to \c {QQuaternion(0,0,0,0) - quaternion}.
*/

/*!
    \fn const QQuaternion operator/(const QQuaternion &quaternion, float divisor)
    \relates QQuaternion

    Returns the QQuaternion object formed by dividing all components of
    the given \a quaternion by the given \a divisor.

    \sa QQuaternion::operator/=()
*/

#ifndef QT_NO_VECTOR3D

/*!
    \fn QVector3D operator*(const QQuaternion &quaternion, const QVector3D &vec)
    \since 5.5
    \relates QQuaternion

    Rotates a vector \a vec with a quaternion \a quaternion to produce a new vector in 3D space.
*/

#endif

/*!
    \fn bool qFuzzyCompare(const QQuaternion& q1, const QQuaternion& q2)
    \relates QQuaternion

    Returns \c true if \a q1 and \a q2 are equal, allowing for a small
    fuzziness factor for floating-point comparisons; false otherwise.
*/

/*!
    Interpolates along the shortest spherical path between the
    rotational positions \a q1 and \a q2.  The value \a t should
    be between 0 and 1, indicating the spherical distance to travel
    between \a q1 and \a q2.

    If \a t is less than or equal to 0, then \a q1 will be returned.
    If \a t is greater than or equal to 1, then \a q2 will be returned.

    \sa nlerp()
*/
QQuaternion QQuaternion::slerp
    (const QQuaternion& q1, const QQuaternion& q2, float t)
{
    // Handle the easy cases first.
    if (t <= 0.0f)
        return q1;
    else if (t >= 1.0f)
        return q2;

    // Determine the angle between the two quaternions.
    QQuaternion q2b(q2);
    float dot = QQuaternion::dotProduct(q1, q2);
    if (dot < 0.0f) {
        q2b = -q2b;
        dot = -dot;
    }

    // Get the scale factors.  If they are too small,
    // then revert to simple linear interpolation.
    float factor1 = 1.0f - t;
    float factor2 = t;
    if ((1.0f - dot) > 0.0000001) {
        float angle = std::acos(dot);
        float sinOfAngle = std::sin(angle);
        if (sinOfAngle > 0.0000001) {
            factor1 = std::sin((1.0f - t) * angle) / sinOfAngle;
            factor2 = std::sin(t * angle) / sinOfAngle;
        }
    }

    // Construct the result quaternion.
    return q1 * factor1 + q2b * factor2;
}

/*!
    Interpolates along the shortest linear path between the rotational
    positions \a q1 and \a q2.  The value \a t should be between 0 and 1,
    indicating the distance to travel between \a q1 and \a q2.
    The result will be normalized().

    If \a t is less than or equal to 0, then \a q1 will be returned.
    If \a t is greater than or equal to 1, then \a q2 will be returned.

    The nlerp() function is typically faster than slerp() and will
    give approximate results to spherical interpolation that are
    good enough for some applications.

    \sa slerp()
*/
QQuaternion QQuaternion::nlerp
    (const QQuaternion& q1, const QQuaternion& q2, float t)
{
    // Handle the easy cases first.
    if (t <= 0.0f)
        return q1;
    else if (t >= 1.0f)
        return q2;

    // Determine the angle between the two quaternions.
    QQuaternion q2b(q2);
    float dot = QQuaternion::dotProduct(q1, q2);
    if (dot < 0.0f)
        q2b = -q2b;

    // Perform the linear interpolation.
    return (q1 * (1.0f - t) + q2b * t).normalized();
}

/*!
    Returns the quaternion as a QVariant.
*/
QQuaternion::operator QVariant() const
{
    return QVariant(QVariant::Quaternion, this);
}

#ifndef QT_NO_DEBUG_STREAM

QDebug operator<<(QDebug dbg, const QQuaternion &q)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QQuaternion(scalar:" << q.scalar()
        << ", vector:(" << q.x() << ", "
        << q.y() << ", " << q.z() << "))";
    return dbg;
}

#endif

#ifndef QT_NO_DATASTREAM

/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QQuaternion &quaternion)
    \relates QQuaternion

    Writes the given \a quaternion to the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &stream, const QQuaternion &quaternion)
{
    stream << quaternion.scalar() << quaternion.x()
           << quaternion.y() << quaternion.z();
    return stream;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QQuaternion &quaternion)
    \relates QQuaternion

    Reads a quaternion from the given \a stream into the given \a quaternion
    and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &stream, QQuaternion &quaternion)
{
    float scalar, x, y, z;
    stream >> scalar;
    stream >> x;
    stream >> y;
    stream >> z;
    quaternion.setScalar(scalar);
    quaternion.setX(x);
    quaternion.setY(y);
    quaternion.setZ(z);
    return stream;
}

#endif // QT_NO_DATASTREAM

#endif

QT_END_NAMESPACE
