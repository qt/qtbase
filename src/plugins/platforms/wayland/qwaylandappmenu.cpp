// Copyright (C) 2024 David Redondo <kde@david-redondo.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandappmenu_p.h"

#include "qwaylandwindow_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandAppMenu::QWaylandAppMenu(QObject *parent) : QObject(parent), org_kde_kwin_appmenu() { }

QWaylandAppMenu::~QWaylandAppMenu()
{
    if (object())
        release();
}

QWaylandAppMenuManager::QWaylandAppMenuManager(wl_registry *registry, quint32 id, int version)
    : org_kde_kwin_appmenu_manager(registry, id, version)
{
}

QWaylandAppMenuManager::~QWaylandAppMenuManager()
{
    org_kde_kwin_appmenu_manager_destroy(object());
}

} // namespace QtWaylandClient

QT_END_NAMESPACE
