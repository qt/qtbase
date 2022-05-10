// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

#include "bubble.h"

Bubble::Bubble(const QPointF &position, qreal radius, const QPointF &velocity)
    : position(position), vel(velocity), radius(radius)
{
    innerColor = randomColor();
    outerColor = randomColor();
    updateBrush();
}

//! [0]
void Bubble::updateCache()
{
    delete cache;
    cache = new QImage(qRound(radius * 2 + 2), qRound(radius * 2 + 2), QImage::Format_ARGB32_Premultiplied);
    cache->fill(0x00000000);
    QPainter p(cache);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::white);
    pen.setWidth(2);
    p.setPen(pen);
    p.setBrush(brush);
    p.drawEllipse(1, 1, int(2*radius), int(2*radius));
}
//! [0]

Bubble::~Bubble()
{
    delete cache;
}

void Bubble::updateBrush()
{
    QRadialGradient gradient(QPointF(radius, radius), radius,
                             QPointF(radius*0.5, radius*0.5));

    gradient.setColorAt(0, QColor(255, 255, 255, 255));
    gradient.setColorAt(0.25, innerColor);
    gradient.setColorAt(1, outerColor);
    brush = QBrush(gradient);
    updateCache();
}

//! [1]
void Bubble::drawBubble(QPainter *painter)
{
    painter->save();
    painter->translate(position.x() - radius, position.y() - radius);
    painter->setOpacity(0.8);
    painter->drawImage(0, 0, *cache);
    painter->restore();
}
//! [1]

QColor Bubble::randomColor()
{
    int red = int(185 + QRandomGenerator::global()->bounded(70));
    int green = int(185 + QRandomGenerator::global()->bounded(70));
    int blue = int(205 + QRandomGenerator::global()->bounded(50));
    int alpha = int(91 + QRandomGenerator::global()->bounded(100));

    return QColor(red, green, blue, alpha);
}

void Bubble::move(const QRect &bbox)
{
    position += vel;
    qreal leftOverflow = position.x() - radius - bbox.left();
    qreal rightOverflow = position.x() + radius - bbox.right();
    qreal topOverflow = position.y() - radius - bbox.top();
    qreal bottomOverflow = position.y() + radius - bbox.bottom();

    if (leftOverflow < 0.0) {
        position.setX(position.x() - 2 * leftOverflow);
        vel.setX(-vel.x());
    } else if (rightOverflow > 0.0) {
        position.setX(position.x() - 2 * rightOverflow);
        vel.setX(-vel.x());
    }

    if (topOverflow < 0.0) {
        position.setY(position.y() - 2 * topOverflow);
        vel.setY(-vel.y());
    } else if (bottomOverflow > 0.0) {
        position.setY(position.y() - 2 * bottomOverflow);
        vel.setY(-vel.y());
    }
}

QRectF Bubble::rect()
{
    return QRectF(position.x() - radius, position.y() - radius,
                  2 * radius, 2 * radius);
}
