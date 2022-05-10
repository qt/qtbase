// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include "mapzoom.h"

int main(int argc, char **argv)
{

    QApplication app(argc, argv);
    app.setApplicationName("LightMaps");

    MapZoom w;
    w.resize(600, 450);
    w.show();

    return app.exec();
}
