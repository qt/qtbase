// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "tabletapplication.h"
#include "tabletcanvas.h"

//! [0]
int main(int argv, char *args[])
{
    TabletApplication app(argv, args);
    TabletCanvas *canvas = new TabletCanvas;
    app.setCanvas(canvas);

    MainWindow mainWindow(canvas);
    mainWindow.resize(500, 500);
    mainWindow.show();
    return app.exec();
}
//! [0]
