// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "database.h"
#include "mainwindow.h"

#include <QApplication>
#include <QFile>

#include <stdlib.h>

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(masterdetail);

    QApplication app(argc, argv);

    if (!createConnection())
        return EXIT_FAILURE;

    QFile albumDetails("albumdetails.xml");
    MainWindow window("artists", "albums", &albumDetails);
    window.show();
    return app.exec();
}
