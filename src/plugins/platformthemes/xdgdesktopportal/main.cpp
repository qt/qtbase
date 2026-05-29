// Copyright (C) 2017-2018 Red Hat, Inc
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatformthemeplugin.h>
#include "qxdgdesktopportaltheme.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QXdgDesktopPortalThemePlugin : public QPlatformThemePlugin
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID QPlatformThemeFactoryInterface_iid FILE "xdgdesktopportal.json")

public:
    QPlatformTheme *create(const QString &key, const QStringList &params) override;
};

QPlatformTheme *QXdgDesktopPortalThemePlugin::create(const QString &key, const QStringList &params)
{
    Q_UNUSED(params);
    return QXdgDesktopPortalTheme::isXdgPlugin(key) ? new QXdgDesktopPortalTheme : nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
