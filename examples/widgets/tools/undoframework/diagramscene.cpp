// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "diagramscene.h"
#include "diagramitem.h"

#include <QGraphicsSceneMouseEvent>

DiagramScene::DiagramScene(QObject *parent)
    : QGraphicsScene(parent)
{}

void DiagramScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF mousePos(event->buttonDownScenePos(Qt::LeftButton).x(),
                     event->buttonDownScenePos(Qt::LeftButton).y());
    const QList<QGraphicsItem *> itemList = items(mousePos);
    movingItem = itemList.isEmpty() ? nullptr : itemList.first();

    if (movingItem != nullptr && event->button() == Qt::LeftButton)
        oldPos = movingItem->pos();

    clearSelection();
    QGraphicsScene::mousePressEvent(event);
}

void DiagramScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (movingItem != nullptr && event->button() == Qt::LeftButton) {
        if (oldPos != movingItem->pos())
            emit itemMoved(qgraphicsitem_cast<DiagramItem *>(movingItem),
                           oldPos);
        movingItem = nullptr;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}
