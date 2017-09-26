/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    if (QGraphicsItem *item = itemAt(event->pos())) {
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

