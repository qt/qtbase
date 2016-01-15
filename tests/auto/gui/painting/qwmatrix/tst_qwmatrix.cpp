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
#include <qmatrix.h>
#include <qmath.h>
#include <qpolygon.h>


class tst_QWMatrix : public QObject
{
    Q_OBJECT

private slots:
    void mapRect_data();
    void mapToPolygon_data();
    void mapRect();
    void operator_star_qwmatrix();
    void assignments();
    void mapToPolygon();
    void translate();
    void scale();
    void mapPolygon();

private:
    void mapping_data();
};

void tst_QWMatrix::mapRect_data()
{
    mapping_data();
}

void tst_QWMatrix::mapToPolygon_data()
{
    mapping_data();
}

void tst_QWMatrix::mapping_data()
{
    //create the testtable instance and define the elements
    QTest::addColumn<QMatrix>("matrix");
    QTest::addColumn<QRect>("src");
    QTest::addColumn<QPolygon>("res");

    //next we fill it with data

    // identity
    QTest::newRow( "identity" ) << QMatrix( 1, 0, 0, 1, 0, 0 )
                                << QRect( 10, 20, 30, 40 )
                                << QPolygon( QRect( 10, 20, 30, 40 ) );
    // scaling
    QTest::newRow( "scale 0" )  << QMatrix( 2, 0, 0, 2, 0, 0 )
                                << QRect( 10, 20, 30, 40 )
                                << QPolygon( QRect( 20, 40, 60, 80 ) );
    QTest::newRow( "scale 1" )  << QMatrix( 10, 0, 0, 10, 0, 0 )
                                << QRect( 10, 20, 30, 40 )
                                << QPolygon( QRect( 100, 200, 300, 400 ) );
    // mirroring
    QTest::newRow( "mirror 0" ) << QMatrix( -1, 0, 0, 1, 0, 0 )
                                << QRect( 10, 20, 30, 40 )
                                << QPolygon( QRect( -40, 20, 30, 40 ) );
    QTest::newRow( "mirror 1" ) << QMatrix( 1, 0, 0, -1, 0, 0 )
                                << QRect( 10, 20, 30, 40 )
                                << QPolygon( QRect( 10, -60, 30, 40 ) );
    QTest::newRow( "mirror 2" ) << QMatrix( -1, 0, 0, -1, 0, 0 )
                                << QRect( 10, 20, 30, 40 )
                                << QPolygon( QRect( -40, -60, 30, 40 ) );
    QTest::newRow( "mirror 3" ) << QMatrix( -2, 0, 0, -2, 0, 0 )
                                << QRect( 10, 20, 30, 40 )
                                << QPolygon( QRect( -80, -120, 60, 80 ) );
    QTest::newRow( "mirror 4" ) << QMatrix( -10, 0, 0, -10, 0, 0 )
                                << QRect( 10, 20, 30, 40 )
                                << QPolygon( QRect( -400, -600, 300, 400 ) );
    QTest::newRow( "mirror 5" ) << QMatrix( -1, 0, 0, 1, 0, 0 )
                                << QRect( 0, 0, 30, 40 )
                                << QPolygon( QRect( -30, 0, 30, 40 ) );
    QTest::newRow( "mirror 6" ) << QMatrix( 1, 0, 0, -1, 0, 0 )
                                << QRect( 0, 0, 30, 40 )
                                << QPolygon( QRect( 0, -40, 30, 40 ) );
    QTest::newRow( "mirror 7" ) << QMatrix( -1, 0, 0, -1, 0, 0 )
                                << QRect( 0, 0, 30, 40 )
                                << QPolygon( QRect( -30, -40, 30, 40 ) );
    QTest::newRow( "mirror 8" ) << QMatrix( -2, 0, 0, -2, 0, 0 )
                                << QRect( 0, 0, 30, 40 )
                                << QPolygon( QRect( -60, -80, 60, 80 ) );
    QTest::newRow( "mirror 9" ) << QMatrix( -10, 0, 0, -10, 0, 0 )
                                << QRect( 0, 0, 30, 40 )
                                << QPolygon( QRect( -300, -400, 300, 400 ) );

#if (defined(Q_OS_WIN) || defined(Q_OS_WINCE)) && !defined(M_PI)
#define M_PI 3.14159265897932384626433832795f
#endif

    // rotations
    float deg = 0.;
    QTest::newRow( "rot 0 a" )  << QMatrix( std::cos( M_PI*deg/180. ), -std::sin( M_PI*deg/180. ),
                                            std::sin( M_PI*deg/180. ),  std::cos( M_PI*deg/180. ), 0, 0 )
                                << QRect( 0, 0, 30, 40 )
                                << QPolygon ( QRect( 0, 0, 30, 40 ) );
    deg = 0.00001f;
    QTest::newRow( "rot 0 b" )  << QMatrix( std::cos( M_PI*deg/180. ), -std::sin( M_PI*deg/180. ),
                                            std::sin( M_PI*deg/180. ),  std::cos( M_PI*deg/180. ), 0, 0 )
                                << QRect( 0, 0, 30, 40 )
                                << QPolygon ( QRect( 0, 0, 30, 40 ) );
    deg = 0.;
    QTest::newRow( "rot 0 c" )  << QMatrix( std::cos( M_PI*deg/180. ), -std::sin( M_PI*deg/180. ),
                                            std::sin( M_PI*deg/180. ),  std::cos( M_PI*deg/180. ), 0, 0 )
                                << QRect( 10, 20, 30, 40 )
                                << QPolygon ( QRect( 10, 20, 30, 40 ) );
    deg = 0.00001f;
    QTest::newRow( "rot 0 d" )  << QMatrix( std::cos( M_PI*deg/180. ), -std::sin( M_PI*deg/180. ),
                                            std::sin( M_PI*deg/180. ),  std::cos( M_PI*deg/180. ), 0, 0 )
                                << QRect( 10, 20, 30, 40 )
                                << QPolygon ( QRect( 10, 20, 30, 40 ) );

#if 0
    // rotations
    deg = 90.;
    QTest::newRow( "rotscale 90 a" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                                  10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0, 0 )
                                      << QRect( 0, 0, 30, 40 )
                                      << QPolygon( QRect( 0, -299, 400, 300 ) );
    deg = 90.00001;
    QTest::newRow( "rotscale 90 b" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                                  10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0, 0 )
                                      << QRect( 0, 0, 30, 40 )
                                      << QPolygon( QRect( 0, -299, 400, 300 ) );
    deg = 90.;
    QTest::newRow( "rotscale 90 c" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                                  10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0, 0 )
                                      << QRect( 10, 20, 30, 40 )
                                      << QPolygon( QRect( 200, -399, 400, 300 ) );
    deg = 90.00001;
    QTest::newRow( "rotscale 90 d" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                                  10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0, 0 )
                                      << QRect( 10, 20, 30, 40 )
                                      << QPolygon( QRect( 200, -399, 400, 300 ) );

    deg = 180.;
    QTest::newRow( "rotscale 180 a" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                                   10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0, 0 )
                                       << QRect( 0, 0, 30, 40 )
                                       << QPolygon( QRect( -299, -399, 300, 400 ) );
    deg = 180.000001;
    QTest::newRow( "rotscale 180 b" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                                   10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0, 0 )
                                       << QRect( 0, 0, 30, 40 )
                                       << QPolygon( QRect( -299, -399, 300, 400 ) );
    deg = 180.;
    QTest::newRow( "rotscale 180 c" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                                   10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0, 0 )
                                       << QRect( 10, 20, 30, 40 )
                                       << QPolygon( QRect( -399, -599, 300, 400 ) );
    deg = 180.000001;
    QTest::newRow( "rotscale 180 d" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                                   10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0, 0 )
                                       << QRect( 10, 20, 30, 40 )
                                       << QPolygon( QRect( -399, -599, 300, 400 ) );

    deg = 270.;
    QTest::newRow( "rotscale 270 a" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                                   10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0, 0 )
                                       << QRect( 0, 0, 30, 40 )
                                       << QPolygon( QRect( -399, 00, 400, 300 ) );
    deg = 270.0000001;
    QTest::newRow( "rotscale 270 b" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                                   10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0, 0 )
                                       << QRect( 0, 0, 30, 40 )
                                       << QPolygon( QRect( -399, 00, 400, 300 ) );
    deg = 270.;
    QTest::newRow( "rotscale 270 c" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                                   10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0, 0 )
                                       << QRect( 10, 20, 30, 40 )
                                       << QPolygon( QRect( -599, 100, 400, 300 ) );
    deg = 270.000001;
    QTest::newRow( "rotscale 270 d" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                                   10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0,   0 )
                                       << QRect( 10, 20, 30, 40 )
                                       << QPolygon( QRect( -599, 100, 400, 300 ) );

    // rotations that are not multiples of 90 degrees. mapRect returns the bounding rect here.
    deg = 45;
    QTest::newRow( "rot 45 a" )  << QMatrix( std::cos( M_PI*deg/180. ), -std::sin( M_PI*deg/180. ),
                                             std::sin( M_PI*deg/180. ),  std::cos( M_PI*deg/180. ), 0, 0 )
                                 << QRect( 0, 0, 10, 10 )
                                 << QPolygon( QRect( 0, -7, 14, 14 ) );
    QTest::newRow( "rot 45 b" )  << QMatrix( std::cos( M_PI*deg/180. ), -std::sin( M_PI*deg/180. ),
                                             std::sin( M_PI*deg/180. ),  std::cos( M_PI*deg/180. ), 0, 0 )
                                 << QRect( 10, 20, 30, 40 )
                                 << QPolygon( QRect( 21, -14, 49, 49 ) );
    QTest::newRow( "rot 45 c" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                             10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0, 0 )
                                 << QRect( 0, 0, 10, 10 )
                                 << QPolygon( QRect( 0, -70, 141, 141 ) );
    QTest::newRow( "rot 45 d" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                             10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0, 0 )
                                 << QRect( 10, 20, 30, 40 )
                                 << QPolygon( QRect( 212, -141, 495, 495 ) );

    deg = -45;
    QTest::newRow( "rot -45 a" )  << QMatrix( std::cos( M_PI*deg/180. ), -std::sin( M_PI*deg/180. ),
                                              std::sin( M_PI*deg/180. ),  std::cos( M_PI*deg/180. ), 0, 0 )
                                  << QRect( 0, 0, 10, 10 )
                                  << QPolygon( QRect( -7, 0, 14, 14 ) );
    QTest::newRow( "rot -45 b" )  << QMatrix( std::cos( M_PI*deg/180. ), -std::sin( M_PI*deg/180. ),
                                              std::sin( M_PI*deg/180. ),  std::cos( M_PI*deg/180. ), 0, 0 )
                                  << QRect( 10, 20, 30, 40 )
                                  << QPolygon( QRect( -35, 21, 49, 49 ) );
    QTest::newRow( "rot -45 c" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                              10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0, 0 )
                                  << QRect( 0, 0, 10, 10 )
                                  << QPolygon( QRect( -70, 0, 141, 141 ) );
    QTest::newRow( "rot -45 d" )  << QMatrix( 10*std::cos( M_PI*deg/180. ), -10*std::sin( M_PI*deg/180. ),
                                              10*std::sin( M_PI*deg/180. ),  10*std::cos( M_PI*deg/180. ), 0, 0 )
                                  << QRect( 10, 20, 30, 40 )
                                  << QPolygon( QRect( -353, 212, 495, 495 ) );
#endif
}

void tst_QWMatrix::mapRect()
{
    QFETCH( QMatrix, matrix );
    QFETCH( QRect, src );
//     qDebug( "got src: %d/%d (%d/%d), matrix=[ %f %f %f %f %f %f ]",
//         src.x(), src.y(), src.width(), src.height(),
//         matrix.m11(), matrix.m12(), matrix.m21(), matrix.m22(), matrix.dx(), matrix.dy() );
    QTEST( QPolygon( matrix.mapRect(src) ), "res" );
}

void tst_QWMatrix::operator_star_qwmatrix()
{
    QMatrix m1( 2, 3, 4, 5, 6, 7 );
    QMatrix m2( 3, 4, 5, 6, 7, 8 );

    QMatrix result1x2( 21, 26, 37, 46, 60, 74 );
    QMatrix result2x1( 22, 29, 34, 45, 52, 68);

    QMatrix product12 = m1*m2;
    QMatrix product21 = m2*m1;

    QVERIFY( product12==result1x2 );
    QVERIFY( product21==result2x1 );
}


void tst_QWMatrix::assignments()
{
    QMatrix m;
    m.scale(2, 3);
    m.rotate(45);
    m.shear(4, 5);

    QMatrix c1(m);

    QCOMPARE(m.m11(), c1.m11());
    QCOMPARE(m.m12(), c1.m12());
    QCOMPARE(m.m21(), c1.m21());
    QCOMPARE(m.m22(), c1.m22());
    QCOMPARE(m.dx(), c1.dx());
    QCOMPARE(m.dy(), c1.dy());

    QMatrix c2 = m;
    QCOMPARE(m.m11(), c2.m11());
    QCOMPARE(m.m12(), c2.m12());
    QCOMPARE(m.m21(), c2.m21());
    QCOMPARE(m.m22(), c2.m22());
    QCOMPARE(m.dx(),  c2.dx());
    QCOMPARE(m.dy(),  c2.dy());
}


void tst_QWMatrix::mapToPolygon()
{
    QFETCH( QMatrix, matrix );
    QFETCH( QRect, src );
    QFETCH( QPolygon, res );

    QCOMPARE( matrix.mapToPolygon( src ), res );
}


void tst_QWMatrix::translate()
{
    QMatrix m( 1, 2, 3, 4, 5, 6 );
    QMatrix res2( m );
    QMatrix res( 1, 2, 3, 4, 75, 106 );
    m.translate( 10,  20 );
    QVERIFY( m == res );
    m.translate( -10,  -20 );
    QVERIFY( m == res2 );
}

void tst_QWMatrix::scale()
{
    QMatrix m( 1, 2, 3, 4, 5, 6 );
    QMatrix res2( m );
    QMatrix res( 10, 20, 60, 80, 5, 6 );
    m.scale( 10,  20 );
    QVERIFY( m == res );
    m.scale( 1./10.,  1./20. );
    QVERIFY( m == res2 );
}

void tst_QWMatrix::mapPolygon()
{
    QPolygon poly;
    poly << QPoint(0, 0) << QPoint(1, 1) << QPoint(100, 1) << QPoint(1, 100) << QPoint(-1, -1) << QPoint(-1000, 1000);

    {
        QMatrix m;
        m.rotate(90);

        // rotating 90 degrees four times should result in original poly
        QPolygon mapped = m.map(m.map(m.map(m.map(poly))));
        QCOMPARE(mapped, poly);

        QMatrix m2;
        m2.scale(10, 10);
        QMatrix m3;
        m3.scale(0.1, 0.1);

        mapped = m3.map(m2.map(poly));
        QCOMPARE(mapped, poly);
    }

    {
        QMatrix m(1, 2, 3, 4, 5, 6);

        QPolygon mapped = m.map(poly);
        for (int i = 0; i < mapped.size(); ++i)
            QCOMPARE(mapped.at(i), m.map(poly.at(i)));
    }
}

QTEST_APPLESS_MAIN(tst_QWMatrix)
#include "tst_qwmatrix.moc"
