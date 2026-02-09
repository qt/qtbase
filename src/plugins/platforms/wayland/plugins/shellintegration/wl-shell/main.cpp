// Copyright (C) 2018 The Qt Company Ltd.
// Copyright (C) 2017 ITAGE Corporation, author: <yusuke.binsaki@itage.co.jp>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandwlshellintegration_p.h"

#include <QtWaylandClient/private/qwaylandshellintegrationplugin_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandWlShellIntegrationPlugin : public QWaylandShellIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QWaylandShellIntegrationFactoryInterface_iid FILE "wl-shell.json")

public:
    QWaylandShellIntegration *create(const QString &key, const QStringList &paramList) override;
};

QWaylandShellIntegration *QWaylandWlShellIntegrationPlugin::create(const QString &key, const QStringList &paramList)
{
    Q_UNUSED(key);
    Q_UNUSED(paramList);
    return new QWaylandWlShellIntegration();
}

}

QT_END_NAMESPACE

#include "main.moc"
