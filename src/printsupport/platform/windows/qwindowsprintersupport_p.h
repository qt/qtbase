// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef WINDOWSPRINTERSUPPORT_H
#define WINDOWSPRINTERSUPPORT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of internal files. This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtPrintSupport/qtprintsupportglobal.h>

#include <qpa/qplatformprintersupport.h>
#include <private/qglobal_p.h>
#ifndef QT_NO_PRINTER

QT_BEGIN_NAMESPACE

class Q_PRINTSUPPORT_EXPORT QWindowsPrinterSupport : public QPlatformPrinterSupport
{
    Q_DISABLE_COPY_MOVE(QWindowsPrinterSupport)
public:
    QWindowsPrinterSupport();
    ~QWindowsPrinterSupport() override;

    QPrintEngine *createNativePrintEngine(QPrinter::PrinterMode printerMode, const QString &deviceId = QString()) override;
    QPaintEngine *createPaintEngine(QPrintEngine *printEngine, QPrinter::PrinterMode) override;

    QPrintDevice createPrintDevice(const QString &id) override;
    QStringList availablePrintDeviceIds() const override;
    QString defaultPrintDeviceId() const override;
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
#endif // WINDOWSPRINTERSUPPORT_H
