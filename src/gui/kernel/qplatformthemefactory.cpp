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

#include <qpa/qplatformthemefactory_p.h>
#include <qpa/qplatformthemeplugin.h>
#include <QDir>
#include "private/qfactoryloader_p.h"
#include "qmutex.h"

#include "qguiapplication.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QPlatformThemeFactoryInterface_iid, QLatin1String("/platformthemes"), Qt::CaseInsensitive))

#if QT_CONFIG(library)
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, directLoader,
                          (QPlatformThemeFactoryInterface_iid, QLatin1String(""), Qt::CaseInsensitive))
#endif

QPlatformTheme *QPlatformThemeFactory::create(const QString& key, const QString &platformPluginPath)
{
    QStringList paramList = key.split(QLatin1Char(':'));
    const QString platform = paramList.takeFirst().toLower();
#if QT_CONFIG(library)
    // Try loading the plugin from platformPluginPath first:
    if (!platformPluginPath.isEmpty()) {
        QCoreApplication::addLibraryPath(platformPluginPath);
        if (QPlatformTheme *ret = qLoadPlugin<QPlatformTheme, QPlatformThemePlugin>(directLoader(), platform, paramList))
            return ret;
    }
#else
    Q_UNUSED(platformPluginPath);
#endif
    return qLoadPlugin<QPlatformTheme, QPlatformThemePlugin>(loader(), platform, paramList);
}

/*!
    Returns the list of valid keys, i.e. the keys this factory can
    create styles for.

    \sa create()
*/
QStringList QPlatformThemeFactory::keys(const QString &platformPluginPath)
{
    QStringList list;

#if QT_CONFIG(library)
    if (!platformPluginPath.isEmpty()) {
        QCoreApplication::addLibraryPath(platformPluginPath);
        list += directLoader()->keyMap().values();
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
    list += loader()->keyMap().values();
    return list;
}

QT_END_NAMESPACE
