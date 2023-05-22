// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>
#include "window.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Window w;

    w.resize(400, 400);
    w.show();

    return app.exec();
}
