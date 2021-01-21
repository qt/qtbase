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
#include "qdirectfbintegration.h"
#include "qdirectfb_egl.h"

QT_BEGIN_NAMESPACE

#ifdef DIRECTFB_GL_EGL
#define QT_EGL_BACKEND_STRING(list) list << "directfbegl";
#define QT_EGL_BACKEND_CREATE(list, out) \
    if (list.toLower() == "directfbegl") \
        out = new QDirectFbIntegrationEGL;
#else
#define QT_EGL_BACKEND_STRING(list)
#define QT_EGL_BACKEND_CREATE(system, out)
#endif

class QDirectFbIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "directfb.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&);
};

QPlatformIntegration * QDirectFbIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList);
    QDirectFbIntegration *integration = 0;

    if (!system.compare(QLatin1String("directfb"), Qt::CaseInsensitive))
        integration = new QDirectFbIntegration;
    QT_EGL_BACKEND_CREATE(system, integration)

    if (!integration)
        return 0;

    integration->connectToDirectFb();

    return integration;
}

QT_END_NAMESPACE

#include "main.moc"
