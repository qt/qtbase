/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#include <AppKit/AppKit.h>

#include "qcocoaprintersupport_p.h"

#ifndef QT_NO_PRINTER

#include <AppKit/AppKit.h>

#include <QtCore/private/qcore_mac_p.h>

#include "qcocoaprintdevice_p.h"
#include "qprintengine_mac_p.h"

#include <private/qprinterinfo_p.h>

#define QT_STATICPLUGIN
#include <qpa/qplatformprintplugin.h>

QT_BEGIN_NAMESPACE

QCocoaPrinterSupport::QCocoaPrinterSupport()
{ }

QCocoaPrinterSupport::~QCocoaPrinterSupport()
{ }

QPrintEngine *QCocoaPrinterSupport::createNativePrintEngine(QPrinter::PrinterMode printerMode, const QString &deviceId)
{
    return new QMacPrintEngine(printerMode, deviceId);
}

QPaintEngine *QCocoaPrinterSupport::createPaintEngine(QPrintEngine *printEngine, QPrinter::PrinterMode printerMode)
{
    Q_UNUSED(printerMode);
    /*
        QMacPrintEngine multiply inherits from QPrintEngine and QPaintEngine,
        the cast here allows conversion of QMacPrintEngine* to QPaintEngine*
    */
    return static_cast<QMacPrintEngine *>(printEngine);
}

QPrintDevice QCocoaPrinterSupport::createPrintDevice(const QString &id)
{
    return QPlatformPrinterSupport::createPrintDevice(new QCocoaPrintDevice(id));
}

QStringList QCocoaPrinterSupport::availablePrintDeviceIds() const
{
    QStringList list;
    QCFType<CFArrayRef> printerList;
    if (PMServerCreatePrinterList(kPMServerLocal, &printerList) == noErr) {
        CFIndex count = CFArrayGetCount(printerList);
        for (CFIndex i = 0; i < count; ++i) {
            PMPrinter printer = static_cast<PMPrinter>(const_cast<void *>(CFArrayGetValueAtIndex(printerList, i)));
            list.append(QString::fromCFString(PMPrinterGetID(printer)));
        }
    }
    return list;
}

QString QCocoaPrinterSupport::defaultPrintDeviceId() const
{
    QCFType<CFArrayRef> printerList;
    if (PMServerCreatePrinterList(kPMServerLocal, &printerList) == noErr) {
        CFIndex count = CFArrayGetCount(printerList);
        for (CFIndex i = 0; i < count; ++i) {
            PMPrinter printer = static_cast<PMPrinter>(const_cast<void *>(CFArrayGetValueAtIndex(printerList, i)));
            if (PMPrinterIsDefault(printer))
                return QString::fromCFString(PMPrinterGetID(printer));
        }
    }
    return QString();
}

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

    return new QCocoaPrinterSupport();
}

#include "qcocoaprintersupport.moc"

QT_END_NAMESPACE

#endif  //QT_NO_PRINTER
