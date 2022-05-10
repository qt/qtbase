// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>

#include "coloritem.h"
#include "robot.h"

#include <cmath>


class GraphicsView : public QGraphicsView
{
public:
    using QGraphicsView::QGraphicsView;

protected:
    void resizeEvent(QResizeEvent *) override
    {
    }
};

//! [0]
int main(int argc, char **argv)
{
    QApplication app(argc, argv);

//! [0]
//! [1]
    QGraphicsScene scene(-200, -200, 400, 400);

    for (int i = 0; i < 10; ++i) {
        ColorItem *item = new ColorItem;
        item->setPos(::sin((i * 6.28) / 10.0) * 150,
                     ::cos((i * 6.28) / 10.0) * 150);

        scene.addItem(item);
    }

    Robot *robot = new Robot;
    robot->setTransform(QTransform::fromScale(1.2, 1.2), true);
    robot->setPos(0, -20);
    scene.addItem(robot);
//! [1]
//! [2]
    GraphicsView view(&scene);
    view.setRenderHint(QPainter::Antialiasing);
    view.setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    view.setBackgroundBrush(QColor(230, 200, 167));
    view.setWindowTitle("Drag and Drop Robot");
    view.show();

    return app.exec();
}
//! [2]
