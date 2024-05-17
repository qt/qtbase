// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>
#include "controllerwidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ControllerWidget controller;
    controller.show();
    return a.exec();
}
