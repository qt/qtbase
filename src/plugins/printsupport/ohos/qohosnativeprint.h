// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSNATIVEPRINT_H_
#define QOHOSNATIVEPRINT_H_

#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>
#if __has_include(<BasicServicesKit/ohprint.h>)
#include <BasicServicesKit/ohprint.h>
#else
#include <ohprint/print_base.h>
#include <ohprint/print_capi.h>
#endif

QT_BEGIN_NAMESPACE

namespace QOhosNativePrint
{

struct PageSize
{
    QString id;
    QString name;
    quint32 width;
    quint32 height;
};

struct PrinterCapabilities
{
    QVector<Print_ColorMode> supportedColorModes;
    QVector<Print_DuplexMode> supportedDuplexModes;
    QVector<PageSize> supportedPageSizes;
    // FIXME: supportedMediaTypes is a list inside a string. It should be parsed. However, currently
    // we are not using information about supported media types, so just treat it as a string
    QString supportedMediaTypes;
    QVector<Print_Quality> supportedQualities;
    // FIXME: supportedPaperSources is a list inside a string. It should be parsed. However,
    // currently we are not using information about supported paper sources, so just treat it as a
    // string
    QString supportedPaperSources;
    quint32 supportedCopies;
    QVector<Print_Resolution> supportedResolutions;
    QVector<Print_OrientationMode> supportedOrientations;
    QString advancedCapabilities;
};

struct PrinterDefaultValues
{
    Print_ColorMode colorMode;
    Print_DuplexMode duplexMode;
    QString mediaType;
    QString pageSizeId;
    Print_Margin margin;
    QString paperSource;
    Print_Quality printQuality;
    quint32 copies;
    Print_Resolution resolution;
    Print_OrientationMode orientation;
    QString otherDefaultValues;
};

struct PrinterInfo
{
    Print_PrinterState state;
    PrinterCapabilities capabilities;
    PrinterDefaultValues defaultValues;
    bool isDefault;
    QString id;
    QString name;
    QString description;
    QString location;
    QString makeAndModel;
    QString uri;
    QString detailedInfo;
};

bool connectPrinter(const QString &deviceId);

bool startPrintJob(const Print_PrintJob &job);

bool queryPrinterIdList(QStringList &printerIdList);

bool queryPrinterInfo(const QString &printerId, PrinterInfo &printerInfo);

bool initializePrintService();

bool releasePrintService();

bool launchPrinterManager();

int convertPixelsPerThousandDpiToPoints(quint32 pixelsPerThousandDpi);

}

QT_END_NAMESPACE

#endif // QOHOSNATIVEPRINT_H_
