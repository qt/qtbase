// Copyright (C) 2016 Jolla Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandshellintegrationfactory_p.h"
#include "qwaylandshellintegrationplugin_p.h"
#include "qwaylandshellintegration_p.h"
#include <QtCore/private/qfactoryloader_p.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, qwsifLoader,
    (QWaylandShellIntegrationFactoryInterface_iid, QLatin1String("/wayland-shell-integration"), Qt::CaseInsensitive))

QStringList QWaylandShellIntegrationFactory::keys()
{
    return qwsifLoader->keyMap().values();
}

QWaylandShellIntegration *QWaylandShellIntegrationFactory::create(const QString &name, QWaylandDisplay *display, const QStringList &args)
{
    std::unique_ptr<QWaylandShellIntegration> integration;
    integration.reset(qLoadPlugin<QWaylandShellIntegration, QWaylandShellIntegrationPlugin>(qwsifLoader(), name, args));

    if (integration && !integration->initialize(display))
        return nullptr;

    return integration.release();
}

}

QT_END_NAMESPACE
