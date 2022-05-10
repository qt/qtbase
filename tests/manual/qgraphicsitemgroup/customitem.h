// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CUSTOMITEM_H
#define CUSTOMITEM_H

#include <QGraphicsItem>
#include <QBrush>
#include <QGraphicsScene>

class CustomGroup : public QGraphicsItemGroup
{
public:
    CustomGroup();
    virtual ~CustomGroup() { }

protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual QRectF boundingRect() const;
};

class CustomItem : public QGraphicsRectItem
{
public:
    CustomItem(qreal x, qreal y, qreal width, qreal height, const QBrush & brush = QBrush());
    virtual ~CustomItem() { }
};

class CustomScene : public QGraphicsScene
{
    Q_OBJECT
public:
    CustomScene() : QGraphicsScene() { }

    QList<CustomItem*> selectedCustomItems() const;
    QList<CustomGroup*> selectedCustomGroups() const;
};

#endif // CUSTOMITEM_H
