// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatforminputcontextplugin_p.h>

#include <QtCore/QStringList>

#include "qcomposeplatforminputcontext.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QComposePlatformInputContextPlugin : public QPlatformInputContextPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformInputContextFactoryInterface_iid FILE "compose.json")

public:
    QComposeInputContext *create(const QString &, const QStringList &) override;
};

QComposeInputContext *QComposePlatformInputContextPlugin::create(const QString &system, const QStringList &paramList)
{
    Q_UNUSED(paramList);

    if (system.compare(system, "compose"_L1, Qt::CaseInsensitive) == 0
            || system.compare(system, "xim"_L1, Qt::CaseInsensitive) == 0)
        return new QComposeInputContext;
    return nullptr;
}

QT_END_NAMESPACE

#include "qcomposeplatforminputcontextmain.moc"
