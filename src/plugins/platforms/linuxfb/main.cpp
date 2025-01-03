// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatformintegrationplugin.h>
#include "qlinuxfbintegration.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QLinuxFbIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "linuxfb.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&) override;
};

QPlatformIntegration* QLinuxFbIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList);
    if (!system.compare("linuxfb"_L1, Qt::CaseInsensitive))
        return new QLinuxFbIntegration(paramList);

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"

