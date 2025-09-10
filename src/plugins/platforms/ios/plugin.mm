// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatformintegrationplugin.h>
#include <qpa/qplatformthemeplugin.h>
#include "qiosintegration.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QIOSIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
        Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "ios.json")
    public:
        QPlatformIntegration *create(const QString&, const QStringList&);
};

QPlatformIntegration * QIOSIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList);
    if (!system.compare("ios"_L1, Qt::CaseInsensitive)
        || !system.compare("tvos"_L1, Qt::CaseInsensitive)) {
        return new QIOSIntegration;
    }

    return 0;
}

QT_END_NAMESPACE

#include "plugin.moc"

// Dummy function that we explicitly tell the linker to look for,
// so that the plugin's static initializer is included and run.
extern "C" void qt_registerPlatformPlugin() {}

Q_IMPORT_PLUGIN(QIOSIntegrationPlugin)
