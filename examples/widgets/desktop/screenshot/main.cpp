// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QScreen>

#include "screenshot.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Screenshot screenshot;
    screenshot.move(screenshot.screen()->availableGeometry().topLeft() + QPoint(20, 20));
    screenshot.show();

    return app.exec();
}
