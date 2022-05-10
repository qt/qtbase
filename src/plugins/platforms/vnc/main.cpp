// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatformintegrationplugin.h>
#include "qvncintegration.h"
#include "qvnc_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QVncIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "vnc.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&) override;
};

QPlatformIntegration* QVncIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    if (!system.compare("vnc"_L1, Qt::CaseInsensitive))
        return new QVncIntegration(paramList);

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"

