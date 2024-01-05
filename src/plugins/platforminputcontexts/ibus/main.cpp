// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatforminputcontextplugin_p.h>
#include <QtCore/QStringList>
#include <QDBusMetaType>
#include "qibusplatforminputcontext.h"
#include "qibustypes.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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

    if (system.compare(system, "ibus"_L1, Qt::CaseInsensitive) == 0) {
        qDBusRegisterMetaType<QIBusAttribute>();
        qDBusRegisterMetaType<QIBusAttributeList>();
        qDBusRegisterMetaType<QIBusText>();
        qDBusRegisterMetaType<QIBusPropTypeClientCommitPreedit>();
        qDBusRegisterMetaType<QIBusPropTypeContentType>();
        return new QIBusPlatformInputContext;
    }

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
