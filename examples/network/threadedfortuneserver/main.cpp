// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QtCore>

#include <stdlib.h>

#include "dialog.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Dialog dialog;
    dialog.show();
    return app.exec();
}
