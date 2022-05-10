// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QDebug>
#include "tabletwidget.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    bool mouseToo = false;
    if (app.arguments().contains(QLatin1String("--nomouse")) || app.arguments().contains(QLatin1String("-nomouse")))
        mouseToo = false;
    else if (app.arguments().contains(QLatin1String("--mouse")) || app.arguments().contains(QLatin1String("-mouse")))
        mouseToo = true;
    if (mouseToo)
        qDebug() << "will show mouse events coming from the tablet as well as QTabletEvents";
    else
        qDebug() << "will not show mouse events from the tablet; use the --mouse option to enable";
    TabletWidget tabletWidget(mouseToo);
    tabletWidget.showMaximized();
    return app.exec();
}
