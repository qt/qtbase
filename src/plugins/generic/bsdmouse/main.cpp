// Copyright (C) 2015-2016 Oleksandr Tymoshenko <gonzo@bluezbox.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qgenericplugin.h>
#include "qbsdmouse.h"

QT_BEGIN_NAMESPACE

class QBsdMousePlugin : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QGenericPluginFactoryInterface" FILE "bsdmouse.json")

public:
    QObject *create(const QString &key, const QString &specification) override;
};

QObject *QBsdMousePlugin::create(const QString &key, const QString &specification)
{
    if (!key.compare(QLatin1String("BsdMouse"), Qt::CaseInsensitive))
        return new QBsdMouseHandler(key, specification);

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
