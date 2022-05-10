// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatformthemeplugin.h>
#include "qgtk3theme.h"

QT_BEGIN_NAMESPACE

class QGtk3ThemePlugin : public QPlatformThemePlugin
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID QPlatformThemeFactoryInterface_iid FILE "gtk3.json")

public:
    QPlatformTheme *create(const QString &key, const QStringList &params) override;
};

QPlatformTheme *QGtk3ThemePlugin::create(const QString &key, const QStringList &params)
{
    Q_UNUSED(params);
    if (!key.compare(QLatin1StringView(QGtk3Theme::name), Qt::CaseInsensitive))
        return new QGtk3Theme;

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
