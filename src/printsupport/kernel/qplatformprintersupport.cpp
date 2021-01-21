/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qplatformprintersupport.h"
#include "qplatformprintdevice.h"

#include <QtGui/qpagesize.h>
#include <QtPrintSupport/qprinterinfo.h>

#include <private/qprinterinfo_p.h>
#include <private/qprintdevice_p.h>

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

QPrintEngine *QPlatformPrinterSupport::createNativePrintEngine(QPrinter::PrinterMode, const QString &)
{
    return nullptr;
}

QPaintEngine *QPlatformPrinterSupport::createPaintEngine(QPrintEngine *, QPrinter::PrinterMode)
{
    return nullptr;
}

QPrintDevice QPlatformPrinterSupport::createPrintDevice(QPlatformPrintDevice *device)
{
    return QPrintDevice(device);
}

QPrintDevice QPlatformPrinterSupport::createPrintDevice(const QString &id)
{
    Q_UNUSED(id)
    return QPrintDevice();
}

QStringList QPlatformPrinterSupport::availablePrintDeviceIds() const
{
    return QStringList();
}

QString QPlatformPrinterSupport::defaultPrintDeviceId() const
{
    return QString();
}

QPageSize QPlatformPrinterSupport::createPageSize(const QString &id, QSize size, const QString &localizedName)
{
    Q_UNUSED(id)
    Q_UNUSED(size)
    Q_UNUSED(localizedName)
    return QPageSize();
}

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
