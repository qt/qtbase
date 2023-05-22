// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "view.h"
#include "../connection.h"

#include <QApplication>

#include <stdlib.h>


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if (!createConnection())
        return EXIT_FAILURE;

    View view("items", "images");
    view.show();
#ifdef QT_KEYPAD_NAVIGATION
    QApplication::setNavigationMode(Qt::NavigationModeCursorAuto);
#endif
    return app.exec();
}
