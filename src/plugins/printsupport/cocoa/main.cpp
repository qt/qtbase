/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtPrintSupport module of the Qt Toolkit.
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

#include <QtCore/QMetaMethod>
#include <QtGui/QGuiApplication>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformprintplugin.h>

QT_BEGIN_NAMESPACE

class QCocoaPrinterSupportPlugin : public QPlatformPrinterSupportPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformPrinterSupportFactoryInterface_iid FILE "cocoa.json")

public:
    QPlatformPrinterSupport *create(const QString &);
};

QPlatformPrinterSupport *QCocoaPrinterSupportPlugin::create(const QString &key)
{
    if (key.compare(key, QLatin1String("cocoaprintersupport"), Qt::CaseInsensitive) != 0)
        return 0;
    QGuiApplication *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
    if (!app)
        return 0;
    QPlatformNativeInterface *platformNativeInterface = app->platformNativeInterface();
    int at = platformNativeInterface->metaObject()->indexOfMethod("createPlatformPrinterSupport()");
    if (at == -1)
        return 0;
    QMetaMethod createPlatformPrinterSupport = platformNativeInterface->metaObject()->method(at);
    QPlatformPrinterSupport *platformPrinterSupport = 0;
    if (!createPlatformPrinterSupport.invoke(platformNativeInterface, Q_RETURN_ARG(QPlatformPrinterSupport *, platformPrinterSupport)))
        return 0;
    return platformPrinterSupport;
}

QT_END_NAMESPACE

#include "main.moc"
