// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatformintegrationplugin.h>
#include "qdirectfbintegration.h"
#include "qdirectfb_egl.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
    QPlatformIntegration *create(const QString&, const QStringList&) override;
};

QPlatformIntegration * QDirectFbIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList);
    QDirectFbIntegration *integration = nullptr;

    if (!system.compare("directfb"_L1, Qt::CaseInsensitive))
        integration = new QDirectFbIntegration;
    QT_EGL_BACKEND_CREATE(system, integration)

    if (!integration)
        return 0;

    integration->connectToDirectFb();

    return integration;
}

QT_END_NAMESPACE

#include "main.moc"
