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


        QFactoryLoader *l = loader();

        // Load plugin metadata
        QMultiMap<QString,QCborMap> plugins;
        QList<QPluginParsedMetaData> meta = l->metaData();
        for (int i = 0; i < meta.size(); ++i) {
            QCborMap obj = meta.at(i).value(QtPluginMetaDataKeys::MetaData).toMap();
            obj.insert(QLatin1String("index"), i);
            QCborValue keys = obj.value(QStringLiteral("Keys"));
            if (keys.isArray() && !keys.toArray().empty())
                plugins.insert(keys.toArray()[0].toString(), obj);
            else if (keys.isString())
                plugins.insert(keys.toString(), obj);
        }

        qInfo() << "Available print plugins";
        for (auto key : plugins.keys()) {
            qInfo() << key;
        }

        // Search for user specified print plugin
        const QMultiMap<int, QString> keyMap = l->keyMap();
        QMultiMap<int, QString>::const_iterator it = keyMap.cbegin();
        bool pluginFound = false;
        if (!qEnvironmentVariableIsEmpty("QT_PRINTER_MODULE")) {
            QString module = qEnvironmentVariable("QT_PRINTER_MODULE");
            QMultiMap<int, QString>::const_iterator it2 = std::find_if(keyMap.cbegin(), keyMap.cend(), [module](const QString &value){ return value == module; });
            if (it2 == keyMap.cend()) {
                qWarning() << "Unable to load printer plugin" << module;
            } else {
                pluginFound = true;
                it = it2;
            }
        }

        // Search for highest priority plugin if user didn't specify one
        if (!pluginFound) {
            int priority = -1;
            QString key;
            for (const auto &&[keyIter, metadata] : plugins.asKeyValueRange()) {
                const int pluginPriority = metadata.value(QStringLiteral("Priority")).toInteger();
                if (pluginPriority > priority) {
                    priority = pluginPriority;
                    key = keyIter;
                }
            }
            QMultiMap<int, QString>::const_iterator it2 = std::find_if(keyMap.cbegin(), keyMap.cend(), [key](const QString &value){ return value == key; });
            if (it2 == keyMap.cend())
                qWarning() << "Unable to load printer plugin" << key;
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
