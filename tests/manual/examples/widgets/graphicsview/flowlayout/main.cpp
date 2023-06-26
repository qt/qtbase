// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [1]
#include "window.h"

#include <QApplication>
#include <QGraphicsView>


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    Window *w = new Window;
    scene.addItem(w);

    view.resize(400, 300);
    view.show();

    return app.exec();
}
//! [1]
