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
    QRectF boundingRect() const;

public slots:
    void accelerate();
    void decelerate();
    void turnLeft();
    void turnRight();

signals:
    void crashed();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr);
    void timerEvent(QTimerEvent *event);

private:
    QBrush color;
    qreal wheelsAngle; // used when applying rotation
    qreal speed; // delta movement along the body axis
};

#endif // CAR_H
