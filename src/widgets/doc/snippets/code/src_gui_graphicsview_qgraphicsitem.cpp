// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

//! [0]
class SimpleItem : public QGraphicsItem
{
public:
    QRectF boundingRect() const override
    {
        qreal penWidth = 1;
        return QRectF(-10 - penWidth / 2, -10 - penWidth / 2,
                      20 + penWidth, 20 + penWidth);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override
    {
        painter->drawRoundedRect(-10, -10, 20, 20, 5, 5);
    }
};
//! [0]


//! [1]
class Item : public QGraphicsItem
{
public:
   enum { Type = UserType + 1 };

   int type() const override
   {
       // Enable the use of qgraphicsitem_cast with this item.
       return Type;
   }
   //...
};
//! [1]

void example(QGraphicsItem *item, QGraphicsView *view)
{
    {
        //! [2]
        item->setCursor(Qt::IBeamCursor);
        //! [2]
    }

    {
        //! [3]
        item->setCursor(Qt::IBeamCursor);
        //! [3]
    }

    {
        //! [4]
        QGraphicsRectItem rect;
        rect.setPos(100, 100);

        rect.sceneTransform().map(QPointF(0, 0));
        // returns QPointF(100, 100);

        rect.sceneTransform().inverted().map(QPointF(100, 100));
        // returns QPointF(0, 0);
        //! [4]
    }

    {
        //! [5]
        QGraphicsRectItem rect;
        rect.setPos(100, 100);

        rect.deviceTransform(view->viewportTransform()).map(QPointF(0, 0));
        // returns the item's (0, 0) point in view's viewport coordinates

        rect.deviceTransform(view->viewportTransform()).inverted().map(QPointF(100, 100));
        // returns view's viewport's (100, 100) coordinate in item coordinates
        //! [5]
    }
}

class CustomItem : public QGraphicsItem
{
 public:
    CustomItem();
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;
    QPainterPath shape() const override;
    void setRadius(qreal newRadius);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
 private:
    qreal radius;
    qreal diameter;

    void setAcceptDrops(bool on);
};

using RoundRectItem = CustomItem;
using RoundItem = CustomItem;
using CircleItem = CustomItem;
using Component = CustomItem;

//! [8]
QRectF CircleItem::boundingRect() const
{
    qreal penWidth = 1;
    return QRectF(-radius - penWidth / 2, -radius - penWidth / 2,
                  diameter + penWidth, diameter + penWidth);
}
//! [8]

//! [9]
QPainterPath RoundItem::shape() const
{
    QPainterPath path;
    path.addEllipse(boundingRect());
    return path;
}
//! [9]

//! [10]
void RoundRectItem::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem *option,
                          QWidget *widget)
{
    painter->drawRoundedRect(-10, -10, 20, 20, 5, 5);
}
//! [10]

void data_snippet(QGraphicsScene &scene, QTransform &transform)
{
    //! [11]
    static const int ObjectName = 0;

    QGraphicsItem *item = scene.itemAt(100, 50, transform);
    if (item->data(ObjectName).toString().isEmpty()) {
        if (qgraphicsitem_cast<CustomItem *>(item))
            item->setData(ObjectName, "Custom");
    }
    //! [11]
}

void wrapperFunction()
{
    //! [12]
    QGraphicsScene scene;
    QGraphicsEllipseItem *ellipse = scene.addEllipse(QRectF(-10, -10, 20, 20));
    QGraphicsLineItem *line = scene.addLine(QLineF(-10, -10, 20, 20));

    line->installSceneEventFilter(ellipse);
    // line's events are filtered by ellipse's sceneEventFilter() function.

    ellipse->installSceneEventFilter(line);
    // ellipse's events are filtered by line's sceneEventFilter() function.
    //! [12]
}

//! [13]
void CustomItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QMenu menu;
    QAction *removeAction = menu.addAction("Remove");
    QAction *markAction = menu.addAction("Mark");
    QAction *selectedAction = menu.exec(event->screenPos());
    // ...
}
//! [13]


//! [14]
CustomItem::CustomItem()
{
    setAcceptDrops(true);
    //...
}

void CustomItem::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    event->setAccepted(event->mimeData()->hasFormat("text/plain"));
}
//! [14]


//! [15]
QVariant Component::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && scene()) {
        // value is the new position.
        QPointF newPos = value.toPointF();
        QRectF rect = scene()->sceneRect();
        if (!rect.contains(newPos)) {
            // Keep the item inside the scene rect.
            newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
            newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
            return newPos;
        }
    }
    return QGraphicsItem::itemChange(change, value);
}
//! [15]

//! [16]
void CircleItem::setRadius(qreal newRadius)
{
    if (radius != newRadius) {
        prepareGeometryChange();
        radius = newRadius;
    }
}
//! [16]

void snippets( QGraphicsScene *scene, QGraphicsItem *item,
               QGraphicsView *view, const QRectF &rect, int dx, int dy)
{
    //! [17]
    // Group all selected items together
    QGraphicsItemGroup *group = scene->createItemGroup(scene->selectedItems());

    // Destroy the group, and delete the group item
    scene->destroyItemGroup(group);
    //! [17]

    //! [19]
    QTransform xform = item->deviceTransform(view->viewportTransform());
    QRect deviceRect = xform.mapRect(rect).toAlignedRect();
    view->viewport()->scroll(dx, dy, deviceRect);
    //! [19]

    //! [20]
    // A 50x30 rectangle whose top-left corner is at (100, 100) in the scene
    QGraphicsRectItem *rectItem = scene->addRect(QRectF(0, 0, 50, 30));
    rectItem->setPos(100, 100);
    //! [20]
}

/* For convenient quoting in the documentation
//! [18]
class QGraphicsPathItem : public QAbstractGraphicsShapeItem
{
 public:
  enum { Type = 2 };
    int type() const override { return Type; }
  //...
};
//! [18]
*/
