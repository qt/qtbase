// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>


#include "controller.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Controller controller;
    controller.show();
    return app.exec();
}
