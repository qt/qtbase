// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QGraphicsView>

#include "knob.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QGraphicsScene scene;
    QGraphicsView view(&scene);

    Knob *knob1 = new Knob;
    knob1->setPos(-110, 0);
    Knob *knob2 = new Knob;

    scene.addItem(knob1);
    scene.addItem(knob2);

    view.showMaximized();
    view.fitInView(scene.sceneRect().adjusted(-20, -20, 20, 20), Qt::KeepAspectRatio);

    return app.exec();
}
