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
#include "qtransform.h"
#include <qpolygon.h>
#include <qdebug.h>


class tst_QTransform : public QObject
{
    Q_OBJECT

private slots:
    void mapRect_data();
    void mapToPolygon_data();
    void mapRect();
    void assignments();
    void mapToPolygon();
    void qhash();
    void translate();
    void scale();
    void matrix();
    void testOffset();
    void types();
    void types2_data();
    void types2();
    void scalarOps();
    void transform();
    void mapEmptyPath();
    void boundingRect();
    void controlPointRect();
    void inverted_data();
    void inverted();
    void projectivePathMapping();
    void mapInt();
    void mapPathWithPoint();

private:
    void mapping_data();
};

Q_DECLARE_METATYPE(QTransform)
Q_DECLARE_METATYPE(QTransform::TransformationType)

void tst_QTransform::mapRect_data()
{
    mapping_data();

    // rotations that are not multiples of 90 degrees. mapRect returns the bounding rect here.
    qreal deg = -45;
    QTest::newRow( "rot 45 a" )
        << QTransform().rotate(deg)
        << QRect( 0, 0, 10, 10 )
        << QPolygon( QRect( 0, -7, 14, 14 ) );
    QTest::newRow( "rot 45 b" )
        << QTransform().rotate(deg)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( 21, -14, 50, 49 ) );
    QTest::newRow( "rot 45 c" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 0, 0, 10, 10 )
        << QPolygon( QRect( 0, -71, 141, 142 ) );
    QTest::newRow( "rot 45 d" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( 212, -141, 495, 495 ) );

    deg = 45;
    QTest::newRow( "rot -45 a" )
        << QTransform().rotate(deg)
        << QRect( 0, 0, 10, 10 )
        << QPolygon( QRect( -7, 0, 14, 14 ) );
    QTest::newRow( "rot -45 b" )
        << QTransform().rotate(deg)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( -35, 21, 49, 50 ) );
    QTest::newRow( "rot -45 c" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 0, 0, 10, 10 )
        << QPolygon( QRect( -71, 0, 142, 141 ) );
    QTest::newRow( "rot -45 d" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( -354, 212, 495, 495 ) );
}

void tst_QTransform::mapToPolygon_data()
{
    mapping_data();
}

void tst_QTransform::mapping_data()
{
    //create the testtable instance and define the elements
    QTest::addColumn<QTransform>("matrix");
    QTest::addColumn<QRect>("src");
    QTest::addColumn<QPolygon>("res");

    //next we fill it with data

    // identity
    QTest::newRow( "identity" )
        << QTransform()
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( 10, 20, 30, 40 ) );
    // scaling
    QTest::newRow( "scale 0" )
        << QTransform().scale(2, 2)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( 20, 40, 60, 80 ) );
    QTest::newRow( "scale 1" )
        << QTransform().scale(10, 10)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( 100, 200, 300, 400 ) );
    // mirroring
    QTest::newRow( "mirror 0" )
        << QTransform().scale(-1, 1)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( -40, 20, 30, 40 ) );
    QTest::newRow( "mirror 1" )
        << QTransform().scale(1, -1)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( 10, -60, 30, 40 ) );
    QTest::newRow( "mirror 2" )
        << QTransform().scale(-1, -1)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( -40, -60, 30, 40 ) );
    QTest::newRow( "mirror 3" )
        << QTransform().scale(-2, -2)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( -80, -120, 60, 80 ) );
    QTest::newRow( "mirror 4" )
        << QTransform().scale(-10, -10)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( -400, -600, 300, 400 ) );
    QTest::newRow( "mirror 5" )
        << QTransform().scale(-1, 1)
        << QRect( 0, 0, 30, 40 )
        << QPolygon( QRect( -30, 0, 30, 40 ) );
    QTest::newRow( "mirror 6" )
        << QTransform().scale(1, -1)
        << QRect( 0, 0, 30, 40 )
        << QPolygon( QRect( 0, -40, 30, 40 ) );
    QTest::newRow( "mirror 7" )
        << QTransform().scale(-1, -1)
        << QRect( 0, 0, 30, 40 )
        << QPolygon( QRect( -30, -40, 30, 40 ) );
    QTest::newRow( "mirror 8" )
        << QTransform().scale(-2, -2)
        << QRect( 0, 0, 30, 40 )
        << QPolygon( QRect( -60, -80, 60, 80 ) );
    QTest::newRow( "mirror 9" )
        << QTransform().scale(-10, -10) << QRect( 0, 0, 30, 40 )
        << QPolygon( QRect( -300, -400, 300, 400 ) );

    // rotations
    float deg = 0.;
    QTest::newRow( "rot 0 a" )
        << QTransform().rotate(deg)
        << QRect( 0, 0, 30, 40 )
        << QPolygon ( QRect( 0, 0, 30, 40 ) );
    deg = 0.00001f;
    QTest::newRow( "rot 0 b" )
        << QTransform().rotate(deg)
        << QRect( 0, 0, 30, 40 )
        << QPolygon ( QRect( 0, 0, 30, 40 ) );
    deg = 0.;
    QTest::newRow( "rot 0 c" )
        << QTransform().rotate(deg)
        << QRect( 10, 20, 30, 40 )
        << QPolygon ( QRect( 10, 20, 30, 40 ) );
    deg = 0.00001f;
    QTest::newRow( "rot 0 d" )
        << QTransform().rotate(deg)
        << QRect( 10, 20, 30, 40 )
        << QPolygon ( QRect( 10, 20, 30, 40 ) );

    // rotations
    deg = -90.f;
    QTest::newRow( "rotscale 90 a" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 0, 0, 30, 40 )
        << QPolygon( QRect( 0, -300, 400, 300 ) );
    deg = -90.00001f;
    QTest::newRow( "rotscale 90 b" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 0, 0, 30, 40 )
        << QPolygon( QRect( 0, -300, 400, 300 ) );
    deg = -90.f;
    QTest::newRow( "rotscale 90 c" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( 200, -400, 400, 300 ) );
    deg = -90.00001f;
    QTest::newRow( "rotscale 90 d" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( 200, -400, 400, 300 ) );

    deg = 180.f;
    QTest::newRow( "rotscale 180 a" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 0, 0, 30, 40 )
        << QPolygon( QRect( -300, -400, 300, 400 ) );
    deg = 180.000001f;
    QTest::newRow( "rotscale 180 b" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 0, 0, 30, 40 )
        << QPolygon( QRect( -300, -400, 300, 400 ) );
    deg = 180.f;
    QTest::newRow( "rotscale 180 c" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( -400, -600, 300, 400 ) );
    deg = 180.000001f;
    QTest::newRow( "rotscale 180 d" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( -400, -600, 300, 400 ) );

    deg = -270.f;
    QTest::newRow( "rotscale 270 a" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 0, 0, 30, 40 )
        << QPolygon( QRect( -400, 0, 400, 300 ) );
    deg = -270.0000001f;
    QTest::newRow( "rotscale 270 b" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 0, 0, 30, 40 )
        << QPolygon( QRect( -400, 0, 400, 300 ) );
    deg = -270.f;
    QTest::newRow( "rotscale 270 c" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( -600, 100, 400, 300 ) );
    deg = -270.000001f;
    QTest::newRow( "rotscale 270 d" )
        << QTransform().rotate(deg).scale(10, 10)
        << QRect( 10, 20, 30, 40 )
        << QPolygon( QRect( -600, 100, 400, 300 ) );
}

void tst_QTransform::mapRect()
{
    QFETCH( QTransform, matrix );
    QFETCH( QRect, src );
    QFETCH( QPolygon, res );
    QRect mapped = matrix.mapRect(src);
    QCOMPARE( mapped, res.boundingRect().adjusted(0, 0, -1, -1) );

    QRectF r = matrix.mapRect(QRectF(src));
    QRect ir(r.topLeft().toPoint(), r.bottomRight().toPoint() - QPoint(1, 1));
    QCOMPARE( mapped, ir );
}

void tst_QTransform::assignments()
{
    QTransform m;
    m.scale(2, 3);
    m.rotate(45);
    m.shear(4, 5);

    QTransform c1(m);

    QCOMPARE(m.m11(), c1.m11());
    QCOMPARE(m.m12(), c1.m12());
    QCOMPARE(m.m21(), c1.m21());
    QCOMPARE(m.m22(), c1.m22());
    QCOMPARE(m.dx(), c1.dx());
    QCOMPARE(m.dy(), c1.dy());

    QTransform c2 = m;
    QCOMPARE(m.m11(), c2.m11());
    QCOMPARE(m.m12(), c2.m12());
    QCOMPARE(m.m21(), c2.m21());
    QCOMPARE(m.m22(), c2.m22());
    QCOMPARE(m.dx(),  c2.dx());
    QCOMPARE(m.dy(),  c2.dy());
}


void tst_QTransform::mapToPolygon()
{
    QFETCH( QTransform, matrix );
    QFETCH( QRect, src );
    QFETCH( QPolygon, res );

    QPolygon poly = matrix.mapToPolygon(src);

    // don't care about starting point
    bool equal = false;
    for (int i = 0; i < poly.size(); ++i) {
        QPolygon rot;
        for (int j = i; j < poly.size(); ++j)
            rot << poly[j];
        for (int j = 0; j < i; ++j)
            rot << poly[j];
        if (rot == res)
            equal = true;
    }

    QVERIFY(equal);
}

void tst_QTransform::qhash()
{
    QMatrix m1;
    m1.shear(3.0, 2.0);
    m1.rotate(44);

    QMatrix m2 = m1;

    QTransform t1(m1);
    QTransform t2(m2);

    // not really much to test here, so just the bare minimum:
    QCOMPARE(qHash(m1), qHash(m2));
    QCOMPARE(qHash(t1), qHash(t2));
}


void tst_QTransform::translate()
{
    QTransform m( 1, 2, 3, 4, 5, 6 );
    QTransform res2( m );
    QTransform res( 1, 2, 3, 4, 75, 106 );
    m.translate( 10,  20 );
    QVERIFY( m == res );
    m.translate( -10,  -20 );
    QVERIFY( m == res2 );
    QVERIFY( QTransform::fromTranslate( 0, 0 ).type() == QTransform::TxNone );
    QVERIFY( QTransform::fromTranslate( 10, 0 ).type() == QTransform::TxTranslate );
    QVERIFY( QTransform::fromTranslate( -1, 5 ) == QTransform().translate( -1, 5 ));
    QVERIFY( QTransform::fromTranslate( 0, 0 ) == QTransform());
}

void tst_QTransform::scale()
{
    QTransform m( 1, 2, 3, 4, 5, 6 );
    QTransform res2( m );
    QTransform res( 10, 20, 60, 80, 5, 6 );
    m.scale( 10,  20 );
    QVERIFY( m == res );
    m.scale( 1./10.,  1./20. );
    QVERIFY( m == res2 );
    QVERIFY( QTransform::fromScale( 1, 1 ).type() == QTransform::TxNone );
    QVERIFY( QTransform::fromScale( 2, 4 ).type() == QTransform::TxScale );
    QVERIFY( QTransform::fromScale( 2, 4 ) == QTransform().scale( 2, 4 ));
    QVERIFY( QTransform::fromScale( 1, 1 ) == QTransform());
}

void tst_QTransform::matrix()
{
    QMatrix mat1;
    mat1.scale(0.3, 0.7);
    mat1.translate(53.3, 94.4);
    mat1.rotate(45);

    QMatrix mat2;
    mat2.rotate(33);
    mat2.scale(0.6, 0.6);
    mat2.translate(13.333, 7.777);

    QTransform tran1(mat1);
    QTransform tran2(mat2);
    QTransform dummy;
    dummy.setMatrix(mat1.m11(), mat1.m12(), 0,
                    mat1.m21(), mat1.m22(), 0,
                    mat1.dx(), mat1.dy(), 1);

    QCOMPARE(tran1, dummy);
    QCOMPARE(tran1.inverted(), dummy.inverted());
    QCOMPARE(tran1.inverted(), QTransform(mat1.inverted()));
    QCOMPARE(tran2.inverted(), QTransform(mat2.inverted()));

    QMatrix mat3 = mat1 * mat2;
    QTransform tran3 = tran1 * tran2;
    QCOMPARE(QTransform(mat3), tran3);

    /* QMatrix::operator==() doesn't use qFuzzyCompare(), which
     * on win32-g++ results in a failure. So we work around it by
     * calling QTranform::operator==(), which performs a fuzzy compare. */
    QCOMPARE(QTransform(mat3), QTransform(tran3.toAffine()));

    QTransform tranInv = tran1.inverted();
    QMatrix   matInv = mat1.inverted();

    QRect rect(43, 70, 200, 200);
    QPoint pt(43, 66);
    QCOMPARE(tranInv.map(pt), matInv.map(pt));
    QCOMPARE(tranInv.map(pt), matInv.map(pt));

    QPainterPath path;
    path.moveTo(55, 60);
    path.lineTo(110, 110);
    path.quadTo(220, 50, 10, 20);
    path.closeSubpath();
    QCOMPARE(tranInv.map(path), matInv.map(path));
}

void tst_QTransform::testOffset()
{
    QTransform trans;
    const QMatrix &aff = trans.toAffine();
    QCOMPARE((void*)(&aff), (void*)(&trans));
}

void tst_QTransform::types()
{
    QTransform m1;
    QCOMPARE(m1.type(), QTransform::TxNone);

    m1.translate(1.0f, 0.0f);
    QCOMPARE(m1.type(), QTransform::TxTranslate);
    QCOMPARE(m1.inverted().type(), QTransform::TxTranslate);

    m1.scale(1.0f, 2.0f);
    QCOMPARE(m1.type(), QTransform::TxScale);
    QCOMPARE(m1.inverted().type(), QTransform::TxScale);

    m1.rotate(45.0f);
    QCOMPARE(m1.type(), QTransform::TxRotate);
    QCOMPARE(m1.inverted().type(), QTransform::TxRotate);

    m1.shear(0.5f, 0.25f);
    QCOMPARE(m1.type(), QTransform::TxShear);
    QCOMPARE(m1.inverted().type(), QTransform::TxShear);

    m1.rotate(45.0f, Qt::XAxis);
    QCOMPARE(m1.type(), QTransform::TxProject);
    m1.shear(0.5f, 0.25f);
    QCOMPARE(m1.type(), QTransform::TxProject);
    m1.rotate(45.0f);
    QCOMPARE(m1.type(), QTransform::TxProject);
    m1.scale(1.0f, 2.0f);
    QCOMPARE(m1.type(), QTransform::TxProject);
    m1.translate(1.0f, 0.0f);
    QCOMPARE(m1.type(), QTransform::TxProject);

    QTransform m2(1.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f,
                  -1.0f, -1.0f, 1.0f);

    QCOMPARE(m2.type(), QTransform::TxTranslate);
    QCOMPARE((m1 * m2).type(), QTransform::TxProject);

    m1 *= QTransform();
    QCOMPARE(m1.type(), QTransform::TxProject);

    m1 *= QTransform(1.0f, 0.0f, 0.0f,
                     0.0f, 1.0f, 0.0f,
                     1.0f, 0.0f, 1.0f);
    QCOMPARE(m1.type(), QTransform::TxProject);

    m2.reset();
    QCOMPARE(m2.type(), QTransform::TxNone);

    m2.setMatrix(1.0f, 0.0f, 0.0f,
                 0.0f, 1.0f, 0.0f,
                 0.0f, 0.0f, 1.0f);
    QCOMPARE(m2.type(), QTransform::TxNone);

    m2 *= QTransform();
    QCOMPARE(m2.type(), QTransform::TxNone);

    m2.setMatrix(2.0f, 0.0f, 0.0f,
                 0.0f, 1.0f, 0.0f,
                 0.0f, 0.0f, 1.0f);
    QCOMPARE(m2.type(), QTransform::TxScale);
    m2 *= QTransform();
    QCOMPARE(m2.type(), QTransform::TxScale);

    m2.setMatrix(0.0f, 1.0f, 0.0f,
                 1.0f, 0.0f, 0.0f,
                 0.0f, 1.0f, 1.0f);
    QCOMPARE(m2.type(), QTransform::TxRotate);
    m2 *= QTransform();
    QCOMPARE(m2.type(), QTransform::TxRotate);

    m2.setMatrix(1.0f, 0.0f, 0.5f,
                 0.0f, 1.0f, 0.0f,
                 0.0f, 0.0f, 1.0f);
    QCOMPARE(m2.type(), QTransform::TxProject);
    m2 *= QTransform();
    QCOMPARE(m2.type(), QTransform::TxProject);

    m2.setMatrix(1.0f, 1.0f, 0.0f,
                 1.0f, 0.0f, 0.0f,
                 0.0f, 1.0f, 1.0f);
    QCOMPARE(m2.type(), QTransform::TxShear);

    m2 *= m2.inverted();
    QCOMPARE(m2.type(), QTransform::TxNone);

    m2.translate(5.0f, 5.0f);
    m2.rotate(45.0f);
    m2.rotate(-45.0f);
    QCOMPARE(m2.type(), QTransform::TxTranslate);

    m2.scale(2.0f, 3.0f);
    m2.shear(1.0f, 0.0f);
    m2.shear(-1.0f, 0.0f);
    QCOMPARE(m2.type(), QTransform::TxScale);

    m2 *= QTransform(1.0f, 1.0f, 0.0f,
                     0.0f, 1.0f, 0.0f,
                     0.0f, 0.0f, 1.0f);
    QCOMPARE(m2.type(), QTransform::TxShear);

    m2 *= QTransform(1.0f, 0.0f, 0.0f,
                     0.0f, 1.0f, 0.0f,
                     1.0f, 0.0f, 1.0f);
    QCOMPARE(m2.type(), QTransform::TxShear);

    QTransform m3(1.8f, 0.0f, 0.0f,
                  0.0f, 1.8f, 0.0f,
                  0.0f, 0.0f, 1.0f);

    QCOMPARE(m3.type(), QTransform::TxScale);
    m3.translate(5.0f, 5.0f);
    QCOMPARE(m3.type(), QTransform::TxScale);
    QCOMPARE(m3.inverted().type(), QTransform::TxScale);

    m3.setMatrix(1.0f, 0.0f, 0.0f,
                 0.0f, 1.0f, 0.0f,
                 0.0f, 0.0f, 2.0f);
    QCOMPARE(m3.type(), QTransform::TxProject);

    m3.setMatrix(0.0f, 2.0f, 0.0f,
                 1.0f, 0.0f, 0.0f,
                 0.0f, 0.0f, 2.0f);
    QCOMPARE(m3.type(), QTransform::TxProject);

    QTransform m4;
    m4.scale(5, 5);
    m4.translate(4, 2);
    m4.rotate(45);

    QCOMPARE(m4.type(), QTransform::TxRotate);

    QTransform m5;
    m5.scale(5, 5);
    m5 = m5.adjoint() / m5.determinant();
    QCOMPARE(m5.type(), QTransform::TxScale);
}

void tst_QTransform::types2_data()
{
    QTest::addColumn<QTransform>("t1");
    QTest::addColumn<QTransform::TransformationType>("type");

    QTest::newRow( "identity" ) << QTransform() << QTransform::TxNone;
    QTest::newRow( "translate" ) << QTransform().translate(10, -0.1) << QTransform::TxTranslate;
    QTest::newRow( "scale" ) << QTransform().scale(10, -0.1) << QTransform::TxScale;
    QTest::newRow( "rotate" ) << QTransform().rotate(10) << QTransform::TxRotate;
    QTest::newRow( "shear" ) << QTransform().shear(10, -0.1) << QTransform::TxShear;
    QTest::newRow( "project" ) << QTransform().rotate(10, Qt::XAxis) << QTransform::TxProject;
    QTest::newRow( "combined" ) << QTransform().translate(10, -0.1).scale(10, -0.1).rotate(10, Qt::YAxis) << QTransform::TxProject;
}

void tst_QTransform::types2()
{
#define CHECKTXTYPE(func) { QTransform t2(func); \
                            QTransform t3(t2.m11(), t2.m12(), t2.m13(), t2.m21(), t2.m22(), t2.m23(), t2.m31(), t2.m32(), t2.m33()); \
                            QVERIFY2(t3.type() == t2.type(), #func); \
                          }

    QFETCH( QTransform, t1 );
    QFETCH( QTransform::TransformationType, type );

    Q_ASSERT(t1.type() == type);

    CHECKTXTYPE(t1.adjoint());
    CHECKTXTYPE(t1.inverted());
    CHECKTXTYPE(t1.transposed());

#undef CHECKTXTYPE
}

void tst_QTransform::scalarOps()
{
    QTransform t;
    QCOMPARE(t.m11(), 1.);
    QCOMPARE(t.m33(), 1.);
    QCOMPARE(t.m21(), 0.);

    t = QTransform() + 3;
    QCOMPARE(t.m11(), 4.);
    QCOMPARE(t.m33(), 4.);
    QCOMPARE(t.m21(), 3.);

    t = t - 3;
    QCOMPARE(t.m11(), 1.);
    QCOMPARE(t.m33(), 1.);
    QCOMPARE(t.m21(), 0.);
    QCOMPARE(t.isIdentity(), true);

    t += 3;
    t = t * 2;
    QCOMPARE(t.m11(), 8.);
    QCOMPARE(t.m33(), 8.);
    QCOMPARE(t.m21(), 6.);
}

void tst_QTransform::transform()
{
    QTransform t;
    t.rotate(30, Qt::YAxis);
    t.translate(15, 10);
    t.scale(2, 2);
    t.rotate(30);
    t.shear(0.5, 0.5);

    QTransform a, b, c, d, e;
    a.rotate(30, Qt::YAxis);
    b.translate(15, 10);
    c.scale(2, 2);
    d.rotate(30);
    e.shear(0.5, 0.5);

    QVERIFY(qFuzzyCompare(t, e * d * c * b * a));
}

void tst_QTransform::mapEmptyPath()
{
    QPainterPath path;
    path.moveTo(10, 10);
    path.lineTo(10, 10);
    QCOMPARE(QTransform().map(path), path);
}

void tst_QTransform::boundingRect()
{
    QPainterPath path;
    path.moveTo(10, 10);
    path.lineTo(10, 10);
    QCOMPARE(path.boundingRect(), QRectF(10, 10, 0, 0));
}

void tst_QTransform::controlPointRect()
{
    QPainterPath path;
    path.moveTo(10, 10);
    path.lineTo(10, 10);
    QCOMPARE(path.controlPointRect(), QRectF(10, 10, 0, 0));
}

void tst_QTransform::inverted_data()
{
    QTest::addColumn<QTransform>("matrix");

    QTest::newRow("identity")
        << QTransform();

    QTest::newRow("TxTranslate")
        << QTransform().translate(200, 10);

    QTest::newRow("TxScale")
        << QTransform().scale(5, 2);

    QTest::newRow("TxTranslate TxScale")
        << QTransform().translate(100, -10).scale(40, 2);

    QTest::newRow("TxScale TxTranslate")
        << QTransform().scale(40, 2).translate(100, -10);

    QTest::newRow("TxRotate")
        << QTransform().rotate(40, Qt::ZAxis);

    QTest::newRow("TxRotate TxScale")
        << QTransform().rotate(60, Qt::ZAxis).scale(2, 0.25);

    QTest::newRow("TxScale TxRotate")
        << QTransform().scale(2, 0.25).rotate(30, Qt::ZAxis);

    QTest::newRow("TxRotate TxScale TxTranslate")
        << QTransform().rotate(60, Qt::ZAxis).scale(2, 0.25).translate(200, -3000);

    QTest::newRow("TxRotate TxTranslate TxScale")
        << QTransform().rotate(60, Qt::ZAxis).translate(200, -3000).scale(19, 77);

    QTest::newRow("TxShear")
        << QTransform().shear(10, 10);

    QTest::newRow("TxShear TxRotate")
        << QTransform().shear(10, 10).rotate(45, Qt::ZAxis);

    QTest::newRow("TxShear TxRotate TxScale")
        << QTransform().shear(10, 10).rotate(45, Qt::ZAxis).scale(19, 81);

    QTest::newRow("TxTranslate TxShear TxRotate TxScale")
        << QTransform().translate(150, -1).shear(10, 10).rotate(45, Qt::ZAxis).scale(19, 81);

    const qreal s = 500000;

    QTransform big;
    big.scale(s, s);

    QTest::newRow("big") << big;

    QTransform smallTransform;
    smallTransform.scale(1/s, 1/s);

    QTest::newRow("small") << smallTransform;
}

void tst_QTransform::inverted()
{
    if (sizeof(qreal) != sizeof(double))
        QSKIP("precision error if qreal is not double");

    QFETCH(QTransform, matrix);

    const QTransform inverted = matrix.inverted();

    QCOMPARE(matrix.isIdentity(), inverted.isIdentity());
    QCOMPARE(matrix.type(), inverted.type());

    QVERIFY((matrix * inverted).isIdentity());
    QVERIFY((inverted * matrix).isIdentity());
}

void tst_QTransform::projectivePathMapping()
{
    QPainterPath path;
    path.addRect(-50, -50, 100, 100);

    const QRectF view(0, 0, 1024, 1024);

    QVERIFY(view.intersects(path.boundingRect()));

    for (int i = 0; i < 85; i += 5) {
        QTransform transform;
        transform.translate(512, 512);
        transform.rotate(i, Qt::YAxis);

        const QPainterPath mapped = transform.map(path);

        QVERIFY(view.intersects(mapped.boundingRect()));
        QVERIFY(transform.inverted().mapRect(view).intersects(path.boundingRect()));
    }
}

void tst_QTransform::mapInt()
{
    int x = 0;
    int y = 0;

    QTransform::fromTranslate(10, 10).map(x, y, &x, &y);

    QCOMPARE(x, 10);
    QCOMPARE(y, 10);
}

void tst_QTransform::mapPathWithPoint()
{
    QPainterPath p(QPointF(10, 10));
    p = QTransform::fromTranslate(10, 10).map(p);
    QCOMPARE(p.currentPosition(), QPointF(20, 20));
}

QTEST_APPLESS_MAIN(tst_QTransform)


#include "tst_qtransform.moc"
