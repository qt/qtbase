/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxcbglintegrationfactory.h"
#include "qxcbglintegrationplugin.h"

#include "qxcbglintegrationplugin.h"
#include "private/qfactoryloader_p.h"
#include "qguiapplication.h"
#include "qdir.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_LIBRARY
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QXcbGlIntegrationFactoryInterface_iid, QLatin1String("/xcbglintegrations"), Qt::CaseInsensitive))
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, directLoader,
                          (QXcbGlIntegrationFactoryInterface_iid, QLatin1String(""), Qt::CaseInsensitive))

static inline QXcbGlIntegration *loadIntegration(QFactoryLoader *loader, const QString &key)
{
    const int index = loader->indexOf(key);
    if (index != -1) {
        if (QXcbGlIntegrationPlugin *factory = qobject_cast<QXcbGlIntegrationPlugin *>(loader->instance(index)))
            if (QXcbGlIntegration *result = factory->create())
                return result;
    }
    return Q_NULLPTR;
}
#endif // !QT_NO_LIBRARY

QStringList QXcbGlIntegrationFactory::keys(const QString &pluginPath)
{
#ifndef QT_NO_LIBRARY
    QStringList list;
    if (!pluginPath.isEmpty()) {
        QCoreApplication::addLibraryPath(pluginPath);
        list = directLoader()->keyMap().values();
        if (!list.isEmpty()) {
            const QString postFix = QStringLiteral(" (from ")
                                    + QDir::toNativeSeparators(pluginPath)
                                    + QLatin1Char(')');
            const QStringList::iterator end = list.end();
            for (QStringList::iterator it = list.begin(); it != end; ++it)
                (*it).append(postFix);
        }
    }
    list.append(loader()->keyMap().values());
    return list;
#else
    Q_UNUSED(pluginPath);
    return QStringList();
#endif
}

QXcbGlIntegration *QXcbGlIntegrationFactory::create(const QString &platform, const QString &pluginPath)
{
#ifndef QT_NO_LIBRARY
    // Try loading the plugin from platformPluginPath first:
    if (!pluginPath.isEmpty()) {
        QCoreApplication::addLibraryPath(pluginPath);
        if (QXcbGlIntegration *ret = loadIntegration(directLoader(), platform))
            return ret;
    }
    if (QXcbGlIntegration *ret = loadIntegration(loader(), platform))
        return ret;
#else
    Q_UNUSED(platform);
    Q_UNUSED(pluginPath);
#endif
    return Q_NULLPTR;
}

QT_END_NAMESPACE
