// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandclientbufferintegrationfactory_p.h"
#include "qwaylandclientbufferintegrationplugin_p.h"
#include "qwaylandclientbufferintegration_p.h"
#include <QtCore/private/qfactoryloader_p.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, qwcbifLoader,
    (QWaylandClientBufferIntegrationFactoryInterface_iid, QLatin1String("/wayland-graphics-integration-client"), Qt::CaseInsensitive))

QStringList QWaylandClientBufferIntegrationFactory::keys()
{
    return qwcbifLoader->keyMap().values();
}

QWaylandClientBufferIntegration *QWaylandClientBufferIntegrationFactory::create(const QString &name, const QStringList &args)
{
    return qLoadPlugin<QWaylandClientBufferIntegration, QWaylandClientBufferIntegrationPlugin>(qwcbifLoader(), name, args);
}

}

QT_END_NAMESPACE
