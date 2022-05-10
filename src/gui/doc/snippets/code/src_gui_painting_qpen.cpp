// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QPainter>
#include <QPen>

namespace src_gui_painting_qpen {
struct Wrapper : QPaintDevice
{
    void wrapper0();
    void wrapper1();
    void wrapper2();
    void wrapper3();
};

void Wrapper::wrapper0() {

//! [0]
QPainter painter(this);
QPen pen(Qt::green, 3, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
painter.setPen(pen);
//! [0]

} // Wrapper::wrapper0


void Wrapper::wrapper1() {

//! [1]
QPainter painter(this);
QPen pen;  // creates a default pen

pen.setStyle(Qt::DashDotLine);
pen.setWidth(3);
pen.setBrush(Qt::green);
pen.setCapStyle(Qt::RoundCap);
pen.setJoinStyle(Qt::RoundJoin);

painter.setPen(pen);
//! [1]

} // Wrapper::wrapper1


void Wrapper::wrapper2() {

//! [2]
QPen pen;
QList<qreal> dashes;
qreal space = 4;

dashes << 1 << space << 3 << space << 9 << space
           << 27 << space << 9 << space;

pen.setDashPattern(dashes);
//! [2]

} // Wrapper::wrapper2


void Wrapper::wrapper3() {
//! [3]
QPen pen;
QList<qreal> dashes;
qreal space = 4;
dashes << 1 << space << 3 << space << 9 << space
           << 27 << space << 9 << space;
pen.setDashPattern(dashes);
//! [3]

} // Wrapper::wrapper3
} // src_gui_painting_qpen
