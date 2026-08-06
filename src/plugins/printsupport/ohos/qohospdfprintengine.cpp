// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosnativeprint.h"
#include "qohospdfprintengine.h"
#include "qohosprintdevice.h"
#include <QtCore/qbytearray.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <cstdint>
#include <qpagesize.h>

QT_BEGIN_NAMESPACE

QTemporaryDir QOhosPdfPrintEngine::s_tempDir;

QOhosPdfPrintEngine::QOhosPdfPrintEngine(QPrinter::PrinterMode mode, const QString &deviceId)
    : QPdfPrintEngine(*new QOhosPdfPrintEnginePrivate(mode))
    , m_deviceId(deviceId)
{
    setProperty(QPrintEngine::PPK_PrinterName, QVariant(deviceId));
    updateUnsupportedPrinterParameters();
}

QOhosPdfPrintEngine::~QOhosPdfPrintEngine() = default;

bool QOhosPdfPrintEngine::begin(QPaintDevice *pdev)
{
    auto __dbg = make_QCScopedDebug("QOhosPdfPrintEngine::begin");

    if (isActive())
        qOhosCritical(QtForOhos) << "QOhosPdfPrintEngine: Cannot print multiple documents at once.";

    Q_D(QPdfPrintEngine);

    d->outputFileName = s_tempDir.filePath(QStringLiteral("doc.pdf"));

    qOhosDebug(QtForOhos) << "QOhosPdfPrintEngine: Printing to pdf file:" << d->outputFileName;

    if (!d->openPrintDevice()) {
        state = QPrinter::Error;
        return false;
    }
    state = QPrinter::Active;

    return QPdfEngine::begin(pdev);
}

bool QOhosPdfPrintEngine::end()
{
    auto __dbg = make_QCScopedDebug("QOhosPdfPrintEngine::end");

    Q_D(QPdfPrintEngine);

    QPdfEngine::end();
    d->closePrintDevice();

    if (!QFile::exists(d->outputFileName)) {
        qOhosCritical(QtForOhos) << "QOhosPdfPrintEngine: output pdf doc" << d->outputFileName << "does not exist. Aborting";
        return false;
    }

    QOhosNativePrint::PrinterInfo printerInfo{};
    if (!QOhosNativePrint::queryPrinterInfo(m_deviceId, printerInfo)) {
        qOhosCritical(QtForOhos) << "QOhosPdfPrintEngine: failed to retrieve printer info";
        return false;
    }

    if (printerInfo.state == ::PRINTER_UNAVAILABLE) {
        qOhosCritical(QtForOhos) << "QOhosPdfPrintEngine: printer is unavailable";
        return false;
    }

    QFile f(d->outputFileName);
    if (!f.open(QIODevice::ReadOnly)) {
        qOhosCritical(QtForOhos) << "QOhosPdfPrintEngine: failed to open generated document";
        return false;
    }
    int fd = f.handle();
    if (fd == -1) {
        qOhosCritical(QtForOhos) << "QOhosPdfPrintEngine: failed to obtain fd of pdf file";
        return false;
    }
    qOhosDebug(QtForOhos) << "QOhosPdfPrintEngine: printing file" << d->outputFileName << "with fd:" << fd;

    std::vector<std::uint32_t> fdList;
    fdList.push_back(static_cast<std::uint32_t>(fd));

    auto printerId = m_deviceId.toUtf8();
    auto supportedPageSize
        = QOhosPrintDevice(m_deviceId).supportedPageSize(d->m_pageLayout.pageSize());
    QByteArray pageSizeId = supportedPageSize.key().toUtf8();
    auto jobName = QPdfPrintEngine::property(PPK_DocumentName).toString().toUtf8();

    Print_PrintJob printJob{};
    printJob.jobName = jobName.data();
    printJob.fdListCount = fdList.size();
    printJob.fdList = fdList.data();
    printJob.printerId = printerId.data();
    printJob.resolution.horizontalDpi = static_cast<std::uint32_t>(d->resolution);
    printJob.resolution.verticalDpi = static_cast<std::uint32_t>(d->resolution);
    printJob.copyNumber = m_copies;
    // FIXME: Use the actual settings, not the default ones.
    printJob.pageSizeId = pageSizeId.data();
    printJob.colorMode = nativeColorMode();
    printJob.duplexMode = nativeDuplexMode();
    printJob.orientationMode = nativeOrientationMode();

    QJsonObject advancedOptions = {
        {"isCollate", QPdfPrintEngine::property(PPK_CollateCopies).toBool()},
        {"isReverse", QPdfPrintEngine::property(PPK_PageOrder).toInt() == QPrinter::LastPageFirst},
    };
    auto advancedOptionsJson = QJsonDocument(advancedOptions).toJson(QJsonDocument::Compact);
    printJob.advancedOptions = advancedOptionsJson.data();

    if (!QOhosNativePrint::startPrintJob(printJob)) {
        qOhosCritical(QtForOhos) << "QOhosPdfPrintEngine: failed to start print job";
        return false;
    }

    state = QPrinter::Idle;

    return true;
}

void QOhosPdfPrintEngine::setProperty(PrintEnginePropertyKey key, const QVariant &value)
{
    const QOhosPrintDevice printDevice(m_deviceId);
    Q_D(QOhosPdfPrintEngine);

    switch (key) {
    case PPK_Duplex: {
        auto requestedDuplexMode = QPrinter::DuplexMode(value.toInt());
        if (printDevice.supportedDuplexModes().contains(static_cast<QPrint::DuplexMode>(requestedDuplexMode)))
            m_duplexMode = requestedDuplexMode;
        break;
    }
    case PPK_CustomPaperSize:
        d->m_pageLayout.setPageSize(printDevice.supportedPageSize(value.toString()));
        break;
    case PPK_CopyCount:
    case PPK_NumberOfCopies:
        if (printDevice.supportsMultipleCopies())
            m_copies = value.toInt();
        break;
    case PPK_ColorMode: {
        auto requestedColorMode = QPrinter::ColorMode(value.toInt());
        if (printDevice.supportedColorModes().contains(static_cast<QPrint::ColorMode>(requestedColorMode)))
            QPdfPrintEngine::setProperty(key, value);
        break;
    }
    default:
        QPdfPrintEngine::setProperty(key, value);
        break;
    }
}

QVariant QOhosPdfPrintEngine::property(PrintEnginePropertyKey key) const
{
    switch (key) {
    case PPK_Duplex:
        return m_duplexMode;
    case PPK_SupportsMultipleCopies:
        return true;
    case PPK_CopyCount:
    case PPK_NumberOfCopies:
        return m_copies;
    default:
        break;
    }
    return QPdfPrintEngine::property(key);
}

Print_DuplexMode QOhosPdfPrintEngine::nativeDuplexMode() const
{
    Q_D(const QOhosPdfPrintEngine);

    switch (m_duplexMode) {
    case QPrinter::DuplexNone:
        return DUPLEX_MODE_ONE_SIDED;
    case QPrinter::DuplexAuto:
        return d->m_pageLayout.orientation() == QPageLayout::Landscape
            ? DUPLEX_MODE_TWO_SIDED_SHORT_EDGE
            : DUPLEX_MODE_TWO_SIDED_LONG_EDGE;
    case QPrinter::DuplexLongSide:
        return DUPLEX_MODE_TWO_SIDED_LONG_EDGE;
    case QPrinter::DuplexShortSide:
        return DUPLEX_MODE_TWO_SIDED_SHORT_EDGE;
    }
}

Print_ColorMode QOhosPdfPrintEngine::nativeColorMode() const
{
    Q_D(const QOhosPdfPrintEngine);

    if (d->colorModel == QPdfEngine::ColorModel::Grayscale)
        return COLOR_MODE_MONOCHROME;
    else
        return COLOR_MODE_COLOR;
}

Print_OrientationMode QOhosPdfPrintEngine::nativeOrientationMode() const
{
    Q_D(const QOhosPdfPrintEngine);

    switch (d->m_pageLayout.orientation()) {
    case QPageLayout::Portrait:
        return ORIENTATION_MODE_PORTRAIT;
    case QPageLayout::Landscape:
        return ORIENTATION_MODE_LANDSCAPE;
    }
}

void QOhosPdfPrintEngine::updateUnsupportedPrinterParameters()
{
    Q_D(QOhosPdfPrintEngine);
    const QOhosPrintDevice currentPrintDevice(m_deviceId);
    auto currentColorMode = static_cast<QPrint::ColorMode>(property(PPK_ColorMode).toInt());
    auto currentDuplexMode = static_cast<QPrint::DuplexMode>(m_duplexMode);
    auto currentPageSize = d->m_pageLayout.pageSize();

    if (!currentPrintDevice.supportedColorModes().contains(currentColorMode))
        setProperty(PPK_ColorMode, QVariant(static_cast<int>(currentPrintDevice.defaultColorMode())));

    if (!currentPrintDevice.supportedDuplexModes().contains(currentDuplexMode))
        m_duplexMode = static_cast<QPrinter::DuplexMode>(currentPrintDevice.defaultDuplexMode());

    d->m_pageLayout.setPageSize(currentPrintDevice.supportedPageSize(currentPageSize));

    if (!currentPrintDevice.supportsMultipleCopies())
        m_copies = 1;
}

QOhosPdfPrintEnginePrivate::QOhosPdfPrintEnginePrivate(QPrinter::PrinterMode m)
    : QPdfPrintEnginePrivate(m)
{
}

QOhosPdfPrintEnginePrivate::~QOhosPdfPrintEnginePrivate() = default;

QT_END_NAMESPACE
