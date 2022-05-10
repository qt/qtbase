// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "echowindow.h"
#include "echointerface.h"

//! [0]
int main(int argv, char *args[])
{
    QApplication app(argv, args);

    EchoWindow window;
    window.show();

    return app.exec();
}
//! [0]
