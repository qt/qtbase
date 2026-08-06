// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatformintegrationplugin.h>
#include "qohosplatformintegration.h"
#include <QtCore/qdebug.h>
#include <qohosjsenv_p.h>

QT_BEGIN_NAMESPACE

class QOhosPlatformIntegrationPlugin: public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "ohos.json")
public:
    QPlatformIntegration *create(const QString &key, const QStringList &paramList) override;
};

QPlatformIntegration *QOhosPlatformIntegrationPlugin::create(const QString &key, const QStringList &paramList)
{
    auto __dbg = make_QCScopedDebug("QOhosPlatformIntegrationPlugin::create");
    Q_UNUSED(paramList);
    if (key.compare(QLatin1String("ohos"), Qt::CaseInsensitive) == 0) {
        auto __dbg = make_QCScopedDebug("QOhosPlatformIntegrationPlugin::create creating ");
        return new QOhosPlatformIntegration(paramList);
    }
    return 0;
}

QT_END_NAMESPACE
#include "qohosplatformplugin.moc"
