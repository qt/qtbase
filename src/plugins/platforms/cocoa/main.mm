// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>

#include <qpa/qplatformintegrationplugin.h>
#include <qpa/qplatformthemeplugin.h>
#include "qcocoaintegration.h"
#include "qcocoatheme.h"

#include <QtCore/private/qcore_mac_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QCocoaIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "cocoa.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&);
};

QPlatformIntegration * QCocoaIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    QMacAutoReleasePool pool;
    if (system.compare("cocoa"_L1, Qt::CaseInsensitive) == 0)
        return new QCocoaIntegration(paramList);

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
