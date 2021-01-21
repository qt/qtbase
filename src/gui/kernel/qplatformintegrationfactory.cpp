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

#include <qpa/qplatformintegrationfactory_p.h>
#include <qpa/qplatformintegrationplugin.h>
#include "private/qfactoryloader_p.h"
#include "qmutex.h"
#include "qdir.h"

#include "qguiapplication.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QPlatformIntegrationFactoryInterface_iid, QLatin1String("/platforms"), Qt::CaseInsensitive))

#if QT_CONFIG(library)
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, directLoader,
                          (QPlatformIntegrationFactoryInterface_iid, QLatin1String(""), Qt::CaseInsensitive))
#endif // QT_CONFIG(library)

QPlatformIntegration *QPlatformIntegrationFactory::create(const QString &platform, const QStringList &paramList, int &argc, char **argv, const QString &platformPluginPath)
{
#if QT_CONFIG(library)
    // Try loading the plugin from platformPluginPath first:
    if (!platformPluginPath.isEmpty()) {
        QCoreApplication::addLibraryPath(platformPluginPath);
        if (QPlatformIntegration *ret = qLoadPlugin<QPlatformIntegration, QPlatformIntegrationPlugin>(directLoader(), platform, paramList, argc, argv))
            return ret;
    }
#else
    Q_UNUSED(platformPluginPath);
#endif
    return qLoadPlugin<QPlatformIntegration, QPlatformIntegrationPlugin>(loader(), platform, paramList, argc, argv);
}

/*!
    Returns the list of valid keys, i.e. the keys this factory can
    create styles for.

    \sa create()
*/

QStringList QPlatformIntegrationFactory::keys(const QString &platformPluginPath)
{
    QStringList list;
#if QT_CONFIG(library)
    if (!platformPluginPath.isEmpty()) {
        QCoreApplication::addLibraryPath(platformPluginPath);
        list = directLoader()->keyMap().values();
        if (!list.isEmpty()) {
            const QString postFix = QLatin1String(" (from ")
                                    + QDir::toNativeSeparators(platformPluginPath)
                                    + QLatin1Char(')');
            const QStringList::iterator end = list.end();
            for (QStringList::iterator it = list.begin(); it != end; ++it)
                (*it).append(postFix);
        }
    }
#else
    Q_UNUSED(platformPluginPath);
#endif
    list.append(loader()->keyMap().values());
    return list;
}

QT_END_NAMESPACE

