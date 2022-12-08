// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QStyleFactory>

#include "stylewindow.h"

//! [0]
int main(int argv, char *args[])
{
    QApplication app(argv, args);

    QStyle *style = QStyleFactory::create("simplestyle");
    if (!style)
        qFatal("Cannot load the 'simplestyle' plugin.");

    QApplication::setStyle(style);

    StyleWindow window;
    window.resize(350, 50);
    window.show();

    return app.exec();
}
//! [0]
