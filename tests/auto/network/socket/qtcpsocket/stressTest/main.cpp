// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "Test.h"

#include <QCoreApplication>
#include <QStringList>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QString arg;
    if (app.arguments().size() > 1)
        arg = app.arguments().at(1).toLower().trimmed();

    Test::Type type;
    if (arg == QLatin1String("qt4client"))
        type = Test::Qt4Client;
    else if (arg == QLatin1String("qt4server"))
        type = Test::Qt4Server;
    else {
        qDebug("usage: ./stressTest <qt4client|qt4server>");
        return 0;
    }

    Test test(type);

    return app.exec();
}
