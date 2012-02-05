/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <qgraphicsitem.h>
#include <qgraphicstransform.h>

class tst_QGraphicsTransform : public QObject {
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void scale();
    void rotation();
    void rotation3d_data();
    void rotation3d();
    void rotation3dArbitraryAxis_data();
    void rotation3dArbitraryAxis();

private:
    QString toString(QTransform const&);
};


// This will be called before the first test function is executed.
// It is only called once.
void tst_QGraphicsTransform::initTestCase()
{
}

// This will be called after the last test function is executed.
// It is only called once.
void tst_QGraphicsTransform::cleanupTestCase()
{
}

// This will be called before each test function is executed.
void tst_QGraphicsTransform::init()
{
}

// This will be called after every test function.
void tst_QGraphicsTransform::cleanup()
{
}

static QTransform transform2D(const QGraphicsTransform& t)
{
    QMatrix4x4 m;
    t.applyTo(&m);
    return m.toTransform();
}

void tst_QGraphicsTransform::scale()
{
    QGraphicsScale scale;

    // check initial conditions
    QCOMPARE(scale.xScale(), qreal(1));
    QCOMPARE(scale.yScale(), qreal(1));
    QCOMPARE(scale.zScale(), qreal(1));
    QCOMPARE(scale.origin(), QVector3D(0, 0, 0));

    scale.setOrigin(QVector3D(10, 10, 0));

    QCOMPARE(scale.xScale(), qreal(1));
    QCOMPARE(scale.yScale(), qreal(1));
    QCOMPARE(scale.zScale(), qreal(1));
    QCOMPARE(scale.origin(), QVector3D(10, 10, 0));

    QMatrix4x4 t;
    scale.applyTo(&t);

    QCOMPARE(t, QMatrix4x4());
    QCOMPARE(transform2D(scale), QTransform());

    scale.setXScale(10);
    scale.setOrigin(QVector3D(0, 0, 0));

    QCOMPARE(scale.xScale(), qreal(10));
    QCOMPARE(scale.yScale(), qreal(1));
    QCOMPARE(scale.zScale(), qreal(1));
    QCOMPARE(scale.origin(), QVector3D(0, 0, 0));

    QTransform res;
    res.scale(10, 1);

    QCOMPARE(transform2D(scale), res);
    QCOMPARE(transform2D(scale).map(QPointF(10, 10)), QPointF(100, 10));

    scale.setOrigin(QVector3D(10, 10, 0));
    QCOMPARE(transform2D(scale).map(QPointF(10, 10)), QPointF(10, 10));
    QCOMPARE(transform2D(scale).map(QPointF(11, 10)), QPointF(20, 10));

    scale.setYScale(2);
    scale.setZScale(4.5);
    scale.setOrigin(QVector3D(1, 2, 3));

    QCOMPARE(scale.xScale(), qreal(10));
    QCOMPARE(scale.yScale(), qreal(2));
    QCOMPARE(scale.zScale(), qreal(4.5));
    QCOMPARE(scale.origin(), QVector3D(1, 2, 3));

    QMatrix4x4 t2;
    scale.applyTo(&t2);

    QCOMPARE(t2.map(QVector3D(4, 5, 6)), QVector3D(31, 8, 16.5));

    // Because the origin has a non-zero z, mapping (4, 5) in 2D
    // will introduce a projective component into the result.
    QTransform t3 = t2.toTransform();
    QCOMPARE(t3.map(QPointF(4, 5)), QPointF(31 / t3.m33(), 8 / t3.m33()));
}

// QMatrix4x4 uses float internally, whereas QTransform uses qreal.
// This can lead to issues with qFuzzyCompare() where it uses double
// precision to compare values that have no more than float precision
// after conversion from QMatrix4x4 to QTransform.  The following
// definitions correct for the difference.
static inline bool fuzzyCompare(qreal p1, qreal p2)
{
    // increase delta on small machines using float instead of double
    if (sizeof(qreal) == sizeof(float))
        return (qAbs(p1 - p2) <= 0.00003f * qMin(qAbs(p1), qAbs(p2)));
    else
        return (qAbs(p1 - p2) <= 0.00001f * qMin(qAbs(p1), qAbs(p2)));
}

static bool fuzzyCompare(const QTransform& t1, const QTransform& t2)
{
    return fuzzyCompare(t1.m11(), t2.m11()) &&
           fuzzyCompare(t1.m12(), t2.m12()) &&
           fuzzyCompare(t1.m13(), t2.m13()) &&
           fuzzyCompare(t1.m21(), t2.m21()) &&
           fuzzyCompare(t1.m22(), t2.m22()) &&
           fuzzyCompare(t1.m23(), t2.m23()) &&
           fuzzyCompare(t1.m31(), t2.m31()) &&
           fuzzyCompare(t1.m32(), t2.m32()) &&
           fuzzyCompare(t1.m33(), t2.m33());
}

static inline bool fuzzyCompare(const QMatrix4x4& m1, const QMatrix4x4& m2)
{
    bool ok = true;
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            ok &= fuzzyCompare(m1(y, x), m2(y, x));
    return ok;
}

void tst_QGraphicsTransform::rotation()
{
    QGraphicsRotation rotation;
    QCOMPARE(rotation.axis(), QVector3D(0, 0, 1));
    QCOMPARE(rotation.origin(), QVector3D(0, 0, 0));
    QCOMPARE(rotation.angle(), (qreal)0);

    rotation.setOrigin(QVector3D(10, 10, 0));

    QCOMPARE(rotation.axis(), QVector3D(0, 0, 1));
    QCOMPARE(rotation.origin(), QVector3D(10, 10, 0));
    QCOMPARE(rotation.angle(), (qreal)0);

    QMatrix4x4 t;
    rotation.applyTo(&t);

    QCOMPARE(t, QMatrix4x4());
    QCOMPARE(transform2D(rotation), QTransform());

    rotation.setAngle(40);
    rotation.setOrigin(QVector3D(0, 0, 0));

    QCOMPARE(rotation.axis(), QVector3D(0, 0, 1));
    QCOMPARE(rotation.origin(), QVector3D(0, 0, 0));
    QCOMPARE(rotation.angle(), (qreal)40);

    QTransform res;
    res.rotate(40);

    QVERIFY(fuzzyCompare(transform2D(rotation), res));

    rotation.setOrigin(QVector3D(10, 10, 0));
    rotation.setAngle(90);
    QCOMPARE(transform2D(rotation).map(QPointF(10, 10)), QPointF(10, 10));
    QCOMPARE(transform2D(rotation).map(QPointF(20, 10)), QPointF(10, 20));

    rotation.setOrigin(QVector3D(0, 0, 0));
    rotation.setAngle(qQNaN());
    QCOMPARE(transform2D(rotation).map(QPointF(20, 10)), QPointF(20, 10));
}

Q_DECLARE_METATYPE(Qt::Axis);
void tst_QGraphicsTransform::rotation3d_data()
{
    QTest::addColumn<Qt::Axis>("axis");
    QTest::addColumn<qreal>("angle");

    for (int angle = 0; angle <= 360; angle++) {
        QTest::newRow("test rotation on X") << Qt::XAxis << qreal(angle);
        QTest::newRow("test rotation on Y") << Qt::YAxis << qreal(angle);
        QTest::newRow("test rotation on Z") << Qt::ZAxis << qreal(angle);
    }
}

void tst_QGraphicsTransform::rotation3d()
{
    QFETCH(Qt::Axis, axis);
    QFETCH(qreal, angle);

    QGraphicsRotation rotation;
    rotation.setAxis(axis);

    QMatrix4x4 t;
    rotation.applyTo(&t);

    QVERIFY(t.isIdentity());
    QVERIFY(transform2D(rotation).isIdentity());

    rotation.setAngle(angle);

    // QGraphicsRotation uses a correct mathematical rotation in 3D.
    // QTransform's Qt::YAxis rotation is inverted from the mathematical
    // version of rotation.  We correct for that here.
    QTransform expected;
    if (axis == Qt::YAxis && angle != 180.)
        expected.rotate(-angle, axis);
    else
        expected.rotate(angle, axis);

    QVERIFY(fuzzyCompare(transform2D(rotation), expected));

    // Check that "rotation" produces the 4x4 form of the 3x3 matrix.
    // i.e. third row and column are 0 0 1 0.
    t.setToIdentity();
    rotation.applyTo(&t);
    QMatrix4x4 r(expected);
    if (sizeof(qreal) == sizeof(float) && angle == 268) {
        // This test fails, on only this angle, when qreal == float
        // because the deg2rad value in QTransform is not accurate
        // enough to match what QMatrix4x4 is doing.
    } else {
        QVERIFY(fuzzyCompare(t, r));
    }

    //now let's check that a null vector will not change the transform
    rotation.setAxis(QVector3D(0, 0, 0));
    rotation.setOrigin(QVector3D(10, 10, 0));

    t.setToIdentity();
    rotation.applyTo(&t);

    QVERIFY(t.isIdentity());
    QVERIFY(transform2D(rotation).isIdentity());

    rotation.setAngle(angle);

    QVERIFY(t.isIdentity());
    QVERIFY(transform2D(rotation).isIdentity());

    rotation.setOrigin(QVector3D(0, 0, 0));

    QVERIFY(t.isIdentity());
    QVERIFY(transform2D(rotation).isIdentity());
}

QByteArray labelForTest(QVector3D const& axis, int angle) {
    return QString("rotation of %1 on (%2, %3, %4)")
        .arg(angle)
        .arg(axis.x())
        .arg(axis.y())
        .arg(axis.z())
        .toLatin1();
}

void tst_QGraphicsTransform::rotation3dArbitraryAxis_data()
{
    QTest::addColumn<QVector3D>("axis");
    QTest::addColumn<qreal>("angle");

    QVector3D axis1 = QVector3D(1.0f, 1.0f, 1.0f);
    QVector3D axis2 = QVector3D(2.0f, -3.0f, 0.5f);
    QVector3D axis3 = QVector3D(-2.0f, 0.0f, -0.5f);
    QVector3D axis4 = QVector3D(0.0001f, 0.0001f, 0.0001f);
    QVector3D axis5 = QVector3D(0.01f, 0.01f, 0.01f);

    for (int angle = 0; angle <= 360; angle++) {
        QTest::newRow(labelForTest(axis1, angle).constData()) << axis1 << qreal(angle);
        QTest::newRow(labelForTest(axis2, angle).constData()) << axis2 << qreal(angle);
        QTest::newRow(labelForTest(axis3, angle).constData()) << axis3 << qreal(angle);
        QTest::newRow(labelForTest(axis4, angle).constData()) << axis4 << qreal(angle);
        QTest::newRow(labelForTest(axis5, angle).constData()) << axis5 << qreal(angle);
    }
}

void tst_QGraphicsTransform::rotation3dArbitraryAxis()
{
    QFETCH(QVector3D, axis);
    QFETCH(qreal, angle);

    QGraphicsRotation rotation;
    rotation.setAxis(axis);

    QMatrix4x4 t;
    rotation.applyTo(&t);

    QVERIFY(t.isIdentity());
    QVERIFY(transform2D(rotation).isIdentity());

    rotation.setAngle(angle);

    // Compute the expected answer using QMatrix4x4 and a projection.
    // These two steps are performed in one hit by QGraphicsRotation.
    QMatrix4x4 exp;
    exp.rotate(angle, axis);
    QTransform expected = exp.toTransform(1024.0f);

#if defined(MAY_HIT_QTBUG_20661)
    // These failures possibly relate to the float vs qreal issue mentioned
    // in the comment above fuzzyCompare().
    if (sizeof(qreal) == sizeof(double)) {
        QEXPECT_FAIL("rotation of 120 on (1, 1, 1)",                "QTBUG-20661", Abort);
        QEXPECT_FAIL("rotation of 240 on (1, 1, 1)",                "QTBUG-20661", Abort);
        QEXPECT_FAIL("rotation of 120 on (0.01, 0.01, 0.01)",       "QTBUG-20661", Abort);
        QEXPECT_FAIL("rotation of 240 on (0.01, 0.01, 0.01)",       "QTBUG-20661", Abort);
        QEXPECT_FAIL("rotation of 120 on (0.0001, 0.0001, 0.0001)", "QTBUG-20661", Abort);
        QEXPECT_FAIL("rotation of 240 on (0.0001, 0.0001, 0.0001)", "QTBUG-20661", Abort);
    }
#endif

    QTransform actual = transform2D(rotation);
    QVERIFY2(fuzzyCompare(actual, expected), qPrintable(
        QString("\nactual:   %1\n"
                  "expected: %2")
        .arg(toString(actual))
        .arg(toString(expected))
    ));

    // Check that "rotation" produces the 4x4 form of the 3x3 matrix.
    // i.e. third row and column are 0 0 1 0.
    t.setToIdentity();
    rotation.applyTo(&t);
    QMatrix4x4 r(expected);
    QVERIFY(qFuzzyCompare(t, r));
}

QString tst_QGraphicsTransform::toString(QTransform const& t)
{
    return QString("[ [ %1  %2   %3 ]; [ %4  %5  %6 ]; [ %7  %8  %9 ] ]")
        .arg(t.m11())
        .arg(t.m12())
        .arg(t.m13())
        .arg(t.m21())
        .arg(t.m22())
        .arg(t.m23())
        .arg(t.m31())
        .arg(t.m32())
        .arg(t.m33())
    ;
}


QTEST_MAIN(tst_QGraphicsTransform)
#include "tst_qgraphicstransform.moc"

