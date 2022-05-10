// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtGui/QGuiApplication>
#include "qopenglcontextwindow.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QOpenGLContextWindow window;
    window.resize(300, 300);
    window.show();

    return app.exec();
}
