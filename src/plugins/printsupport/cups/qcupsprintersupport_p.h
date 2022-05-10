// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2014 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCUPSPRINTERSUPPORT_H
#define QCUPSPRINTERSUPPORT_H

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

#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QCupsPrinterSupport : public QPlatformPrinterSupport
{
public:
    QCupsPrinterSupport();
    ~QCupsPrinterSupport();

    QPrintEngine *createNativePrintEngine(QPrinter::PrinterMode printerMode, const QString &deviceId = QString()) override;
    QPaintEngine *createPaintEngine(QPrintEngine *printEngine, QPrinter::PrinterMode) override;

    QPrintDevice createPrintDevice(const QString &id) override;
    QStringList availablePrintDeviceIds() const override;
    QString defaultPrintDeviceId() const override;

    static QString staticDefaultPrintDeviceId();

private:
    QString cupsOption(int i, const QString &key) const;
};

QT_END_NAMESPACE

#endif // QCUPSPRINTERSUPPORT_H
