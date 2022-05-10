// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QCommandLineParser>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(sdi);
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("SDI Example");
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "The file(s) to open.");
    parser.process(app);

    MainWindow *mainWin = nullptr;
    const QStringList posArgs = parser.positionalArguments();
    for (const QString &file : posArgs) {
        MainWindow *newWin = new MainWindow(file);
        newWin->tile(mainWin);
        newWin->show();
        mainWin = newWin;
    }

    if (!mainWin)
        mainWin = new MainWindow;
    mainWin->show();

    return app.exec();
}
