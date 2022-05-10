// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include "viewer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Viewer viewer;
    viewer.show();
    return app.exec();
}
