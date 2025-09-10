// Copyright (C) 2014 Robin Burchell <robin.burchell@viroteck.net>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qgenericplugin.h>
#include <QCoreApplication>

#include "qtuiohandler_p.h"

QT_BEGIN_NAMESPACE

class QTuioTouchPlugin : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QGenericPluginFactoryInterface_iid FILE "tuiotouch.json")

public:
    QTuioTouchPlugin();

    QObject* create(const QString &key, const QString &specification) override;
};

QTuioTouchPlugin::QTuioTouchPlugin()
{
}

QObject* QTuioTouchPlugin::create(const QString &key,
                                         const QString &spec)
{
    if (!key.compare(QLatin1String("TuioTouch"), Qt::CaseInsensitive))
        return new QTuioHandler(spec);

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
