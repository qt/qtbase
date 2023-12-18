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

QStringList QPlatformInputContextFactory::requested()
{
    QStringList imList;
    QByteArray env = qgetenv("QT_IM_MODULES");

    if (!env.isEmpty())
        imList = QString::fromLocal8Bit(env).split(QChar::fromLatin1(';'), Qt::SkipEmptyParts);

    if (!imList.isEmpty())
        return imList;

    env = qgetenv("QT_IM_MODULE");
    if (!env.isEmpty())
        imList = {QString::fromLocal8Bit(env)};

    return imList;
}

QPlatformInputContext *QPlatformInputContextFactory::create(const QStringList& keys)
{
    for (const QString &key : keys) {
        auto plugin = create(key);
        if (plugin)
            return plugin;
    }

    return nullptr;
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

