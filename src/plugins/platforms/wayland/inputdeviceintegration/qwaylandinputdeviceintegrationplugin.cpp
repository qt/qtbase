// Copyright (C) 2016 LG Electronics Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandinputdeviceintegrationplugin_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandInputDeviceIntegrationPlugin::QWaylandInputDeviceIntegrationPlugin(QObject *parent)
                              : QObject(parent)
{
}

QWaylandInputDeviceIntegrationPlugin::~QWaylandInputDeviceIntegrationPlugin()
{
}

}

QT_END_NAMESPACE

#include "moc_qwaylandinputdeviceintegrationplugin_p.cpp"
