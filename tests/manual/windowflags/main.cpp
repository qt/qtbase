// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QStringList>

#include "controllerwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QStringList arguments = QCoreApplication::arguments();
    arguments.pop_front();

    ControllerWindow controller;
    if (!arguments.contains(QLatin1String("-l")))
        LogWidget::install();
    if (!arguments.contains(QLatin1String("-e")))
        controller.registerEventFilter();
    controller.show();
    controller.lower();
    return app.exec();
}
