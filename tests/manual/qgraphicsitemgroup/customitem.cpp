// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "customitem.h"

#include <QPainter>
#include <QStyle>
#include <QStyleOption>

QList<CustomGroup*> CustomScene::selectedCustomGroups() const
{
    const QList<QGraphicsItem*> all = selectedItems();
    QList<CustomGroup*> groups;

    for (QGraphicsItem *item : all) {
        CustomGroup* group = qgraphicsitem_cast<CustomGroup*>(item);
        if (group)
            groups.append(group);
    }

    return groups;
}

QList<CustomItem*> CustomScene::selectedCustomItems() const
{
    const QList<QGraphicsItem*> all = selectedItems();
    QList<CustomItem*> items;

    for (QGraphicsItem *item : all) {
        CustomItem* citem = qgraphicsitem_cast<CustomItem*>(item);
        if (citem)
            items.append(citem);
    }

    return items;
}

CustomGroup::CustomGroup() :
    QGraphicsItemGroup()
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
}

void CustomGroup::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    if (option->state & QStyle::State_Selected)
        painter->setOpacity(1.);
    else
        painter->setOpacity(0.2);

    painter->setPen(QPen(QColor(100, 100, 100), 2, Qt::DashLine));
    painter->drawRect(boundingRect().adjusted(-2, -2, 2, 2));
}

QRectF CustomGroup::boundingRect() const
{
    return QGraphicsItemGroup::boundingRect().adjusted(-4, -4, 4 ,4);
}

CustomItem::CustomItem(qreal x, qreal y, qreal width, qreal height, const QBrush &brush) :
    QGraphicsRectItem(x, y, width, height)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setBrush(brush);
    setPen(Qt::NoPen);
    setTransformOriginPoint(boundingRect().center());
}
