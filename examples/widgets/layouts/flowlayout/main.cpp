// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [1]
#include <QApplication>

#include "window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Window window;
    window.show();
    return app.exec();
}
//! [1]
