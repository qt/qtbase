// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>
#include <QtNetwork>
#include "lightmaps.h"
#include "mapzoom.h"

MapZoom::MapZoom()
    : QMainWindow(0)
{
    map = new LightMaps(this);
    setCentralWidget(map);
    map->setFocus();

    QAction *osloAction = new QAction(tr("&Oslo"), this);
    QAction *berlinAction = new QAction(tr("&Berlin"), this);
    QAction *jakartaAction = new QAction(tr("&Jakarta"), this);
    QAction *nightModeAction = new QAction(tr("Night Mode"), this);
    nightModeAction->setCheckable(true);
    nightModeAction->setChecked(false);
    QAction *osmAction = new QAction(tr("About OpenStreetMap"), this);
    connect(osloAction, SIGNAL(triggered()), SLOT(chooseOslo()));
    connect(berlinAction, SIGNAL(triggered()), SLOT(chooseBerlin()));
    connect(jakartaAction, SIGNAL(triggered()), SLOT(chooseJakarta()));
    connect(nightModeAction, SIGNAL(triggered()), map, SLOT(toggleNightMode()));
    connect(osmAction, SIGNAL(triggered()), SLOT(aboutOsm()));

    QMenu *menu = menuBar()->addMenu(tr("&Options"));
    menu->addAction(osloAction);
    menu->addAction(berlinAction);
    menu->addAction(jakartaAction);
    menu->addSeparator();
    menu->addAction(nightModeAction);
    menu->addAction(osmAction);

    setWindowTitle(tr("Light Maps"));
}

void MapZoom::chooseOslo()
{
    map->setCenter(59.9138204, 10.7387413);
}

void MapZoom::chooseBerlin()
{
    map->setCenter(52.52958999943302, 13.383053541183472);
}

void MapZoom::chooseJakarta()
{
    map->setCenter(-6.211544, 106.845172);
}

void MapZoom::aboutOsm()
{
    QDesktopServices::openUrl(QUrl("http://www.openstreetmap.org"));
}
