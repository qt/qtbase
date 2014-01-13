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
#include <qregion.h>

#include <qbitmap.h>
#include <qpainter.h>
#include <qpolygon.h>
#ifdef Q_WS_X11
#include <private/qt_x11_p.h>
#endif

class tst_QRegion : public QObject
{
    Q_OBJECT

public:
    tst_QRegion();

private slots:
    void boundingRect();
    void rects();
    void swap();
    void setRects();
    void ellipseRegion();
    void polygonRegion();
    void bitmapRegion();
    void intersected_data();
    void intersected();
    void emptyPolygonRegion_data();
    void emptyPolygonRegion();

    void intersects_region_data();
    void intersects_region();
    void intersects_rect_data();
    void intersects_rect();
    void contains_point();

    void operator_plus_data();
    void operator_plus();
    void operator_minus_data();
    void operator_minus();
    void operator_intersect_data();
    void operator_intersect();
    void operator_xor_data();
    void operator_xor();

    void rectCount_data();
    void rectCount();

    void isEmpty_data();
    void isEmpty();
#if defined(Q_WS_X11) && defined(QT_BUILD_INTERNAL)
    void clipRectangles();
#endif

    void regionFromPath();

#ifdef QT_BUILD_INTERNAL
    void regionToPath_data();
    void regionToPath();
#endif
};

tst_QRegion::tst_QRegion()
{
}

void tst_QRegion::boundingRect()
{
    {
        QRect rect;
        QRegion region(rect);
        QCOMPARE(region.boundingRect(), rect);
    }
    {
        QRect rect(10, -20, 30, 40);
        QRegion region(rect);
        QCOMPARE(region.boundingRect(), rect);
    }
    {
        QRect rect(15,25,10,10);
        QRegion region(rect);
        QCOMPARE(region.boundingRect(), rect);
    }

}

void tst_QRegion::rects()
{
    {
        QRect rect;
        QRegion region(rect);
        QVERIFY(region.isEmpty());
        QVERIFY(region.rects().isEmpty());
    }
    {
        QRect rect(10, -20, 30, 40);
        QRegion region(rect);
        QCOMPARE(region.rects().count(), 1);
        QCOMPARE(region.rects()[0], rect);
    }
    {
        QRect r(QPoint(10, 10), QPoint(40, 40));
        QRegion region(r);
        QVERIFY(region.contains(QPoint(10,10)));
        QVERIFY(region.contains(QPoint(20,40)));
        QVERIFY(region.contains(QPoint(40,20)));
        QVERIFY(!region.contains(QPoint(20,41)));
        QVERIFY(!region.contains(QPoint(41,20)));
    }
    {
        QRect r(10, 10, 30, 30);
        QRegion region(r);
        QVERIFY(region.contains(QPoint(10,10)));
        QVERIFY(region.contains(QPoint(20,39)));
        QVERIFY(region.contains(QPoint(39,20)));
        QVERIFY(!region.contains(QPoint(20,40)));
        QVERIFY(!region.contains(QPoint(40,20)));
    }
}

void tst_QRegion::swap()
{
    QRegion r1(QRect(0, 0,10,10));
    QRegion r2(QRect(10,10,10,10));
    r1.swap(r2);
    QCOMPARE(r1.rects().front(), QRect(10,10,10,10));
    QCOMPARE(r2.rects().front(), QRect(0, 0,10,10));
}

void tst_QRegion::setRects()
{
    {
        QRegion region;
        region.setRects(0, 0);
        QVERIFY(region.rects().isEmpty());
    }
    {
        QRegion region;
        QRect rect;
        region.setRects(&rect, 0);
        QVERIFY(region.isEmpty());
        QVERIFY(region == QRegion());
        QVERIFY(!region.boundingRect().isValid());
        QVERIFY(region.rects().isEmpty());
    }
    {
        QRegion region;
        QRect rect;
        region.setRects(&rect, 1);
        QVERIFY(!region.boundingRect().isValid());
        QVERIFY(region.rects().isEmpty());
    }
    {
        QRegion region;
        QRect rect(10, -20, 30, 40);
        region.setRects(&rect, 1);
        QCOMPARE(region.rects().count(), 1);
        QCOMPARE(region.rects()[0], rect);
    }
}

void tst_QRegion::ellipseRegion()
{
    QRegion region(0, 0, 100, 100, QRegion::Ellipse);

    // These should not be inside the circe
    QVERIFY(!region.contains(QPoint(13, 13)));
    QVERIFY(!region.contains(QPoint(13, 86)));
    QVERIFY(!region.contains(QPoint(86, 13)));
    QVERIFY(!region.contains(QPoint(86, 86)));

    // These should be inside
    QVERIFY(region.contains(QPoint(16, 16)));
    QVERIFY(region.contains(QPoint(16, 83)));
    QVERIFY(region.contains(QPoint(83, 16)));
    QVERIFY(region.contains(QPoint(83, 83)));

    //     ..a..
    //   ..     ..
    //  .         .
    // .           .
    // b           c
    // .           .
    //  .         .
    //   ..     ..
    //     ..d..
    QVERIFY(region.contains(QPoint(50, 0)));   // Mid-top    (a)
    QVERIFY(region.contains(QPoint(0, 50)));   // Mid-left   (b)
    QVERIFY(region.contains(QPoint(99, 50)));  // Mid-right  (c)
    QVERIFY(region.contains(QPoint(50, 99)));  // Mid-bottom (d)

    QRect bounds = region.boundingRect();
    QCOMPARE(bounds.x(), 0);
    QCOMPARE(bounds.y(), 0);
    QCOMPARE(bounds.width(), 100);
    QCOMPARE(bounds.height(), 100);
}

void tst_QRegion::polygonRegion()
{
    QPolygon pa;
    {
        QRegion region (pa);
        QVERIFY(region.isEmpty());
    }
    {
        pa.setPoints(8, 10, 10, //  a____________b
                        40, 10, //  |            |
                        40, 20, //  |___      ___|
                        30, 20, //      |    |
                        30, 40, //      |    |
                        20, 40, //      |    |
                        20, 20, //      |____c
                        10, 20);

        QRegion region (pa);
        QVERIFY(!region.isEmpty());

        // These should not be inside the circle
        QVERIFY(!region.contains(QPoint( 9,  9)));
        QVERIFY(!region.contains(QPoint(30, 41)));
        QVERIFY(!region.contains(QPoint(41, 10)));
        QVERIFY(!region.contains(QPoint(31, 21)));

        // These should be inside
        QVERIFY(region.contains(QPoint(10, 10))); // Upper-left  (a)

    }
}

void tst_QRegion::emptyPolygonRegion_data()
{
    QTest::addColumn<QPolygon>("pa");
    QTest::addColumn<bool>("isEmpty");
    QTest::addColumn<int>("numRects");
    QTest::addColumn<QVector<QRect> >("rects");

    QPolygon pa;


    QTest::newRow("no points") << pa << true << 0 << QVector<QRect>();
    pa = QPolygon() << QPoint(10,10);
    QTest::newRow("one point") << pa << true << 0 << QVector<QRect>();
    pa = QPolygon() << QPoint(10,10) << QPoint(10,20);
    QTest::newRow("two points, horizontal") << pa << true << 0 << QVector<QRect>();

    pa = QPolygon() << QPoint(10,10) << QPoint(20,10);
    QTest::newRow("two points, vertical") << pa << true << 0 << QVector<QRect>();

    pa = QPolygon() << QPoint(10,10) << QPoint(20,20);
    QTest::newRow("two points, diagonal") << pa << true << 0 << QVector<QRect>();

    pa = QPolygon() << QPoint(10,10) << QPoint(15,15) << QPoint(10,15) << QPoint(10, 10) ;
    QVector<QRect> v;
    v << QRect(10,11,1, 1) << QRect(10,12,2,1) << QRect(10,13,3,1) << QRect(10,14,4,1);
    QTest::newRow("triangle") << pa << false << 4 << v;

    v.clear();
    v << QRect(10,10,10,10);

    QTest::newRow("rectangle") << QPolygon(QRect(10,10,10,10))  << false << 1 << v;

}

void tst_QRegion::emptyPolygonRegion()
{
    QFETCH(QPolygon, pa);

    QRegion r(pa);
    QTEST(r.isEmpty(), "isEmpty");
    QTEST(r.rects().count(), "numRects");
    QTEST(r.rects(), "rects");
}


static const char *circle_xpm[] = {
    "20 20 2 1",
    "  c #FFFFFF",
    ". c #000000",
    "       ......       ",
    "     ..........     ",
    "   ..............   ",
    "  ................  ",
    "  ................  ",
    " .................. ",
    " .................. ",
    "....................",
    "....................",
    "....................",
    "....................",
    "....................",
    "....................",
    " .................. ",
    " .................. ",
    "  ................  ",
    "  ................  ",
    "   ..............   ",
    "     ..........     ",
    "       ......       "
};

void tst_QRegion::bitmapRegion()
{
    QBitmap circle;
    {
        QRegion region(circle);
        QVERIFY(region.isEmpty());
    }
    {
        circle = QPixmap(circle_xpm);
        QRegion region(circle);

        //// These should not be inside the circe
        QVERIFY(!region.contains(QPoint(2,   2)));
        QVERIFY(!region.contains(QPoint(2,  17)));
        QVERIFY(!region.contains(QPoint(17,  2)));
        QVERIFY(!region.contains(QPoint(17, 17)));

        //// These should be inside
        QVERIFY(region.contains(QPoint(3,   3)));
        QVERIFY(region.contains(QPoint(3,  16)));
        QVERIFY(region.contains(QPoint(16,  3)));
        QVERIFY(region.contains(QPoint(16, 16)));

        QVERIFY(region.contains(QPoint(0, 10)));  // Mid-left
        QVERIFY(region.contains(QPoint(10, 0)));  // Mid-top
        QVERIFY(region.contains(QPoint(19, 10))); // Mid-right
        QVERIFY(region.contains(QPoint(10, 19))); // Mid-bottom
    }
}

void tst_QRegion::intersected_data()
{
    QTest::addColumn<QRegion>("r1");
    QTest::addColumn<QRegion>("r2");
    QTest::addColumn<bool>("intersects");
    // QTest::addColumn<QRegion>("intersected");

    QPolygon ps1(8);
    QPolygon ps2(8);
    ps1.putPoints(0,8, 20,20, 50,20, 50,100, 70,100, 70,20, 120,20, 120,200, 20, 200);
    ps2.putPoints(0,8, 100,150, 140,150, 140,160, 160,160, 160,150, 200,150, 200,180, 100,180);
    QTest::newRow("task30716") << QRegion(ps1) << QRegion(ps2) << true;
}

void tst_QRegion::intersected()
{
    QFETCH(QRegion, r1);
    QFETCH(QRegion, r2);
    QFETCH(bool, intersects);

    QRegion interReg = r1.intersected(r2);
    QVERIFY(interReg.isEmpty() != intersects);
    // Need a way to test the intersected QRegion is right
}

void tst_QRegion::intersects_region_data()
{
    QTest::addColumn<QRegion>("r1");
    QTest::addColumn<QRegion>("r2");
    QTest::addColumn<bool>("intersects");

    QTest::newRow("rect overlap rect") << QRegion(100, 100, 200, 200)
                                       << QRegion(200, 200, 200, 200)
                                       << true;

    QTest::newRow("rect not overlap rect") << QRegion(100, 100, 200, 200)
                                           << QRegion(400, 400, 200, 200)
                                           << false;

    QTest::newRow("ellipse overlap ellipse") << QRegion(100, 100, 200, 200, QRegion::Ellipse)
                                             << QRegion(200, 200, 200, 200, QRegion::Ellipse)
                                             << true;

    QTest::newRow("ellipse not overlap ellipse") << QRegion(100, 100, 200, 200, QRegion::Ellipse)
                                                 << QRegion(400, 400, 200, 200, QRegion::Ellipse)
                                                 << false;
}

void tst_QRegion::intersects_region()
{
    QFETCH(QRegion, r1);
    QFETCH(QRegion, r2);
    QFETCH(bool, intersects);
    QCOMPARE(r1.intersects(r2), intersects);
}


void tst_QRegion::intersects_rect_data()
{
    QTest::addColumn<QRegion>("region");
    QTest::addColumn<QRect>("rect");
    QTest::addColumn<bool>("intersects");

    QTest::newRow("rect overlap rect") << QRegion(100, 100, 200, 200)
                                       << QRect(200, 200, 200, 200)
                                       << true;

    QTest::newRow("rect not overlap rect") << QRegion(100, 100, 200, 200)
                                           << QRect(400, 400, 200, 200)
                                           << false;

    QTest::newRow("ellipse overlap rect") << QRegion(100, 100, 200, 200, QRegion::Ellipse)
                                          << QRect(200, 200, 200, 200)
                                          << true;

    QTest::newRow("ellipse not overlap rect") << QRegion(100, 100, 200, 200, QRegion::Ellipse)
                                              << QRect(400, 400, 200, 200)
                                              << false;
}

void tst_QRegion::intersects_rect()
{
    QFETCH(QRegion, region);
    QFETCH(QRect, rect);
    QFETCH(bool, intersects);
    QCOMPARE(region.intersects(rect), intersects);
}

void tst_QRegion::contains_point()
{
    QCOMPARE(QRegion().contains(QPoint(1,1)),false);
    QCOMPARE(QRegion(0,0,2,2).contains(QPoint(1,1)),true);
}

void tst_QRegion::operator_plus_data()
{
    QTest::addColumn<QRegion>("r1");
    QTest::addColumn<QRegion>("r2");
    QTest::addColumn<QRegion>("expected");

    QTest::newRow("empty 0") << QRegion() << QRegion() << QRegion();
    QTest::newRow("empty 1") << QRegion() << QRegion(QRect(10, 10, 10, 10))
                             << QRegion(QRect(10, 10, 10, 10));
    QTest::newRow("empty 2") << QRegion(QRect(10, 10, 10, 10)) << QRegion()
                             << QRegion(QRect(10, 10, 10, 10));

    QRegion expected;
    QVector<QRect> rects;
    rects << QRect(10, 10, 10, 10) << QRect(22, 10, 10, 10);
    expected.setRects(rects.constData(), rects.size());
    QTest::newRow("non overlapping") << QRegion(10, 10, 10, 10)
                                     << QRegion(22, 10, 10, 10)
                                     << expected;

    rects.clear();
    rects << QRect(50, 0, 50, 2);
    expected.setRects(rects.constData(), rects.size());
    QTest::newRow("adjacent y-rects") << QRegion(50, 0, 50, 1)
                                      << QRegion(50, 1, 50, 1)
                                      << expected;

    rects.clear();
    rects << QRect(50, 0, 2, 1);
    expected.setRects(rects.constData(), rects.size());
    QTest::newRow("adjacent x-rects") << QRegion(50, 0, 1, 1)
                                      << QRegion(51, 0, 1, 1)
                                      << expected;

    rects.clear();
    rects << QRect(10, 10, 10, 10) << QRect(10, 20, 5, 10);
    QRegion r1;
    r1.setRects(rects.constData(), rects.size());
    QTest::newRow("double merge") << r1 << QRegion(15, 20, 5, 10)
                                  << QRegion(10, 10, 10, 20);
    rects.clear();
    rects << QRect(15, 10, 5, 10) << QRect(10, 20, 10, 10);
    r1.setRects(rects.constData(), rects.size());
    QTest::newRow("double merge 2") << r1 << QRegion(10, 10, 5, 10)
                                    << QRegion(10, 10, 10, 20);
    QTest::newRow("overlapping x") << QRegion(10, 10, 10, 10)
                                   << QRegion(15, 10, 10, 10)
                                   << QRegion(10, 10, 15, 10);
    QTest::newRow("overlapping y") << QRegion(10, 10, 10, 10)
                                   << QRegion(10, 15, 10, 10)
                                   << QRegion(10, 10, 10, 15);
    rects.clear();
    rects << QRect(10, 10, 10, 10) << QRect(10, 20, 5, 10);
    r1.setRects(rects.constData(), rects.size());
    rects.clear();
    rects << QRect(15, 20, 5, 10) << QRect(10, 30, 10, 10);
    QRegion r2;
    r2.setRects(rects.constData(), rects.size());
    QTest::newRow("triple merge") << r1 << r2
                                  << QRegion(10, 10, 10, 30);

    rects.clear();
    rects << QRect(10, 10, 4, 10) << QRect(15, 10, 10, 10);
    r1.setRects(rects.constData(), rects.size());
    rects.clear();
    rects << QRect(15, 20, 10, 10);
    r2.setRects(rects.constData(), rects.size());
    rects.clear();
    rects << QRect(10, 10, 4, 10) << QRect(15, 10, 10, 10)
          << QRect(15, 20, 10, 10);
    expected.setRects(rects.constData(), rects.size());
    QTest::newRow("don't merge y") << r1 << r2 << expected;

    QTest::newRow("equal 1") << QRegion(10, 10, 10, 10)
                             << QRegion(10, 10, 10, 10)
                             << QRegion(10, 10, 10, 10);
    QTest::newRow("equal 2") << expected << expected << expected;
}

void tst_QRegion::operator_plus()
{
    QFETCH(QRegion, r1);
    QFETCH(QRegion, r2);
    QFETCH(QRegion, expected);

    if (r1 + r2 != expected) {
        qDebug() << "r1 + r2" << (r1 + r2);
        qDebug() << "expected" << expected;
    }
    QCOMPARE(r1 + r2, expected);
    if (r2.rectCount() == 1) {
        if (r1 + r2.boundingRect() != expected) {
            qDebug() << "r1 + QRect(r2)" << (r1 + r2.boundingRect());
            qDebug() << "expected" << expected;
        }
        QCOMPARE(r1 + r2.boundingRect(), expected);
    }

    if (r2 + r1 != expected) {
        qDebug() << "r2 + r1" << (r2 + r1);
        qDebug() << "expected" << expected;
    }
    QCOMPARE(r2 + r1, expected);
    if (r1.rectCount() == 1) {
        if (r1 + r2.boundingRect() != expected) {
            qDebug() << "r2 + QRect(r1)" << (r2 + r1.boundingRect());
            qDebug() << "expected" << expected;
        }
        QCOMPARE(r2 + r1.boundingRect(), expected);
    }

    QRegion result1 = r1;
    result1 += r2;
    if (result1 != expected) {
        qDebug() << "r1 += r2" << result1;
        qDebug() << "expected" << expected;
    }
    QCOMPARE(result1, expected);
    if (r2.rectCount() == 1) {
        result1 = r1;
        result1 += r2.boundingRect();
        if (result1 != expected) {
            qDebug() << "r1 += QRect(r2)" << result1;
            qDebug() << "expected" << expected;
        }
        QCOMPARE(result1, expected);
    }

    QRegion result2 = r2;
    result2 += r1;
    if (result2 != expected) {
        qDebug() << "r2 += r1" << result2;
        qDebug() << "expected" << expected;
    }
    QCOMPARE(result2, expected);
    if (r1.rectCount() == 1) {
        result2 = r2;
        result2 += r1.boundingRect();
        if (result2 != expected) {
            qDebug() << "r2 += QRect(r1)" << result2;
            qDebug() << "expected" << expected;
        }
        QCOMPARE(result2, expected);
    }
}

void tst_QRegion::operator_minus_data()
{
    QTest::addColumn<QRegion>("dest");
    QTest::addColumn<QRegion>("subtract");
    QTest::addColumn<QRegion>("expected");

    QTest::newRow("empty 0") << QRegion() << QRegion() << QRegion();
    QTest::newRow("empty 1") << QRegion() << QRegion(QRect(10, 10, 10, 10))
                             << QRegion();
    QTest::newRow("empty 2") << QRegion(QRect(10, 10, 10, 10)) << QRegion()
                             << QRegion(QRect(10, 10, 10, 10));

    QRegion dest;
    QVector<QRect> rects;
    rects << QRect(10, 10, 10, 10) << QRect(22, 10, 10, 10);
    dest.setRects(rects.constData(), rects.size());
    QTest::newRow("simple 1") << dest
                              << QRegion(22, 10, 10, 10)
                              << QRegion(10, 10, 10, 10);
    QTest::newRow("simple 2") << dest
                              << QRegion(10, 10, 10, 10)
                              << QRegion(22, 10, 10, 10);

    rects.clear();
    rects << QRect(0, 0, 10, 10) << QRect(15, 0, 10, 10);
    dest.setRects(rects.constData(), rects.size());

    QRegion minus;
    rects.clear();
    rects << QRect(0, 0, 12, 12) << QRect(15, 0, 12, 12);
    minus.setRects(rects.constData(), rects.size());
    QTest::newRow("empty 3") << dest << minus << QRegion();
}

void tst_QRegion::operator_minus()
{
    QFETCH(QRegion, dest);
    QFETCH(QRegion, subtract);
    QFETCH(QRegion, expected);

    if (dest - subtract != expected) {
        qDebug() << "dest - subtract" << (dest - subtract);
        qDebug() << "expected" << expected;
    };
    QCOMPARE(dest - subtract, expected);

    dest -= subtract;

    if (dest != expected) {
        qDebug() << "dest" << dest;
        qDebug() << "expected" << expected;
    };
    QCOMPARE(dest, expected);
}

void tst_QRegion::operator_intersect_data()
{
    QTest::addColumn<QRegion>("r1");
    QTest::addColumn<QRegion>("r2");
    QTest::addColumn<QRegion>("expected");

    QTest::newRow("empty 0") << QRegion() << QRegion() << QRegion();
    QTest::newRow("empty 1") << QRegion() << QRegion(QRect(10, 10, 10, 10))
                             << QRegion();
    QTest::newRow("empty 2") << QRegion(QRect(10, 10, 10, 10)) << QRegion()
                             << QRegion();

    QRegion dest;
    QVector<QRect> rects;
    rects << QRect(10, 10, 10, 10) << QRect(22, 10, 10, 10);
    dest.setRects(rects.constData(), rects.size());
    QTest::newRow("simple 1") << dest
                              << QRegion(22, 10, 10, 10)
                              << QRegion(22, 10, 10, 10);
    QTest::newRow("simple 2") << dest
                              << QRegion(10, 10, 10, 10)
                              << QRegion(10, 10, 10, 10);

    rects.clear();
    rects << QRect(10, 10, 10, 10) << QRect(10, 20, 15, 10);
    dest.setRects(rects.constData(), rects.size());
    QTest::newRow("merge 1") << dest
                             << QRegion(10, 10, 10, 20)
                             << QRegion(10, 10, 10, 20);

    rects.clear();
    rects << QRect(11, 11, 218, 117) << QRect(11, 128, 218, 27)
          << QRect(264, 128, 122, 27) << QRect(11, 155, 218, 43)
          << QRect(11, 198, 218, 27) << QRect(264, 198, 122, 27)
          << QRect(11, 225, 218, 221);
    dest.setRects(rects.constData(), rects.size());
    QTest::newRow("merge 2") << dest << QRegion(11, 11, 218, 458)
                             << QRegion(11, 11, 218, 435);

    rects.clear();
    rects << QRect(0, 0, 10, 10) << QRect(20, 0, 10, 10);
    dest.setRects(rects.constData(), rects.size());
    QTest::newRow("empty 3") << dest << QRegion(11, 0, 5, 5) << QRegion();

    QTest::newRow("extents check") << dest << QRegion(0, 0, 15, 15)
                                   << QRegion(0, 0, 10, 10);

    rects.clear();
    rects << QRect(10, 10, 10, 10) << QRect(10, 20, 10, 10)
          << QRect(30, 20, 10, 10) << QRect(10, 30, 10, 10);
    dest.setRects(rects.constData(), rects.size());
    rects.clear();
    rects << QRect(10, 10, 10, 10) << QRect(10, 20, 10, 10)
          << QRect(30, 20, 10, 10);
    QRegion expected;
    expected.setRects(rects.constData(), rects.size());
    QTest::newRow("dont merge") << dest << QRegion(0, 0, 100, 30)
                                << expected;
}

void tst_QRegion::operator_intersect()
{
    QFETCH(QRegion, r1);
    QFETCH(QRegion, r2);
    QFETCH(QRegion, expected);

    if ((r1 & r2) != expected) {
        qDebug() << "r1 & r2" << (r1 & r2);
        qDebug() << "expected" << expected;
    }
    QCOMPARE(r1 & r2, expected);

    if ((r2 & r1) != expected) {
        qDebug() << "r2 & r1" << (r2 & r1);
        qDebug() << "expected" << expected;
    }
    QCOMPARE(r2 & r1, expected);

    r1 &= r2;
    QCOMPARE(r1, expected);
}

void tst_QRegion::operator_xor_data()
{
    QTest::addColumn<QRegion>("dest");
    QTest::addColumn<QRegion>("arg");
    QTest::addColumn<QRegion>("expected");

    QTest::newRow("empty 0") << QRegion() << QRegion() << QRegion();
    QTest::newRow("empty 1") << QRegion() << QRegion(QRect(10, 10, 10, 10))
                             << QRegion(QRect(10, 10, 10, 10));
    QTest::newRow("empty 2") << QRegion(QRect(10, 10, 10, 10)) << QRegion()
                             << QRegion(QRect(10, 10, 10, 10));

    QRegion dest;
    QVector<QRect> rects;
    rects << QRect(10, 10, 10, 10) << QRect(22, 10, 10, 10);
    dest.setRects(rects.constData(), rects.size());
    QTest::newRow("simple 1") << dest
                              << QRegion(22, 10, 10, 10)
                              << QRegion(10, 10, 10, 10);
    QTest::newRow("simple 2") << dest
                              << QRegion(10, 10, 10, 10)
                              << QRegion(22, 10, 10, 10);
    QTest::newRow("simple 3") << dest << dest << QRegion();
    QTest::newRow("simple 4") << QRegion(10, 10, 10, 10)
                              << QRegion(10, 10, 5, 10)
                              << QRegion(15, 10, 5, 10);
    QTest::newRow("simple 5") << QRegion(10, 10, 10, 10)
                              << QRegion(10, 10, 10, 5)
                              << QRegion(10, 15, 10, 5);

    const QRegion rgnA(0, 0, 100, 100);
    const QRegion rgnB(0, 0, 10, 10);

    QTest::newRow("simple 6") << rgnA
                              << rgnA - rgnB
                              << rgnB;

    QTest::newRow("simple 7") << rgnB
                              << rgnA
                              << rgnA - rgnB;
}

void tst_QRegion::operator_xor()
{
    QFETCH(QRegion, dest);
    QFETCH(QRegion, arg);
    QFETCH(QRegion, expected);

    QCOMPARE(dest ^ arg, expected);
    QCOMPARE(dest.xored(arg), expected);

    dest ^= arg;
    QCOMPARE(dest, expected);
}

void tst_QRegion::rectCount_data()
{
    QTest::addColumn<QRegion>("region");
    QTest::addColumn<int>("expected");

    QTest::newRow("empty") << QRegion() << 0;
    QTest::newRow("rect") << QRegion(10, 10, 10, 10) << 1;

    QRegion dest;
    QVector<QRect> rects;
    rects << QRect(10, 10, 10, 10) << QRect(22, 10, 10, 10);
    dest.setRects(rects.constData(), rects.size());

    QTest::newRow("2 rects") << dest << rects.size();
}

void tst_QRegion::rectCount()
{
    QFETCH(QRegion, region);
    QFETCH(int, expected);

    QCOMPARE(region.rectCount(), expected);
}

void tst_QRegion::isEmpty_data()
{
    QTest::addColumn<QRegion>("region");

    QTest::newRow("QRegion") << QRegion();

    QVector<QRect> rects;
    rects << QRect(0, 0, 10, 10) << QRect(15, 0, 10, 10);
    QRegion r1;
    r1.setRects(rects.constData(), rects.size());

    QRegion r2;
    rects.clear();
    rects << QRect(0, 0, 12, 12) << QRect(15, 0, 12, 12);
    r2.setRects(rects.constData(), rects.size());
    QTest::newRow("minus") << (r1 - r2);
}

void tst_QRegion::isEmpty()
{
    QFETCH(QRegion, region);

    QVERIFY(region.isEmpty());
    QCOMPARE(region, QRegion());
    QCOMPARE(region.rectCount(), 0);
    QCOMPARE(region.boundingRect(), QRect());
    QVERIFY(region.rects().isEmpty());
}

#if defined(Q_WS_X11) && defined(QT_BUILD_INTERNAL)
void tst_QRegion::clipRectangles()
{
    QRegion region(30, 30, 30, 30);
    int num = 0;
    qt_getClipRects(region, num);
    QCOMPARE(num, 1);

    region += QRegion(10, 10, 10, 10);
    XRectangle *rects2 = static_cast<XRectangle *>(qt_getClipRects(region, num));
    QCOMPARE(num, 2);

    // Here's the important part (Y-sorted):
    QCOMPARE(int(rects2[0].y), 10);
    QCOMPARE(int(rects2[1].y), 30);
}
#endif

void tst_QRegion::regionFromPath()
{
    {
        QPainterPath path;
        path.addRect(0, 0, 10, 10);
        path.addRect(0, 100, 100, 1000);

        QRegion rgn(path.toFillPolygon().toPolygon());
        QCOMPARE(rgn.rects().size(), 2);
        QCOMPARE(rgn.rects().at(0), QRect(0, 0, 10, 10));
        QCOMPARE(rgn.rects().at(1), QRect(0, 100, 100, 1000));

        QCOMPARE(rgn.boundingRect(), QRect(0, 0, 100, 1100));
    }

    {
        QPainterPath path;
        path.addRect(0, 0, 100, 100);
        path.addRect(10, 10, 80, 80);

        QRegion rgn(path.toFillPolygon().toPolygon());
        QCOMPARE(rgn.rects().size(), 4);

        QCOMPARE(rgn.rects().at(0), QRect(0, 0, 100, 10));
        QCOMPARE(rgn.rects().at(1), QRect(0, 10, 10, 80));
        QCOMPARE(rgn.rects().at(2), QRect(90, 10, 10, 80));
        QCOMPARE(rgn.rects().at(3), QRect(0, 90, 100, 10));

        QCOMPARE(rgn.boundingRect(), QRect(0, 0, 100, 100));
    }
}

Q_DECLARE_METATYPE(QPainterPath)

#ifdef QT_BUILD_INTERNAL
void tst_QRegion::regionToPath_data()
{
    QTest::addColumn<QPainterPath>("path");
    {
        QPainterPath path;
        path.addRect(QRect(0, 0, 10, 10));

        QTest::newRow("Rectangle") << path;
    }

    {
        QPainterPath path;
        path.addRect(QRect(0, 0, 10, 10));
        path.addRect(QRect(20, 0, 10, 10));

        QTest::newRow("Two rects") << path;
    }

    {
        QPainterPath path;
        path.addEllipse(QRect(0, 0, 10, 10));

        QTest::newRow("Ellipse") << path;
    }

    {
        QPainterPath path;
        path.addRect(QRect(0, 0, 3, 8));
        path.addRect(QRect(6, 0, 3, 8));
        path.addRect(QRect(3, 3, 3, 2));
        path.addRect(QRect(12, 3, 3, 2));

        QTest::newRow("H-dot") << path;
    }

    {
        QPainterPath path;
        for (int y = 0; y <= 10; ++y) {
            for (int x = 0; x <= 10; ++x) {
                if (!(y & 1) || ((x ^ y) & 1))
                    path.addRect(QRect(x, y, 1, 1));
            }
        }

        QTest::newRow("Grid") << path;
    }
}
#endif

#ifdef QT_BUILD_INTERNAL
QT_BEGIN_NAMESPACE
extern QPainterPath qt_regionToPath(const QRegion &region);
QT_END_NAMESPACE
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QRegion::regionToPath()
{

    QFETCH(QPainterPath, path);

    for (int i = 0; i < 360; i += 10) {

        QTransform transform;
        transform.scale(5, 5);
        transform.rotate(i);

        QPainterPath mapped = transform.map(path);
        QRegion region(mapped.toFillPolygon().toPolygon());

        QPainterPath a;
        a.addRegion(region);

        QPainterPath b = qt_regionToPath(region);

        QRect r = a.boundingRect().toAlignedRect();
        QImage ia(r.size(), QImage::Format_RGB32);
        ia.fill(0xffffffff);
        QImage ib = ia;

        QPainter p(&ia);
        p.translate(-r.x(), -r.y());
        p.fillPath(a, Qt::red);
        p.end();
        p.begin(&ib);
        p.translate(-r.x(), -r.y());
        p.fillPath(b, Qt::red);
        p.end();

        QCOMPARE(ia, ib);
        QCOMPARE(a.boundingRect(), b.boundingRect());
    }
}
#endif

QTEST_MAIN(tst_QRegion)
#include "tst_qregion.moc"
