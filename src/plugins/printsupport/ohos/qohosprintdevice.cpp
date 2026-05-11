// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosnativeprint.h"
#include "qohosprintdevice.h"
#include <QPageSize>
#include <QtCore/private/qohoslogger_p.h>
#if __has_include(<BasicServicesKit/ohprint.h>)
#include <BasicServicesKit/ohprint.h>
#else
#include <ohprint/print_base.h>
#endif

QT_BEGIN_NAMESPACE

namespace
{

QPrint::DuplexMode convertOhosToQtDuplexMode(Print_DuplexMode nativeDuplexMode)
{
    switch (nativeDuplexMode) {
    case DUPLEX_MODE_ONE_SIDED:
        return QPrint::DuplexNone;
    case DUPLEX_MODE_TWO_SIDED_LONG_EDGE:
        return QPrint::DuplexLongSide;
    case DUPLEX_MODE_TWO_SIDED_SHORT_EDGE:
        return QPrint::DuplexShortSide;
    }
}

QPrint::ColorMode convertOhosToQtColorMode(Print_ColorMode nativeColorMode)
{
    switch (nativeColorMode) {
    case COLOR_MODE_MONOCHROME:
        return QPrint::GrayScale;
    case COLOR_MODE_COLOR:
        return QPrint::Color;
    case COLOR_MODE_AUTO:
        // NOTE: Qt doesn't have a QPrint::ColorMode::Auto option. Use GrayScale for safety.
        return QPrint::GrayScale;
    }
}

}

QOhosPrintDevice::QOhosPrintDevice()
    : QPlatformPrintDevice()
{
}

QOhosPrintDevice::QOhosPrintDevice(const QString &id)
    : QPlatformPrintDevice(id)
{
    QOhosNativePrint::PrinterInfo printerInfo;
    if (QOhosNativePrint::queryPrinterInfo(m_id, printerInfo)) {
        m_name = printerInfo.name;
        m_location = printerInfo.location;
        m_makeAndModel = printerInfo.makeAndModel;
        m_supportsMultipleCopies = printerInfo.capabilities.supportedCopies > 1;
    }
}

QOhosPrintDevice::~QOhosPrintDevice() = default;

bool QOhosPrintDevice::isValid() const
{
    QStringList printerIdList;
    if (!QOhosNativePrint::queryPrinterIdList(printerIdList))
        return false;
    return printerIdList.contains(m_id);
}

bool QOhosPrintDevice::isDefault() const
{
    QOhosNativePrint::PrinterInfo printerInfo;
    if (!QOhosNativePrint::queryPrinterInfo(m_id, printerInfo)) {
        qOhosCritical(QtForOhos) << "QOhosPrintDevice::isDefault: Failed to get printerInfo";
        return false;
    }

    return printerInfo.isDefault;
}

QPrint::DeviceState QOhosPrintDevice::state() const
{
    if (!isValid())
        return QPrint::Error;

    QOhosNativePrint::PrinterInfo pi;
    if (!QOhosNativePrint::queryPrinterInfo(m_id, pi)) {
        qOhosCritical(QtForOhos) << "Failed to get printerInfo";
        return QPrint::Error;
    }

    switch (pi.state) {
    case PRINTER_IDLE:
        return QPrint::Idle;

    case PRINTER_BUSY:
        return QPrint::Active;

    case PRINTER_UNAVAILABLE:
        return QPrint::Error;
    }
}

QPageSize QOhosPrintDevice::defaultPageSize() const
{
    auto __dbg = make_QCScopedDebug("QOhosPrintDevice::defaultPageSize");

    QOhosNativePrint::PrinterInfo printerInfo;
    if (QOhosNativePrint::queryPrinterInfo(m_id, printerInfo)) {
        for (const auto &pageSize : supportedPageSizes()) {
            if (pageSize.key() == printerInfo.defaultValues.pageSizeId)
                return pageSize;
        }
    }

    // NOTE: OHOS provides just a page size ID of the default page size. It does not provide the
    // size, which is required by Qt. Use the A4 format as a backup for default page.  However, we
    // cannot simply return QPageSize(QPageSize::A4) as a backup. The problem is that page size IDs
    // provided by OHOS are different than what Qt uses. For example, the A4 size has ID "ISO_A4" in
    // OHOS, while Qt uses "A4".
    QString isoA4OhosId = QString::fromLocal8Bit("ISO_A4");
    QString isoA4OhosName = QString::fromLocal8Bit("iso_a4_210x297mm");
    return createPageSize(isoA4OhosId, QPageSize(QPageSize::A4).sizePoints(), isoA4OhosName);
}

QPrint::DuplexMode QOhosPrintDevice::defaultDuplexMode() const
{
    auto __dbg = make_QCScopedDebug("QOhosPrintDevice::defaultDuplexMode");

    QOhosNativePrint::PrinterInfo printerInfo;
    if (!QOhosNativePrint::queryPrinterInfo(m_id, printerInfo)) {
        qOhosCritical(QtForOhos) << "Failed to get printerInfo";
        return QPrint::DuplexNone;
    }

    return convertOhosToQtDuplexMode(printerInfo.defaultValues.duplexMode);
}

QPrint::ColorMode QOhosPrintDevice::defaultColorMode() const
{
    auto __dbg = make_QCScopedDebug("QOhosPrintDevice::defaultColorMode");

    QOhosNativePrint::PrinterInfo printerInfo;
    if (!QOhosNativePrint::queryPrinterInfo(m_id, printerInfo)) {
        qOhosCritical(QtForOhos) << "Failed to get printerInfo";
        return QPrint::GrayScale;
    }

    return convertOhosToQtColorMode(printerInfo.defaultValues.colorMode);
}

int QOhosPrintDevice::defaultResolution() const
{
    auto __dbg = make_QCScopedDebug("QOhosPrintDevice::defaultResolution");

    QOhosNativePrint::PrinterInfo printerInfo;
    if (!QOhosNativePrint::queryPrinterInfo(m_id, printerInfo)) {
        qOhosCritical(QtForOhos) << "Failed to get printerInfo";
        return 0;
    }

    return printerInfo.defaultValues.resolution.verticalDpi;
}

QStringList QOhosPrintDevice::availablePrintDeviceIds()
{
    auto __dbg = make_QCScopedDebug("QOhosPrintDevice::availablePrintDeviceIds");

    QStringList printerIdList;
    if (!QOhosNativePrint::queryPrinterIdList(printerIdList))
        return {};

    return printerIdList;
}

QString QOhosPrintDevice::defaultPrintDeviceId()
{
    auto __dbg = make_QCScopedDebug("QOhosPrintDevice::defaultPrintDeviceId");

    for (const auto &printerId : availablePrintDeviceIds()) {
        QOhosNativePrint::PrinterInfo printerInfo;
        if (QOhosNativePrint::queryPrinterInfo(printerId, printerInfo) && printerInfo.isDefault)
            return printerId;
    }

    return QString();
}

void QOhosPrintDevice::loadPageSizes() const
{
    auto __dbg = make_QCScopedDebug("QOhosPrintDevice::loadPageSizes");

    QOhosNativePrint::PrinterInfo printerInfo;
    if (!QOhosNativePrint::queryPrinterInfo(m_id, printerInfo)) {
        qOhosCritical(QtForOhos) << "Failed to get printerInfo";
        return;
    }

    qOhosDebug(QtForOhos) << "Available page sizes:";

    m_pageSizes.clear();
    for (const auto &nativePageSize : printerInfo.capabilities.supportedPageSizes) {
        QSize size(
            QOhosNativePrint::convertPixelsPerThousandDpiToPoints(nativePageSize.width),
            QOhosNativePrint::convertPixelsPerThousandDpiToPoints(nativePageSize.height));
        QPageSize pageSize = createPageSize(nativePageSize.id, size, nativePageSize.name);

        qOhosDebug(QtForOhos) << "\t*" << pageSize;

        m_pageSizes.append(pageSize);
    }

    m_havePageSizes = true;
}

void QOhosPrintDevice::loadDuplexModes() const
{
    auto __dbg = make_QCScopedDebug("QOhosPrintDevice::loadDuplexModes");

    QOhosNativePrint::PrinterInfo printerInfo;
    if (!QOhosNativePrint::queryPrinterInfo(m_id, printerInfo)) {
        qOhosCritical(QtForOhos) << "Failed to get printerInfo";
        return;
    }

    qOhosDebug(QtForOhos) << "Available duplex modes:";

    m_duplexModes.clear();
    for (auto nativeDuplexMode : printerInfo.capabilities.supportedDuplexModes) {
        auto duplexMode = convertOhosToQtDuplexMode(nativeDuplexMode);

        qOhosDebug(QtForOhos) << "\t*" << duplexMode;

        m_duplexModes.append(duplexMode);
    }

    m_haveDuplexModes = true;
}

void QOhosPrintDevice::loadColorModes() const
{
    auto __dbg = make_QCScopedDebug("QOhosPrintDevice::loadColorModes");

    QOhosNativePrint::PrinterInfo printerInfo;
    if (!QOhosNativePrint::queryPrinterInfo(m_id, printerInfo)) {
        qOhosCritical(QtForOhos) << "Failed to get printerInfo";
        return;
    }

    qOhosDebug(QtForOhos) << "Printer colorModes:";

    m_colorModes.clear();
    for (auto nativeColorMode : printerInfo.capabilities.supportedColorModes) {
        auto colorMode = convertOhosToQtColorMode(nativeColorMode);

        qOhosDebug(QtForOhos) << "\t*" << colorMode;

        m_colorModes.append(colorMode);
    }

    m_haveColorModes = true;
}

void QOhosPrintDevice::loadResolutions() const
{
    auto __dbg = make_QCScopedDebug("QOhosPrintDevice::loadResolutions");

    QOhosNativePrint::PrinterInfo printerInfo;
    if (!QOhosNativePrint::queryPrinterInfo(m_id, printerInfo)) {
        qOhosCritical(QtForOhos) << "Failed to get printerInfo";
        return;
    }

    qOhosDebug(QtForOhos) << "Printer resolutions:";

    m_resolutions.clear();
    for (const auto &nativeResolution : printerInfo.capabilities.supportedResolutions) {
        auto resolution = nativeResolution.verticalDpi;

        qOhosDebug(QtForOhos) << "\t*" << resolution;

        m_resolutions.append(resolution);
    }

    m_haveResolutions = true;
}

QT_END_NAMESPACE
