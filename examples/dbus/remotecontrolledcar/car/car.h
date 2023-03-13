// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CAR_H
#define CAR_H

#include <QGraphicsObject>
#include <QBrush>

class Car : public QGraphicsObject
{
    Q_OBJECT
public:
    Car();
    QRectF boundingRect() const override;

public slots:
    void accelerate();
    void decelerate();
    void turnLeft();
    void turnRight();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;
    void timerEvent(QTimerEvent *event) override;

private:
    QBrush color = Qt::green;
    qreal wheelsAngle = 0; // used when applying rotation
    qreal speed = 0; // delta movement along the body axis
};

#endif // CAR_H
