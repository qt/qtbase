// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"

#include <QApplication>
#include <QScreen>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    const auto screenSize = window.screen()->availableSize();
    window.resize({screenSize.width() / 2, screenSize.height() * 2 / 3});
    window.show();
    return QCoreApplication::exec();
}
