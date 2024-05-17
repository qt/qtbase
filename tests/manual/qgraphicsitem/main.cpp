// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMessageBox>

class MyObject : public QObject
{
public:
    MyObject(QGraphicsItem *i, QObject *parent = nullptr) : QObject(parent), itemToToggle(i)
    {
        startTimer(500);
    }
protected:
    void timerEvent(QTimerEvent *)
    {
        itemToToggle->setVisible(!itemToToggle->isVisible());
    }
private:
    QGraphicsItem *itemToToggle;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QGraphicsView gv;
    QGraphicsScene *scene = new QGraphicsScene(&gv);
    gv.setScene(scene);
    QGraphicsItem *rect = scene->addRect(0, 0, 200, 200, QPen(Qt::NoPen), QBrush(Qt::yellow));
    rect->setFlag(QGraphicsItem::ItemHasNoContents);
    rect->setFlag(QGraphicsItem::ItemClipsChildrenToShape);

    QGraphicsItem *childRect = scene->addRect(0, 0, 100, 100, QPen(Qt::NoPen), QBrush(Qt::red));
    childRect->setParentItem(rect);
    gv.show();
    MyObject o(rect);
    QMessageBox::information(0, "What you should see",
                             "The red rectangle should toggle visibility, so you should see it flash on and off");
    return a.exec();
}
