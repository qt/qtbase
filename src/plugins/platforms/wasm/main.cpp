// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qpa/qplatformintegrationplugin.h>
#include "qwasmintegration.h"

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

class QWasmIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "wasm.json")
public:
    QPlatformIntegration *create(const QString &, const QStringList &) override;
};

QPlatformIntegration *QWasmIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList);
    if (!system.compare("wasm"_L1, Qt::CaseInsensitive))
        return new QWasmIntegration;

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
