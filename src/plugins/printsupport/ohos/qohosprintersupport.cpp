// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosnativeprint.h"
#include "qohospdfprintengine.h"
#include "qohosprintdevice.h"
#include "qohosprintersupport.h"
#include <QtCore/private/qohoslogger_p.h>

QT_BEGIN_NAMESPACE

QOhosPrinterSupport::QOhosPrinterSupport()
    : QPlatformPrinterSupport()
{
    qOhosDebug(QtForOhos) << "Creating OHOS print support plugin";
    if (!QOhosNativePrint::initializePrintService()) {
        qOhosCritical(QtForOhos)
            << "Failed to initialize native Print Service during OHOS print support plugin initialization";
    }
    if (!QOhosNativePrint::launchPrinterManager()) {
        qOhosCritical(QtForOhos)
            << "Failed to launch Printer Manager during OHOS print support plugin initialization";
    }
}

QOhosPrinterSupport::~QOhosPrinterSupport()
{
    QOhosNativePrint::releasePrintService();
}

QPrintEngine *QOhosPrinterSupport::createNativePrintEngine(QPrinter::PrinterMode printerMode, const QString &deviceId)
{
    auto __dbg = make_QCScopedDebug("QOhosPrinterSupport::createNativePrintEngine");
    return new QOhosPdfPrintEngine(printerMode, deviceId);
}

QPaintEngine *QOhosPrinterSupport::createPaintEngine(QPrintEngine *engine, QPrinter::PrinterMode printerMode)
{
    Q_UNUSED(printerMode)
    auto __dbg = make_QCScopedDebug("QOhosPrinterSupport::createPaintEngine");
    return static_cast<QOhosPdfPrintEngine *>(engine);
}

QPrintDevice QOhosPrinterSupport::createPrintDevice(const QString &id)
{
    auto __dbg = make_QCScopedDebug("QOhosPrinterSupport::createPrintDevice");
    return QPlatformPrinterSupport::createPrintDevice(new QOhosPrintDevice(id));
}

QStringList QOhosPrinterSupport::availablePrintDeviceIds() const
{
    auto __dbg = make_QCScopedDebug("QOhosPrinterSupport::availablePrintDeviceIds");
    return QOhosPrintDevice::availablePrintDeviceIds();
}

QString QOhosPrinterSupport::defaultPrintDeviceId() const
{
    auto __dbg = make_QCScopedDebug("QOhosPrinterSupport::defaultPrintDeviceId");
    return QOhosPrintDevice::defaultPrintDeviceId();
}

QT_END_NAMESPACE
