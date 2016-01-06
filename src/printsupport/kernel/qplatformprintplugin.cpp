/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformprintplugin.h"
#include "qplatformprintersupport.h"
#include "qprinterinfo.h"
#include "private/qfactoryloader_p.h"
#include <qcoreapplication.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QPlatformPrinterSupportFactoryInterface_iid, QLatin1String("/printsupport"), Qt::CaseInsensitive))

QPlatformPrinterSupportPlugin::QPlatformPrinterSupportPlugin(QObject *parent)
    : QObject(parent)
{
}

QPlatformPrinterSupportPlugin::~QPlatformPrinterSupportPlugin()
{
}

static QPlatformPrinterSupport *printerSupport = 0;

static void cleanupPrinterSupport()
{
    delete printerSupport;
    printerSupport = 0;
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
    if (!printerSupport) {
        const QMultiMap<int, QString> keyMap = loader()->keyMap();
        if (!keyMap.isEmpty())
            printerSupport = qLoadPlugin<QPlatformPrinterSupport, QPlatformPrinterSupportPlugin>(loader(), keyMap.constBegin().value());
        if (printerSupport)
            qAddPostRoutine(cleanupPrinterSupport);
    }
    return printerSupport;
}

QT_END_NAMESPACE
