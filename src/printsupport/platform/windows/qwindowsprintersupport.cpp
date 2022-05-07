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

#include "qwindowsprintersupport_p.h"

#ifndef QT_NO_PRINTER

#include "qwindowsprintdevice_p.h"

#include <QtCore/QStringList>
#include <private/qprintengine_win_p.h>
#include <private/qprintdevice_p.h>

#define QT_STATICPLUGIN
#include <qpa/qplatformprintplugin.h>

QT_BEGIN_NAMESPACE

QWindowsPrinterSupport::QWindowsPrinterSupport()
    : QPlatformPrinterSupport()
{
}

QWindowsPrinterSupport::~QWindowsPrinterSupport()
{
}

QPrintEngine *QWindowsPrinterSupport::createNativePrintEngine(QPrinter::PrinterMode printerMode, const QString &deviceId)
{
    return new QWin32PrintEngine(printerMode, deviceId);
}

QPaintEngine *QWindowsPrinterSupport::createPaintEngine(QPrintEngine *engine, QPrinter::PrinterMode printerMode)
{
    Q_UNUSED(printerMode);
    return static_cast<QWin32PrintEngine *>(engine);
}

QPrintDevice QWindowsPrinterSupport::createPrintDevice(const QString &id)
{
    return QPlatformPrinterSupport::createPrintDevice(new QWindowsPrintDevice(id));
}

QStringList QWindowsPrinterSupport::availablePrintDeviceIds() const
{
    return QWindowsPrintDevice::availablePrintDeviceIds();
}

QString QWindowsPrinterSupport::defaultPrintDeviceId() const
{
    return QWindowsPrintDevice::defaultPrintDeviceId();
}

class QWindowsPrinterSupportPlugin : public QPlatformPrinterSupportPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformPrinterSupportFactoryInterface_iid FILE "windows.json")

public:
    QPlatformPrinterSupport *create(const QString &) override;
};

QPlatformPrinterSupport *QWindowsPrinterSupportPlugin::create(const QString &key)
{
    if (key.compare(key, QLatin1String("windowsprintsupport"), Qt::CaseInsensitive) == 0)
        return new QWindowsPrinterSupport;
    return nullptr;
}

#include "qwindowsprintersupport.moc"

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
