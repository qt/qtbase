// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "renderarea.h"

#include <QPainter>

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
    painter.setPen(QPen(penColor, penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    QLinearGradient gradient(0, 0, 0, 100);
    gradient.setColorAt(0.0, fillColor1);
    gradient.setColorAt(1.0, fillColor2);
    painter.setBrush(gradient);
    painter.drawPath(path);
}
//! [10]
