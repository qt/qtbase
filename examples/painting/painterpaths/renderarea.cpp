/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include "renderarea.h"

//! [0]
RenderArea::RenderArea(const QPainterPath &path, QWidget *parent)
    : QWidget(parent), path(path)
{
    penWidth = 1;
    rotationAngle = 0;
    setBackgroundRole(QPalette::Base);
}
//! [0]

//! [1]
QSize RenderArea::minimumSizeHint() const
{
    return QSize(50, 50);
}
//! [1]

//! [2]
QSize RenderArea::sizeHint() const
{
    return QSize(100, 100);
}
//! [2]

//! [3]
void RenderArea::setFillRule(Qt::FillRule rule)
{
    path.setFillRule(rule);
    update();
}
//! [3]

//! [4]
void RenderArea::setFillGradient(const QColor &color1, const QColor &color2)
{
    fillColor1 = color1;
    fillColor2 = color2;
    update();
}
//! [4]

//! [5]
void RenderArea::setPenWidth(int width)
{
    penWidth = width;
    update();
}
//! [5]

//! [6]
void RenderArea::setPenColor(const QColor &color)
{
    penColor = color;
    update();
}
//! [6]

//! [7]
void RenderArea::setRotationAngle(int degrees)
{
    rotationAngle = degrees;
    update();
}
//! [7]

//! [8]
void RenderArea::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
//! [8] //! [9]
    painter.scale(width() / 100.0, height() / 100.0);
    painter.translate(50.0, 50.0);
    painter.rotate(-rotationAngle);
    painter.translate(-50.0, -50.0);

//! [9] //! [10]
    painter.setPen(QPen(penColor, penWidth, Qt::SolidLine, Qt::RoundCap,
                        Qt::RoundJoin));
    QLinearGradient gradient(0, 0, 0, 100);
    gradient.setColorAt(0.0, fillColor1);
    gradient.setColorAt(1.0, fillColor2);
    painter.setBrush(gradient);
    painter.drawPath(path);
}
//! [10]
