// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtMath>
#include <QtWidgets>

#include "mouse.h"

static constexpr int MouseCount = 7;

//! [0]
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
//! [0]

//! [1]
    QGraphicsScene scene;
    scene.setSceneRect(-300, -300, 600, 600);
//! [1] //! [2]
    scene.setItemIndexMethod(QGraphicsScene::NoIndex);
//! [2]

//! [3]
    for (int i = 0; i < MouseCount; ++i) {
        Mouse *mouse = new Mouse;
        mouse->setPos(::sin((i * 6.28) / MouseCount) * 200,
                      ::cos((i * 6.28) / MouseCount) * 200);
        scene.addItem(mouse);
    }
//! [3]

//! [4]
    QGraphicsView view(&scene);
    view.setRenderHint(QPainter::Antialiasing);
    view.setBackgroundBrush(QPixmap(":/images/cheese.jpg"));
//! [4] //! [5]
    view.setCacheMode(QGraphicsView::CacheBackground);
    view.setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    view.setDragMode(QGraphicsView::ScrollHandDrag);
//! [5] //! [6]
    view.setWindowTitle(QT_TRANSLATE_NOOP(QGraphicsView, "Colliding Mice"));
    view.resize(400, 300);
    view.show();

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &scene, &QGraphicsScene::advance);
    timer.start(1000 / 33);

    return app.exec();
}
//! [6]
