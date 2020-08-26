/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
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
