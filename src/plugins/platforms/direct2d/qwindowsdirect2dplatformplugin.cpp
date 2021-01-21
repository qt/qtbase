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

#include "qwindowsdirect2dintegration.h"

#include <QtGui/qpa/qplatformintegrationplugin.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QWindowsDirect2DIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "direct2d.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&);
};

QPlatformIntegration *QWindowsDirect2DIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    if (system.compare(system, QLatin1String("direct2d"), Qt::CaseInsensitive) == 0)
        return QWindowsDirect2DIntegration::create(paramList);
    return nullptr;
}

QT_END_NAMESPACE

#include "qwindowsdirect2dplatformplugin.moc"
