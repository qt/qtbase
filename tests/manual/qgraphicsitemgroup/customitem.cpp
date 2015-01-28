/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
        CustomGroup* group = qgraphicsitem_cast<CustomGroup*>(item);
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
