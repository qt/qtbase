// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow *window1 = new MainWindow;
    MainWindow *window2 = new MainWindow;
    window1->show();
    window2->show();
    return app.exec();
}
