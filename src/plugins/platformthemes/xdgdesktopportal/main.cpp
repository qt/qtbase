/****************************************************************************
**
** Copyright (C) 2017-2018 Red Hat, Inc
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

#include <qpa/qplatformthemeplugin.h>
#include "qxdgdesktopportaltheme.h"

QT_BEGIN_NAMESPACE

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
    if (!key.compare(QLatin1String("xdgdesktopportal"), Qt::CaseInsensitive) ||
        !key.compare(QLatin1String("flatpak"), Qt::CaseInsensitive) ||
        !key.compare(QLatin1String("snap"), Qt::CaseInsensitive))
        return new QXdgDesktopPortalTheme;

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
