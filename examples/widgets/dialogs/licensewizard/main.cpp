// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>

#include "licensewizard.h"

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#if QT_CONFIG(translation)
    QTranslator translator;
    for (const QString &trPath : QLibraryInfo::paths(QLibraryInfo::TranslationsPath)) {
        if (translator.load(QLocale(), u"qtbase"_s, u"_"_s, trPath)) {
            app.installTranslator(&translator);
            break;
        }
    }
#endif

    LicenseWizard wizard;
    wizard.show();
    return app.exec();
}
