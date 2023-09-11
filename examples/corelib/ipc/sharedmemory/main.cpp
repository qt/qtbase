// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "dialog.h"

#include <QApplication>

//! [0]
int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    Dialog dialog;
    dialog.show();
    return application.exec();
}
//! [0]

