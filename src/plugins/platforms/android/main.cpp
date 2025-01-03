// Copyright (C) 2019 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include <qpa/qplatformintegrationplugin.h>
#include "qandroidplatformintegration.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QAndroidIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "android.json")

public:
    QPlatformIntegration *create(const QString& system, const QStringList& paramList) override;
};

QPlatformIntegration *QAndroidIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    if (!system.compare("android"_L1, Qt::CaseInsensitive))
        return new QAndroidPlatformIntegration(paramList);

    return nullptr;
}

QT_END_NAMESPACE
#include "main.moc"
