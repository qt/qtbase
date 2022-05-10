// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QtGui>

class CustomScene : public QGraphicsScene
{
public:
    CustomScene()
        { addItem(new QGraphicsEllipseItem(QRect(10, 10, 30, 30))); }

    void drawItems(QPainter *painter, int numItems, QGraphicsItem *items[],
                   const QStyleOptionGraphicsItem options[],
                   QWidget *widget = nullptr) override;
};

//! [0]
void CustomScene::drawItems(QPainter *painter, int numItems,
                            QGraphicsItem *items[],
                            const QStyleOptionGraphicsItem options[],
                            QWidget *widget)
{
    for (int i = 0; i < numItems; ++i) {
         // Draw the item
         painter->save();
         painter->setTransform(items[i]->sceneTransform(), true);
         items[i]->paint(painter, &options[i], widget);
         painter->restore();
     }
}
//! [0]
