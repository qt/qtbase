// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QPainter>
#include <QWidget>

namespace doc_src_coordsys {
struct SnippetWrappers : public QWidget
{
    void wrapper0();
    void wrapper1();
    void wrapper2();
    void wrapper3();
    void wrapper4();
};


void SnippetWrappers::wrapper0() {

//! [0]
QPainter painter(this);

painter.setPen(Qt::darkGreen);
// Using the (x  y  w  h) overload
painter.drawRect(1, 2, 6, 4);
//! [0]

} // wrapper0


void SnippetWrappers::wrapper1() {

//! [1]
QPainter painter(this);

painter.setPen(Qt::darkGreen);
painter.drawLine(2, 7, 6, 1);
//! [1]

} // wrapper2


void SnippetWrappers::wrapper2() {

//! [2]
QPainter painter(this);
painter.setRenderHint(
    QPainter::Antialiasing);
painter.setPen(Qt::darkGreen);
// Using the (x  y  w  h) overload
painter.drawRect(1, 2, 6, 4);
//! [2]

} // wrapper2


void SnippetWrappers::wrapper3() {

//! [3]
QPainter painter(this);
painter.setRenderHint(
    QPainter::Antialiasing);
painter.setPen(Qt::darkGreen);
painter.drawLine(2, 7, 6, 1);
//! [3]

} // wrapper3


void SnippetWrappers::wrapper4() {

//! [4]
QPainter painter(this);
painter.setWindow(QRect(-50, -50, 100, 100));
//! [4]


//! [5]
int side = qMin(width(), height());
int x = (width() - side / 2);
int y = (height() - side / 2);

painter.setViewport(x, y, side, side);
//! [5]

} // wrapper4

} // doc_src_coordsys
