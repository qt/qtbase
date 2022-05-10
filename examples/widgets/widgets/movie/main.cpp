// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "movieplayer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MoviePlayer player;
    player.show();
    player.show();
    return app.exec();
}
