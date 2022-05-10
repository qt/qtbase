// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CHIP_H
#define CHIP_H

#include <QColor>
#include <QGraphicsItem>

class Chip : public QGraphicsItem
{
public:
    Chip(const QColor &color, int x, int y);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    int x;
    int y;
    QColor color;
    QList<QPointF> stuff;
};

#endif // CHIP_H
