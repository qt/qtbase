/****************************************************************************
**
** Copyright (C) 2014 John Layt <jlayt@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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
    QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
    if (!ps)
        QSKIP("Could not load platform plugin");

    QString defaultId = ps->defaultPrintDeviceId();
    if (defaultId.isEmpty()) {
        qDebug() << "No default printer found";
    } else {
        qDebug() << "Default Printer ID :" << defaultId;
        QVERIFY(ps->availablePrintDeviceIds().contains(defaultId));
    }

    qDebug() << "Available Printer IDs :" << ps->availablePrintDeviceIds();

    // Just exercise the api for now as we don't know what is installed
    foreach (const QString id, ps->availablePrintDeviceIds()) {
        QPrintDevice printDevice = ps->createPrintDevice(id);
        qDebug() << "Created printer" << id;
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
}

QTEST_MAIN(tst_QPrintDevice)

#include "tst_qprintdevice.moc"
