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

#include "qplatformprintplugin.h"
#include "qplatformprintersupport.h"
#include "qprinterinfo.h"
#include "private/qfactoryloader_p.h"
#include <qcoreapplication.h>
#include <qdebug.h>

#ifndef QT_NO_PRINTER

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

static QPlatformPrinterSupport *printerSupport = nullptr;

static void cleanupPrinterSupport()
{
    delete printerSupport;
    printerSupport = nullptr;
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
        QMultiMap<int, QString>::const_iterator it = keyMap.cbegin();
        if (!qEnvironmentVariableIsEmpty("QT_PRINTER_MODULE")) {
            QString module = QString::fromLocal8Bit(qgetenv("QT_PRINTER_MODULE"));
            QMultiMap<int, QString>::const_iterator it2 = std::find_if(keyMap.cbegin(), keyMap.cend(), [module](const QString &value){ return value == module; });
            if (it2 == keyMap.cend())
                qWarning() << "Unable to load printer plugin" << module;
            else
                it = it2;
        }
        if (it != keyMap.cend())
            printerSupport = qLoadPlugin<QPlatformPrinterSupport, QPlatformPrinterSupportPlugin>(loader(), it.value());
        if (printerSupport)
            qAddPostRoutine(cleanupPrinterSupport);
    }
    return printerSupport;
}

QT_END_NAMESPACE

#endif
