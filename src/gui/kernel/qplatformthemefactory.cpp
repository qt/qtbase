// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatformthemefactory_p.h>
#include <qpa/qplatformthemeplugin.h>
#include <QDir>
#include "private/qfactoryloader_p.h"
#include "qmutex.h"

#include "qguiapplication.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QPlatformThemeFactoryInterface_iid, "/platformthemes"_L1, Qt::CaseInsensitive))

QPlatformTheme *QPlatformThemeFactory::create(const QString& key, const QString &platformPluginPath)
{
    QStringList paramList = key.split(u':');
    const QString platform = paramList.takeFirst().toLower();
    loader->setExtraSearchPath(platformPluginPath);
    return qLoadPlugin<QPlatformTheme, QPlatformThemePlugin>(loader(), platform, paramList);
}

/*!
    Returns the list of valid keys, i.e. the keys this factory can
    create styles for.

    \sa create()
*/
QStringList QPlatformThemeFactory::keys(const QString &platformPluginPath)
{
    loader->setExtraSearchPath(platformPluginPath);
    return loader->keyMap().values();
}

QT_END_NAMESPACE
