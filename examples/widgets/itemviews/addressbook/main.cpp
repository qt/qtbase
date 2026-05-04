// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"

#include <QApplication>

#include <QScreen>

#include <QRect>

//! [0]
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow mw;
    const auto availableGeometry = mw.screen()->availableGeometry();
    mw.resize(availableGeometry.width() / 3, availableGeometry.height() / 3);
    mw.show();
    return QApplication::exec();
}
//! [0]
