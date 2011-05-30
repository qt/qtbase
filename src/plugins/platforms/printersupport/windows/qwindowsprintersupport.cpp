/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qwindowsprintersupport.h>
#include <qprintengine_win_p.h>

#include <QtGui/QPrinterInfo>

#include <QtCore/QStringList>

#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

QPrintEngine *QWindowsPrinterSupport::createNativePrintEngine(QPrinter::PrinterMode mode)
{
    return new QWin32PrintEngine(mode);
}

QPaintEngine *QWindowsPrinterSupport::createPaintEngine(QPrintEngine *engine, QPrinter::PrinterMode)
{
    return static_cast<QWin32PrintEngine *>(engine);
}

QList<QPrinter::PaperSize> QWindowsPrinterSupport::supportedPaperSizes(const QPrinterInfo &printerInfo) const
{
    QList<QPrinter::PaperSize> paperSizes;
    const QString printerName = printerInfo.printerName();
    const wchar_t *nameUtf16 = reinterpret_cast<const wchar_t*>(printerName.utf16());
    DWORD size = DeviceCapabilities(nameUtf16, NULL, DC_PAPERS, NULL, NULL);
    if ((int)size != -1) {
        wchar_t *papers = new wchar_t[size];
        size = DeviceCapabilities(nameUtf16, NULL, DC_PAPERS, papers, NULL);
        for (int c = 0; c < (int)size; ++c)
            paperSizes.append(mapDevmodePaperSize(papers[c]));
        delete [] papers;
    }
    return paperSizes;
}

QList<QPrinterInfo> QWindowsPrinterSupport::availablePrinters()
{
    QList<QPrinterInfo> printers;

    DWORD needed = 0;
    DWORD returned = 0;
    if (!EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 4, 0, 0, &needed, &returned)) {
        LPBYTE buffer = new BYTE[needed];
        if (EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 4, buffer, needed, &needed, &returned)) {
            PPRINTER_INFO_4 infoList = reinterpret_cast<PPRINTER_INFO_4>(buffer);
            QPrinterInfo defPrn = defaultPrinter();
            for (uint i = 0; i < returned; ++i) {
                const QString printerName(QString::fromWCharArray(infoList[i].pPrinterName));
                const bool isDefault = printerName == defPrn.printerName();
                printers.append(QPlatformPrinterSupport::printerInfo(printerName,
                                                                     isDefault));
            }
        }
        delete [] buffer;
    }

    return printers;
}

QPrinterInfo QWindowsPrinterSupport::defaultPrinter()
{
    QString noPrinters(QLatin1String("qt_no_printers"));
    wchar_t buffer[256];
    GetProfileString(L"windows", L"device", (wchar_t*)noPrinters.utf16(), buffer, 256);
    QString output = QString::fromWCharArray(buffer);
    if (output != noPrinters) {
        // Filter out the name of the printer, which should be everything before a comma.
        const QString printerName = output.split(QLatin1Char(',')).value(0);
        return QPlatformPrinterSupport::printerInfo(printerName, true);
    }

    return QPrinterInfo();
}

QT_END_NAMESPACE
