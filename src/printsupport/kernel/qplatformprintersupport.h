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

#ifndef QPLATFORMPRINTINGSUPPORT_H
#define QPLATFORMPRINTINGSUPPORT_H
//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtPrintSupport/qprinter.h>

#include <QtCore/qlist.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

class QPrintEngine;

class Q_PRINTSUPPORT_EXPORT QPlatformPrinterSupport
{
public:
    QPlatformPrinterSupport();
    virtual ~QPlatformPrinterSupport();

    virtual QPrintEngine *createNativePrintEngine(QPrinter::PrinterMode printerMode);
    virtual QPaintEngine *createPaintEngine(QPrintEngine *, QPrinter::PrinterMode printerMode);
    virtual QList<QPrinter::PaperSize> supportedPaperSizes(const QPrinterInfo &) const;

    virtual QList<QPrinterInfo> availablePrinters();
    virtual QPrinterInfo defaultPrinter();
    virtual QPrinterInfo printerInfo(const QString &printerName);

    static QPrinter::PaperSize convertQSizeFToPaperSize(const QSizeF &sizef);
    static QSizeF convertPaperSizeToQSizeF(QPrinter::PaperSize paperSize);

protected:
    static int printerIndex(const QPrinterInfo &printer);
    static QPrinterInfo createPrinterInfo(const QString &name, const QString &description,
                                          const QString &location, const QString &makeAndModel,
                                          bool isDefault, int index);

    QList<QPrinterInfo> m_printers;
};

#endif // QT_NO_PRINTER

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPLATFORMPRINTINGSUPPORT_H
