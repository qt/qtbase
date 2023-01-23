// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qplatforminputcontextplugin_p.h>
#include <qpa/qplatforminputcontext.h>
#include "private/qfactoryloader_p.h"

#include "qguiapplication.h"
#include "qdebug.h"
#include <stdlib.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if QT_CONFIG(settings)
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, icLoader,
    (QPlatformInputContextFactoryInterface_iid, "/platforminputcontexts"_L1, Qt::CaseInsensitive))
#endif

QStringList QPlatformInputContextFactory::keys()
{
#if QT_CONFIG(settings)
    return icLoader()->keyMap().values();
#else
    return QStringList();
#endif
}

QString QPlatformInputContextFactory::requested()
{
    QByteArray env = qgetenv("QT_IM_MODULE");
    return env.isNull() ? QString() : QString::fromLocal8Bit(env);
}

QPlatformInputContext *QPlatformInputContextFactory::create(const QString& key)
{
#if QT_CONFIG(settings)
    if (!key.isEmpty()) {
        QStringList paramList = key.split(u':');
        const QString platform = paramList.takeFirst().toLower();

        QPlatformInputContext *ic = qLoadPlugin<QPlatformInputContext, QPlatformInputContextPlugin>
                                                 (icLoader(), platform, paramList);
        if (ic && ic->isValid())
            return ic;

        delete ic;
    }
#else
    Q_UNUSED(key);
#endif
    return nullptr;
}

QPlatformInputContext *QPlatformInputContextFactory::create()
{
    return create(requested());
}

QT_END_NAMESPACE

