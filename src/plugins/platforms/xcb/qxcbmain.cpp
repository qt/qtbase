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
#include "qxcbintegration.h"

QT_BEGIN_NAMESPACE

class QXcbIntegrationPlugin : public QPlatformIntegrationPlugin
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "xcb.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&, int &, char **) override;
};

QPlatformIntegration* QXcbIntegrationPlugin::create(const QString& system, const QStringList& parameters, int &argc, char **argv)
{
    if (!system.compare(QLatin1String("xcb"), Qt::CaseInsensitive)) {
        auto xcbIntegration = new QXcbIntegration(parameters, argc, argv);
        if (!xcbIntegration->hasDefaultConnection()) {
            delete xcbIntegration;
            return nullptr;
        }
        return xcbIntegration;
    }

    return nullptr;
}

QT_END_NAMESPACE

#include "qxcbmain.moc"
