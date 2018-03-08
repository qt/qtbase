/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
