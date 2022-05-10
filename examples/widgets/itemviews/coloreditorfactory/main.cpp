// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

#include "window.h"

int main(int argv, char **args)
{
    QApplication app(argv, args);

    Window window;
    window.show();

    return app.exec();
}
