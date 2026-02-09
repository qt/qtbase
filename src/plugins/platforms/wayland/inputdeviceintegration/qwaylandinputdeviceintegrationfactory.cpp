// Copyright (C) 2016 LG Electronics Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandinputdeviceintegrationfactory_p.h"
#include "qwaylandinputdeviceintegrationplugin_p.h"
#include "qwaylandinputdeviceintegration_p.h"
#include <QtCore/private/qfactoryloader_p.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, qwidfiLoader,
    (QWaylandInputDeviceIntegrationFactoryInterface_iid, QLatin1String("/wayland-inputdevice-integration"), Qt::CaseInsensitive))

QStringList QWaylandInputDeviceIntegrationFactory::keys()
{
    return qwidfiLoader->keyMap().values();
}

QWaylandInputDeviceIntegration *QWaylandInputDeviceIntegrationFactory::create(const QString &name, const QStringList &args)
{
    return qLoadPlugin<QWaylandInputDeviceIntegration, QWaylandInputDeviceIntegrationPlugin>(qwidfiLoader(), name, args);
}

}

QT_END_NAMESPACE
