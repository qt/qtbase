// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include <qpolygon.h>
#include <qpainterpath.h>
#include <math.h>

#include <qpainter.h>

class tst_QPolygon : public QObject
{
    Q_OBJECT

private slots:
    void constructors();
    void toPolygonF();
    void boundingRect_data();
    void boundingRect();
    void boundingRectF_data();
    void boundingRectF();
    void makeEllipse();
    void swap();
    void intersections_data();
    void intersections();
};

void constructors_helper(QPolygon) {}
void constructors_helperF(QPolygonF) {}

void tst_QPolygon::constructors()
{
    constructors_helper(QPolygon());
    constructors_helper({});
    constructors_helper({ QPoint(1, 2), QPoint(3, 4)});
    constructors_helper({ {1, 2}, {3, 4} });
    constructors_helper(QPolygon(12));
    QList<QPoint> pointList;
    constructors_helper(pointList);
    constructors_helper(std::move(pointList));
    constructors_helper(QRect(1, 2, 3, 4));
    const int points[2] = { 10, 20 };
    constructors_helper(QPolygon(1, points));

    constructors_helperF(QPolygonF());
    constructors_helperF({});
    constructors_helperF({ QPointF(1, 2), QPointF(3, 4)});
    constructors_helperF({ {1, 2}, {3, 4} });
    constructors_helperF(QPolygonF(12));
    constructors_helperF(QPolygon());
    QList<QPointF> pointFList;
    constructors_helperF(pointFList);
    constructors_helperF(std::move(pointFList));
    constructors_helperF(QRectF(1, 2, 3, 4));
}

void tst_QPolygon::toPolygonF()
{
    const QPolygon p = {{1, 1}, {-1, 1}, {-1, -1}, {1, -1}};
    auto pf = p.toPolygonF();
    static_assert(std::is_same_v<decltype(pf), QPolygonF>);
    QCOMPARE(pf.size(), p.size());
    auto p2 = pf.toPolygon();
    static_assert(std::is_same_v<decltype(p2), QPolygon>);
    QCOMPARE(p, p2);
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
    QPolygon p1(QList<QPoint>() << QPoint(0, 0) << QPoint(10, 10) << QPoint(-10, 10));
    QPolygon p2(QList<QPoint>() << QPoint(0, 0) << QPoint(0, 10) << QPoint(10, 10)
                                << QPoint(10, 0));
    p1.swap(p2);
    QCOMPARE(p1.size(),4);
    QCOMPARE(p2.size(),3);
}

void tst_QPolygon::intersections_data()
{
    QTest::addColumn<QPolygon>("poly1");
    QTest::addColumn<QPolygon>("poly2");
    QTest::addColumn<bool>("result");

    QTest::newRow("empty intersects nothing")
            << QPolygon()
            << QPolygon(QList<QPoint>() << QPoint(0, 0) << QPoint(10, 10) << QPoint(-10, 10))
            << false;
    QTest::newRow("identical triangles")
            << QPolygon(QList<QPoint>() << QPoint(0, 0) << QPoint(10, 10) << QPoint(-10, 10))
            << QPolygon(QList<QPoint>() << QPoint(0, 0) << QPoint(10, 10) << QPoint(-10, 10))
            << true;
    QTest::newRow("not intersecting")
            << QPolygon(QList<QPoint>() << QPoint(0, 0) << QPoint(10, 10) << QPoint(-10, 10))
            << QPolygon(QList<QPoint>() << QPoint(0, 20) << QPoint(10, 12) << QPoint(-10, 12))
            << false;
    QTest::newRow("clean intersection of squares")
            << QPolygon(QList<QPoint>()
                        << QPoint(0, 0) << QPoint(0, 10) << QPoint(10, 10) << QPoint(10, 0))
            << QPolygon(QList<QPoint>()
                        << QPoint(5, 5) << QPoint(5, 15) << QPoint(15, 15) << QPoint(15, 5))
            << true;
    QTest::newRow("clean contains of squares")
            << QPolygon(QList<QPoint>()
                        << QPoint(0, 0) << QPoint(0, 10) << QPoint(10, 10) << QPoint(10, 0))
            << QPolygon(QList<QPoint>()
                        << QPoint(5, 5) << QPoint(5, 8) << QPoint(8, 8) << QPoint(8, 5))
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
