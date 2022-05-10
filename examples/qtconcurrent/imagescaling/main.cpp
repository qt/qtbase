// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtWidgets>
#include <QtConcurrent>

#include "imagescaling.h"

int main(int argc, char *argv[])
{
    QApplication app(argc,argv);

    Images imageView;
    imageView.show();

    return app.exec();
}
