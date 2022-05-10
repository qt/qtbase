// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QLocale>
#include "languagechooser.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(i18n);

    QApplication app(argc, argv);
    LanguageChooser chooser(QLocale::system().name());
    chooser.show();
    return app.exec();
}
