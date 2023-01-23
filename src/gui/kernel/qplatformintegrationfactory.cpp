// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatformintegrationfactory_p.h>
#include <qpa/qplatformintegrationplugin.h>
#include "private/qfactoryloader_p.h"
#include "qmutex.h"
#include "qdir.h"

#include "qguiapplication.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, piLoader,
    (QPlatformIntegrationFactoryInterface_iid, "/platforms"_L1, Qt::CaseInsensitive))

QPlatformIntegration *QPlatformIntegrationFactory::create(const QString &platform, const QStringList &paramList, int &argc, char **argv, const QString &platformPluginPath)
{
    piLoader->setExtraSearchPath(platformPluginPath);
    return qLoadPlugin<QPlatformIntegration, QPlatformIntegrationPlugin>(piLoader(), platform, paramList, argc, argv);
}

/*!
    Returns the list of valid keys, i.e. the keys this factory can
    create styles for.

    \sa create()
*/

QStringList QPlatformIntegrationFactory::keys(const QString &platformPluginPath)
{
    piLoader->setExtraSearchPath(platformPluginPath);
    return piLoader->keyMap().values();
}

QT_END_NAMESPACE

