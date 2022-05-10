// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QMainWindow>
#include "triviswidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMainWindow wnd;
    wnd.resize(1280, 800);
    wnd.setCentralWidget(new TriangulationVisualizer);
    wnd.show();

    return app.exec();
}
