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

#include <qpolygon.h>
#include <qpainterpath.h>
#include <math.h>

#include <qpainter.h>

class tst_QPolygon : public QObject
{
    Q_OBJECT

public:
    tst_QPolygon();

private slots:
    void boundingRect_data();
    void boundingRect();
    void boundingRectF_data();
    void boundingRectF();
    void makeEllipse();
    void swap();
    void intersections_data();
    void intersections();
};

tst_QPolygon::tst_QPolygon()
{
}

void tst_QPolygon::boundingRect_data()
{
    QTest::addColumn<QPolygon>("poly");
    QTest::addColumn<QRect>("brect");

#define ROW(args, rect) \
    do { \
        QPolygon poly; \
        poly.setPoints args; \
        QTest::newRow(#args) << poly << QRect rect; \
    } while (0)

    QTest::newRow("empty") << QPolygon() << QRect(0, 0, 0, 0);
    ROW((1,  0,1),             ( 0, 1, 1, 1));
    ROW((2,  0,1,  1,0),       ( 0, 0, 2, 2));
    ROW((3, -1,1, -1,-1, 1,0), (-1,-1, 3, 3));
#undef ROW
}

void tst_QPolygon::boundingRect()
{
    QFETCH(QPolygon, poly);
    QFETCH(QRect, brect);

    QCOMPARE(poly.boundingRect(), brect);
}

namespace {
struct MyPolygonF : QPolygonF
{
    // QPolygonF doesn't have setPoints...
    void setPoints(int nPoints, int firstx, int firsty, ...) {
        va_list ap;
        reserve(nPoints);
        *this << QPointF(firstx, firsty);
        va_start(ap, firsty);
        while (--nPoints) {
            const int x = va_arg(ap, int);
            const int y = va_arg(ap, int);
            *this << QPointF(x, y);
        }
        va_end(ap);
    }
};
}

void tst_QPolygon::boundingRectF_data()
{
    QTest::addColumn<QPolygonF>("poly");
    QTest::addColumn<QRectF>("brect");

#define ROW(args, rect) \
    do { \
        MyPolygonF poly; \
        poly.setPoints args; \
        QTest::newRow(#args) << QPolygonF(poly) << QRectF rect; \
    } while (0)

    QTest::newRow("empty") << QPolygonF() << QRectF(0, 0, 0, 0);
    ROW((1,  0,1),             ( 0, 1, 0, 0));
    ROW((2,  0,1,  1,0),       ( 0, 0, 1, 1));
    ROW((3, -1,1, -1,-1, 1,0), (-1,-1, 2, 2));
#undef ROW
}

void tst_QPolygon::boundingRectF()
{
    QFETCH(QPolygonF, poly);
    QFETCH(QRectF, brect);

    QCOMPARE(poly.boundingRect(), brect);
}

void tst_QPolygon::makeEllipse()
{
    // create an ellipse with R1 = R2 = R, i.e. a circle
    QPolygon pa;
    const int R = 50; // radius
    QPainterPath path;
    path.addEllipse(0, 0, 2*R, 2*R);
    pa = path.toSubpathPolygons().at(0).toPolygon();

    int i;
    // make sure that all points are R+-1 away from the center
    bool err = false;
    for (i = 1; i < pa.size(); i++) {
        QPoint p = pa.at(i);
        double r = sqrt(pow(double(p.x() - R), 2.0) + pow(double(p.y() - R), 2.0));
        // ### too strict ? at least from visual inspection it looks
        // quite odd around the main axes. 2.0 passes easily.
        err |= (qAbs(r - double(R)) > 2.0);
    }
    QVERIFY( !err );
}

void tst_QPolygon::swap()
{
    QPolygon p1(QVector<QPoint>() << QPoint(0,0) << QPoint(10,10) << QPoint(-10,10));
    QPolygon p2(QVector<QPoint>() << QPoint(0,0) << QPoint( 0,10) << QPoint( 10,10) << QPoint(10,0));
    p1.swap(p2);
    QCOMPARE(p1.count(),4);
    QCOMPARE(p2.count(),3);
}

void tst_QPolygon::intersections_data()
{
    QTest::addColumn<QPolygon>("poly1");
    QTest::addColumn<QPolygon>("poly2");
    QTest::addColumn<bool>("result");

    QTest::newRow("empty intersects nothing")
            << QPolygon()
            << QPolygon(QVector<QPoint>() << QPoint(0,0) << QPoint(10,10) << QPoint(-10,10))
            << false;
    QTest::newRow("identical triangles")
            << QPolygon(QVector<QPoint>() << QPoint(0,0) << QPoint(10,10) << QPoint(-10,10))
            << QPolygon(QVector<QPoint>() << QPoint(0,0) << QPoint(10,10) << QPoint(-10,10))
            << true;
    QTest::newRow("not intersecting")
            << QPolygon(QVector<QPoint>() << QPoint(0,0) << QPoint(10,10) << QPoint(-10,10))
            << QPolygon(QVector<QPoint>() << QPoint(0,20) << QPoint(10,12) << QPoint(-10,12))
            << false;
    QTest::newRow("clean intersection of squares")
            << QPolygon(QVector<QPoint>() << QPoint(0,0) << QPoint(0,10) << QPoint(10,10) << QPoint(10,0))
            << QPolygon(QVector<QPoint>() << QPoint(5,5) << QPoint(5,15) << QPoint(15,15) << QPoint(15,5))
            << true;
    QTest::newRow("clean contains of squares")
            << QPolygon(QVector<QPoint>() << QPoint(0,0) << QPoint(0,10) << QPoint(10,10) << QPoint(10,0))
            << QPolygon(QVector<QPoint>() << QPoint(5,5) << QPoint(5,8) << QPoint(8,8) << QPoint(8,5))
            << true;
}

void tst_QPolygon::intersections()
{
    QFETCH(QPolygon, poly1);
    QFETCH(QPolygon, poly2);
    QFETCH(bool, result);

    QCOMPARE(poly2.intersects(poly1), poly1.intersects(poly2));
    QCOMPARE(poly2.intersected(poly1).isEmpty(), poly1.intersected(poly2).isEmpty());
    QCOMPARE(!poly1.intersected(poly2).isEmpty(), poly1.intersects(poly2));
    QCOMPARE(poly1.intersects(poly2), result);
}

QTEST_APPLESS_MAIN(tst_QPolygon)
#include "tst_qpolygon.moc"
