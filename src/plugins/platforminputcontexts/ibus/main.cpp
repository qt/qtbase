/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <qpa/qplatforminputcontextplugin_p.h>
#include <QtCore/QStringList>
#include <QDBusMetaType>
#include "qibusplatforminputcontext.h"
#include "qibustypes.h"

QT_BEGIN_NAMESPACE

class QIbusPlatformInputContextPlugin : public QPlatformInputContextPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformInputContextFactoryInterface_iid FILE "ibus.json")

public:
    QIBusPlatformInputContext *create(const QString&, const QStringList&) override;
};

QIBusPlatformInputContext *QIbusPlatformInputContextPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList);

    if (system.compare(system, QLatin1String("ibus"), Qt::CaseInsensitive) == 0) {
        qDBusRegisterMetaType<QIBusAttribute>();
        qDBusRegisterMetaType<QIBusAttributeList>();
        qDBusRegisterMetaType<QIBusText>();
        return new QIBusPlatformInputContext;
    }
    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
