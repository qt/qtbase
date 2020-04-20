/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtPrintSupport module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcocoaprintersupport.h"

#ifndef QT_NO_PRINTER

#include <AppKit/AppKit.h>

#include <QtCore/private/qcore_mac_p.h>

#include "qcocoaprintdevice.h"
#include "qprintengine_mac_p.h"

#include <private/qprinterinfo_p.h>

QT_BEGIN_NAMESPACE

QCocoaPrinterSupport::QCocoaPrinterSupport()
{ }

QCocoaPrinterSupport::~QCocoaPrinterSupport()
{ }

QPrintEngine *QCocoaPrinterSupport::createNativePrintEngine(QPrinter::PrinterMode printerMode, const QString &deviceId)
{
    return new QMacPrintEngine(printerMode, deviceId);
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

QPrintDevice QCocoaPrinterSupport::createPrintDevice(const QString &id)
{
    return QPlatformPrinterSupport::createPrintDevice(new QCocoaPrintDevice(id));
}

QStringList QCocoaPrinterSupport::availablePrintDeviceIds() const
{
    QStringList list;
    QCFType<CFArrayRef> printerList;
    if (PMServerCreatePrinterList(kPMServerLocal, &printerList) == noErr) {
        CFIndex count = CFArrayGetCount(printerList);
        for (CFIndex i = 0; i < count; ++i) {
            PMPrinter printer = static_cast<PMPrinter>(const_cast<void *>(CFArrayGetValueAtIndex(printerList, i)));
            list.append(QString::fromCFString(PMPrinterGetID(printer)));
        }
    }
    return list;
}

QString QCocoaPrinterSupport::defaultPrintDeviceId() const
{
    QCFType<CFArrayRef> printerList;
    if (PMServerCreatePrinterList(kPMServerLocal, &printerList) == noErr) {
        CFIndex count = CFArrayGetCount(printerList);
        for (CFIndex i = 0; i < count; ++i) {
            PMPrinter printer = static_cast<PMPrinter>(const_cast<void *>(CFArrayGetValueAtIndex(printerList, i)));
            if (PMPrinterIsDefault(printer))
                return QString::fromCFString(PMPrinterGetID(printer));
        }
    }
    return QString();
}

QT_END_NAMESPACE

#endif  //QT_NO_PRINTER
