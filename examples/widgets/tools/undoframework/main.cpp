// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "mainwindow.h"

//! [0]
int main(int argv, char *args[])
{
    QApplication app(argv, args);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
//! [0]
