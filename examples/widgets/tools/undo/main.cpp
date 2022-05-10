// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include "mainwindow.h"

int main(int argc, char **argv)
{
    Q_INIT_RESOURCE(undo);

    QApplication app(argc, argv);

    MainWindow win;
    win.resize(800, 600);
    win.show();

    return app.exec();
}
