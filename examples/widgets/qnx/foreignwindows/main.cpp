// Copyright (C) 2018 QNX Software Systems. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "collector.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Collector collector;
    collector.resize(640, 480);
    collector.show();

    return app.exec();
}
