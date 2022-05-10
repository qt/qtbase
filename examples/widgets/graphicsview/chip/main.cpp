// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(images);

    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    MainWindow window;
    window.show();

    return app.exec();
}
