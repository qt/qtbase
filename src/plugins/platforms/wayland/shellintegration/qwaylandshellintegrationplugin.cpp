// Copyright (C) 2016 Jolla Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandshellintegrationplugin_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandShellIntegrationPlugin::QWaylandShellIntegrationPlugin(QObject *parent)
                              : QObject(parent)
{
}

QWaylandShellIntegrationPlugin::~QWaylandShellIntegrationPlugin()
{
}

}

QT_END_NAMESPACE

#include "moc_qwaylandshellintegrationplugin_p.cpp"
