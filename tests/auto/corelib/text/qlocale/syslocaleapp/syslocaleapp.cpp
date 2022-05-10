// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QLocale>
#include <QCoreApplication>
#include <QTextStream>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QLocale l;
    QTextStream str(stdout);
    str << l.name();

    return 0;
}
