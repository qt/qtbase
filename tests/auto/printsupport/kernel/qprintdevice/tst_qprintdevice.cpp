/****************************************************************************
**
** Copyright (C) 2014 John Layt <jlayt@kde.org>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <qpa/qplatformprintplugin.h>
#include <qpa/qplatformprintersupport.h>

#include <private/qprintdevice_p.h>

class tst_QPrintDevice : public QObject
{
    Q_OBJECT

private slots:
    void basics();
};

void tst_QPrintDevice::basics()
{
#ifndef QT_NO_PRINTER
    QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
    if (!ps)
        QSKIP("Could not load platform plugin");

    QString defaultId = ps->defaultPrintDeviceId();
    if (defaultId.isEmpty()) {
        qDebug() << "No default printer found";
    } else {
        QVERIFY(ps->availablePrintDeviceIds().contains(defaultId));
    }

    qDebug() << "Available Printer IDs :" << ps->availablePrintDeviceIds();

    // Just exercise the api for now as we don't know what is installed
    foreach (const QString id, ps->availablePrintDeviceIds()) {
        QPrintDevice printDevice = ps->createPrintDevice(id);
        const char quote = id == defaultId ? '*' : '"';
        qDebug().noquote().nospace() << "\nCreated printer " << quote << id
            << quote << ":\n" << printDevice << '\n';
        QCOMPARE(printDevice.isValid(), true);
        printDevice.id();
        printDevice.name();
        printDevice.location();
        printDevice.makeAndModel();
        printDevice.isValid();
        printDevice.isDefault();
        printDevice.isRemote();
        printDevice.state();
        printDevice.supportsMultipleCopies();
        printDevice.supportsCollateCopies();
        printDevice.defaultPageSize();
        printDevice.supportedPageSizes();
        printDevice.supportsCustomPageSizes();
        printDevice.minimumPhysicalPageSize();
        printDevice.maximumPhysicalPageSize();
        printDevice.defaultResolution();
        printDevice.supportedResolutions();
        printDevice.defaultInputSlot();
        printDevice.supportedInputSlots();
        printDevice.defaultOutputBin();
        printDevice.supportedOutputBins();
        printDevice.defaultDuplexMode();
        printDevice.supportedDuplexModes();
        printDevice.defaultColorMode();
        printDevice.supportedColorModes();
        printDevice.supportedMimeTypes();
    }
#endif // QT_NO_PRINTER
}

QTEST_MAIN(tst_QPrintDevice)

#include "tst_qprintdevice.moc"
