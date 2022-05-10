// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtGui>

#include "valgrind/callgrind.h"

class ItemMover : public QObject
{
    Q_OBJECT
public:
    ItemMover(QGraphicsItem *item)
        : _item(item)
    {
        startTimer(0);
    }

protected:
    void timerEvent(QTimerEvent *event)
    {
        _item->moveBy(-1, 0);
    }

private:
    QGraphicsItem *_item;
};

class ClipItem : public QGraphicsRectItem
{
public:
    ClipItem(qreal x, qreal y, qreal w, qreal h, const QPen &pen, const QBrush &brush)
        : QGraphicsRectItem(x, y, w, h)
    {
        setPen(pen);
        setBrush(brush);
    }

    QPainterPath shape() const
    {
        QPainterPath path;
        path.addRect(rect());
        return path;
    }
};

class CountView : public QGraphicsView
{
protected:
    void paintEvent(QPaintEvent *event)
    {
        static int n = 0;
        if (n)
            CALLGRIND_START_INSTRUMENTATION
        QGraphicsView::paintEvent(event);
        if (n)
            CALLGRIND_STOP_INSTRUMENTATION
        if (++n == 500)
            qApp->quit();
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QGraphicsScene scene;
    scene.setItemIndexMethod(QGraphicsScene::NoIndex);

    ClipItem *clipItem = new ClipItem(0, 0, 100, 100, QPen(), QBrush(Qt::blue));
    clipItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    clipItem->setData(0, "clipItem");
    scene.addItem(clipItem);

    QGraphicsRectItem *scrollItem = scene.addRect(0, 0, 10, 10, QPen(Qt::NoPen), QBrush(Qt::NoBrush));
    scrollItem->setParentItem(clipItem);
    scrollItem->setFlag(QGraphicsItem::ItemIsMovable);
    scrollItem->setData(0, "scrollItem");

    for (int y = 0; y < 25; ++y) {
        for (int x = 0; x < 25; ++x) {
            ClipItem *rect = new ClipItem(0, 0, 90, 20, QPen(Qt::NoPen), QBrush(Qt::green));
            rect->setParentItem(scrollItem);
            rect->setPos(x * 95, y * 25);
            rect->setData(0, qPrintable(QString("rect %1 %2").arg(x).arg(y)));
            rect->setFlag(QGraphicsItem::ItemClipsChildrenToShape);

            QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(-5, -5, 10, 10);
            ellipse->setPen(QPen(Qt::NoPen));
            ellipse->setBrush(QBrush(Qt::yellow));
            ellipse->setParentItem(rect);
            ellipse->setData(0, qPrintable(QString("ellipse %1 %2").arg(x).arg(y)));
        }
    }

    scrollItem->setRect(scrollItem->childrenBoundingRect());

#if 0
    ItemMover mover(scrollItem);
#endif

    CountView view;
    view.setScene(&scene);
    view.setSceneRect(-25, -25, 150, 150);
    view.resize(300, 300);
    view.show();

    return app.exec();
}

#include "main.moc"
