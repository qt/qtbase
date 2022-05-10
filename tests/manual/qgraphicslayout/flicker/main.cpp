// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QMainWindow>
#include <QApplication>
#include "window.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Window *window = new Window();
    window->resize(800, 600);

    window->show();

    return app.exec();

}
