// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtGui>
#include "rasterwindow.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    RasterWindow window;
    window.show();

    return app.exec();
}
