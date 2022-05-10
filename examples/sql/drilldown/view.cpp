// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "informationwindow.h"
#include "imageitem.h"
#include "view.h"

//! [0]
View::View(const QString &items, const QString &images, QWidget *parent)
    : QGraphicsView(parent)
{
    itemTable = new QSqlRelationalTableModel(this);
    itemTable->setTable(items);
    itemTable->setRelation(1, QSqlRelation(images, "itemid", "file"));
    itemTable->select();
//! [0]

//! [1]
    scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, 465, 365);
    setScene(scene);

    addItems();

    setMinimumSize(470, 370);
    setMaximumSize(470, 370);

    QLinearGradient gradient(QPointF(0, 0), QPointF(0, 370));
    gradient.setColorAt(0, QColor("#868482"));
    gradient.setColorAt(1, QColor("#5d5b59"));
    setBackgroundBrush(gradient);
}
//! [1]

//! [3]
void View::addItems()
{
    int itemCount = itemTable->rowCount();

    int imageOffset = 150;
    int leftMargin = 70;
    int topMargin = 40;

    for (int i = 0; i < itemCount; i++) {
        QSqlRecord record = itemTable->record(i);

        int id = record.value("id").toInt();
        QString file = record.value("file").toString();
        QString item = record.value("itemtype").toString();

        int columnOffset = ((i % 2) * 37);
        int x = ((i % 2) * imageOffset) + leftMargin + columnOffset;
        int y = ((i / 2) * imageOffset) + topMargin;

        ImageItem *image = new ImageItem(id, QPixmap(":/" + file));
        image->setData(0, i);
        image->setPos(x, y);
        scene->addItem(image);

        QGraphicsTextItem *label = scene->addText(item);
        label->setDefaultTextColor(QColor("#d7d6d5"));
        QPointF labelOffset((120 - label->boundingRect().width()) / 2, 120.0);
        label->setPos(QPointF(x, y) + labelOffset);
    }
}
//! [3]

//! [5]
void View::mouseReleaseEvent(QMouseEvent *event)
{
    if (QGraphicsItem *item = itemAt(event->position().toPoint())) {
        if (ImageItem *image = qgraphicsitem_cast<ImageItem *>(item))
            showInformation(image);
    }
    QGraphicsView::mouseReleaseEvent(event);
}
//! [5]

//! [6]
void View::showInformation(ImageItem *image)
{
    int id = image->id();
    if (id < 0 || id >= itemTable->rowCount())
        return;

    InformationWindow *window = findWindow(id);
    if (!window) {
        window = new InformationWindow(id, itemTable, this);

        connect(window, QOverload<int,const QString &>::of(&InformationWindow::imageChanged),
                this, QOverload<int,const QString &>::of(&View::updateImage));

        window->move(pos() + QPoint(20, 40));
        window->show();
        informationWindows.append(window);
    }

    if (window->isVisible()) {
        window->raise();
        window->activateWindow();
    } else
        window->show();
}
//! [6]

//! [7]
void View::updateImage(int id, const QString &fileName)
{
    QList<QGraphicsItem *> items = scene->items();

    while(!items.empty()) {
        QGraphicsItem *item = items.takeFirst();

        if (ImageItem *image = qgraphicsitem_cast<ImageItem *>(item)) {
            if (image->id() == id){
                image->setPixmap(QPixmap(":/" +fileName));
                image->adjust();
                break;
            }
        }
    }
}
//! [7]

//! [8]
InformationWindow *View::findWindow(int id) const
{
    for (auto window : informationWindows) {
        if (window && (window->id() == id))
            return window;
    }
    return nullptr;
}
//! [8]

