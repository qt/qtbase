/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qplatforminputcontextplugin_p.h>
#include <qpa/qplatforminputcontext.h>
#include "private/qfactoryloader_p.h"

#include "qguiapplication.h"
#include "qdebug.h"
#include <stdlib.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(settings)
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QPlatformInputContextFactoryInterface_iid, QLatin1String("/platforminputcontexts"), Qt::CaseInsensitive))
#endif

QStringList QPlatformInputContextFactory::keys()
{
#if QT_CONFIG(settings)
    return loader()->keyMap().values();
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
        QStringList paramList = key.split(QLatin1Char(':'));
        const QString platform = paramList.takeFirst().toLower();

        QPlatformInputContext *ic = qLoadPlugin<QPlatformInputContext, QPlatformInputContextPlugin>
                                                 (loader(), platform, paramList);
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

