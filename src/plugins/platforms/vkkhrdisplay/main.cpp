// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatformintegrationplugin.h>
#include "qvkkhrdisplayintegration.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QVkKhrDisplayIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "vkkhrdisplay.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&) override;
};

QPlatformIntegration *QVkKhrDisplayIntegrationPlugin::create(const QString &system, const QStringList &paramList)
{
    if (!system.compare("vkkhrdisplay"_L1, Qt::CaseInsensitive))
        return new QVkKhrDisplayIntegration(paramList);

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
