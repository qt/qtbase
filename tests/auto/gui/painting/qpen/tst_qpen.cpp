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

#include "qpen.h"
#include "qbrush.h"

#include <qdebug.h>

class tst_QPen : public QObject
{
    Q_OBJECT

public:
    tst_QPen();

private slots:
    void getSetCheck();
    void swap();
    void operator_eq_eq();
    void operator_eq_eq_data();

    void stream();
    void stream_data();

    void constructor();
    void constructor_data();
};

// Testing get/set functions
void tst_QPen::getSetCheck()
{
    QPen obj1;
    // qreal QPen::miterLimit()
    // void QPen::setMiterLimit(qreal)
    obj1.setMiterLimit(0.0);
    QCOMPARE(0.0, obj1.miterLimit());
    obj1.setMiterLimit(qreal(1.1));
    QCOMPARE(qreal(1.1), obj1.miterLimit());

    // qreal QPen::widthF()
    // void QPen::setWidthF(qreal)
    obj1.setWidthF(0.0);
    QCOMPARE(0.0, obj1.widthF());
    obj1.setWidthF(qreal(1.1));
    QCOMPARE(qreal(1.1), obj1.widthF());

    // int QPen::width()
    // void QPen::setWidth(int)
    for (int i = 0; i < 100; ++i) {
        obj1.setWidth(i);
        QCOMPARE(i, obj1.width());
    }
}

void tst_QPen::swap()
{
    QPen p1(Qt::black), p2(Qt::white);
    p1.swap(p2);
    QCOMPARE(p1.color(), QColor(Qt::white));
    QCOMPARE(p2.color(), QColor(Qt::black));
}


tst_QPen::tst_QPen()

{
}

void tst_QPen::operator_eq_eq_data()
{
    QTest::addColumn<QPen>("pen1");
    QTest::addColumn<QPen>("pen2");
    QTest::addColumn<bool>("isEqual");

    QTest::newRow("differentColor") << QPen(Qt::red)
                                    << QPen(Qt::blue)
                                    << false;
    QTest::newRow("differentWidth") << QPen(Qt::red, 2)
                                    << QPen(Qt::red, 3)
                                    << false;
    QTest::newRow("differentPenStyle") << QPen(Qt::red, 2, Qt::DashLine)
                                       << QPen(Qt::red, 2, Qt::DotLine)
                                       << false;
    QTest::newRow("differentCapStyle") << QPen(Qt::red, 2, Qt::DashLine, Qt::RoundCap, Qt::BevelJoin)
                                       << QPen(Qt::red, 2, Qt::DashLine, Qt::SquareCap, Qt::BevelJoin)
                                       << false;
    QTest::newRow("differentJoinStyle") << QPen(Qt::red, 2, Qt::DashLine, Qt::RoundCap, Qt::BevelJoin)
                                        << QPen(Qt::red, 2, Qt::DashLine, Qt::RoundCap, Qt::MiterJoin)
                                        << false;
    QTest::newRow("same") << QPen(Qt::red, 2, Qt::DashLine, Qt::RoundCap, Qt::BevelJoin)
                          << QPen(Qt::red, 2, Qt::DashLine, Qt::RoundCap, Qt::BevelJoin)
                          << true;

}

void tst_QPen::operator_eq_eq()
{
    QFETCH(QPen, pen1);
    QFETCH(QPen, pen2);
    QFETCH(bool, isEqual);
    QCOMPARE(pen1 == pen2, isEqual);
}


void tst_QPen::constructor_data()
{
    QTest::addColumn<QPen>("pen");
    QTest::addColumn<QBrush>("brush");
    QTest::addColumn<double>("width");
    QTest::addColumn<int>("style");
    QTest::addColumn<int>("capStyle");
    QTest::addColumn<int>("joinStyle");

    QTest::newRow("solid_black") << QPen() << QBrush(Qt::black) << 1. << (int)Qt::SolidLine
                              << (int) Qt::SquareCap << (int)Qt::BevelJoin;
    QTest::newRow("solid_red") << QPen(Qt::red) << QBrush(Qt::red) << 1. << (int)Qt::SolidLine
                            << (int)Qt::SquareCap << (int)Qt::BevelJoin;
    QTest::newRow("full") << QPen(QBrush(QLinearGradient(0, 0, 100, 100)), 10,
                               Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin)
                       << QBrush(QLinearGradient(0, 0, 100, 100)) << 10. << (int)Qt::SolidLine
                       << (int)Qt::RoundCap << (int)Qt::MiterJoin;

}


void tst_QPen::constructor()
{
    QFETCH(QPen, pen);
    QFETCH(QBrush, brush);
    QFETCH(double, width);
    QFETCH(int, style);
    QFETCH(int, capStyle);
    QFETCH(int, joinStyle);

    QCOMPARE(pen.style(), Qt::PenStyle(style));
    QCOMPARE(pen.capStyle(), Qt::PenCapStyle(capStyle));
    QCOMPARE(pen.joinStyle(), Qt::PenJoinStyle(joinStyle));
    QCOMPARE(pen.widthF(), width);
    QCOMPARE(pen.brush(), brush);
}


void tst_QPen::stream_data()
{
    QTest::addColumn<QPen>("pen");

    QTest::newRow("solid_black") << QPen();
    QTest::newRow("solid_red") << QPen(Qt::red);
    QTest::newRow("full") << QPen(QBrush(QLinearGradient(0, 0, 100, 100)), 10, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin);
}


void tst_QPen::stream()
{
    QFETCH(QPen, pen);

    QByteArray bytes;

    {
        QDataStream stream(&bytes, QIODevice::WriteOnly);
        stream << pen;
    }

    QPen cmp;
    {
        QDataStream stream(&bytes, QIODevice::ReadOnly);
        stream >> cmp;
    }

    QCOMPARE(pen.widthF(), cmp.widthF());
    QCOMPARE(pen.style(), cmp.style());
    QCOMPARE(pen.capStyle(), cmp.capStyle());
    QCOMPARE(pen.joinStyle(), cmp.joinStyle());
    QCOMPARE(pen.brush(), cmp.brush());

    QCOMPARE(pen, cmp);
}

QTEST_APPLESS_MAIN(tst_QPen)
#include "tst_qpen.moc"
