// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtGui>

#include "valgrind/callgrind.h"

class View : public QGraphicsView
{
    Q_OBJECT
public:
    View(QGraphicsScene *scene, QGraphicsItem *item)
        : QGraphicsView(scene), _item(item)
    {
    }

protected:
    void paintEvent(QPaintEvent *event)
    {
        static int n = 0;
        if (n)
            CALLGRIND_START_INSTRUMENTATION
        QGraphicsView::paintEvent(event);
        _item->moveBy(1, 1);
        if (n)
            CALLGRIND_STOP_INSTRUMENTATION
        if (++n == 200)
            qApp->quit();
    }

private:
    QGraphicsItem *_item;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if (argc < 2) {
        qDebug("usage: ./%s <numItems>", argv[0]);
        return 1;
    }

    QGraphicsScene scene(-150, -150, 300, 300);
    scene.setItemIndexMethod(QGraphicsScene::NoIndex);

    QGraphicsRectItem *item = scene.addRect(-50, -50, 100, 100, QPen(Qt::NoPen), QBrush(Qt::blue));
    item->setFlag(QGraphicsItem::ItemIsMovable);

    for (int i = 0; i < atoi(argv[1]); ++i) {
        QGraphicsRectItem *child = scene.addRect(-5, -5, 10, 10, QPen(Qt::NoPen), QBrush(Qt::blue));
        child->setPos(-50 + QRandomGenerator::global()->bounded(100), -50 + QRandomGenerator::global()->bounded(100));
        child->setParentItem(item);
    }

    View view(&scene, item);
    view.resize(300, 300);
    view.show();

    return app.exec();
}

#include "main.moc"
