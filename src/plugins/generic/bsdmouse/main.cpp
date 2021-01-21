/****************************************************************************
**
** Copyright (C) 2015-2016 Oleksandr Tymoshenko <gonzo@bluezbox.com>
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
