// Copyright (C) 2014 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QMimeType>

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
    const QStringList availableIds = ps->availablePrintDeviceIds();
    if (defaultId.isEmpty()) {
        qDebug() << "No default printer found";
    } else {
        QVERIFY(availableIds.contains(defaultId));
    }

    qDebug() << "Available Printer IDs :" << availableIds;

    // Just exercise the api for now as we don't know what is installed
    for (const QString &id : availableIds) {
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
