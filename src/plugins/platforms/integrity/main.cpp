// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatformintegrationplugin.h>
#include "qintegrityfbintegration.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QIntegrityFbIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "integrity.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&) override;
};

QPlatformIntegration* QIntegrityFbIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList);
    if (!system.compare("integrityfb"_L1, Qt::CaseInsensitive))
        return new QIntegrityFbIntegration(paramList);

    return 0;
}

QT_END_NAMESPACE

#include "main.moc"

