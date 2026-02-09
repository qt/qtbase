// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandserverbufferintegrationfactory_p.h"
#include "qwaylandserverbufferintegrationplugin_p.h"

#include <QtCore/private/qfactoryloader_p.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, qwsbifLoader,
    (QWaylandServerBufferIntegrationFactoryInterface_iid, QLatin1String("/wayland-graphics-integration-client"), Qt::CaseInsensitive))

QStringList QWaylandServerBufferIntegrationFactory::keys()
{
    return qwsbifLoader->keyMap().values();
}

QWaylandServerBufferIntegration *QWaylandServerBufferIntegrationFactory::create(const QString &name, const QStringList &args)
{
    return qLoadPlugin<QWaylandServerBufferIntegration, QWaylandServerBufferIntegrationPlugin>(qwsbifLoader(), name, args);
}

}

QT_END_NAMESPACE
