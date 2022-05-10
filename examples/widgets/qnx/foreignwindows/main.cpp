// Copyright (C) 2018 QNX Software Systems. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
