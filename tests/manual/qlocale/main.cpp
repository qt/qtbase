// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>

#include "window.h"

int main(int argv, char *args[])
{
    QApplication app(argv, args);
    Window window;
    window.show();
    return app.exec();
}
