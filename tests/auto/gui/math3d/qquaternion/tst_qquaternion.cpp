/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/qmath.h>
#include <QtGui/qquaternion.h>

// This is a more tolerant version of qFuzzyCompare that also handles the case
// where one or more of the values being compare are close to zero
static inline bool myFuzzyCompare(float p1, float p2)
{
    if (qFuzzyIsNull(p1) && qFuzzyIsNull(p2))
        return true;
    return qAbs(qAbs(p1) - qAbs(p2)) <= 0.00003f;
}

static inline bool myFuzzyCompare(const QVector3D &v1, const QVector3D &v2)
{
    return myFuzzyCompare(v1.x(), v2.x())
            && myFuzzyCompare(v1.y(), v2.y())
            && myFuzzyCompare(v1.z(), v2.z());
}

static inline bool myFuzzyCompare(const QQuaternion &q1, const QQuaternion &q2)
{
    const float d = QQuaternion::dotProduct(q1, q2);
    return myFuzzyCompare(d * d, 1.0f);
}

static inline bool myFuzzyCompareRadians(float p1, float p2)
{
    static const float fPI = float(M_PI);
    if (p1 < -fPI)
        p1 += 2.0f * fPI;
    else if (p1 > fPI)
        p1 -= 2.0f * fPI;

    if (p2 < -fPI)
        p2 += 2.0f * fPI;
    else if (p2 > fPI)
        p2 -= 2.0f * fPI;

    return qAbs(qAbs(p1) - qAbs(p2)) <= qDegreesToRadians(0.05f);
}

static inline bool myFuzzyCompareDegrees(float p1, float p2)
{
    p1 = qDegreesToRadians(p1);
    p2 = qDegreesToRadians(p2);
    return myFuzzyCompareRadians(p1, p2);
}


class tst_QQuaternion : public QObject
{
    Q_OBJECT
public:
    tst_QQuaternion() {}
    ~tst_QQuaternion() {}

private slots:
    void create();

    void dotProduct_data();
    void dotProduct();

    void length_data();
    void length();

    void normalized_data();
    void normalized();

    void normalize_data();
    void normalize();

    void inverted_data();
    void inverted();

    void compare();

    void add_data();
    void add();

    void subtract_data();
    void subtract();

    void multiply_data();
    void multiply();

    void multiplyFactor_data();
    void multiplyFactor();

    void divide_data();
    void divide();

    void negate_data();
    void negate();

    void conjugate_data();
    void conjugate();

    void fromAxisAndAngle_data();
    void fromAxisAndAngle();

    void fromRotationMatrix_data();
    void fromRotationMatrix();

    void fromAxes_data();
    void fromAxes();

    void rotationTo_data();
    void rotationTo();

    void fromDirection_data();
    void fromDirection();

    void fromEulerAngles_data();
    void fromEulerAngles();

    void slerp_data();
    void slerp();

    void nlerp_data();
    void nlerp();

    void properties();
    void metaTypes();
};

// Test the creation of QQuaternion objects in various ways:
// construct, copy, and modify.
void tst_QQuaternion::create()
{
    QQuaternion identity;
    QCOMPARE(identity.x(), 0.0f);
    QCOMPARE(identity.y(), 0.0f);
    QCOMPARE(identity.z(), 0.0f);
    QCOMPARE(identity.scalar(), 1.0f);
    QVERIFY(identity.isIdentity());

    QQuaternion negativeZeroIdentity(1.0f, -0.0f, -0.0f, -0.0f);
    QCOMPARE(negativeZeroIdentity.x(), -0.0f);
    QCOMPARE(negativeZeroIdentity.y(), -0.0f);
    QCOMPARE(negativeZeroIdentity.z(), -0.0f);
    QCOMPARE(negativeZeroIdentity.scalar(), 1.0f);
    QVERIFY(negativeZeroIdentity.isIdentity());

    QQuaternion v1(34.0f, 1.0f, 2.5f, -89.25f);
    QCOMPARE(v1.x(), 1.0f);
    QCOMPARE(v1.y(), 2.5f);
    QCOMPARE(v1.z(), -89.25f);
    QCOMPARE(v1.scalar(), 34.0f);
    QVERIFY(!v1.isNull());

    QQuaternion v1i(34, 1, 2, -89);
    QCOMPARE(v1i.x(), 1.0f);
    QCOMPARE(v1i.y(), 2.0f);
    QCOMPARE(v1i.z(), -89.0f);
    QCOMPARE(v1i.scalar(), 34.0f);
    QVERIFY(!v1i.isNull());

    QQuaternion v2(v1);
    QCOMPARE(v2.x(), 1.0f);
    QCOMPARE(v2.y(), 2.5f);
    QCOMPARE(v2.z(), -89.25f);
    QCOMPARE(v2.scalar(), 34.0f);
    QVERIFY(!v2.isNull());

    QQuaternion v4;
    QCOMPARE(v4.x(), 0.0f);
    QCOMPARE(v4.y(), 0.0f);
    QCOMPARE(v4.z(), 0.0f);
    QCOMPARE(v4.scalar(), 1.0f);
    QVERIFY(v4.isIdentity());
    v4 = v1;
    QCOMPARE(v4.x(), 1.0f);
    QCOMPARE(v4.y(), 2.5f);
    QCOMPARE(v4.z(), -89.25f);
    QCOMPARE(v4.scalar(), 34.0f);
    QVERIFY(!v4.isNull());

    QQuaternion v9(34, QVector3D(1.0f, 2.5f, -89.25f));
    QCOMPARE(v9.x(), 1.0f);
    QCOMPARE(v9.y(), 2.5f);
    QCOMPARE(v9.z(), -89.25f);
    QCOMPARE(v9.scalar(), 34.0f);
    QVERIFY(!v9.isNull());

    v1.setX(3.0f);
    QCOMPARE(v1.x(), 3.0f);
    QCOMPARE(v1.y(), 2.5f);
    QCOMPARE(v1.z(), -89.25f);
    QCOMPARE(v1.scalar(), 34.0f);
    QVERIFY(!v1.isNull());

    v1.setY(10.5f);
    QCOMPARE(v1.x(), 3.0f);
    QCOMPARE(v1.y(), 10.5f);
    QCOMPARE(v1.z(), -89.25f);
    QCOMPARE(v1.scalar(), 34.0f);
    QVERIFY(!v1.isNull());

    v1.setZ(15.5f);
    QCOMPARE(v1.x(), 3.0f);
    QCOMPARE(v1.y(), 10.5f);
    QCOMPARE(v1.z(), 15.5f);
    QCOMPARE(v1.scalar(), 34.0f);
    QVERIFY(!v1.isNull());

    v1.setScalar(6.0f);
    QCOMPARE(v1.x(), 3.0f);
    QCOMPARE(v1.y(), 10.5f);
    QCOMPARE(v1.z(), 15.5f);
    QCOMPARE(v1.scalar(), 6.0f);
    QVERIFY(!v1.isNull());

    v1.setVector(2.0f, 6.5f, -1.25f);
    QCOMPARE(v1.x(), 2.0f);
    QCOMPARE(v1.y(), 6.5f);
    QCOMPARE(v1.z(), -1.25f);
    QCOMPARE(v1.scalar(), 6.0f);
    QVERIFY(!v1.isNull());
    QVERIFY(v1.vector() == QVector3D(2.0f, 6.5f, -1.25f));

    v1.setVector(QVector3D(-2.0f, -6.5f, 1.25f));
    QCOMPARE(v1.x(), -2.0f);
    QCOMPARE(v1.y(), -6.5f);
    QCOMPARE(v1.z(), 1.25f);
    QCOMPARE(v1.scalar(), 6.0f);
    QVERIFY(!v1.isNull());
    QVERIFY(v1.vector() == QVector3D(-2.0f, -6.5f, 1.25f));

    v1.setX(0.0f);
    v1.setY(0.0f);
    v1.setZ(0.0f);
    v1.setScalar(0.0f);
    QCOMPARE(v1.x(), 0.0f);
    QCOMPARE(v1.y(), 0.0f);
    QCOMPARE(v1.z(), 0.0f);
    QCOMPARE(v1.scalar(), 0.0f);
    QVERIFY(v1.isNull());

    QVector4D v10 = v9.toVector4D();
    QCOMPARE(v10.x(), 1.0f);
    QCOMPARE(v10.y(), 2.5f);
    QCOMPARE(v10.z(), -89.25f);
    QCOMPARE(v10.w(), 34.0f);
}

// Test the computation of dot product.
void tst_QQuaternion::dotProduct_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("scalar1");
    QTest::addColumn<float>("x2");
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("z2");
    QTest::addColumn<float>("scalar2");
    QTest::addColumn<float>("dot");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 0.0f << 0.0f
        << 0.0f;

    QTest::newRow("identity")
        << 0.0f << 0.0f << 0.0f << 1.0f
        << 0.0f << 0.0f << 0.0f << 1.0f
        << 1.0f;

    QTest::newRow("unitvec")
        << 1.0f << 0.0f << 0.0f << 0.0f
        << 0.0f << 1.0f << 0.0f << 0.0f
        << 0.0f;

    QTest::newRow("complex")
        << 1.0f << 2.0f << 3.0f << 4.0f
        << 4.0f << 5.0f << 6.0f << 7.0f
        << 60.0f;
}
void tst_QQuaternion::dotProduct()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, scalar1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, scalar2);
    QFETCH(float, dot);

    QQuaternion q1(scalar1, x1, y1, z1);
    QQuaternion q2(scalar2, x2, y2, z2);

    QCOMPARE(QQuaternion::dotProduct(q1, q2), dot);
    QCOMPARE(QQuaternion::dotProduct(q2, q1), dot);
}

// Test length computation for quaternions.
void tst_QQuaternion::length_data()
{
    QTest::addColumn<float>("x");
    QTest::addColumn<float>("y");
    QTest::addColumn<float>("z");
    QTest::addColumn<float>("w");
    QTest::addColumn<float>("len");

    QTest::newRow("null") << 0.0f << 0.0f << 0.0f << 0.0f << 0.0f;
    QTest::newRow("1x") << 1.0f << 0.0f << 0.0f << 0.0f << 1.0f;
    QTest::newRow("1y") << 0.0f << 1.0f << 0.0f << 0.0f << 1.0f;
    QTest::newRow("1z") << 0.0f << 0.0f << 1.0f << 0.0f << 1.0f;
    QTest::newRow("1w") << 0.0f << 0.0f << 0.0f << 1.0f << 1.0f;
    QTest::newRow("-1x") << -1.0f << 0.0f << 0.0f << 0.0f << 1.0f;
    QTest::newRow("-1y") << 0.0f << -1.0f << 0.0f << 0.0f << 1.0f;
    QTest::newRow("-1z") << 0.0f << 0.0f << -1.0f << 0.0f << 1.0f;
    QTest::newRow("-1w") << 0.0f << 0.0f << 0.0f << -1.0f << 1.0f;
    QTest::newRow("two") << 2.0f << -2.0f << 2.0f << 2.0f << std::sqrt(16.0f);
}
void tst_QQuaternion::length()
{
    QFETCH(float, x);
    QFETCH(float, y);
    QFETCH(float, z);
    QFETCH(float, w);
    QFETCH(float, len);

    QQuaternion v(w, x, y, z);
    QCOMPARE(v.length(), len);
    QCOMPARE(v.lengthSquared(), x * x + y * y + z * z + w * w);
}

// Test the unit vector conversion for quaternions.
void tst_QQuaternion::normalized_data()
{
    // Use the same test data as the length test.
    length_data();
}
void tst_QQuaternion::normalized()
{
    QFETCH(float, x);
    QFETCH(float, y);
    QFETCH(float, z);
    QFETCH(float, w);
    QFETCH(float, len);

    QQuaternion v(w, x, y, z);
    QQuaternion u = v.normalized();
    if (v.isNull())
        QVERIFY(u.isNull());
    else
        QCOMPARE(u.length(), 1.0f);
    QCOMPARE(u.x() * len, v.x());
    QCOMPARE(u.y() * len, v.y());
    QCOMPARE(u.z() * len, v.z());
    QCOMPARE(u.scalar() * len, v.scalar());
}

// Test the unit vector conversion for quaternions.
void tst_QQuaternion::normalize_data()
{
    // Use the same test data as the length test.
    length_data();
}
void tst_QQuaternion::normalize()
{
    QFETCH(float, x);
    QFETCH(float, y);
    QFETCH(float, z);
    QFETCH(float, w);

    QQuaternion v(w, x, y, z);
    bool isNull = v.isNull();
    v.normalize();
    if (isNull)
        QVERIFY(v.isNull());
    else
        QCOMPARE(v.length(), 1.0f);
}

void tst_QQuaternion::inverted_data()
{
    // Use the same test data as the length test.
    length_data();
}
void tst_QQuaternion::inverted()
{
    QFETCH(float, x);
    QFETCH(float, y);
    QFETCH(float, z);
    QFETCH(float, w);
    QFETCH(float, len);

    QQuaternion v(w, x, y, z);
    QQuaternion u = v.inverted();
    if (v.isNull()) {
        QVERIFY(u.isNull());
    } else {
        len *= len;
        QCOMPARE(-u.x() * len, v.x());
        QCOMPARE(-u.y() * len, v.y());
        QCOMPARE(-u.z() * len, v.z());
        QCOMPARE(u.scalar() * len, v.scalar());
    }
}

// Test the comparison operators for quaternions.
void tst_QQuaternion::compare()
{
    QQuaternion v1(8, 1, 2, 4);
    QQuaternion v2(8, 1, 2, 4);
    QQuaternion v3(8, 3, 2, 4);
    QQuaternion v4(8, 1, 3, 4);
    QQuaternion v5(8, 1, 2, 3);
    QQuaternion v6(3, 1, 2, 4);

    QCOMPARE(v1, v2);
    QVERIFY(v1 != v3);
    QVERIFY(v1 != v4);
    QVERIFY(v1 != v5);
    QVERIFY(v1 != v6);
}

// Test addition for quaternions.
void tst_QQuaternion::add_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("w1");
    QTest::addColumn<float>("x2");
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("z2");
    QTest::addColumn<float>("w2");
    QTest::addColumn<float>("x3");
    QTest::addColumn<float>("y3");
    QTest::addColumn<float>("z3");
    QTest::addColumn<float>("w3");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 0.0f << 0.0f;

    QTest::newRow("xonly")
        << 1.0f << 0.0f << 0.0f << 0.0f
        << 2.0f << 0.0f << 0.0f << 0.0f
        << 3.0f << 0.0f << 0.0f << 0.0f;

    QTest::newRow("yonly")
        << 0.0f << 1.0f << 0.0f << 0.0f
        << 0.0f << 2.0f << 0.0f << 0.0f
        << 0.0f << 3.0f << 0.0f << 0.0f;

    QTest::newRow("zonly")
        << 0.0f << 0.0f << 1.0f << 0.0f
        << 0.0f << 0.0f << 2.0f << 0.0f
        << 0.0f << 0.0f << 3.0f << 0.0f;

    QTest::newRow("wonly")
        << 0.0f << 0.0f << 0.0f << 1.0f
        << 0.0f << 0.0f << 0.0f << 2.0f
        << 0.0f << 0.0f << 0.0f << 3.0f;

    QTest::newRow("all")
        << 1.0f << 2.0f << 3.0f << 8.0f
        << 4.0f << 5.0f << -6.0f << 9.0f
        << 5.0f << 7.0f << -3.0f << 17.0f;
}
void tst_QQuaternion::add()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, w1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, w2);
    QFETCH(float, x3);
    QFETCH(float, y3);
    QFETCH(float, z3);
    QFETCH(float, w3);

    QQuaternion v1(w1, x1, y1, z1);
    QQuaternion v2(w2, x2, y2, z2);
    QQuaternion v3(w3, x3, y3, z3);

    QVERIFY((v1 + v2) == v3);

    QQuaternion v4(v1);
    v4 += v2;
    QCOMPARE(v4, v3);

    QCOMPARE(v4.x(), v1.x() + v2.x());
    QCOMPARE(v4.y(), v1.y() + v2.y());
    QCOMPARE(v4.z(), v1.z() + v2.z());
    QCOMPARE(v4.scalar(), v1.scalar() + v2.scalar());
}

// Test subtraction for quaternions.
void tst_QQuaternion::subtract_data()
{
    // Use the same test data as the add test.
    add_data();
}
void tst_QQuaternion::subtract()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, w1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, w2);
    QFETCH(float, x3);
    QFETCH(float, y3);
    QFETCH(float, z3);
    QFETCH(float, w3);

    QQuaternion v1(w1, x1, y1, z1);
    QQuaternion v2(w2, x2, y2, z2);
    QQuaternion v3(w3, x3, y3, z3);

    QVERIFY((v3 - v1) == v2);
    QVERIFY((v3 - v2) == v1);

    QQuaternion v4(v3);
    v4 -= v1;
    QCOMPARE(v4, v2);

    QCOMPARE(v4.x(), v3.x() - v1.x());
    QCOMPARE(v4.y(), v3.y() - v1.y());
    QCOMPARE(v4.z(), v3.z() - v1.z());
    QCOMPARE(v4.scalar(), v3.scalar() - v1.scalar());

    QQuaternion v5(v3);
    v5 -= v2;
    QCOMPARE(v5, v1);

    QCOMPARE(v5.x(), v3.x() - v2.x());
    QCOMPARE(v5.y(), v3.y() - v2.y());
    QCOMPARE(v5.z(), v3.z() - v2.z());
    QCOMPARE(v5.scalar(), v3.scalar() - v2.scalar());
}

// Test quaternion multiplication.
void tst_QQuaternion::multiply_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("w1");
    QTest::addColumn<float>("x2");
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("z2");
    QTest::addColumn<float>("w2");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 0.0f << 0.0f;

    QTest::newRow("unitvec")
        << 1.0f << 0.0f << 0.0f << 1.0f
        << 0.0f << 1.0f << 0.0f << 1.0f;

    QTest::newRow("complex")
        << 1.0f << 2.0f << 3.0f << 7.0f
        << 4.0f << 5.0f << 6.0f << 8.0f;

    for (float w = -1.0f; w <= 1.0f; w += 0.5f)
        for (float x = -1.0f; x <= 1.0f; x += 0.5f)
            for (float y = -1.0f; y <= 1.0f; y += 0.5f)
                for (float z = -1.0f; z <= 1.0f; z += 0.5f) {
                    QTest::newRow("exhaustive")
                        << x << y << z << w
                        << z << w << y << x;
                }
}
void tst_QQuaternion::multiply()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, w1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, w2);

    QQuaternion q1(w1, x1, y1, z1);
    QQuaternion q2(w2, x2, y2, z2);

    // Use the simple algorithm at:
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q53
    // to calculate the answer we expect to get.
    QVector3D v1(x1, y1, z1);
    QVector3D v2(x2, y2, z2);
    float scalar = w1 * w2 - QVector3D::dotProduct(v1, v2);
    QVector3D vector = w1 * v2 + w2 * v1 + QVector3D::crossProduct(v1, v2);
    QQuaternion result(scalar, vector);

    QVERIFY((q1 * q2) == result);
}

// Test multiplication by a factor for quaternions.
void tst_QQuaternion::multiplyFactor_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("w1");
    QTest::addColumn<float>("factor");
    QTest::addColumn<float>("x2");
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("z2");
    QTest::addColumn<float>("w2");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f << 0.0f
        << 100.0f
        << 0.0f << 0.0f << 0.0f << 0.0f;

    QTest::newRow("xonly")
        << 1.0f << 0.0f << 0.0f << 0.0f
        << 2.0f
        << 2.0f << 0.0f << 0.0f << 0.0f;

    QTest::newRow("yonly")
        << 0.0f << 1.0f << 0.0f << 0.0f
        << 2.0f
        << 0.0f << 2.0f << 0.0f << 0.0f;

    QTest::newRow("zonly")
        << 0.0f << 0.0f << 1.0f << 0.0f
        << 2.0f
        << 0.0f << 0.0f << 2.0f << 0.0f;

    QTest::newRow("wonly")
        << 0.0f << 0.0f << 0.0f << 1.0f
        << 2.0f
        << 0.0f << 0.0f << 0.0f << 2.0f;

    QTest::newRow("all")
        << 1.0f << 2.0f << -3.0f << 4.0f
        << 2.0f
        << 2.0f << 4.0f << -6.0f << 8.0f;

    QTest::newRow("allzero")
        << 1.0f << 2.0f << -3.0f << 4.0f
        << 0.0f
        << 0.0f << 0.0f << 0.0f << 0.0f;
}
void tst_QQuaternion::multiplyFactor()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, w1);
    QFETCH(float, factor);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, w2);

    QQuaternion v1(w1, x1, y1, z1);
    QQuaternion v2(w2, x2, y2, z2);

    QVERIFY((v1 * factor) == v2);
    QVERIFY((factor * v1) == v2);

    QQuaternion v3(v1);
    v3 *= factor;
    QCOMPARE(v3, v2);

    QCOMPARE(v3.x(), v1.x() * factor);
    QCOMPARE(v3.y(), v1.y() * factor);
    QCOMPARE(v3.z(), v1.z() * factor);
    QCOMPARE(v3.scalar(), v1.scalar() * factor);
}

// Test division by a factor for quaternions.
void tst_QQuaternion::divide_data()
{
    // Use the same test data as the multiply test.
    multiplyFactor_data();
}
void tst_QQuaternion::divide()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, w1);
    QFETCH(float, factor);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, w2);

    QQuaternion v1(w1, x1, y1, z1);
    QQuaternion v2(w2, x2, y2, z2);

    if (factor == 0.0f)
        return;

    QVERIFY((v2 / factor) == v1);

    QQuaternion v3(v2);
    v3 /= factor;
    QCOMPARE(v3, v1);

    QCOMPARE(v3.x(), v2.x() / factor);
    QCOMPARE(v3.y(), v2.y() / factor);
    QCOMPARE(v3.z(), v2.z() / factor);
    QCOMPARE(v3.scalar(), v2.scalar() / factor);
}

// Test negation for quaternions.
void tst_QQuaternion::negate_data()
{
    // Use the same test data as the add test.
    add_data();
}
void tst_QQuaternion::negate()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, w1);

    QQuaternion v1(w1, x1, y1, z1);
    QQuaternion v2(-w1, -x1, -y1, -z1);

    QCOMPARE(-v1, v2);
}

// Test quaternion conjugate calculations.
void tst_QQuaternion::conjugate_data()
{
    // Use the same test data as the add test.
    add_data();
}
void tst_QQuaternion::conjugate()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, w1);

    QQuaternion v1(w1, x1, y1, z1);
    QQuaternion v2(w1, -x1, -y1, -z1);

#if QT_DEPRECATED_SINCE(5, 5)
    QCOMPARE(v1.conjugate(), v2);
#endif
    QCOMPARE(v1.conjugated(), v2);
}

// Test quaternion creation from an axis and an angle.
void tst_QQuaternion::fromAxisAndAngle_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("angle");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f << 0.0f;

    QTest::newRow("xonly")
        << 1.0f << 0.0f << 0.0f << 90.0f;

    QTest::newRow("yonly")
        << 0.0f << 1.0f << 0.0f << 180.0f;

    QTest::newRow("zonly")
        << 0.0f << 0.0f << 1.0f << 270.0f;

    QTest::newRow("complex")
        << 1.0f << 2.0f << -3.0f << 45.0f;
}
void tst_QQuaternion::fromAxisAndAngle()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, angle);

    // Use a straight-forward implementation of the algorithm at:
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q56
    // to calculate the answer we expect to get.
    QVector3D vector = QVector3D(x1, y1, z1).normalized();
    const float a = qDegreesToRadians(angle) / 2.0;
    const float sin_a = std::sin(a);
    const float cos_a = std::cos(a);
    QQuaternion result(cos_a,
                       (vector.x() * sin_a),
                       (vector.y() * sin_a),
                       (vector.z() * sin_a));
    result = result.normalized();

    QQuaternion answer = QQuaternion::fromAxisAndAngle(QVector3D(x1, y1, z1), angle);
    QVERIFY(qFuzzyCompare(answer.x(), result.x()));
    QVERIFY(qFuzzyCompare(answer.y(), result.y()));
    QVERIFY(qFuzzyCompare(answer.z(), result.z()));
    QVERIFY(qFuzzyCompare(answer.scalar(), result.scalar()));

    {
        QVector3D answerAxis;
        float answerAngle;
        answer.getAxisAndAngle(&answerAxis, &answerAngle);
        QVERIFY(qFuzzyCompare(answerAxis.x(), vector.x()));
        QVERIFY(qFuzzyCompare(answerAxis.y(), vector.y()));
        QVERIFY(qFuzzyCompare(answerAxis.z(), vector.z()));
        QVERIFY(qFuzzyCompare(answerAngle, angle));
    }

    answer = QQuaternion::fromAxisAndAngle(x1, y1, z1, angle);
    QVERIFY(qFuzzyCompare(answer.x(), result.x()));
    QVERIFY(qFuzzyCompare(answer.y(), result.y()));
    QVERIFY(qFuzzyCompare(answer.z(), result.z()));
    QVERIFY(qFuzzyCompare(answer.scalar(), result.scalar()));

    {
        float answerAxisX, answerAxisY, answerAxisZ;
        float answerAngle;
        answer.getAxisAndAngle(&answerAxisX, &answerAxisY, &answerAxisZ, &answerAngle);
        QVERIFY(qFuzzyCompare(answerAxisX, vector.x()));
        QVERIFY(qFuzzyCompare(answerAxisY, vector.y()));
        QVERIFY(qFuzzyCompare(answerAxisZ, vector.z()));
        QVERIFY(qFuzzyCompare(answerAngle, angle));
    }
}

// Test quaternion convertion to and from rotation matrix.
void tst_QQuaternion::fromRotationMatrix_data()
{
    fromAxisAndAngle_data();
}
void tst_QQuaternion::fromRotationMatrix()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, angle);

    QQuaternion result = QQuaternion::fromAxisAndAngle(QVector3D(x1, y1, z1), angle);
    QMatrix3x3 rot3x3 = result.toRotationMatrix();
    QQuaternion answer = QQuaternion::fromRotationMatrix(rot3x3);

    QVERIFY(qFuzzyCompare(answer, result) || qFuzzyCompare(-answer, result));
}

// Test quaternion convertion to and from orthonormal axes.
void tst_QQuaternion::fromAxes_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("angle");
    QTest::addColumn<QVector3D>("xAxis");
    QTest::addColumn<QVector3D>("yAxis");
    QTest::addColumn<QVector3D>("zAxis");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f << 0.0f
        << QVector3D(1, 0, 0) << QVector3D(0, 1, 0) << QVector3D(0, 0, 1);

    QTest::newRow("xonly")
        << 1.0f << 0.0f << 0.0f << 90.0f
        << QVector3D(1, 0, 0) << QVector3D(0, 0, 1) << QVector3D(0, -1, 0);

    QTest::newRow("yonly")
        << 0.0f << 1.0f << 0.0f << 180.0f
        << QVector3D(-1, 0, 0) << QVector3D(0, 1, 0) << QVector3D(0, 0, -1);

    QTest::newRow("zonly")
        << 0.0f << 0.0f << 1.0f << 270.0f
        << QVector3D(0, -1, 0) << QVector3D(1, 0, 0) << QVector3D(0, 0, 1);

    QTest::newRow("complex")
        << 1.0f << 2.0f << -3.0f << 45.0f
        << QVector3D(0.728028, -0.525105, -0.440727) << QVector3D(0.608789, 0.790791, 0.0634566) << QVector3D(0.315202, -0.314508, 0.895395);
}
void tst_QQuaternion::fromAxes()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, angle);
    QFETCH(QVector3D, xAxis);
    QFETCH(QVector3D, yAxis);
    QFETCH(QVector3D, zAxis);

    QQuaternion result = QQuaternion::fromAxisAndAngle(QVector3D(x1, y1, z1), angle);

    QVector3D axes[3];
    result.getAxes(&axes[0], &axes[1], &axes[2]);
    QVERIFY(myFuzzyCompare(axes[0], xAxis));
    QVERIFY(myFuzzyCompare(axes[1], yAxis));
    QVERIFY(myFuzzyCompare(axes[2], zAxis));

    QQuaternion answer = QQuaternion::fromAxes(axes[0], axes[1], axes[2]);

    QVERIFY(qFuzzyCompare(answer, result) || qFuzzyCompare(-answer, result));
}

// Test shortest arc quaternion.
void tst_QQuaternion::rotationTo_data()
{
    QTest::addColumn<QVector3D>("from");
    QTest::addColumn<QVector3D>("to");

    // same
    QTest::newRow("+X -> +X") << QVector3D(10.0f, 0.0f, 0.0f) << QVector3D(10.0f, 0.0f, 0.0f);
    QTest::newRow("-X -> -X") << QVector3D(-10.0f, 0.0f, 0.0f) << QVector3D(-10.0f, 0.0f, 0.0f);
    QTest::newRow("+Y -> +Y") << QVector3D(0.0f, 10.0f, 0.0f) << QVector3D(0.0f, 10.0f, 0.0f);
    QTest::newRow("-Y -> -Y") << QVector3D(0.0f, -10.0f, 0.0f) << QVector3D(0.0f, -10.0f, 0.0f);
    QTest::newRow("+Z -> +Z") << QVector3D(0.0f, 0.0f, 10.0f) << QVector3D(0.0f, 0.0f, 10.0f);
    QTest::newRow("-Z -> -Z") << QVector3D(0.0f, 0.0f, -10.0f) << QVector3D(0.0f, 0.0f, -10.0f);
    QTest::newRow("+X+Y+Z -> +X+Y+Z") << QVector3D(10.0f, 10.0f, 10.0f) << QVector3D(10.0f, 10.0f, 10.0f);
    QTest::newRow("-X-Y-Z -> -X-Y-Z") << QVector3D(-10.0f, -10.0f, -10.0f) << QVector3D(-10.0f, -10.0f, -10.0f);

    // arbitrary
    QTest::newRow("+Z -> +X") << QVector3D(0.0f, 0.0f, 10.0f) << QVector3D(10.0f, 0.0f, 0.0f);
    QTest::newRow("+Z -> -X") << QVector3D(0.0f, 0.0f, 10.0f) << QVector3D(-10.0f, 0.0f, 0.0f);
    QTest::newRow("+Z -> +Y") << QVector3D(0.0f, 0.0f, 10.0f) << QVector3D(0.0f, 10.0f, 0.0f);
    QTest::newRow("+Z -> -Y") << QVector3D(0.0f, 0.0f, 10.0f) << QVector3D(0.0f, -10.0f, 0.0f);
    QTest::newRow("-Z -> +X") << QVector3D(0.0f, 0.0f, -10.0f) << QVector3D(10.0f, 0.0f, 0.0f);
    QTest::newRow("-Z -> -X") << QVector3D(0.0f, 0.0f, -10.0f) << QVector3D(-10.0f, 0.0f, 0.0f);
    QTest::newRow("-Z -> +Y") << QVector3D(0.0f, 0.0f, -10.0f) << QVector3D(0.0f, 10.0f, 0.0f);
    QTest::newRow("-Z -> -Y") << QVector3D(0.0f, 0.0f, -10.0f) << QVector3D(0.0f, -10.0f, 0.0f);
    QTest::newRow("+X -> +Y") << QVector3D(10.0f, 0.0f, 0.0f) << QVector3D(0.0f, 10.0f, 0.0f);
    QTest::newRow("+X -> -Y") << QVector3D(10.0f, 0.0f, 0.0f) << QVector3D(0.0f, -10.0f, 0.0f);
    QTest::newRow("-X -> +Y") << QVector3D(-10.0f, 0.0f, 0.0f) << QVector3D(0.0f, 10.0f, 0.0f);
    QTest::newRow("-X -> -Y") << QVector3D(-10.0f, 0.0f, 0.0f) << QVector3D(0.0f, -10.0f, 0.0f);
    QTest::newRow("+X+Y+Z -> +X-Y-Z") << QVector3D(10.0f, 10.0f, 10.0f) << QVector3D(10.0f, -10.0f, -10.0f);
    QTest::newRow("-X-Y+Z -> -X+Y-Z") << QVector3D(-10.0f, -10.0f, 10.0f) << QVector3D(-10.0f, 10.0f, -10.0f);
    QTest::newRow("+X+Y+Z -> +Z") << QVector3D(10.0f, 10.0f, 10.0f) << QVector3D(0.0f, 0.0f, 10.0f);

    // collinear
    QTest::newRow("+X -> -X") << QVector3D(10.0f, 0.0f, 0.0f) << QVector3D(-10.0f, 0.0f, 0.0f);
    QTest::newRow("+Y -> -Y") << QVector3D(0.0f, 10.0f, 0.0f) << QVector3D(0.0f, -10.0f, 0.0f);
    QTest::newRow("+Z -> -Z") << QVector3D(0.0f, 0.0f, 10.0f) << QVector3D(0.0f, 0.0f, -10.0f);
    QTest::newRow("+X+Y+Z -> -X-Y-Z") << QVector3D(10.0f, 10.0f, 10.0f) << QVector3D(-10.0f, -10.0f, -10.0f);
}
void tst_QQuaternion::rotationTo()
{
    QFETCH(QVector3D, from);
    QFETCH(QVector3D, to);

    QQuaternion q1 = QQuaternion::rotationTo(from, to);
    QVERIFY(myFuzzyCompare(q1, q1.normalized()));
    QVector3D vec1(q1 * from);
    vec1 *= (to.length() / from.length()); // discard rotated length
    QVERIFY(myFuzzyCompare(vec1, to));

    QQuaternion q2 = QQuaternion::rotationTo(to, from);
    QVERIFY(myFuzzyCompare(q2, q2.normalized()));
    QVector3D vec2(q2 * to);
    vec2 *= (from.length() / to.length()); // discard rotated length
    QVERIFY(myFuzzyCompare(vec2, from));
}

static QByteArray testnameForAxis(const QVector3D &axis)
{
    QByteArray testname;
    if (axis == QVector3D()) {
        testname = "null";
    } else {
        if (axis.x()) {
            testname += axis.x() < 0 ? '-' : '+';
            testname += 'X';
        }
        if (axis.y()) {
            testname += axis.y() < 0 ? '-' : '+';
            testname += 'Y';
        }
        if (axis.z()) {
            testname += axis.z() < 0 ? '-' : '+';
            testname += 'Z';
        }
    }
    return testname;
}

// Test quaternion convertion to and from orthonormal axes.
void tst_QQuaternion::fromDirection_data()
{
    QTest::addColumn<QVector3D>("direction");
    QTest::addColumn<QVector3D>("up");

    QList<QQuaternion> orientations;
    orientations << QQuaternion();
    for (int angle = 45; angle <= 360; angle += 45) {
        orientations << QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), angle)
                     << QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), angle)
                     << QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), angle)
                     << QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), angle)
                        * QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), angle)
                        * QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), angle);
    }

    // othonormal up and dir
    foreach (const QQuaternion &q, orientations) {
        QVector3D xAxis, yAxis, zAxis;
        q.getAxes(&xAxis, &yAxis, &zAxis);

        QTest::newRow("dir: " + testnameForAxis(zAxis) + ", up: " + testnameForAxis(yAxis))
            << zAxis * 10.0f << yAxis * 10.0f;
    }

    // collinear up and dir
    QTest::newRow("dir: +X, up: +X") << QVector3D(10.0f, 0.0f, 0.0f) << QVector3D(10.0f, 0.0f, 0.0f);
    QTest::newRow("dir: +X, up: -X") << QVector3D(10.0f, 0.0f, 0.0f) << QVector3D(-10.0f, 0.0f, 0.0f);
    QTest::newRow("dir: +Y, up: +Y") << QVector3D(0.0f, 10.0f, 0.0f) << QVector3D(0.0f, 10.0f, 0.0f);
    QTest::newRow("dir: +Y, up: -Y") << QVector3D(0.0f, 10.0f, 0.0f) << QVector3D(0.0f, -10.0f, 0.0f);
    QTest::newRow("dir: +Z, up: +Z") << QVector3D(0.0f, 0.0f, 10.0f) << QVector3D(0.0f, 0.0f, 10.0f);
    QTest::newRow("dir: +Z, up: -Z") << QVector3D(0.0f, 0.0f, 10.0f) << QVector3D(0.0f, 0.0f, -10.0f);
    QTest::newRow("dir: +X+Y+Z, up: +X+Y+Z") << QVector3D(10.0f, 10.0f, 10.0f) << QVector3D(10.0f, 10.0f, 10.0f);
    QTest::newRow("dir: +X+Y+Z, up: -X-Y-Z") << QVector3D(10.0f, 10.0f, 10.0f) << QVector3D(-10.0f, -10.0f, -10.0f);

    // invalid up
    foreach (const QQuaternion &q, orientations) {
        QVector3D xAxis, yAxis, zAxis;
        q.getAxes(&xAxis, &yAxis, &zAxis);

        QTest::newRow("dir: " + testnameForAxis(zAxis) + ", up: null")
            << zAxis * 10.0f << QVector3D();
    }
}
void tst_QQuaternion::fromDirection()
{
    QFETCH(QVector3D, direction);
    QFETCH(QVector3D, up);

    QVector3D expextedZ(direction != QVector3D() ? direction.normalized() : QVector3D(0, 0, 1));
    QVector3D expextedY(up.normalized());

    QQuaternion result = QQuaternion::fromDirection(direction, up);
    QVERIFY(myFuzzyCompare(result, result.normalized()));

    QVector3D xAxis, yAxis, zAxis;
    result.getAxes(&xAxis, &yAxis, &zAxis);

    QVERIFY(myFuzzyCompare(zAxis, expextedZ));

    if (!qFuzzyIsNull(QVector3D::crossProduct(expextedZ, expextedY).lengthSquared())) {
        QVector3D expextedX(QVector3D::crossProduct(expextedY, expextedZ));

        QVERIFY(myFuzzyCompare(yAxis, expextedY));
        QVERIFY(myFuzzyCompare(xAxis, expextedX));
    }
}

// Test quaternion creation from an axis and an angle.
void tst_QQuaternion::fromEulerAngles_data()
{
    QTest::addColumn<float>("pitch");
    QTest::addColumn<float>("yaw");
    QTest::addColumn<float>("roll");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f;

    QTest::newRow("xonly")
        << 90.0f << 0.0f << 0.0f;

    QTest::newRow("yonly")
        << 0.0f << 180.0f << 0.0f;

    QTest::newRow("zonly")
        << 0.0f << 0.0f << 270.0f;

    QTest::newRow("x+z")
        << 30.0f << 0.0f << 45.0f;

    QTest::newRow("x+y")
        << 30.0f << 90.0f << 0.0f;

    QTest::newRow("y+z")
        << 0.0f << 45.0f << 30.0f;

    QTest::newRow("complex")
        << 30.0f << 240.0f << -45.0f;
}
void tst_QQuaternion::fromEulerAngles()
{
    QFETCH(float, pitch);
    QFETCH(float, yaw);
    QFETCH(float, roll);

    // Use a straight-forward implementation of the algorithm at:
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q60
    // to calculate the answer we expect to get.
    QQuaternion qx = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), pitch);
    QQuaternion qy = QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), yaw);
    QQuaternion qz = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), roll);
    QQuaternion result = qy * (qx * qz);
    QQuaternion answer = QQuaternion::fromEulerAngles(QVector3D(pitch, yaw, roll));

    QVERIFY(myFuzzyCompare(answer.x(), result.x()));
    QVERIFY(myFuzzyCompare(answer.y(), result.y()));
    QVERIFY(myFuzzyCompare(answer.z(), result.z()));
    QVERIFY(myFuzzyCompare(answer.scalar(), result.scalar()));

    {
        QVector3D answerEulerAngles = answer.toEulerAngles();
        QVERIFY(myFuzzyCompareDegrees(answerEulerAngles.x(), pitch));
        QVERIFY(myFuzzyCompareDegrees(answerEulerAngles.y(), yaw));
        QVERIFY(myFuzzyCompareDegrees(answerEulerAngles.z(), roll));
    }

    answer = QQuaternion::fromEulerAngles(pitch, yaw, roll);
    QVERIFY(myFuzzyCompare(answer.x(), result.x()));
    QVERIFY(myFuzzyCompare(answer.y(), result.y()));
    QVERIFY(myFuzzyCompare(answer.z(), result.z()));
    QVERIFY(myFuzzyCompare(answer.scalar(), result.scalar()));

    {
        float answerPitch, answerYaw, answerRoll;
        answer.getEulerAngles(&answerPitch, &answerYaw, &answerRoll);
        QVERIFY(myFuzzyCompareDegrees(answerPitch, pitch));
        QVERIFY(myFuzzyCompareDegrees(answerYaw, yaw));
        QVERIFY(myFuzzyCompareDegrees(answerRoll, roll));
    }
}

// Test spherical interpolation of quaternions.
void tst_QQuaternion::slerp_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("angle1");
    QTest::addColumn<float>("x2");
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("z2");
    QTest::addColumn<float>("angle2");
    QTest::addColumn<float>("t");
    QTest::addColumn<float>("x3");
    QTest::addColumn<float>("y3");
    QTest::addColumn<float>("z3");
    QTest::addColumn<float>("angle3");

    QTest::newRow("first")
        << 1.0f << 2.0f << -3.0f << 90.0f
        << 1.0f << 2.0f << -3.0f << 180.0f
        << 0.0f
        << 1.0f << 2.0f << -3.0f << 90.0f;
    QTest::newRow("first2")
        << 1.0f << 2.0f << -3.0f << 90.0f
        << 1.0f << 2.0f << -3.0f << 180.0f
        << -0.5f
        << 1.0f << 2.0f << -3.0f << 90.0f;
    QTest::newRow("second")
        << 1.0f << 2.0f << -3.0f << 90.0f
        << 1.0f << 2.0f << -3.0f << 180.0f
        << 1.0f
        << 1.0f << 2.0f << -3.0f << 180.0f;
    QTest::newRow("second2")
        << 1.0f << 2.0f << -3.0f << 90.0f
        << 1.0f << 2.0f << -3.0f << 180.0f
        << 1.5f
        << 1.0f << 2.0f << -3.0f << 180.0f;
    QTest::newRow("middle")
        << 1.0f << 2.0f << -3.0f << 90.0f
        << 1.0f << 2.0f << -3.0f << 180.0f
        << 0.5f
        << 1.0f << 2.0f << -3.0f << 135.0f;
    QTest::newRow("wide angle")
        << 1.0f << 2.0f << -3.0f << 0.0f
        << 1.0f << 2.0f << -3.0f << 270.0f
        << 0.5f
        << 1.0f << 2.0f << -3.0f << -45.0f;
}
void tst_QQuaternion::slerp()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, angle1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, angle2);
    QFETCH(float, t);
    QFETCH(float, x3);
    QFETCH(float, y3);
    QFETCH(float, z3);
    QFETCH(float, angle3);

    QQuaternion q1 = QQuaternion::fromAxisAndAngle(x1, y1, z1, angle1);
    QQuaternion q2 = QQuaternion::fromAxisAndAngle(x2, y2, z2, angle2);
    QQuaternion q3 = QQuaternion::fromAxisAndAngle(x3, y3, z3, angle3);

    QQuaternion result = QQuaternion::slerp(q1, q2, t);

    QVERIFY(qFuzzyCompare(result.x(), q3.x()));
    QVERIFY(qFuzzyCompare(result.y(), q3.y()));
    QVERIFY(qFuzzyCompare(result.z(), q3.z()));
    QVERIFY(qFuzzyCompare(result.scalar(), q3.scalar()));
}

// Test normalized linear interpolation of quaternions.
void tst_QQuaternion::nlerp_data()
{
    slerp_data();
}
void tst_QQuaternion::nlerp()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, angle1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, angle2);
    QFETCH(float, t);

    QQuaternion q1 = QQuaternion::fromAxisAndAngle(x1, y1, z1, angle1);
    QQuaternion q2 = QQuaternion::fromAxisAndAngle(x2, y2, z2, angle2);

    QQuaternion result = QQuaternion::nlerp(q1, q2, t);

    float resultx, resulty, resultz, resultscalar;
    if (t <= 0.0f) {
        resultx = q1.x();
        resulty = q1.y();
        resultz = q1.z();
        resultscalar = q1.scalar();
    } else if (t >= 1.0f) {
        resultx = q2.x();
        resulty = q2.y();
        resultz = q2.z();
        resultscalar = q2.scalar();
    } else if (qAbs(angle1 - angle2) <= 180.f) {
        resultx = q1.x() * (1 - t) + q2.x() * t;
        resulty = q1.y() * (1 - t) + q2.y() * t;
        resultz = q1.z() * (1 - t) + q2.z() * t;
        resultscalar = q1.scalar() * (1 - t) + q2.scalar() * t;
    } else {
        // Angle greater than 180 degrees: negate q2.
        resultx = q1.x() * (1 - t) - q2.x() * t;
        resulty = q1.y() * (1 - t) - q2.y() * t;
        resultz = q1.z() * (1 - t) - q2.z() * t;
        resultscalar = q1.scalar() * (1 - t) - q2.scalar() * t;
    }

    QQuaternion q3 = QQuaternion(resultscalar, resultx, resulty, resultz).normalized();

    QVERIFY(qFuzzyCompare(result.x(), q3.x()));
    QVERIFY(qFuzzyCompare(result.y(), q3.y()));
    QVERIFY(qFuzzyCompare(result.z(), q3.z()));
    QVERIFY(qFuzzyCompare(result.scalar(), q3.scalar()));
}

class tst_QQuaternionProperties : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuaternion quaternion READ quaternion WRITE setQuaternion)
public:
    tst_QQuaternionProperties(QObject *parent = 0) : QObject(parent) {}

    QQuaternion quaternion() const { return q; }
    void setQuaternion(const QQuaternion& value) { q = value; }

private:
    QQuaternion q;
};

// Test getting and setting quaternion properties via the metaobject system.
void tst_QQuaternion::properties()
{
    tst_QQuaternionProperties obj;

    obj.setQuaternion(QQuaternion(6.0f, 7.0f, 8.0f, 9.0f));

    QQuaternion q = qvariant_cast<QQuaternion>(obj.property("quaternion"));
    QCOMPARE(q.scalar(), 6.0f);
    QCOMPARE(q.x(), 7.0f);
    QCOMPARE(q.y(), 8.0f);
    QCOMPARE(q.z(), 9.0f);

    obj.setProperty("quaternion",
                    QVariant::fromValue(QQuaternion(-6.0f, -7.0f, -8.0f, -9.0f)));

    q = qvariant_cast<QQuaternion>(obj.property("quaternion"));
    QCOMPARE(q.scalar(), -6.0f);
    QCOMPARE(q.x(), -7.0f);
    QCOMPARE(q.y(), -8.0f);
    QCOMPARE(q.z(), -9.0f);
}

void tst_QQuaternion::metaTypes()
{
    QCOMPARE(QMetaType::type("QQuaternion"), int(QMetaType::QQuaternion));

    QCOMPARE(QByteArray(QMetaType::typeName(QMetaType::QQuaternion)),
             QByteArray("QQuaternion"));

    QVERIFY(QMetaType::isRegistered(QMetaType::QQuaternion));

    QCOMPARE(qMetaTypeId<QQuaternion>(), int(QMetaType::QQuaternion));
}

QTEST_APPLESS_MAIN(tst_QQuaternion)

#include "tst_qquaternion.moc"
