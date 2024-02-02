// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QCoreApplication>
#include <QByteArray>
#include <QDir>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QByteArray currentPath = QDir::currentPath().toLocal8Bit();
    fprintf(stdout, "%s", currentPath.constData());
    app.exit();
}
