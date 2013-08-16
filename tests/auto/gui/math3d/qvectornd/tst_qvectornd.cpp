/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/qmath.h>
#include <QtGui/qvector2d.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qvector4d.h>

class tst_QVectorND : public QObject
{
    Q_OBJECT
public:
    tst_QVectorND() {}
    ~tst_QVectorND() {}

private slots:
    void create2();
    void create3();
    void create4();

    void modify2();
    void modify3();
    void modify4();

    void length2_data();
    void length2();
    void length3_data();
    void length3();
    void length4_data();
    void length4();

    void normalized2_data();
    void normalized2();
    void normalized3_data();
    void normalized3();
    void normalized4_data();
    void normalized4();

    void normalize2_data();
    void normalize2();
    void normalize3_data();
    void normalize3();
    void normalize4_data();
    void normalize4();

    void compare2();
    void compare3();
    void compare4();

    void add2_data();
    void add2();
    void add3_data();
    void add3();
    void add4_data();
    void add4();

    void subtract2_data();
    void subtract2();
    void subtract3_data();
    void subtract3();
    void subtract4_data();
    void subtract4();

    void multiply2_data();
    void multiply2();
    void multiply3_data();
    void multiply3();
    void multiply4_data();
    void multiply4();

    void multiplyFactor2_data();
    void multiplyFactor2();
    void multiplyFactor3_data();
    void multiplyFactor3();
    void multiplyFactor4_data();
    void multiplyFactor4();

    void divide2_data();
    void divide2();
    void divide3_data();
    void divide3();
    void divide4_data();
    void divide4();

    void negate2_data();
    void negate2();
    void negate3_data();
    void negate3();
    void negate4_data();
    void negate4();

    void crossProduct_data();
    void crossProduct();
    void normal_data();
    void normal();
    void distanceToPoint2_data();
    void distanceToPoint2();
    void distanceToPoint3_data();
    void distanceToPoint3();
    void distanceToPlane_data();
    void distanceToPlane();
    void distanceToLine2_data();
    void distanceToLine2();
    void distanceToLine3_data();
    void distanceToLine3();

    void dotProduct2_data();
    void dotProduct2();
    void dotProduct3_data();
    void dotProduct3();
    void dotProduct4_data();
    void dotProduct4();

    void properties();
    void metaTypes();
};

// Test the creation of QVector2D objects in various ways:
// construct, copy, and modify.
void tst_QVectorND::create2()
{
    QVector2D null;
    QCOMPARE(null.x(), 0.0f);
    QCOMPARE(null.y(), 0.0f);
    QVERIFY(null.isNull());

    QVector2D nullNegativeZero(-0.0f, -0.0f);
    QCOMPARE(nullNegativeZero.x(), -0.0f);
    QCOMPARE(nullNegativeZero.y(), -0.0f);
    QVERIFY(nullNegativeZero.isNull());

    QVector2D v1(1.0f, 2.5f);
    QCOMPARE(v1.x(), 1.0f);
    QCOMPARE(v1.y(), 2.5f);
    QVERIFY(!v1.isNull());

    QVector2D v1i(1, 2);
    QCOMPARE(v1i.x(), 1.0f);
    QCOMPARE(v1i.y(), 2.0f);
    QVERIFY(!v1i.isNull());

    QVector2D v2(v1);
    QCOMPARE(v2.x(), 1.0f);
    QCOMPARE(v2.y(), 2.5f);
    QVERIFY(!v2.isNull());

    QVector2D v4;
    QCOMPARE(v4.x(), 0.0f);
    QCOMPARE(v4.y(), 0.0f);
    QVERIFY(v4.isNull());
    v4 = v1;
    QCOMPARE(v4.x(), 1.0f);
    QCOMPARE(v4.y(), 2.5f);
    QVERIFY(!v4.isNull());

    QVector2D v5(QPoint(1, 2));
    QCOMPARE(v5.x(), 1.0f);
    QCOMPARE(v5.y(), 2.0f);
    QVERIFY(!v5.isNull());

    QVector2D v6(QPointF(1, 2.5));
    QCOMPARE(v6.x(), 1.0f);
    QCOMPARE(v6.y(), 2.5f);
    QVERIFY(!v6.isNull());

    QVector2D v7(QVector3D(1.0f, 2.5f, 54.25f));
    QCOMPARE(v7.x(), 1.0f);
    QCOMPARE(v7.y(), 2.5f);
    QVERIFY(!v6.isNull());

    QVector2D v8(QVector4D(1.0f, 2.5f, 54.25f, 34.0f));
    QCOMPARE(v8.x(), 1.0f);
    QCOMPARE(v8.y(), 2.5f);
    QVERIFY(!v6.isNull());

    v1.setX(3.0f);
    QCOMPARE(v1.x(), 3.0f);
    QCOMPARE(v1.y(), 2.5f);
    QVERIFY(!v1.isNull());

    v1.setY(10.5f);
    QCOMPARE(v1.x(), 3.0f);
    QCOMPARE(v1.y(), 10.5f);
    QVERIFY(!v1.isNull());

    v1.setX(0.0f);
    v1.setY(0.0f);
    QCOMPARE(v1.x(), 0.0f);
    QCOMPARE(v1.y(), 0.0f);
    QVERIFY(v1.isNull());

    QPoint p1 = v8.toPoint();
    QCOMPARE(p1.x(), 1);
    QCOMPARE(p1.y(), 3);

    QPointF p2 = v8.toPointF();
    QCOMPARE(p2.x(), 1.0f);
    QCOMPARE(p2.y(), 2.5f);

    QVector3D v9 = v8.toVector3D();
    QCOMPARE(v9.x(), 1.0f);
    QCOMPARE(v9.y(), 2.5f);
    QCOMPARE(v9.z(), 0.0f);

    QVector4D v10 = v8.toVector4D();
    QCOMPARE(v10.x(), 1.0f);
    QCOMPARE(v10.y(), 2.5f);
    QCOMPARE(v10.z(), 0.0f);
    QCOMPARE(v10.w(), 0.0f);
}

// Test the creation of QVector3D objects in various ways:
// construct, copy, and modify.
void tst_QVectorND::create3()
{
    QVector3D null;
    QCOMPARE(null.x(), 0.0f);
    QCOMPARE(null.y(), 0.0f);
    QCOMPARE(null.z(), 0.0f);
    QVERIFY(null.isNull());

    QVector3D nullNegativeZero(-0.0f, -0.0f, -0.0f);
    QCOMPARE(nullNegativeZero.x(), -0.0f);
    QCOMPARE(nullNegativeZero.y(), -0.0f);
    QCOMPARE(nullNegativeZero.z(), -0.0f);
    QVERIFY(nullNegativeZero.isNull());

    QVector3D v1(1.0f, 2.5f, -89.25f);
    QCOMPARE(v1.x(), 1.0f);
    QCOMPARE(v1.y(), 2.5f);
    QCOMPARE(v1.z(), -89.25f);
    QVERIFY(!v1.isNull());

    QVector3D v1i(1, 2, -89);
    QCOMPARE(v1i.x(), 1.0f);
    QCOMPARE(v1i.y(), 2.0f);
    QCOMPARE(v1i.z(), -89.0f);
    QVERIFY(!v1i.isNull());

    QVector3D v2(v1);
    QCOMPARE(v2.x(), 1.0f);
    QCOMPARE(v2.y(), 2.5f);
    QCOMPARE(v2.z(), -89.25f);
    QVERIFY(!v2.isNull());

    QVector3D v3(1.0f, 2.5f, 0.0f);
    QCOMPARE(v3.x(), 1.0f);
    QCOMPARE(v3.y(), 2.5f);
    QCOMPARE(v3.z(), 0.0f);
    QVERIFY(!v3.isNull());

    QVector3D v3i(1, 2, 0);
    QCOMPARE(v3i.x(), 1.0f);
    QCOMPARE(v3i.y(), 2.0f);
    QCOMPARE(v3i.z(), 0.0f);
    QVERIFY(!v3i.isNull());

    QVector3D v4;
    QCOMPARE(v4.x(), 0.0f);
    QCOMPARE(v4.y(), 0.0f);
    QCOMPARE(v4.z(), 0.0f);
    QVERIFY(v4.isNull());
    v4 = v1;
    QCOMPARE(v4.x(), 1.0f);
    QCOMPARE(v4.y(), 2.5f);
    QCOMPARE(v4.z(), -89.25f);
    QVERIFY(!v4.isNull());

    QVector3D v5(QPoint(1, 2));
    QCOMPARE(v5.x(), 1.0f);
    QCOMPARE(v5.y(), 2.0f);
    QCOMPARE(v5.z(), 0.0f);
    QVERIFY(!v5.isNull());

    QVector3D v6(QPointF(1, 2.5));
    QCOMPARE(v6.x(), 1.0f);
    QCOMPARE(v6.y(), 2.5f);
    QCOMPARE(v6.z(), 0.0f);
    QVERIFY(!v6.isNull());

    QVector3D v7(QVector2D(1.0f, 2.5f));
    QCOMPARE(v7.x(), 1.0f);
    QCOMPARE(v7.y(), 2.5f);
    QCOMPARE(v7.z(), 0.0f);
    QVERIFY(!v7.isNull());

    QVector3D v8(QVector2D(1.0f, 2.5f), 54.25f);
    QCOMPARE(v8.x(), 1.0f);
    QCOMPARE(v8.y(), 2.5f);
    QCOMPARE(v8.z(), 54.25f);
    QVERIFY(!v8.isNull());

    QVector3D v9(QVector4D(1.0f, 2.5f, 54.25f, 34.0f));
    QCOMPARE(v9.x(), 1.0f);
    QCOMPARE(v9.y(), 2.5f);
    QCOMPARE(v9.z(), 54.25f);
    QVERIFY(!v9.isNull());

    v1.setX(3.0f);
    QCOMPARE(v1.x(), 3.0f);
    QCOMPARE(v1.y(), 2.5f);
    QCOMPARE(v1.z(), -89.25f);
    QVERIFY(!v1.isNull());

    v1.setY(10.5f);
    QCOMPARE(v1.x(), 3.0f);
    QCOMPARE(v1.y(), 10.5f);
    QCOMPARE(v1.z(), -89.25f);
    QVERIFY(!v1.isNull());

    v1.setZ(15.5f);
    QCOMPARE(v1.x(), 3.0f);
    QCOMPARE(v1.y(), 10.5f);
    QCOMPARE(v1.z(), 15.5f);
    QVERIFY(!v1.isNull());

    v1.setX(0.0f);
    v1.setY(0.0f);
    v1.setZ(0.0f);
    QCOMPARE(v1.x(), 0.0f);
    QCOMPARE(v1.y(), 0.0f);
    QCOMPARE(v1.z(), 0.0f);
    QVERIFY(v1.isNull());

    QPoint p1 = v8.toPoint();
    QCOMPARE(p1.x(), 1);
    QCOMPARE(p1.y(), 3);

    QPointF p2 = v8.toPointF();
    QCOMPARE(p2.x(), 1.0f);
    QCOMPARE(p2.y(), 2.5f);

    QVector2D v10 = v8.toVector2D();
    QCOMPARE(v10.x(), 1.0f);
    QCOMPARE(v10.y(), 2.5f);

    QVector4D v11 = v8.toVector4D();
    QCOMPARE(v11.x(), 1.0f);
    QCOMPARE(v11.y(), 2.5f);
    QCOMPARE(v11.z(), 54.25f);
    QCOMPARE(v11.w(), 0.0f);
}

// Test the creation of QVector4D objects in various ways:
// construct, copy, and modify.
void tst_QVectorND::create4()
{
    QVector4D null;
    QCOMPARE(null.x(), 0.0f);
    QCOMPARE(null.y(), 0.0f);
    QCOMPARE(null.z(), 0.0f);
    QCOMPARE(null.w(), 0.0f);
    QVERIFY(null.isNull());

    QVector4D nullNegativeZero(-0.0f, -0.0f, -0.0f, -0.0f);
    QCOMPARE(nullNegativeZero.x(), -0.0f);
    QCOMPARE(nullNegativeZero.y(), -0.0f);
    QCOMPARE(nullNegativeZero.z(), -0.0f);
    QCOMPARE(nullNegativeZero.w(), -0.0f);
    QVERIFY(nullNegativeZero.isNull());

    QVector4D v1(1.0f, 2.5f, -89.25f, 34.0f);
    QCOMPARE(v1.x(), 1.0f);
    QCOMPARE(v1.y(), 2.5f);
    QCOMPARE(v1.z(), -89.25f);
    QCOMPARE(v1.w(), 34.0f);
    QVERIFY(!v1.isNull());

    QVector4D v1i(1, 2, -89, 34);
    QCOMPARE(v1i.x(), 1.0f);
    QCOMPARE(v1i.y(), 2.0f);
    QCOMPARE(v1i.z(), -89.0f);
    QCOMPARE(v1i.w(), 34.0f);
    QVERIFY(!v1i.isNull());

    QVector4D v2(v1);
    QCOMPARE(v2.x(), 1.0f);
    QCOMPARE(v2.y(), 2.5f);
    QCOMPARE(v2.z(), -89.25f);
    QCOMPARE(v2.w(), 34.0f);
    QVERIFY(!v2.isNull());

    QVector4D v3(1.0f, 2.5f, 0.0f, 0.0f);
    QCOMPARE(v3.x(), 1.0f);
    QCOMPARE(v3.y(), 2.5f);
    QCOMPARE(v3.z(), 0.0f);
    QCOMPARE(v3.w(), 0.0f);
    QVERIFY(!v3.isNull());

    QVector4D v3i(1, 2, 0, 0);
    QCOMPARE(v3i.x(), 1.0f);
    QCOMPARE(v3i.y(), 2.0f);
    QCOMPARE(v3i.z(), 0.0f);
    QCOMPARE(v3i.w(), 0.0f);
    QVERIFY(!v3i.isNull());

    QVector4D v3b(1.0f, 2.5f, -89.25f, 0.0f);
    QCOMPARE(v3b.x(), 1.0f);
    QCOMPARE(v3b.y(), 2.5f);
    QCOMPARE(v3b.z(), -89.25f);
    QCOMPARE(v3b.w(), 0.0f);
    QVERIFY(!v3b.isNull());

    QVector4D v3bi(1, 2, -89, 0);
    QCOMPARE(v3bi.x(), 1.0f);
    QCOMPARE(v3bi.y(), 2.0f);
    QCOMPARE(v3bi.z(), -89.0f);
    QCOMPARE(v3bi.w(), 0.0f);
    QVERIFY(!v3bi.isNull());

    QVector4D v4;
    QCOMPARE(v4.x(), 0.0f);
    QCOMPARE(v4.y(), 0.0f);
    QCOMPARE(v4.z(), 0.0f);
    QCOMPARE(v4.w(), 0.0f);
    QVERIFY(v4.isNull());
    v4 = v1;
    QCOMPARE(v4.x(), 1.0f);
    QCOMPARE(v4.y(), 2.5f);
    QCOMPARE(v4.z(), -89.25f);
    QCOMPARE(v4.w(), 34.0f);
    QVERIFY(!v4.isNull());

    QVector4D v5(QPoint(1, 2));
    QCOMPARE(v5.x(), 1.0f);
    QCOMPARE(v5.y(), 2.0f);
    QCOMPARE(v5.z(), 0.0f);
    QCOMPARE(v5.w(), 0.0f);
    QVERIFY(!v5.isNull());

    QVector4D v6(QPointF(1, 2.5));
    QCOMPARE(v6.x(), 1.0f);
    QCOMPARE(v6.y(), 2.5f);
    QCOMPARE(v6.z(), 0.0f);
    QCOMPARE(v6.w(), 0.0f);
    QVERIFY(!v6.isNull());

    QVector4D v7(QVector2D(1.0f, 2.5f));
    QCOMPARE(v7.x(), 1.0f);
    QCOMPARE(v7.y(), 2.5f);
    QCOMPARE(v7.z(), 0.0f);
    QCOMPARE(v7.w(), 0.0f);
    QVERIFY(!v7.isNull());

    QVector4D v8(QVector3D(1.0f, 2.5f, -89.25f));
    QCOMPARE(v8.x(), 1.0f);
    QCOMPARE(v8.y(), 2.5f);
    QCOMPARE(v8.z(), -89.25f);
    QCOMPARE(v8.w(), 0.0f);
    QVERIFY(!v8.isNull());

    QVector4D v9(QVector3D(1.0f, 2.5f, -89.25f), 34);
    QCOMPARE(v9.x(), 1.0f);
    QCOMPARE(v9.y(), 2.5f);
    QCOMPARE(v9.z(), -89.25f);
    QCOMPARE(v9.w(), 34.0f);
    QVERIFY(!v9.isNull());

    QVector4D v10(QVector2D(1.0f, 2.5f), 23.5f, -8);
    QCOMPARE(v10.x(), 1.0f);
    QCOMPARE(v10.y(), 2.5f);
    QCOMPARE(v10.z(), 23.5f);
    QCOMPARE(v10.w(), -8.0f);
    QVERIFY(!v10.isNull());

    v1.setX(3.0f);
    QCOMPARE(v1.x(), 3.0f);
    QCOMPARE(v1.y(), 2.5f);
    QCOMPARE(v1.z(), -89.25f);
    QCOMPARE(v1.w(), 34.0f);
    QVERIFY(!v1.isNull());

    v1.setY(10.5f);
    QCOMPARE(v1.x(), 3.0f);
    QCOMPARE(v1.y(), 10.5f);
    QCOMPARE(v1.z(), -89.25f);
    QCOMPARE(v1.w(), 34.0f);
    QVERIFY(!v1.isNull());

    v1.setZ(15.5f);
    QCOMPARE(v1.x(), 3.0f);
    QCOMPARE(v1.y(), 10.5f);
    QCOMPARE(v1.z(), 15.5f);
    QCOMPARE(v1.w(), 34.0f);
    QVERIFY(!v1.isNull());

    v1.setW(6.0f);
    QCOMPARE(v1.x(), 3.0f);
    QCOMPARE(v1.y(), 10.5f);
    QCOMPARE(v1.z(), 15.5f);
    QCOMPARE(v1.w(), 6.0f);
    QVERIFY(!v1.isNull());

    v1.setX(0.0f);
    v1.setY(0.0f);
    v1.setZ(0.0f);
    v1.setW(0.0f);
    QCOMPARE(v1.x(), 0.0f);
    QCOMPARE(v1.y(), 0.0f);
    QCOMPARE(v1.z(), 0.0f);
    QCOMPARE(v1.w(), 0.0f);
    QVERIFY(v1.isNull());

    QPoint p1 = v8.toPoint();
    QCOMPARE(p1.x(), 1);
    QCOMPARE(p1.y(), 3);

    QPointF p2 = v8.toPointF();
    QCOMPARE(p2.x(), 1.0f);
    QCOMPARE(p2.y(), 2.5f);

    QVector2D v11 = v8.toVector2D();
    QCOMPARE(v11.x(), 1.0f);
    QCOMPARE(v11.y(), 2.5f);

    QVector3D v12 = v8.toVector3D();
    QCOMPARE(v12.x(), 1.0f);
    QCOMPARE(v12.y(), 2.5f);
    QCOMPARE(v12.z(), -89.25f);

    QVector2D v13 = v9.toVector2DAffine();
    QVERIFY(qFuzzyCompare(v13.x(), (1.0f / 34.0f)));
    QVERIFY(qFuzzyCompare(v13.y(), (2.5f / 34.0f)));

    QVector4D zerow(1.0f, 2.0f, 3.0f, 0.0f);
    v13 = zerow.toVector2DAffine();
    QVERIFY(v13.isNull());

    QVector3D v14 = v9.toVector3DAffine();
    QVERIFY(qFuzzyCompare(v14.x(), (1.0f / 34.0f)));
    QVERIFY(qFuzzyCompare(v14.y(), (2.5f / 34.0f)));
    QVERIFY(qFuzzyCompare(v14.z(), (-89.25f / 34.0f)));

    v14 = zerow.toVector3DAffine();
    QVERIFY(v14.isNull());
}

// Test modifying vectors in various ways
void tst_QVectorND::modify2()
{
    const float e = 2.7182818f;
    const float pi = 3.14159f;
    const QVector2D p(e, pi);

    QVector2D p1;
    p1.setX(e);
    p1.setY(pi);
    QVERIFY(qFuzzyCompare(p, p1));

    QVector2D p2;
    p2[0] = e;
    p2[1] = pi;
    QVERIFY(qFuzzyCompare(p, p2));

    QVector2D p3;
    for (int i = 0; i < 2; ++i)
        p3[i] = p[i];
    QVERIFY(qFuzzyCompare(p, p3));
}

void tst_QVectorND::modify3()
{
    const float one = 1.0f;
    const float e = 2.7182818f;
    const float pi = 3.14159f;
    const QVector3D p(one, e, pi);

    QVector3D p1;
    p1.setX(one);
    p1.setY(e);
    p1.setZ(pi);
    QVERIFY(qFuzzyCompare(p, p1));

    QVector3D p2;
    p2[0] = one;
    p2[1] = e;
    p2[2] = pi;
    QVERIFY(qFuzzyCompare(p, p2));

    QVector3D p3;
    for (int i = 0; i < 3; ++i)
        p3[i] = p[i];
    QVERIFY(qFuzzyCompare(p, p3));
}

void tst_QVectorND::modify4()
{
    const float one = 1.0f;
    const float e = 2.7182818f;
    const float pi = 3.14159f;
    const float big = 1.0e6f;
    const QVector4D p(one, e, pi, big);

    QVector4D p1;
    p1.setX(one);
    p1.setY(e);
    p1.setZ(pi);
    p1.setW(big);
    QVERIFY(qFuzzyCompare(p, p1));

    QVector4D p2;
    p2[0] = one;
    p2[1] = e;
    p2[2] = pi;
    p2[3] = big;
    QVERIFY(qFuzzyCompare(p, p2));

    QVector4D p3;
    for (int i = 0; i < 4; ++i)
        p3[i] = p[i];
    QVERIFY(qFuzzyCompare(p, p3));
}

// Test vector length computation for 2D vectors.
void tst_QVectorND::length2_data()
{
    QTest::addColumn<float>("x");
    QTest::addColumn<float>("y");
    QTest::addColumn<float>("len");

    QTest::newRow("null") <<  0.0f <<  0.0f << 0.0f;
    QTest::newRow("1x")   <<  1.0f <<  0.0f << 1.0f;
    QTest::newRow("1y")   <<  0.0f <<  1.0f << 1.0f;
    QTest::newRow("-1x")  << -1.0f <<  0.0f << 1.0f;
    QTest::newRow("-1y")  <<  0.0f << -1.0f << 1.0f;
    QTest::newRow("two")  <<  2.0f << -2.0f << sqrtf(8.0f);
}
void tst_QVectorND::length2()
{
    QFETCH(float, x);
    QFETCH(float, y);
    QFETCH(float, len);

    QVector2D v(x, y);
    QCOMPARE(v.length(), len);
    QCOMPARE(v.lengthSquared(), x * x + y * y);
}

// Test vector length computation for 3D vectors.
void tst_QVectorND::length3_data()
{
    QTest::addColumn<float>("x");
    QTest::addColumn<float>("y");
    QTest::addColumn<float>("z");
    QTest::addColumn<float>("len");

    QTest::newRow("null") << 0.0f << 0.0f << 0.0f << 0.0f;
    QTest::newRow("1x") << 1.0f << 0.0f << 0.0f << 1.0f;
    QTest::newRow("1y") << 0.0f << 1.0f << 0.0f << 1.0f;
    QTest::newRow("1z") << 0.0f << 0.0f << 1.0f << 1.0f;
    QTest::newRow("-1x") << -1.0f << 0.0f << 0.0f << 1.0f;
    QTest::newRow("-1y") << 0.0f << -1.0f << 0.0f << 1.0f;
    QTest::newRow("-1z") << 0.0f << 0.0f << -1.0f << 1.0f;
    QTest::newRow("two") << 2.0f << -2.0f << 2.0f << sqrtf(12.0f);
}
void tst_QVectorND::length3()
{
    QFETCH(float, x);
    QFETCH(float, y);
    QFETCH(float, z);
    QFETCH(float, len);

    QVector3D v(x, y, z);
    QCOMPARE(v.length(), len);
    QCOMPARE(v.lengthSquared(), x * x + y * y + z * z);
}

// Test vector length computation for 4D vectors.
void tst_QVectorND::length4_data()
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
    QTest::newRow("two") << 2.0f << -2.0f << 2.0f << 2.0f << sqrtf(16.0f);
}
void tst_QVectorND::length4()
{
    QFETCH(float, x);
    QFETCH(float, y);
    QFETCH(float, z);
    QFETCH(float, w);
    QFETCH(float, len);

    QVector4D v(x, y, z, w);
    QCOMPARE(v.length(), len);
    QCOMPARE(v.lengthSquared(), x * x + y * y + z * z + w * w);
}

// Test the unit vector conversion for 2D vectors.
void tst_QVectorND::normalized2_data()
{
    // Use the same test data as the length test.
    length2_data();
}
void tst_QVectorND::normalized2()
{
    QFETCH(float, x);
    QFETCH(float, y);
    QFETCH(float, len);

    QVector2D v(x, y);
    QVector2D u = v.normalized();
    if (v.isNull())
        QVERIFY(u.isNull());
    else
        QVERIFY(qFuzzyCompare(u.length(), 1.0f));
    QVERIFY(qFuzzyCompare(u.x() * len, v.x()));
    QVERIFY(qFuzzyCompare(u.y() * len, v.y()));
}

// Test the unit vector conversion for 3D vectors.
void tst_QVectorND::normalized3_data()
{
    // Use the same test data as the length test.
    length3_data();
}
void tst_QVectorND::normalized3()
{
    QFETCH(float, x);
    QFETCH(float, y);
    QFETCH(float, z);
    QFETCH(float, len);

    QVector3D v(x, y, z);
    QVector3D u = v.normalized();
    if (v.isNull())
        QVERIFY(u.isNull());
    else
        QVERIFY(qFuzzyCompare(u.length(), 1.0f));
    QVERIFY(qFuzzyCompare(u.x() * len, v.x()));
    QVERIFY(qFuzzyCompare(u.y() * len, v.y()));
    QVERIFY(qFuzzyCompare(u.z() * len, v.z()));
}

// Test the unit vector conversion for 4D vectors.
void tst_QVectorND::normalized4_data()
{
    // Use the same test data as the length test.
    length4_data();
}
void tst_QVectorND::normalized4()
{
    QFETCH(float, x);
    QFETCH(float, y);
    QFETCH(float, z);
    QFETCH(float, w);
    QFETCH(float, len);

    QVector4D v(x, y, z, w);
    QVector4D u = v.normalized();
    if (v.isNull())
        QVERIFY(u.isNull());
    else
        QVERIFY(qFuzzyCompare(u.length(), 1.0f));
    QVERIFY(qFuzzyCompare(u.x() * len, v.x()));
    QVERIFY(qFuzzyCompare(u.y() * len, v.y()));
    QVERIFY(qFuzzyCompare(u.z() * len, v.z()));
    QVERIFY(qFuzzyCompare(u.w() * len, v.w()));
}

// Test the unit vector conversion for 2D vectors.
void tst_QVectorND::normalize2_data()
{
    // Use the same test data as the length test.
    length2_data();
}
void tst_QVectorND::normalize2()
{
    QFETCH(float, x);
    QFETCH(float, y);

    QVector2D v(x, y);
    bool isNull = v.isNull();
    v.normalize();
    if (isNull)
        QVERIFY(v.isNull());
    else
        QVERIFY(qFuzzyCompare(v.length(), 1.0f));
}

// Test the unit vector conversion for 3D vectors.
void tst_QVectorND::normalize3_data()
{
    // Use the same test data as the length test.
    length3_data();
}
void tst_QVectorND::normalize3()
{
    QFETCH(float, x);
    QFETCH(float, y);
    QFETCH(float, z);

    QVector3D v(x, y, z);
    bool isNull = v.isNull();
    v.normalize();
    if (isNull)
        QVERIFY(v.isNull());
    else
        QVERIFY(qFuzzyCompare(v.length(), 1.0f));
}

// Test the unit vector conversion for 4D vectors.
void tst_QVectorND::normalize4_data()
{
    // Use the same test data as the length test.
    length4_data();
}
void tst_QVectorND::normalize4()
{
    QFETCH(float, x);
    QFETCH(float, y);
    QFETCH(float, z);
    QFETCH(float, w);

    QVector4D v(x, y, z, w);
    bool isNull = v.isNull();
    v.normalize();
    if (isNull)
        QVERIFY(v.isNull());
    else
        QVERIFY(qFuzzyCompare(v.length(), 1.0f));
}

// Test the comparison operators for 2D vectors.
void tst_QVectorND::compare2()
{
    QVector2D v1(1, 2);
    QVector2D v2(1, 2);
    QVector2D v3(3, 2);
    QVector2D v4(1, 3);

    QVERIFY(v1 == v2);
    QVERIFY(v1 != v3);
    QVERIFY(v1 != v4);
}

// Test the comparison operators for 3D vectors.
void tst_QVectorND::compare3()
{
    QVector3D v1(1, 2, 4);
    QVector3D v2(1, 2, 4);
    QVector3D v3(3, 2, 4);
    QVector3D v4(1, 3, 4);
    QVector3D v5(1, 2, 3);

    QVERIFY(v1 == v2);
    QVERIFY(v1 != v3);
    QVERIFY(v1 != v4);
    QVERIFY(v1 != v5);
}

// Test the comparison operators for 4D vectors.
void tst_QVectorND::compare4()
{
    QVector4D v1(1, 2, 4, 8);
    QVector4D v2(1, 2, 4, 8);
    QVector4D v3(3, 2, 4, 8);
    QVector4D v4(1, 3, 4, 8);
    QVector4D v5(1, 2, 3, 8);
    QVector4D v6(1, 2, 4, 3);

    QVERIFY(v1 == v2);
    QVERIFY(v1 != v3);
    QVERIFY(v1 != v4);
    QVERIFY(v1 != v5);
    QVERIFY(v1 != v6);
}

// Test vector addition for 2D vectors.
void tst_QVectorND::add2_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("x2");
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("x3");
    QTest::addColumn<float>("y3");

    QTest::newRow("null")
        << 0.0f << 0.0f
        << 0.0f << 0.0f
        << 0.0f << 0.0f;

    QTest::newRow("xonly")
        << 1.0f << 0.0f
        << 2.0f << 0.0f
        << 3.0f << 0.0f;

    QTest::newRow("yonly")
        << 0.0f << 1.0f
        << 0.0f << 2.0f
        << 0.0f << 3.0f;

    QTest::newRow("all")
        << 1.0f << 2.0f
        << 4.0f << 5.0f
        << 5.0f << 7.0f;
}
void tst_QVectorND::add2()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, x3);
    QFETCH(float, y3);

    QVector2D v1(x1, y1);
    QVector2D v2(x2, y2);
    QVector2D v3(x3, y3);

    QVERIFY((v1 + v2) == v3);

    QVector2D v4(v1);
    v4 += v2;
    QVERIFY(v4 == v3);

    QCOMPARE(v4.x(), v1.x() + v2.x());
    QCOMPARE(v4.y(), v1.y() + v2.y());
}

// Test vector addition for 3D vectors.
void tst_QVectorND::add3_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("x2");
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("z2");
    QTest::addColumn<float>("x3");
    QTest::addColumn<float>("y3");
    QTest::addColumn<float>("z3");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 0.0f;

    QTest::newRow("xonly")
        << 1.0f << 0.0f << 0.0f
        << 2.0f << 0.0f << 0.0f
        << 3.0f << 0.0f << 0.0f;

    QTest::newRow("yonly")
        << 0.0f << 1.0f << 0.0f
        << 0.0f << 2.0f << 0.0f
        << 0.0f << 3.0f << 0.0f;

    QTest::newRow("zonly")
        << 0.0f << 0.0f << 1.0f
        << 0.0f << 0.0f << 2.0f
        << 0.0f << 0.0f << 3.0f;

    QTest::newRow("all")
        << 1.0f << 2.0f << 3.0f
        << 4.0f << 5.0f << -6.0f
        << 5.0f << 7.0f << -3.0f;
}
void tst_QVectorND::add3()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, x3);
    QFETCH(float, y3);
    QFETCH(float, z3);

    QVector3D v1(x1, y1, z1);
    QVector3D v2(x2, y2, z2);
    QVector3D v3(x3, y3, z3);

    QVERIFY((v1 + v2) == v3);

    QVector3D v4(v1);
    v4 += v2;
    QVERIFY(v4 == v3);

    QCOMPARE(v4.x(), v1.x() + v2.x());
    QCOMPARE(v4.y(), v1.y() + v2.y());
    QCOMPARE(v4.z(), v1.z() + v2.z());
}

// Test vector addition for 4D vectors.
void tst_QVectorND::add4_data()
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
void tst_QVectorND::add4()
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

    QVector4D v1(x1, y1, z1, w1);
    QVector4D v2(x2, y2, z2, w2);
    QVector4D v3(x3, y3, z3, w3);

    QVERIFY((v1 + v2) == v3);

    QVector4D v4(v1);
    v4 += v2;
    QVERIFY(v4 == v3);

    QCOMPARE(v4.x(), v1.x() + v2.x());
    QCOMPARE(v4.y(), v1.y() + v2.y());
    QCOMPARE(v4.z(), v1.z() + v2.z());
    QCOMPARE(v4.w(), v1.w() + v2.w());
}

// Test vector subtraction for 2D vectors.
void tst_QVectorND::subtract2_data()
{
    // Use the same test data as the add test.
    add2_data();
}
void tst_QVectorND::subtract2()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, x3);
    QFETCH(float, y3);

    QVector2D v1(x1, y1);
    QVector2D v2(x2, y2);
    QVector2D v3(x3, y3);

    QVERIFY((v3 - v1) == v2);
    QVERIFY((v3 - v2) == v1);

    QVector2D v4(v3);
    v4 -= v1;
    QVERIFY(v4 == v2);

    QCOMPARE(v4.x(), v3.x() - v1.x());
    QCOMPARE(v4.y(), v3.y() - v1.y());

    QVector2D v5(v3);
    v5 -= v2;
    QVERIFY(v5 == v1);

    QCOMPARE(v5.x(), v3.x() - v2.x());
    QCOMPARE(v5.y(), v3.y() - v2.y());
}

// Test vector subtraction for 3D vectors.
void tst_QVectorND::subtract3_data()
{
    // Use the same test data as the add test.
    add3_data();
}
void tst_QVectorND::subtract3()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, x3);
    QFETCH(float, y3);
    QFETCH(float, z3);

    QVector3D v1(x1, y1, z1);
    QVector3D v2(x2, y2, z2);
    QVector3D v3(x3, y3, z3);

    QVERIFY((v3 - v1) == v2);
    QVERIFY((v3 - v2) == v1);

    QVector3D v4(v3);
    v4 -= v1;
    QVERIFY(v4 == v2);

    QCOMPARE(v4.x(), v3.x() - v1.x());
    QCOMPARE(v4.y(), v3.y() - v1.y());
    QCOMPARE(v4.z(), v3.z() - v1.z());

    QVector3D v5(v3);
    v5 -= v2;
    QVERIFY(v5 == v1);

    QCOMPARE(v5.x(), v3.x() - v2.x());
    QCOMPARE(v5.y(), v3.y() - v2.y());
    QCOMPARE(v5.z(), v3.z() - v2.z());
}

// Test vector subtraction for 4D vectors.
void tst_QVectorND::subtract4_data()
{
    // Use the same test data as the add test.
    add4_data();
}
void tst_QVectorND::subtract4()
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

    QVector4D v1(x1, y1, z1, w1);
    QVector4D v2(x2, y2, z2, w2);
    QVector4D v3(x3, y3, z3, w3);

    QVERIFY((v3 - v1) == v2);
    QVERIFY((v3 - v2) == v1);

    QVector4D v4(v3);
    v4 -= v1;
    QVERIFY(v4 == v2);

    QCOMPARE(v4.x(), v3.x() - v1.x());
    QCOMPARE(v4.y(), v3.y() - v1.y());
    QCOMPARE(v4.z(), v3.z() - v1.z());
    QCOMPARE(v4.w(), v3.w() - v1.w());

    QVector4D v5(v3);
    v5 -= v2;
    QVERIFY(v5 == v1);

    QCOMPARE(v5.x(), v3.x() - v2.x());
    QCOMPARE(v5.y(), v3.y() - v2.y());
    QCOMPARE(v5.z(), v3.z() - v2.z());
    QCOMPARE(v5.w(), v3.w() - v2.w());
}

// Test component-wise vector multiplication for 2D vectors.
void tst_QVectorND::multiply2_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("x2");
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("x3");
    QTest::addColumn<float>("y3");

    QTest::newRow("null")
        << 0.0f << 0.0f
        << 0.0f << 0.0f
        << 0.0f << 0.0f;

    QTest::newRow("xonly")
        << 1.0f << 0.0f
        << 2.0f << 0.0f
        << 2.0f << 0.0f;

    QTest::newRow("yonly")
        << 0.0f << 1.0f
        << 0.0f << 2.0f
        << 0.0f << 2.0f;

    QTest::newRow("all")
        << 1.0f << 2.0f
        << 4.0f << 5.0f
        << 4.0f << 10.0f;
}
void tst_QVectorND::multiply2()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, x3);
    QFETCH(float, y3);

    QVector2D v1(x1, y1);
    QVector2D v2(x2, y2);
    QVector2D v3(x3, y3);

    QVERIFY((v1 * v2) == v3);

    QVector2D v4(v1);
    v4 *= v2;
    QVERIFY(v4 == v3);

    QCOMPARE(v4.x(), v1.x() * v2.x());
    QCOMPARE(v4.y(), v1.y() * v2.y());
}

// Test component-wise vector multiplication for 3D vectors.
void tst_QVectorND::multiply3_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("x2");
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("z2");
    QTest::addColumn<float>("x3");
    QTest::addColumn<float>("y3");
    QTest::addColumn<float>("z3");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 0.0f;

    QTest::newRow("xonly")
        << 1.0f << 0.0f << 0.0f
        << 2.0f << 0.0f << 0.0f
        << 2.0f << 0.0f << 0.0f;

    QTest::newRow("yonly")
        << 0.0f << 1.0f << 0.0f
        << 0.0f << 2.0f << 0.0f
        << 0.0f << 2.0f << 0.0f;

    QTest::newRow("zonly")
        << 0.0f << 0.0f << 1.0f
        << 0.0f << 0.0f << 2.0f
        << 0.0f << 0.0f << 2.0f;

    QTest::newRow("all")
        << 1.0f << 2.0f << 3.0f
        << 4.0f << 5.0f << -6.0f
        << 4.0f << 10.0f << -18.0f;
}
void tst_QVectorND::multiply3()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, x3);
    QFETCH(float, y3);
    QFETCH(float, z3);

    QVector3D v1(x1, y1, z1);
    QVector3D v2(x2, y2, z2);
    QVector3D v3(x3, y3, z3);

    QVERIFY((v1 * v2) == v3);

    QVector3D v4(v1);
    v4 *= v2;
    QVERIFY(v4 == v3);

    QCOMPARE(v4.x(), v1.x() * v2.x());
    QCOMPARE(v4.y(), v1.y() * v2.y());
    QCOMPARE(v4.z(), v1.z() * v2.z());
}

// Test component-wise vector multiplication for 4D vectors.
void tst_QVectorND::multiply4_data()
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
        << 2.0f << 0.0f << 0.0f << 0.0f;

    QTest::newRow("yonly")
        << 0.0f << 1.0f << 0.0f << 0.0f
        << 0.0f << 2.0f << 0.0f << 0.0f
        << 0.0f << 2.0f << 0.0f << 0.0f;

    QTest::newRow("zonly")
        << 0.0f << 0.0f << 1.0f << 0.0f
        << 0.0f << 0.0f << 2.0f << 0.0f
        << 0.0f << 0.0f << 2.0f << 0.0f;

    QTest::newRow("wonly")
        << 0.0f << 0.0f << 0.0f << 1.0f
        << 0.0f << 0.0f << 0.0f << 2.0f
        << 0.0f << 0.0f << 0.0f << 2.0f;

    QTest::newRow("all")
        << 1.0f << 2.0f << 3.0f << 8.0f
        << 4.0f << 5.0f << -6.0f << 9.0f
        << 4.0f << 10.0f << -18.0f << 72.0f;
}
void tst_QVectorND::multiply4()
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

    QVector4D v1(x1, y1, z1, w1);
    QVector4D v2(x2, y2, z2, w2);
    QVector4D v3(x3, y3, z3, w3);

    QVERIFY((v1 * v2) == v3);

    QVector4D v4(v1);
    v4 *= v2;
    QVERIFY(v4 == v3);

    QCOMPARE(v4.x(), v1.x() * v2.x());
    QCOMPARE(v4.y(), v1.y() * v2.y());
    QCOMPARE(v4.z(), v1.z() * v2.z());
    QCOMPARE(v4.w(), v1.w() * v2.w());
}

// Test vector multiplication by a factor for 2D vectors.
void tst_QVectorND::multiplyFactor2_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("factor");
    QTest::addColumn<float>("x2");
    QTest::addColumn<float>("y2");

    QTest::newRow("null")
        << 0.0f << 0.0f
        << 100.0f
        << 0.0f << 0.0f;

    QTest::newRow("xonly")
        << 1.0f << 0.0f
        << 2.0f
        << 2.0f << 0.0f;

    QTest::newRow("yonly")
        << 0.0f << 1.0f
        << 2.0f
        << 0.0f << 2.0f;

    QTest::newRow("all")
        << 1.0f << 2.0f
        << 2.0f
        << 2.0f << 4.0f;

    QTest::newRow("allzero")
        << 1.0f << 2.0f
        << 0.0f
        << 0.0f << 0.0f;
}
void tst_QVectorND::multiplyFactor2()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, factor);
    QFETCH(float, x2);
    QFETCH(float, y2);

    QVector2D v1(x1, y1);
    QVector2D v2(x2, y2);

    QVERIFY((v1 * factor) == v2);
    QVERIFY((factor * v1) == v2);

    QVector2D v3(v1);
    v3 *= factor;
    QVERIFY(v3 == v2);

    QCOMPARE(v3.x(), v1.x() * factor);
    QCOMPARE(v3.y(), v1.y() * factor);
}

// Test vector multiplication by a factor for 3D vectors.
void tst_QVectorND::multiplyFactor3_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("factor");
    QTest::addColumn<float>("x2");
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("z2");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f
        << 100.0f
        << 0.0f << 0.0f << 0.0f;

    QTest::newRow("xonly")
        << 1.0f << 0.0f << 0.0f
        << 2.0f
        << 2.0f << 0.0f << 0.0f;

    QTest::newRow("yonly")
        << 0.0f << 1.0f << 0.0f
        << 2.0f
        << 0.0f << 2.0f << 0.0f;

    QTest::newRow("zonly")
        << 0.0f << 0.0f << 1.0f
        << 2.0f
        << 0.0f << 0.0f << 2.0f;

    QTest::newRow("all")
        << 1.0f << 2.0f << -3.0f
        << 2.0f
        << 2.0f << 4.0f << -6.0f;

    QTest::newRow("allzero")
        << 1.0f << 2.0f << -3.0f
        << 0.0f
        << 0.0f << 0.0f << 0.0f;
}
void tst_QVectorND::multiplyFactor3()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, factor);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);

    QVector3D v1(x1, y1, z1);
    QVector3D v2(x2, y2, z2);

    QVERIFY((v1 * factor) == v2);
    QVERIFY((factor * v1) == v2);

    QVector3D v3(v1);
    v3 *= factor;
    QVERIFY(v3 == v2);

    QCOMPARE(v3.x(), v1.x() * factor);
    QCOMPARE(v3.y(), v1.y() * factor);
    QCOMPARE(v3.z(), v1.z() * factor);
}

// Test vector multiplication by a factor for 4D vectors.
void tst_QVectorND::multiplyFactor4_data()
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
void tst_QVectorND::multiplyFactor4()
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

    QVector4D v1(x1, y1, z1, w1);
    QVector4D v2(x2, y2, z2, w2);

    QVERIFY((v1 * factor) == v2);
    QVERIFY((factor * v1) == v2);

    QVector4D v3(v1);
    v3 *= factor;
    QVERIFY(v3 == v2);

    QCOMPARE(v3.x(), v1.x() * factor);
    QCOMPARE(v3.y(), v1.y() * factor);
    QCOMPARE(v3.z(), v1.z() * factor);
    QCOMPARE(v3.w(), v1.w() * factor);
}

// Test vector division by a factor for 2D vectors.
void tst_QVectorND::divide2_data()
{
    // Use the same test data as the multiply test.
    multiplyFactor2_data();
}
void tst_QVectorND::divide2()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, factor);
    QFETCH(float, x2);
    QFETCH(float, y2);

    QVector2D v1(x1, y1);
    QVector2D v2(x2, y2);

    if (factor == 0.0f)
        return;

    QVERIFY((v2 / factor) == v1);

    QVector2D v3(v2);
    v3 /= factor;
    QVERIFY(v3 == v1);

    QCOMPARE(v3.x(), v2.x() / factor);
    QCOMPARE(v3.y(), v2.y() / factor);
}

// Test vector division by a factor for 3D vectors.
void tst_QVectorND::divide3_data()
{
    // Use the same test data as the multiply test.
    multiplyFactor3_data();
}
void tst_QVectorND::divide3()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, factor);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);

    QVector3D v1(x1, y1, z1);
    QVector3D v2(x2, y2, z2);

    if (factor == 0.0f)
        return;

    QVERIFY((v2 / factor) == v1);

    QVector3D v3(v2);
    v3 /= factor;
    QVERIFY(v3 == v1);

    QCOMPARE(v3.x(), v2.x() / factor);
    QCOMPARE(v3.y(), v2.y() / factor);
    QCOMPARE(v3.z(), v2.z() / factor);
}

// Test vector division by a factor for 4D vectors.
void tst_QVectorND::divide4_data()
{
    // Use the same test data as the multiply test.
    multiplyFactor4_data();
}
void tst_QVectorND::divide4()
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

    QVector4D v1(x1, y1, z1, w1);
    QVector4D v2(x2, y2, z2, w2);

    if (factor == 0.0f)
        return;

    QVERIFY((v2 / factor) == v1);

    QVector4D v3(v2);
    v3 /= factor;
    QVERIFY(v3 == v1);

    QCOMPARE(v3.x(), v2.x() / factor);
    QCOMPARE(v3.y(), v2.y() / factor);
    QCOMPARE(v3.z(), v2.z() / factor);
    QCOMPARE(v3.w(), v2.w() / factor);
}

// Test vector negation for 2D vectors.
void tst_QVectorND::negate2_data()
{
    // Use the same test data as the add test.
    add2_data();
}
void tst_QVectorND::negate2()
{
    QFETCH(float, x1);
    QFETCH(float, y1);

    QVector2D v1(x1, y1);
    QVector2D v2(-x1, -y1);

    QVERIFY(-v1 == v2);
}

// Test vector negation for 3D vectors.
void tst_QVectorND::negate3_data()
{
    // Use the same test data as the add test.
    add3_data();
}
void tst_QVectorND::negate3()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);

    QVector3D v1(x1, y1, z1);
    QVector3D v2(-x1, -y1, -z1);

    QVERIFY(-v1 == v2);
}

// Test vector negation for 4D vectors.
void tst_QVectorND::negate4_data()
{
    // Use the same test data as the add test.
    add4_data();
}
void tst_QVectorND::negate4()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, w1);

    QVector4D v1(x1, y1, z1, w1);
    QVector4D v2(-x1, -y1, -z1, -w1);

    QVERIFY(-v1 == v2);
}

// Test the computation of vector cross-products.
void tst_QVectorND::crossProduct_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("x2");
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("z2");
    QTest::addColumn<float>("x3");
    QTest::addColumn<float>("y3");
    QTest::addColumn<float>("z3");
    QTest::addColumn<float>("dot");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 0.0f
        << 0.0f;

    QTest::newRow("unitvec")
        << 1.0f << 0.0f << 0.0f
        << 0.0f << 1.0f << 0.0f
        << 0.0f << 0.0f << 1.0f
        << 0.0f;

    QTest::newRow("complex")
        << 1.0f << 2.0f << 3.0f
        << 4.0f << 5.0f << 6.0f
        << -3.0f << 6.0f << -3.0f
        << 32.0f;
}
void tst_QVectorND::crossProduct()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, x3);
    QFETCH(float, y3);
    QFETCH(float, z3);

    QVector3D v1(x1, y1, z1);
    QVector3D v2(x2, y2, z2);
    QVector3D v3(x3, y3, z3);

    QVector3D v4 = QVector3D::crossProduct(v1, v2);
    QVERIFY(v4 == v3);

    // Compute the cross-product long-hand and check again.
    float xres = y1 * z2 - z1 * y2;
    float yres = z1 * x2 - x1 * z2;
    float zres = x1 * y2 - y1 * x2;

    QCOMPARE(v4.x(), xres);
    QCOMPARE(v4.y(), yres);
    QCOMPARE(v4.z(), zres);
}

// Test the computation of normals.
void tst_QVectorND::normal_data()
{
    // Use the same test data as the crossProduct test.
    crossProduct_data();
}
void tst_QVectorND::normal()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, x3);
    QFETCH(float, y3);
    QFETCH(float, z3);

    QVector3D v1(x1, y1, z1);
    QVector3D v2(x2, y2, z2);
    QVector3D v3(x3, y3, z3);

    QVERIFY(QVector3D::normal(v1, v2) == v3.normalized());
    QVERIFY(QVector3D::normal(QVector3D(), v1, v2) == v3.normalized());

    QVector3D point(1.0f, 2.0f, 3.0f);
    QVERIFY(QVector3D::normal(point, v1 + point, v2 + point) == v3.normalized());
}

// Test distance to point calculations.
void tst_QVectorND::distanceToPoint2_data()
{
    QTest::addColumn<float>("x1");  // Point to test for distance
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("x2");  // Point to test against
    QTest::addColumn<float>("y2");

    QTest::addColumn<float>("distance");

    QTest::newRow("null")
        << 0.0f << 0.0f
        << 0.0f << 1.0f
        << 1.0f;

    QTest::newRow("on point")
        << 1.0f << 1.0f
        << 1.0f << 1.0f
        << 0.0f;

    QTest::newRow("off point")
        << 0.0f << 1.0f
        << 0.0f << 2.0f
        << 1.0f;

    QTest::newRow("off point 2")
        << 0.0f << 0.0f
        << 0.0f << 2.0f
        << 2.0f;

    QTest::newRow("minus point")
        << 0.0f << 0.0f
        << 0.0f << -2.0f
        << 2.0f;
}
void tst_QVectorND::distanceToPoint2()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, distance);

    QVector2D v1(x1, y1);
    QVector2D v2(x2, y2);

    QCOMPARE(v1.distanceToPoint(v2), distance);
}

// Test distance to point calculations.
void tst_QVectorND::distanceToPoint3_data()
{
    QTest::addColumn<float>("x1");  // Point to test for distance
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("x2");  // Point to test against
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("z2");

    QTest::addColumn<float>("distance");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 1.0f
        << 1.0f;

    QTest::newRow("on point")
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 0.0f
        << 0.0f;

    QTest::newRow("off point")
        << 0.0f << 0.0f << 1.0f
        << 0.0f << 0.0f << 2.0f
        << 1.0f;

    QTest::newRow("off point 2")
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 2.0f << 0.0f
        << 2.0f;

    QTest::newRow("minus point")
        << 0.0f << 0.0f << 0.0f
        << 0.0f << -2.0f << 0.0f
        << 2.0f;
}
void tst_QVectorND::distanceToPoint3()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, distance);

    QVector3D v1(x1, y1, z1);
    QVector3D v2(x2, y2, z2);

    QCOMPARE(v1.distanceToPoint(v2), distance);
}

// Test distance to plane calculations.
void tst_QVectorND::distanceToPlane_data()
{
    QTest::addColumn<float>("x1");  // Point on plane
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("x2");  // Normal to plane
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("z2");
    QTest::addColumn<float>("x3");  // Point to test for distance
    QTest::addColumn<float>("y3");
    QTest::addColumn<float>("z3");
    QTest::addColumn<float>("x4");  // Second point on plane
    QTest::addColumn<float>("y4");
    QTest::addColumn<float>("z4");
    QTest::addColumn<float>("x5");  // Third point on plane
    QTest::addColumn<float>("y5");
    QTest::addColumn<float>("z5");
    QTest::addColumn<float>("distance");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 1.0f
        << 0.0f << 0.0f << 0.0f
        << 1.0f << 0.0f << 0.0f
        << 0.0f << 2.0f << 0.0f
        << 0.0f;

    QTest::newRow("above")
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 1.0f
        << 0.0f << 0.0f << 2.0f
        << 1.0f << 0.0f << 0.0f
        << 0.0f << 2.0f << 0.0f
        << 2.0f;

    QTest::newRow("below")
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 1.0f
        << -1.0f << 1.0f << -2.0f
        << 1.0f << 0.0f << 0.0f
        << 0.0f << 2.0f << 0.0f
        << -2.0f;
}
void tst_QVectorND::distanceToPlane()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, x3);
    QFETCH(float, y3);
    QFETCH(float, z3);
    QFETCH(float, x4);
    QFETCH(float, y4);
    QFETCH(float, z4);
    QFETCH(float, x5);
    QFETCH(float, y5);
    QFETCH(float, z5);
    QFETCH(float, distance);

    QVector3D v1(x1, y1, z1);
    QVector3D v2(x2, y2, z2);
    QVector3D v3(x3, y3, z3);
    QVector3D v4(x4, y4, z4);
    QVector3D v5(x5, y5, z5);

    QCOMPARE(v3.distanceToPlane(v1, v2), distance);
    QCOMPARE(v3.distanceToPlane(v1, v4, v5), distance);
}

// Test distance to line calculations.
void tst_QVectorND::distanceToLine2_data()
{
    QTest::addColumn<float>("x1");  // Point on line
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("x2");  // Direction of the line
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("x3");  // Point to test for distance
    QTest::addColumn<float>("y3");
    QTest::addColumn<float>("distance");

    QTest::newRow("null")
        << 0.0f << 0.0f
        << 0.0f << 0.1f
        << 0.0f << 0.0f
        << 0.0f;

    QTest::newRow("on line")
        << 0.0f << 0.0f
        << 0.0f << 1.0f
        << 0.0f << 5.0f
        << 0.0f;

    QTest::newRow("off line")
        << 0.0f << 0.0f
        << 0.0f << 1.0f
        << 1.0f << 0.0f
        << 1.0f;

    QTest::newRow("off line 2")
        << 0.0f << 0.0f
        << 0.0f << 1.0f
        << -2.0f << 0.0f
        << 2.0f;

    QTest::newRow("points")
        << 0.0f << 0.0f
        << 0.0f << 0.0f
        << 0.0f << 5.0f
        << 5.0f;
}

void tst_QVectorND::distanceToLine2()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, x3);
    QFETCH(float, y3);
    QFETCH(float, distance);

    QVector2D v1(x1, y1);
    QVector2D v2(x2, y2);
    QVector2D v3(x3, y3);

    QCOMPARE(v3.distanceToLine(v1, v2), distance);
}
// Test distance to line calculations.
void tst_QVectorND::distanceToLine3_data()
{
    QTest::addColumn<float>("x1");  // Point on line
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("x2");  // Direction of the line
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("z2");
    QTest::addColumn<float>("x3");  // Point to test for distance
    QTest::addColumn<float>("y3");
    QTest::addColumn<float>("z3");
    QTest::addColumn<float>("distance");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 1.0f
        << 0.0f << 0.0f << 0.0f
        << 0.0f;

    QTest::newRow("on line")
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 1.0f
        << 0.0f << 0.0f << 5.0f
        << 0.0f;

    QTest::newRow("off line")
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 1.0f
        << 1.0f << 0.0f << 0.0f
        << 1.0f;

    QTest::newRow("off line 2")
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 1.0f
        << 0.0f << -2.0f << 0.0f
        << 2.0f;

    QTest::newRow("points")
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 0.0f
        << 0.0f << 5.0f << 0.0f
        << 5.0f;
}
void tst_QVectorND::distanceToLine3()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, x3);
    QFETCH(float, y3);
    QFETCH(float, z3);
    QFETCH(float, distance);

    QVector3D v1(x1, y1, z1);
    QVector3D v2(x2, y2, z2);
    QVector3D v3(x3, y3, z3);

    QCOMPARE(v3.distanceToLine(v1, v2), distance);
}

// Test the computation of dot products for 2D vectors.
void tst_QVectorND::dotProduct2_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("x2");
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("dot");

    QTest::newRow("null")
        << 0.0f << 0.0f
        << 0.0f << 0.0f
        << 0.0f;

    QTest::newRow("unitvec")
        << 1.0f << 0.0f
        << 0.0f << 1.0f
        << 0.0f;

    QTest::newRow("complex")
        << 1.0f << 2.0f
        << 4.0f << 5.0f
        << 14.0f;
}
void tst_QVectorND::dotProduct2()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, dot);

    QVector2D v1(x1, y1);
    QVector2D v2(x2, y2);

    QVERIFY(QVector2D::dotProduct(v1, v2) == dot);

    // Compute the dot-product long-hand and check again.
    float d = x1 * x2 + y1 * y2;

    QCOMPARE(QVector2D::dotProduct(v1, v2), d);
}

// Test the computation of dot products for 3D vectors.
void tst_QVectorND::dotProduct3_data()
{
    // Use the same test data as the crossProduct test.
    crossProduct_data();
}
void tst_QVectorND::dotProduct3()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, x3);
    QFETCH(float, y3);
    QFETCH(float, z3);
    QFETCH(float, dot);

    Q_UNUSED(x3);
    Q_UNUSED(y3);
    Q_UNUSED(z3);

    QVector3D v1(x1, y1, z1);
    QVector3D v2(x2, y2, z2);

    QVERIFY(QVector3D::dotProduct(v1, v2) == dot);

    // Compute the dot-product long-hand and check again.
    float d = x1 * x2 + y1 * y2 + z1 * z2;

    QCOMPARE(QVector3D::dotProduct(v1, v2), d);
}

// Test the computation of dot products for 4D vectors.
void tst_QVectorND::dotProduct4_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("w1");
    QTest::addColumn<float>("x2");
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("z2");
    QTest::addColumn<float>("w2");
    QTest::addColumn<float>("dot");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 0.0f << 0.0f
        << 0.0f;

    QTest::newRow("unitvec")
        << 1.0f << 0.0f << 0.0f << 0.0f
        << 0.0f << 1.0f << 0.0f << 0.0f
        << 0.0f;

    QTest::newRow("complex")
        << 1.0f << 2.0f << 3.0f << 4.0f
        << 4.0f << 5.0f << 6.0f << 7.0f
        << 60.0f;
}
void tst_QVectorND::dotProduct4()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, w1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, w2);
    QFETCH(float, dot);

    QVector4D v1(x1, y1, z1, w1);
    QVector4D v2(x2, y2, z2, w2);

    QVERIFY(QVector4D::dotProduct(v1, v2) == dot);

    // Compute the dot-product long-hand and check again.
    float d = x1 * x2 + y1 * y2 + z1 * z2 + w1 * w2;

    QCOMPARE(QVector4D::dotProduct(v1, v2), d);
}

class tst_QVectorNDProperties : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVector2D vector2D READ vector2D WRITE setVector2D)
    Q_PROPERTY(QVector3D vector3D READ vector3D WRITE setVector3D)
    Q_PROPERTY(QVector4D vector4D READ vector4D WRITE setVector4D)
public:
    tst_QVectorNDProperties(QObject *parent = 0) : QObject(parent) {}

    QVector2D vector2D() const { return v2; }
    void setVector2D(const QVector2D& value) { v2 = value; }

    QVector3D vector3D() const { return v3; }
    void setVector3D(const QVector3D& value) { v3 = value; }

    QVector4D vector4D() const { return v4; }
    void setVector4D(const QVector4D& value) { v4 = value; }

private:
    QVector2D v2;
    QVector3D v3;
    QVector4D v4;
};

// Test getting and setting vector properties via the metaobject system.
void tst_QVectorND::properties()
{
    tst_QVectorNDProperties obj;

    obj.setVector2D(QVector2D(1.0f, 2.0f));
    obj.setVector3D(QVector3D(3.0f, 4.0f, 5.0f));
    obj.setVector4D(QVector4D(6.0f, 7.0f, 8.0f, 9.0f));

    QVector2D v2 = qvariant_cast<QVector2D>(obj.property("vector2D"));
    QCOMPARE(v2.x(), 1.0f);
    QCOMPARE(v2.y(), 2.0f);

    QVector3D v3 = qvariant_cast<QVector3D>(obj.property("vector3D"));
    QCOMPARE(v3.x(), 3.0f);
    QCOMPARE(v3.y(), 4.0f);
    QCOMPARE(v3.z(), 5.0f);

    QVector4D v4 = qvariant_cast<QVector4D>(obj.property("vector4D"));
    QCOMPARE(v4.x(), 6.0f);
    QCOMPARE(v4.y(), 7.0f);
    QCOMPARE(v4.z(), 8.0f);
    QCOMPARE(v4.w(), 9.0f);

    obj.setProperty("vector2D",
                    QVariant::fromValue(QVector2D(-1.0f, -2.0f)));
    obj.setProperty("vector3D",
                    QVariant::fromValue(QVector3D(-3.0f, -4.0f, -5.0f)));
    obj.setProperty("vector4D",
                    QVariant::fromValue(QVector4D(-6.0f, -7.0f, -8.0f, -9.0f)));

    v2 = qvariant_cast<QVector2D>(obj.property("vector2D"));
    QCOMPARE(v2.x(), -1.0f);
    QCOMPARE(v2.y(), -2.0f);

    v3 = qvariant_cast<QVector3D>(obj.property("vector3D"));
    QCOMPARE(v3.x(), -3.0f);
    QCOMPARE(v3.y(), -4.0f);
    QCOMPARE(v3.z(), -5.0f);

    v4 = qvariant_cast<QVector4D>(obj.property("vector4D"));
    QCOMPARE(v4.x(), -6.0f);
    QCOMPARE(v4.y(), -7.0f);
    QCOMPARE(v4.z(), -8.0f);
    QCOMPARE(v4.w(), -9.0f);
}

void tst_QVectorND::metaTypes()
{
    QVERIFY(QMetaType::type("QVector2D") == QMetaType::QVector2D);
    QVERIFY(QMetaType::type("QVector3D") == QMetaType::QVector3D);
    QVERIFY(QMetaType::type("QVector4D") == QMetaType::QVector4D);

    QCOMPARE(QByteArray(QMetaType::typeName(QMetaType::QVector2D)),
             QByteArray("QVector2D"));
    QCOMPARE(QByteArray(QMetaType::typeName(QMetaType::QVector3D)),
             QByteArray("QVector3D"));
    QCOMPARE(QByteArray(QMetaType::typeName(QMetaType::QVector4D)),
             QByteArray("QVector4D"));

    QVERIFY(QMetaType::isRegistered(QMetaType::QVector2D));
    QVERIFY(QMetaType::isRegistered(QMetaType::QVector3D));
    QVERIFY(QMetaType::isRegistered(QMetaType::QVector4D));

    QVERIFY(qMetaTypeId<QVector2D>() == QMetaType::QVector2D);
    QVERIFY(qMetaTypeId<QVector3D>() == QMetaType::QVector3D);
    QVERIFY(qMetaTypeId<QVector4D>() == QMetaType::QVector4D);
}

QTEST_APPLESS_MAIN(tst_QVectorND)

#include "tst_qvectornd.moc"
