// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QLabel>
#include <QSurfaceFormat>
#include <utility>

#include "../joint.h"

#ifndef QT_NO_OPENGL
#include "mainwidget.h"
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);

    app.setApplicationName("Tpose");
    app.setApplicationVersion("0.1");
#ifndef QT_NO_OPENGL
    std::string fileName = "walk1.bvh";
    MainWidget* widget = new MainWidget(fileName);
    widget->show();
#else
    QLabel note("OpenGL Support required");
    note.show();
#endif
    return app.exec();
}