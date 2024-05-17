// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("BigMenuCreator");
    parser.addHelpOption();
    parser.addOptions({
        { "new-menubar", QLatin1String("Use new menubar instead of QMainWindow's own.") },
        { "no-parent", QLatin1String("When using a new menubar, do *not* set its parent on construction.") }
    });

    parser.process(a);

    MainWindow::newMenubar = parser.isSet("new-menubar");
    MainWindow::parentlessMenubar = parser.isSet("no-parent");

    MainWindow w;
    w.show();

    return a.exec();
}
