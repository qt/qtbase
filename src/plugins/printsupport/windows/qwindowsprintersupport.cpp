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

#include "qwindowsprintersupport.h"
#include "qwindowsprintdevice.h"

#include <QtCore/QList>
#include <QtCore/QScopedArrayPointer>
#include <QtPrintSupport/QPrinterInfo>
#include <qprintengine_win_p.h>
#include <private/qpaintengine_alpha_p.h>
#include <private/qprinterinfo_p.h>
#include <private/qprintdevice_p.h>

QT_BEGIN_NAMESPACE

QWindowsPrinterSupport::QWindowsPrinterSupport()
    : QPlatformPrinterSupport()
{
    m_printers = QWindowsPrinterSupport::queryPrinters();
}

QWindowsPrinterSupport::~QWindowsPrinterSupport()
{
}

QPrintEngine *QWindowsPrinterSupport::createNativePrintEngine(QPrinter::PrinterMode printerMode)
{
        return new QWin32PrintEngine(printerMode);
}

QPaintEngine *QWindowsPrinterSupport::createPaintEngine(QPrintEngine *engine, QPrinter::PrinterMode printerMode)
{
    Q_UNUSED(printerMode)
    return static_cast<QWin32PrintEngine *>(engine);
}

QPrintDevice QWindowsPrinterSupport::createPrintDevice(const QString &id)
{
    return QPlatformPrinterSupport::createPrintDevice(new QWindowsPrintDevice(id));
}

QStringList QWindowsPrinterSupport::availablePrintDeviceIds() const
{
    return QWindowsPrintDevice::availablePrintDeviceIds();
}

QString QWindowsPrinterSupport::defaultPrintDeviceId() const
{
    return QWindowsPrintDevice::defaultPrintDeviceId();
}

QList<QPrinter::PaperSize> QWindowsPrinterSupport::supportedPaperSizes(const QPrinterInfo &printerInfo) const
{
    return QWin32PrintEngine::supportedPaperSizes(printerInfo);
}

QList<QPair<QString, QSizeF> >QWindowsPrinterSupport::supportedSizesWithNames(const QPrinterInfo &printerInfo) const
{
    return QWin32PrintEngine::supportedSizesWithNames(printerInfo);
}

QList<QPrinterInfo> QWindowsPrinterSupport::availablePrinters()
{
    m_printers = QWindowsPrinterSupport::queryPrinters();
    return QPlatformPrinterSupport::availablePrinters();
}

QList<QPrinterInfo> QWindowsPrinterSupport::queryPrinters()
{
    QList<QPrinterInfo> result;
    DWORD needed = 0;
    DWORD returned = 0;
    if ((!EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 4, 0, 0, &needed, &returned) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        || !needed) {
        return result;
    }
    QScopedArrayPointer<BYTE> buffer(new BYTE[needed]);
    if (!EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 4, buffer.data(), needed, &needed, &returned))
        return result;
    PPRINTER_INFO_4 infoList = reinterpret_cast<PPRINTER_INFO_4>(buffer.data());
    QString defaultPrinterName;
    QWin32PrintEngine::queryDefaultPrinter(defaultPrinterName);
    for (uint i = 0; i < returned; ++i) {
        const QString printerName(QString::fromWCharArray(infoList[i].pPrinterName));
        const bool isDefault = (printerName == defaultPrinterName);
        result.append(QPlatformPrinterSupport::createPrinterInfo(printerName, QString(), QString(), QString(), isDefault, i));
    }
    return result;
}

QT_END_NAMESPACE
