// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandclientbufferintegrationplugin_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandClientBufferIntegrationPlugin::QWaylandClientBufferIntegrationPlugin(QObject *parent) :
    QObject(parent)
{
}

QWaylandClientBufferIntegrationPlugin::~QWaylandClientBufferIntegrationPlugin()
{
}

}

QT_END_NAMESPACE

#include "moc_qwaylandclientbufferintegrationplugin_p.cpp"
