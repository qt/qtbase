// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MOUSE_H
#define MOUSE_H

#include <QGraphicsItem>

//! [0]
class Mouse : public QGraphicsItem
{
public:
    Mouse();

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

protected:
    void advance(int step) override;

private:
    qreal angle = 0;
    qreal speed = 0;
    qreal mouseEyeDirection = 0;
    QColor color;
};
//! [0]

#endif
