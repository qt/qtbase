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

#include "qxcbglintegrationfactory.h"
#include "qxcbglintegrationplugin.h"

#include "qxcbglintegrationplugin.h"
#include "private/qfactoryloader_p.h"
#include "qguiapplication.h"
#include "qdir.h"

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QXcbGlIntegrationFactoryInterface_iid, QLatin1String("/xcbglintegrations"), Qt::CaseInsensitive))

#if QT_CONFIG(library)
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, directLoader,
                          (QXcbGlIntegrationFactoryInterface_iid, QLatin1String(""), Qt::CaseInsensitive))
#endif // QT_CONFIG(library)

QXcbGlIntegration *QXcbGlIntegrationFactory::create(const QString &platform, const QString &pluginPath)
{
#if QT_CONFIG(library)
    // Try loading the plugin from pluginPath first:
    if (!pluginPath.isEmpty()) {
        QCoreApplication::addLibraryPath(pluginPath);
        if (QXcbGlIntegration *ret = qLoadPlugin<QXcbGlIntegration, QXcbGlIntegrationPlugin>(directLoader(), platform))
            return ret;
    }
#else
    Q_UNUSED(pluginPath);
#endif
    return qLoadPlugin<QXcbGlIntegration, QXcbGlIntegrationPlugin>(loader(), platform);
}

QT_END_NAMESPACE
