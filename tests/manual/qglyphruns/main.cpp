// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "controller.h"

#include <QtWidgets>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    Controller controller;
    controller.showMaximized();

    return app.exec();
}

#include "main.moc"
