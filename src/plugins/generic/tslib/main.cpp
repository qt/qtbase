// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include <QtGui/qgenericplugin.h>
#include <QtInputSupport/private/qtslib_p.h>

QT_BEGIN_NAMESPACE

class QTsLibPlugin : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QGenericPluginFactoryInterface_iid FILE "tslib.json")

public:
    QObject* create(const QString &key, const QString &specification) override;
};

QObject* QTsLibPlugin::create(const QString &key,
                              const QString &specification)
{
    if (!key.compare(QLatin1String("Tslib"), Qt::CaseInsensitive)
        || !key.compare(QLatin1String("TslibRaw"), Qt::CaseInsensitive))
        return new QTsLibMouseHandler(key, specification);

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
