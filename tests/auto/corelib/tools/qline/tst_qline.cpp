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
#include <qline.h>
#include <math.h>

#ifndef M_2PI
#define M_2PI 6.28318530717958647692528676655900576
#endif

class tst_QLine : public QObject
{
    Q_OBJECT
private slots:
    void testIntersection();
    void testIntersection_data();

    void testLength();
    void testLength_data();

    void testNormalVector();
    void testNormalVector_data();

    void testAngle();
    void testAngle_data();

    void testAngle2();
    void testAngle2_data();

    void testAngle3();

    void testAngleTo();
    void testAngleTo_data();

    void testSet();
};

// Square root of two
#define SQRT2 1.4142135623731

// Length of unit vector projected to x from 45 degrees
#define UNITX_45 0.707106781186547

const qreal epsilon = sizeof(qreal) == sizeof(double) ? 1e-8 : 1e-4;

void tst_QLine::testSet()
{
    {
        QLine l;
        l.setP1(QPoint(1, 2));
        l.setP2(QPoint(3, 4));

        QCOMPARE(l.x1(), 1);
        QCOMPARE(l.y1(), 2);
        QCOMPARE(l.x2(), 3);
        QCOMPARE(l.y2(), 4);

        l.setPoints(QPoint(5, 6), QPoint(7, 8));
        QCOMPARE(l.x1(), 5);
        QCOMPARE(l.y1(), 6);
        QCOMPARE(l.x2(), 7);
        QCOMPARE(l.y2(), 8);

        l.setLine(9, 10, 11, 12);
        QCOMPARE(l.x1(), 9);
        QCOMPARE(l.y1(), 10);
        QCOMPARE(l.x2(), 11);
        QCOMPARE(l.y2(), 12);
    }

    {
        QLineF l;
        l.setP1(QPointF(1, 2));
        l.setP2(QPointF(3, 4));

        QCOMPARE(l.x1(), 1.0);
        QCOMPARE(l.y1(), 2.0);
        QCOMPARE(l.x2(), 3.0);
        QCOMPARE(l.y2(), 4.0);

        l.setPoints(QPointF(5, 6), QPointF(7, 8));
        QCOMPARE(l.x1(), 5.0);
        QCOMPARE(l.y1(), 6.0);
        QCOMPARE(l.x2(), 7.0);
        QCOMPARE(l.y2(), 8.0);

        l.setLine(9.0, 10.0, 11.0, 12.0);
        QCOMPARE(l.x1(), 9.0);
        QCOMPARE(l.y1(), 10.0);
        QCOMPARE(l.x2(), 11.0);
        QCOMPARE(l.y2(), 12.0);
    }

}

void tst_QLine::testIntersection_data()
{
    QTest::addColumn<double>("xa1");
    QTest::addColumn<double>("ya1");
    QTest::addColumn<double>("xa2");
    QTest::addColumn<double>("ya2");
    QTest::addColumn<double>("xb1");
    QTest::addColumn<double>("yb1");
    QTest::addColumn<double>("xb2");
    QTest::addColumn<double>("yb2");
    QTest::addColumn<int>("type");
    QTest::addColumn<double>("ix");
    QTest::addColumn<double>("iy");

    QTest::newRow("parallel") << 1.0 << 1.0 << 3.0 << 4.0
                           << 5.0 << 6.0 << 7.0 << 9.0
                           << int(QLineF::NoIntersection) << 0.0 << 0.0;
    QTest::newRow("unbounded") << 1.0 << 1.0 << 5.0 << 5.0
                            << 0.0 << 4.0 << 3.0 << 4.0
                            << int(QLineF::UnboundedIntersection) << 4.0 << 4.0;
    QTest::newRow("bounded") << 1.0 << 1.0 << 5.0 << 5.0
                          << 0.0 << 4.0 << 5.0 << 4.0
                          << int(QLineF::BoundedIntersection) << 4.0 << 4.0;

    QTest::newRow("almost vertical") << 0.0 << 10.0 << 20.0000000000001 << 10.0
                                     << 10.0 << 0.0 << 10.0 << 20.0
                                     << int(QLineF::BoundedIntersection) << 10.0 << 10.0;

    QTest::newRow("almost horizontal") << 0.0 << 10.0 << 20.0 << 10.0
                                       << 10.0000000000001 << 0.0 << 10.0 << 20.0
                                       << int(QLineF::BoundedIntersection) << 10.0 << 10.0;

    QTest::newRow("long vertical") << 100.1599256468623
                                   << 100.7861905065196
                                   << 100.1599256468604
                                   << -9999.78619050651
                                   << 10.0 << 50.0 << 190.0 << 50.0
                                   << int(QLineF::BoundedIntersection)
                                   << 100.1599256468622
                                   << 50.0;

    QLineF baseA(0, -50, 0, 50);
    QLineF baseB(-50, 0, 50, 0);

    for (int i = 0; i < 1000; ++i) {
        QLineF a = QLineF::fromPolar(50, i);
        a.setP1(-a.p2());

        QLineF b = QLineF::fromPolar(50, i * 0.997 + 90);
        b.setP1(-b.p2());

        // make the qFuzzyCompare be a bit more lenient
        a = a.translated(1, 1);
        b = b.translated(1, 1);

        QTest::newRow(qPrintable(QString::fromLatin1("rotation-%0").arg(i)))
            << (double)a.x1() << (double)a.y1() << (double)a.x2() << (double)a.y2()
            << (double)b.x1() << (double)b.y1() << (double)b.x2() << (double)b.y2()
            << int(QLineF::BoundedIntersection)
            << 1.0
            << 1.0;
    }
}

void tst_QLine::testIntersection()
{
    QFETCH(double, xa1);
    QFETCH(double, ya1);
    QFETCH(double, xa2);
    QFETCH(double, ya2);
    QFETCH(double, xb1);
    QFETCH(double, yb1);
    QFETCH(double, xb2);
    QFETCH(double, yb2);
    QFETCH(int, type);
    QFETCH(double, ix);
    QFETCH(double, iy);

    QLineF a(xa1, ya1, xa2, ya2);
    QLineF b(xb1, yb1, xb2, yb2);


    QPointF ip;
    QLineF::IntersectType itype = a.intersect(b, &ip);

    QCOMPARE(int(itype), type);
    if (type != QLineF::NoIntersection) {
        QVERIFY(qAbs(ip.x() - ix) < epsilon);
        QVERIFY(qAbs(ip.y() - iy) < epsilon);
    }
}

void tst_QLine::testLength_data()
{
    QTest::addColumn<double>("x1");
    QTest::addColumn<double>("y1");
    QTest::addColumn<double>("x2");
    QTest::addColumn<double>("y2");
    QTest::addColumn<double>("length");
    QTest::addColumn<double>("lengthToSet");
    QTest::addColumn<double>("vx");
    QTest::addColumn<double>("vy");

    QTest::newRow("[1,0]*2") << 0.0 << 0.0 << 1.0 << 0.0 << 1.0 << 2.0 << 2.0 << 0.0;
    QTest::newRow("[0,1]*2") << 0.0 << 0.0 << 0.0 << 1.0 << 1.0 << 2.0 << 0.0 << 2.0;
    QTest::newRow("[-1,0]*2") << 0.0 << 0.0 << -1.0 << 0.0 << 1.0 << 2.0 << -2.0 << 0.0;
    QTest::newRow("[0,-1]*2") << 0.0 << 0.0 << 0.0 << -1.0 << 1.0 << 2.0 << 0.0 << -2.0;
    QTest::newRow("[1,1]->|1|") << 0.0 << 0.0 << 1.0 << 1.0
                             << double(SQRT2) << 1.0 << double(UNITX_45) << double(UNITX_45);
    QTest::newRow("[-1,1]->|1|") << 0.0 << 0.0 << -1.0 << 1.0
                             << double(SQRT2) << 1.0 << double(-UNITX_45) << double(UNITX_45);
    QTest::newRow("[1,-1]->|1|") << 0.0 << 0.0 << 1.0 << -1.0
                             << double(SQRT2) << 1.0 << double(UNITX_45) << double(-UNITX_45);
    QTest::newRow("[-1,-1]->|1|") << 0.0 << 0.0 << -1.0 << -1.0
                             << double(SQRT2) << 1.0 << double(-UNITX_45) << double(-UNITX_45);
    QTest::newRow("[1,0]*2 (2,2)") << 2.0 << 2.0 << 3.0 << 2.0 << 1.0 << 2.0 << 2.0 << 0.0;
    QTest::newRow("[0,1]*2 (2,2)") << 2.0 << 2.0 << 2.0 << 3.0 << 1.0 << 2.0 << 0.0 << 2.0;
    QTest::newRow("[-1,0]*2 (2,2)") << 2.0 << 2.0 << 1.0 << 2.0 << 1.0 << 2.0 << -2.0 << 0.0;
    QTest::newRow("[0,-1]*2 (2,2)") << 2.0 << 2.0 << 2.0 << 1.0 << 1.0 << 2.0 << 0.0 << -2.0;
    QTest::newRow("[1,1]->|1| (2,2)") << 2.0 << 2.0 << 3.0 << 3.0
                                   << double(SQRT2) << 1.0 << double(UNITX_45) << double(UNITX_45);
    QTest::newRow("[-1,1]->|1| (2,2)") << 2.0 << 2.0 << 1.0 << 3.0
                                    << double(SQRT2) << 1.0 << double(-UNITX_45) << double(UNITX_45);
    QTest::newRow("[1,-1]->|1| (2,2)") << 2.0 << 2.0 << 3.0 << 1.0
                                    << double(SQRT2) << 1.0 << double(UNITX_45) << double(-UNITX_45);
    QTest::newRow("[-1,-1]->|1| (2,2)") << 2.0 << 2.0 << 1.0 << 1.0
                                     << double(SQRT2) << 1.0 << double(-UNITX_45) << double(-UNITX_45);
}

void tst_QLine::testLength()
{
    QFETCH(double, x1);
    QFETCH(double, y1);
    QFETCH(double, x2);
    QFETCH(double, y2);
    QFETCH(double, length);
    QFETCH(double, lengthToSet);
    QFETCH(double, vx);
    QFETCH(double, vy);

    QLineF l(x1, y1, x2, y2);
    QCOMPARE(l.length(), qreal(length));

    l.setLength(lengthToSet);
    QCOMPARE(l.length(), qreal(lengthToSet));
    QCOMPARE(l.dx(), qreal(vx));
    QCOMPARE(l.dy(), qreal(vy));
}


void tst_QLine::testNormalVector_data()
{
    QTest::addColumn<double>("x1");
    QTest::addColumn<double>("y1");
    QTest::addColumn<double>("x2");
    QTest::addColumn<double>("y2");
    QTest::addColumn<double>("nvx");
    QTest::addColumn<double>("nvy");

    QTest::newRow("[1, 0]") << 0.0 << 0.0 << 1.0 << 0.0 << 0.0 << -1.0;
    QTest::newRow("[-1, 0]") << 0.0 << 0.0 << -1.0 << 0.0 << 0.0 << 1.0;
    QTest::newRow("[0, 1]") << 0.0 << 0.0 << 0.0 << 1.0 << 1.0 << 0.0;
    QTest::newRow("[0, -1]") << 0.0 << 0.0 << 0.0 << -1.0 << -1.0 << 0.0;
    QTest::newRow("[2, 3]") << 2.0 << 3.0 << 4.0 << 6.0 << 3.0 << -2.0;
}

void tst_QLine::testNormalVector()
{
    QFETCH(double, x1);
    QFETCH(double, y1);
    QFETCH(double, x2);
    QFETCH(double, y2);
    QFETCH(double, nvx);
    QFETCH(double, nvy);

    QLineF l(x1, y1, x2, y2);
    QLineF n = l.normalVector();

    QCOMPARE(l.x1(), n.x1());
    QCOMPARE(l.y1(), n.y1());

    QCOMPARE(n.dx(), qreal(nvx));
    QCOMPARE(n.dy(), qreal(nvy));
}

void tst_QLine::testAngle_data()
{
    QTest::addColumn<double>("xa1");
    QTest::addColumn<double>("ya1");
    QTest::addColumn<double>("xa2");
    QTest::addColumn<double>("ya2");
    QTest::addColumn<double>("xb1");
    QTest::addColumn<double>("yb1");
    QTest::addColumn<double>("xb2");
    QTest::addColumn<double>("yb2");
    QTest::addColumn<double>("angle");

    QTest::newRow("parallel") << 1.0 << 1.0 << 3.0 << 4.0
                           << 5.0 << 6.0 << 7.0 << 9.0
                           << 0.0;
    QTest::newRow("[4,4]-[4,0]") << 1.0 << 1.0 << 5.0 << 5.0
                              << 0.0 << 4.0 << 3.0 << 4.0
                              << 45.0;
    QTest::newRow("[4,4]-[-4,0]") << 1.0 << 1.0 << 5.0 << 5.0
                              << 3.0 << 4.0 << 0.0 << 4.0
                              << 135.0;

    for (int i=0; i<180; ++i) {
        QTest::newRow(QString("angle:%1").arg(i).toLatin1())
            << 0.0 << 0.0 << double(cos(i*M_2PI/360)) << double(sin(i*M_2PI/360))
            << 0.0 << 0.0 << 1.0 << 0.0
            << double(i);
    }
}

void tst_QLine::testAngle()
{
    QFETCH(double, xa1);
    QFETCH(double, ya1);
    QFETCH(double, xa2);
    QFETCH(double, ya2);
    QFETCH(double, xb1);
    QFETCH(double, yb1);
    QFETCH(double, xb2);
    QFETCH(double, yb2);
    QFETCH(double, angle);

    QLineF a(xa1, ya1, xa2, ya2);
    QLineF b(xb1, yb1, xb2, yb2);

    double resultAngle = a.angle(b);
    QCOMPARE(qRound(resultAngle), qRound(angle));
}

void tst_QLine::testAngle2_data()
{
    QTest::addColumn<qreal>("x1");
    QTest::addColumn<qreal>("y1");
    QTest::addColumn<qreal>("x2");
    QTest::addColumn<qreal>("y2");
    QTest::addColumn<qreal>("angle");

    QTest::newRow("right") << qreal(0.0) << qreal(0.0) << qreal(10.0) << qreal(0.0) << qreal(0.0);
    QTest::newRow("left") << qreal(0.0) << qreal(0.0) << qreal(-10.0) << qreal(0.0) << qreal(180.0);
    QTest::newRow("up") << qreal(0.0) << qreal(0.0) << qreal(0.0) << qreal(-10.0) << qreal(90.0);
    QTest::newRow("down") << qreal(0.0) << qreal(0.0) << qreal(0.0) << qreal(10.0) << qreal(270.0);

    QTest::newRow("diag a") << qreal(0.0) << qreal(0.0) << qreal(10.0) << qreal(-10.0) << qreal(45.0);
    QTest::newRow("diag b") << qreal(0.0) << qreal(0.0) << qreal(-10.0) << qreal(-10.0) << qreal(135.0);
    QTest::newRow("diag c") << qreal(0.0) << qreal(0.0) << qreal(-10.0) << qreal(10.0) << qreal(225.0);
    QTest::newRow("diag d") << qreal(0.0) << qreal(0.0) << qreal(10.0) << qreal(10.0) << qreal(315.0);
}

void tst_QLine::testAngle2()
{
    QFETCH(qreal, x1);
    QFETCH(qreal, y1);
    QFETCH(qreal, x2);
    QFETCH(qreal, y2);
    QFETCH(qreal, angle);

    QLineF line(x1, y1, x2, y2);
    QCOMPARE(line.angle(), angle);

    QLineF polar = QLineF::fromPolar(line.length(), angle);

    QVERIFY(qAbs(line.x1() - polar.x1()) < epsilon);
    QVERIFY(qAbs(line.y1() - polar.y1()) < epsilon);
    QVERIFY(qAbs(line.x2() - polar.x2()) < epsilon);
    QVERIFY(qAbs(line.y2() - polar.y2()) < epsilon);
}

void tst_QLine::testAngle3()
{
    for (int i = -720; i <= 720; ++i) {
        QLineF line(0, 0, 100, 0);
        line.setAngle(i);
        const int expected = (i + 720) % 360;

        QVERIFY2(qAbs(line.angle() - qreal(expected)) < epsilon, qPrintable(QString::fromLatin1("value: %1").arg(i)));

        QCOMPARE(line.length(), qreal(100.0));

        QCOMPARE(QLineF::fromPolar(100.0, i), line);
    }
}

void tst_QLine::testAngleTo()
{
    QFETCH(qreal, xa1);
    QFETCH(qreal, ya1);
    QFETCH(qreal, xa2);
    QFETCH(qreal, ya2);
    QFETCH(qreal, xb1);
    QFETCH(qreal, yb1);
    QFETCH(qreal, xb2);
    QFETCH(qreal, yb2);
    QFETCH(qreal, angle);

    QLineF a(xa1, ya1, xa2, ya2);
    QLineF b(xb1, yb1, xb2, yb2);

    const qreal resultAngle = a.angleTo(b);
    QVERIFY(qAbs(resultAngle - angle) < epsilon);

    a.translate(b.p1() - a.p1());
    a.setAngle(a.angle() + resultAngle);
    a.setLength(b.length());

    QCOMPARE(a, b);
}

void tst_QLine::testAngleTo_data()
{
    QTest::addColumn<qreal>("xa1");
    QTest::addColumn<qreal>("ya1");
    QTest::addColumn<qreal>("xa2");
    QTest::addColumn<qreal>("ya2");
    QTest::addColumn<qreal>("xb1");
    QTest::addColumn<qreal>("yb1");
    QTest::addColumn<qreal>("xb2");
    QTest::addColumn<qreal>("yb2");
    QTest::addColumn<qreal>("angle");

    QTest::newRow("parallel") << qreal(1.0) << qreal(1.0) << qreal(3.0) << qreal(4.0)
                           << qreal(5.0) << qreal(6.0) << qreal(7.0) << qreal(9.0)
                           << qreal(0.0);
    QTest::newRow("[4,4]-[4,0]") << qreal(1.0) << qreal(1.0) << qreal(5.0) << qreal(5.0)
                              << qreal(0.0) << qreal(4.0) << qreal(3.0) << qreal(4.0)
                              << qreal(45.0);
    QTest::newRow("[4,4]-[-4,0]") << qreal(1.0) << qreal(1.0) << qreal(5.0) << qreal(5.0)
                              << qreal(3.0) << qreal(4.0) << qreal(0.0) << qreal(4.0)
                              << qreal(225.0);

    for (int i = 0; i < 360; ++i) {
        const QLineF l = QLineF::fromPolar(1, i);
        QTest::newRow(QString("angle:%1").arg(i).toLatin1())
            << qreal(0.0) << qreal(0.0) << qreal(1.0) << qreal(0.0)
            << qreal(0.0) << qreal(0.0) << l.p2().x() << l.p2().y()
            << qreal(i);
    }
}

QTEST_MAIN(tst_QLine)
#include "tst_qline.moc"
