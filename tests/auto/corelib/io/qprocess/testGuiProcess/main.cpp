// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <stdio.h>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QLabel label("This process is just waiting to die");
    label.show();

    int c;
    Q_UNUSED(c);
    fgetc(stdin); // block until fed

    qDebug("Process is running");

    return 0;
}
