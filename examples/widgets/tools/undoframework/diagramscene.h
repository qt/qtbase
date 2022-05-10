// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DIAGRAMSCENE_H
#define DIAGRAMSCENE_H

#include <QObject>
#include <QGraphicsScene>

class DiagramItem;
QT_BEGIN_NAMESPACE
class QGraphicsSceneDragDropEvent;
class QGraphicsViewItem;
QT_END_NAMESPACE

//! [0]
class DiagramScene : public QGraphicsScene
{
    Q_OBJECT

public:
    DiagramScene(QObject *parent = nullptr);

signals:
    void itemMoved(DiagramItem *movedItem, const QPointF &movedFromPosition);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QGraphicsItem *movingItem = nullptr;
    QPointF oldPos;
};
//! [0]

#endif
