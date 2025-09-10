// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcupsprintengine_p.h"

#include <qpa/qplatformprintplugin.h>
#include <qpa/qplatformprintersupport.h>

#include <qiodevice.h>
#include <qfile.h>
#include <qdebug.h>
#include <qbuffer.h>
#include "private/qcups_p.h" // Only needed for PPK_CupsOptions
#include <QtGui/qpagelayout.h>

#include <cups/cups.h>

#include "private/qcore_unix_p.h" // overrides QT_OPEN

QT_BEGIN_NAMESPACE

extern QMarginsF qt_convertMargins(const QMarginsF &margins, QPageLayout::Unit fromUnits, QPageLayout::Unit toUnits);

QCupsPrintEngine::QCupsPrintEngine(QPrinter::PrinterMode m, const QString &deviceId)
    : QPdfPrintEngine(*new QCupsPrintEnginePrivate(m))
{
    Q_D(QCupsPrintEngine);
    d->changePrinter(deviceId);
    state = QPrinter::Idle;
}

QCupsPrintEngine::~QCupsPrintEngine()
{
}

void QCupsPrintEngine::setProperty(PrintEnginePropertyKey key, const QVariant &value)
{
    Q_D(QCupsPrintEngine);

    switch (int(key)) {
    case PPK_PageSize:
        d->setPageSize(QPageSize(QPageSize::PageSizeId(value.toInt())));
        break;
    case PPK_WindowsPageSize:
        d->setPageSize(QPageSize(QPageSize::id(value.toInt())));
        break;
    case PPK_CustomPaperSize:
        d->setPageSize(QPageSize(value.toSizeF(), QPageSize::Point));
        break;
    case PPK_PaperName:
        // Get the named page size from the printer if supported
        d->setPageSize(d->m_printDevice.supportedPageSize(value.toString()));
        break;
    case PPK_Duplex: {
        QPrint::DuplexMode mode = QPrint::DuplexMode(value.toInt());
        if (d->m_printDevice.supportedDuplexModes().contains(mode)) {
            d->duplex = mode;
            d->duplexRequestedExplicitly = true;
        }
        break;
    }
    case PPK_PrinterName:
        d->changePrinter(value.toString());
        break;
    case PPK_CupsOptions:
        d->cupsOptions = value.toStringList();
        if (d->cupsOptions.size() % 2 == 1) {
            qWarning("%s: malformed value for key = PPK_CupsOptions "
                     "(odd number of elements in the string-list; "
                     "appending an empty entry)", Q_FUNC_INFO);
            d->cupsOptions.emplace_back();
        }
        break;
    case PPK_QPageSize:
        d->setPageSize(qvariant_cast<QPageSize>(value));
        break;
    case PPK_QPageLayout: {
        QPageLayout pageLayout = qvariant_cast<QPageLayout>(value);
        if (pageLayout.isValid() && (d->m_printDevice.isValidPageLayout(pageLayout, d->resolution)
                                     || d->m_printDevice.supportsCustomPageSizes()
                                     || d->m_printDevice.supportedPageSizes().isEmpty())) {
            // supportedPageSizes().isEmpty() because QPageSetupWidget::initPageSizes says
            // "If no available printer page sizes, populate with all page sizes"
            d->m_pageLayout = pageLayout;
            d->setPageSize(pageLayout.pageSize());
        }
        break;
    }
    default:
        QPdfPrintEngine::setProperty(key, value);
        break;
    }
}

QVariant QCupsPrintEngine::property(PrintEnginePropertyKey key) const
{
    Q_D(const QCupsPrintEngine);

    QVariant ret;
    switch (int(key)) {
    case PPK_SupportsMultipleCopies:
        // CUPS server always supports multiple copies, even if individual m_printDevice doesn't
        ret = true;
        break;
    case PPK_NumberOfCopies:
        ret = 1;
        break;
    case PPK_CupsOptions:
        ret = d->cupsOptions;
        break;
    case PPK_Duplex:
        ret = d->duplex;
        break;
    default:
        ret = QPdfPrintEngine::property(key);
        break;
    }
    return ret;
}


QCupsPrintEnginePrivate::QCupsPrintEnginePrivate(QPrinter::PrinterMode m)
    : QPdfPrintEnginePrivate(m)
    , duplex(QPrint::DuplexNone)
{
}

QCupsPrintEnginePrivate::~QCupsPrintEnginePrivate()
{
}

bool QCupsPrintEnginePrivate::openPrintDevice()
{
    if (outDevice)
        return false;

    if (!outputFileName.isEmpty()) {
        QFile *file = new QFile(outputFileName);
        if (! file->open(QFile::WriteOnly|QFile::Truncate)) {
            delete file;
            return false;
        }
        outDevice = file;
    } else {
        char filename[512];
        fd = cupsTempFd(filename, 512);
        if (fd < 0) {
            qWarning("QPdfPrinter: Could not open temporary file to print");
            return false;
        }
        cupsTempFile = QString::fromLocal8Bit(filename);
        outDevice = new QFile();
        if (!static_cast<QFile *>(outDevice)->open(fd, QIODevice::WriteOnly)) {
            qWarning("QPdfPrinter: Could not open CUPS temporary file descriptor: %s",
                     qPrintable(outDevice->errorString()));
            delete outDevice;
            outDevice = nullptr;

#if defined(Q_OS_WIN) && defined(Q_CC_MSVC)
            ::_close(fd);
#else
            ::close(fd);
#endif
            fd = -1;
            return false;
        }
    }

    return true;
}

void QCupsPrintEnginePrivate::closePrintDevice()
{
    QPdfPrintEnginePrivate::closePrintDevice();

    if (!cupsTempFile.isEmpty()) {
        QString tempFile = cupsTempFile;
        cupsTempFile.clear();

        // Should never have got here without a printer, but check anyway
        if (printerName.isEmpty()) {
            qWarning("Could not determine printer to print to");
            QFile::remove(tempFile);
            return;
        }

        // Set up print options.
        QList<std::pair<QByteArray, QByteArray> > options;
        QList<cups_option_t> cupsOptStruct;

        options.emplace_back("media", m_pageLayout.pageSize().key().toLocal8Bit());

        if (copies > 1)
            options.emplace_back("copies", QString::number(copies).toLocal8Bit());

        if (copies > 1 && collate)
            options.emplace_back("Collate", "True");

        switch (duplex) {
        case QPrint::DuplexNone:
            options.emplace_back("sides", "one-sided");
            break;
        case QPrint::DuplexAuto:
            if (m_pageLayout.orientation() == QPageLayout::Portrait)
                options.emplace_back("sides", "two-sided-long-edge");
            else
                options.emplace_back("sides", "two-sided-short-edge");
            break;
        case QPrint::DuplexLongSide:
            options.emplace_back("sides", "two-sided-long-edge");
            break;
        case QPrint::DuplexShortSide:
            options.emplace_back("sides", "two-sided-short-edge");
            break;
        }

        if (m_pageLayout.orientation() == QPageLayout::Landscape)
            options.emplace_back("landscape", "true");

        QStringList::const_iterator it = cupsOptions.constBegin();
        Q_ASSERT(cupsOptions.size() % 2 == 0);
        while (it != cupsOptions.constEnd()) {
            options.emplace_back((*it).toLocal8Bit(), (*(it+1)).toLocal8Bit());
            it += 2;
        }

        const int numOptions = options.size();
        cupsOptStruct.reserve(numOptions);
        for (int c = 0; c < numOptions; ++c) {
            cups_option_t opt;
            opt.name = options[c].first.data();
            opt.value = options[c].second.data();
            cupsOptStruct.append(opt);
        }

        // Print the file
        // Cups expect the printer original name without instance, the full name is used only to retrieve the configuration
        const auto parts = QStringView{printerName}.split(u'/');
        const auto printerOriginalName = parts.at(0);
        cups_option_t* optPtr = cupsOptStruct.size() ? &cupsOptStruct.first() : 0;
        cupsPrintFile(printerOriginalName.toLocal8Bit().constData(), tempFile.toLocal8Bit().constData(),
                      title.toLocal8Bit().constData(), cupsOptStruct.size(), optPtr);

        QFile::remove(tempFile);
    }
}

void QCupsPrintEnginePrivate::changePrinter(const QString &newPrinter)
{
    // Don't waste time if same printer name
    if (newPrinter == printerName)
        return;

    // Should never have reached here if no plugin available, but check just in case
    QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
    if (!ps)
        return;

    // Try create the printer, only use it if it returns valid
    QPrintDevice printDevice = ps->createPrintDevice(newPrinter);
    if (!printDevice.isValid())
        return;
    m_printDevice.swap(printDevice);
    printerName = m_printDevice.id();

    // in case a duplex value was explicitly set, check if new printer supports current value,
    // otherwise use device default
    if (!duplexRequestedExplicitly || !m_printDevice.supportedDuplexModes().contains(duplex)) {
        duplex = m_printDevice.defaultDuplexMode();
        duplexRequestedExplicitly = false;
    }
    QPrint::ColorMode colorMode = static_cast<QPrint::ColorMode>(printerColorMode());
    if (!m_printDevice.supportedColorModes().contains(colorMode)) {
        colorModel = (m_printDevice.defaultColorMode() == QPrint::GrayScale)
                         ? QPdfEngine::ColorModel::Grayscale
                         : QPdfEngine::ColorModel::RGB;
    }

    // Get the equivalent page size for this printer as supported names may be different
    if (m_printDevice.supportedPageSize(m_pageLayout.pageSize()).isValid())
        setPageSize(m_pageLayout.pageSize());
    else
        setPageSize(QPageSize(m_pageLayout.pageSize().size(QPageSize::Point), QPageSize::Point));
}

void QCupsPrintEnginePrivate::setPageSize(const QPageSize &pageSize)
{
    if (pageSize.isValid()) {
        // Find if the requested page size has a matching printer page size, if so use its defined name instead
        QPageSize printerPageSize = m_printDevice.supportedPageSize(pageSize);
        QPageSize usePageSize = printerPageSize.isValid() ? printerPageSize : pageSize;
        QMarginsF printable = m_printDevice.printableMargins(usePageSize, m_pageLayout.orientation(), resolution);
        m_pageLayout.setPageSize(usePageSize, qt_convertMargins(printable, QPageLayout::Point, m_pageLayout.units()));
    }
}

QT_END_NAMESPACE
