// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsprintersupport_p.h"

#ifndef QT_NO_PRINTER

#include "qwindowsprintdevice_p.h"

#include <QtCore/QStringList>
#include <private/qprintengine_win_p.h>
#include <private/qprintdevice_p.h>

#define QT_STATICPLUGIN
#include <qpa/qplatformprintplugin.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
    if (key.compare(key, "windowsprintsupport"_L1, Qt::CaseInsensitive) == 0)
        return new QWindowsPrinterSupport;
    return nullptr;
}

QT_END_NAMESPACE

#include "qwindowsprintersupport.moc"

#endif // QT_NO_PRINTER
