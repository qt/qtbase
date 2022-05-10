// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "graphwidget.h"

#include <QApplication>
#include <QTime>
#include <QMainWindow>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    GraphWidget *widget = new GraphWidget;

    QMainWindow mainWindow;
    mainWindow.setCentralWidget(widget);

    mainWindow.show();
    return app.exec();
}
