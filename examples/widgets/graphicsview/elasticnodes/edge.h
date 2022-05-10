// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef EDGE_H
#define EDGE_H

#include <QGraphicsItem>

class Node;

//! [0]
class Edge : public QGraphicsItem
{
public:
    Edge(Node *sourceNode, Node *destNode);

    Node *sourceNode() const;
    Node *destNode() const;

    void adjust();

    enum { Type = UserType + 2 };
    int type() const override { return Type; }

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    Node *source, *dest;

    QPointF sourcePoint;
    QPointF destPoint;
    qreal arrowSize = 10;
};
//! [0]

#endif // EDGE_H
