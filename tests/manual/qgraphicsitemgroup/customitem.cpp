/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "customitem.h"

#include <QPainter>
#include <QStyle>
#include <QStyleOption>

QList<CustomGroup*> CustomScene::selectedCustomGroups() const
{
    QList<QGraphicsItem*> all = selectedItems();
    QList<CustomGroup*> groups;

    foreach (QGraphicsItem *item, all) {
        CustomGroup* group = dynamic_cast<CustomGroup*>(item);
        if (group)
            groups.append(group);
    }

    return groups;
}

QList<CustomItem*> CustomScene::selectedCustomItems() const
{
    QList<QGraphicsItem*> all = selectedItems();
    QList<CustomItem*> items;

    foreach (QGraphicsItem *item, all) {
        CustomItem* citem = dynamic_cast<CustomItem*>(item);
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

void CustomGroup::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
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
