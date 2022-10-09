// Copyright (C) 2022-2023 Gaurav Guleria <tinytrebuchet@protonmail.com>om>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcpdbprintdevice.h"

#include <qpa/qplatformprintplugin.h>

#include "qcpdbprintersupport_p.h"

#if QT_CONFIG(mimetype)
#include <QtCore/QMimeDatabase>
#endif

using namespace Qt::StringLiterals;

QCpdbPrintDevice::QCpdbPrintDevice(cpdb_printer_obj_t * const printerObj)
    : m_printerObj(printerObj)
{
    if (printerObj) {
        m_name = printerObj->name;
        m_id = printerObj->id;
        m_location = printerObj->location;
        m_makeAndModel = printerObj->make_and_model;

        cpdb_option_t *opt = cpdbGetOption(printerObj, CPDB_OPTION_COPIES);
        if (opt && opt->num_supported > 0) {
            QList<QByteArray> copiesRange = QByteArray(opt->supported_values[0]).split('-');
            bool ok;
            int maxCopies = copiesRange.last().toInt(&ok);
            if (ok && maxCopies > 1) {
                m_supportsMultipleCopies = true;
            }
        }

        opt = cpdbGetOption(printerObj, CPDB_OPTION_COLLATE);
        if (opt && opt->num_supported > 1)
            m_supportsCollateCopies = true;
    }
}

QCpdbPrintDevice::~QCpdbPrintDevice() = default;

bool QCpdbPrintDevice::isValid() const
{
    return (m_printerObj != nullptr);
}

bool QCpdbPrintDevice::isDefault() const
{
    if (QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get())
        return (ps->defaultPrintDeviceId() == m_printerObj->id);
    return false;
}

QPrint::DeviceState QCpdbPrintDevice::state() const
{
    const QByteArray state = cpdbGetState(m_printerObj);
    if (state == CPDB_STATE_IDLE)
        return QPrint::Idle;
    if (state == CPDB_STATE_PRINTING)
        return QPrint::Active;
    if (state == CPDB_STATE_STOPPED)
        return QPrint::Aborted;
    return QPrint::Error;
}

void QCpdbPrintDevice::loadPageSizes() const
{
    m_pageSizes.clear();
    m_havePageSizes = true;

    if (const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_MEDIA)) {
        m_pageSizes.reserve(opt->num_supported);
        for (int i = 0; i < opt->num_supported; i++) {
            int width, length;
            if (!cpdbGetMediaSize(m_printerObj, opt->supported_values[i], &width, &length))
                continue;

            const QByteArray value = opt->supported_values[i];
            auto key = QString::fromLocal8Bit(value);
            auto size = QSizeF(width/100.0, length/100.0);
            auto name = QString::fromLocal8Bit(QCPDBSupport::translateChoice(m_printerObj,
                                                                             CPDB_OPTION_MEDIA,
                                                                             value.constData()));
            auto pageSize = createPageSize(key, size, QPageSize::Millimeter, name);
            if (value.startsWith("custom_min"))
                m_minimumPhysicalPageSize = pageSize.sizePoints();
            else if (value.startsWith("custom_max"))
                m_maximumPhysicalPageSize = pageSize.sizePoints();
            else
                m_pageSizes << pageSize;
        }
    }
}

QPageSize QCpdbPrintDevice::defaultPageSize() const
{
    if (const char *defaultVal = cpdbGetDefault(m_printerObj, CPDB_OPTION_MEDIA)) {
        int width, length;
        if (cpdbGetMediaSize(m_printerObj, defaultVal, &width, &length)) {
            auto key = QString::fromLocal8Bit(defaultVal);
            auto size = QSizeF(width/100.0, length/100.0);
            auto name = QString::fromLocal8Bit(QCPDBSupport::translateChoice(m_printerObj,
                                                                             CPDB_OPTION_MEDIA,
                                                                             defaultVal));
            return createPageSize(key, size, QPageSize::Millimeter, name);
        }
    }

    return QPlatformPrintDevice::defaultPageSize();
}

QMarginsF QCpdbPrintDevice::printableMargins(const QPageSize &pageSize,
                                             QPageLayout::Orientation orientation,
                                             int resolution) const
{
    Q_UNUSED(orientation);
    Q_UNUSED(resolution);

    cpdb_margin_t *margins;
    const QByteArray media = pageSize.key().toLocal8Bit();
    int num_margins = cpdbGetMediaMargins(m_printerObj, media.constData(), &margins);
    if (num_margins > 0) {
        qreal left = margins[0].left / 100.0 * QCPDBSupport::pointsMultiplier;
        qreal right = margins[0].right / 100.0 * QCPDBSupport::pointsMultiplier;
        qreal top = margins[0].top / 100.0 * QCPDBSupport::pointsMultiplier;
        qreal bottom = margins[0].bottom / 100.0 * QCPDBSupport::pointsMultiplier;
        return QMarginsF(left, top, right, bottom);
    }

    return QMarginsF();
}

void QCpdbPrintDevice::loadResolutions() const
{
    m_resolutions.clear();
    m_haveResolutions = true;

    const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_RESOLUTION);
    if (opt && opt->num_supported > 1) {
        m_resolutions.reserve(opt->num_supported);
        for (int i = 0; i < opt->num_supported; i++) {
            QByteArray resolution = opt->supported_values[i];
            // example: 300dpi
            if (resolution.size() > 3)
                resolution.chop(3);
            bool ok;
            int value = resolution.toInt(&ok);
            if (ok)
                m_resolutions << value;
        }
    }
}

int QCpdbPrintDevice::defaultResolution() const
{
    QByteArray defaultVal = cpdbGetDefault(m_printerObj, CPDB_OPTION_RESOLUTION);
    if (defaultVal.size() > 3)
        defaultVal.chop(3);
    bool ok;
    int value = defaultVal.toInt(&ok);
    if (ok)
        return value;

    return QPlatformPrintDevice::defaultResolution();
}

void QCpdbPrintDevice::loadDuplexModes() const
{
    m_duplexModes.clear();
    m_haveDuplexModes = true;

    const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_SIDES);
    if (opt && opt->num_supported > 1) {
        m_duplexModes.reserve(opt->num_supported);
        for (int i = 0; i < opt->num_supported; i++) {
            const QByteArray value = opt->supported_values[i];
            if (value == CPDB_SIDES_ONE_SIDED)
                m_duplexModes << QPrint::DuplexNone;
            else if (value == CPDB_SIDES_TWO_SIDED_SHORT)
                m_duplexModes << QPrint::DuplexShortSide;
            else if (value == CPDB_SIDES_TWO_SIDED_LONG)
                m_duplexModes << QPrint::DuplexLongSide;
        }
    }
}

QPrint::DuplexMode QCpdbPrintDevice::defaultDuplexMode() const
{
    const QByteArray defaultVal = cpdbGetDefault(m_printerObj, CPDB_OPTION_SIDES);
    if (defaultVal == CPDB_SIDES_ONE_SIDED)
        return QPrint::DuplexNone;
    if (defaultVal == CPDB_SIDES_TWO_SIDED_SHORT)
        return QPrint::DuplexShortSide;
    if (defaultVal == CPDB_SIDES_TWO_SIDED_LONG)
        return QPrint::DuplexLongSide;

    return QPlatformPrintDevice::defaultDuplexMode();
}

void QCpdbPrintDevice::loadColorModes() const
{
    m_colorModes.clear();
    m_haveColorModes = true;

    const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_COLOR_MODE);
    if (opt && opt->num_supported > 1) {
        m_colorModes.reserve(opt->num_supported);
        for (int i = 0; i < opt->num_supported; i++) {
            const QByteArray value = opt->supported_values[i];
            if (value == CPDB_COLOR_MODE_BW)
                m_colorModes << QPrint::GrayScale;
            else if (value == CPDB_COLOR_MODE_COLOR)
                m_colorModes << QPrint::Color;
        }
    }
}

QPrint::ColorMode QCpdbPrintDevice::defaultColorMode() const
{
    const QByteArray defaultVal = cpdbGetDefault(m_printerObj, CPDB_OPTION_COLOR_MODE);
    if (defaultVal == CPDB_COLOR_MODE_COLOR)
        return QPrint::Color;
    if (defaultVal == CPDB_COLOR_MODE_BW)
        return QPrint::GrayScale;

    return QPlatformPrintDevice::defaultColorMode();
}

void QCpdbPrintDevice::loadInputSlots() const
{
    m_inputSlots.clear();
    m_haveInputSlots = true;

    const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_MEDIA_SOURCE);
    if (opt && opt->num_supported > 1) {
        m_inputSlots.reserve(opt->num_supported);
        for (int i = 0; i < opt->num_supported; i++) {
            QPrint::InputSlot inputSlot;
            inputSlot.key = opt->supported_values[i];
            inputSlot.name = QCPDBSupport::translateChoice(m_printerObj, CPDB_OPTION_MEDIA_SOURCE, opt->supported_values[i]);
            inputSlot.id = QPrint::CustomInputSlot;
            inputSlot.windowsId = DMBIN_USER;
            m_inputSlots << inputSlot;
        }
    }
}

QPrint::InputSlot QCpdbPrintDevice::defaultInputSlot() const
{
    const QByteArray defaultVal = cpdbGetDefault(m_printerObj, CPDB_OPTION_MEDIA_SOURCE);
    if (!defaultVal.isNull()) {
        QPrint::InputSlot inputSlot;
        inputSlot.key = defaultVal;
        inputSlot.name = QCPDBSupport::translateChoice(m_printerObj, CPDB_OPTION_MEDIA_SOURCE, defaultVal.constData());
        inputSlot.id = QPrint::CustomInputSlot;
        inputSlot.windowsId = DMBIN_USER;
        return inputSlot;
    }

    return QPlatformPrintDevice::defaultInputSlot();
}

void QCpdbPrintDevice::loadOutputBins() const
{
    m_outputBins.clear();
    m_haveOutputBins = true;

    const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_OUTPUT_BIN);
    if (opt && opt->num_supported > 1) {
        m_outputBins.reserve(opt->num_supported);
        for (int i = 0; i < opt->num_supported; i++) {
            QPrint::OutputBin outputBin;
            outputBin.key = opt->supported_values[i];
            outputBin.name = QCPDBSupport::translateChoice(m_printerObj, CPDB_OPTION_OUTPUT_BIN, opt->supported_values[i]);
            outputBin.id = QPrint::CustomOutputBin;
            m_outputBins << outputBin;
        }
    }
}

QPrint::OutputBin QCpdbPrintDevice::defaultOutputBin() const
{
    QByteArray defaultVal = cpdbGetDefault(m_printerObj, CPDB_OPTION_OUTPUT_BIN);
    if (!defaultVal.isNull()) {
        QPrint::OutputBin outputBin;
        outputBin.key = defaultVal;
        outputBin.name = QCPDBSupport::translateChoice(m_printerObj, CPDB_OPTION_OUTPUT_BIN, defaultVal.constData());
        outputBin.id = QPrint::CustomOutputBin;
        return outputBin;
    }

    return QPlatformPrintDevice::defaultOutputBin();
}

void QCpdbPrintDevice::loadMimeTypes() const
{
    QMimeDatabase db;
    m_mimeTypes.append(db.mimeTypeForName(u"application/pdf"_s));
    m_mimeTypes.append(db.mimeTypeForName(u"application/postscript"_s));
    m_mimeTypes.append(db.mimeTypeForName(u"image/gif"_s));
    m_mimeTypes.append(db.mimeTypeForName(u"image/png"_s));
    m_mimeTypes.append(db.mimeTypeForName(u"image/jpeg"_s));
    m_mimeTypes.append(db.mimeTypeForName(u"image/tiff"_s));
    m_mimeTypes.append(db.mimeTypeForName(u"text/html"_s));
    m_mimeTypes.append(db.mimeTypeForName(u"text/plain"_s));
    m_haveMimeTypes = true;
}

bool QCpdbPrintDevice::isFeatureAvailable(QPrintDevice::PrintDevicePropertyKey key, const QVariant &params) const
{
    Q_UNUSED(key);
    Q_UNUSED(params);

    return true;
}

QVariant QCpdbPrintDevice::property(QPrintDevice::PrintDevicePropertyKey key) const
{
    if (key == PDPK_CpdbPrinterObj)
        return QVariant::fromValue<cpdb_printer_obj_t *>(m_printerObj);

    return QPlatformPrintDevice::property(key);
}

bool QCpdbPrintDevice::setProperty(QPrintDevice::PrintDevicePropertyKey key, const QVariant &value)
{
    return QPlatformPrintDevice::setProperty(key, value);
}
