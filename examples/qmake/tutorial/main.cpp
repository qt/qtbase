// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include "hello.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    MyPushButton helloButton("Hello world!");
    helloButton.resize(100, 30);

    helloButton.show();
    return app.exec();
}
