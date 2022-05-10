// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Settings Editor");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    MainWindow mainWin;
    mainWin.show();
    return app.exec();
}
