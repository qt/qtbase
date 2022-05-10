// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qopengltextureblitwindow.h"
#include <QtGui/QGuiApplication>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QOpenGLTextureBlitWindow window;
    window.show();

    return app.exec();
}
