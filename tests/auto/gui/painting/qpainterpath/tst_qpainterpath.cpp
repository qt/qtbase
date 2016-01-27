/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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


#include <QtTest/QtTest>

#include <qfile.h>
#include <qpainterpath.h>
#include <qpen.h>
#include <qmath.h>

class tst_QPainterPath : public QObject
{
    Q_OBJECT

public:
public slots:
    void cleanupTestCase();
private slots:
    void getSetCheck();
    void swap();

    void contains_QPointF_data();
    void contains_QPointF();

    void contains_QRectF_data();
    void contains_QRectF();

    void intersects_QRectF_data();
    void intersects_QRectF();

    void testContainsAndIntersects_data();
    void testContainsAndIntersects();

    void testSimplified_data();
    void testSimplified();

    void testStroker_data();
    void testStroker();

    void currentPosition();

    void testOperatorEquals();
    void testOperatorEquals_fuzzy();
    void testOperatorDatastream();

    void testArcMoveTo_data();
    void testArcMoveTo();
    void setElementPositionAt();

    void testOnPath_data();
    void testOnPath();

    void pointAtPercent_data();
    void pointAtPercent();

    void angleAtPercent();

    void arcWinding_data();
    void arcWinding();

    void testToFillPolygons();

    void testNaNandInfinites();

    void closing();

    void operators_data();
    void operators();

    void connectPathDuplicatePoint();
    void connectPathMoveTo();

    void translate();

    void lineWithinBounds();
};

void tst_QPainterPath::cleanupTestCase()
{
    QFile::remove(QLatin1String("data"));
}

// Testing get/set functions
void tst_QPainterPath::getSetCheck()
{
    QPainterPathStroker obj1;
    // qreal QPainterPathStroker::width()
    // void QPainterPathStroker::setWidth(qreal)
    obj1.setWidth(0.0);
    QCOMPARE(qreal(1.0), obj1.width()); // Pathstroker sets with to 1 if <= 0
    obj1.setWidth(0.5);
    QCOMPARE(qreal(0.5), obj1.width());
    obj1.setWidth(1.1);
    QCOMPARE(qreal(1.1), obj1.width());

    // qreal QPainterPathStroker::miterLimit()
    // void QPainterPathStroker::setMiterLimit(qreal)
    obj1.setMiterLimit(0.0);
    QCOMPARE(qreal(0.0), obj1.miterLimit());
    obj1.setMiterLimit(1.1);
    QCOMPARE(qreal(1.1), obj1.miterLimit());

    // qreal QPainterPathStroker::curveThreshold()
    // void QPainterPathStroker::setCurveThreshold(qreal)
    obj1.setCurveThreshold(0.0);
    QCOMPARE(qreal(0.0), obj1.curveThreshold());
    obj1.setCurveThreshold(1.1);
    QCOMPARE(qreal(1.1), obj1.curveThreshold());
}

void tst_QPainterPath::swap()
{
    QPainterPath p1;
    p1.addRect( 0, 0,10,10);
    QPainterPath p2;
    p2.addRect(10,10,10,10);
    p1.swap(p2);
    QCOMPARE(p1.boundingRect().toRect(), QRect(10,10,10,10));
    QCOMPARE(p2.boundingRect().toRect(), QRect( 0, 0,10,10));
}

Q_DECLARE_METATYPE(QPainterPath)

void tst_QPainterPath::currentPosition()
{
    QPainterPath p;

    QCOMPARE(p.currentPosition(), QPointF());

    p.moveTo(100, 100);
    QCOMPARE(p.currentPosition(), QPointF(100, 100));

    p.lineTo(200, 200);
    QCOMPARE(p.currentPosition(), QPointF(200, 200));

    p.cubicTo(300, 200, 200, 300, 500, 500);
    QCOMPARE(p.currentPosition(), QPointF(500, 500));
}

void tst_QPainterPath::contains_QPointF_data()
{
    QTest::addColumn<QPainterPath>("path");
    QTest::addColumn<QPointF>("pt");
    QTest::addColumn<bool>("contained");

    QPainterPath path;
    path.addRect(0, 0, 100, 100);

    // #####
    // #   #
    // #   #
    // #   #
    // #####

    QTest::newRow("[0,0] in [0,0,100,100]") << path << QPointF(0, 0) << true;

    QTest::newRow("[99,0] in [0,0,100,100]") << path << QPointF(99, 0) << true;
    QTest::newRow("[0,99] in [0,0,100,100]") << path << QPointF(0, 99) << true;
    QTest::newRow("[99,99] in [0,0,100,100]") << path << QPointF(99, 99) << true;

    QTest::newRow("[99.99,0] in [0,0,100,100]") << path << QPointF(99.99, 0) << true;
    QTest::newRow("[0,99.99] in [0,0,100,100]") << path << QPointF(0, 99.99) << true;
    QTest::newRow("[99.99,99.99] in [0,0,100,100]") << path << QPointF(99.99, 99.99) << true;

    QTest::newRow("[0.01,0.01] in [0,0,100,100]") << path << QPointF(0.01, 0.01) << true;
    QTest::newRow("[0,0.01] in [0,0,100,100]") << path << QPointF(0, 0.01) << true;
    QTest::newRow("[0.01,0] in [0,0,100,100]") << path << QPointF(0.01, 0) << true;

    QTest::newRow("[-0.01,-0.01] in [0,0,100,100]") << path << QPointF(-0.01, -0.01) << false;
    QTest::newRow("[-0,-0.01] in [0,0,100,100]") << path << QPointF(0, -0.01) << false;
    QTest::newRow("[-0.01,0] in [0,0,100,100]") << path << QPointF(-0.01, 0) << false;


    QTest::newRow("[-10,0] in [0,0,100,100]") << path << QPointF(-10, 0) << false;
    QTest::newRow("[100,0] in [0,0,100,100]") << path << QPointF(100, 0) << false;

    QTest::newRow("[0,-10] in [0,0,100,100]") << path << QPointF(0, -10) << false;
    QTest::newRow("[0,100] in [0,0,100,100]") << path << QPointF(0, 100) << false;

    QTest::newRow("[100.1,0] in [0,0,100,100]") << path << QPointF(100.1, 0) << false;
    QTest::newRow("[0,100.1] in [0,0,100,100]") << path << QPointF(0, 100.1) << false;

    path.addRect(50, 50, 100, 100);

    // #####
    // #   #
    // # #####
    // # # # #
    // ##### #
    //   #   #
    //   #####

    QTest::newRow("[49,49] in 2 rects") << path << QPointF(49,49) << true;
    QTest::newRow("[50,50] in 2 rects") << path << QPointF(50,50) << false;
    QTest::newRow("[100,100] in 2 rects") << path << QPointF(100,100) << true;

    path.setFillRule(Qt::WindingFill);
    QTest::newRow("[50,50] in 2 rects (winding)") << path << QPointF(50,50) << true;

    path.addEllipse(0, 0, 150, 150);

    // #####
    // ##  ##
    // # #####
    // # # # #
    // ##### #
    //  ##  ##
    //   #####

    QTest::newRow("[50,50] in complex (winding)") << path << QPointF(50, 50) << true;

    path.setFillRule(Qt::OddEvenFill);
    QTest::newRow("[50,50] in complex (windinf)") << path << QPointF(50, 50) << true;
    QTest::newRow("[49,49] in complex") << path << QPointF(49,49) << false;
    QTest::newRow("[100,100] in complex") << path << QPointF(49,49) << false;


    // unclosed triangle
    path = QPainterPath();
    path.moveTo(100, 100);
    path.lineTo(130, 70);
    path.lineTo(150, 110);

    QTest::newRow("[100,100] in triangle") << path << QPointF(100, 100) << true;
    QTest::newRow("[140,100] in triangle") << path << QPointF(140, 100) << true;
    QTest::newRow("[130,80] in triangle") << path << QPointF(130, 80) << true;

    QTest::newRow("[110,80] in triangle") << path << QPointF(110, 80) << false;
    QTest::newRow("[150,100] in triangle") << path << QPointF(150, 100) << false;
    QTest::newRow("[120,110] in triangle") << path << QPointF(120, 110) << false;

    QRectF base_rect(0, 0, 20, 20);

    path = QPainterPath();
    path.addEllipse(base_rect);

    // not strictly precise, but good enougth to verify fair precision.
    QPainterPath inside;
    inside.addEllipse(base_rect.adjusted(5, 5, -5, -5));
    QPolygonF inside_poly = inside.toFillPolygon();
    for (int i=0; i<inside_poly.size(); ++i)
        QTest::newRow(qPrintable(QString("inside_ellipse %1").arg(i))) << path << inside_poly.at(i) << true;

    QPainterPath outside;
    outside.addEllipse(base_rect.adjusted(-5, -5, 5, 5));
    QPolygonF outside_poly = outside.toFillPolygon();
    for (int i=0; i<outside_poly.size(); ++i)
        QTest::newRow(qPrintable(QString("outside_ellipse %1").arg(i))) << path << outside_poly.at(i) << false;

    path = QPainterPath();
    base_rect = QRectF(50, 50, 200, 200);
    path.addEllipse(base_rect);
    path.setFillRule(Qt::WindingFill);

    QTest::newRow("topleft outside ellipse") << path << base_rect.topLeft() << false;
    QTest::newRow("topright outside ellipse") << path << base_rect.topRight() << false;
    QTest::newRow("bottomright outside ellipse") << path << base_rect.bottomRight() << false;
    QTest::newRow("bottomleft outside ellipse") << path << base_rect.bottomLeft() << false;

    // Test horizontal curve segment
    path = QPainterPath();
    path.moveTo(100, 100);
    path.cubicTo(120, 100, 180, 100, 200, 100);
    path.lineTo(150, 200);
    path.closeSubpath();

    QTest::newRow("horizontal cubic, out left") << path << QPointF(0, 100) << false;
    QTest::newRow("horizontal cubic, out right") << path << QPointF(300, 100) <<false;
    QTest::newRow("horizontal cubic, in mid") << path << QPointF(150, 100) << true;

    path = QPainterPath();
    path.addEllipse(QRectF(-5000.0, -5000.0, 1500000.0, 1500000.0));
    QTest::newRow("huge ellipse, qreal=float crash") << path << QPointF(1100000.35, 1098000.2) << true;

}

void tst_QPainterPath::contains_QPointF()
{
    QFETCH(QPainterPath, path);
    QFETCH(QPointF, pt);
    QFETCH(bool, contained);

    QCOMPARE(path.contains(pt), contained);
}

void tst_QPainterPath::contains_QRectF_data()
{
    QTest::addColumn<QPainterPath>("path");
    QTest::addColumn<QRectF>("rect");
    QTest::addColumn<bool>("contained");

    QPainterPath path;
    path.addRect(0, 0, 100, 100);

    QTest::newRow("same rect") << path << QRectF(0.1, 0.1, 99, 99) << true; // ###
    QTest::newRow("outside") << path << QRectF(-1, -1, 100, 100) << false;
    QTest::newRow("covers") << path << QRectF(-1, -1, 102, 102) << false;
    QTest::newRow("left") << path << QRectF(-10, 50, 5, 5) << false;
    QTest::newRow("top") << path << QRectF(50, -10, 5, 5) << false;
    QTest::newRow("right") << path << QRectF(110, 50, 5, 5) << false;
    QTest::newRow("bottom") << path << QRectF(50, 110, 5, 5) << false;

    path.addRect(50, 50, 100, 100);

    QTest::newRow("r1 top") << path << QRectF(0.1, 0.1, 99, 49) << true;
    QTest::newRow("r1 left") << path << QRectF(0.1, 0.1, 49, 99) << true;
    QTest::newRow("r2 right") << path << QRectF(100.01, 50.1, 49, 99) << true;
    QTest::newRow("r2 bottom") << path << QRectF(50.1, 100.1, 99, 49) << true;
    QTest::newRow("inside 2 rects") << path << QRectF(51, 51, 48, 48) << false;
    QTest::newRow("topRight 2 rects") << path << QRectF(100, 0, 49, 49) << false;
    QTest::newRow("bottomLeft 2 rects") << path << QRectF(0, 100, 49, 49) << false;

    path.setFillRule(Qt::WindingFill);
    QTest::newRow("inside 2 rects (winding)") << path << QRectF(51, 51, 48, 48) << true;

    path.addEllipse(0, 0, 150, 150);
    QTest::newRow("topRight 2 rects") << path << QRectF(100, 25, 24, 24) << true;
    QTest::newRow("bottomLeft 2 rects") << path << QRectF(25, 100, 24, 24) << true;

    path.setFillRule(Qt::OddEvenFill);
    QTest::newRow("inside 2 rects") << path << QRectF(50, 50, 49, 49) << false;
}

void tst_QPainterPath::contains_QRectF()
{
    QFETCH(QPainterPath, path);
    QFETCH(QRectF, rect);
    QFETCH(bool, contained);

    QCOMPARE(path.contains(rect), contained);
}

static inline QPainterPath rectPath(qreal x, qreal y, qreal w, qreal h)
{
    QPainterPath path;
    path.addRect(x, y, w, h);
    path.closeSubpath();
    return path;
}

static inline QPainterPath ellipsePath(qreal x, qreal y, qreal w, qreal h)
{
    QPainterPath path;
    path.addEllipse(x, y, w, h);
    path.closeSubpath();
    return path;
}

static inline QPainterPath linePath(qreal x1, qreal y1, qreal x2, qreal y2)
{
    QPainterPath path;
    path.moveTo(x1, y1);
    path.lineTo(x2, y2);
    return path;
}

void tst_QPainterPath::intersects_QRectF_data()
{
    QTest::addColumn<QPainterPath>("path");
    QTest::addColumn<QRectF>("rect");
    QTest::addColumn<bool>("intersects");

    QPainterPath path;
    path.addRect(0, 0, 100, 100);

    QTest::newRow("same rect") << path << QRectF(0.1, 0.1, 99, 99) << true; // ###
    QTest::newRow("outside") << path << QRectF(-1, -1, 100, 100) << true;
    QTest::newRow("covers") << path << QRectF(-1, -1, 102, 102) << true;
    QTest::newRow("left") << path << QRectF(-10, 50, 5, 5) << false;
    QTest::newRow("top") << path << QRectF(50, -10, 5, 5) << false;
    QTest::newRow("right") << path << QRectF(110, 50, 5, 5) << false;
    QTest::newRow("bottom") << path << QRectF(50, 110, 5, 5) << false;

    path.addRect(50, 50, 100, 100);

    QTest::newRow("r1 top") << path << QRectF(0.1, 0.1, 99, 49) << true;
    QTest::newRow("r1 left") << path << QRectF(0.1, 0.1, 49, 99) << true;
    QTest::newRow("r2 right") << path << QRectF(100.01, 50.1, 49, 99) << true;
    QTest::newRow("r2 bottom") << path << QRectF(50.1, 100.1, 99, 49) << true;
    QTest::newRow("inside 2 rects") << path << QRectF(51, 51, 48, 48) << false;

    path.setFillRule(Qt::WindingFill);
    QTest::newRow("inside 2 rects (winding)") << path << QRectF(51, 51, 48, 48) << true;

    path.addEllipse(0, 0, 150, 150);
    QTest::newRow("topRight 2 rects") << path << QRectF(100, 25, 24, 24) << true;
    QTest::newRow("bottomLeft 2 rects") << path << QRectF(25, 100, 24, 24) << true;

    QTest::newRow("horizontal line") << linePath(0, 0, 10, 0) << QRectF(1, -1, 2, 2) << true;
    QTest::newRow("vertical line") << linePath(0, 0, 0, 10) << QRectF(-1, 1, 2, 2) << true;

    path = QPainterPath();
    path.addEllipse(QRectF(-5000.0, -5000.0, 1500000.0, 1500000.0));
    QTest::newRow("huge ellipse, qreal=float crash") << path << QRectF(1100000.35, 1098000.2, 1500000.0, 1500000.0) << true;
}

void tst_QPainterPath::intersects_QRectF()
{
    QFETCH(QPainterPath, path);
    QFETCH(QRectF, rect);
    QFETCH(bool, intersects);

    QCOMPARE(path.intersects(rect), intersects);
}


void tst_QPainterPath::testContainsAndIntersects_data()
{
    QTest::addColumn<QPainterPath>("path");
    QTest::addColumn<QPainterPath>("candidate");
    QTest::addColumn<bool>("contained");
    QTest::addColumn<bool>("intersects");

    QTest::newRow("rect vs small ellipse (upper left)") << rectPath(0, 0, 100, 100) << ellipsePath(0, 0, 50, 50) << false << true;
    QTest::newRow("rect vs small ellipse (upper right)") << rectPath(0, 0, 100, 100) << ellipsePath(50, 0, 50, 50) << false << true;
    QTest::newRow("rect vs small ellipse (lower right)") << rectPath(0, 0, 100, 100) << ellipsePath(50, 50, 50, 50) << false << true;
    QTest::newRow("rect vs small ellipse (lower left)") << rectPath(0, 0, 100, 100) << ellipsePath(0, 50, 50, 50) << false << true;
    QTest::newRow("rect vs small ellipse (centered)") << rectPath(0, 0, 100, 100) << ellipsePath(25, 25, 50, 50) << true << true;
    QTest::newRow("rect vs equal ellipse") << rectPath(0, 0, 100, 100) << ellipsePath(0, 0, 100, 100) << false << true;
    QTest::newRow("rect vs big ellipse") << rectPath(0, 0, 100, 100) << ellipsePath(-10, -10, 120, 120) << false << true;

    QPainterPath twoEllipses = ellipsePath(0, 0, 100, 100).united(ellipsePath(200, 0, 100, 100));

    QTest::newRow("rect vs two small ellipses") << rectPath(0, 0, 100, 100) << ellipsePath(25, 25, 50, 50).united(ellipsePath(225, 25, 50, 50)) << false << true;
    QTest::newRow("rect vs two equal ellipses") << rectPath(0, 0, 100, 100) << twoEllipses << false << true;

    QTest::newRow("rect vs self") << rectPath(0, 0, 100, 100) << rectPath(0, 0, 100, 100) << false << true;
    QTest::newRow("ellipse vs self") << ellipsePath(0, 0, 100, 100) << ellipsePath(0, 0, 100, 100) << false << true;

    QPainterPath twoRects = rectPath(0, 0, 100, 100).united(rectPath(200, 0, 100, 100));
    QTest::newRow("two rects vs small ellipse (upper left)") << twoRects << ellipsePath(0, 0, 50, 50) << false << true;
    QTest::newRow("two rects vs small ellipse (upper right)") << twoRects << ellipsePath(50, 0, 50, 50) << false << true;
    QTest::newRow("two rects vs small ellipse (lower right)") << twoRects << ellipsePath(50, 50, 50, 50) << false << true;
    QTest::newRow("two rects vs small ellipse (lower left)") << twoRects << ellipsePath(0, 50, 50, 50) << false << true;
    QTest::newRow("two rects vs small ellipse (centered)") << twoRects << ellipsePath(25, 25, 50, 50) << true << true;
    QTest::newRow("two rects vs equal ellipse") << twoRects << ellipsePath(0, 0, 100, 100) << false << true;
    QTest::newRow("two rects vs big ellipse") << twoRects << ellipsePath(-10, -10, 120, 120) << false << true;

    QTest::newRow("two rects vs two small ellipses") << twoRects << ellipsePath(25, 25, 50, 50).united(ellipsePath(225, 25, 50, 50)) << true << true;
    QTest::newRow("two rects vs two equal ellipses") << twoRects << ellipsePath(0, 0, 100, 100).united(ellipsePath(200, 0, 100, 100)) << false << true;

    QTest::newRow("two rects vs self") << twoRects << twoRects << false << true;
    QTest::newRow("two ellipses vs self") << twoEllipses << twoEllipses << false << true;

    QPainterPath windingRect = rectPath(0, 0, 100, 100);
    windingRect.addRect(25, 25, 100, 50);
    windingRect.setFillRule(Qt::WindingFill);

    QTest::newRow("rect with winding rule vs tall rect") << windingRect << rectPath(40, 20, 20, 60) << true << true;
    QTest::newRow("rect with winding rule vs self") << windingRect << windingRect << false << true;

    QPainterPath thickFrame = rectPath(0, 0, 100, 100).subtracted(rectPath(25, 25, 50, 50));
    QPainterPath thinFrame = rectPath(10, 10, 80, 80).subtracted(rectPath(15, 15, 70, 70));

    QTest::newRow("thin frame in thick frame") << thickFrame << thinFrame << true << true;
    QTest::newRow("rect in thick frame") << thickFrame << rectPath(40, 40, 20, 20) << false << false;
    QTest::newRow("rect in thin frame") << thinFrame << rectPath(40, 40, 20, 20) << false << false;

    QPainterPath ellipses;
    ellipses.addEllipse(0, 0, 10, 10);
    ellipses.addEllipse(4, 4, 2, 2);
    ellipses.setFillRule(Qt::WindingFill);

    // the definition of QPainterPath::intersects() and contains() is fill-area based,
    QTest::newRow("line in rect") << rectPath(0, 0, 100, 100) << linePath(10, 10, 90, 90) << true << true;
    QTest::newRow("horizontal line in rect") << rectPath(0, 0, 100, 100) << linePath(10, 50, 90, 50) << true << true;
    QTest::newRow("vertical line in rect") << rectPath(0, 0, 100, 100) << linePath(50, 10, 50, 90) << true << true;

    QTest::newRow("line through rect") << rectPath(0, 0, 100, 100) << linePath(-10, -10, 110, 110) << false << true;
    QTest::newRow("line through rect 2") << rectPath(0, 0, 100, 100) << linePath(-10, 0, 110, 100) << false << true;
    QTest::newRow("line through rect 3") << rectPath(0, 0, 100, 100) << linePath(5, 10, 110, 100) << false << true;
    QTest::newRow("line through rect 4") << rectPath(0, 0, 100, 100) << linePath(-10, 0, 90, 90) << false << true;

    QTest::newRow("horizontal line through rect") << rectPath(0, 0, 100, 100) << linePath(-10, 50, 110, 50) << false << true;
    QTest::newRow("vertical line through rect") << rectPath(0, 0, 100, 100) << linePath(50, -10, 50, 110) << false << true;

    QTest::newRow("line vs line") << linePath(0, 0, 10, 10) << linePath(10, 0, 0, 10) << false << true;

    QTest::newRow("line in rect with hole") << rectPath(0, 0, 10, 10).subtracted(rectPath(2, 2, 6, 6)) << linePath(4, 4, 6, 6) << false << false;
    QTest::newRow("line in ellipse") << ellipses << linePath(3, 5, 7, 5) << false << true;
    QTest::newRow("line in ellipse 2") << ellipses << linePath(4.5, 5, 5.5, 5) << true << true;

    QTest::newRow("winding ellipse") << ellipses << ellipsePath(4, 4, 2, 2) << false << true;
    QTest::newRow("winding ellipse 2") << ellipses << ellipsePath(4.5, 4.5, 1, 1) << true << true;
    ellipses.setFillRule(Qt::OddEvenFill);
    QTest::newRow("odd even ellipse") << ellipses << ellipsePath(4, 4, 2, 2) << false << true;
    QTest::newRow("odd even ellipse 2") << ellipses << ellipsePath(4.5, 4.5, 1, 1) << false << false;
}

void tst_QPainterPath::testContainsAndIntersects()
{
    QFETCH(QPainterPath, path);
    QFETCH(QPainterPath, candidate);
    QFETCH(bool, contained);
    QFETCH(bool, intersects);

    QCOMPARE(path.intersects(candidate), intersects);
    QCOMPARE(path.contains(candidate), contained);
}

void tst_QPainterPath::testSimplified_data()
{
    QTest::addColumn<QPainterPath>("path");
    QTest::addColumn<int>("elements");

    QTest::newRow("rect") << rectPath(0, 0, 10, 10) << 5;

    QPainterPath twoRects = rectPath(0, 0, 10, 10);
    twoRects.addPath(rectPath(5, 0, 10, 10));
    QTest::newRow("two rects (odd)") << twoRects << 10;

    twoRects.setFillRule(Qt::WindingFill);
    QTest::newRow("two rects (winding)") << twoRects << 5;

    QPainterPath threeSteps = rectPath(0, 0, 10, 10);
    threeSteps.addPath(rectPath(0, 10, 20, 10));
    threeSteps.addPath(rectPath(0, 20, 30, 10));

    QTest::newRow("three rects (steps)") << threeSteps << 9;
}

void tst_QPainterPath::testSimplified()
{
    QFETCH(QPainterPath, path);
    QFETCH(int, elements);

    QPainterPath simplified = path.simplified();

    QCOMPARE(simplified.elementCount(), elements);

    QVERIFY(simplified.subtracted(path).isEmpty());
    QVERIFY(path.subtracted(simplified).isEmpty());
}

void tst_QPainterPath::testStroker_data()
{
    QTest::addColumn<QPainterPath>("path");
    QTest::addColumn<QPen>("pen");
    QTest::addColumn<QPainterPath>("stroke");

    QTest::newRow("line 1") << linePath(2, 2, 10, 2) << QPen(Qt::black, 2, Qt::SolidLine, Qt::FlatCap) << rectPath(2, 1, 8, 2);
    QTest::newRow("line 2") << linePath(2, 2, 10, 2) << QPen(Qt::black, 2, Qt::SolidLine, Qt::SquareCap) << rectPath(1, 1, 10, 2);

    QTest::newRow("rect") << rectPath(1, 1, 8, 8) << QPen(Qt::black, 2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin) << rectPath(0, 0, 10, 10).subtracted(rectPath(2, 2, 6, 6));

    QTest::newRow("dotted line") << linePath(0, 0, 10, 0) << QPen(Qt::black, 2, Qt::DotLine) << rectPath(-1, -1, 4, 2).united(rectPath(5, -1, 4, 2));
}

void tst_QPainterPath::testStroker()
{
    QFETCH(QPainterPath, path);
    QFETCH(QPen, pen);
    QFETCH(QPainterPath, stroke);

    QPainterPathStroker stroker;
    stroker.setWidth(pen.widthF());
    stroker.setCapStyle(pen.capStyle());
    stroker.setJoinStyle(pen.joinStyle());
    stroker.setMiterLimit(pen.miterLimit());
    stroker.setDashPattern(pen.style());
    stroker.setDashOffset(pen.dashOffset());

    QPainterPath result = stroker.createStroke(path);

    // check if stroke == result
    QVERIFY(result.subtracted(stroke).isEmpty());
    QVERIFY(stroke.subtracted(result).isEmpty());
}

void tst_QPainterPath::testOperatorEquals()
{
    QPainterPath empty1;
    QPainterPath empty2;
    QCOMPARE(empty1, empty2);

    QPainterPath rect1;
    rect1.addRect(100, 100, 100, 100);
    QCOMPARE(rect1, rect1);
    QVERIFY(rect1 != empty1);

    QPainterPath rect2;
    rect2.addRect(100, 100, 100, 100);
    QCOMPARE(rect1, rect2);

    rect2.setFillRule(Qt::WindingFill);
    QVERIFY(rect1 != rect2);

    QPainterPath ellipse1;
    ellipse1.addEllipse(50, 50, 100, 100);
    QVERIFY(rect1 != ellipse1);

    QPainterPath ellipse2;
    ellipse2.addEllipse(50, 50, 100, 100);
    QCOMPARE(ellipse1, ellipse2);
}

void tst_QPainterPath::testOperatorEquals_fuzzy()
{
    // if operator== returns true for two paths it should
    // also return true when the same transform is applied to both paths
    {
        QRectF a(100, 100, 100, 50);
        QRectF b = a.translated(1e-14, 1e-14);

        QPainterPath pa;
        pa.addRect(a);
        QPainterPath pb;
        pb.addRect(b);

        QCOMPARE(pa, pb);

        QTransform transform;
        transform.translate(-100, -100);

        QCOMPARE(transform.map(pa), transform.map(pb));
    }

    // higher tolerance for error when path's bounding rect is big
    {
        QRectF a(1, 1, 1e6, 0.5e6);
        QRectF b = a.translated(1e-7, 1e-7);

        QPainterPath pa;
        pa.addRect(a);
        QPainterPath pb;
        pb.addRect(b);

        QCOMPARE(pa, pb);

        QTransform transform;
        transform.translate(-1, -1);

        QCOMPARE(transform.map(pa), transform.map(pb));
    }

    // operator== should return true for a path that has
    // been transformed and then inverse transformed
    {
        QPainterPath a;
        a.addRect(0, 0, 100, 100);

        QTransform transform;
        transform.translate(100, 20);
        transform.scale(1.5, 1.5);

        QPainterPath b = transform.inverted().map(transform.map(a));

        QCOMPARE(a, b);
    }

    {
        QPainterPath a;
        a.lineTo(10, 0);
        a.lineTo(10, 10);
        a.lineTo(0, 10);

        QPainterPath b;
        b.lineTo(10, 0);
        b.moveTo(10, 10);
        b.lineTo(0, 10);

        QVERIFY(a != b);
    }
}

void tst_QPainterPath::testOperatorDatastream()
{
    QPainterPath path;
    path.addEllipse(0, 0, 100, 100);
    path.addRect(0, 0, 100, 100);
    path.setFillRule(Qt::WindingFill);

    // Write out
    {
        QFile data("data");
        bool ok = data.open(QFile::WriteOnly);
        QVERIFY(ok);
        QDataStream stream(&data);
        stream << path;
    }

    QPainterPath other;
    // Read in
    {
        QFile data("data");
        bool ok = data.open(QFile::ReadOnly);
        QVERIFY(ok);
        QDataStream stream(&data);
        stream >> other;
    }

    QCOMPARE(other, path);
}

void tst_QPainterPath::closing()
{
    // lineto's
    {
        QPainterPath triangle(QPoint(100, 100));

        triangle.lineTo(200, 100);
        triangle.lineTo(200, 200);
        QCOMPARE(triangle.elementCount(), 3);

        //add this line to make sure closeSubpath() also calls detach() and detached properly
        QPainterPath copied = triangle;
        triangle.closeSubpath();
        QCOMPARE(copied.elementCount(), 3);

        QCOMPARE(triangle.elementCount(), 4);
        QCOMPARE(triangle.elementAt(3).type, QPainterPath::LineToElement);

        triangle.moveTo(300, 300);
        QCOMPARE(triangle.elementCount(), 5);
        QCOMPARE(triangle.elementAt(4).type, QPainterPath::MoveToElement);

        triangle.lineTo(400, 300);
        triangle.lineTo(400, 400);
        QCOMPARE(triangle.elementCount(), 7);

        triangle.closeSubpath();
        QCOMPARE(triangle.elementCount(), 8);

        // this will should trigger implicit moveto...
        triangle.lineTo(600, 300);
        QCOMPARE(triangle.elementCount(), 10);
        QCOMPARE(triangle.elementAt(8).type, QPainterPath::MoveToElement);
        QCOMPARE(triangle.elementAt(9).type, QPainterPath::LineToElement);

        triangle.lineTo(600, 700);
        QCOMPARE(triangle.elementCount(), 11);
    }

    // curveto's
    {
        QPainterPath curves(QPoint(100, 100));

        curves.cubicTo(200, 100, 100, 200, 200, 200);
        QCOMPARE(curves.elementCount(), 4);

        curves.closeSubpath();
        QCOMPARE(curves.elementCount(), 5);
        QCOMPARE(curves.elementAt(4).type, QPainterPath::LineToElement);

        curves.moveTo(300, 300);
        QCOMPARE(curves.elementCount(), 6);
        QCOMPARE(curves.elementAt(5).type, QPainterPath::MoveToElement);

        curves.cubicTo(400, 300, 300, 400, 400, 400);
        QCOMPARE(curves.elementCount(), 9);

        curves.closeSubpath();
        QCOMPARE(curves.elementCount(), 10);

        // should trigger implicit moveto..
        curves.cubicTo(100, 800, 800, 100, 800, 800);
        QCOMPARE(curves.elementCount(), 14);
        QCOMPARE(curves.elementAt(10).type, QPainterPath::MoveToElement);
        QCOMPARE(curves.elementAt(11).type, QPainterPath::CurveToElement);
    }

    {
        QPainterPath rects;
        rects.addRect(100, 100, 100, 100);

        QCOMPARE(rects.elementCount(), 5);
        QCOMPARE(rects.elementAt(0).type, QPainterPath::MoveToElement);
        QCOMPARE(rects.elementAt(4).type, QPainterPath::LineToElement);

        rects.addRect(300, 100, 100,100);
        QCOMPARE(rects.elementCount(), 10);
        QCOMPARE(rects.elementAt(5).type, QPainterPath::MoveToElement);
        QCOMPARE(rects.elementAt(9).type, QPainterPath::LineToElement);

        rects.lineTo(0, 0);
        QCOMPARE(rects.elementCount(), 12);
        QCOMPARE(rects.elementAt(10).type, QPainterPath::MoveToElement);
        QCOMPARE(rects.elementAt(11).type, QPainterPath::LineToElement);
    }

    {
        QPainterPath ellipses;
        ellipses.addEllipse(100, 100, 100, 100);

        QCOMPARE(ellipses.elementCount(), 13);
        QCOMPARE(ellipses.elementAt(0).type, QPainterPath::MoveToElement);
        QCOMPARE(ellipses.elementAt(10).type, QPainterPath::CurveToElement);

        ellipses.addEllipse(300, 100, 100,100);
        QCOMPARE(ellipses.elementCount(), 26);
        QCOMPARE(ellipses.elementAt(13).type, QPainterPath::MoveToElement);
        QCOMPARE(ellipses.elementAt(23).type, QPainterPath::CurveToElement);

        ellipses.lineTo(0, 0);
        QCOMPARE(ellipses.elementCount(), 28);
        QCOMPARE(ellipses.elementAt(26).type, QPainterPath::MoveToElement);
        QCOMPARE(ellipses.elementAt(27).type, QPainterPath::LineToElement);
    }

    {
        QPainterPath path;
        path.moveTo(10, 10);
        path.lineTo(40, 10);
        path.lineTo(25, 20);
        path.lineTo(10 + 1e-13, 10 + 1e-13);
        QCOMPARE(path.elementCount(), 4);
        path.closeSubpath();
        QCOMPARE(path.elementCount(), 4);
    }
}

void tst_QPainterPath::testArcMoveTo_data()
{
    QTest::addColumn<QRectF>("rect");
    QTest::addColumn<qreal>("angle");

    QList<QRectF> rects;
    rects << QRectF(100, 100, 100, 100)
          << QRectF(100, 100, -100, 100)
          << QRectF(100, 100, 100, -100)
          << QRectF(100, 100, -100, -100);

    for (int domain=0; domain<rects.size(); ++domain) {
        for (int i=-360; i<=360; ++i) {
            QTest::newRow(qPrintable(QString("test %1 %2").arg(domain).arg(i))) << rects.at(domain) << (qreal) i;
        }

        // test low angles
        QTest::newRow("low angles 1") << rects.at(domain) << (qreal) 1e-10;
        QTest::newRow("low angles 2") << rects.at(domain) << (qreal)-1e-10;
    }
}

void tst_QPainterPath::operators_data()
{
    QTest::addColumn<QPainterPath>("test");
    QTest::addColumn<QPainterPath>("expected");

    QPainterPath a;
    QPainterPath b;
    a.addRect(0, 0, 100, 100);
    b.addRect(50, 50, 100, 100);

    QTest::newRow("a & b") << (a & b) << a.intersected(b);
    QTest::newRow("a | b") << (a | b) << a.united(b);
    QTest::newRow("a + b") << (a + b) << a.united(b);
    QTest::newRow("a - b") << (a - b) << a.subtracted(b);

    QPainterPath c = a;
    QTest::newRow("a &= b") << (a &= b) << a.intersected(b);
    c = a;
    QTest::newRow("a |= b") << (a |= b) << a.united(b);
    c = a;
    QTest::newRow("a += b") << (a += b) << a.united(b);
    c = a;
    QTest::newRow("a -= b") << (a -= b) << a.subtracted(b);
}

void tst_QPainterPath::operators()
{
    QFETCH(QPainterPath, test);
    QFETCH(QPainterPath, expected);

    QCOMPARE(test, expected);
}

static inline bool pathFuzzyCompare(double p1, double p2)
{
    return qAbs(p1 - p2) < 0.001;
}


static inline bool pathFuzzyCompare(float p1, float p2)
{
    return qAbs(p1 - p2) < 0.001;
}


void tst_QPainterPath::testArcMoveTo()
{
    QFETCH(QRectF, rect);
    QFETCH(qreal, angle);

    QPainterPath path;
    path.arcMoveTo(rect, angle);
    path.arcTo(rect, angle, 30);
    path.arcTo(rect, angle + 30, 30);

    QPointF pos = path.elementAt(0);

    QVERIFY((path.elementCount()-1) % 3 == 0);

    qreal x_radius = rect.width() / 2.0;
    qreal y_radius = rect.height() / 2.0;

    QPointF shouldBe = rect.center()
                       + QPointF(x_radius * qCos(qDegreesToRadians(angle)), -y_radius * qSin(qDegreesToRadians(angle)));

    qreal iw = 1 / rect.width();
    qreal ih = 1 / rect.height();

    QVERIFY(pathFuzzyCompare(pos.x() * iw, shouldBe.x() * iw));
    QVERIFY(pathFuzzyCompare(pos.y() * ih, shouldBe.y() * ih));
}

void tst_QPainterPath::testOnPath_data()
{
    QTest::addColumn<QPainterPath>("path");
    QTest::addColumn<qreal>("start");
    QTest::addColumn<qreal>("middle");
    QTest::addColumn<qreal>("end");

    QPainterPath path = QPainterPath(QPointF(153, 199));
    path.cubicTo(QPointF(147, 61), QPointF(414, 18),
                 QPointF(355, 201));

    QTest::newRow("First case") << path
                                << qreal(93.0)
                                << qreal(4.0)
                                << qreal(252.13);

    path = QPainterPath(QPointF(328, 197));
    path.cubicTo(QPointF(150, 50), QPointF(401, 50),
                 QPointF(225, 197));
    QTest::newRow("Second case") << path
                                 << qreal(140.0)
                                 << qreal(0.0)
                                 << qreal(220.0);

    path = QPainterPath(QPointF(328, 197));
    path.cubicTo(QPointF(101 , 153), QPointF(596, 151),
                 QPointF(353, 197));
    QTest::newRow("Third case") << path
                                << qreal(169.0)
                                << qreal(0.22)
                                <<  qreal(191.0);

    path = QPainterPath(QPointF(153, 199));
    path.cubicTo(QPointF(59, 53), QPointF(597, 218),
                  QPointF(355, 201));
    QTest::newRow("Fourth case") << path
                                 << qreal(122.0)
                                 <<  qreal(348.0)
                                 << qreal(175.0);

}

#define SIGN(x) ((x < 0)?-1:1)
void tst_QPainterPath::testOnPath()
{
    QFETCH(QPainterPath, path);
    QFETCH(qreal, start);
    QFETCH(qreal, middle);
    QFETCH(qreal, end);

    int signStart = SIGN(start);
    int signMid   = SIGN(middle);
    int signEnd   = SIGN(end);

    static const qreal diff = 3;

    qreal angle = path.angleAtPercent(0);
    QCOMPARE(SIGN(angle), signStart);
    QVERIFY(qAbs(angle-start) < diff);

    angle = path.angleAtPercent(0.5);
    QCOMPARE(SIGN(angle), signMid);
    QVERIFY(qAbs(angle-middle) < diff);

    angle = path.angleAtPercent(1);
    QCOMPARE(SIGN(angle), signEnd);
    QVERIFY(qAbs(angle-end) < diff);
}

void tst_QPainterPath::pointAtPercent_data()
{
    QTest::addColumn<QPainterPath>("path");
    QTest::addColumn<qreal>("percent");
    QTest::addColumn<QPointF>("point");

    QPainterPath path;
    path.lineTo(100, 0);

    QTest::newRow("Case 1") << path << qreal(0.2) << QPointF(20, 0);
    QTest::newRow("Case 2") << path << qreal(0.5) << QPointF(50, 0);
    QTest::newRow("Case 3") << path << qreal(0.0) << QPointF(0, 0);
    QTest::newRow("Case 4") << path << qreal(1.0) << QPointF(100, 0);

    path = QPainterPath();
    path.lineTo(0, 100);

    QTest::newRow("Case 5") << path << qreal(0.2) << QPointF(0, 20);
    QTest::newRow("Case 6") << path << qreal(0.5) << QPointF(0, 50);
    QTest::newRow("Case 7") << path << qreal(0.0) << QPointF(0, 0);
    QTest::newRow("Case 8") << path << qreal(1.0) << QPointF(0, 100);

    path.lineTo(300, 100);

    QTest::newRow("Case 9")  << path << qreal(0.25) << QPointF(0, 100);
    QTest::newRow("Case 10") << path << qreal(0.5) << QPointF(100, 100);
    QTest::newRow("Case 11") << path << qreal(0.75) << QPointF(200, 100);

    path = QPainterPath();
    path.addEllipse(0, 0, 100, 100);

    QTest::newRow("Case 12") << path << qreal(0.0)  << QPointF(100, 50);
    QTest::newRow("Case 13") << path << qreal(0.25) << QPointF(50, 100);
    QTest::newRow("Case 14") << path << qreal(0.5)  << QPointF(0, 50);
    QTest::newRow("Case 15") << path << qreal(0.75) << QPointF(50, 0);
    QTest::newRow("Case 16") << path << qreal(1.0)  << QPointF(100, 50);

    path = QPainterPath();
    QRectF rect(241, 273, 185, 228);
    path.addEllipse(rect);
    QTest::newRow("Case 17") << path << qreal(1.0) << QPointF(rect.right(), qreal(0.5) * (rect.top() + rect.bottom()));

    path = QPainterPath();
    path.moveTo(100, 100);
    QTest::newRow("Case 18") << path << qreal(0.0) << QPointF(100, 100);
    QTest::newRow("Case 19") << path << qreal(1.0) << QPointF(100, 100);
}

void tst_QPainterPath::pointAtPercent()
{
    QFETCH(QPainterPath, path);
    QFETCH(qreal, percent);
    QFETCH(QPointF, point);

    QPointF result = path.pointAtPercent(percent);
    QVERIFY(pathFuzzyCompare(point.x() , result.x()));
    QVERIFY(pathFuzzyCompare(point.y() , result.y()));
}

void tst_QPainterPath::setElementPositionAt()
{
    QPainterPath path(QPointF(42., 42.));
    QCOMPARE(path.elementCount(), 1);
    QCOMPARE(path.elementAt(0).type, QPainterPath::MoveToElement);
    QCOMPARE(path.elementAt(0).x, qreal(42.));
    QCOMPARE(path.elementAt(0).y, qreal(42.));

    QPainterPath copy = path;
    copy.setElementPositionAt(0, qreal(0), qreal(0));
    QCOMPARE(copy.elementCount(), 1);
    QCOMPARE(copy.elementAt(0).type, QPainterPath::MoveToElement);
    QCOMPARE(copy.elementAt(0).x, qreal(0));
    QCOMPARE(copy.elementAt(0).y, qreal(0));

    QCOMPARE(path.elementCount(), 1);
    QCOMPARE(path.elementAt(0).type, QPainterPath::MoveToElement);
    QCOMPARE(path.elementAt(0).x, qreal(42.));
    QCOMPARE(path.elementAt(0).y, qreal(42.));
}

void tst_QPainterPath::angleAtPercent()
{
    for (int angle = 0; angle < 360; ++angle) {
        QLineF line = QLineF::fromPolar(100, angle);
        QPainterPath path;
        path.moveTo(line.p1());
        path.lineTo(line.p2());

        QCOMPARE(path.angleAtPercent(0.5), line.angle());
    }
}

void tst_QPainterPath::arcWinding_data()
{
    QTest::addColumn<QPainterPath>("path");
    QTest::addColumn<QPointF>("point");
    QTest::addColumn<bool>("inside");

    QPainterPath a;
    a.addEllipse(0, 0, 100, 100);
    a.addRect(50, 50, 100, 100);

    QTest::newRow("Case A (oddeven)") << a << QPointF(55, 55) << false;
    a.setFillRule(Qt::WindingFill);
    QTest::newRow("Case A (winding)") << a << QPointF(55, 55) << true;

    QPainterPath b;
    b.arcMoveTo(0, 0, 100, 100, 10);
    b.arcTo(0, 0, 100, 100, 10, 360);
    b.addRect(50, 50, 100, 100);

    QTest::newRow("Case B (oddeven)") << b << QPointF(55, 55) << false;
    b.setFillRule(Qt::WindingFill);
    QTest::newRow("Case B (winding)") << b << QPointF(55, 55) << false;

    QPainterPath c;
    c.arcMoveTo(0, 0, 100, 100, 0);
    c.arcTo(0, 0, 100, 100, 0, 360);
    c.addRect(50, 50, 100, 100);

    QTest::newRow("Case C (oddeven)") << c << QPointF(55, 55) << false;
    c.setFillRule(Qt::WindingFill);
    QTest::newRow("Case C (winding)") << c << QPointF(55, 55) << false;

    QPainterPath d;
    d.arcMoveTo(0, 0, 100, 100, 10);
    d.arcTo(0, 0, 100, 100, 10, -360);
    d.addRect(50, 50, 100, 100);

    QTest::newRow("Case D (oddeven)") << d << QPointF(55, 55) << false;
    d.setFillRule(Qt::WindingFill);
    QTest::newRow("Case D (winding)") << d << QPointF(55, 55) << true;

    QPainterPath e;
    e.arcMoveTo(0, 0, 100, 100, 0);
    e.arcTo(0, 0, 100, 100, 0, -360);
    e.addRect(50, 50, 100, 100);

    QTest::newRow("Case E (oddeven)") << e << QPointF(55, 55) << false;
    e.setFillRule(Qt::WindingFill);
    QTest::newRow("Case E (winding)") << e << QPointF(55, 55) << true;
}

void tst_QPainterPath::arcWinding()
{
    QFETCH(QPainterPath, path);
    QFETCH(QPointF, point);
    QFETCH(bool, inside);

    QCOMPARE(path.contains(point), inside);
}

void tst_QPainterPath::testToFillPolygons()
{
    QPainterPath path;
    path.lineTo(QPointF(0, 50));
    path.lineTo(QPointF(50, 50));

    path.moveTo(QPointF(70, 50));
    path.lineTo(QPointF(70, 100));
    path.lineTo(QPointF(40, 100));

    const QList<QPolygonF> polygons = path.toFillPolygons();
    QCOMPARE(polygons.size(), 2);
    QCOMPARE(polygons.first().count(QPointF(70, 50)), 0);
}

void tst_QPainterPath::testNaNandInfinites()
{
    QPainterPath path1;
    QPainterPath path2 = path1;

    QPointF p1 = QPointF(qSNaN(), 1);
    QPointF p2 = QPointF(qQNaN(), 1);
    QPointF p3 = QPointF(qQNaN(), 1);
    QPointF pInf = QPointF(qInf(), 1);

    // all these operations with NaN/Inf should be ignored
    // can't test operator>> reliably, as we can't create a path with NaN to << later

    path1.moveTo(p1);
    path1.moveTo(qSNaN(), qQNaN());
    path1.moveTo(pInf);

    path1.lineTo(p1);
    path1.lineTo(qSNaN(), qQNaN());
    path1.lineTo(pInf);

    path1.cubicTo(p1, p2, p3);
    path1.cubicTo(p1, QPointF(1, 1), QPointF(2, 2));
    path1.cubicTo(pInf, QPointF(10, 10), QPointF(5, 1));

    path1.quadTo(p1, p2);
    path1.quadTo(QPointF(1, 1), p3);
    path1.quadTo(QPointF(1, 1), pInf);

    path1.arcTo(QRectF(p1, p2), 5, 5);
    path1.arcTo(QRectF(pInf, QPointF(1, 1)), 5, 5);

    path1.addRect(QRectF(p1, p2));
    path1.addRect(QRectF(pInf, QPointF(1, 1)));

    path1.addEllipse(QRectF(p1, p2));
    path1.addEllipse(QRectF(pInf, QPointF(1, 1)));

    QCOMPARE(path1, path2);

    path1.lineTo(QPointF(1, 1));
    QVERIFY(path1 != path2);
}

void tst_QPainterPath::connectPathDuplicatePoint()
{
    QPainterPath a;
    a.moveTo(10, 10);
    a.lineTo(20, 20);

    QPainterPath b;
    b.moveTo(20, 20);
    b.lineTo(30, 10);

    a.connectPath(b);

    QPainterPath c;
    c.moveTo(10, 10);
    c.lineTo(20, 20);
    c.lineTo(30, 10);

    QCOMPARE(c, a);
}

void tst_QPainterPath::connectPathMoveTo()
{
    QPainterPath path1;
    QPainterPath path2;
    QPainterPath path3;
    QPainterPath path4;

    path1.moveTo(1,1);

    path2.moveTo(4,4);
    path2.lineTo(5,6);
    path2.lineTo(6,7);

    path3.connectPath(path2);

    path4.lineTo(5,5);

    path1.connectPath(path2);

    QCOMPARE(path1.elementAt(0).type, QPainterPath::MoveToElement);
    QCOMPARE(path2.elementAt(0).type, QPainterPath::MoveToElement);
    QCOMPARE(path3.elementAt(0).type, QPainterPath::MoveToElement);
    QCOMPARE(path4.elementAt(0).type, QPainterPath::MoveToElement);
}

void tst_QPainterPath::translate()
{
    QPainterPath path;

    // Path with no elements.
    QCOMPARE(path.currentPosition(), QPointF());
    path.translate(50.5, 50.5);
    QCOMPARE(path.currentPosition(), QPointF());
    QCOMPARE(path.translated(50.5, 50.5).currentPosition(), QPointF());

    // path.isEmpty(), but we have one MoveTo element that should be translated.
    path.moveTo(50, 50);
    QCOMPARE(path.currentPosition(), QPointF(50, 50));
    path.translate(99.9, 99.9);
    QCOMPARE(path.currentPosition(), QPointF(149.9, 149.9));
    path.translate(-99.9, -99.9);
    QCOMPARE(path.currentPosition(), QPointF(50, 50));
    QCOMPARE(path.translated(-50, -50).currentPosition(), QPointF(0, 0));

    // Complex path.
    QRegion shape(100, 100, 300, 200, QRegion::Ellipse);
    shape -= QRect(225, 175, 50, 50);
    QPainterPath complexPath;
    complexPath.addRegion(shape);
    QVector<QPointF> untranslatedElements;
    for (int i = 0; i < complexPath.elementCount(); ++i)
        untranslatedElements.append(QPointF(complexPath.elementAt(i)));

    const QPainterPath untranslatedComplexPath(complexPath);
    const QPointF offset(100, 100);
    complexPath.translate(offset);

    for (int i = 0; i < complexPath.elementCount(); ++i)
        QCOMPARE(QPointF(complexPath.elementAt(i)) - offset, untranslatedElements.at(i));

    QCOMPARE(complexPath.translated(-offset), untranslatedComplexPath);
}


void tst_QPainterPath::lineWithinBounds()
{
    const int iteration_count = 3;
    volatile const qreal yVal = 0.5;
    QPointF a(0.0, yVal);
    QPointF b(1000.0, yVal);
    QPointF c(2000.0, yVal);
    QPointF d(3000.0, yVal);
    QPainterPath path;
    path.moveTo(QPointF(0, yVal));
    path.cubicTo(QPointF(1000.0, yVal), QPointF(2000.0, yVal), QPointF(3000.0, yVal));
    for(int i=0; i<=iteration_count; i++) {
        qreal actual = path.pointAtPercent(qreal(i) / iteration_count).y();
        QVERIFY(actual == yVal); // don't use QCOMPARE, don't want fuzzy comparison
    }
}


QTEST_APPLESS_MAIN(tst_QPainterPath)

#include "tst_qpainterpath.moc"
