// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAPZOOM_H
#define MAPZOOM_H

#include <QMainWindow>

class LightMaps;

class MapZoom : public QMainWindow
{
    Q_OBJECT

public:
    MapZoom();

private slots:
    void chooseOslo();
    void chooseBerlin();
    void chooseJakarta();
    void aboutOsm();

private:
    LightMaps *map;
};

#endif
