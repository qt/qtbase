// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include <QtGui/qgenericplugin.h>
#include <QtInputSupport/private/qevdevtouchmanager_p.h>

QT_BEGIN_NAMESPACE

class QEvdevTouchScreenPlugin : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QGenericPluginFactoryInterface_iid FILE "evdevtouch.json")

public:
    QEvdevTouchScreenPlugin();

    QObject* create(const QString &key, const QString &specification) override;
};

QEvdevTouchScreenPlugin::QEvdevTouchScreenPlugin()
{
}

QObject* QEvdevTouchScreenPlugin::create(const QString &key,
                                         const QString &spec)
{
    if (!key.compare(QLatin1String("EvdevTouch"), Qt::CaseInsensitive))
        return new QEvdevTouchManager(key, spec);

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
