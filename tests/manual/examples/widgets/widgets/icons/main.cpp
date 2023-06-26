// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QCommandLineParser>
#include <QScreen>

#include "mainwindow.h"

//! [45]
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(MainWindow::tr("Icons"));
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser commandLineParser;
    commandLineParser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    commandLineParser.addHelpOption();
    commandLineParser.addVersionOption();
        commandLineParser.addPositionalArgument(MainWindow::tr("[file]"), MainWindow::tr("Icon file(s) to open."));
    commandLineParser.process(QCoreApplication::arguments());

    MainWindow mainWin;
    if (!commandLineParser.positionalArguments().isEmpty())
        mainWin.loadImages(commandLineParser.positionalArguments());

    const QRect availableGeometry = mainWin.screen()->availableGeometry();
    mainWin.resize(availableGeometry.width() / 2, availableGeometry.height() * 2 / 3);
    mainWin.move((availableGeometry.width() - mainWin.width()) / 2, (availableGeometry.height() - mainWin.height()) / 2);

    mainWin.show();
    return app.exec();
}
//! [45]
