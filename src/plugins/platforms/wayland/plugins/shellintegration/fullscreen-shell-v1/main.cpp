// Copyright (C) 2018 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtWaylandClient/private/qwaylandshellintegrationplugin_p.h>

#include "qwaylandfullscreenshellv1integration.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandFullScreenShellV1IntegrationPlugin : public QWaylandShellIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QWaylandShellIntegrationFactoryInterface_iid FILE "fullscreen-shell-v1.json")
public:
    QWaylandShellIntegration *create(const QString &key, const QStringList &paramList) override;
};

QWaylandShellIntegration *QWaylandFullScreenShellV1IntegrationPlugin::create(const QString &key, const QStringList &paramList)
{
    Q_UNUSED(paramList);

    if (key == QLatin1String("fullscreen-shell-v1"))
        return new QWaylandFullScreenShellV1Integration();
    return nullptr;
}

} // namespace QtWaylandClient

QT_END_NAMESPACE

#include "main.moc"
