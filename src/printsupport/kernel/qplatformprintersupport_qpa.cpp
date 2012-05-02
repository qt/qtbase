/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qplatformprintersupport_qpa.h"

#include <QtPrintSupport/qprinterinfo.h>

#include <private/qprinterinfo_p.h>

#ifndef QT_NO_PRINTER

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformPrinterSupport
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformPrinterSupport class provides an abstraction for print support.
 */

QPlatformPrinterSupport::QPlatformPrinterSupport()
{
}

QPlatformPrinterSupport::~QPlatformPrinterSupport()
{
}

QPrintEngine *QPlatformPrinterSupport::createNativePrintEngine(QPrinter::PrinterMode)
{
    return 0;
}

QPaintEngine *QPlatformPrinterSupport::createPaintEngine(QPrintEngine *, QPrinter::PrinterMode)
{
    return 0;
}

QList<QPrinter::PaperSize> QPlatformPrinterSupport::supportedPaperSizes(const QPrinterInfo &) const
{
    return QList<QPrinter::PaperSize>();
}

QList<QPrinterInfo> QPlatformPrinterSupport::availablePrinters()
{
    return QList<QPrinterInfo>();
}

QPrinterInfo QPlatformPrinterSupport::defaultPrinter()
{
    const QList<QPrinterInfo> printers = availablePrinters();
    foreach (const QPrinterInfo &printerInfo, printers) {
        if (printerInfo.isDefault())
            return printerInfo;
    }
    return printers.isEmpty() ? QPrinterInfo() : printers.front();
}

QPrinterInfo QPlatformPrinterSupport::printerInfo(const QString &printerName, bool isDefault)
{
    QPrinterInfo pi = QPrinterInfo(printerName);
    pi.d_func()->isDefault = isDefault;
    return pi;
}

void QPlatformPrinterSupport::setPrinterInfoDefault(QPrinterInfo *p, bool isDefault)
{
    p->d_func()->isDefault = isDefault;
}

bool QPlatformPrinterSupport::printerInfoIsDefault(const QPrinterInfo &p)
{
    return p.d_func()->isDefault;
}

int QPlatformPrinterSupport::printerInfoCupsPrinterIndex(const QPrinterInfo &p)
{
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    return p.isNull() ? -1 : p.d_func()->cupsPrinterIndex;
#else
    Q_UNUSED(p)
    return -1;
#endif
}

void QPlatformPrinterSupport::setPrinterInfoCupsPrinterIndex(QPrinterInfo *p, int index)
{
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    p->d_func()->cupsPrinterIndex = index;
#else
    Q_UNUSED(p)
    Q_UNUSED(index)
#endif
}

/*
    Converts QSizeF in millimeters to a predefined PaperSize (returns Custom if
    the size isn't a standard size)
*/
extern QPrinter::PaperSize qSizeFTopaperSize(const QSizeF &);
QPrinter::PaperSize QPlatformPrinterSupport::convertQSizeFToPaperSize(const QSizeF &sizef)
{
    return qSizeFTopaperSize(sizef);
}

/*
    Converts a predefined PaperSize to a QSizeF in millimeters (returns
    QSizeF(0.0, 0.0) if PaperSize is Custom)
*/
extern QSizeF qt_paperSizeToQSizeF(QPrinter::PaperSize size);
QSizeF QPlatformPrinterSupport::convertPaperSizeToQSizeF(QPrinter::PaperSize paperSize)
{
    return qt_paperSizeToQSizeF(paperSize);
}

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
