// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "dropsitewindow.h"

//! [main() function]
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    DropSiteWindow window;
    window.show();
    return app.exec();
}
//! [main() function]
