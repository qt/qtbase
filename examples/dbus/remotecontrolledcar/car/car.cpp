// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "car.h"
#include <QtWidgets/QtWidgets>
#include <cmath>

QRectF Car::boundingRect() const
{
    return QRectF(-35, -81, 70, 115);
}

Car::Car()
{
    startTimer(1000 / 33);
    setFlags(ItemIsMovable | ItemIsFocusable);
}

void Car::accelerate()
{
    if (speed < 10)
        ++speed;
}

void Car::decelerate()
{
    if (speed > -10)
        --speed;
}

void Car::turnLeft()
{
    if (wheelsAngle > -30)
        wheelsAngle -= 5;
}

void Car::turnRight()
{
    if (wheelsAngle < 30)
       wheelsAngle += 5;
}

void Car::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setBrush(Qt::gray);
    painter->drawRect(-20, -58, 40, 2); // front axel
    painter->drawRect(-20, 7, 40, 2); // rear axel

    painter->setBrush(color);
    painter->drawRect(-25, -79, 50, 10); // front wing

    painter->drawEllipse(-25, -48, 50, 20); // side pods
    painter->drawRect(-25, -38, 50, 35); // side pods
    painter->drawRect(-5, 9, 10, 10); // back pod

    painter->drawEllipse(-10, -81, 20, 100); // main body

    painter->drawRect(-17, 19, 34, 15); // rear wing

    painter->setBrush(Qt::black);
    painter->drawPie(-5, -51, 10, 15, 0, 180 * 16);
    painter->drawRect(-5, -44, 10, 10); // cocpit

    painter->save();
    painter->translate(-20, -58);
    painter->rotate(wheelsAngle);
    painter->drawRect(-10, -7, 10, 15); // front left
    painter->restore();

    painter->save();
    painter->translate(20, -58);
    painter->rotate(wheelsAngle);
    painter->drawRect(0, -7, 10, 15); // front left
    painter->restore();

    painter->drawRect(-30, 0, 12, 17); // rear left
    painter->drawRect(19, 0, 12, 17);  // rear right
}

void Car::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

    const qreal axelDistance = 54;
    qreal wheelsAngleRads = qDegreesToRadians(wheelsAngle);
    qreal turnDistance = std::cos(wheelsAngleRads) * axelDistance * 2;
    qreal turnRateRads = wheelsAngleRads / turnDistance;  // rough estimate
    qreal turnRate = qRadiansToDegrees(turnRateRads);
    qreal rotation = speed * turnRate;

    setTransform(QTransform().rotate(rotation), true);
    setTransform(QTransform::fromTranslate(0, -speed), true);

    if (!scene()->views().isEmpty()) {
        QRect viewRect = scene()->views().at(0)->sceneRect().toRect();
        QTransform fx = transform();
        qreal dx = fx.dx();
        qreal dy = fx.dy();
        while (dx < viewRect.left() - 10)
            dx += viewRect.width();
        while (dy < viewRect.top() - 10)
            dy += viewRect.height();
        while (dx > viewRect.right() + 10)
            dx -= viewRect.width();
        while (dy > viewRect.bottom() + 10)
            dy -= viewRect.width();
        setTransform(QTransform(fx.m11(), fx.m12(), fx.m13(),
                                fx.m21(), fx.m22(), fx.m23(),
                                dx, dy, fx.m33()));
    }

    update();
}
