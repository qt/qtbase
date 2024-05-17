// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore>
#include <QtWidgets>

#include "window.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    Window window;
    window.show();


    return app.exec();
}



