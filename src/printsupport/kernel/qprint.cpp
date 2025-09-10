// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qprint_p.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

// Note: PPD standard does not define a standard set of InputSlot keywords,
// it is a free form text field left to the PPD writer to decide,
// but it does suggest some names for consistency with the Windows enum.
static const InputSlotMap inputSlotMap[] = {
    { QPrint::Upper,           DMBIN_UPPER,          "Upper"          },
    { QPrint::Lower,           DMBIN_LOWER,          "Lower"          },
    { QPrint::Middle,          DMBIN_MIDDLE,         "Middle"         },
    { QPrint::Manual,          DMBIN_MANUAL,         "Manual"         },
    { QPrint::Envelope,        DMBIN_ENVELOPE,       "Envelope"       },
    { QPrint::EnvelopeManual,  DMBIN_ENVMANUAL,      "EnvelopeManual" },
    { QPrint::Auto,            DMBIN_AUTO,           "Auto"           },
    { QPrint::Tractor,         DMBIN_TRACTOR,        "Tractor"        },
    { QPrint::SmallFormat,     DMBIN_SMALLFMT,       "AnySmallFormat" },
    { QPrint::LargeFormat,     DMBIN_LARGEFMT,       "AnyLargeFormat" },
    { QPrint::LargeCapacity,   DMBIN_LARGECAPACITY,  "LargeCapacity"  },
    { QPrint::Cassette,        DMBIN_CASSETTE,       "Cassette"       },
    { QPrint::FormSource,      DMBIN_FORMSOURCE,     "FormSource"     },
    { QPrint::Manual,          DMBIN_MANUAL,         "ManualFeed"     },
    { QPrint::OnlyOne,         DMBIN_ONLYONE,        "OnlyOne"        }, // = QPrint::Upper
    { QPrint::CustomInputSlot, DMBIN_USER,           ""               }  // Must always be last row
};

static const OutputBinMap outputBinMap[] = {
    { QPrint::AutoOutputBin,   ""      }, // Not a PPD defined value, internal use only
    { QPrint::UpperBin,        "Upper" },
    { QPrint::LowerBin,        "Lower" },
    { QPrint::RearBin,         "Rear"  },
    { QPrint::CustomOutputBin, ""      }  // Must always be last row
};

namespace QPrintUtils {

QPrint::InputSlotId inputSlotKeyToInputSlotId(const QByteArray &key)
{
    for (int i = 0; inputSlotMap[i].id != QPrint::CustomInputSlot; ++i) {
        if (inputSlotMap[i].key == key)
            return inputSlotMap[i].id;
    }
    return QPrint::CustomInputSlot;
}

QByteArray inputSlotIdToInputSlotKey(QPrint::InputSlotId id)
{
    for (int i = 0; inputSlotMap[i].id != QPrint::CustomInputSlot; ++i) {
        if (inputSlotMap[i].id == id)
            return QByteArray(inputSlotMap[i].key);
    }
    return QByteArray();
}

int inputSlotIdToWindowsId(QPrint::InputSlotId id)
{
    for (int i = 0; inputSlotMap[i].id != QPrint::CustomInputSlot; ++i) {
        if (inputSlotMap[i].id == id)
            return inputSlotMap[i].windowsId;
    }
    return 0;
}

QPrint::OutputBinId outputBinKeyToOutputBinId(const QByteArray &key)
{
    for (int i = 0; outputBinMap[i].id != QPrint::CustomOutputBin; ++i) {
        if (outputBinMap[i].key == key)
            return outputBinMap[i].id;
    }
    return QPrint::CustomOutputBin;
}

QByteArray outputBinIdToOutputBinKey(QPrint::OutputBinId id)
{
    for (int i = 0; outputBinMap[i].id != QPrint::CustomOutputBin; ++i) {
        if (outputBinMap[i].id == id)
            return QByteArray(outputBinMap[i].key);
    }
    return QByteArray();
}

QPrint::InputSlot paperBinToInputSlot(int windowsId, const QString &name)
{
    QPrint::InputSlot slot;
    slot.name = name;
    int i;
    for (i = 0; inputSlotMap[i].id != QPrint::CustomInputSlot; ++i) {
        if (inputSlotMap[i].windowsId == windowsId) {
            slot.key = inputSlotMap[i].key;
            slot.id = inputSlotMap[i].id;
            slot.windowsId = inputSlotMap[i].windowsId;
            return slot;
        }
    }
    slot.key = inputSlotMap[i].key;
    slot.id = inputSlotMap[i].id;
    slot.windowsId = windowsId;
    return slot;
}

#if (defined Q_OS_MACOS) || (defined Q_OS_UNIX && QT_CONFIG(cups))

// PPD utilities shared by CUPS and Mac plugins requiring CUPS headers
// May turn into a proper internal QPpd class if enough shared between Mac and CUPS,
// but where would it live?  Not in base module as don't want to link to CUPS.
// May have to have two copies in plugins to keep in sync.

QPrint::InputSlot ppdChoiceToInputSlot(const ppd_choice_t &choice)
{
    QPrint::InputSlot input;
    input.key = choice.choice;
    input.name = QString::fromUtf8(choice.text);
    input.id = inputSlotKeyToInputSlotId(input.key);
    input.windowsId = inputSlotMap[input.id].windowsId;
    return input;
}

QPrint::OutputBin ppdChoiceToOutputBin(const ppd_choice_t &choice)
{
    QPrint::OutputBin output;
    output.key = choice.choice;
    output.name = QString::fromUtf8(choice.text);
    output.id = outputBinKeyToOutputBinId(output.key);
    return output;
}

int parsePpdResolution(const QByteArray &value)
{
    if (value.isEmpty())
        return -1;
    // value can be in form 600dpi or 600x600dpi
    QByteArray result = value.split('x').at(0);
    if (result.endsWith("dpi"))
        result.chop(3);
    return result.toInt();
}

QPrint::DuplexMode ppdChoiceToDuplexMode(const QByteArray &choice)
{
    if (choice == "DuplexTumble")
        return QPrint::DuplexShortSide;
    else if (choice == "DuplexNoTumble")
        return QPrint::DuplexLongSide;
    else // None or SimplexTumble or SimplexNoTumble
        return QPrint::DuplexNone;
}

#endif // Mac and CUPS PPD Utilities

}

#endif // QT_NO_PRINTER

QT_END_NAMESPACE
