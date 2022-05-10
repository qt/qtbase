// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "server.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Server server;

    return (server.exec() == QDialog::Accepted) ? 0 : -1;
}
