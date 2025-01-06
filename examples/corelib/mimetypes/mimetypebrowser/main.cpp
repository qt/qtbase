// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QScreen>
#if QT_CONFIG(translation)
#  include <QLocale>
#  include <QLibraryInfo>
#  include <QTranslator>
#endif

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#if QT_CONFIG(translation)
    QTranslator translator;
    if (translator.load(QLocale::system(), "qtbase"_L1, "_"_L1,
                        QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(&translator);
    }
#endif

    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    parser.process(app);

    MainWindow mainWindow;
    const QRect availableGeometry = mainWindow.screen()->availableGeometry();
    mainWindow.resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
    mainWindow.show();

    return app.exec();
}
