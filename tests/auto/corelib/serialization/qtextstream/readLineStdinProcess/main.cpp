// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <QtCore/QString>


int main(int argc, char **argv)
{
    QCoreApplication a(argc, argv);
    QTextStream qin(stdin);
    QTextStream qerr(stderr);
    QString line;
    do {
        line = qin.readLine();
        if (!line.isNull())
            qerr << line << Qt::flush;
    } while (!line.isNull());
    return 0;
}
