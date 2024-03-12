// Copyright (C) 2014 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qppdprintdevice.h"

#include "qcupsprintersupport_p.h"
#include "qcups_p.h" // Only needed for PDPK_*

#if QT_CONFIG(mimetype)
#include <QtCore/QMimeDatabase>
#endif

#include <QPrintDialog>

#ifndef QT_LINUXBASE // LSB merges everything into cups.h
#include <cups/language.h>
#endif

QT_BEGIN_NAMESPACE

// avoid all the warnings about using deprecated API from CUPS (as there is no real replacement)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED

QPpdPrintDevice::QPpdPrintDevice(const QString &id)
    : QPlatformPrintDevice(id),
      m_cupsDest(0),
      m_ppd(0)
{
    if (!id.isEmpty()) {

        // TODO For now each dest is an individual device
        const auto parts = QStringView{id}.split(u'/');
        m_cupsName = parts.at(0).toUtf8();
        if (parts.size() > 1)
            m_cupsInstance = parts.at(1).toUtf8();

        // Get the print instance and PPD file
        m_cupsDest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, m_cupsName, m_cupsInstance.isNull() ? nullptr : m_cupsInstance.constData());
        if (m_cupsDest) {
            const char *ppdFile = cupsGetPPD(m_cupsName);
            if (ppdFile) {
                m_ppd = ppdOpenFile(ppdFile);
                unlink(ppdFile);
            }
            if (m_ppd) {
                ppdMarkDefaults(m_ppd);
                cupsMarkOptions(m_ppd, m_cupsDest->num_options, m_cupsDest->options);
                ppdLocalize(m_ppd);

                m_minimumPhysicalPageSize = QSize(m_ppd->custom_min[0], m_ppd->custom_min[1]);
                m_maximumPhysicalPageSize = QSize(m_ppd->custom_max[0], m_ppd->custom_max[1]);
                m_customMargins = QMarginsF(m_ppd->custom_margins[0], m_ppd->custom_margins[3],
                                            m_ppd->custom_margins[2], m_ppd->custom_margins[1]);
            }

            m_name = printerOption("printer-info");
            m_location = printerOption("printer-location");
            m_makeAndModel = printerOption("printer-make-and-model");
            cups_ptype_e type = printerTypeFlags();
            m_isRemote = type & CUPS_PRINTER_REMOTE;
            // Note this is if the hardware does multiple copies, not if Cups can
            m_supportsMultipleCopies = type & CUPS_PRINTER_COPIES;
            // Note this is if the hardware does collation, not if Cups can
            m_supportsCollateCopies = type & CUPS_PRINTER_COLLATE;

            // Custom Page Size support
            // Cups cups_ptype_e CUPS_PRINTER_VARIABLE
            // Cups ppd_file_t variable_sizes custom_min custom_max
            // PPD MaxMediaWidth MaxMediaHeight
            m_supportsCustomPageSizes = type & CUPS_PRINTER_VARIABLE;
        }
    }
}

QPpdPrintDevice::~QPpdPrintDevice()
{
    if (m_ppd)
        ppdClose(m_ppd);
    if (m_cupsDest)
        cupsFreeDests(1, m_cupsDest);
    m_cupsDest = 0;
    m_ppd = 0;
}

bool QPpdPrintDevice::isValid() const
{
    return m_cupsDest;
}

bool QPpdPrintDevice::isDefault() const
{
    // There seems to be a bug in cups in which printerTypeFlags
    // returns CUPS_PRINTER_DEFAULT based only on system values, ignoring user lpoptions
    // so we can't use that. And also there seems to be a bug in which dests returned
    // by cupsGetNamedDest don't have is_default set at all so we can't use that either
    // so go the long route and compare our id against the defaultPrintDeviceId
    return id() == QCupsPrinterSupport::staticDefaultPrintDeviceId();
}

QPrint::DeviceState QPpdPrintDevice::state() const
{
    // 3 = idle, 4 = printing, 5 = stopped
    // More details available from printer-state-message and printer-state-reasons
    int state =  printerOption(QStringLiteral("printer-state")).toInt();
    if (state == 3)
        return QPrint::Idle;
    else if (state == 4)
        return QPrint::Active;
    else
        return QPrint::Error;
}

void QPpdPrintDevice::loadPageSizes() const
{
    m_pageSizes.clear();
    m_printableMargins.clear();

    ppd_option_t *pageSizes = ppdFindOption(m_ppd, "PageSize");
    if (pageSizes) {
        for (int i = 0; i < pageSizes->num_choices; ++i) {
            const ppd_size_t *ppdSize = ppdPageSize(m_ppd, pageSizes->choices[i].choice);
            if (ppdSize) {
                // Returned size is in points
                QString key = QString::fromUtf8(ppdSize->name);
                QSize size = QSize(qRound(ppdSize->width), qRound(ppdSize->length));
                QString name = QString::fromUtf8(pageSizes->choices[i].text);
                if (!size.isEmpty()) {
                    QPageSize ps = createPageSize(key, size, name);
                    if (ps.isValid()) {
                        m_pageSizes.append(ps);
                        m_printableMargins.insert(key, QMarginsF(ppdSize->left, ppdSize->length - ppdSize->top,
                                                                 ppdSize->width - ppdSize->right, ppdSize->bottom));
                    }
                }
            }
        }
    }
    m_havePageSizes = true;
}

QPageSize QPpdPrintDevice::defaultPageSize() const
{
    ppd_choice_t *defaultChoice = ppdFindMarkedChoice(m_ppd, "PageSize");
    if (defaultChoice) {
        ppd_size_t *ppdSize = ppdPageSize(m_ppd, defaultChoice->choice);
        if (ppdSize) {
            // Returned size is in points
            QString key = QString::fromUtf8(ppdSize->name);
            QSize size = QSize(qRound(ppdSize->width), qRound(ppdSize->length));
            QString name = QString::fromUtf8(defaultChoice->text);
            return createPageSize(key, size, name);
        }
    }
    return QPageSize();
}

QMarginsF QPpdPrintDevice::printableMargins(const QPageSize &pageSize,
                                            QPageLayout::Orientation orientation,
                                            int resolution) const
{
    Q_UNUSED(orientation);
    Q_UNUSED(resolution);
    if (!m_havePageSizes)
        loadPageSizes();
    // TODO Orientation?
    if (m_printableMargins.contains(pageSize.key()))
        return m_printableMargins.value(pageSize.key());
    return m_customMargins;
}

void QPpdPrintDevice::loadResolutions() const
{
    m_resolutions.clear();

    // Try load standard PPD options first
    ppd_option_t *resolutions = ppdFindOption(m_ppd, "Resolution");
    if (resolutions) {
        for (int i = 0; i < resolutions->num_choices; ++i) {
            int res = QPrintUtils::parsePpdResolution(resolutions->choices[i].choice);
            if (res > 0)
                m_resolutions.append(res);
        }
    }
    // If no result, try just the default
    if (m_resolutions.size() == 0) {
        resolutions = ppdFindOption(m_ppd, "DefaultResolution");
        if (resolutions) {
            int res = QPrintUtils::parsePpdResolution(resolutions->choices[0].choice);
            if (res > 0)
                m_resolutions.append(res);
        }
    }
    // If still no result, then try HP's custom options
    if (m_resolutions.size() == 0) {
        resolutions = ppdFindOption(m_ppd, "HPPrintQuality");
        if (resolutions) {
            for (int i = 0; i < resolutions->num_choices; ++i) {
                int res = QPrintUtils::parsePpdResolution(resolutions->choices[i].choice);
                if (res > 0)
                    m_resolutions.append(res);
            }
        }
    }
    if (m_resolutions.size() == 0) {
        resolutions = ppdFindOption(m_ppd, "DefaultHPPrintQuality");
        if (resolutions) {
            int res = QPrintUtils::parsePpdResolution(resolutions->choices[0].choice);
            if (res > 0)
                m_resolutions.append(res);
        }
    }
    m_haveResolutions = true;
}

int QPpdPrintDevice::defaultResolution() const
{
    // Try load standard PPD option first
    ppd_option_t *resolution = ppdFindOption(m_ppd, "DefaultResolution");
    if (resolution) {
        int res = QPrintUtils::parsePpdResolution(resolution->choices[0].choice);
        if (res > 0)
            return res;
    }
    // If no result, then try a marked option
    ppd_choice_t *defaultChoice = ppdFindMarkedChoice(m_ppd, "Resolution");
    if (defaultChoice) {
        int res = QPrintUtils::parsePpdResolution(defaultChoice->choice);
        if (res > 0)
            return res;
    }
    // If still no result, then try HP's custom options
    resolution = ppdFindOption(m_ppd, "DefaultHPPrintQuality");
    if (resolution) {
        int res = QPrintUtils::parsePpdResolution(resolution->choices[0].choice);
        if (res > 0)
            return res;
    }
    defaultChoice = ppdFindMarkedChoice(m_ppd, "HPPrintQuality");
    if (defaultChoice) {
        int res = QPrintUtils::parsePpdResolution(defaultChoice->choice);
        if (res > 0)
            return res;
    }
    // Otherwise return a sensible default.
    // TODO What is sensible? 150? 300?
    return 72;
}

void QPpdPrintDevice::loadInputSlots() const
{
    // NOTE: Implemented in both CUPS and Mac plugins, please keep in sync
    // TODO Deal with concatenated names like Tray1Manual or Tray1_Man,
    //      will currently show as CustomInputSlot
    // TODO Deal with separate ManualFeed key
    // Try load standard PPD options first
    m_inputSlots.clear();
    if (m_ppd) {
        ppd_option_t *inputSlots = ppdFindOption(m_ppd, "InputSlot");
        if (inputSlots) {
            m_inputSlots.reserve(inputSlots->num_choices);
            for (int i = 0; i < inputSlots->num_choices; ++i)
                m_inputSlots.append(QPrintUtils::ppdChoiceToInputSlot(inputSlots->choices[i]));
        }
        // If no result, try just the default
        if (m_inputSlots.size() == 0) {
            inputSlots = ppdFindOption(m_ppd, "DefaultInputSlot");
            if (inputSlots)
                m_inputSlots.append(QPrintUtils::ppdChoiceToInputSlot(inputSlots->choices[0]));
        }
    }
    // If still no result, just use Auto
    if (m_inputSlots.size() == 0)
        m_inputSlots.append(QPlatformPrintDevice::defaultInputSlot());
    m_haveInputSlots = true;
}

QPrint::InputSlot QPpdPrintDevice::defaultInputSlot() const
{
    // NOTE: Implemented in both CUPS and Mac plugins, please keep in sync
    // Try load standard PPD option first
    if (m_ppd) {
        ppd_option_t *inputSlot = ppdFindOption(m_ppd, "DefaultInputSlot");
        if (inputSlot)
            return QPrintUtils::ppdChoiceToInputSlot(inputSlot->choices[0]);
        // If no result, then try a marked option
        ppd_choice_t *defaultChoice = ppdFindMarkedChoice(m_ppd, "InputSlot");
        if (defaultChoice)
            return QPrintUtils::ppdChoiceToInputSlot(*defaultChoice);
    }
    // Otherwise return Auto
    return QPlatformPrintDevice::defaultInputSlot();
}

void QPpdPrintDevice::loadOutputBins() const
{
    // NOTE: Implemented in both CUPS and Mac plugins, please keep in sync
    m_outputBins.clear();
    if (m_ppd) {
        ppd_option_t *outputBins = ppdFindOption(m_ppd, "OutputBin");
        if (outputBins) {
            m_outputBins.reserve(outputBins->num_choices);
            for (int i = 0; i < outputBins->num_choices; ++i)
                m_outputBins.append(QPrintUtils::ppdChoiceToOutputBin(outputBins->choices[i]));
        }
        // If no result, try just the default
        if (m_outputBins.size() == 0) {
            outputBins = ppdFindOption(m_ppd, "DefaultOutputBin");
            if (outputBins)
                m_outputBins.append(QPrintUtils::ppdChoiceToOutputBin(outputBins->choices[0]));
        }
    }
    // If still no result, just use Auto
    if (m_outputBins.size() == 0)
        m_outputBins.append(QPlatformPrintDevice::defaultOutputBin());
    m_haveOutputBins = true;
}

QPrint::OutputBin QPpdPrintDevice::defaultOutputBin() const
{
    // NOTE: Implemented in both CUPS and Mac plugins, please keep in sync
    // Try load standard PPD option first
    if (m_ppd) {
        ppd_option_t *outputBin = ppdFindOption(m_ppd, "DefaultOutputBin");
        if (outputBin)
            return QPrintUtils::ppdChoiceToOutputBin(outputBin->choices[0]);
        // If no result, then try a marked option
        ppd_choice_t *defaultChoice = ppdFindMarkedChoice(m_ppd, "OutputBin");
        if (defaultChoice)
            return QPrintUtils::ppdChoiceToOutputBin(*defaultChoice);
    }
    // Otherwise return AutoBin
    return QPlatformPrintDevice::defaultOutputBin();
}

void QPpdPrintDevice::loadDuplexModes() const
{
    // NOTE: Implemented in both CUPS and Mac plugins, please keep in sync
    // Try load standard PPD options first
    m_duplexModes.clear();
    if (m_ppd) {
        ppd_option_t *duplexModes = ppdFindOption(m_ppd, "Duplex");
        if (duplexModes) {
            m_duplexModes.reserve(duplexModes->num_choices);
            for (int i = 0; i < duplexModes->num_choices; ++i) {
                if (ppdInstallableConflict(m_ppd, duplexModes->keyword, duplexModes->choices[i].choice) == 0) {
                    m_duplexModes.append(QPrintUtils::ppdChoiceToDuplexMode(duplexModes->choices[i].choice));
                }
            }
        }
        // If no result, try just the default
        if (m_duplexModes.size() == 0) {
            duplexModes = ppdFindOption(m_ppd, "DefaultDuplex");
            if (duplexModes && (ppdInstallableConflict(m_ppd, duplexModes->keyword, duplexModes->choices[0].choice) == 0)) {
                m_duplexModes.append(QPrintUtils::ppdChoiceToDuplexMode(duplexModes->choices[0].choice));
            }
        }
    }
    // If still no result, or not added in PPD, then add None
    if (m_duplexModes.size() == 0 || !m_duplexModes.contains(QPrint::DuplexNone))
        m_duplexModes.append(QPrint::DuplexNone);
    // If have both modes, then can support DuplexAuto
    if (m_duplexModes.contains(QPrint::DuplexLongSide) && m_duplexModes.contains(QPrint::DuplexShortSide))
        m_duplexModes.append(QPrint::DuplexAuto);
    m_haveDuplexModes = true;
}

QPrint::DuplexMode QPpdPrintDevice::defaultDuplexMode() const
{
    // Try load standard PPD option first
    if (m_ppd) {
        ppd_option_t *inputSlot = ppdFindOption(m_ppd, "DefaultDuplex");
        if (inputSlot)
            return QPrintUtils::ppdChoiceToDuplexMode(inputSlot->choices[0].choice);
        // If no result, then try a marked option
        ppd_choice_t *defaultChoice = ppdFindMarkedChoice(m_ppd, "Duplex");
        if (defaultChoice)
            return QPrintUtils::ppdChoiceToDuplexMode(defaultChoice->choice);
    }
    // Otherwise return None
    return QPrint::DuplexNone;
}

void QPpdPrintDevice::loadColorModes() const
{
    // Cups cups_ptype_e CUPS_PRINTER_BW CUPS_PRINTER_COLOR
    // Cups ppd_file_t color_device
    // PPD ColorDevice
    m_colorModes.clear();
    cups_ptype_e type = printerTypeFlags();
    if (type & CUPS_PRINTER_BW)
        m_colorModes.append(QPrint::GrayScale);
    if (type & CUPS_PRINTER_COLOR)
        m_colorModes.append(QPrint::Color);
    m_haveColorModes = true;
}

QPrint::ColorMode QPpdPrintDevice::defaultColorMode() const
{
    // NOTE: Implemented in both CUPS and Mac plugins, please keep in sync
    // Not a proper option, usually only know if supports color or not, but some
    // users known to abuse ColorModel to always force GrayScale.
    if (m_ppd && supportedColorModes().contains(QPrint::Color)) {
        ppd_option_t *colorModel = ppdFindOption(m_ppd, "DefaultColorModel");
        if (!colorModel)
            colorModel = ppdFindOption(m_ppd, "ColorModel");
        if (!colorModel || qstrcmp(colorModel->defchoice, "Gray") != 0)
            return QPrint::Color;
    }
    return QPrint::GrayScale;
}

QVariant QPpdPrintDevice::property(QPrintDevice::PrintDevicePropertyKey key) const
{
    if (key == PDPK_PpdFile) {
        return QVariant::fromValue<ppd_file_t *>(m_ppd);
    }

    if (key == QPrintDevice::PDPK_PageSet) {
        QPrint::OptionCombo option;
        option.name = "page-set";
        option.displayName = QPrintDialog::tr("Page Set");
        option.choices
                << "all"
                << "odd"
                << "even";
        option.displayChoices
                << QPrintDialog::tr("All Pages")
                << QPrintDialog::tr("Odd Pages")
                << QPrintDialog::tr("Even Pages");
        option.defaultChoice = 0;
        return QVariant::fromValue(option);
    }

    if (key == QPrintDevice::PDPK_JobHold) {
        QPrint::OptionCombo option;
        option.name = "job-hold-until";
        option.choices
                << "no-hold"
                << "indefinite"
                << "day-time"
                << "night"
                << "second-shift"
                << "third-shift"
                << "weekend";
        option.displayChoices
                << QPrintDialog::tr("Print Immediately")
                << QPrintDialog::tr("Hold Indefinitely")
                << QPrintDialog::tr("Day (06:00 to 17:59)")
                << QPrintDialog::tr("Night (18:00 to 05:59)")
                << QPrintDialog::tr("Second Shift (16:00 to 23:59)")
                << QPrintDialog::tr("Third Shift (00:00 to 07:59)")
                << QPrintDialog::tr("Weekend (Saturday to Sunday)")
                << QPrintDialog::tr("Specific Time");

        QByteArray defaultVal = printerOption(QString(option.name)).toUtf8();
        option.defaultChoice = qMax(option.choices.indexOf(defaultVal), 0);
        return QVariant::fromValue(option);
    }

    if (key == QPrintDevice::PDPK_JobBillingInfo) {
        return QVariant::fromValue(printerOption(QStringLiteral("job-billing")));
    }

    if (key == QPrintDevice::PDPK_JobPriority) {
        bool ok;
        int defaultVal = printerOption(QStringLiteral("job-priority")).toInt(&ok);
        if (!ok || defaultVal > 100 || defaultVal < 0)
            defaultVal = 50;
        return QVariant(defaultVal);
    }

    if (key == QPrintDevice::PDPK_JobStartCoverPage) {
        QPrint::OptionCombo option;
        option.name = "job-sheets";
        option.choices
                << "none"
                << "standard"
                << "unclassified"
                << "confidential"
                << "classified"
                << "secret"
                << "topsecret";
        option.displayChoices
                << QPrintDialog::tr("None", "CUPS Banner page")
                << QPrintDialog::tr("Standard", "CUPS Banner page")
                << QPrintDialog::tr("Unclassified", "CUPS Banner page")
                << QPrintDialog::tr("Confidential", "CUPS Banner page")
                << QPrintDialog::tr("Classified", "CUPS Banner page")
                << QPrintDialog::tr("Secret", "CUPS Banner page")
                << QPrintDialog::tr("Top Secret", "CUPS Banner page");

        QByteArray defaultVal = printerOption(QString(option.name)).toUtf8();
        option.defaultChoice = qMax(option.choices.indexOf(defaultVal), 0);
        return QVariant::fromValue(option);
    }

    if (key == QPrintDevice::PDPK_JobEndCoverPage) {
        QPrint::OptionCombo option;
        option.name = "job-sheets";
        option.choices
                << "none"
                << "standard"
                << "unclassified"
                << "confidential"
                << "classified"
                << "secret"
                << "topsecret";
        option.displayChoices
                << QPrintDialog::tr("None", "CUPS Banner page")
                << QPrintDialog::tr("Standard", "CUPS Banner page")
                << QPrintDialog::tr("Unclassified", "CUPS Banner page")
                << QPrintDialog::tr("Confidential", "CUPS Banner page")
                << QPrintDialog::tr("Classified", "CUPS Banner page")
                << QPrintDialog::tr("Secret", "CUPS Banner page")
                << QPrintDialog::tr("Top Secret", "CUPS Banner page");

        QByteArray defaultVal = printerOption(QString(option.name)).toUtf8();
        option.defaultChoice = qMax(option.choices.indexOf(defaultVal), 0);
        return QVariant::fromValue(option);
    }

    if (key == QPrintDevice::PDPK_AdvancedOptions) {
        QList<QPrint::OptionCombosGroup> optionsGroups;
        if (!m_ppd)
            return QVariant::fromValue(optionsGroups);

        auto toUnicode = QStringDecoder(m_ppd->lang_encoding, QStringDecoder::Flag::Stateless);
        if (!toUnicode.isValid()) {
            qWarning() << "QPrinSupport: Cups uses unsupported encoding" << m_ppd->lang_encoding;
            toUnicode = QStringDecoder(QStringDecoder::Utf8, QStringDecoder::Flag::Stateless);
        }
        for (int i = 0; i < m_ppd->num_groups; ++i) {

            const ppd_group_t *group = &m_ppd->groups[i];
            if (QCUPSSupport::isBlacklistedGroup(group))
                continue;

            QPrint::OptionCombosGroup optionsGroup;
            optionsGroup.groupName = group->name;
            optionsGroup.displayGroup = toUnicode(group->text);

            for (int i = 0; i < group->num_options; ++i) {
                const ppd_option_t *option = &group->options[i];

                if (QCUPSSupport::isBlacklistedOption(option->keyword) || option->num_choices <= 1)
                    continue;

                QPrint::OptionCombo optionCombo;
                optionCombo.name = option->keyword;
                optionCombo.displayName = option->text;

                optionCombo.choices.reserve(option->num_choices);
                optionCombo.displayChoices.reserve(option->num_choices);
                optionCombo.defaultChoice = -1;

                bool foundMarkedChoice = false;
                bool markedChoiceNotAvailable = false;
                for (int i = 0; i < option->num_choices; ++i) {
                    const ppd_choice_t *choice = &option->choices[i];
                    const bool choiceIsInstallableConflict = ppdInstallableConflict(m_ppd, option->keyword, choice->choice);
                    if (choiceIsInstallableConflict && static_cast<int>(choice->marked) == 1) {
                        markedChoiceNotAvailable = true;
                    } else if (!choiceIsInstallableConflict) {
                        optionCombo.choices.push_back(choice->choice);
                        optionCombo.displayChoices.push_back(toUnicode(choice->text));
                        if (static_cast<int>(choice->marked) == 1) {
                            optionCombo.defaultChoice = optionCombo.choices.size() - 1;
                            foundMarkedChoice = true;
                        } else if (!foundMarkedChoice && qstrcmp(choice->choice, option->defchoice) == 0) {
                            optionCombo.defaultChoice = optionCombo.choices.size() - 1;
                        }
                    }
                }

                if (markedChoiceNotAvailable) {
                    // If the user default option is not available because of it conflicting with
                    // the installed options, we need to set the internal ppd value to the value
                    // being shown in the combo
                    optionCombo.defaultChoice = qMax(optionCombo.defaultChoice, 0);
                    ppdMarkOption(m_ppd, optionCombo.name, optionCombo.choices[optionCombo.defaultChoice]);
                }

                optionsGroup.options.push_back(optionCombo);
            }
            optionsGroups.push_back(optionsGroup);
        }
        return QVariant::fromValue(optionsGroups);
    }

    if (key == QPrintDevice::PDPK_OptionConflict) {
        QByteArray pagesPerSheet = this->getCupsOption(QStringLiteral("number-up")).toUtf8();
        QByteArray pageSet = this->getCupsOption(QStringLiteral("page-set")).toUtf8();
        if (pagesPerSheet != "1x1" && pageSet != "all")
            return QVariant(QPrintDialog::tr("Options 'Pages Per Sheet' and 'Page Set' cannot be used together.\nPlease turn one of those options off."));

        return QVariant::fromValue(nullptr);
    }

    if (key == QPrintDevice::PDPK_NumberUp) {
        QPrint::OptionCombo option;
        option.name = "number-up";
        option.choices
                << "1x1"
                << "2x1"
                << "2x2"
                << "2x3"
                << "3x3"
                << "4x4";
        option.displayChoices
                << QPrintDialog::tr("1 (1x1)")
                << QPrintDialog::tr("2 (2x1)")
                << QPrintDialog::tr("4 (2x2)")
                << QPrintDialog::tr("6 (2x3)")
                << QPrintDialog::tr("9 (3x3)")
                << QPrintDialog::tr("16 (4x4)");

        option.defaultChoice = 0;
        return QVariant::fromValue(option);
    }
    if (key == QPrintDevice::PDPK_NumberUpLayout) {
        QPrint::OptionCombo option;
        option.name = "number-up-layout";
        option.choices
                << "lrtb"
                << "lrbt"
                << "rlbt"
                << "rltb"
                << "btlr"
                << "btrl"
                << "tblr"
                << "tbrl";
        option.displayChoices
                << QPrintDialog::tr("Left to Right, Top to Bottom")
                << QPrintDialog::tr("Left to Right, Bottom to Top")
                << QPrintDialog::tr("Right to Left, Bottom to Top")
                << QPrintDialog::tr("Right to Left, Top to Bottom")
                << QPrintDialog::tr("Bottom to Top, Left to Right")
                << QPrintDialog::tr("Bottom to Top, Right to Left")
                << QPrintDialog::tr("Top to Bottom, Left to Right")
                << QPrintDialog::tr("Top to Bottom, Right to Left");

        option.defaultChoice = 0;
        return QVariant::fromValue(option);
    }

    return QPlatformPrintDevice::property(key);
}

bool QPpdPrintDevice::setProperty(QPrintDevice::PrintDevicePropertyKey key, const QVariant &value)
{
    if (key == QPrintDevice::PDPK_Duplex) {
        auto choice = qvariant_cast<QPrint::DuplexMode>(value);
        bool marked = false;
        if (choice == QPrint::DuplexNone) {
            ppdMarkOption(m_ppd, "Duplex", "None");
            marked = true;
        } else if (choice == QPrint::DuplexLongSide) {
            ppdMarkOption(m_ppd, "Duplex", "DuplexNoTumble");
            marked = true;
        } else if (choice == QPrint::DuplexShortSide) {
            ppdMarkOption(m_ppd, "Duplex", "DuplexTumble");
            marked = true;
        }
        return marked;
    }

    if (key == QPrintDevice::PDPK_PageSet) {
        auto choice = qvariant_cast<QByteArray>(value);
        setCupsOption(QStringLiteral("page-set"), choice);
        return true;
    }

    if (key == QPrintDevice::PDPK_PageRange) {
        auto choice = qvariant_cast<QString>(value);
        setCupsOption(QStringLiteral("page-ranges"), choice);
        return true;
    }

    if (key == QPrintDevice::PDPK_JobHold) {
        auto choice = qvariant_cast<QByteArray>(value);
        setCupsOption(QStringLiteral("job-hold-until"), choice);
        return true;
    }

    if (key == QPrintDevice::PDPK_JobBillingInfo) {
        auto choice = qvariant_cast<QString>(value);
        setCupsOption(QStringLiteral("job-billing"), choice);
        return true;
    }

    if (key == QPrintDevice::PDPK_JobPriority) {
        int priority = qvariant_cast<int>(value);
        setCupsOption(QStringLiteral("job-priority"), QString::number(priority));
        return true;
    }

    if (key == QPrintDevice::PDPK_JobStartCoverPage) {
        auto choice = qvariant_cast<QByteArray>(value);
        QString currentSetting = this->getCupsOption(QStringLiteral("job-sheets"));
        if (!currentSetting.contains(u',')) {
            QString defaultSheet = printerOption(QStringLiteral("job-sheets"));
            currentSetting = defaultSheet + u',' + defaultSheet;
        }
        QString newSetting = choice + u',' + currentSetting.split(u',')[1];
        setCupsOption(QStringLiteral("job-sheets"), newSetting);
        return true;
    }

    if (key == QPrintDevice::PDPK_JobEndCoverPage) {
        auto choice = qvariant_cast<QByteArray>(value);
        QString currentSetting = this->getCupsOption(QStringLiteral("job-sheets"));
        if (!currentSetting.contains(u',')) {
            QString defaultSheet = printerOption(QStringLiteral("job-sheets"));
            currentSetting = defaultSheet + u',' + defaultSheet;
        }
        QString newSetting = currentSetting.split(u',')[0] + u',' + choice;
        setCupsOption(QStringLiteral("job-sheets"), newSetting.toUtf8());
        return true;
    }

    if (key == QPrintDevice::PDPK_AdvancedOptions) {
        if (value.canConvert<QByteArray>() && value.toByteArray() == "#clear#") {
            m_cupsOptions = QStringList();
            return true;
        }
        auto setting = qvariant_cast<QPrint::OptionSetting>(value);
        ppdMarkOption(m_ppd, setting.name, setting.choice);
        return true;
    }

    if (key == QPrintDevice::PDPK_NumberUp) {
        auto choice = qvariant_cast<QByteArray>(value);
        setCupsOption(QStringLiteral("number-up"), choice);
    }

    if (key == QPrintDevice::PDPK_NumberUpLayout) {
        auto choice = qvariant_cast<QByteArray>(value);
        setCupsOption(QStringLiteral("number-up-layout"), choice);
        return true;
    }

    if (key == QPrintDevice::PDPK_PageSize) {
        auto choice = qvariant_cast<QByteArray>(value);
        if (choice == "#custom#") {
            ppdMarkOption(m_ppd, "PageSize", "Custom");
        } else {
            ppdMarkOption(m_ppd, "PageSize", choice);
        }
        return true;
    }

    return QPlatformPrintDevice::setProperty(key, value);
}

bool QPpdPrintDevice::isFeatureAvailable(QPrintDevice::PrintDevicePropertyKey key, const QVariant &params) const
{
    if (key == QPrintDevice::PDPK_Duplex) {
        auto duplexPpdOption = findPpdOption("Duplex");
        if (params.toByteArray() == "conflict")
            return (duplexPpdOption && duplexPpdOption->conflicted);
        return (duplexPpdOption != nullptr);
    }

    if (key == QPrintDevice::PDPK_PageSet
            || key == QPrintDevice::PDPK_PageRange
            || key == QPrintDevice::PDPK_JobHold
            || key == QPrintDevice::PDPK_JobBillingInfo
            || key == QPrintDevice::PDPK_JobPriority
            || key == QPrintDevice::PDPK_JobStartCoverPage
            || key == QPrintDevice::PDPK_JobEndCoverPage
            || key == QPrintDevice::PDPK_AdvancedOptions
            || key == QPrintDevice::PDPK_NumberUp
            || key == QPrintDevice::PDPK_NumberUpLayout
            || key == QPrintDevice::PDPK_AdvancedColorMode) {
        return true;
    }

    if (key == QPrintDevice::PDPK_AdvancedOptions) {
        return (m_ppd != nullptr);
    }

    if (key == QPrintDevice::PDPK_OptionConflict) {
        auto setting = qvariant_cast<QPrint::OptionSetting>(params);
        return ppdInstallableConflict(m_ppd, setting.name, setting.choice);
    }

    if (key == QPrintDevice::PDPK_PageSize) {
        auto pageSizePpdOption = findPpdOption("PageSize");
        if (params.toByteArray() == "conflict")
            return (pageSizePpdOption && pageSizePpdOption->conflicted);
        return (pageSizePpdOption != nullptr);
    }

    return QPlatformPrintDevice::isFeatureAvailable(key, params);
}

#if QT_CONFIG(mimetype)
void QPpdPrintDevice::loadMimeTypes() const
{
    // TODO No CUPS api? Need to manually load CUPS mime.types file?
    //      For now hard-code most common support types
    QMimeDatabase db;
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("application/pdf")));
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("application/postscript")));
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("image/gif")));
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("image/png")));
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("image/jpeg")));
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("image/tiff")));
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("text/html")));
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("text/plain")));
    m_haveMimeTypes = true;
}
#endif

void QPpdPrintDevice::setCupsOption(const QString &option, const QString &value)
{
    if (m_cupsOptions.contains(option)) {
        m_cupsOptions.replace(m_cupsOptions.indexOf(option) + 1, value);
    } else {
        m_cupsOptions.append(option);
        m_cupsOptions.append(value);
    }
}

QString QPpdPrintDevice::getCupsOption(const QString &option) const
{
    if (m_cupsOptions.contains(option)) {
        return m_cupsOptions.at(m_cupsOptions.indexOf(option) + 1);
    }
    return QString();
}

QString QPpdPrintDevice::printerOption(const QString &key) const
{
    return cupsGetOption(key.toUtf8(), m_cupsDest->num_options, m_cupsDest->options);
}

ppd_option_t *QPpdPrintDevice::findPpdOption(const char *optionName) const
{
    if (m_ppd) {
        for (int i = 0; i < m_ppd->num_groups; ++i) {
            ppd_group_t *group = &m_ppd->groups[i];

            for (int i = 0; i < group->num_options; ++i) {
                ppd_option_t *option = &group->options[i];

                if (qstrcmp(option->keyword, optionName) == 0)
                    return option;
            }
        }
    }

    return nullptr;
}

cups_ptype_e QPpdPrintDevice::printerTypeFlags() const
{
    return static_cast<cups_ptype_e>(printerOption("printer-type").toUInt());
}

QT_WARNING_POP

QT_END_NAMESPACE
