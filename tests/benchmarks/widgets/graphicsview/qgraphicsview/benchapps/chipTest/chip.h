// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CHIP_H
#define CHIP_H

#include <QtGui/QColor>
#include <QtGui/QGraphicsItem>

class Chip : public QGraphicsItem
{
public:
    Chip(const QColor &color, int x, int y);

    QRectF boundingRect() const;
    QPainterPath shape() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    int x, y;
    QColor color;
    QList<QPointF> stuff;
};

#endif
