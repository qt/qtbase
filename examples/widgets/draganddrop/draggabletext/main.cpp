// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "dragwidget.h"

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(draggabletext);

    QApplication app(argc, argv);
    DragWidget window;
    window.show();
    return app.exec();
}
