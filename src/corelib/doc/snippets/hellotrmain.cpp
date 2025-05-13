// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
// Required for using the '_L1' string literal.
using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QTranslator translator;
    // look up e.g. :/i18n/myapp_de.qm
    if (translator.load(QLocale(), "myapp"_L1, "_"_L1, ":/i18n"_L1))
        QCoreApplication::installTranslator(&translator);

    QPushButton hello(QCoreApplication::translate("main", "Hello world!"));
    hello.resize(100, 30);

    hello.show();
    return app.exec();
}
//! [0]

