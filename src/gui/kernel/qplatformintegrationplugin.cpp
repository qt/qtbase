// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformintegrationplugin.h"

QT_BEGIN_NAMESPACE

QPlatformIntegrationPlugin::QPlatformIntegrationPlugin(QObject *parent)
    : QObject(parent)
{
}

QPlatformIntegrationPlugin::~QPlatformIntegrationPlugin()
{
}

QPlatformIntegration *QPlatformIntegrationPlugin::create(const QString &key, const QStringList &paramList)
{
    Q_UNUSED(key);
    Q_UNUSED(paramList);
    return nullptr;
}

QPlatformIntegration *QPlatformIntegrationPlugin::create(const QString &key, const QStringList &paramList, int &argc, char **argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    return create(key, paramList); // Fallback for platform plugins that do not implement the argc/argv version.
}

QT_END_NAMESPACE

#include "moc_qplatformintegrationplugin.cpp"
