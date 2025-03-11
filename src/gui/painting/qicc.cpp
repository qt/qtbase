// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qicc_p.h"

#include <qbuffer.h>
#include <qbytearray.h>
#include <qvarlengtharray.h>
#include <qhash.h>
#include <qdatastream.h>
#include <qendian.h>
#include <qloggingcategory.h>
#include <qstring.h>

#include "qcolorclut_p.h"
#include "qcolormatrix_p.h"
#include "qcolorspace_p.h"
#include "qcolortrc_p.h"

#include <array>

QT_BEGIN_NAMESPACE
Q_STATIC_LOGGING_CATEGORY(lcIcc, "qt.gui.icc", QtWarningMsg)

namespace QIcc {

struct ICCProfileHeader
{
    quint32_be profileSize;

    quint32_be preferredCmmType;

    quint32_be profileVersion;
    quint32_be profileClass;
    quint32_be inputColorSpace;
    quint32_be pcs;
    quint32_be datetime[3];
    quint32_be signature;
    quint32_be platformSignature;
    quint32_be flags;
    quint32_be deviceManufacturer;
    quint32_be deviceModel;
    quint32_be deviceAttributes[2];

    quint32_be renderingIntent;
    qint32_be  illuminantXyz[3];

    quint32_be creatorSignature;
    quint32_be profileId[4];

    quint32_be reserved[7];

// Technically after the header, but easier to include here:
    quint32_be tagCount;
};

constexpr quint32 IccTag(uchar a, uchar b, uchar c, uchar d)
{
    return (a << 24) | (b << 16) | (c << 8) | d;
}

enum class ColorSpaceType : quint32 {
    Rgb       = IccTag('R', 'G', 'B', ' '),
    Gray      = IccTag('G', 'R', 'A', 'Y'),
    Cmyk      = IccTag('C', 'M', 'Y', 'K'),
};

enum class ProfileClass : quint32 {
    Input       = IccTag('s', 'c', 'n', 'r'),
    Display     = IccTag('m', 'n', 't', 'r'),
    Output      = IccTag('p', 'r', 't', 'r'),
    ColorSpace  = IccTag('s', 'p', 'a', 'c'),
    // Not supported:
    DeviceLink  = IccTag('l', 'i', 'n', 'k'),
    Abstract    = IccTag('a', 'b', 's', 't'),
    NamedColor  = IccTag('n', 'm', 'c', 'l'),
};

enum class Tag : quint32 {
    acsp = IccTag('a', 'c', 's', 'p'),
    Lab_ = IccTag('L', 'a', 'b', ' '),
    RGB_ = IccTag('R', 'G', 'B', ' '),
    XYZ_ = IccTag('X', 'Y', 'Z', ' '),
    rXYZ = IccTag('r', 'X', 'Y', 'Z'),
    gXYZ = IccTag('g', 'X', 'Y', 'Z'),
    bXYZ = IccTag('b', 'X', 'Y', 'Z'),
    rTRC = IccTag('r', 'T', 'R', 'C'),
    gTRC = IccTag('g', 'T', 'R', 'C'),
    bTRC = IccTag('b', 'T', 'R', 'C'),
    kTRC = IccTag('k', 'T', 'R', 'C'),
    A2B0 = IccTag('A', '2', 'B', '0'),
    A2B1 = IccTag('A', '2', 'B', '1'),
    A2B2 = IccTag('A', '2', 'B', '2'),
    B2A0 = IccTag('B', '2', 'A', '0'),
    B2A1 = IccTag('B', '2', 'A', '1'),
    B2A2 = IccTag('B', '2', 'A', '2'),
    B2D0 = IccTag('B', '2', 'D', '0'),
    B2D1 = IccTag('B', '2', 'D', '1'),
    B2D2 = IccTag('B', '2', 'D', '2'),
    B2D3 = IccTag('B', '2', 'D', '3'),
    D2B0 = IccTag('D', '2', 'B', '0'),
    D2B1 = IccTag('D', '2', 'B', '1'),
    D2B2 = IccTag('D', '2', 'B', '2'),
    D2B3 = IccTag('D', '2', 'B', '3'),
    desc = IccTag('d', 'e', 's', 'c'),
    text = IccTag('t', 'e', 'x', 't'),
    cprt = IccTag('c', 'p', 'r', 't'),
    curv = IccTag('c', 'u', 'r', 'v'),
    para = IccTag('p', 'a', 'r', 'a'),
    wtpt = IccTag('w', 't', 'p', 't'),
    bkpt = IccTag('b', 'k', 'p', 't'),
    mft1 = IccTag('m', 'f', 't', '1'),
    mft2 = IccTag('m', 'f', 't', '2'),
    mluc = IccTag('m', 'l', 'u', 'c'),
    mpet = IccTag('m', 'p', 'e', 't'),
    mAB_ = IccTag('m', 'A', 'B', ' '),
    mBA_ = IccTag('m', 'B', 'A', ' '),
    chad = IccTag('c', 'h', 'a', 'd'),
    cicp = IccTag('c', 'i', 'c', 'p'),
    gamt = IccTag('g', 'a', 'm', 't'),
    sf32 = IccTag('s', 'f', '3', '2'),

    // Apple extensions for ICCv2:
    aarg = IccTag('a', 'a', 'r', 'g'),
    aagg = IccTag('a', 'a', 'g', 'g'),
    aabg = IccTag('a', 'a', 'b', 'g'),
};

} // namespace QIcc

namespace QIcc {

struct TagTableEntry
{
    quint32_be signature;
    quint32_be offset;
    quint32_be size;
};

struct GenericTagData {
    quint32_be type;
    quint32_be null;
};

struct XYZTagData : GenericTagData {
    qint32_be fixedX;
    qint32_be fixedY;
    qint32_be fixedZ;
};

struct CurvTagData : GenericTagData {
    quint32_be valueCount;
    // followed by curv values: quint16_be[]
};

struct ParaTagData : GenericTagData {
    quint16_be curveType;
    quint16_be null2;
    // followed by parameter values: quint32_be[1-7];
};

struct DescTagData : GenericTagData {
    quint32_be asciiDescriptionLength;
    // followed by ascii description: char[]
    // .. we ignore the rest
};

struct MlucTagRecord {
    quint16_be languageCode;
    quint16_be countryCode;
    quint32_be size;
    quint32_be offset;
};

struct MlucTagData : GenericTagData {
    quint32_be recordCount;
    quint32_be recordSize; // = sizeof(MlucTagRecord)
    MlucTagRecord records[1];
};

struct Lut8TagData : GenericTagData {
    quint8 inputChannels;
    quint8 outputChannels;
    quint8 clutGridPoints;
    quint8 padding;
    qint32_be e1;
    qint32_be e2;
    qint32_be e3;
    qint32_be e4;
    qint32_be e5;
    qint32_be e6;
    qint32_be e7;
    qint32_be e8;
    qint32_be e9;
    // followed by parameter values: quint8[inputChannels * 256];
    // followed by parameter values: quint8[outputChannels * clutGridPoints^inputChannels];
    // followed by parameter values: quint8[outputChannels * 256];
};

struct Lut16TagData : GenericTagData {
    quint8 inputChannels;
    quint8 outputChannels;
    quint8 clutGridPoints;
    quint8 padding;
    qint32_be e1;
    qint32_be e2;
    qint32_be e3;
    qint32_be e4;
    qint32_be e5;
    qint32_be e6;
    qint32_be e7;
    qint32_be e8;
    qint32_be e9;
    quint16_be inputTableEntries;
    quint16_be outputTableEntries;
    // followed by parameter values: quint16_be[inputChannels * inputTableEntries];
    // followed by parameter values: quint16_be[outputChannels * clutGridPoints^inputChannels];
    // followed by parameter values: quint16_be[outputChannels * outputTableEntries];
};

// For both mAB and mBA
struct mABTagData : GenericTagData {
    quint8 inputChannels;
    quint8 outputChannels;
    quint8 padding[2];
    quint32_be bCurvesOffset;
    quint32_be matrixOffset;
    quint32_be mCurvesOffset;
    quint32_be clutOffset;
    quint32_be aCurvesOffset;
    // followed by embedded data for the offsets above
};

struct mpetTagData : GenericTagData {
    quint16_be inputChannels;
    quint16_be outputChannels;
    quint32_be processingElements;
    // element offset table
    // element data
};

struct Sf32TagData : GenericTagData {
    quint32_be value[9];
};

struct CicpTagData : GenericTagData {
    quint8 colorPrimaries;
    quint8 transferCharacteristics;
    quint8 matrixCoefficients;
    quint8 videoFullRangeFlag;
};

struct MatrixElement {
    qint32_be e0;
    qint32_be e1;
    qint32_be e2;
    qint32_be e3;
    qint32_be e4;
    qint32_be e5;
    qint32_be e6;
    qint32_be e7;
    qint32_be e8;
    qint32_be e9;
    qint32_be e10;
    qint32_be e11;
};

static int toFixedS1516(float x)
{
    if (x < float(SHRT_MIN))
        return INT_MIN;
    if (x > float(SHRT_MAX))
        return INT_MAX;
    return qRound(x * 65536.0f);
}

static float fromFixedS1516(int x)
{
    return x * (1.0f / 65536.0f);
}

static bool isValidIccProfile(const ICCProfileHeader &header)
{
    if (header.signature != uint(Tag::acsp)) {
        qCWarning(lcIcc, "Failed ICC signature test");
        return false;
    }

    // Don't overflow 32bit integers:
    if (header.tagCount >= (INT32_MAX - sizeof(ICCProfileHeader)) / sizeof(TagTableEntry)) {
        qCWarning(lcIcc, "Failed tag count sanity");
        return false;
    }
    if (header.profileSize - sizeof(ICCProfileHeader) < header.tagCount * sizeof(TagTableEntry)) {
        qCWarning(lcIcc, "Failed basic size sanity");
        return false;
    }

    if (header.profileClass != uint(ProfileClass::Input)
        && header.profileClass != uint(ProfileClass::Display)
        && header.profileClass != uint(ProfileClass::Output)
        && header.profileClass != uint(ProfileClass::ColorSpace))  {
        qCInfo(lcIcc, "Unsupported ICC profile class 0x%x", quint32(header.profileClass));
        return false;
    }
    if (header.inputColorSpace != uint(ColorSpaceType::Rgb)
        && header.inputColorSpace != uint(ColorSpaceType::Gray)
        && header.inputColorSpace != uint(ColorSpaceType::Cmyk)) {
        qCInfo(lcIcc, "Unsupported ICC input color space 0x%x", quint32(header.inputColorSpace));
        return false;
    }
    if (header.pcs != uint(Tag::XYZ_) && header.pcs != uint(Tag::Lab_)) {
        qCInfo(lcIcc, "Invalid ICC profile connection space 0x%x", quint32(header.pcs));
        return false;
    }

    QColorVector illuminant;
    illuminant.x = fromFixedS1516(header.illuminantXyz[0]);
    illuminant.y = fromFixedS1516(header.illuminantXyz[1]);
    illuminant.z = fromFixedS1516(header.illuminantXyz[2]);
    if (illuminant != QColorVector::D50()) {
        qCWarning(lcIcc, "Invalid ICC illuminant");
        return false;
    }

    return true;
}

static int writeColorTrc(QDataStream &stream, const QColorTrc &trc)
{
    if (trc.isIdentity()) {
        stream << uint(Tag::curv) << uint(0);
        stream << uint(0);
        return 12;
    }

    if (trc.m_type == QColorTrc::Type::ParameterizedFunction) {
        const QColorTransferFunction &fun = trc.m_fun;
        stream << uint(Tag::para) << uint(0);
        if (fun.isGamma()) {
            stream << ushort(0) << ushort(0);
            stream << toFixedS1516(fun.m_g);
            return 12 + 4;
        }
        bool type3 = qFuzzyIsNull(fun.m_e) && qFuzzyIsNull(fun.m_f);
        stream << ushort(type3 ? 3 : 4) << ushort(0);
        stream << toFixedS1516(fun.m_g);
        stream << toFixedS1516(fun.m_a);
        stream << toFixedS1516(fun.m_b);
        stream << toFixedS1516(fun.m_c);
        stream << toFixedS1516(fun.m_d);
        if (type3)
            return 12 + 5 * 4;
        stream << toFixedS1516(fun.m_e);
        stream << toFixedS1516(fun.m_f);
        return 12 + 7 * 4;
    }
    if (trc.m_type != QColorTrc::Type::Table) {
        stream << uint(Tag::curv) << uint(0);
        stream << uint(16);
        for (uint i = 0; i < 16; ++i) {
            stream << ushort(qBound(0, qRound(trc.apply(i / 15.f) * 65535.f), 65535));
        }
        return 12 + 16 * 2;
    }

    Q_ASSERT(trc.m_type == QColorTrc::Type::Table);
    stream << uint(Tag::curv) << uint(0);
    stream << uint(trc.m_table.m_tableSize);
    if (!trc.m_table.m_table16.isEmpty()) {
        for (uint i = 0; i < trc.m_table.m_tableSize; ++i) {
            stream << ushort(trc.m_table.m_table16[i]);
        }
    } else {
        for (uint i = 0; i < trc.m_table.m_tableSize; ++i) {
            stream << ushort(trc.m_table.m_table8[i] * 257U);
        }
    }
    if (trc.m_table.m_tableSize & 1) {
        stream << ushort(0);
        return 12 + 2 * trc.m_table.m_tableSize + 2;
    }
    return 12 + 2 * trc.m_table.m_tableSize;
}

// very simple version for small values (<=4) of exp.
static constexpr qsizetype intPow(qsizetype x, qsizetype exp)
{
    return (exp <= 1) ? x : x * intPow(x, exp - 1);
}

struct ElementCombo
{
    const QColorMatrix *inMatrix = nullptr;
    const QColorSpacePrivate::TransferElement *inTable = nullptr;
    const QColorCLUT *clut = nullptr;
    const QColorSpacePrivate::TransferElement *midTable = nullptr;
    const QColorMatrix *midMatrix = nullptr;
    const QColorVector *midOffset = nullptr;
    const QColorSpacePrivate::TransferElement *outTable = nullptr;
};

static void visitElement(ElementCombo &combo, const QColorSpacePrivate::TransferElement &element, int number,
                         const QList<QColorSpacePrivate::Element> &list)
{
    if (number == 0)
        combo.inTable = &element;
    else if (number == list.size() - 1)
        combo.outTable = &element;
    else if (number == 1 && combo.inMatrix)
        combo.inTable = &element;
    else
        combo.midTable = &element;
}

static void visitElement(ElementCombo &combo, const QColorMatrix &element, int number,
                         const QList<QColorSpacePrivate::Element> &)
{
    if (number == 0)
        combo.inMatrix = &element;
    else
        combo.midMatrix = &element;
}

static void visitElement(ElementCombo &combo, const QColorVector &element, int,
                         const QList<QColorSpacePrivate::Element> &)
{
    combo.midOffset = &element;
}

static void visitElement(ElementCombo &combo, const QColorCLUT &element, int,
                         const QList<QColorSpacePrivate::Element> &)
{
    combo.clut = &element;
}

static bool isTableTrc(const QColorSpacePrivate::TransferElement *transfer)
{
    int i = 0;
    while (i < 4 && transfer->trc[i].isValid()) {
        if (transfer->trc[i].m_type != QColorTrc::Type::Table)
            return false;
        i++;
    }
    return i > 0;
}

static bool isTableTrcSingleSize(const QColorSpacePrivate::TransferElement *transfer)
{
    Q_ASSERT(transfer->trc[0].m_type == QColorTrc::Type::Table);
    int i = 1;
    const uint32_t size = transfer->trc[0].table().m_tableSize;
    while (i < 4 && transfer->trc[i].isValid()) {
        if (transfer->trc[i].table().m_tableSize != size)
            return false;
        i++;
    }
    return true;
}

static int writeMab(QDataStream &stream, const QList<QColorSpacePrivate::Element> &abList, bool isAb, bool pcsLab, bool isCmyk)
{
    int number = 0;
    ElementCombo combo;
    for (auto &&element : abList)
        std::visit([&](auto &&elm) { visitElement(combo, elm, number++, abList); }, element);

    Q_ASSERT(!(combo.inMatrix && combo.midMatrix));

    // qWarning() << Q_FUNC_INFO << bool(combo.inMatrix) << bool(combo.inTable) << bool(combo.clut) << bool(combo.midTable) << bool(combo.midMatrix) << bool(combo.midOffset) << bool(combo.outTable);
    bool lut16 = true;
    if (combo.midMatrix || combo.midTable || combo.midOffset)
        lut16 = false;
    if (combo.clut && (combo.clut->gridPointsX != combo.clut->gridPointsY ||
                       combo.clut->gridPointsX != combo.clut->gridPointsZ ||
                       (combo.clut->gridPointsW > 1 && combo.clut->gridPointsX != combo.clut->gridPointsW)))
        lut16 = false;
    if (lut16 && combo.inTable)
        lut16 = isTableTrc(combo.inTable) && isTableTrcSingleSize(combo.inTable);
    if (lut16 && combo.outTable)
        lut16 = isTableTrc(combo.outTable) && isTableTrcSingleSize(combo.outTable);

    if (!lut16) {
        if (combo.inMatrix)
            qSwap(combo.inMatrix, combo.midMatrix);
        if (isAb)
            stream << uint(Tag::mAB_) << uint(0);
        else
            stream << uint(Tag::mBA_) << uint(0);
    } else {
        stream << uint(Tag::mft2) << uint(0);
    }

    const int inChannels = (isCmyk && isAb) ? 4 : 3;
    const int outChannels =  (isCmyk && !isAb) ? 4 : 3;
    stream << uchar(inChannels) << uchar(outChannels);
    qsizetype gridPointsLut16 = 0;
    if (lut16 && combo.clut)
        gridPointsLut16 = combo.clut->gridPointsX;
    if (lut16)
        stream << uchar(gridPointsLut16) << uchar(0);
    else
        stream << quint16(0);
    if (lut16) {
        if (combo.inMatrix) {
            stream << toFixedS1516(combo.inMatrix->r.x);
            stream << toFixedS1516(combo.inMatrix->g.x);
            stream << toFixedS1516(combo.inMatrix->b.x);
            stream << toFixedS1516(combo.inMatrix->r.y);
            stream << toFixedS1516(combo.inMatrix->g.y);
            stream << toFixedS1516(combo.inMatrix->b.y);
            stream << toFixedS1516(combo.inMatrix->r.z);
            stream << toFixedS1516(combo.inMatrix->g.z);
            stream << toFixedS1516(combo.inMatrix->b.z);
        } else {
            stream << toFixedS1516(1.0f);
            stream << toFixedS1516(0.0f);
            stream << toFixedS1516(0.0f);
            stream << toFixedS1516(0.0f);
            stream << toFixedS1516(1.0f);
            stream << toFixedS1516(0.0f);
            stream << toFixedS1516(0.0f);
            stream << toFixedS1516(0.0f);
            stream << toFixedS1516(1.0f);
        }
        int inputEntries = 0, outputEntries = 0;
        if (combo.inTable)
            inputEntries = combo.inTable->trc[0].table().m_tableSize;
        else
            inputEntries = 2;
        if (combo.outTable)
            outputEntries = combo.outTable->trc[0].table().m_tableSize;
        else
            outputEntries = 2;
        stream << quint16(inputEntries);
        stream << quint16(outputEntries);
        auto writeTable = [&](const QColorSpacePrivate::TransferElement *table, int entries, int channels) {
            if (table) {
                for (int j = 0; j < channels; ++j) {
                    if (!table->trc[j].table().m_table16.isEmpty()) {
                        for (int i = 0; i < entries; ++i)
                            stream << table->trc[j].table().m_table16[i];
                    } else {
                        for (int i = 0; i < entries; ++i)
                            stream << quint16(table->trc[j].table().m_table8[i] * 257);
                    }
                }
            } else {
                for (int j = 0; j < channels; ++j)
                    stream << quint16(0) << quint16(65535);
            }
        };

        writeTable(combo.inTable, inputEntries, inChannels);

        if (combo.clut) {
            if (isAb && pcsLab) {
                for (const QColorVector &v : combo.clut->table) {
                    stream << quint16(v.x * 65280.0f + 0.5f);
                    stream << quint16(v.y * 65280.0f + 0.5f);
                    stream << quint16(v.z * 65280.0f + 0.5f);
                }
            } else {
                if (outChannels == 4) {
                    for (const QColorVector &v : combo.clut->table) {
                        stream << quint16(v.x * 65535.0f + 0.5f);
                        stream << quint16(v.y * 65535.0f + 0.5f);
                        stream << quint16(v.z * 65535.0f + 0.5f);
                        stream << quint16(v.w * 65535.0f + 0.5f);
                    }
                } else {
                    for (const QColorVector &v : combo.clut->table) {
                        stream << quint16(v.x * 65535.0f + 0.5f);
                        stream << quint16(v.y * 65535.0f + 0.5f);
                        stream << quint16(v.z * 65535.0f + 0.5f);
                    }
                }
            }
        }

        writeTable(combo.outTable, outputEntries, outChannels);

        qsizetype offset = sizeof(Lut16TagData) + 2 * inChannels * inputEntries
                                                + 2 * outChannels * outputEntries
                                                + 2 * outChannels * intPow(gridPointsLut16, inChannels);
        if (offset & 0x2) {
            stream << quint16(0);
            offset += 2;
        }
        return offset;
    } else {
        // mAB/mBA tag:
        if (isAb) {
            if (!combo.clut && combo.inTable && combo.midMatrix && !combo.midTable)
                std::swap(combo.inTable, combo.midTable);
        } else {
            if (!combo.clut && combo.outTable && combo.midMatrix && !combo.midTable)
                std::swap(combo.outTable, combo.midTable);
        }
        quint32 offset = sizeof(mABTagData);
        QBuffer buffer2;
        buffer2.open(QIODevice::WriteOnly);
        QDataStream stream2(&buffer2);
        quint32 bOffset = offset;
        quint32 matrixOffset = 0;
        quint32 mOffset = 0;
        quint32 clutOffset = 0;
        quint32 aOffset = 0;
        // Tags must start on 4 byte offsets, but sampled curves might have sizes 2-byte aligned
        auto alignTag = [&]() {
            if (offset & 0x2) {
                stream2 << quint16(0);
                offset += 2;
            }
        };

        const QColorSpacePrivate::TransferElement *aCurve, *bCurve;
        int aChannels;
        if (isAb) {
            aCurve = combo.inTable;
            aChannels = inChannels;
            bCurve = combo.outTable;
            Q_ASSERT(outChannels == 3);
        } else {
            aCurve = combo.outTable;
            aChannels = outChannels;
            bCurve = combo.inTable;
            Q_ASSERT(inChannels == 3);
        }
        if (bCurve) {
            offset += writeColorTrc(stream2, bCurve->trc[0]);
            alignTag();
            offset += writeColorTrc(stream2, bCurve->trc[1]);
            alignTag();
            offset += writeColorTrc(stream2, bCurve->trc[2]);
            alignTag();
        } else {
            stream2 << uint(Tag::curv) << uint(0) << uint(0);
            stream2 << uint(Tag::curv) << uint(0) << uint(0);
            stream2 << uint(Tag::curv) << uint(0) << uint(0);
            offset += 12 * 3;
        }
        if (combo.midMatrix || combo.midOffset || combo.midTable) {
            matrixOffset = offset;
            if (combo.midMatrix) {
                stream2 << toFixedS1516(combo.midMatrix->r.x);
                stream2 << toFixedS1516(combo.midMatrix->g.x);
                stream2 << toFixedS1516(combo.midMatrix->b.x);
                stream2 << toFixedS1516(combo.midMatrix->r.y);
                stream2 << toFixedS1516(combo.midMatrix->g.y);
                stream2 << toFixedS1516(combo.midMatrix->b.y);
                stream2 << toFixedS1516(combo.midMatrix->r.z);
                stream2 << toFixedS1516(combo.midMatrix->g.z);
                stream2 << toFixedS1516(combo.midMatrix->b.z);
            } else {
                stream2 << toFixedS1516(1.0f);
                stream2 << toFixedS1516(0.0f);
                stream2 << toFixedS1516(0.0f);
                stream2 << toFixedS1516(0.0f);
                stream2 << toFixedS1516(1.0f);
                stream2 << toFixedS1516(0.0f);
                stream2 << toFixedS1516(0.0f);
                stream2 << toFixedS1516(0.0f);
                stream2 << toFixedS1516(1.0f);
            }
            if (combo.midOffset) {
                stream2 << toFixedS1516(combo.midOffset->x);
                stream2 << toFixedS1516(combo.midOffset->y);
                stream2 << toFixedS1516(combo.midOffset->z);
            } else {
                stream2 << toFixedS1516(0.0f);
                stream2 << toFixedS1516(0.0f);
                stream2 << toFixedS1516(0.0f);
            }
            offset += 12 * 4;
            mOffset = offset;
            if (combo.midTable) {
                offset += writeColorTrc(stream2, combo.midTable->trc[0]);
                alignTag();
                offset += writeColorTrc(stream2, combo.midTable->trc[1]);
                alignTag();
                offset += writeColorTrc(stream2, combo.midTable->trc[2]);
                alignTag();
            } else {
                stream2 << uint(Tag::curv) << uint(0) << uint(0);
                stream2 << uint(Tag::curv) << uint(0) << uint(0);
                stream2 << uint(Tag::curv) << uint(0) << uint(0);
                offset += 12 * 3;
            }
        }
        if (combo.clut || aCurve) {
            clutOffset = offset;
            if (combo.clut) {
                stream2 << uchar(combo.clut->gridPointsX);
                stream2 << uchar(combo.clut->gridPointsY);
                stream2 << uchar(combo.clut->gridPointsZ);
                if (inChannels == 4)
                    stream2 << uchar(combo.clut->gridPointsW);
                else
                    stream2 << uchar(0);
                for (int i = 0; i < 12; ++i)
                    stream2 << uchar(0);
                stream2 << uchar(2) << uchar(0) << uchar(0) << uchar(0);
                offset += 20;
                if (outChannels == 4) {
                    for (const QColorVector &v : combo.clut->table) {
                        stream2 << quint16(v.x * 65535.0f + 0.5f);
                        stream2 << quint16(v.y * 65535.0f + 0.5f);
                        stream2 << quint16(v.z * 65535.0f + 0.5f);
                        stream2 << quint16(v.w * 65535.0f + 0.5f);
                    }
                } else {
                    for (const QColorVector &v : combo.clut->table) {
                        stream2 << quint16(v.x * 65535.0f + 0.5f);
                        stream2 << quint16(v.y * 65535.0f + 0.5f);
                        stream2 << quint16(v.z * 65535.0f + 0.5f);
                    }
                }
                offset += 2 * outChannels * combo.clut->table.size();
                alignTag();
            } else {
                for (int i = 0; i < 16; ++i)
                    stream2 << uchar(0);
                stream2 << uchar(1) << uchar(0) << uchar(0) << uchar(0);
                offset += 20;
            }
            aOffset = offset;
            if (aCurve) {
                offset += writeColorTrc(stream2, aCurve->trc[0]);
                alignTag();
                offset += writeColorTrc(stream2, aCurve->trc[1]);
                alignTag();
                offset += writeColorTrc(stream2, aCurve->trc[2]);
                alignTag();
                if (aChannels == 4) {
                    offset += writeColorTrc(stream2, aCurve->trc[3]);
                    alignTag();
                }
            } else {
                stream2 << uint(Tag::curv) << uint(0) << uint(0);
                stream2 << uint(Tag::curv) << uint(0) << uint(0);
                stream2 << uint(Tag::curv) << uint(0) << uint(0);
                if (aChannels == 4)
                    stream2 << uint(Tag::curv) << uint(0) << uint(0);
                offset += 12 * aChannels;
            }
        }
        buffer2.close();
        QByteArray tagData = buffer2.buffer();
        stream << quint32(bOffset);
        stream << quint32(matrixOffset);
        stream << quint32(mOffset);
        stream << quint32(clutOffset);
        stream << quint32(aOffset);
        stream.writeRawData(tagData.data(), tagData.size());

        return int(sizeof(mABTagData) + tagData.size());
    }
}

QByteArray toIccProfile(const QColorSpace &space)
{
    if (!space.isValid())
        return QByteArray();

    const QColorSpacePrivate *spaceDPtr = QColorSpacePrivate::get(space);
    if (!spaceDPtr->iccProfile.isEmpty())
        return spaceDPtr->iccProfile;

    int fixedLengthTagCount = 5;
    if (!spaceDPtr->isThreeComponentMatrix())
        fixedLengthTagCount = 2;
    else if (spaceDPtr->colorModel == QColorSpace::ColorModel::Gray)
        fixedLengthTagCount = 2;
    bool writeChad = false;
    bool writeB2a = true;
    bool writeCicp = false;
    if (spaceDPtr->isThreeComponentMatrix() && !spaceDPtr->chad.isIdentity()) {
        writeChad = true;
        fixedLengthTagCount++;
    }
    int varLengthTagCount = 4;
    if (!spaceDPtr->isThreeComponentMatrix())
        varLengthTagCount = 3;
    else if (spaceDPtr->colorModel == QColorSpace::ColorModel::Gray)
        varLengthTagCount = 2;

    if (!space.isValidTarget()) {
        writeB2a = false;
        Q_ASSERT(!spaceDPtr->isThreeComponentMatrix());
        varLengthTagCount--;
    }
    switch (spaceDPtr->transferFunction) {
    case QColorSpace::TransferFunction::St2084:
    case QColorSpace::TransferFunction::Hlg:
        writeCicp = true;
        fixedLengthTagCount++;
        break;
    default:
        break;
    }

    const int tagCount = fixedLengthTagCount + varLengthTagCount;
    const uint profileDataOffset = 128 + 4 + 12 * tagCount;
    uint variableTagTableOffsets = 128 + 4 + 12 * fixedLengthTagCount;

    uint currentOffset = 0;
    uint rTrcOffset = 0;
    uint gTrcOffset = 0;
    uint bTrcOffset = 0;
    uint kTrcOffset = 0;
    uint rTrcSize = 0;
    uint gTrcSize = 0;
    uint bTrcSize = 0;
    uint kTrcSize = 0;
    uint descOffset = 0;
    uint descSize = 0;
    uint mA2bOffset = 0;
    uint mB2aOffset = 0;
    uint mA2bSize = 0;
    uint mB2aSize = 0;

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream stream(&buffer);

    // Profile header:
    stream << uint(0); // Size, we will update this later
    stream << uint(0);
    stream << uint(0x04400000); // Version 4.4
    stream << uint(ProfileClass::Display);
    switch (spaceDPtr->colorModel) {
    case QColorSpace::ColorModel::Rgb:
        stream << uint(ColorSpaceType::Rgb);
        break;
    case QColorSpace::ColorModel::Gray:
        stream << uint(ColorSpaceType::Gray);
        break;
    case QColorSpace::ColorModel::Cmyk:
        stream << uint(ColorSpaceType::Cmyk);
        break;
    case QColorSpace::ColorModel::Undefined:
        Q_UNREACHABLE();
    }
    stream << (spaceDPtr->isPcsLab ? uint(Tag::Lab_) : uint(Tag::XYZ_));
    stream << uint(0) << uint(0) << uint(0);
    stream << uint(Tag::acsp);
    stream << uint(0) << uint(0) << uint(0);
    stream << uint(0) << uint(0) << uint(0);
    stream << uint(0); // Rendering intent
    stream << uint(0x0000f6d6); // D50 X
    stream << uint(0x00010000); // D50 Y
    stream << uint(0x0000d32d); // D50 Z
    stream << IccTag('Q','t', QT_VERSION_MAJOR, QT_VERSION_MINOR);
    stream << uint(0) << uint(0) << uint(0) << uint(0);
    stream << uint(0) << uint(0) << uint(0) << uint(0) << uint(0) << uint(0) << uint(0);

    currentOffset = profileDataOffset;
    if (spaceDPtr->isThreeComponentMatrix()) {
        // Tag table:
        stream << uint(tagCount);
        if (spaceDPtr->colorModel == QColorSpace::ColorModel::Rgb) {
            stream << uint(Tag::rXYZ) << uint(currentOffset + 00) << uint(20);
            stream << uint(Tag::gXYZ) << uint(currentOffset + 20) << uint(20);
            stream << uint(Tag::bXYZ) << uint(currentOffset + 40) << uint(20);
            currentOffset += 20 + 20 + 20;
        }
        stream << uint(Tag::wtpt) << uint(currentOffset + 00) << uint(20);
        stream << uint(Tag::cprt) << uint(currentOffset + 20) << uint(34);
        currentOffset += 20 + 34 + 2;
        if (writeChad) {
            stream << uint(Tag::chad) << uint(currentOffset) << uint(44);
            currentOffset += 44;
        }
        if (writeCicp) {
            stream << uint(Tag::cicp) << uint(currentOffset) << uint(12);
            currentOffset += 12;
        }
        // From here the offset and size will be updated later:
        if (spaceDPtr->colorModel == QColorSpace::ColorModel::Rgb) {
            stream << uint(Tag::rTRC) << uint(0) << uint(0);
            stream << uint(Tag::gTRC) << uint(0) << uint(0);
            stream << uint(Tag::bTRC) << uint(0) << uint(0);
        } else {
            stream << uint(Tag::kTRC) << uint(0) << uint(0);
        }
        stream << uint(Tag::desc) << uint(0) << uint(0);

        // Tag data:
        if (spaceDPtr->colorModel == QColorSpace::ColorModel::Rgb) {
            stream << uint(Tag::XYZ_) << uint(0);
            stream << toFixedS1516(spaceDPtr->toXyz.r.x);
            stream << toFixedS1516(spaceDPtr->toXyz.r.y);
            stream << toFixedS1516(spaceDPtr->toXyz.r.z);
            stream << uint(Tag::XYZ_) << uint(0);
            stream << toFixedS1516(spaceDPtr->toXyz.g.x);
            stream << toFixedS1516(spaceDPtr->toXyz.g.y);
            stream << toFixedS1516(spaceDPtr->toXyz.g.z);
            stream << uint(Tag::XYZ_) << uint(0);
            stream << toFixedS1516(spaceDPtr->toXyz.b.x);
            stream << toFixedS1516(spaceDPtr->toXyz.b.y);
            stream << toFixedS1516(spaceDPtr->toXyz.b.z);
        }
        stream << uint(Tag::XYZ_) << uint(0);
        stream << toFixedS1516(spaceDPtr->whitePoint.x);
        stream << toFixedS1516(spaceDPtr->whitePoint.y);
        stream << toFixedS1516(spaceDPtr->whitePoint.z);
        stream << uint(Tag::mluc) << uint(0);
        stream << uint(1) << uint(12);
        stream << uchar('e') << uchar('n') << uchar('U') << uchar('S');
        stream << uint(6) << uint(28);
        stream << ushort('N') << ushort('/') << ushort('A');
        stream << ushort(0); // 4-byte alignment
        if (writeChad) {
            const QColorMatrix &chad = spaceDPtr->chad;
            stream << uint(Tag::sf32) << uint(0);
            stream << toFixedS1516(chad.r.x);
            stream << toFixedS1516(chad.g.x);
            stream << toFixedS1516(chad.b.x);
            stream << toFixedS1516(chad.r.y);
            stream << toFixedS1516(chad.g.y);
            stream << toFixedS1516(chad.b.y);
            stream << toFixedS1516(chad.r.z);
            stream << toFixedS1516(chad.g.z);
            stream << toFixedS1516(chad.b.z);
        }
        if (writeCicp) {
            stream << uint(Tag::cicp) << uint(0);
            stream << uchar(2); // Unspecified primaries
            if (spaceDPtr->transferFunction == QColorSpace::TransferFunction::St2084)
                stream << uchar(16);
            else if (spaceDPtr->transferFunction == QColorSpace::TransferFunction::Hlg)
                stream << uchar(18);
            else
                Q_UNREACHABLE();
            stream << uchar(0); // Only for YCbCr, otherwise 0
            stream << uchar(1); // Video full range
        }

        // From now on the data is variable sized:
        if (spaceDPtr->colorModel == QColorSpace::ColorModel::Rgb) {
            rTrcOffset = currentOffset;
            rTrcSize = writeColorTrc(stream, spaceDPtr->trc[0]);
            currentOffset += rTrcSize;
            if (spaceDPtr->trc[0] == spaceDPtr->trc[1]) {
                gTrcOffset = rTrcOffset;
                gTrcSize = rTrcSize;
            } else {
                gTrcOffset = currentOffset;
                gTrcSize = writeColorTrc(stream, spaceDPtr->trc[1]);
                currentOffset += gTrcSize;
            }
            if (spaceDPtr->trc[0] == spaceDPtr->trc[2]) {
                bTrcOffset = rTrcOffset;
                bTrcSize = rTrcSize;
            } else {
                bTrcOffset = currentOffset;
                bTrcSize = writeColorTrc(stream, spaceDPtr->trc[2]);
                currentOffset += bTrcSize;
            }
        } else {
            Q_ASSERT(spaceDPtr->colorModel == QColorSpace::ColorModel::Gray);
            kTrcOffset = currentOffset;
            kTrcSize = writeColorTrc(stream, spaceDPtr->trc[0]);
            currentOffset += kTrcSize;
        }
    } else {
        // Tag table:
        stream << uint(tagCount);
        stream << uint(Tag::wtpt) << uint(profileDataOffset + 00) << uint(20);
        stream << uint(Tag::cprt) << uint(profileDataOffset + 20) << uint(34);
        currentOffset += 20 + 34 + 2;
        // From here the offset and size will be updated later:
        stream << uint(Tag::A2B0) << uint(0) << uint(0);
        if (writeB2a)
            stream << uint(Tag::B2A0) << uint(0) << uint(0);
        stream << uint(Tag::desc) << uint(0) << uint(0);

        // Fixed tag data
        stream << uint(Tag::XYZ_) << uint(0);
        stream << toFixedS1516(spaceDPtr->whitePoint.x);
        stream << toFixedS1516(spaceDPtr->whitePoint.y);
        stream << toFixedS1516(spaceDPtr->whitePoint.z);
        stream << uint(Tag::mluc) << uint(0);
        stream << uint(1) << uint(12);
        stream << uchar('e') << uchar('n') << uchar('U') << uchar('S');
        stream << uint(6) << uint(28);
        stream << ushort('N') << ushort('/') << ushort('A');
        stream << ushort(0); // 4-byte alignment

        // From now on the data is variable sized:
        mA2bOffset = currentOffset;
        mA2bSize = writeMab(stream, spaceDPtr->mAB, true, spaceDPtr->isPcsLab, spaceDPtr->colorModel == QColorSpace::ColorModel::Cmyk);
        currentOffset += mA2bSize;
        if (writeB2a) {
            mB2aOffset = currentOffset;
            mB2aSize = writeMab(stream, spaceDPtr->mBA, false, spaceDPtr->isPcsLab, spaceDPtr->colorModel == QColorSpace::ColorModel::Cmyk);
            currentOffset += mB2aSize;
        }
    }

    // Writing description
    descOffset = currentOffset;
    const QString description = space.description();
    stream << uint(Tag::mluc) << uint(0);
    stream << uint(1) << uint(12);
    stream << uchar('e') << uchar('n') << uchar('U') << uchar('S');
    stream << uint(description.size() * 2) << uint(28);
    for (QChar ch : description)
        stream << ushort(ch.unicode());
    descSize = 28 + description.size() * 2;
    if (description.size() & 1) {
        stream << ushort(0);
        currentOffset += 2;
    }
    currentOffset += descSize;

    buffer.close();
    QByteArray iccProfile = buffer.buffer();
    // Now write final size
    *(quint32_be *)iccProfile.data() = iccProfile.size();
    // And the final indices and sizes of variable size tags:
    if (spaceDPtr->isThreeComponentMatrix()) {
        if (spaceDPtr->colorModel == QColorSpace::ColorModel::Rgb) {
            *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 4) = rTrcOffset;
            *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 8) = rTrcSize;
            *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 12 + 4) = gTrcOffset;
            *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 12 + 8) = gTrcSize;
            *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 2 * 12 + 4) = bTrcOffset;
            *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 2 * 12 + 8) = bTrcSize;
            *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 3 * 12 + 4) = descOffset;
            *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 3 * 12 + 8) = descSize;
        } else {
            *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 4) = kTrcOffset;
            *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 8) = kTrcSize;
            *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 12 + 4) = descOffset;
            *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 12 + 8) = descSize;
        }
    } else {
        *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 4) = mA2bOffset;
        *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 8) = mA2bSize;
        variableTagTableOffsets += 12;
        if (writeB2a) {
            *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 4) = mB2aOffset;
            *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 8) = mB2aSize;
            variableTagTableOffsets += 12;
        }
        *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 4) = descOffset;
        *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 8) = descSize;
    }

#if !defined(QT_NO_DEBUG) || defined(QT_FORCE_ASSERTS)
    const ICCProfileHeader *iccHeader = (const ICCProfileHeader *)iccProfile.constData();
    Q_ASSERT(qsizetype(iccHeader->profileSize) == qsizetype(iccProfile.size()));
    Q_ASSERT(isValidIccProfile(*iccHeader));
#endif

    return iccProfile;
}

struct TagEntry {
    quint32 offset;
    quint32 size;
};

static bool parseXyzData(const QByteArray &data, const TagEntry &tagEntry, QColorVector &colorVector)
{
    if (tagEntry.size < sizeof(XYZTagData)) {
        qCWarning(lcIcc) << "Undersized XYZ tag";
        return false;
    }
    const XYZTagData xyz = qFromUnaligned<XYZTagData>(data.constData() + tagEntry.offset);
    if (xyz.type != quint32(Tag::XYZ_)) {
        qCWarning(lcIcc) << "Bad XYZ content type";
        return false;
    }
    const float x = fromFixedS1516(xyz.fixedX);
    const float y = fromFixedS1516(xyz.fixedY);
    const float z = fromFixedS1516(xyz.fixedZ);

    colorVector = QColorVector(x, y, z);
    return true;
}

static quint32 parseTRC(const QByteArrayView &tagData, QColorTrc &gamma, QColorTransferTable::Type type = QColorTransferTable::TwoWay)
{
    if (tagData.size() < 12)
        return 0;
    const GenericTagData trcData = qFromUnaligned<GenericTagData>(tagData.constData());
    if (trcData.type == quint32(Tag::curv)) {
        Q_STATIC_ASSERT(sizeof(CurvTagData) == 12);
        const CurvTagData curv = qFromUnaligned<CurvTagData>(tagData.constData());
        if (curv.valueCount > (1 << 16)) {
            qCWarning(lcIcc) << "Invalid count in curv table";
            return 0;
        }
        if (tagData.size() < qsizetype(12 + 2 * curv.valueCount)) {
            qCWarning(lcIcc) << "Truncated curv table";
            return 0;
        }
        const auto valueOffset = sizeof(CurvTagData);
        if (curv.valueCount == 0) {
            gamma.m_type = QColorTrc::Type::ParameterizedFunction;
            gamma.m_fun = QColorTransferFunction(); // Linear
        } else if (curv.valueCount == 1) {
            const quint16 v = qFromBigEndian<quint16>(tagData.constData() + valueOffset);
            gamma.m_type = QColorTrc::Type::ParameterizedFunction;
            gamma.m_fun = QColorTransferFunction::fromGamma(v * (1.0f / 256.0f));
        } else {
            QList<quint16> tabl;
            tabl.resize(curv.valueCount);
            static_assert(sizeof(GenericTagData) == 2 * sizeof(quint32_be),
                          "GenericTagData has padding. The following code is a subject to UB.");
            qFromBigEndian<quint16>(tagData.constData() + valueOffset, curv.valueCount, tabl.data());
            QColorTransferTable table(curv.valueCount, tabl, type);
            QColorTransferFunction curve;
            if (!table.checkValidity()) {
                qCWarning(lcIcc) << "Invalid curv table";
                return 0;
            } else if (!table.asColorTransferFunction(&curve)) {
                gamma.m_type = QColorTrc::Type::Table;
                gamma.m_table = table;
            } else {
                qCDebug(lcIcc) << "Detected curv table as function";
                gamma.m_type = QColorTrc::Type::ParameterizedFunction;
                gamma.m_fun = curve;
            }
        }
        return 12 + 2 * curv.valueCount;
    }
    if (trcData.type == quint32(Tag::para)) {
        Q_STATIC_ASSERT(sizeof(ParaTagData) == 12);
        const ParaTagData para = qFromUnaligned<ParaTagData>(tagData.constData());
        const auto parametersOffset = sizeof(ParaTagData);
        quint32 parameters[7];
        switch (para.curveType) {
        case 0: {
            if (tagData.size() < 12 + 1 * 4)
                return 0;
            qFromBigEndian<quint32>(tagData.constData() + parametersOffset, 1, parameters);
            float g = fromFixedS1516(parameters[0]);
            gamma.m_type = QColorTrc::Type::ParameterizedFunction;
            gamma.m_fun = QColorTransferFunction::fromGamma(g);
            return 12 + 1 * 4;
        }
        case 1: {
            if (tagData.size() < 12 + 3 * 4)
                return 0;
            qFromBigEndian<quint32>(tagData.constData() + parametersOffset, 3, parameters);
            if (parameters[1] == 0)
                return 0;
            float g = fromFixedS1516(parameters[0]);
            float a = fromFixedS1516(parameters[1]);
            float b = fromFixedS1516(parameters[2]);
            float d = -b / a;
            gamma.m_type = QColorTrc::Type::ParameterizedFunction;
            gamma.m_fun = QColorTransferFunction(a, b, 0.0f, d, 0.0f, 0.0f, g);
            return 12 + 3 * 4;
        }
        case 2: {
            if (tagData.size() < 12 + 4 * 4)
                return 0;
            qFromBigEndian<quint32>(tagData.constData() + parametersOffset, 4, parameters);
            if (parameters[1] == 0)
                return 0;
            float g = fromFixedS1516(parameters[0]);
            float a = fromFixedS1516(parameters[1]);
            float b = fromFixedS1516(parameters[2]);
            float c = fromFixedS1516(parameters[3]);
            float d = -b / a;
            gamma.m_type = QColorTrc::Type::ParameterizedFunction;
            gamma.m_fun = QColorTransferFunction(a, b, 0.0f, d, c, c, g);
            return 12 + 4 * 4;
        }
        case 3: {
            if (tagData.size() < 12 + 5 * 4)
                return 0;
            qFromBigEndian<quint32>(tagData.constData() + parametersOffset, 5, parameters);
            float g = fromFixedS1516(parameters[0]);
            float a = fromFixedS1516(parameters[1]);
            float b = fromFixedS1516(parameters[2]);
            float c = fromFixedS1516(parameters[3]);
            float d = fromFixedS1516(parameters[4]);
            gamma.m_type = QColorTrc::Type::ParameterizedFunction;
            gamma.m_fun = QColorTransferFunction(a, b, c, d, 0.0f, 0.0f, g);
            return 12 + 5 * 4;
        }
        case 4: {
            if (tagData.size() < 12 + 7 * 4)
                return 0;
            qFromBigEndian<quint32>(tagData.constData() + parametersOffset, 7, parameters);
            float g = fromFixedS1516(parameters[0]);
            float a = fromFixedS1516(parameters[1]);
            float b = fromFixedS1516(parameters[2]);
            float c = fromFixedS1516(parameters[3]);
            float d = fromFixedS1516(parameters[4]);
            float e = fromFixedS1516(parameters[5]);
            float f = fromFixedS1516(parameters[6]);
            gamma.m_type = QColorTrc::Type::ParameterizedFunction;
            gamma.m_fun = QColorTransferFunction(a, b, c, d, e, f, g);
            return 12 + 7 * 4;
        }
        default:
            qCWarning(lcIcc)  << "Unknown para type" << uint(para.curveType);
            return 0;
        }
        return true;
    }
    qCWarning(lcIcc) << "Invalid TRC data type" << Qt::hex << trcData.type;
    return 0;
}

template<typename T>
static void parseCLUT(const T *tableData, const float f, QColorCLUT *clut, uchar outputChannels)
{
    if (outputChannels == 4) {
        for (qsizetype index = 0; index < clut->table.size(); ++index) {
            QColorVector v(tableData[index * 4 + 0] * f,
                           tableData[index * 4 + 1] * f,
                           tableData[index * 4 + 2] * f,
                           tableData[index * 4 + 3] * f);
            clut->table[index] = v;
        };
    } else {
        for (qsizetype index = 0; index < clut->table.size(); ++index) {
            QColorVector v(tableData[index * 3 + 0] * f,
                           tableData[index * 3 + 1] * f,
                           tableData[index * 3 + 2] * f);
            clut->table[index] = v;
        };
    }
}

// Parses lut8 and lut16 type elements
template<typename T>
static bool parseLutData(const QByteArray &data, const TagEntry &tagEntry, QColorSpacePrivate *colorSpacePrivate, bool isAb)
{
    if (tagEntry.size < sizeof(T)) {
        qCWarning(lcIcc) << "Undersized lut8/lut16 tag";
        return false;
    }
    if (qsizetype(tagEntry.size) > data.size()) {
        qCWarning(lcIcc) << "Truncated lut8/lut16 tag";
        return false;
    }
    using S = std::conditional_t<std::is_same_v<T, Lut8TagData>, uint8_t, uint16_t>;
    const T lut = qFromUnaligned<T>(data.constData() + tagEntry.offset);
    int inputTableEntries, outputTableEntries, precision;
    if constexpr (std::is_same_v<T, Lut8TagData>) {
        Q_ASSERT(lut.type == quint32(Tag::mft1));
        if (!colorSpacePrivate->isPcsLab && isAb) {
            qCWarning(lcIcc) << "Lut8 can not output XYZ values";
            return false;
        }
        inputTableEntries = 256;
        outputTableEntries = 256;
        precision = 1;
    } else {
        Q_ASSERT(lut.type == quint32(Tag::mft2));
        inputTableEntries = lut.inputTableEntries;
        outputTableEntries = lut.outputTableEntries;
        if (inputTableEntries < 2 || inputTableEntries > 4096)
            return false;
        if (outputTableEntries < 2 || outputTableEntries > 4096)
            return false;
        precision = 2;
    }

    bool inTableIsLinear = true, outTableIsLinear = true;
    QColorSpacePrivate::TransferElement inTableElement;
    QColorSpacePrivate::TransferElement outTableElement;
    QColorCLUT clutElement;
    QColorMatrix matrixElement;

    matrixElement.r.x = fromFixedS1516(lut.e1);
    matrixElement.g.x = fromFixedS1516(lut.e2);
    matrixElement.b.x = fromFixedS1516(lut.e3);
    matrixElement.r.y = fromFixedS1516(lut.e4);
    matrixElement.g.y = fromFixedS1516(lut.e5);
    matrixElement.b.y = fromFixedS1516(lut.e6);
    matrixElement.r.z = fromFixedS1516(lut.e7);
    matrixElement.g.z = fromFixedS1516(lut.e8);
    matrixElement.b.z = fromFixedS1516(lut.e9);
    if (!colorSpacePrivate->isPcsLab && !isAb && !matrixElement.isValid()) {
        qCWarning(lcIcc) << "Invalid matrix values in lut8/lut16";
        return false;
    }

    const int inputChannels = (isAb && colorSpacePrivate->colorModel == QColorSpace::ColorModel::Cmyk) ? 4 : 3;
    const int outputChannels = (!isAb && colorSpacePrivate->colorModel == QColorSpace::ColorModel::Cmyk) ? 4 : 3;

    if (lut.inputChannels != inputChannels) {
        qCWarning(lcIcc) << "Unsupported lut8/lut16 input channel count" << lut.inputChannels;
        return false;
    }

    if (lut.outputChannels != outputChannels) {
        qCWarning(lcIcc) << "Unsupported lut8/lut16 output channel count" << lut.outputChannels;
        return false;
    }

    const qsizetype clutTableSize = intPow(lut.clutGridPoints, inputChannels);
    if (tagEntry.size < (sizeof(T) + precision * inputChannels * inputTableEntries
                                   + precision * outputChannels * outputTableEntries
                                   + precision * outputChannels * clutTableSize)) {
        qCWarning(lcIcc) << "Undersized lut8/lut16 tag, no room for tables";
        return false;
    }
    if (colorSpacePrivate->colorModel == QColorSpace::ColorModel::Cmyk && clutTableSize == 0) {
        qCWarning(lcIcc) << "Cmyk conversion must have a CLUT";
        return false;
    }

    const uint8_t *tableData = reinterpret_cast<const uint8_t *>(data.constData() + tagEntry.offset + sizeof(T));

    for (int j = 0; j < inputChannels; ++j) {
        QList<S> input(inputTableEntries);
        qFromBigEndian<S>(tableData, inputTableEntries, input.data());
        QColorTransferTable table(inputTableEntries, input, QColorTransferTable::OneWay);
        if (!table.checkValidity()) {
            qCWarning(lcIcc) << "Bad input table in lut8/lut16";
            return false;
        }
        if (!table.isIdentity())
            inTableIsLinear = false;
        inTableElement.trc[j] = std::move(table);
        tableData += inputTableEntries * precision;
    }

    clutElement.table.resize(clutTableSize);
    clutElement.gridPointsX = clutElement.gridPointsY = clutElement.gridPointsZ = lut.clutGridPoints;
    if (inputChannels == 4)
        clutElement.gridPointsW = lut.clutGridPoints;

    if constexpr (std::is_same_v<T, Lut8TagData>) {
        parseCLUT(tableData, 1.f / 255.f, &clutElement, outputChannels);
    } else {
        float f = 1.0f / 65535.f;
        if (colorSpacePrivate->isPcsLab && isAb) // Legacy lut16 conversion to Lab
            f = 1.0f / 65280.f;
        QList<S> clutTable(clutTableSize * outputChannels);
        qFromBigEndian<S>(tableData, clutTable.size(), clutTable.data());
        parseCLUT(clutTable.constData(), f, &clutElement, outputChannels);
    }
    tableData += clutTableSize * outputChannels * precision;

    for (int j = 0; j < outputChannels; ++j) {
        QList<S> output(outputTableEntries);
        qFromBigEndian<S>(tableData, outputTableEntries, output.data());
        QColorTransferTable table(outputTableEntries, output, QColorTransferTable::OneWay);
        if (!table.checkValidity()) {
            qCWarning(lcIcc) << "Bad output table in lut8/lut16";
            return false;
        }
        if (!table.isIdentity())
            outTableIsLinear = false;
        outTableElement.trc[j] = std::move(table);
        tableData += outputTableEntries * precision;
    }

    if (isAb) {
        if (!inTableIsLinear)
            colorSpacePrivate->mAB.append(inTableElement);
        if (!clutElement.isEmpty())
            colorSpacePrivate->mAB.append(clutElement);
        if (!outTableIsLinear || colorSpacePrivate->mAB.isEmpty())
            colorSpacePrivate->mAB.append(outTableElement);
    } else {
        // The matrix is only to be applied if the input color-space is XYZ
        if (!colorSpacePrivate->isPcsLab && !matrixElement.isIdentity())
            colorSpacePrivate->mBA.append(matrixElement);
        if (!inTableIsLinear)
            colorSpacePrivate->mBA.append(inTableElement);
        if (!clutElement.isEmpty())
            colorSpacePrivate->mBA.append(clutElement);
        if (!outTableIsLinear || colorSpacePrivate->mBA.isEmpty())
            colorSpacePrivate->mBA.append(outTableElement);
    }
    return true;
}

// Parses mAB and mBA type elements
static bool parseMabData(const QByteArray &data, const TagEntry &tagEntry, QColorSpacePrivate *colorSpacePrivate, bool isAb)
{
    if (tagEntry.size < sizeof(mABTagData)) {
        qCWarning(lcIcc) << "Undersized mAB/mBA tag";
        return false;
    }
    if (qsizetype(tagEntry.size) > data.size()) {
        qCWarning(lcIcc) << "Truncated mAB/mBA tag";
        return false;
    }
    const mABTagData mab = qFromUnaligned<mABTagData>(data.constData() + tagEntry.offset);
    if ((mab.type != quint32(Tag::mAB_) && isAb) || (mab.type != quint32(Tag::mBA_) && !isAb)){
        qCWarning(lcIcc) << "Bad mAB/mBA content type";
        return false;
    }

    const int inputChannels = (isAb && colorSpacePrivate->colorModel == QColorSpace::ColorModel::Cmyk) ? 4 : 3;
    const int outputChannels = (!isAb && colorSpacePrivate->colorModel == QColorSpace::ColorModel::Cmyk) ? 4 : 3;

    if (mab.inputChannels != inputChannels) {
        qCWarning(lcIcc) << "Unsupported mAB/mBA input channel count" << mab.inputChannels;
        return false;
    }

    if (mab.outputChannels != outputChannels) {
        qCWarning(lcIcc) << "Unsupported mAB/mBA output channel count" << mab.outputChannels;
        return false;
    }

    // These combinations are legal: B, M + Matrix + B, A + Clut + B,  A + Clut + M + Matrix + B
    if (!mab.bCurvesOffset) {
        qCWarning(lcIcc) << "Illegal mAB/mBA without B table";
        return false;
    }
    if (((bool)mab.matrixOffset != (bool)mab.mCurvesOffset) ||
        ((bool)mab.aCurvesOffset != (bool)mab.clutOffset)) {
        qCWarning(lcIcc) << "Illegal mAB/mBA element combination";
        return false;
    }

    if (mab.aCurvesOffset > (tagEntry.size - 3 * sizeof(GenericTagData)) ||
        mab.bCurvesOffset > (tagEntry.size - 3 * sizeof(GenericTagData)) ||
        mab.mCurvesOffset > (tagEntry.size - 3 * sizeof(GenericTagData)) ||
        mab.matrixOffset > (tagEntry.size - 4 * 12) ||
        mab.clutOffset > (tagEntry.size - 20)) {
        qCWarning(lcIcc) << "Illegal mAB/mBA element offset";
        return false;
    }

    QColorSpacePrivate::TransferElement bTableElement;
    QColorSpacePrivate::TransferElement aTableElement;
    QColorCLUT clutElement;
    QColorSpacePrivate::TransferElement mTableElement;
    QColorMatrix matrixElement;
    QColorVector offsetElement;

    auto parseCurves = [&data, &tagEntry] (uint curvesOffset, QColorTrc *table, int channels) {
        for (int i = 0; i < channels; ++i) {
            if (qsizetype(tagEntry.offset + curvesOffset + 12) > data.size() || curvesOffset + 12 > tagEntry.size) {
                qCWarning(lcIcc) << "Space missing for channel curves in mAB/mBA";
                return false;
            }
            auto size = parseTRC(QByteArrayView(data).sliced(tagEntry.offset + curvesOffset, tagEntry.size - curvesOffset), table[i], QColorTransferTable::OneWay);
            if (!size)
                return false;
            if (size & 2) size += 2; // possible padding
            curvesOffset += size;
        }
        return true;
    };

    bool bCurvesAreLinear = true, aCurvesAreLinear = true, mCurvesAreLinear = true;

    // B Curves
    if (!parseCurves(mab.bCurvesOffset, bTableElement.trc, isAb ? outputChannels : inputChannels)) {
        qCWarning(lcIcc) << "Invalid B curves";
        return false;
    } else {
        bCurvesAreLinear = bTableElement.trc[0].isIdentity() && bTableElement.trc[1].isIdentity() && bTableElement.trc[2].isIdentity();
    }

    // A Curves
    if (mab.aCurvesOffset) {
        if (!parseCurves(mab.aCurvesOffset, aTableElement.trc, isAb ? inputChannels : outputChannels)) {
            qCWarning(lcIcc) << "Invalid A curves";
            return false;
        } else {
            aCurvesAreLinear = aTableElement.trc[0].isIdentity() && aTableElement.trc[1].isIdentity() && aTableElement.trc[2].isIdentity();
        }
    }

    // M Curves
    if (mab.mCurvesOffset) {
        if (!parseCurves(mab.mCurvesOffset, mTableElement.trc, 3)) {
            qCWarning(lcIcc) << "Invalid M curves";
            return false;
        } else {
            mCurvesAreLinear = mTableElement.trc[0].isIdentity() && mTableElement.trc[1].isIdentity() && mTableElement.trc[2].isIdentity();
        }
    }

    // Matrix
    if (mab.matrixOffset) {
        const MatrixElement matrix = qFromUnaligned<MatrixElement>(data.constData() + tagEntry.offset + mab.matrixOffset);
        matrixElement.r.x = fromFixedS1516(matrix.e0);
        matrixElement.g.x = fromFixedS1516(matrix.e1);
        matrixElement.b.x = fromFixedS1516(matrix.e2);
        matrixElement.r.y = fromFixedS1516(matrix.e3);
        matrixElement.g.y = fromFixedS1516(matrix.e4);
        matrixElement.b.y = fromFixedS1516(matrix.e5);
        matrixElement.r.z = fromFixedS1516(matrix.e6);
        matrixElement.g.z = fromFixedS1516(matrix.e7);
        matrixElement.b.z = fromFixedS1516(matrix.e8);
        offsetElement.x = fromFixedS1516(matrix.e9);
        offsetElement.y = fromFixedS1516(matrix.e10);
        offsetElement.z = fromFixedS1516(matrix.e11);
        if (!matrixElement.isValid() || !offsetElement.isValid()) {
            qCWarning(lcIcc) << "Invalid matrix values in mAB/mBA element";
            return false;
        }
    }

    // CLUT
    if (mab.clutOffset) {
        clutElement.gridPointsX = uint8_t(data[tagEntry.offset + mab.clutOffset]);
        clutElement.gridPointsY = uint8_t(data[tagEntry.offset + mab.clutOffset + 1]);
        clutElement.gridPointsZ = uint8_t(data[tagEntry.offset + mab.clutOffset + 2]);
        clutElement.gridPointsW = std::max(uint8_t(data[tagEntry.offset + mab.clutOffset + 3]), uint8_t(1));
        const uchar precision = data[tagEntry.offset + mab.clutOffset + 16];
        if (precision > 2 || precision < 1) {
            qCWarning(lcIcc) << "Invalid mAB/mBA element CLUT precision";
            return false;
        }
        if (clutElement.gridPointsX < 2 ||  clutElement.gridPointsY < 2 || clutElement.gridPointsZ < 2) {
            qCWarning(lcIcc) << "Empty CLUT";
            return false;
        }
        const qsizetype clutTableSize = clutElement.gridPointsX * clutElement.gridPointsY * clutElement.gridPointsZ * clutElement.gridPointsW;
        if ((mab.clutOffset + 20 + clutTableSize * outputChannels * precision) > tagEntry.size) {
            qCWarning(lcIcc) << "CLUT oversized for tag";
            return false;
        }

        clutElement.table.resize(clutTableSize);
        if (precision == 2)  {
            QList<uint16_t> clutTable(clutTableSize * outputChannels);
            qFromBigEndian<uint16_t>(data.constData() + tagEntry.offset + mab.clutOffset + 20, clutTable.size(), clutTable.data());
            parseCLUT(clutTable.constData(), (1.f/65535.f), &clutElement, outputChannels);
        } else {
            const uint8_t *clutTable = reinterpret_cast<const uint8_t *>(data.constData() + tagEntry.offset + mab.clutOffset + 20);
            parseCLUT(clutTable, (1.f/255.f), &clutElement, outputChannels);
        }
    } else if (colorSpacePrivate->colorModel == QColorSpace::ColorModel::Cmyk) {
        qCWarning(lcIcc) << "Cmyk conversion must have a CLUT";
        return false;
    }

    if (isAb) {
        if (mab.aCurvesOffset) {
            if (!aCurvesAreLinear)
                colorSpacePrivate->mAB.append(std::move(aTableElement));
            if (!clutElement.isEmpty())
                colorSpacePrivate->mAB.append(std::move(clutElement));
        }
        if (mab.mCurvesOffset && outputChannels == 3) {
            if (!mCurvesAreLinear)
                colorSpacePrivate->mAB.append(std::move(mTableElement));
            if (!matrixElement.isIdentity())
                colorSpacePrivate->mAB.append(std::move(matrixElement));
            if (!offsetElement.isNull())
                colorSpacePrivate->mAB.append(std::move(offsetElement));
        }
        if (!bCurvesAreLinear|| colorSpacePrivate->mAB.isEmpty())
            colorSpacePrivate->mAB.append(std::move(bTableElement));
    } else {
        if (!bCurvesAreLinear)
            colorSpacePrivate->mBA.append(std::move(bTableElement));
        if (mab.mCurvesOffset && inputChannels == 3) {
            if (!matrixElement.isIdentity())
                colorSpacePrivate->mBA.append(std::move(matrixElement));
            if (!offsetElement.isNull())
                colorSpacePrivate->mBA.append(std::move(offsetElement));
            if (!mCurvesAreLinear)
                colorSpacePrivate->mBA.append(std::move(mTableElement));
        }
        if (mab.aCurvesOffset) {
            if (!clutElement.isEmpty())
                colorSpacePrivate->mBA.append(std::move(clutElement));
            if (!aCurvesAreLinear)
                colorSpacePrivate->mBA.append(std::move(aTableElement));
        }
        if (colorSpacePrivate->mBA.isEmpty()) // Ensure non-empty to indicate valid empty transform
            colorSpacePrivate->mBA.append(std::move(bTableElement));
    }

    return true;
}

static bool parseA2B(const QByteArray &data, const TagEntry &tagEntry, QColorSpacePrivate *privat, bool isAb)
{
    const GenericTagData a2bData = qFromUnaligned<GenericTagData>(data.constData() + tagEntry.offset);
    if (a2bData.type == quint32(Tag::mft1))
        return parseLutData<Lut8TagData>(data, tagEntry, privat, isAb);
    else if (a2bData.type == quint32(Tag::mft2))
        return parseLutData<Lut16TagData>(data, tagEntry, privat, isAb);
    else if (a2bData.type == quint32(Tag::mAB_) || a2bData.type == quint32(Tag::mBA_))
        return parseMabData(data, tagEntry, privat, isAb);

    qCWarning(lcIcc) << "fromIccProfile: Unknown A2B/B2A data type";
    return false;
}

static bool parseDesc(const QByteArray &data, const TagEntry &tagEntry, QString &descName)
{
    const GenericTagData tag = qFromUnaligned<GenericTagData>(data.constData() + tagEntry.offset);

    // Either 'desc' (ICCv2) or 'mluc' (ICCv4)
    if (tag.type == quint32(Tag::desc)) {
        if (tagEntry.size < sizeof(DescTagData))
            return false;
        Q_STATIC_ASSERT(sizeof(DescTagData) == 12);
        const DescTagData desc = qFromUnaligned<DescTagData>(data.constData() + tagEntry.offset);
        const quint32 len = desc.asciiDescriptionLength;
        if (len < 1)
            return false;
        if (tagEntry.size - 12 < len)
            return false;
        const char *asciiDescription = data.constData() + tagEntry.offset + sizeof(DescTagData);
        if (asciiDescription[len - 1] != '\0')
            return false;
        descName = QString::fromLatin1(asciiDescription, len - 1);
        return true;
    }
    if (tag.type != quint32(Tag::mluc))
        return false;

    if (tagEntry.size < sizeof(MlucTagData))
        return false;
    const MlucTagData mluc = qFromUnaligned<MlucTagData>(data.constData() + tagEntry.offset);
    if (mluc.recordCount < 1)
        return false;
    if (mluc.recordSize != 12)
        return false;
    // We just use the primary record regardless of language or country.
    const quint32 stringOffset = mluc.records[0].offset;
    const quint32 stringSize = mluc.records[0].size;
    if (tagEntry.size < stringOffset || tagEntry.size - stringOffset < stringSize )
        return false;
    if ((stringSize | stringOffset) & 1)
        return false;
    quint32 stringLen = stringSize / 2;
    QVarLengthArray<char16_t> utf16hostendian(stringLen);
    qFromBigEndian<char16_t>(data.constData() + tagEntry.offset + stringOffset, stringLen,
                             utf16hostendian.data());
    // The given length shouldn't include 0-termination, but might.
    if (stringLen > 1 && utf16hostendian[stringLen - 1] == 0)
        --stringLen;
    descName = QString::fromUtf16(utf16hostendian.data(), stringLen);
    return true;
}

static bool parseRgbMatrix(const QByteArray &data, const QHash<Tag, TagEntry> &tagIndex, QColorSpacePrivate *colorspaceDPtr)
{
    // Parse XYZ tags
    if (!parseXyzData(data, tagIndex[Tag::rXYZ], colorspaceDPtr->toXyz.r))
        return false;
    if (!parseXyzData(data, tagIndex[Tag::gXYZ], colorspaceDPtr->toXyz.g))
        return false;
    if (!parseXyzData(data, tagIndex[Tag::bXYZ], colorspaceDPtr->toXyz.b))
        return false;
    if (!parseXyzData(data, tagIndex[Tag::wtpt], colorspaceDPtr->whitePoint))
        return false;
    if (!colorspaceDPtr->toXyz.isValid() || !colorspaceDPtr->whitePoint.isValid() || colorspaceDPtr->whitePoint.isNull()) {
        qCWarning(lcIcc) << "Invalid XYZ values in RGB matrix";
        return false;
    }

    colorspaceDPtr->primaries = QColorSpace::Primaries::Custom;
    if (colorspaceDPtr->toXyz == QColorMatrix::toXyzFromSRgb()) {
        qCDebug(lcIcc) << "fromIccProfile: sRGB primaries detected";
        colorspaceDPtr->primaries = QColorSpace::Primaries::SRgb;
    } else if (colorspaceDPtr->toXyz == QColorMatrix::toXyzFromAdobeRgb()) {
        qCDebug(lcIcc) << "fromIccProfile: Adobe RGB primaries detected";
        colorspaceDPtr->primaries = QColorSpace::Primaries::AdobeRgb;
    } else if (colorspaceDPtr->toXyz == QColorMatrix::toXyzFromDciP3D65()) {
        qCDebug(lcIcc) << "fromIccProfile: DCI-P3 D65 primaries detected";
        colorspaceDPtr->primaries = QColorSpace::Primaries::DciP3D65;
    } else if (colorspaceDPtr->toXyz == QColorMatrix::toXyzFromProPhotoRgb()) {
        qCDebug(lcIcc) << "fromIccProfile: ProPhoto RGB primaries detected";
        colorspaceDPtr->primaries = QColorSpace::Primaries::ProPhotoRgb;
    } else if (colorspaceDPtr->toXyz == QColorMatrix::toXyzFromBt2020()) {
        qCDebug(lcIcc) << "fromIccProfile: BT.2020 primaries detected";
        colorspaceDPtr->primaries = QColorSpace::Primaries::Bt2020;
    }
    return true;
}

static bool parseGrayMatrix(const QByteArray &data, const QHash<Tag, TagEntry> &tagIndex, QColorSpacePrivate *colorspaceDPtr)
{
    QColorVector whitePoint;
    if (!parseXyzData(data, tagIndex[Tag::wtpt], whitePoint))
        return false;
    if (!whitePoint.isValid() || !qFuzzyCompare(whitePoint.y, 1.0f) || (1.0f + whitePoint.z + whitePoint.x) == 0.0f) {
        qCWarning(lcIcc) << "fromIccProfile: Invalid ICC profile - gray white-point not normalized";
        return false;
    }
    colorspaceDPtr->primaries = QColorSpace::Primaries::Custom;
    colorspaceDPtr->whitePoint = whitePoint;
    return true;
}

static bool parseChad(const QByteArray &data, const TagEntry &tagEntry, QColorSpacePrivate *colorspaceDPtr)
{
    if (tagEntry.size < sizeof(Sf32TagData) || qsizetype(tagEntry.size) > data.size())
        return false;
    const Sf32TagData chadtag = qFromUnaligned<Sf32TagData>(data.constData() + tagEntry.offset);
    if (chadtag.type != uint32_t(Tag::sf32)) {
        qCWarning(lcIcc, "fromIccProfile: bad chad data type");
        return false;
    }
    QColorMatrix chad;
    chad.r.x = fromFixedS1516(chadtag.value[0]);
    chad.g.x = fromFixedS1516(chadtag.value[1]);
    chad.b.x = fromFixedS1516(chadtag.value[2]);
    chad.r.y = fromFixedS1516(chadtag.value[3]);
    chad.g.y = fromFixedS1516(chadtag.value[4]);
    chad.b.y = fromFixedS1516(chadtag.value[5]);
    chad.r.z = fromFixedS1516(chadtag.value[6]);
    chad.g.z = fromFixedS1516(chadtag.value[7]);
    chad.b.z = fromFixedS1516(chadtag.value[8]);

    if (!chad.isValid()) {
        qCWarning(lcIcc, "fromIccProfile: invalid chad matrix");
        return false;
    }
    colorspaceDPtr->chad = chad;
    return true;
}

static bool parseTRCs(const QByteArray &data, const QHash<Tag, TagEntry> &tagIndex, QColorSpacePrivate *colorspaceDPtr, bool isColorSpaceTypeGray)
{
    TagEntry rTrc;
    TagEntry gTrc;
    TagEntry bTrc;
    if (isColorSpaceTypeGray) {
        rTrc = tagIndex[Tag::kTRC];
        gTrc = tagIndex[Tag::kTRC];
        bTrc = tagIndex[Tag::kTRC];
    } else if (tagIndex.contains(Tag::aarg) && tagIndex.contains(Tag::aagg) && tagIndex.contains(Tag::aabg)) {
        // Apple extension for parametric version of TRCs in ICCv2:
        rTrc = tagIndex[Tag::aarg];
        gTrc = tagIndex[Tag::aagg];
        bTrc = tagIndex[Tag::aabg];
    } else {
        rTrc = tagIndex[Tag::rTRC];
        gTrc = tagIndex[Tag::gTRC];
        bTrc = tagIndex[Tag::bTRC];
    }

    QColorTrc rCurve;
    QColorTrc gCurve;
    QColorTrc bCurve;
    if (!parseTRC(QByteArrayView(data).sliced(rTrc.offset, rTrc.size), rCurve, QColorTransferTable::TwoWay)) {
        qCWarning(lcIcc) << "fromIccProfile: Invalid rTRC";
        return false;
    }
    if (!parseTRC(QByteArrayView(data).sliced(gTrc.offset, gTrc.size), gCurve, QColorTransferTable::TwoWay)) {
        qCWarning(lcIcc) << "fromIccProfile: Invalid gTRC";
        return false;
    }
    if (!parseTRC(QByteArrayView(data).sliced(bTrc.offset, bTrc.size), bCurve, QColorTransferTable::TwoWay)) {
        qCWarning(lcIcc) << "fromIccProfile: Invalid bTRC";
        return false;
    }
    if (rCurve == gCurve && gCurve == bCurve)  {
        if (rCurve.isIdentity()) {
            qCDebug(lcIcc) << "fromIccProfile: Linear gamma detected";
            colorspaceDPtr->trc[0] = QColorTransferFunction();
            colorspaceDPtr->transferFunction = QColorSpace::TransferFunction::Linear;
            colorspaceDPtr->gamma = 1.0f;
        } else if (rCurve.m_type == QColorTrc::Type::ParameterizedFunction && rCurve.m_fun.isGamma()) {
            qCDebug(lcIcc) << "fromIccProfile: Simple gamma detected";
            colorspaceDPtr->trc[0] = QColorTransferFunction::fromGamma(rCurve.m_fun.m_g);
            colorspaceDPtr->transferFunction = QColorSpace::TransferFunction::Gamma;
            colorspaceDPtr->gamma = rCurve.m_fun.m_g;
        } else if (rCurve.m_type == QColorTrc::Type::ParameterizedFunction && rCurve.m_fun.isSRgb()) {
            qCDebug(lcIcc) << "fromIccProfile: sRGB gamma detected";
            colorspaceDPtr->trc[0] = QColorTransferFunction::fromSRgb();
            colorspaceDPtr->transferFunction = QColorSpace::TransferFunction::SRgb;
        } else {
            colorspaceDPtr->trc[0] = rCurve;
            colorspaceDPtr->transferFunction = QColorSpace::TransferFunction::Custom;
        }
        colorspaceDPtr->trc[1] = colorspaceDPtr->trc[0];
        colorspaceDPtr->trc[2] = colorspaceDPtr->trc[0];
    } else {
        colorspaceDPtr->trc[0] = rCurve;
        colorspaceDPtr->trc[1] = gCurve;
        colorspaceDPtr->trc[2] = bCurve;
        colorspaceDPtr->transferFunction = QColorSpace::TransferFunction::Custom;
    }
    return true;
}

static bool parseCicp(const QByteArray &data, const TagEntry &tagEntry, QColorSpacePrivate *colorspaceDPtr)
{
    if (colorspaceDPtr->isPcsLab)
        return false;
    if (tagEntry.size < sizeof(CicpTagData) || qsizetype(tagEntry.size) > data.size())
        return false;
    const CicpTagData cicp = qFromUnaligned<CicpTagData>(data.constData() + tagEntry.offset);
    if (cicp.type != uint32_t(Tag::cicp)) {
        qCWarning(lcIcc, "fromIccProfile: bad cicp data type");
        return false;
    }
    switch (cicp.transferCharacteristics) {
    case 4:
        colorspaceDPtr->transferFunction = QColorSpace::TransferFunction::Gamma;
        colorspaceDPtr->gamma = 2.2f;
        break;
    case 5:
        colorspaceDPtr->transferFunction = QColorSpace::TransferFunction::Gamma;
        colorspaceDPtr->gamma = 2.8f;
        break;
    case 8:
        colorspaceDPtr->transferFunction = QColorSpace::TransferFunction::Linear;
        break;
    case 13:
        colorspaceDPtr->transferFunction = QColorSpace::TransferFunction::SRgb;
        break;
    case 1:
    case 6:
    case 14:
    case 15:
        colorspaceDPtr->transferFunction = QColorSpace::TransferFunction::Bt2020;
        break;
    case 16:
        colorspaceDPtr->transferFunction = QColorSpace::TransferFunction::St2084;
        break;
    case 18:
        colorspaceDPtr->transferFunction = QColorSpace::TransferFunction::Hlg;
        break;
    default:
        return false;
    }
    switch (cicp.colorPrimaries) {
    case 1:
        colorspaceDPtr->primaries = QColorSpace::Primaries::SRgb;
        break;
    case 9:
        colorspaceDPtr->primaries = QColorSpace::Primaries::Bt2020;
        break;
    case 12:
        colorspaceDPtr->primaries = QColorSpace::Primaries::DciP3D65;
        break;
    default:
        return false;
    }
    return true;
}

bool fromIccProfile(const QByteArray &data, QColorSpace *colorSpace)
{
    if (data.size() < qsizetype(sizeof(ICCProfileHeader))) {
        qCWarning(lcIcc) << "fromIccProfile: failed size sanity 1";
        return false;
    }
    const ICCProfileHeader header = qFromUnaligned<ICCProfileHeader>(data.constData());
    if (!isValidIccProfile(header))
        return false; // if failed we already printing a warning
    if (qsizetype(header.profileSize) > data.size() || qsizetype(header.profileSize) < qsizetype(sizeof(ICCProfileHeader))) {
        qCWarning(lcIcc) << "fromIccProfile: failed size sanity 2";
        return false;
    }

    const qsizetype offsetToData = sizeof(ICCProfileHeader) + header.tagCount * sizeof(TagTableEntry);
    Q_ASSERT(offsetToData > 0);
    if (offsetToData > data.size()) {
        qCWarning(lcIcc) << "fromIccProfile: failed index size sanity";
        return false;
    }

    QHash<Tag, TagEntry> tagIndex;
    for (uint i = 0; i < header.tagCount; ++i) {
        // Read tag index
        const qsizetype tableOffset = sizeof(ICCProfileHeader) + i * sizeof(TagTableEntry);
        const TagTableEntry tagTable = qFromUnaligned<TagTableEntry>(data.constData()
                                                                     + tableOffset);

        // Sanity check tag sizes and offsets:
        if (qsizetype(tagTable.offset) < offsetToData) {
            qCWarning(lcIcc) << "fromIccProfile: failed tag offset sanity 1";
            return false;
        }
        // Checked separately from (+ size) to handle overflow.
        if (tagTable.offset > header.profileSize) {
            qCWarning(lcIcc) << "fromIccProfile: failed tag offset sanity 2";
            return false;
        }
        if (tagTable.size < 8) {
            qCWarning(lcIcc) << "fromIccProfile: failed minimal tag size sanity";
            return false;
        }
        if (tagTable.size > header.profileSize - tagTable.offset) {
            qCWarning(lcIcc) << "fromIccProfile: failed tag offset + size sanity";
            return false;
        }
        if (tagTable.offset & 0x03) {
            qCWarning(lcIcc) << "fromIccProfile: invalid tag offset alignment";
            return false;
        }
//        printf("'%4s' %d %d\n", (const char *)&tagTable.signature,
//                                quint32(tagTable.offset),
//                                quint32(tagTable.size));
        tagIndex.insert(Tag(quint32(tagTable.signature)), { tagTable.offset, tagTable.size });
    }

    bool threeComponentMatrix = true;

    if (header.inputColorSpace == uint(ColorSpaceType::Rgb)) {
        // Check the profile is three-component matrix based:
        if (!tagIndex.contains(Tag::rXYZ) || !tagIndex.contains(Tag::gXYZ) || !tagIndex.contains(Tag::bXYZ) ||
            !tagIndex.contains(Tag::rTRC) || !tagIndex.contains(Tag::gTRC) || !tagIndex.contains(Tag::bTRC) ||
            !tagIndex.contains(Tag::wtpt) || header.pcs == uint(Tag::Lab_)) {
            threeComponentMatrix = false;
            // Check if the profile is valid n-LUT based:
            if (!tagIndex.contains(Tag::A2B0)) {
                qCWarning(lcIcc) << "fromIccProfile: Invalid ICC profile - neither valid three component nor n-LUT";
                return false;
            }
        }
    } else if (header.inputColorSpace == uint(ColorSpaceType::Gray)) {
        if (!tagIndex.contains(Tag::kTRC) || !tagIndex.contains(Tag::wtpt)) {
            qCWarning(lcIcc) << "fromIccProfile: Invalid ICC profile - not valid gray scale based";
            return false;
        }
    } else if (header.inputColorSpace == uint(ColorSpaceType::Cmyk)) {
        threeComponentMatrix = false;
        if (!tagIndex.contains(Tag::A2B0)) {
            qCWarning(lcIcc) << "fromIccProfile: Invalid ICC profile - CMYK, not n-LUT";
            return false;
        }
    } else {
        Q_UNREACHABLE();
    }

    colorSpace->detach();
    QColorSpacePrivate *colorspaceDPtr = QColorSpacePrivate::get(*colorSpace);

    colorspaceDPtr->isPcsLab = (header.pcs == uint(Tag::Lab_));
    if (tagIndex.contains(Tag::cicp) && header.inputColorSpace == uint(ColorSpaceType::Rgb)) {
        // Let cicp override nLut profiles if we fully recognize it.
        if (parseCicp(data, tagIndex[Tag::cicp], colorspaceDPtr))
            threeComponentMatrix = true;
        if (colorspaceDPtr->primaries != QColorSpace::Primaries::Custom)
            colorspaceDPtr->setToXyzMatrix();
        if (colorspaceDPtr->transferFunction != QColorSpace::TransferFunction::Custom)
            colorspaceDPtr->setTransferFunction();
    }

    if (threeComponentMatrix) {
        colorspaceDPtr->transformModel = QColorSpace::TransformModel::ThreeComponentMatrix;

        if (header.inputColorSpace == uint(ColorSpaceType::Rgb)) {
            if (colorspaceDPtr->primaries == QColorSpace::Primaries::Custom && !parseRgbMatrix(data, tagIndex, colorspaceDPtr))
                return false;
            colorspaceDPtr->colorModel = QColorSpace::ColorModel::Rgb;
        } else if (header.inputColorSpace == uint(ColorSpaceType::Gray)) {
            if (!parseGrayMatrix(data, tagIndex, colorspaceDPtr))
                return false;
            colorspaceDPtr->colorModel = QColorSpace::ColorModel::Gray;
        } else {
            Q_UNREACHABLE();
        }
        if (auto it = tagIndex.constFind(Tag::chad); it != tagIndex.constEnd()) {
            if (!parseChad(data, it.value(), colorspaceDPtr))
                return false;
        } else {
            colorspaceDPtr->chad = QColorMatrix::chromaticAdaptation(colorspaceDPtr->whitePoint);
        }
        if (colorspaceDPtr->colorModel == QColorSpace::ColorModel::Gray)
            colorspaceDPtr->toXyz = colorspaceDPtr->chad;

        // Reset the matrix to our canonical values:
        if (colorspaceDPtr->primaries != QColorSpace::Primaries::Custom)
            colorspaceDPtr->setToXyzMatrix();

        if (colorspaceDPtr->transferFunction == QColorSpace::TransferFunction::Custom &&
            !parseTRCs(data, tagIndex, colorspaceDPtr, header.inputColorSpace == uint(ColorSpaceType::Gray)))
            return false;
    } else {
        colorspaceDPtr->transformModel = QColorSpace::TransformModel::ElementListProcessing;
        if (header.inputColorSpace == uint(ColorSpaceType::Cmyk))
            colorspaceDPtr->colorModel = QColorSpace::ColorModel::Cmyk;
        else
            colorspaceDPtr->colorModel = QColorSpace::ColorModel::Rgb;

        // Only parse the default perceptual transform for now
        if (!parseA2B(data, tagIndex[Tag::A2B0], colorspaceDPtr, true))
            return false;
        if (auto it = tagIndex.constFind(Tag::B2A0); it != tagIndex.constEnd()) {
            if (!parseA2B(data, it.value(), colorspaceDPtr, false))
                return false;
        }

        if (auto it = tagIndex.constFind(Tag::wtpt); it != tagIndex.constEnd()) {
            if (!parseXyzData(data, it.value(), colorspaceDPtr->whitePoint))
                return false;
        }
        if (auto it = tagIndex.constFind(Tag::chad); it != tagIndex.constEnd()) {
            if (!parseChad(data, it.value(), colorspaceDPtr))
                return false;
        } else if (!colorspaceDPtr->whitePoint.isNull()) {
            colorspaceDPtr->chad = QColorMatrix::chromaticAdaptation(colorspaceDPtr->whitePoint);
        }
    }

    if (auto it = tagIndex.constFind(Tag::desc); it != tagIndex.constEnd()) {
        if (!parseDesc(data, it.value(), colorspaceDPtr->description))
            qCWarning(lcIcc) << "fromIccProfile: Failed to parse description";
        else
            qCDebug(lcIcc) << "fromIccProfile: Description" << colorspaceDPtr->description;
    }

    colorspaceDPtr->identifyColorSpace();
    if (colorspaceDPtr->namedColorSpace)
        qCDebug(lcIcc) << "fromIccProfile: Named colorspace detected: " << QColorSpace::NamedColorSpace(colorspaceDPtr->namedColorSpace);

    colorspaceDPtr->iccProfile = data;

    Q_ASSERT(colorspaceDPtr->isValid());
    return true;
}

} // namespace QIcc

QT_END_NAMESPACE
