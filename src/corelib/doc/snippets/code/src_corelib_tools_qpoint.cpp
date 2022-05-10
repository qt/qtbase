// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QPoint p;

p.setX(p.x() + 1);
p += QPoint(1, 0);
p.rx()++;
//! [0]


//! [1]
QPoint p(1, 2);
p.rx()--;   // p becomes (0, 2)
//! [1]


//! [2]
QPoint p(1, 2);
p.ry()++;   // p becomes (1, 3)
//! [2]


//! [3]
QPoint p( 3, 7);
QPoint q(-1, 4);
p += q;    // p becomes (2, 11)
//! [3]


//! [4]
QPoint p( 3, 7);
QPoint q(-1, 4);
p -= q;    // p becomes (4, 3)
//! [4]


//! [5]
QPoint p(-1, 4);
p *= 2.5;    // p becomes (-3, 10)
//! [5]


//! [16]
QPoint p( 3, 7);
QPoint q(-1, 4);
int dotProduct = QPoint::dotProduct(p, q);   // dotProduct becomes 25
//! [16]


//! [6]
QPoint p(-3, 10);
p /= 2.5;           // p becomes (-1, 4)
//! [6]


//! [7]
QPoint oldPosition;

MyWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint point = event->pos() - oldPosition;
    if (point.manhattanLength() > 3)
        // the mouse has moved more than 3 pixels since the oldPosition
}
//! [7]


//! [8]
double trueLength = std::sqrt(std::pow(x(), 2) + std::pow(y(), 2));
//! [8]


//! [9]
QPointF p;

p.setX(p.x() + 1.0);
p += QPointF(1.0, 0.0);
p.rx()++;
//! [9]


//! [10]
 QPointF p(1.1, 2.5);
 p.rx()--;   // p becomes (0.1, 2.5)
//! [10]


//! [11]
QPointF p(1.1, 2.5);
p.ry()++;   // p becomes (1.1, 3.5)
//! [11]


//! [12]
QPointF p( 3.1, 7.1);
QPointF q(-1.0, 4.1);
p += q;    // p becomes (2.1, 11.2)
//! [12]


//! [13]
QPointF p( 3.1, 7.1);
QPointF q(-1.0, 4.1);
p -= q;    // p becomes (4.1, 3.0)
//! [13]


//! [14]
QPointF p(-1.1, 4.1);
p *= 2.5;    // p becomes (-2.75, 10.25)
//! [14]


//! [15]
QPointF p(-2.75, 10.25);
p /= 2.5;           // p becomes (-1.1, 4.1)
//! [15]


//! [17]
QPointF p( 3.1, 7.1);
QPointF q(-1.0, 4.1);
qreal dotProduct = QPointF::dotProduct(p, q);   // dotProduct becomes 26.01
//! [17]
