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
            if (QCPDBSupport::duplexMap.contains(value))
                m_duplexModes << QCPDBSupport::duplexMap[value];
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
    if (key == QPrintDevice::PDPK_Duplex) {
        if (params.toByteArray() == "conflict")
            return false;
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_SIDES);
        return (opt && opt->num_supported > 1);
    }

    if (key == QPrintDevice::PDPK_PageSet) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_PAGE_SET);
        return (opt && opt->num_supported > 1);
    }

    if (key == QPrintDevice::PDPK_PageRange) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_PAGE_RANGES);
        return (opt);
    }

    if (key == QPrintDevice::PDPK_JobHold) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_JOB_HOLD_UNTIL);
        if (params.toByteArray() == "#specific#")
            return true;
        return (opt && opt->num_supported > 1);
    }

    if (key == QPrintDevice::PDPK_JobBillingInfo) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_BILLING_INFO);
        return (opt != nullptr);
    }

    if (key == QPrintDevice::PDPK_JobPriority) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_JOB_PRIORITY);
        return (opt != nullptr);
    }

    if (key == QPrintDevice::PDPK_JobStartCoverPage) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_JOB_SHEETS);
        return (opt && opt->num_supported > 1);
    }

    if (key == QPrintDevice::PDPK_JobEndCoverPage) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_JOB_SHEETS);
        return (opt && opt->num_supported > 1);
    }

    if (key == QPrintDevice::PDPK_AdvancedOptions) {
        const cpdb_options_t *opts = cpdbGetAllOptions(m_printerObj);
        return (opts != nullptr);
    }

    if (key == QPrintDevice::PDPK_OptionConflict) {
        return false;
    }

    if (key == QPrintDevice::PDPK_NumberUp) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_NUMBER_UP);
        return (opt && opt->num_supported > 1);
    }

    if (key == QPrintDevice::PDPK_NumberUpLayout) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_NUMBER_UP_LAYOUT);
        return (opt && opt->num_supported > 1);
    }

    if (key == QPrintDevice::PDPK_PageSize) {
        if (params.toByteArray() == "conflict")
            return false;
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_MEDIA);
        return (opt && opt->num_supported > 1);
    }

    if (key == QPrintDevice::PDPK_AdvancedColorMode) {
        return false;
    }

    return QPlatformPrintDevice::isFeatureAvailable(key, params);
}

QVariant QCpdbPrintDevice::property(QPrintDevice::PrintDevicePropertyKey key) const
{
    if (key == PDPK_CpdbPrinterObj) {
        return QVariant::fromValue(m_printerObj);
    }

    if (key == QPrintDevice::PDPK_PageSet) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_PAGE_SET);
        if (opt && opt->num_supported > 1) {
            QPrint::OptionCombo option;
            option.name = opt->option_name;
            option.displayName = QString(QCPDBSupport::translateOption(m_printerObj, opt->option_name));
            option.choices.reserve(opt->num_supported);
            option.displayChoices.reserve(opt->num_supported);
            for (int i = 0; i < opt->num_supported; i++) {
                option.choices.push_back(opt->supported_values[i]);
                option.displayChoices.emplace_back(
                            QCPDBSupport::translateChoice(m_printerObj, opt->option_name, opt->supported_values[i]));
            }
            option.defaultChoice = qMax(0, option.choices.indexOf(opt->default_value));
            return QVariant::fromValue(option);
        }
    }

    if (key == QPrintDevice::PDPK_JobHold) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_JOB_HOLD_UNTIL);
        if (opt && opt->num_supported > 1) {
            QPrint::OptionCombo option;
            option.name = opt->option_name;
            option.displayName = QString(QCPDBSupport::translateOption(m_printerObj, opt->option_name));
            option.choices.reserve(opt->num_supported);
            option.displayChoices.reserve(opt->num_supported);
            for (int i = 0; i < opt->num_supported; i++) {
                option.choices.push_back(opt->supported_values[i]);
                option.displayChoices.emplace_back(
                            QCPDBSupport::translateChoice(m_printerObj, opt->option_name, opt->supported_values[i]));
            }
            option.defaultChoice = qMax(0, option.choices.indexOf(opt->default_value));
            return QVariant::fromValue(option);
        }
    }

    if (key == QPrintDevice::PDPK_JobBillingInfo) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_BILLING_INFO);
        if (opt) {
            auto defaultVal = QString::fromUtf8(opt->default_value);
            return QVariant(defaultVal);
        }
    }

    if (key == QPrintDevice::PDPK_JobPriority) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_JOB_PRIORITY);
        bool ok;
        int defaultVal = QByteArray(opt->default_value).toInt(&ok);
        if (ok) {
            return QVariant(defaultVal);
        }
    }

    if (key == QPrintDevice::PDPK_JobStartCoverPage) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_JOB_SHEETS);
        if (opt && opt->num_supported > 1) {
            QPrint::OptionCombo option;
            option.name = opt->option_name;
            option.displayName = QString(QCPDBSupport::translateOption(m_printerObj, opt->option_name));
            option.choices.reserve(opt->num_supported);
            option.displayChoices.reserve(opt->num_supported);
            for (int i = 0; i < opt->num_supported; i++) {
                option.choices.push_back(opt->supported_values[i]);
                option.displayChoices.emplace_back(
                            QCPDBSupport::translateChoice(m_printerObj, opt->option_name, opt->supported_values[i]));
            }
            QByteArray defaultValues = opt->default_value;
            option.defaultChoice = qMax(0, option.choices.indexOf(defaultValues.split(',')[0]));
            return QVariant::fromValue(option);
        }
    }

    if (key == QPrintDevice::PDPK_JobEndCoverPage) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_JOB_SHEETS);
        if (opt && opt->num_supported > 1) {
            QPrint::OptionCombo option;
            option.name = opt->option_name;
            option.displayName = QString(QCPDBSupport::translateOption(m_printerObj, opt->option_name));
            option.choices.reserve(opt->num_supported);
            option.displayChoices.reserve(opt->num_supported);
            for (int i = 0; i < opt->num_supported; i++) {
                option.choices.push_back(opt->supported_values[i]);
                option.displayChoices.emplace_back(
                            QCPDBSupport::translateChoice(m_printerObj, opt->option_name, opt->supported_values[i]));
            }
            QByteArray defaultValues = opt->default_value;
            option.defaultChoice = qMax(0, option.choices.indexOf(defaultValues.split(',')[1]));
            return QVariant::fromValue(option);
        }
    }

    if (key == QPrintDevice::PDPK_AdvancedOptions) {
        const cpdb_options_t *opts = cpdbGetAllOptions(m_printerObj);
        if (opts) {
            GHashTableIter iter;
            gpointer key, value;
            QHash<QByteArray,QPrint::OptionCombosGroup> optionsGroups;
            g_hash_table_iter_init(&iter, opts->table);
            while (g_hash_table_iter_next(&iter, &key, &value)) {
                QByteArray optionName = static_cast<char *>(key);
                const cpdb_option_t *opt = static_cast<cpdb_option_t *>(value);

                if (!m_nonAdvancedOptions.contains(optionName) && opt && opt->num_supported > 1) {
                    QByteArray groupName = opt->group_name;
                    optionsGroups[groupName].groupName = groupName;
                    optionsGroups[groupName].displayGroup = QString(QCPDBSupport::translateGroup(m_printerObj, opt->group_name));

                    QPrint::OptionCombo option;
                    option.name = optionName;
                    option.displayName = QString(QCPDBSupport::translateOption(m_printerObj, opt->option_name));
                    option.choices.reserve(opt->num_supported);
                    option.displayChoices.reserve(opt->num_supported);
                    for (int i = 0; i < opt->num_supported; i++) {
                        option.choices.push_back(opt->supported_values[i]);
                        option.displayChoices.emplace_back(
                                    QCPDBSupport::translateChoice(m_printerObj, opt->option_name, opt->supported_values[i]));
                    }
                    option.defaultChoice = qMax(0, option.choices.indexOf(opt->default_value));
                    optionsGroups[groupName].options.push_back(option);
                }
            }
            return QVariant::fromValue(optionsGroups.values());
        }
    }

    if (key == QPrintDevice::PDPK_OptionConflict) {
        return QVariant::fromValue(nullptr);
    }

    if (key == QPrintDevice::PDPK_NumberUp) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_NUMBER_UP);
        if (opt && opt->num_supported > 1) {
            QPrint::OptionCombo option;
            option.name = opt->option_name;
            option.displayName = QString(QCPDBSupport::translateOption(m_printerObj, opt->option_name));
            option.choices.reserve(opt->num_supported);
            option.displayChoices.reserve(opt->num_supported);
            for (int i = 0; i < opt->num_supported; i++) {
                option.choices.push_back(opt->supported_values[i]);
                if (QCPDBSupport::numUpDist.contains(option.choices[i]))
                    option.choices[i] = QCPDBSupport::numUpDist.value(option.choices[i]);
                option.displayChoices.emplace_back(
                            QCPDBSupport::translateChoice(m_printerObj, opt->option_name, opt->supported_values[i]));
            }
            option.defaultChoice = qMax(0, option.choices.indexOf(opt->default_value));
            return QVariant::fromValue(option);
        }
    }
    if (key == QPrintDevice::PDPK_NumberUpLayout) {
        const cpdb_option_t *opt = cpdbGetOption(m_printerObj, CPDB_OPTION_NUMBER_UP_LAYOUT);
        if (opt && opt->num_supported > 1) {
            QPrint::OptionCombo option;
            option.name = opt->option_name;
            option.displayName = QString(QCPDBSupport::translateOption(m_printerObj, opt->option_name));
            option.choices.reserve(opt->num_supported);
            option.displayChoices.reserve(opt->num_supported);
            for (int i = 0; i < opt->num_supported; i++) {
                option.choices.push_back(opt->supported_values[i]);
                option.displayChoices.emplace_back(
                            QCPDBSupport::translateChoice(m_printerObj, opt->option_name, opt->supported_values[i]));
            }
            option.defaultChoice = qMax(0, option.choices.indexOf(opt->default_value));
            return QVariant::fromValue(option);
        }
    }

    return QPlatformPrintDevice::property(key);
}

bool QCpdbPrintDevice::setProperty(QPrintDevice::PrintDevicePropertyKey key, const QVariant &value)
{
    if (key == QPrintDevice::PDPK_Duplex) {
        auto choice = qvariant_cast<QPrint::DuplexMode>(value);
        if (QCPDBSupport::qDuplexMap.contains(choice)) {
            cpdbAddSettingToPrinter(m_printerObj, CPDB_OPTION_SIDES, QCPDBSupport::qDuplexMap.value(choice));
            return true;
        }
        return false;
    }

    if (key == QPrintDevice::PDPK_PageSet) {
        auto choice = qvariant_cast<QByteArray>(value);
        cpdbAddSettingToPrinter(m_printerObj, CPDB_OPTION_PAGE_SET, choice);
        return true;
    }

    if (key == QPrintDevice::PDPK_PageRange) {
        auto choice = qvariant_cast<QString>(value);
        cpdbAddSettingToPrinter(m_printerObj, CPDB_OPTION_PAGE_RANGES, choice.toUtf8());
        return true;
    }

    if (key == QPrintDevice::PDPK_JobHold) {
        auto choice = qvariant_cast<QByteArray>(value);
        cpdbAddSettingToPrinter(m_printerObj, CPDB_OPTION_JOB_HOLD_UNTIL, choice);
        return true;
    }

    if (key == QPrintDevice::PDPK_JobBillingInfo) {
        auto choice = qvariant_cast<QString>(value);
        cpdbAddSettingToPrinter(m_printerObj, CPDB_OPTION_BILLING_INFO, choice.toUtf8());
        return true;
    }

    if (key == QPrintDevice::PDPK_JobPriority) {
        int priority = qvariant_cast<int>(value);
        cpdbAddSettingToPrinter(m_printerObj, CPDB_OPTION_JOB_PRIORITY, QByteArray::number(priority));
        return true;
    }

    if (key == QPrintDevice::PDPK_JobStartCoverPage) {
        auto choice = qvariant_cast<QByteArray>(value);
        QByteArray currentSetting = cpdbGetCurrent(m_printerObj, CPDB_OPTION_JOB_SHEETS);
        if (!currentSetting.contains(',')) // JobSheets option setting must be in <StartCoverPage,EndCoverPage> format
            return false;
        QByteArray newSetting = choice + "," + currentSetting.split(',')[1];
        cpdbAddSettingToPrinter(m_printerObj, CPDB_OPTION_JOB_SHEETS, newSetting);
        return true;
    }

    if (key == QPrintDevice::PDPK_JobEndCoverPage) {
        auto choice = qvariant_cast<QByteArray>(value);
        QByteArray currentSetting = cpdbGetCurrent(m_printerObj, CPDB_OPTION_JOB_SHEETS);
        if (!currentSetting.contains(',')) // JobSheets option setting must be in <StartCoverPage,EndCoverPage> format
            return false;
        QByteArray newSetting = currentSetting.split(',')[0] + "," + choice;
        cpdbAddSettingToPrinter(m_printerObj, CPDB_OPTION_JOB_SHEETS, newSetting);
        return true;
    }

    if (key == QPrintDevice::PDPK_AdvancedOptions) {
        if (value.canConvert<QByteArray>() && value.toByteArray() == "#clear#") {
            GList *keys = g_hash_table_get_keys(m_printerObj->settings->table);
            GList *iter = keys;
            while (iter != NULL) {
                const char *key = (const char *) iter->data;
                if (!m_nonAdvancedOptions.contains(QByteArray(key)))
                    cpdbClearSettingFromPrinter(m_printerObj, key);
                iter = iter->next;
            }
            g_list_free(keys);
            return true;
        }
        auto setting = qvariant_cast<QPrint::OptionSetting>(value);
        cpdbAddSettingToPrinter(m_printerObj, setting.name, setting.choice);
        return true;
    }

    if (key == QPrintDevice::PDPK_NumberUp) {
        auto choice = qvariant_cast<QByteArray>(value);
        if (!choice.contains('x')) {
            cpdbAddSettingToPrinter(m_printerObj, CPDB_OPTION_NUMBER_UP, choice);
            return true;
        } else if (QCPDBSupport::numUpDist.contains(choice)) {
            cpdbAddSettingToPrinter(m_printerObj, CPDB_OPTION_NUMBER_UP, QCPDBSupport::numUpDist.value(choice));
            return true;
        }
        return false;
    }

    if (key == QPrintDevice::PDPK_NumberUpLayout) {
        auto choice = qvariant_cast<QByteArray>(value);
        cpdbAddSettingToPrinter(m_printerObj, CPDB_OPTION_NUMBER_UP_LAYOUT, choice);
        return true;
    }

    if (key == QPrintDevice::PDPK_PageSize) {
        auto choice = qvariant_cast<QByteArray>(value);
        if (choice != "#custom#") {
            cpdbClearSetting(m_printerObj->settings, CPDB_OPTION_MEDIA_COL);
            cpdbAddSettingToPrinter(m_printerObj, CPDB_OPTION_MEDIA, choice);
        }
        return true;
    }

    if (key == QPrintDevice::PDPK_PageLayout) {
        auto choice = qvariant_cast<QPrint::PageLayout>(value);

        int width = choice.size.width() * 100.0;
        int height = choice.size.height() * 100.0;
        int left = choice.margins.left() * 100.0;
        int right = choice.margins.right() * 100.0;
        int top = choice.margins.top() * 100.0;
        int bottom = choice.margins.bottom() * 100.0;

        QString mediaCol = QStringLiteral("{media-size={x-dimension=%1 y-dimension=%2} media-bottom-margin=%3"
                                          " media-left-margin=%4 media-right-margin=%5 media-top-margin=%6}")
                                .arg(width).arg(height).arg(bottom).arg(left).arg(right).arg(top);

        cpdbAddSettingToPrinter(m_printerObj, CPDB_OPTION_MEDIA_COL, mediaCol.toLocal8Bit().constData());
        return true;
    }

    return QPlatformPrintDevice::setProperty(key, value);
}
