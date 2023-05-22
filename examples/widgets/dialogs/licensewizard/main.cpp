// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>

#include "licensewizard.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#ifndef QT_NO_TRANSLATION
    QString translatorFileName = QLatin1String("qtbase_");
    translatorFileName += QLocale::system().name();
    QTranslator *translator = new QTranslator(&app);
    if (translator->load(translatorFileName, QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        app.installTranslator(translator);
#endif

    LicenseWizard wizard;
    wizard.show();
    return app.exec();
}
