// Copyright (C) 2022 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "application.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    Application app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
