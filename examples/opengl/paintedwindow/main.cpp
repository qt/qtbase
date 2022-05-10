// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QRect>

#include "paintedwindow.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    PaintedWindow window;
    window.show();

    return app.exec();
}

