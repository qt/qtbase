/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcupsprintersupport_p.h"

#include "qcupsprintengine_p.h"
#include <private/qprinterinfo_p.h>

#include <QtPrintSupport/QPrinterInfo>

#include "qcups_p.h"

QT_BEGIN_NAMESPACE

QCupsPrinterSupport::QCupsPrinterSupport() : QPlatformPrinterSupport()
{
}

QCupsPrinterSupport::~QCupsPrinterSupport()
{
}

QPrintEngine *QCupsPrinterSupport::createNativePrintEngine(QPrinter::PrinterMode printerMode)
{
    return new QCupsPrintEngine(printerMode);
}

QPaintEngine *QCupsPrinterSupport::createPaintEngine(QPrintEngine *engine, QPrinter::PrinterMode printerMode)
{
    Q_UNUSED(printerMode)
    return static_cast<QCupsPrintEngine *>(engine);
}

QList<QPrinter::PaperSize> QCupsPrinterSupport::supportedPaperSizes(const QPrinterInfo &printerInfo) const
{
    return QCUPSSupport::getCupsPrinterPaperSizes(printerInfoCupsPrinterIndex(printerInfo));
}

QList<QPrinterInfo> QCupsPrinterSupport::availablePrinters()
{
    QList<QPrinterInfo> printers;
    foreach (const QCUPSSupport::Printer &p,  QCUPSSupport::availableUnixPrinters()) {
        QPrinterInfo printer(p.name);
        printer.d_func()->isDefault = p.isDefault;
        setPrinterInfoCupsPrinterIndex(&printer, p.cupsPrinterIndex);
        printers.append(printer);
    }
    return printers;
}

int QCupsPrinterSupport::printerInfoCupsPrinterIndex(const QPrinterInfo &p)
{
    return p.isNull() ? -1 : p.d_func()->cupsPrinterIndex;
}

void QCupsPrinterSupport::setPrinterInfoCupsPrinterIndex(QPrinterInfo *p, int index)
{
    p->d_func()->cupsPrinterIndex = index;
}

QT_END_NAMESPACE
