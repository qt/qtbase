// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qprinterinfo.h"
#include "qprinterinfo_p.h"

#include <qstringlist.h>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#ifndef QT_NO_PRINTER

extern QPrinter::PaperSize mapDevmodePaperSize(int s);

//QList<QPrinterInfo> QPrinterInfo::availablePrinters()
//{
//    QList<QPrinterInfo> printers;

//    DWORD needed = 0;
//    DWORD returned = 0;
//    if (!EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 4, 0, 0, &needed, &returned)) {
//        LPBYTE buffer = new BYTE[needed];
//        if (EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 4, buffer, needed, &needed, &returned)) {
//            PPRINTER_INFO_4 infoList = reinterpret_cast<PPRINTER_INFO_4>(buffer);
//            QPrinterInfo defPrn = defaultPrinter();
//            for (uint i = 0; i < returned; ++i) {
//                QString printerName(QString::fromWCharArray(infoList[i].pPrinterName));

//                QPrinterInfo printerInfo(printerName);
//                if (printerInfo.printerName() == defPrn.printerName())
//                    printerInfo.d_ptr->isDefault = true;
//                printers.append(printerInfo);
//            }
//        }
//        delete [] buffer;
//    }

//    return printers;
//}

//QPrinterInfo QPrinterInfo::defaultPrinter()
//{
//    QString noPrinters("qt_no_printers"_L1);
//    wchar_t buffer[256];
//    GetProfileString(L"windows", L"device", (wchar_t*)noPrinters.utf16(), buffer, 256);
//    QString output = QString::fromWCharArray(buffer);
//    if (output != noPrinters) {
//        // Filter out the name of the printer, which should be everything before a comma.
//        QString printerName = output.split(u',').value(0);
//        QPrinterInfo printerInfo(printerName);
//        printerInfo.d_ptr->isDefault = true;
//        return printerInfo;
//    }

//    return QPrinterInfo();
//}

//QList<QPrinter::PaperSize> QPrinterInfo::supportedPaperSizes() const
//{
//    const Q_D(QPrinterInfo);

//    QList<QPrinter::PaperSize> paperSizes;
//    if (isNull())
//        return paperSizes;

//    DWORD size = DeviceCapabilities(reinterpret_cast<const wchar_t*>(d->name.utf16()),
//                                    NULL, DC_PAPERS, NULL, NULL);
//    if ((int)size != -1) {
//        wchar_t *papers = new wchar_t[size];
//        size = DeviceCapabilities(reinterpret_cast<const wchar_t*>(d->name.utf16()),
//                                  NULL, DC_PAPERS, papers, NULL);
//        for (int c = 0; c < (int)size; ++c)
//            paperSizes.append(mapDevmodePaperSize(papers[c]));
//        delete [] papers;
//    }

//    return paperSizes;
//}

#endif // QT_NO_PRINTER

QT_END_NAMESPACE
