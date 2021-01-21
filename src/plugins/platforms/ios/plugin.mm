/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include <qpa/qplatformintegrationplugin.h>
#include <qpa/qplatformthemeplugin.h>
#include "qiosintegration.h"

QT_BEGIN_NAMESPACE

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
    if (!system.compare(QLatin1String("ios"), Qt::CaseInsensitive)
        || !system.compare(QLatin1String("tvos"), Qt::CaseInsensitive)) {
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
