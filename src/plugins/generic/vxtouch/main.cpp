// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qgenericplugin.h>
#include <QtInputSupport/private/qvxtouchmanager_p.h>

QT_BEGIN_NAMESPACE

class QVxTouchScreenPlugin : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QGenericPluginFactoryInterface_iid FILE "vxtouch.json")

public:
    QVxTouchScreenPlugin();

    QObject* create(const QString &key, const QString &specification) override;
};

QVxTouchScreenPlugin::QVxTouchScreenPlugin()
{
}

QObject* QVxTouchScreenPlugin::create(const QString &key, const QString &spec)
{
    if (!key.compare(QLatin1String("VxTouch"), Qt::CaseInsensitive))
        return new QVxTouchManager(key, spec);

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
