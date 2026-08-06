// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosnativeprint.h"
#include <QtCore/qdebug.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qglobal.h>
#include <cmath>
#include <cstdint>

QT_BEGIN_NAMESPACE

namespace QOhosNativePrint
{

namespace
{

QString stringifyOhosNativePrintReturn(std::int32_t retVal)
{
    QString s = QString("%1 : ").arg(retVal);
    switch (retVal)
    {
    case ::PRINT_ERROR_NONE:
        s += QStringLiteral("Ok");
        break;
    case ::PRINT_ERROR_NO_PERMISSION:
        s += QStringLiteral("No Permission");
        break;
    case ::PRINT_ERROR_INVALID_PARAMETER:
        s += QStringLiteral("Invalid Parameter");
        break;
    case ::PRINT_ERROR_GENERIC_FAILURE:
        s += QStringLiteral("Printing Failure");
        break;
    case ::PRINT_ERROR_RPC_FAILURE:
        s += QStringLiteral("RPC Failure");
        break;
    case ::PRINT_ERROR_SERVER_FAILURE:
        s += QStringLiteral("Print Service Failure");
        break;
    case ::PRINT_ERROR_INVALID_EXTENSION:
        s += QStringLiteral("Invalid Printer Extension");
        break;
    case ::PRINT_ERROR_INVALID_PRINTER:
        s += QStringLiteral("Invalid Printer");
        break;
    case ::PRINT_ERROR_INVALID_PRINT_JOB:
        s += QStringLiteral("Invalid Print Job");
        break;
    case ::PRINT_ERROR_FILE_IO:
        s += QStringLiteral("Incorrect File I/O");
        break;
    default:
        s += QStringLiteral("Unknown");
        break;
    }
    return s;
}

QDebug operator<<(QDebug d, Print_Resolution nativePrintResolution)
{
    const QDebugStateSaver saver(d);
    d.nospace() << "Print_Resolution(" << nativePrintResolution.horizontalDpi << " x "
        << nativePrintResolution.verticalDpi << " dpi)";
    return d;
}

PageSize convertNativeToOhosPageSize(Print_PageSize nativePageSize)
{
    PageSize pageSize;

    pageSize.id = QString::fromUtf8(nativePageSize.id);
    pageSize.name = QString::fromUtf8(nativePageSize.name);
    pageSize.width = nativePageSize.width;
    pageSize.height = nativePageSize.height;

    return pageSize;
}

PrinterCapabilities convertNativeToOhosPrinterCapability(
    Print_PrinterCapability nativeCapabilities)
{
    PrinterCapabilities capabilities{};

    for (quint32 i = 0; i < nativeCapabilities.supportedColorModesCount; i++)
        capabilities.supportedColorModes << nativeCapabilities.supportedColorModes[i];
    for (quint32 i = 0; i < nativeCapabilities.supportedDuplexModesCount; i++)
        capabilities.supportedDuplexModes << nativeCapabilities.supportedDuplexModes[i];
    capabilities.supportedMediaTypes = QString::fromUtf8(nativeCapabilities.supportedMediaTypes);
    for (quint32 i = 0; i < nativeCapabilities.supportedQualitiesCount; i++)
        capabilities.supportedQualities << nativeCapabilities.supportedQualities[i];
    capabilities.supportedPaperSources
            = QString::fromUtf8(nativeCapabilities.supportedPaperSources);
    capabilities.supportedCopies = nativeCapabilities.supportedCopies;
    for (quint32 i = 0; i < nativeCapabilities.supportedResolutionsCount; i++)
        capabilities.supportedResolutions << nativeCapabilities.supportedResolutions[i];
    for (quint32 i = 0; i < nativeCapabilities.supportedOrientationsCount; i++)
        capabilities.supportedOrientations << nativeCapabilities.supportedOrientations[i];
    capabilities.advancedCapabilities = QString::fromUtf8(nativeCapabilities.advancedCapability);
    for (quint32 i = 0; i < nativeCapabilities.supportedPageSizesCount; i++) {
        capabilities.supportedPageSizes
            << convertNativeToOhosPageSize(nativeCapabilities.supportedPageSizes[i]);
    }

    return capabilities;
}

PrinterDefaultValues convertNativeToOhosPrinterDefaultValue(Print_DefaultValue nativeDefaultValues)
{
    PrinterDefaultValues defaultValues{};

    defaultValues.colorMode = nativeDefaultValues.defaultColorMode;
    defaultValues.duplexMode = nativeDefaultValues.defaultDuplexMode;
    defaultValues.mediaType = QString::fromUtf8(nativeDefaultValues.defaultMediaType);
    defaultValues.pageSizeId = QString::fromUtf8(nativeDefaultValues.defaultPageSizeId);
    defaultValues.margin = nativeDefaultValues.defaultMargin;
    defaultValues.paperSource = QString::fromUtf8(nativeDefaultValues.defaultPaperSource);
    defaultValues.printQuality = nativeDefaultValues.defaultPrintQuality;
    defaultValues.copies = nativeDefaultValues.defaultCopies;
    defaultValues.resolution = nativeDefaultValues.defaultResolution;
    defaultValues.orientation = nativeDefaultValues.defaultOrientation;
    defaultValues.otherDefaultValues = QString::fromUtf8(nativeDefaultValues.otherDefaultValues);

    return defaultValues;
}

PrinterInfo convertNativeToOhosPrinterInfo(Print_PrinterInfo nativePrinterInfo)
{
    PrinterInfo printerInfo{};

    printerInfo.state = nativePrinterInfo.printerState;
    printerInfo.capabilities = convertNativeToOhosPrinterCapability(nativePrinterInfo.capability);
    printerInfo.defaultValues
        = convertNativeToOhosPrinterDefaultValue(nativePrinterInfo.defaultValue);
    printerInfo.isDefault = nativePrinterInfo.isDefaultPrinter;
    printerInfo.id = QString::fromUtf8(nativePrinterInfo.printerId);
    printerInfo.name = QString::fromUtf8(nativePrinterInfo.printerName);
    printerInfo.description = QString::fromUtf8(nativePrinterInfo.description);
    printerInfo.location = QString::fromUtf8(nativePrinterInfo.location);
    printerInfo.makeAndModel = QString::fromUtf8(nativePrinterInfo.makeAndModel);
    printerInfo.uri = QString::fromUtf8(nativePrinterInfo.printerUri);
    printerInfo.detailedInfo = QString::fromUtf8(nativePrinterInfo.detailInfo);

    return printerInfo;
}

}

bool connectPrinter(const QString &deviceId)
{
    qOhosDebug(QtForOhos) << "Connecting NativePrint Printer with deviceId =" << deviceId;
    std::int32_t ret = OH_Print_ConnectPrinter(deviceId.toStdString().c_str());
    if (ret != ::PRINT_ERROR_NONE) {
        qOhosCritical(QtForOhos) << "Failed to connect printer with id \"" << deviceId << "\", reason: " << stringifyOhosNativePrintReturn(ret);
        return false;
    }
    return true;
}

bool startPrintJob(const Print_PrintJob &job)
{
    qOhosDebug(QtForOhos) << "Starting NativePrint PrintJob";
    qOhosDebug(QtForOhos) << "NativePrintJob.printerId =" << job.printerId;
    qOhosDebug(QtForOhos) << "NativePrintJob.resolution =" << job.resolution;
    qOhosDebug(QtForOhos) << "NativePrintJob.copies =" << job.copyNumber;
    qOhosDebug(QtForOhos) << "NativePrintJob.paperSource =" << job.paperSource;
    qOhosDebug(QtForOhos) << "NativePrintJob.pageSizeId = " << job.pageSizeId;
    qOhosDebug(QtForOhos) << "NativePrintJob.colorMode =" << job.colorMode;
    qOhosDebug(QtForOhos) << "NativePrintJob.duplexMode =" << job.duplexMode;
    qOhosDebug(QtForOhos) << "NativePrintJob.advancedOptions =" << job.advancedOptions;
    for (int i = 0; i < static_cast<int>(job.fdListCount); ++i)
        qOhosWarning(QtForOhos) << "NativePrintJob.fd[" << i << "] =" << job.fdList[i];

    std::int32_t ret = OH_Print_StartPrintJob(&job);
    if (ret != ::PRINT_ERROR_NONE) {
        qOhosCritical(QtForOhos) << "Failed to start print job, reason:" << stringifyOhosNativePrintReturn(ret);
        return false;
    }
    return true;
}

bool queryPrinterIdList(QStringList &printerIdList)
{
    qOhosDebug(QtForOhos) << "Querying Printer Id List";

    Print_StringList nativePrinterIdList;
    std::int32_t ret = OH_Print_QueryPrinterList(&nativePrinterIdList);
    if (ret != ::PRINT_ERROR_NONE) {
        qOhosCritical(QtForOhos) << "Failed to query printer id list, reason:"
            << stringifyOhosNativePrintReturn(ret);
        return false;
    }

    for (std::uint32_t i = 0; i < nativePrinterIdList.count; i++) {
        if (nativePrinterIdList.list[i])
            printerIdList << QString::fromUtf8(nativePrinterIdList.list[i]);
    }

    OH_Print_ReleasePrinterList(&nativePrinterIdList);

    return true;
}

bool queryPrinterInfo(const QString &printerId, PrinterInfo &printerInfo)
{
    qOhosDebug(QtForOhos) << "Querying Printer Info";

    Print_PrinterInfo *nativePrinterInfo = nullptr;

    // NOTE: OH_Print_QueryPrinterInfo requires a pointer to a pointer to Print_PrinterInfo
    std::int32_t ret
        = OH_Print_QueryPrinterInfo(printerId.toUtf8().constData(), &nativePrinterInfo);
    if (ret != ::PRINT_ERROR_NONE) {
        qOhosCritical(QtForOhos) << "Failed to query printer info, reason:"
            << stringifyOhosNativePrintReturn(ret);
        return false;
    }

    Q_ASSERT(nativePrinterInfo != nullptr);
    printerInfo = convertNativeToOhosPrinterInfo(*nativePrinterInfo);
    OH_Print_ReleasePrinterInfo(nativePrinterInfo);

    return true;
}

bool initializePrintService()
{
    qOhosDebug(QtForOhos) << "Initializing Print Service";
    std::int32_t ret = OH_Print_Init();
    if (ret != ::PRINT_ERROR_NONE) {
        qOhosCritical(QtForOhos) << "Failed to initialize print service, reason:"
            << stringifyOhosNativePrintReturn(ret);
        return false;
    }
    return true;
}

bool releasePrintService()
{
    qOhosDebug(QtForOhos) << "Releasing Print Service";
    std::int32_t ret = OH_Print_Release();
    if (ret != ::PRINT_ERROR_NONE) {
        qOhosCritical(QtForOhos) << "Failed to release print service, reason:"
            << stringifyOhosNativePrintReturn(ret);
        return false;
    }
    return true;
}

bool launchPrinterManager()
{
    qOhosDebug(QtForOhos) << "Launching Printer Manager";
    std::int32_t ret = OH_Print_LaunchPrinterManager();
    if (ret != ::PRINT_ERROR_NONE) {
        qOhosCritical(QtForOhos) << "Failed to launch Printer Manager, reason:"
            << stringifyOhosNativePrintReturn(ret);
        return false;
    }
    return true;
}

int convertPixelsPerThousandDpiToPoints(quint32 pixelsPerThousandDpi)
{
    static constexpr double pixelsPerThousandDpiToMillimeterMultiplier = 2.54 / 0.1 / 1000.0;
    static constexpr double millimeterToPointsMultiplier = 2.83464566929;

    auto mm = std::round(
        static_cast<double>(pixelsPerThousandDpi) * pixelsPerThousandDpiToMillimeterMultiplier);
    auto points = std::round(mm * millimeterToPointsMultiplier);

    return static_cast<int>(points);
}

}

QT_END_NAMESPACE
