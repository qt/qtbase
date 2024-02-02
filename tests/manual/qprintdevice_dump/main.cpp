// Copyright (C) 2014 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

#include <qpa/qplatformprintplugin.h>
#include <qpa/qplatformprintersupport.h>

#include <private/qprintdevice_p.h>

#include <QApplication>
#include <QMimeType>
#include <QDebug>

/*
    This test is designed to dump the current printer configuration details
    to output, to assist in debugging of print device problems.
*/

static QString stateToString(QPrint::DeviceState state)
{
    switch (state) {
    case QPrint::Idle:
        return QStringLiteral("Idle");
    case QPrint::Active:
        return QStringLiteral("Active");
    case QPrint::Aborted:
        return QStringLiteral("Aborted");
    case QPrint::Error:
        return QStringLiteral("Error");
    }
    return QStringLiteral("Invalid DeviceState");
}

static QString duplexToString(QPrint::DuplexMode duplex)
{
    switch (duplex) {
    case QPrint::DuplexNone:
        return QStringLiteral("DuplexNone");
    case QPrint::DuplexAuto:
        return QStringLiteral("DuplexAuto");
    case QPrint::DuplexLongSide:
        return QStringLiteral("DuplexLongSide");
    case QPrint::DuplexShortSide:
        return QStringLiteral("DuplexShortSide");
    }
    return QStringLiteral("Invalid DuplexMode");
}

static QString colorToString(QPrint::ColorMode color)
{
    switch (color) {
    case QPrint::GrayScale:
        return QStringLiteral("GrayScale");
    case QPrint::Color:
        return QStringLiteral("Color");
    }
    return QStringLiteral("Invalid ColorMode");
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qDebug() << "\n********************************";
    qDebug() << "***** QPrintDevice Details *****";
    qDebug() << "********************************\n";

    QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
    if (!ps) {
        qDebug() << "Could not load platform plugin!";
        return -1;
    }

    QString defaultId = ps->defaultPrintDeviceId();
    if (defaultId.isEmpty())
        qDebug() << "No default printer found";
    else
        qDebug() << "Default Printer ID    :" << defaultId;
    qDebug() << "Available Printer IDs :" << ps->availablePrintDeviceIds() << "\n";

    foreach (const QString id, ps->availablePrintDeviceIds()) {
        QPrintDevice printDevice = ps->createPrintDevice(id);
        if (printDevice.isValid()) {
            qDebug() << "===" << printDevice.id() << "===\n";
            qDebug() << "Device ID       :" << printDevice.id();
            qDebug() << "Device Name     :" << printDevice.name();
            qDebug() << "Device Location :" << printDevice.location();
            qDebug() << "Device Make     :" << printDevice.makeAndModel();
            qDebug() << "";
            qDebug() << "isValid   :" << printDevice.isValid();
            qDebug() << "isDefault :" << printDevice.isDefault();
            qDebug() << "isRemote  :" << printDevice.isRemote();
            qDebug() << "";
            qDebug() << "state :" << stateToString(printDevice.state());
            qDebug() << "";
            qDebug() << "supportsMultipleCopies :" << printDevice.supportsMultipleCopies();
            qDebug() << "supportsCollateCopies  :" << printDevice.supportsCollateCopies();
            qDebug() << "";
            qDebug() << "defaultPageSize    :" << printDevice.defaultPageSize();
            qDebug() << "supportedPageSizes :";
            foreach (const QPageSize &page, printDevice.supportedPageSizes())
                qDebug() << "                    " << page << printDevice.printableMargins(page, QPageLayout::Portrait, 300);
            qDebug() << "";
            qDebug() << "supportsCustomPageSizes :" << printDevice.supportsCustomPageSizes();
            qDebug() << "";
            qDebug() << "minimumPhysicalPageSize :" << printDevice.minimumPhysicalPageSize();
            qDebug() << "maximumPhysicalPageSize :" << printDevice.maximumPhysicalPageSize();
            qDebug() << "";
            qDebug() << "defaultResolution    :" << printDevice.defaultResolution();
            qDebug() << "supportedResolutions :" << printDevice.supportedResolutions();
            qDebug() << "";
            qDebug() << "defaultInputSlot    :" << printDevice.defaultInputSlot().key
                                                <<  printDevice.defaultInputSlot().name
                                                <<  printDevice.defaultInputSlot().id;
            qDebug() << "supportedInputSlots :";
            foreach (const QPrint::InputSlot &slot, printDevice.supportedInputSlots())
                qDebug() << "                     " << slot.key << slot.name << slot.id;
            qDebug() << "";
            qDebug() << "defaultOutputBin    :" << printDevice.defaultOutputBin().key
                                                <<  printDevice.defaultOutputBin().name
                                                <<  printDevice.defaultOutputBin().id;
            qDebug() << "supportedOutputBins :";
            foreach (const QPrint::OutputBin &bin, printDevice.supportedOutputBins())
                qDebug() << "                     " << bin.key <<  bin.name <<  bin.id;
            qDebug() << "";
            qDebug() << "defaultDuplexMode    :" << duplexToString(printDevice.defaultDuplexMode());
            qDebug() << "supportedDuplexModes :";
            foreach (QPrint::DuplexMode mode, printDevice.supportedDuplexModes())
                qDebug() << "                      " << duplexToString(mode);
            qDebug() << "";
            qDebug() << "defaultColorMode    :" << colorToString(printDevice.defaultColorMode());
            qDebug() << "supportedColorModes :";
            foreach (QPrint::ColorMode mode, printDevice.supportedColorModes())
                qDebug() << "                     " << colorToString(mode);
            qDebug() << "";
            qDebug() << "supportedMimeTypes :";
            foreach (const QMimeType &type, printDevice.supportedMimeTypes())
                qDebug() << "                    " << type.name();
        } else {
            qDebug() << "Create printer failed" << id;
        }
        qDebug() << "\n";
    }
}
