// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include <qpa/qplatformintegrationplugin.h>

QT_BEGIN_NAMESPACE

class QHaikuIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "haiku.json")

public:
    QPlatformIntegration *create(const QString&, const QStringList&) override;
};

QT_END_NAMESPACE
