// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QPolygon>
namespace polygon {

void wrapper() {
{
// STREAM
//! [0]
QPolygon polygon;
polygon << QPoint(10, 20) << QPoint(20, 30);
//! [0]
}

{
// STREAMF
//! [1]
QPolygonF polygon;
polygon << QPointF(10.4, 20.5) << QPointF(20.2, 30.2);
//! [1]
}

{
// SETPOINTS
//! [2]
static const int points[] = { 10, 20, 30, 40 };
QPolygon polygon;
polygon.setPoints(2, points);
//! [2]
}

{
// SETPOINTS2
//! [3]
QPolygon polygon;
polygon.setPoints(2, 10, 20, 30, 40);
//! [3]
}

{
// PUTPOINTS
//! [4]
QPolygon polygon(1);
polygon[0] = QPoint(4, 5);
polygon.putPoints(1, 2, 6,7, 8,9);
//! [4]
}

{
// PUTPOINTS2
//! [5]
QPolygon polygon(3);
polygon.putPoints(0, 3, 4,5, 0,0, 8,9);
polygon.putPoints(1, 1, 6,7);
//! [5]
}

{
// PUTPOINTS3
//! [6]
QPolygon polygon1;
polygon1.putPoints(0, 3, 1,2, 0,0, 5,6);
// polygon1 is now the three-point polygon(1,2, 0,0, 5,6);

QPolygon polygon2;
polygon2.putPoints(0, 3, 4,4, 5,5, 6,6);
// polygon2 is now (4,4, 5,5, 6,6);

polygon1.putPoints(2, 3, polygon2);
// polygon1 is now the five-point polygon(1,2, 0,0, 4,4, 5,5, 6,6);
//! [6]
}

} // wrapper
} // polygon
