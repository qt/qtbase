// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUATERNION_H
#define QQUATERNION_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qgenericmatrix.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qvector4d.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_QUATERNION

class QMatrix4x4;
class QVariant;

class Q_GUI_EXPORT QQuaternion
{
public:
    inline
    QQuaternion();
    explicit QQuaternion(Qt::Initialization) {}
    inline
    QQuaternion(float scalar, float xpos, float ypos, float zpos);
#ifndef QT_NO_VECTOR3D
    inline
    QQuaternion(float scalar, const QVector3D& vector);
#endif
#ifndef QT_NO_VECTOR4D
    inline
    explicit QQuaternion(const QVector4D& vector);
#endif

    inline
    bool isNull() const;
    inline
    bool isIdentity() const;

#ifndef QT_NO_VECTOR3D
    inline
    QVector3D vector() const;
    inline
    void setVector(const QVector3D& vector);
#endif
    inline
    void setVector(float x, float y, float z);

    inline
    float x() const;
    inline
    float y() const;
    inline
    float z() const;
    inline
    float scalar() const;

    inline
    void setX(float x);
    inline
    void setY(float y);
    inline
    void setZ(float z);
    inline
    void setScalar(float scalar);

    constexpr static inline float dotProduct(const QQuaternion &q1, const QQuaternion &q2);

    float length() const;
    float lengthSquared() const;

    [[nodiscard]] QQuaternion normalized() const;
    void normalize();

    inline QQuaternion inverted() const;

    [[nodiscard]] inline QQuaternion conjugated() const;

    QVector3D rotatedVector(const QVector3D& vector) const;

    inline
    QQuaternion &operator+=(const QQuaternion &quaternion);
    inline
    QQuaternion &operator-=(const QQuaternion &quaternion);
    inline
    QQuaternion &operator*=(float factor);
    inline
    QQuaternion &operator*=(const QQuaternion &quaternion);
    inline
    QQuaternion &operator/=(float divisor);

QT_WARNING_PUSH
QT_WARNING_DISABLE_FLOAT_COMPARE
    friend inline bool operator==(const QQuaternion &q1, const QQuaternion &q2) noexcept
    {
        return q1.wp == q2.wp && q1.xp == q2.xp && q1.yp == q2.yp && q1.zp == q2.zp;
    }
    friend inline bool operator!=(const QQuaternion &q1, const QQuaternion &q2) noexcept
    {
        return !(q1 == q2);
    }
QT_WARNING_POP

    friend inline const QQuaternion operator+(const QQuaternion &q1, const QQuaternion &q2);
    friend inline const QQuaternion operator-(const QQuaternion &q1, const QQuaternion &q2);
    friend inline const QQuaternion operator*(float factor, const QQuaternion &quaternion);
    friend inline const QQuaternion operator*(const QQuaternion &quaternion, float factor);
    friend inline const QQuaternion operator*(const QQuaternion &q1, const QQuaternion& q2);
    friend inline const QQuaternion operator-(const QQuaternion &quaternion);
    friend inline const QQuaternion operator/(const QQuaternion &quaternion, float divisor);

    friend inline bool qFuzzyCompare(const QQuaternion& q1, const QQuaternion& q2);

#ifndef QT_NO_VECTOR4D
    inline
    QVector4D toVector4D() const;
#endif

    operator QVariant() const;

#ifndef QT_NO_VECTOR3D
    inline void getAxisAndAngle(QVector3D *axis, float *angle) const;
    static QQuaternion fromAxisAndAngle(const QVector3D& axis, float angle);
#endif
    void getAxisAndAngle(float *x, float *y, float *z, float *angle) const;
    static QQuaternion fromAxisAndAngle
            (float x, float y, float z, float angle);

#ifndef QT_NO_VECTOR3D
    inline QVector3D toEulerAngles() const;
    static inline QQuaternion fromEulerAngles(const QVector3D &angles);
#endif
    void getEulerAngles(float *pitch, float *yaw, float *roll) const;
    static QQuaternion fromEulerAngles(float pitch, float yaw, float roll);

    QMatrix3x3 toRotationMatrix() const;
    static QQuaternion fromRotationMatrix(const QMatrix3x3 &rot3x3);

#ifndef QT_NO_VECTOR3D
    void getAxes(QVector3D *xAxis, QVector3D *yAxis, QVector3D *zAxis) const;
    static QQuaternion fromAxes(const QVector3D &xAxis, const QVector3D &yAxis, const QVector3D &zAxis);

    static QQuaternion fromDirection(const QVector3D &direction, const QVector3D &up);

    static QQuaternion rotationTo(const QVector3D &from, const QVector3D &to);
#endif // QT_NO_VECTOR3D

    static QQuaternion slerp
        (const QQuaternion& q1, const QQuaternion& q2, float t);
    static QQuaternion nlerp
        (const QQuaternion& q1, const QQuaternion& q2, float t);

private:
    float wp, xp, yp, zp;
};

Q_DECLARE_TYPEINFO(QQuaternion, Q_PRIMITIVE_TYPE);

QQuaternion::QQuaternion() : wp(1.0f), xp(0.0f), yp(0.0f), zp(0.0f) {}

QQuaternion::QQuaternion(float aScalar, float xpos, float ypos, float zpos) : wp(aScalar), xp(xpos), yp(ypos), zp(zpos) {}

QT_WARNING_PUSH
QT_WARNING_DISABLE_FLOAT_COMPARE

bool QQuaternion::isNull() const
{
    return wp == 0.0f && xp == 0.0f && yp == 0.0f && zp == 0.0f;
}

bool QQuaternion::isIdentity() const
{
    return wp == 1.0f && xp == 0.0f && yp == 0.0f && zp == 0.0f;
}
QT_WARNING_POP

float QQuaternion::x() const { return xp; }
float QQuaternion::y() const { return yp; }
float QQuaternion::z() const { return zp; }
float QQuaternion::scalar() const { return wp; }

void QQuaternion::setX(float aX) { xp = aX; }
void QQuaternion::setY(float aY) { yp = aY; }
void QQuaternion::setZ(float aZ) { zp = aZ; }
void QQuaternion::setScalar(float aScalar) { wp = aScalar; }

constexpr float QQuaternion::dotProduct(const QQuaternion &q1, const QQuaternion &q2)
{
    return q1.wp * q2.wp + q1.xp * q2.xp + q1.yp * q2.yp + q1.zp * q2.zp;
}

QQuaternion QQuaternion::inverted() const
{
    // Need some extra precision if the length is very small.
    double len = double(wp) * double(wp) +
                 double(xp) * double(xp) +
                 double(yp) * double(yp) +
                 double(zp) * double(zp);
    if (!qFuzzyIsNull(len))
        return QQuaternion(float(double(wp) / len), float(double(-xp) / len),
                           float(double(-yp) / len), float(double(-zp) / len));
    return QQuaternion(0.0f, 0.0f, 0.0f, 0.0f);
}

QQuaternion QQuaternion::conjugated() const
{
    return QQuaternion(wp, -xp, -yp, -zp);
}

QQuaternion &QQuaternion::operator+=(const QQuaternion &quaternion)
{
    wp += quaternion.wp;
    xp += quaternion.xp;
    yp += quaternion.yp;
    zp += quaternion.zp;
    return *this;
}

QQuaternion &QQuaternion::operator-=(const QQuaternion &quaternion)
{
    wp -= quaternion.wp;
    xp -= quaternion.xp;
    yp -= quaternion.yp;
    zp -= quaternion.zp;
    return *this;
}

QQuaternion &QQuaternion::operator*=(float factor)
{
    wp *= factor;
    xp *= factor;
    yp *= factor;
    zp *= factor;
    return *this;
}

const QQuaternion operator*(const QQuaternion &q1, const QQuaternion& q2)
{
    float yy = (q1.wp - q1.yp) * (q2.wp + q2.zp);
    float zz = (q1.wp + q1.yp) * (q2.wp - q2.zp);
    float ww = (q1.zp + q1.xp) * (q2.xp + q2.yp);
    float xx = ww + yy + zz;
    float qq = 0.5f * (xx + (q1.zp - q1.xp) * (q2.xp - q2.yp));

    float w = qq - ww + (q1.zp - q1.yp) * (q2.yp - q2.zp);
    float x = qq - xx + (q1.xp + q1.wp) * (q2.xp + q2.wp);
    float y = qq - yy + (q1.wp - q1.xp) * (q2.yp + q2.zp);
    float z = qq - zz + (q1.zp + q1.yp) * (q2.wp - q2.xp);

    return QQuaternion(w, x, y, z);
}

QQuaternion &QQuaternion::operator*=(const QQuaternion &quaternion)
{
    *this = *this * quaternion;
    return *this;
}

QQuaternion &QQuaternion::operator/=(float divisor)
{
    wp /= divisor;
    xp /= divisor;
    yp /= divisor;
    zp /= divisor;
    return *this;
}

const QQuaternion operator+(const QQuaternion &q1, const QQuaternion &q2)
{
    return QQuaternion(q1.wp + q2.wp, q1.xp + q2.xp, q1.yp + q2.yp, q1.zp + q2.zp);
}

const QQuaternion operator-(const QQuaternion &q1, const QQuaternion &q2)
{
    return QQuaternion(q1.wp - q2.wp, q1.xp - q2.xp, q1.yp - q2.yp, q1.zp - q2.zp);
}

const QQuaternion operator*(float factor, const QQuaternion &quaternion)
{
    return QQuaternion(quaternion.wp * factor, quaternion.xp * factor, quaternion.yp * factor, quaternion.zp * factor);
}

const QQuaternion operator*(const QQuaternion &quaternion, float factor)
{
    return QQuaternion(quaternion.wp * factor, quaternion.xp * factor, quaternion.yp * factor, quaternion.zp * factor);
}

const QQuaternion operator-(const QQuaternion &quaternion)
{
    return QQuaternion(-quaternion.wp, -quaternion.xp, -quaternion.yp, -quaternion.zp);
}

const QQuaternion operator/(const QQuaternion &quaternion, float divisor)
{
    return QQuaternion(quaternion.wp / divisor, quaternion.xp / divisor, quaternion.yp / divisor, quaternion.zp / divisor);
}

bool qFuzzyCompare(const QQuaternion& q1, const QQuaternion& q2)
{
    return qFuzzyCompare(q1.wp, q2.wp) &&
           qFuzzyCompare(q1.xp, q2.xp) &&
           qFuzzyCompare(q1.yp, q2.yp) &&
           qFuzzyCompare(q1.zp, q2.zp);
}

#ifndef QT_NO_VECTOR3D

QQuaternion::QQuaternion(float aScalar, const QVector3D& aVector)
    : wp(aScalar), xp(aVector.x()), yp(aVector.y()), zp(aVector.z()) {}

void QQuaternion::setVector(const QVector3D& aVector)
{
    xp = aVector.x();
    yp = aVector.y();
    zp = aVector.z();
}

QVector3D QQuaternion::vector() const
{
    return QVector3D(xp, yp, zp);
}

inline QVector3D operator*(const QQuaternion &quaternion, const QVector3D &vec)
{
    return quaternion.rotatedVector(vec);
}

void QQuaternion::getAxisAndAngle(QVector3D *axis, float *angle) const
{
    float aX, aY, aZ;
    getAxisAndAngle(&aX, &aY, &aZ, angle);
    *axis = QVector3D(aX, aY, aZ);
}

QVector3D QQuaternion::toEulerAngles() const
{
    float pitch, yaw, roll;
    getEulerAngles(&pitch, &yaw, &roll);
    return QVector3D(pitch, yaw, roll);
}

QQuaternion QQuaternion::fromEulerAngles(const QVector3D &angles)
{
    return QQuaternion::fromEulerAngles(angles.x(), angles.y(), angles.z());
}

#endif // QT_NO_VECTOR3D

void QQuaternion::setVector(float aX, float aY, float aZ)
{
    xp = aX;
    yp = aY;
    zp = aZ;
}

#ifndef QT_NO_VECTOR4D

QQuaternion::QQuaternion(const QVector4D& aVector)
    : wp(aVector.w()), xp(aVector.x()), yp(aVector.y()), zp(aVector.z()) {}

QVector4D QQuaternion::toVector4D() const
{
    return QVector4D(xp, yp, zp, wp);
}

#endif // QT_NO_VECTOR4D

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QQuaternion &q);
#endif

#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QQuaternion &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QQuaternion &);
#endif

#endif // QT_NO_QUATERNION

QT_END_NAMESPACE

#endif // QQUATERNION_H
