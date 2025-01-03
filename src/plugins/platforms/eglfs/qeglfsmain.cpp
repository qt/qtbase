// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatformintegrationplugin.h>
#include <private/qeglfsintegration_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QEglFSIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "eglfs.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&) override;
};

QPlatformIntegration* QEglFSIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList);
    if (!system.compare("eglfs"_L1, Qt::CaseInsensitive))
        return new QEglFSIntegration;

    return nullptr;
}

QT_END_NAMESPACE

#include "qeglfsmain.moc"
