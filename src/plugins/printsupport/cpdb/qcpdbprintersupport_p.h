// Copyright (C) 2022-2023 Gaurav Guleria <tinytrebuchet@protonmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCPDBPRINTERSUPPORT_H
#define QCPDBPRINTERSUPPORT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qpa/qplatformprintersupport.h>

#include <qcpdb_p.h>

QT_BEGIN_NAMESPACE

class QCpdbPrinterSupport : public QPlatformPrinterSupport
{
public:
    QCpdbPrinterSupport();
    ~QCpdbPrinterSupport();

    QPrintEngine *createNativePrintEngine(QPrinter::PrinterMode p, const QString &deviceId = QString()) override;
    QPaintEngine *createPaintEngine(QPrintEngine *printEngine, QPrinter::PrinterMode) override;

    QPrintDevice createPrintDevice(const QString &id) override;
    QStringList availablePrintDeviceIds() const override;
    QString defaultPrintDeviceId() const override;

    cpdb_frontend_obj_t *frontendObj;

private:

};

QT_END_NAMESPACE

#endif // QCPDBPRINTERSUPPORT_H
