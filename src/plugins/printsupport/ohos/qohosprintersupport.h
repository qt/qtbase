// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPRINTERSUPPORT_H
#define QOHOSPRINTERSUPPORT_H

#include <qpa/qplatformprintersupport.h>

QT_BEGIN_NAMESPACE

class QOhosPrinterSupport : public QPlatformPrinterSupport
{
    Q_DISABLE_COPY(QOhosPrinterSupport)

public:
    QOhosPrinterSupport();
    ~QOhosPrinterSupport() override;

    QPrintEngine *createNativePrintEngine(QPrinter::PrinterMode printerMode, const QString &deviceId = QString()) override;
    QPaintEngine *createPaintEngine(QPrintEngine *printEngine, QPrinter::PrinterMode printerMode) override;

    QPrintDevice createPrintDevice(const QString &id) override;
    QStringList availablePrintDeviceIds() const override;
    QString defaultPrintDeviceId() const override;
};

QT_END_NAMESPACE

#endif // QOHOSPRINTERSUPPORT_H
