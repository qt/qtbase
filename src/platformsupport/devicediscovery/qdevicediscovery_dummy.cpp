// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdevicediscovery_dummy_p.h"

QT_BEGIN_NAMESPACE

QDeviceDiscovery *QDeviceDiscovery::create(QDeviceTypes types, QObject *parent)
{
    return new QDeviceDiscoveryDummy(types, parent);
}

QDeviceDiscoveryDummy::QDeviceDiscoveryDummy(QDeviceTypes types, QObject *parent)
    : QDeviceDiscovery(types, parent)
{
}

QStringList QDeviceDiscoveryDummy::scanConnectedDevices()
{
    return QStringList();
}

QT_END_NAMESPACE
