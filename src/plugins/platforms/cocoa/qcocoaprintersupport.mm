/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtPrintSupport module of the Qt Toolkit.
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

#include "qcocoaprintersupport.h"
#include "qprintengine_mac_p.h"

#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrinterInfo>
#include <private/qprinterinfo_p.h>

QCocoaPrinterSupport::QCocoaPrinterSupport()
{ }

QCocoaPrinterSupport::~QCocoaPrinterSupport()
{ }

QPrintEngine *QCocoaPrinterSupport::createNativePrintEngine(QPrinter::PrinterMode printerMode)
{
    return new QMacPrintEngine(printerMode);
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

QList<QPrinter::PaperSize> QCocoaPrinterSupport::supportedPaperSizes(const QPrinterInfo &printerInfo) const
{
    QList<QPrinter::PaperSize> returnValue;
    if (printerInfo.isNull())
        return returnValue;

    PMPrinter printer = PMPrinterCreateFromPrinterID(QCFString::toCFStringRef(printerInfo.printerName()));
    if (!printer)
        return returnValue;

    CFArrayRef array;
    if (PMPrinterGetPaperList(printer, &array) != noErr) {
        PMRelease(printer);
        return returnValue;
    }

    CFIndex count = CFArrayGetCount(array);
    for (CFIndex i = 0; i < count; ++i) {
        PMPaper paper = static_cast<PMPaper>(const_cast<void *>(CFArrayGetValueAtIndex(array, i)));
        double width, height;
        if (PMPaperGetWidth(paper, &width) == noErr
            && PMPaperGetHeight(paper, &height) == noErr) {
            // width and height are in points, convertQSizeFToPaperSize() expects millimeters
            static const double OnePointInMillimeters = 1.0 / 72.0 * 25.4;
            QSizeF size(width * OnePointInMillimeters, height * OnePointInMillimeters);
            returnValue += QPlatformPrinterSupport::convertQSizeFToPaperSize(size);
        }
    }

    PMRelease(printer);

    return returnValue;
}

QList<QPrinterInfo> QCocoaPrinterSupport::availablePrinters()
{
    QList<QPrinterInfo> returnValue;
    QCFType<CFArrayRef> printerList;
    if (PMServerCreatePrinterList(kPMServerLocal, &printerList) == noErr) {
        CFIndex count = CFArrayGetCount(printerList);
        for (CFIndex i = 0; i < count; ++i) {
            PMPrinter printer = static_cast<PMPrinter>(const_cast<void *>(CFArrayGetValueAtIndex(printerList, i)));
            returnValue += printerInfoFromPMPrinter(printer);
        }
    }
    return returnValue;
}

QPrinterInfo QCocoaPrinterSupport::printerInfo(const QString &printerName)
{
    PMPrinter printer = PMPrinterCreateFromPrinterID(QCFString::toCFStringRef(printerName));
    QPrinterInfo pi = printerInfoFromPMPrinter(printer);
    PMRelease(printer);
    return pi;
}

QPrinterInfo QCocoaPrinterSupport::printerInfoFromPMPrinter(const PMPrinter &printer)
{
    if (!printer)
        return QPrinterInfo();

    QPrinterInfo pi = QPrinterInfo(QCFString::toQString(PMPrinterGetID(printer)));
    pi.d_func()->isDefault = PMPrinterIsDefault(printer);

    return pi;
}
