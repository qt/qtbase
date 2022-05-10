// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
  main.cpp

  A simple example that shows how selections can be used directly on a model.
  It shows the result of some selections made using a table view.
*/

#include <QApplication>

#include "window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QMainWindow *window = new MainWindow;
    window->show();
    window->resize(640, 480);
    return app.exec();
}
