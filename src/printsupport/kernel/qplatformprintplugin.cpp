// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformprintplugin.h"
#include "qplatformprintersupport.h"
#include "qprinterinfo.h"
#include "private/qfactoryloader_p.h"
#include <qcoreapplication.h>
#include <qdebug.h>

#ifndef QT_NO_PRINTER

#if defined(Q_OS_MACOS)
Q_IMPORT_PLUGIN(QCocoaPrinterSupportPlugin)
#elif defined(Q_OS_WIN)
Q_IMPORT_PLUGIN(QWindowsPrinterSupportPlugin)
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QPlatformPrinterSupportFactoryInterface_iid, "/printsupport"_L1, Qt::CaseInsensitive))

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

#include "moc_qplatformprintplugin.cpp"

#endif
