// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QStyleFactory>

#include "stylewindow.h"

//! [0]
int main(int argv, char *args[])
{
    QApplication app(argv, args);
    QApplication::setStyle(QStyleFactory::create("simplestyle"));

    StyleWindow window;
    window.resize(200, 50);
    window.show();

    return app.exec();
}
//! [0]
