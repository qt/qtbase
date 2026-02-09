// Copyright (C) 2016 Robin Burchell <robin.burchell@viroteck.net>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylanddecorationplugin_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandDecorationPlugin::QWaylandDecorationPlugin(QObject *parent)
    : QObject(parent)
{
}
QWaylandDecorationPlugin::~QWaylandDecorationPlugin()
{
}

}

QT_END_NAMESPACE

#include "moc_qwaylanddecorationplugin_p.cpp"
