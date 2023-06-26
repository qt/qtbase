// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "validatorwidget.h"

#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    ValidatorWidget w;
    w.show();

    return app.exec();
}
