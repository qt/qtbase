/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcupsprintersupport_p.h"

#ifndef QT_NO_PRINTER

#include "qcupsprintengine_p.h"
#include <private/qprinterinfo_p.h>

#include <QtPrintSupport/QPrinterInfo>

#include <cups/ppd.h>
#ifndef QT_LINUXBASE // LSB merges everything into cups.h
# include <cups/language.h>
#endif

QT_BEGIN_NAMESPACE

QCupsPrinterSupport::QCupsPrinterSupport() : QPlatformPrinterSupport(),
                                             m_cups(QLatin1String("cups"), 2),
                                             m_cupsPrinters(0),
                                             m_cupsPrintersCount(0)
{
    loadCups();
    loadCupsPrinters();
}

QCupsPrinterSupport::~QCupsPrinterSupport()
{
    if (cupsFreeDests)
        cupsFreeDests(m_cupsPrintersCount, m_cupsPrinters);
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
    return QCUPSSupport::getCupsPrinterPaperSizes(printerIndex(printerInfo));
}

QList<QPair<QString, QSizeF> > QCupsPrinterSupport::supportedSizesWithNames(const QPrinterInfo &printerInfo) const
{
    return QCUPSSupport::getCupsPrinterPaperSizesWithNames(printerIndex(printerInfo));
}

void QCupsPrinterSupport::loadCups()
{
    cupsGetDests = (CupsGetDests) m_cups.resolve("cupsGetDests");
    cupsFreeDests = (CupsFreeDests) m_cups.resolve("cupsFreeDests");
    cupsGetOption = (CupsGetOption) m_cups.resolve("cupsGetOption");
}

void QCupsPrinterSupport::loadCupsPrinters()
{
    m_cupsPrintersCount = 0;
    m_printers.clear();

    if (cupsFreeDests)
        cupsFreeDests(m_cupsPrintersCount, m_cupsPrinters);

    if (cupsGetDests)
        m_cupsPrintersCount = cupsGetDests(&m_cupsPrinters);

    for (int i = 0; i < m_cupsPrintersCount; ++i) {
        QString printerName = QString::fromLocal8Bit(m_cupsPrinters[i].name);
        if (m_cupsPrinters[i].instance)
            printerName += QLatin1Char('/') + QString::fromLocal8Bit(m_cupsPrinters[i].instance);
        QString description = cupsOption(i, "printer-info");
        QString location = cupsOption(i, "printer-location");
        QString makeAndModel = cupsOption(i, "printer-make-and-model");
        QPrinterInfo printer = createPrinterInfo(printerName, description, location, makeAndModel,
                                                 m_cupsPrinters[i].is_default, i);
        m_printers.append(printer);
    }
}

QString QCupsPrinterSupport::printerOption(const QPrinterInfo &printer, const QString &key) const
{
    return cupsOption(printerIndex(printer), key);
}

QString QCupsPrinterSupport::cupsOption(int i, const QString &key) const
{
    QString value;
    if (i > -1 && i < m_cupsPrintersCount && cupsGetOption)
        value = cupsGetOption(key.toLocal8Bit(), m_cupsPrinters[i].num_options, m_cupsPrinters[i].options);
    return value;
}

PrinterOptions QCupsPrinterSupport::printerOptions(const QPrinterInfo &printer) const
{
    PrinterOptions options;
    int p = printerIndex(printer);
    if (p <= -1 || p >= m_cupsPrintersCount)
        return options;
    int numOptions = m_cupsPrinters[p].num_options;
    for (int i = 0; i < numOptions; ++i) {
        QString name = m_cupsPrinters[p].options[i].name;
        QString value = m_cupsPrinters[p].options[i].value;
        options.insert(name, value);
    }
    return options;
}

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
