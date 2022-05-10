// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "circlewidget.h"

#include <QPainter>

#include <stdlib.h>

//! [0]
CircleWidget::CircleWidget(QWidget *parent)
    : QWidget(parent)
{
    floatBased = false;
    antialiased = false;
    frameNo = 0;

    setBackgroundRole(QPalette::Base);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
//! [0]

//! [1]
void CircleWidget::setFloatBased(bool floatBased)
{
    this->floatBased = floatBased;
    update();
}
//! [1]

//! [2]
void CircleWidget::setAntialiased(bool antialiased)
{
    this->antialiased = antialiased;
    update();
}
//! [2]

//! [3]
QSize CircleWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}
//! [3]

//! [4]
QSize CircleWidget::sizeHint() const
{
    return QSize(180, 180);
}
//! [4]

//! [5]
void CircleWidget::nextAnimationFrame()
{
    ++frameNo;
    update();
}
//! [5]

//! [6]
void CircleWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, antialiased);
    painter.translate(width() / 2, height() / 2);
//! [6]

//! [7]
    for (int diameter = 0; diameter < 256; diameter += 9) {
        int delta = abs((frameNo % 128) - diameter / 2);
        int alpha = 255 - (delta * delta) / 4 - diameter;
//! [7] //! [8]
        if (alpha > 0) {
            painter.setPen(QPen(QColor(0, diameter / 2, 127, alpha), 3));

            if (floatBased)
                painter.drawEllipse(QRectF(-diameter / 2.0, -diameter / 2.0, diameter, diameter));
            else
                painter.drawEllipse(QRect(-diameter / 2, -diameter / 2, diameter, diameter));
        }
    }
}
//! [8]
