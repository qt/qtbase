/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformprintplugin_qpa.h"
#include "private/qfactoryloader_p.h"

QT_BEGIN_NAMESPACE

#if !defined(QT_NO_LIBRARY) && !defined(QT_NO_SETTINGS)
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QPlatformPrinterSupportFactoryInterface_iid, QLatin1String("/printsupport"), Qt::CaseInsensitive))
#endif

QPlatformPrinterSupportPlugin::QPlatformPrinterSupportPlugin(QObject *parent)
    : QObject(parent)
{
}

QPlatformPrinterSupportPlugin::~QPlatformPrinterSupportPlugin()
{
}

/*!
    \internal

    Returns a lazily-initialized singleton. Ownership is granted to the
    QPlatformPrinterSupportPlugin, which is never unloaded or destroyed until
    application exit, i.e. you can expect this pointer to always be valid and
    multiple calls to this function will always return the same pointer.
*/
QPlatformPrinterSupport *QPlatformPrinterSupportPlugin::get()
{
    static QPlatformPrinterSupport *singleton = 0;
    if (!singleton) {
        QStringList k = loader()->keys();
        if (k.isEmpty())
            return 0;
        QPlatformPrinterSupportPlugin *plugin = qobject_cast<QPlatformPrinterSupportPlugin *>(loader()->instance(k.first()));
        if (!plugin)
            return 0;
        singleton = plugin->create(k.first());
    }
    return singleton;
}

QT_END_NAMESPACE
