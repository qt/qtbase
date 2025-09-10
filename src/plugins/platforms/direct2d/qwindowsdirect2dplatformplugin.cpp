// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsdirect2dintegration.h"

#include <QtGui/qpa/qplatformintegrationplugin.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QWindowsDirect2DIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "direct2d.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&) override;
};

QPlatformIntegration *QWindowsDirect2DIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    if (system.compare(system, "direct2d"_L1, Qt::CaseInsensitive) == 0)
        return QWindowsDirect2DIntegration::create(paramList);
    return nullptr;
}

QT_END_NAMESPACE

#include "qwindowsdirect2dplatformplugin.moc"
