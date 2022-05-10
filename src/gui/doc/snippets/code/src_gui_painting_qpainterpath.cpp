// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

namespace src_gui_painting_qpainterpath {
struct Wrapper : QPaintDevice
{
    Q_OBJECT

    void wrapper0();
    void wrapper1();
    void wrapper2();
    void wrapper3();
    void wrapper4();
    void wrapper5();
    void wrapper6();
    void wrapper7();
};

void Wrapper::wrapper0() {

//! [0]
QPainterPath path;
path.addRect(20, 20, 60, 60);

path.moveTo(0, 0);
path.cubicTo(99, 0,  50, 50,  99, 99);
path.cubicTo(0, 99,  50, 50,  0, 0);

QPainter painter(this);
painter.fillRect(0, 0, 100, 100, Qt::white);
painter.setPen(QPen(QColor(79, 106, 25), 1, Qt::SolidLine,
                    Qt::FlatCap, Qt::MiterJoin));
painter.setBrush(QColor(122, 163, 39));

painter.drawPath(path);
//! [0]

} // Wrapper::wrapper0


void Wrapper::wrapper1() {
const QPointF c1;
const QPointF c2;
const QPointF endPoint;

//! [1]
QLinearGradient myGradient;
QPen myPen;

QPainterPath myPath;
myPath.cubicTo(c1, c2, endPoint);

QPainter painter(this);
painter.setBrush(myGradient);
painter.setPen(myPen);
painter.drawPath(myPath);
//! [1]

} // Wrapper::wrapper1


void Wrapper::wrapper2() {
const QRectF boundingRect;
qreal startAngle = 0;
qreal sweepLength = 0;
QPointF center;
QLinearGradient myGradient;
QPen myPen;
//! [2]
QPainterPath myPath;
myPath.moveTo(center);
myPath.arcTo(boundingRect, startAngle,
             sweepLength);

QPainter painter(this);
painter.setBrush(myGradient);
painter.setPen(myPen);
painter.drawPath(myPath);
//! [2]

} // Wrapper::wrapper2


void Wrapper::wrapper3() {

//! [3]
QLinearGradient myGradient;
QPen myPen;
QRectF myRectangle;

QPainterPath myPath;
myPath.addRect(myRectangle);

QPainter painter(this);
painter.setBrush(myGradient);
painter.setPen(myPen);
painter.drawPath(myPath);
//! [3]

} // Wrapper::wrapper3


void Wrapper::wrapper4() {

//! [4]
QLinearGradient myGradient;
QPen myPen;
QPolygonF myPolygon;

QPainterPath myPath;
myPath.addPolygon(myPolygon);

QPainter painter(this);
painter.setBrush(myGradient);
painter.setPen(myPen);
painter.drawPath(myPath);
//! [4]

} // Wrapper::wrapper4


void Wrapper::wrapper5() {

//! [5]
QLinearGradient myGradient;
QPen myPen;
QRectF boundingRectangle;

QPainterPath myPath;
myPath.addEllipse(boundingRectangle);

QPainter painter(this);
painter.setBrush(myGradient);
painter.setPen(myPen);
painter.drawPath(myPath);
//! [5]

} // Wrapper::wrapper5


void Wrapper::wrapper6() {
qreal x = 0;
qreal y = 0;

//! [6]
QLinearGradient myGradient;
QPen myPen;
QFont myFont;
QPointF baseline(x, y);

QPainterPath myPath;
myPath.addText(baseline, myFont, tr("Qt"));

QPainter painter(this);
painter.setBrush(myGradient);
painter.setPen(myPen);
painter.drawPath(myPath);
//! [6]


} // Wrapper::wrapper6
} // src_gui_painting_qpainterpath
