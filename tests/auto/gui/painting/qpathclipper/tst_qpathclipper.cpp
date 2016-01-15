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
#include "private/qpathclipper_p.h"
#include "paths.h"
#include "pathcompare.h"

#include <QtTest/QtTest>

#include <qpainterpath.h>
#include <qpolygon.h>
#include <qdebug.h>
#include <qpainter.h>

#include <math.h>

class tst_QPathClipper : public QObject
{
    Q_OBJECT

public:
    tst_QPathClipper();
    virtual ~tst_QPathClipper();

private:
    void clipTest(int subjectIndex, int clipIndex, QPathClipper::Operation op);

    QList<QPainterPath> paths;

public slots:
    void initTestCase();

private slots:
    void testWingedEdge();

    void testComparePaths();

    void clip_data();
    void clip();

    void clip2();
    void clip3();

    void testIntersections();
    void testIntersections2();
    void testIntersections3();
    void testIntersections4();
    void testIntersections5();
    void testIntersections6();
    void testIntersections7();
    void testIntersections8();
    void testIntersections9();

    void zeroDerivativeCurves();

    void task204301_data();
    void task204301();

    void task209056();
    void task251909();

    void qtbug3778();
};

Q_DECLARE_METATYPE(QPainterPath)
Q_DECLARE_METATYPE(QPathClipper::Operation)

tst_QPathClipper::tst_QPathClipper()
{
}

tst_QPathClipper::~tst_QPathClipper()
{
}

void tst_QPathClipper::initTestCase()
{
    paths << Paths::rect();
    paths << Paths::heart();
    paths << Paths::body();
    paths << Paths::mailbox();
    paths << Paths::deer();
    paths << Paths::fire();

    paths << Paths::random1();
    paths << Paths::random2();

    paths << Paths::heart2();
    paths << Paths::rect2();
    paths << Paths::rect3();
    paths << Paths::rect4();
    paths << Paths::rect5();
    paths << Paths::rect6();

    paths << Paths::frame1();
    paths << Paths::frame2();
    paths << Paths::frame3();
    paths << Paths::frame4();

    paths << Paths::triangle1();
    paths << Paths::triangle2();

    paths << Paths::node();
    paths << Paths::interRect();

    paths << Paths::simpleCurve();
    paths << Paths::simpleCurve2();
    paths << Paths::simpleCurve3();

    paths << Paths::bezier1();
    paths << Paths::bezier2();
    paths << Paths::bezier3();
    paths << Paths::bezier4();

    paths << Paths::bezierFlower();
    paths << Paths::lips();
    paths << Paths::clover();
    paths << Paths::ellipses();
    paths << Paths::windingFill();
    paths << Paths::oddEvenFill();
    paths << Paths::squareWithHole();
    paths << Paths::circleWithHole();
    paths << Paths::bezierQuadrant();

    // make sure all the bounding rects are centered at the origin
    for (int i = 0; i < paths.size(); ++i) {
        QRectF bounds = paths[i].boundingRect();

        QMatrix m(1, 0,
                  0, 1,
                  -bounds.center().x(), -bounds.center().y());

        paths[i] = m.map(paths[i]);
    }
}

static QPainterPath samplePath1()
{
    QPainterPath path;
    path.moveTo(QPointF(200, 246.64789));
    path.lineTo(QPointF(200, 206.64789));
    path.lineTo(QPointF(231.42858, 206.64789));
    path.lineTo(QPointF(231.42858, 246.64789));
    path.lineTo(QPointF(200, 246.64789));
    return path;
}

static QPainterPath samplePath2()
{
    QPainterPath path;
    path.moveTo(QPointF(200, 146.64789));
    path.lineTo(QPointF(200, 106.64789));
    path.lineTo(QPointF(231.42858, 106.64789));
    path.lineTo(QPointF(231.42858, 146.64789));
    path.lineTo(QPointF(200, 146.64789));
    return path;
}

static QPainterPath samplePath3()
{
    QPainterPath path;
    path.moveTo(QPointF(231.42858, 80.933609));
    path.lineTo(QPointF(200, 80.933609));
    path.lineTo(QPointF(200, 96.64788999999999));
    path.lineTo(QPointF(231.42858, 96.64788999999999));
    path.lineTo(QPointF(231.42858, 80.933609));
    return path;
}

static QPainterPath samplePath4()
{
    QPainterPath path;
    path.moveTo(QPointF(288.571434, 80.933609));
    path.lineTo(QPointF(431.42858, 80.933609));
    path.lineTo(QPointF(431.42858, 96.64788999999999));
    path.lineTo(QPointF(288.571434, 96.64788999999999));
    path.lineTo(QPointF(288.571434, 80.933609));
    return path;
}

static QPainterPath samplePath5()
{
    QPainterPath path;
    path.moveTo(QPointF(588.571434, 80.933609));
    path.lineTo(QPointF(682.85715, 80.933609));
    path.lineTo(QPointF(682.85715, 96.64788999999999));
    path.lineTo(QPointF(588.571434, 96.64788999999999));
    path.lineTo(QPointF(588.571434, 80.933609));
    return path;
}

static QPainterPath samplePath6()
{
    QPainterPath path;
    path.moveTo(QPointF(588.571434, 80.933609));
    path.lineTo(QPointF(200, 80.933609));
    path.lineTo(QPointF(200, 446.6479));
    path.lineTo(QPointF(682.85715, 446.6479));
    path.lineTo(QPointF(682.85715, 96.64788999999999));
    path.lineTo(QPointF(731.42858, 96.64788999999999));
    path.lineTo(QPointF(731.42858, 56.64788999999999));
    path.lineTo(QPointF(588.571434, 56.64788999999999));
    path.lineTo(QPointF(588.571434, 80.933609));
    return path;
}

static QPainterPath samplePath7()
{
    QPainterPath path;
    path.moveTo(QPointF(682.85715, 206.64789));
    path.lineTo(QPointF(682.85715, 246.64789));
    path.lineTo(QPointF(588.571434, 246.64789));
    path.lineTo(QPointF(588.571434, 206.64789));
    path.lineTo(QPointF(682.85715, 206.64789));
    return path;
}

static QPainterPath samplePath8()
{
    QPainterPath path;
    path.moveTo(QPointF(682.85715, 406.64789));
    path.lineTo(QPointF(682.85715, 446.64789));
    path.lineTo(QPointF(588.571434, 446.64789));
    path.lineTo(QPointF(588.571434, 406.64789));
    path.lineTo(QPointF(682.85715, 406.64789));
    return path;
}

static QPainterPath samplePath9()
{
    QPainterPath path;
    path.moveTo(QPointF(682.85715, 426.64789));
    path.lineTo(QPointF(682.85715, 446.6479));
    path.lineTo(QPointF(568.571434, 446.6479));
    path.lineTo(QPointF(568.571434, 426.64789));
    path.lineTo(QPointF(682.85715, 426.64789));
    return path;
}

static QPainterPath samplePath10()
{
    QPainterPath path;
    path.moveTo(QPointF(511.42858, 446.6479));
    path.lineTo(QPointF(368.571434, 446.6479));
    path.lineTo(QPointF(368.571434, 426.64789));
    path.lineTo(QPointF(511.42858, 426.64789));
    path.lineTo(QPointF(511.42858, 446.6479));
    return path;
}

static QPainterPath samplePath13()
{
    QPainterPath path;
    path.moveTo(QPointF(160, 200));
    path.lineTo(QPointF(100, 200));
    path.lineTo(QPointF(100, 130));
    path.lineTo(QPointF(160, 130));
    path.lineTo(QPointF(160, 200));
    return path;
}

static QPainterPath samplePath14()
{
    QPainterPath path;

    path.moveTo(160, 80);
    path.lineTo(160, 180);
    path.lineTo(100, 180);
    path.lineTo(100, 80);
    path.lineTo(160, 80);
    path.moveTo(160, 80);
    path.lineTo(160, 100);
    path.lineTo(120, 100);
    path.lineTo(120, 80);

    return path;
}

void tst_QPathClipper::clip_data()
{
    //create the testtable instance and define the elements
    QTest::addColumn<QPainterPath>("subject");
    QTest::addColumn<QPainterPath>("clip");
    QTest::addColumn<QPathClipper::Operation>("op");
    QTest::addColumn<QPainterPath>("result");

    //next we fill it with data
    QTest::newRow( "simple1" )  << Paths::frame3()
                                << Paths::frame4()
                                << QPathClipper::BoolAnd
                                << samplePath1();

    QTest::newRow( "simple2" )  << Paths::frame3()
                                << Paths::frame4() * QTransform().translate(0, -100)
                                << QPathClipper::BoolAnd
                                << samplePath2();

    QTest::newRow( "simple3" )  << Paths::frame3()
                                << Paths::frame4() * QTransform().translate(0, -150)
                                << QPathClipper::BoolAnd
                                << samplePath3();

    QTest::newRow( "simple4" )  << Paths::frame3()
                                << Paths::frame4() * QTransform().translate(200, -150)
                                << QPathClipper::BoolAnd
                                << samplePath4();

    QTest::newRow( "simple5" )  << Paths::frame3()
                                << Paths::frame4() * QTransform().translate(500, -150)
                                << QPathClipper::BoolAnd
                                << samplePath5();

    QTest::newRow( "simple6" )  << Paths::frame3()
                                << Paths::frame4() * QTransform().translate(500, -150)
                                << QPathClipper::BoolOr
                                << samplePath6();

    QTest::newRow( "simple7" )  << Paths::frame3()
                                << Paths::frame4() * QTransform().translate(500, 0)
                                << QPathClipper::BoolAnd
                                << samplePath7();

    QTest::newRow( "simple8" )  << Paths::frame3()
                                << Paths::frame4() * QTransform().translate(500, 200)
                                << QPathClipper::BoolAnd
                                << samplePath8();

    QTest::newRow( "simple9" )  << Paths::frame3()
                                << Paths::frame4() * QTransform().translate(480, 220)
                                << QPathClipper::BoolAnd
                                << samplePath9();

    QTest::newRow( "simple10" )  << Paths::frame3()
                                 << Paths::frame4() * QTransform().translate(280, 220)
                                 << QPathClipper::BoolAnd
                                 << samplePath10();

    QTest::newRow( "simple_move_to1" )  << Paths::rect4()
                                       << Paths::rect2() * QTransform().translate(-20, 50)
                                       << QPathClipper::BoolAnd
                                       << samplePath13();

    QTest::newRow( "simple_move_to2" )  << Paths::rect4()
                                        << Paths::rect2() * QTransform().translate(-20, 0)
                                        << QPathClipper::BoolAnd
                                        << samplePath14();
}

// sanity check to make sure comparePaths declared above works
void tst_QPathClipper::testComparePaths()
{
    QPainterPath a;
    QPainterPath b;

    a.addRect(0, 0, 10, 10);
    b.addRect(0, 0, 10.00001, 10.00001);

    QVERIFY(!QPathCompare::comparePaths(a, b));

    b = QPainterPath();
    b.addRect(0, 0, 10.00000000001, 10.00000000001);

    QVERIFY(QPathCompare::comparePaths(a, b));

    b = QPainterPath();
    b.moveTo(10, 0);
    b.lineTo(0, 0);
    b.lineTo(0, 10);
    b.lineTo(10, 10);

    QVERIFY(QPathCompare::comparePaths(a, b));
    b.lineTo(10, 0);
    QVERIFY(QPathCompare::comparePaths(a, b));

    b = QPainterPath();
    b.moveTo(10, 0);
    b.lineTo(0, 10);
    b.lineTo(0, 0);
    b.lineTo(10, 10);

    QVERIFY(!QPathCompare::comparePaths(a, b));
}

void tst_QPathClipper::clip()
{
    if (sizeof(double) != sizeof(qreal)) {
        QSKIP("This test only works for qreal=double, otherwise ends in rounding errors");
    }
    QFETCH( QPainterPath, subject );
    QFETCH( QPainterPath, clip );
    QFETCH( QPathClipper::Operation, op );
    QFETCH( QPainterPath,  result);
    QPathClipper clipper(subject, clip);
    QPainterPath x = clipper.clip(op);

    QVERIFY(QPathCompare::comparePaths(x, result));
}

static inline QPointF randomPointInRect(const QRectF &rect)
{
    qreal rx = qrand() / (RAND_MAX + 1.);
    qreal ry = qrand() / (RAND_MAX + 1.);

    return QPointF(rect.left() + rx * rect.width(),
                   rect.top() + ry * rect.height());
}

void tst_QPathClipper::clipTest(int subjectIndex, int clipIndex, QPathClipper::Operation op)
{
    const QPainterPath &subject = paths[subjectIndex];
    const QPainterPath &clip = paths[clipIndex];
    const int count = 40;

    QRectF bounds = subject.boundingRect().united(clip.boundingRect());

    const qreal adjustX = bounds.width() * 0.01;
    const qreal adjustY = bounds.height() * 0.01;

    // make sure we test some points that are outside both paths as well
    bounds = bounds.adjusted(-adjustX, -adjustY, adjustX, adjustY);

    const int dim = 256;
    const qreal scale = qMin(dim / bounds.width(), dim / bounds.height());

    QPathClipper clipper(subject, clip);
    QPainterPath result = clipper.clip(op);

    // using the image here is a bit of a hacky way to make sure we don't test points that
    // are too close to the path edges to avoid test fails that are due to numerical errors
    QImage img(dim, dim, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x0);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(scale, scale);
    p.translate(-bounds.topLeft());
    p.setPen(QPen(Qt::black, 0));
    p.drawPath(subject);
    p.setPen(QPen(Qt::red, 0));
    p.drawPath(clip);
    p.end();

    for (int i = 0; i < count; ++i) {
        QPointF point;
        QRgb pixel;
        do {
            point = randomPointInRect(bounds);
            const QPointF imagePoint = (point - bounds.topLeft()) * scale;

            pixel = img.pixel(int(imagePoint.x()), int(imagePoint.y()));
        } while (qAlpha(pixel) > 0);

        const bool inSubject = subject.contains(point);
        const bool inClip = clip.contains(point);

        const bool inResult = result.contains(point);

        bool expected = false;
        switch (op) {
        case QPathClipper::BoolAnd:
            expected = inSubject && inClip;
            break;
        case QPathClipper::BoolOr:
            expected = inSubject || inClip;
            break;
        case QPathClipper::BoolSub:
            expected = inSubject && !inClip;
            break;
        default:
            break;
        }

        if (expected != inResult) {
            char str[256];
            const char *opStr =
                 op == QPathClipper::BoolAnd ? "and" :
                 op == QPathClipper::BoolOr ? "or" : "sub";
            sprintf(str, "Expected: %d, actual: %d, subject: %d, clip: %d, op: %s\n",
                     int(expected), int(inResult), subjectIndex, clipIndex, opStr);
            QFAIL(str);
        }
    }
}

void tst_QPathClipper::clip2()
{
    if (sizeof(double) != sizeof(qreal))
        QSKIP("This test only works for qreal=double, otherwise ends in rounding errors");

    int operation = 0;

    for (int i = 0; i < paths.size(); ++i) {
        for (int j = 0; j <= i; ++j) {
            QPathClipper::Operation op = QPathClipper::Operation((operation++) % 3);
            clipTest(i, j, op);
        }
    }
}

void tst_QPathClipper::clip3()
{
    int operation = 0;

    // this subset should work correctly for qreal = float
    for (int i = 0; i < 20; ++i) {
        for (int j = 0; j <= i; ++j) {
            QPathClipper::Operation op = QPathClipper::Operation((operation++) % 3);
            clipTest(i, j, op);
        }
    }
}

void tst_QPathClipper::testIntersections()
{
    QPainterPath path1;
    QPainterPath path2;

    path1.addRect(0, 0, 100, 100);
    path2.addRect(20, 20, 20, 20);
    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));
    QVERIFY(path1.contains(path2));
    QVERIFY(!path2.contains(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addEllipse(0, 0, 100, 100);
    path2.addEllipse(200, 200, 100, 100);
    QVERIFY(!path1.intersects(path2));
    QVERIFY(!path2.intersects(path1));
    QVERIFY(!path1.contains(path2));
    QVERIFY(!path2.contains(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addEllipse(0, 0, 100, 100);
    path2.addEllipse(50, 50, 100, 100);
    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));
    QVERIFY(!path1.contains(path2));
    QVERIFY(!path2.contains(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addRect(100, 100, 100, 100);
    path2.addRect(50, 100, 100, 20);
    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));
    QVERIFY(!path1.contains(path2));
    QVERIFY(!path2.contains(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addRect(100, 100, 100, 100);
    path2.addRect(110, 201, 100, 20);
    QVERIFY(!path1.intersects(path2));
    QVERIFY(!path2.intersects(path1));
    QVERIFY(!path1.contains(path2));
    QVERIFY(!path2.contains(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addRect(0, 0, 100, 100);
    path2.addRect(20, 20, 20, 20);
    path2.addRect(25, 25, 5, 5);
    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));
    QVERIFY(path1.contains(path2));
    QVERIFY(!path2.contains(path1));
}

void tst_QPathClipper::testIntersections2()
{
    QPainterPath path1;
    QPainterPath path2;

    path1 = QPainterPath();
    path2 = QPainterPath();

    path1.moveTo(-8,-8);
    path1.lineTo(107,-8);
    path1.lineTo(107,107);
    path1.lineTo(-8,107);

    path2.moveTo(0,0);
    path2.lineTo(100,0);
    path2.lineTo(100,100);
    path2.lineTo(0,100);
    path2.lineTo(0,0);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));
    QVERIFY(path1.contains(path2));
    QVERIFY(!path2.contains(path1));

    path1.closeSubpath();

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));
    QVERIFY(path1.contains(path2));
    QVERIFY(!path2.contains(path1));
}

void tst_QPathClipper::testIntersections3()
{
    QPainterPath path1 = Paths::node();
    QPainterPath path2 = Paths::interRect();

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));
}

void tst_QPathClipper::testIntersections4()
{
    QPainterPath path1;
    QPainterPath path2;

    path1.moveTo(-5, 0);
    path1.lineTo(5, 0);

    path2.moveTo(0, -5);
    path2.lineTo(0, 5);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));
}

void tst_QPathClipper::testIntersections5()
{
    QPainterPath path1;
    QPainterPath path2;

    path1.addRect(0, 0, 4, 4);
    path1.addRect(2, 1, 1, 1);
    path2.addRect(0.5, 2, 1, 1);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));
}

void tst_QPathClipper::testIntersections6()
{
    QPainterPath path1;
    QPainterPath path2;

    path1.moveTo(QPointF(-115.567, -98.3254));
    path1.lineTo(QPointF(-45.9007, -98.3254));
    path1.lineTo(QPointF(-45.9007, -28.6588));
    path1.lineTo(QPointF(-115.567, -28.6588));

    path2.moveTo(QPointF(-110, -110));
    path2.lineTo(QPointF(110, -110));
    path2.lineTo(QPointF(110, 110));
    path2.lineTo(QPointF(-110, 110));
    path2.lineTo(QPointF(-110, -110));

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));
}


void tst_QPathClipper::testIntersections7()
{
    QPainterPath path1;
    QPainterPath path2;

    path1.addRect(0, 0, 10, 10);
    path2.addRect(5, 0, 10, 10);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addRect(0, 0, 10, 10);
    path2.addRect(0, 5, 10, 10);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addRect(0, 0, 10, 10);
    path2.addRect(0, 0, 10, 10);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    ///
    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addRect(1, 1, 10, 10);
    path2.addRect(5, 1, 10, 10);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addRect(1, 1, 10, 10);
    path2.addRect(1, 5, 10, 10);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addRect(1, 1, 10, 10);
    path2.addRect(1, 1, 10, 10);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addRect(1, 1, 10, 10);
    path2.addRect(5, 5, 10, 10);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addRect(1, 1, 10, 10);
    path2.addRect(9, 9, 10, 10);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addRect(1, 1, 10, 10);
    path2.addRect(10, 10, 10, 10);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addRect(1, 1, 9, 9);
    path2.addRect(11, 11, 10, 10);

    QVERIFY(!path1.intersects(path2));
    QVERIFY(!path2.intersects(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addRect(1, 1, 10, 10);
    path2.addRect(12, 12, 10, 10);

    QVERIFY(!path1.intersects(path2));
    QVERIFY(!path2.intersects(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addRect(11, 11, 10, 10);
    path2.addRect(12, 12, 10, 10);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = QPainterPath();
    path2 = QPainterPath();
    path1.addRect(11, 11, 10, 10);
    path2.addRect(10, 10, 10, 10);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));
}


void tst_QPathClipper::testIntersections8()
{
    QPainterPath path1 = Paths::node() * QTransform().translate(100, 50);
    QPainterPath path2 = Paths::node() * QTransform().translate(150, 50);;

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = Paths::node();
    path2 = Paths::node();

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = Paths::node();
    path2 = Paths::node() * QTransform().translate(0, 30);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = Paths::node();
    path2 = Paths::node() * QTransform().translate(30, 0);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = Paths::node();
    path2 = Paths::node() * QTransform().translate(30, 30);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = Paths::node();
    path2 = Paths::node() * QTransform().translate(1, 1);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));
}


void tst_QPathClipper::testIntersections9()
{
    QPainterPath path1;
    QPainterPath path2;

    path1.addRect(QRectF(-1,143, 146, 106));
    path2.addRect(QRectF(-9,145, 150, 100));

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = QPainterPath();;
    path2 = QPainterPath();

    path1.addRect(QRectF(-1,191, 136, 106));
    path2.addRect(QRectF(-19,194, 150, 100));
    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));

    path1 = QPainterPath();;
    path2 = QPainterPath();

    path1.moveTo(-1 ,  143);
    path1.lineTo(148 ,  143);
    path1.lineTo(148 ,  250);
    path1.lineTo(-1 ,  250);

    path2.moveTo(-5 ,  146);
    path2.lineTo(145 ,  146);
    path2.lineTo(145 ,  246);
    path2.lineTo(-5 ,  246);
    path2.lineTo(-5 ,  146);

    QVERIFY(path1.intersects(path2));
    QVERIFY(path2.intersects(path1));
}

QPainterPath pathFromRect(qreal x, qreal y, qreal w, qreal h)
{
    QPainterPath path;
    path.addRect(QRectF(x, y, w, h));
    return path;
}

QPainterPath pathFromLine(qreal x1, qreal y1, qreal x2, qreal y2)
{
    QPainterPath path;
    path.moveTo(x1, y1);
    path.lineTo(x2, y2);
    return path;
}

static int loopLength(const QWingedEdge &list, QWingedEdge::TraversalStatus status)
{
    int start = status.edge;

    int length = 0;
    do {
        ++length;
        status = list.next(status);
    } while (status.edge != start);

    return length;
}

void tst_QPathClipper::testWingedEdge()
{
    {
        QWingedEdge list;
        int e1 = list.addEdge(QPointF(0, 0), QPointF(10, 0));
        int e2 = list.addEdge(QPointF(0, 0), QPointF(0, 10));
        int e3 = list.addEdge(QPointF(0, 0), QPointF(-10, 0));
        int e4 = list.addEdge(QPointF(0, 0), QPointF(0, -10));

        QCOMPARE(list.edgeCount(), 4);
        QCOMPARE(list.vertexCount(), 5);

        QWingedEdge::TraversalStatus status = { e1, QPathEdge::RightTraversal, QPathEdge::Forward };

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Backward);
        QCOMPARE(status.traversal, QPathEdge::LeftTraversal);
        QCOMPARE(status.edge, e1);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Forward);
        QCOMPARE(status.traversal, QPathEdge::RightTraversal);
        QCOMPARE(status.edge, e4);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Backward);
        QCOMPARE(status.traversal, QPathEdge::LeftTraversal);
        QCOMPARE(status.edge, e4);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Forward);
        QCOMPARE(status.traversal, QPathEdge::RightTraversal);
        QCOMPARE(status.edge, e3);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Backward);
        QCOMPARE(status.traversal, QPathEdge::LeftTraversal);
        QCOMPARE(status.edge, e3);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Forward);
        QCOMPARE(status.traversal, QPathEdge::RightTraversal);
        QCOMPARE(status.edge, e2);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Backward);
        QCOMPARE(status.traversal, QPathEdge::LeftTraversal);
        QCOMPARE(status.edge, e2);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Forward);
        QCOMPARE(status.traversal, QPathEdge::RightTraversal);
        QCOMPARE(status.edge, e1);
    }
    {
        QWingedEdge list;
        int e1 = list.addEdge(QPointF(5, 0), QPointF(5, 10));
        int e2 = list.addEdge(QPointF(5, 0), QPointF(10, 5));
        int e3 = list.addEdge(QPointF(10, 5), QPointF(5, 10));
        int e4 = list.addEdge(QPointF(5, 0), QPointF(0, 5));
        int e5 = list.addEdge(QPointF(0, 5), QPointF(5, 10));

        QCOMPARE(list.edgeCount(), 5);
        QCOMPARE(list.vertexCount(), 4);

        QWingedEdge::TraversalStatus status = { e1, QPathEdge::RightTraversal, QPathEdge::Forward };

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Backward);
        QCOMPARE(status.traversal, QPathEdge::LeftTraversal);
        QCOMPARE(status.edge, e5);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Backward);
        QCOMPARE(status.traversal, QPathEdge::LeftTraversal);
        QCOMPARE(status.edge, e4);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Forward);
        QCOMPARE(status.traversal, QPathEdge::RightTraversal);
        QCOMPARE(status.edge, e1);

        QCOMPARE(loopLength(list, status), 3);

        status.flip();
        QCOMPARE(status.direction, QPathEdge::Backward);
        QCOMPARE(status.traversal, QPathEdge::LeftTraversal);
        QCOMPARE(loopLength(list, status), 3);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Forward);
        QCOMPARE(status.traversal, QPathEdge::RightTraversal);
        QCOMPARE(status.edge, e2);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Forward);
        QCOMPARE(status.traversal, QPathEdge::RightTraversal);
        QCOMPARE(status.edge, e3);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Backward);
        QCOMPARE(status.traversal, QPathEdge::LeftTraversal);
        QCOMPARE(status.edge, e1);

        status = list.next(status);
        status.flip();
        QCOMPARE(status.direction, QPathEdge::Backward);
        QCOMPARE(status.traversal, QPathEdge::LeftTraversal);
        QCOMPARE(status.edge, e2);
        QCOMPARE(loopLength(list, status), 4);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Forward);
        QCOMPARE(status.traversal, QPathEdge::RightTraversal);
        QCOMPARE(status.edge, e4);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Forward);
        QCOMPARE(status.traversal, QPathEdge::RightTraversal);
        QCOMPARE(status.edge, e5);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Backward);
        QCOMPARE(status.traversal, QPathEdge::LeftTraversal);
        QCOMPARE(status.edge, e3);

        status = list.next(status);
        QCOMPARE(status.direction, QPathEdge::Backward);
        QCOMPARE(status.traversal, QPathEdge::LeftTraversal);
        QCOMPARE(status.edge, e2);
    }
    {
        QPainterPath path = pathFromRect(0, 0, 20, 20);
        QWingedEdge list(path, QPainterPath());

        QCOMPARE(list.edgeCount(), 4);
        QCOMPARE(list.vertexCount(), 4);

        QWingedEdge::TraversalStatus status = { 0, QPathEdge::RightTraversal, QPathEdge::Forward };

        QPathEdge *edge = list.edge(status.edge);
        QCOMPARE(QPointF(*list.vertex(edge->first)), QPointF(0, 0));
        QCOMPARE(QPointF(*list.vertex(edge->second)), QPointF(20, 0));

        status = list.next(status);
        QCOMPARE(status.edge, 1);

        status = list.next(status);
        QCOMPARE(status.edge, 2);

        status = list.next(status);
        QCOMPARE(status.edge, 3);

        status = list.next(status);
        QCOMPARE(status.edge, 0);

        status.flipDirection();
        status = list.next(status);
        QCOMPARE(status.edge, 3);

        status = list.next(status);
        QCOMPARE(status.edge, 2);

        status = list.next(status);
        QCOMPARE(status.edge, 1);

        status = list.next(status);
        QCOMPARE(status.edge, 0);

        QWingedEdge list2(path, pathFromRect(10, 5, 20, 10));

        QCOMPARE(list2.edgeCount(), 12);
        QCOMPARE(list2.vertexCount(), 10);

        status.flipDirection();
        QCOMPARE(loopLength(list2, status), 8);

        status = list2.next(status);
        edge = list2.edge(status.edge);
        QCOMPARE(QPointF(*list2.vertex(edge->first)), QPointF(20, 0));
        QCOMPARE(QPointF(*list2.vertex(edge->second)), QPointF(20, 5));

        status = list2.next(status);
        status.flipTraversal();

        edge = list2.edge(status.edge);
        QCOMPARE(QPointF(*list2.vertex(edge->first)), QPointF(10, 5));
        QCOMPARE(QPointF(*list2.vertex(edge->second)), QPointF(20, 5));

        QCOMPARE(loopLength(list2, status), 4);

        status.flipDirection();
        status = list2.next(status);
        status.flipTraversal();

        edge = list2.edge(status.edge);
        QCOMPARE(QPointF(*list2.vertex(edge->first)), QPointF(20, 5));
        QCOMPARE(QPointF(*list2.vertex(edge->second)), QPointF(20, 15));

        QCOMPARE(loopLength(list2, status), 4);
        status = list2.next(status);
        status = list2.next(status);

        edge = list2.edge(status.edge);
        QCOMPARE(QPointF(*list2.vertex(edge->first)), QPointF(30, 5));
        QCOMPARE(QPointF(*list2.vertex(edge->second)), QPointF(30, 15));
    }
}

void tst_QPathClipper::zeroDerivativeCurves()
{
    // zero derivative at end
    {
        QPainterPath a;
        a.cubicTo(100, 0, 100, 100, 100, 100);
        a.lineTo(100, 200);
        a.lineTo(0, 200);

        QPainterPath b;
        b.moveTo(50, 100);
        b.lineTo(150, 100);
        b.lineTo(150, 150);
        b.lineTo(50, 150);

        QPainterPath c = a.united(b);
        QVERIFY(c.contains(QPointF(25, 125)));
        QVERIFY(c.contains(QPointF(75, 125)));
        QVERIFY(c.contains(QPointF(125, 125)));
    }

    // zero derivative at start
    {
        QPainterPath a;
        a.cubicTo(100, 0, 100, 100, 100, 100);
        a.lineTo(100, 200);
        a.lineTo(0, 200);

        QPainterPath b;
        b.moveTo(50, 100);
        b.lineTo(150, 100);
        b.lineTo(150, 150);
        b.lineTo(50, 150);

        QPainterPath c = a.united(b);
        QVERIFY(c.contains(QPointF(25, 125)));
        QVERIFY(c.contains(QPointF(75, 125)));
        QVERIFY(c.contains(QPointF(125, 125)));
    }
}

static bool strictContains(const QPainterPath &a, const QPainterPath &b)
{
    return b.subtracted(a) == QPainterPath();
}


void tst_QPathClipper::task204301_data()
{
    QTest::addColumn<QPolygonF>("points");

    {
        QPointF a(51.09013255685567855835, 31.30814891308546066284);
        QPointF b(98.39898971840739250183, 11.02079074829816818237);
        QPointF c(91.23911846894770860672, 45.86981737054884433746);
        QPointF d(66.58616356085985898972, 63.10526528395712375641);
        QPointF e(82.08219456479714892794, 94.90238165489137145414);
        QPointF f(16.09013040543221251255, 105.66263409332729850121);
        QPointF g(10.62811442650854587555, 65.09154842235147953033);
        QPointF h(5.16609844751656055450, 24.52046275138854980469);
        QPolygonF v;
        v << a << b << c << d << e << f << g << h;
        QTest::newRow("failed_on_linux") << v;
    }

    {
        QPointF a(50.014648437500000, 24.392089843750000);
        QPointF b(92.836303710937500, 5.548706054687500);
        QPointF c(92.145690917968750, 54.390258789062500);
        QPointF d(65.402221679687500, 74.345092773437500);
        QPointF e(80.789794921787347, 124.298095703129690);
        QPointF f(34.961242675812954, 87.621459960852135);
        QPointF g(18.305969238281250, 57.426757812500000);
        QPointF h(1.650695800781250, 27.232055664062500);
        QPolygonF v;
        v << a << b << c << d << e << f << g << h;
        QTest::newRow("failed_on_windows") << v;
    }
}

void tst_QPathClipper::task204301()
{
    QFETCH(QPolygonF, points);

    QPointF a = points[0];
    QPointF b = points[1];
    QPointF c = points[2];
    QPointF d = points[3];
    QPointF e = points[4];
    QPointF f = points[5];
    QPointF g = points[6];
    QPointF h = points[7];

    QPainterPath subA;
    subA.addPolygon(QPolygonF() << a << b << c << d);
    subA.closeSubpath();

    QPainterPath subB;
    subB.addPolygon(QPolygonF() << f << e << d << g);
    subB.closeSubpath();

    QPainterPath subC;
    subC.addPolygon(QPolygonF() << h << a << d << g);
    subC.closeSubpath();

    QPainterPath path;
    path.addPath(subA);
    path.addPath(subB);
    path.addPath(subC);

    QPainterPath simplified = path.simplified();

    QVERIFY(strictContains(simplified, subA));
    QVERIFY(strictContains(simplified, subB));
    QVERIFY(strictContains(simplified, subC));
}

void tst_QPathClipper::task209056()
{
    QPainterPath p1;
    p1.moveTo( QPointF(188.506, 287.793) );
    p1.lineTo( QPointF(288.506, 287.793) );
    p1.lineTo( QPointF(288.506, 387.793) );
    p1.lineTo( QPointF(188.506, 387.793) );
    p1.lineTo( QPointF(188.506, 287.793) );

    QPainterPath p2;
    p2.moveTo( QPointF(419.447, 164.383) );
    p2.cubicTo( QPointF(419.447, 69.5486), QPointF(419.447, 259.218),QPointF(419.447, 164.383) );

    p2.cubicTo( QPointF(48.9378, 259.218), QPointF(131.879, 336.097),QPointF(234.192, 336.097) );
    p2.cubicTo( QPointF(336.506, 336.097), QPointF(419.447, 259.218),QPointF(419.447, 164.383) );

    QPainterPath p3 = p1.intersected(p2);

    QVERIFY(p3 != QPainterPath());
}

void tst_QPathClipper::task251909()
{
    QPainterPath p1;
    p1.moveTo(0, -10);
    p1.lineTo(10, -10);
    p1.lineTo(10, 0);
    p1.lineTo(0, 0);

    QPainterPath p2;
    p2.moveTo(0, 8e-14);
    p2.lineTo(10, -8e-14);
    p2.lineTo(10, 10);
    p2.lineTo(0, 10);

    QPainterPath result = p1.united(p2);

    QVERIFY(result.elementCount() <= 5);
}

void tst_QPathClipper::qtbug3778()
{
    if (sizeof(double) != sizeof(qreal)) {
        QSKIP("This test only works for qreal=double, otherwise ends in rounding errors");
    }
    QPainterPath path1;
    path1.moveTo(200, 3.22409e-5);
    // e-5 and higher leads to a bug
    // Using 3.22409e-4 starts to work correctly
    path1.lineTo(0, 0);
    path1.lineTo(1.07025e-13, 1450);
    path1.lineTo(750, 950);
    path1.lineTo(950, 750);
    path1.lineTo(200, 3.22409e-13);

    QPainterPath path2;
    path2.moveTo(0, 0);
    path2.lineTo(200, 800);
    path2.lineTo(600, 1500);
    path2.lineTo(1500, 1400);
    path2.lineTo(1900, 1200);
    path2.lineTo(2000, 1000);
    path2.lineTo(1400, 0);
    path2.lineTo(0, 0);

    QPainterPath p12 = path1.intersected(path2);

    QVERIFY(p12.contains(QPointF(100, 100)));
}

QTEST_MAIN(tst_QPathClipper)


#include "tst_qpathclipper.moc"
