/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qicc_p.h"

#include <qbuffer.h>
#include <qbytearray.h>
#include <qdatastream.h>
#include <qendian.h>
#include <qloggingcategory.h>
#include <qstring.h>

#include "qcolorspace_p.h"
#include "qcolortrc_p.h"

#include <array>

QT_BEGIN_NAMESPACE
Q_LOGGING_CATEGORY(lcIcc, "qt.gui.icc")

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
};

enum class ProfileClass : quint32 {
    Input       = IccTag('s', 'c', 'r', 'n'),
    Display     = IccTag('m', 'n', 't', 'r'),
    // Not supported:
    Output      = IccTag('p', 'r', 't', 'r'),
    ColorSpace  = IccTag('s', 'p', 'a', 'c'),
};

enum class Tag : quint32 {
    acsp = IccTag('a', 'c', 's', 'p'),
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
    B2A0 = IccTag('B', '2', 'A', '0'),
    B2A1 = IccTag('B', '2', 'A', '1'),
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
    mAB_ = IccTag('m', 'A', 'B', ' '),
    mBA_ = IccTag('m', 'B', 'A', ' '),
    chad = IccTag('c', 'h', 'a', 'd'),
    sf32 = IccTag('s', 'f', '3', '2'),

    // Apple extensions for ICCv2:
    aarg = IccTag('a', 'a', 'r', 'g'),
    aagg = IccTag('a', 'a', 'g', 'g'),
    aabg = IccTag('a', 'a', 'b', 'g'),
};

inline uint qHash(const Tag &key, uint seed = 0)
{
    return qHash(quint32(key), seed);
}

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
    quint16_be value[1];
};

struct ParaTagData : GenericTagData {
    quint16_be curveType;
    quint16_be null2;
    quint32_be parameter[1];
};

struct DescTagData : GenericTagData {
    quint32_be asciiDescriptionLength;
    char asciiDescription[1];
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
};

struct Sf32TagData : GenericTagData {
    quint32_be value[1];
};

static int toFixedS1516(float x)
{
    return int(x * 65536.0f + 0.5f);
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
        && header.profileClass != uint(ProfileClass::Display)) {
        qCWarning(lcIcc, "Unsupported ICC profile class %x", quint32(header.profileClass));
        return false;
    }
    if (header.inputColorSpace != uint(ColorSpaceType::Rgb)
        && header.inputColorSpace != uint(ColorSpaceType::Gray)) {
        qCWarning(lcIcc, "Unsupported ICC input color space %x", quint32(header.inputColorSpace));
        return false;
    }
    if (header.pcs != 0x58595a20 /* 'XYZ '*/) {
        // ### support PCSLAB
        qCWarning(lcIcc, "Unsupported ICC profile connection space %x", quint32(header.pcs));
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
    if (trc.isLinear()) {
        stream << uint(Tag::curv) << uint(0);
        stream << uint(0);
        return 12;
    }

    if (trc.m_type == QColorTrc::Type::Function) {
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
    return 12 + 2 * trc.m_table.m_tableSize;
}

QByteArray toIccProfile(const QColorSpace &space)
{
    if (!space.isValid())
        return QByteArray();

    const QColorSpacePrivate *spaceDPtr = QColorSpacePrivate::get(space);

    constexpr int tagCount = 9;
    constexpr uint profileDataOffset = 128 + 4 + 12 * tagCount;
    constexpr uint variableTagTableOffsets = 128 + 4 + 12 * 5;
    uint currentOffset = 0;
    uint rTrcOffset, gTrcOffset, bTrcOffset;
    uint rTrcSize, gTrcSize, bTrcSize;
    uint descOffset, descSize;

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream stream(&buffer);

    // Profile header:
    stream << uint(0); // Size, we will update this later
    stream << uint(0);
    stream << uint(0x02400000); // Version 2.4 (note we use 'para' from version 4)
    stream << uint(ProfileClass::Display);
    stream << uint(Tag::RGB_);
    stream << uint(Tag::XYZ_);
    stream << uint(0) << uint(0) << uint(0);
    stream << uint(Tag::acsp);
    stream << uint(0) << uint(0) << uint(0);
    stream << uint(0) << uint(0) << uint(0);
    stream << uint(1); // Rendering intent
    stream << uint(0x0000f6d6); // D50 X
    stream << uint(0x00010000); // D50 Y
    stream << uint(0x0000d32d); // D50 Z
    stream << IccTag('Q','t', QT_VERSION_MAJOR, QT_VERSION_MINOR);
    stream << uint(0) << uint(0) << uint(0) << uint(0);
    stream << uint(0) << uint(0) << uint(0) << uint(0) << uint(0) << uint(0) << uint(0);

    // Tag table:
    stream << uint(tagCount);
    stream << uint(Tag::rXYZ) << uint(profileDataOffset + 00) << uint(20);
    stream << uint(Tag::gXYZ) << uint(profileDataOffset + 20) << uint(20);
    stream << uint(Tag::bXYZ) << uint(profileDataOffset + 40) << uint(20);
    stream << uint(Tag::wtpt) << uint(profileDataOffset + 60) << uint(20);
    stream << uint(Tag::cprt) << uint(profileDataOffset + 80) << uint(12);
    // From here the offset and size will be updated later:
    stream << uint(Tag::rTRC) << uint(0) << uint(0);
    stream << uint(Tag::gTRC) << uint(0) << uint(0);
    stream << uint(Tag::bTRC) << uint(0) << uint(0);
    stream << uint(Tag::desc) << uint(0) << uint(0);
    // TODO: consider adding 'chad' tag (required in ICC >=4 when we have non-D50 whitepoint)
    currentOffset = profileDataOffset;

    // Tag data:
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
    stream << uint(Tag::XYZ_) << uint(0);
    stream << toFixedS1516(spaceDPtr->whitePoint.x);
    stream << toFixedS1516(spaceDPtr->whitePoint.y);
    stream << toFixedS1516(spaceDPtr->whitePoint.z);
    stream << uint(Tag::text) << uint(0);
    stream << uint(IccTag('N', '/', 'A', '\0'));
    currentOffset += 92;

    // From now on the data is variable sized:
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

    descOffset = currentOffset;
    QByteArray description = spaceDPtr->description.toUtf8();
    stream << uint(Tag::desc) << uint(0);
    stream << uint(description.size() + 1);
    stream.writeRawData(description.constData(), description.size() + 1);
    stream << uint(0) << uint(0);
    stream << ushort(0) << uchar(0);
    QByteArray macdesc(67, '\0');
    stream.writeRawData(macdesc.constData(), 67);
    descSize = 90 + description.size() + 1;
    currentOffset += descSize;

    buffer.close();
    QByteArray iccProfile = buffer.buffer();
    // Now write final size
    *(quint32_be *)iccProfile.data() = iccProfile.size();
    // And the final indices and sizes of variable size tags:
    *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 4) = rTrcOffset;
    *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 8) = rTrcSize;
    *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 12 + 4) = gTrcOffset;
    *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 12 + 8) = gTrcSize;
    *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 2 * 12 + 4) = bTrcOffset;
    *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 2 * 12 + 8) = bTrcSize;
    *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 3 * 12 + 4) = descOffset;
    *(quint32_be *)(iccProfile.data() + variableTagTableOffsets + 3 * 12 + 8) = descSize;

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

bool parseXyzData(const QByteArray &data, const TagEntry &tagEntry, QColorVector &colorVector)
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

bool parseTRC(const QByteArray &data, const TagEntry &tagEntry, QColorTrc &gamma)
{
    const GenericTagData trcData = qFromUnaligned<GenericTagData>(data.constData()
                                                                  + tagEntry.offset);
    if (trcData.type == quint32(Tag::curv)) {
        const CurvTagData curv = qFromUnaligned<CurvTagData>(data.constData() + tagEntry.offset);
        if (curv.valueCount > (1 << 16))
            return false;
        if (tagEntry.size - 12 < 2 * curv.valueCount)
            return false;
        if (curv.valueCount == 0) {
            gamma.m_type = QColorTrc::Type::Function;
            gamma.m_fun = QColorTransferFunction(); // Linear
        } else if (curv.valueCount == 1) {
            float g = curv.value[0] * (1.0f / 256.0f);
            gamma.m_type = QColorTrc::Type::Function;
            gamma.m_fun = QColorTransferFunction::fromGamma(g);
        } else {
            QVector<quint16> tabl;
            tabl.resize(curv.valueCount);
            static_assert(sizeof(GenericTagData) == 2 * sizeof(quint32_be),
                          "GenericTagData has padding. The following code is a subject to UB.");
            const auto offset = tagEntry.offset + sizeof(GenericTagData) + sizeof(quint32_be);
            qFromBigEndian<quint16>(data.constData() + offset, curv.valueCount, tabl.data());
            QColorTransferTable table = QColorTransferTable(curv.valueCount, std::move(tabl));
            QColorTransferFunction curve;
            if (!table.checkValidity()) {
                qCWarning(lcIcc) << "Invalid curv table";
                return false;
            } else if (!table.asColorTransferFunction(&curve)) {
                gamma.m_type = QColorTrc::Type::Table;
                gamma.m_table = table;
            } else {
                qCDebug(lcIcc) << "Detected curv table as function";
                gamma.m_type = QColorTrc::Type::Function;
                gamma.m_fun = curve;
            }
        }
        return true;
    }
    if (trcData.type == quint32(Tag::para)) {
        if (tagEntry.size < sizeof(ParaTagData))
            return false;
        static_assert(sizeof(GenericTagData) == 2 * sizeof(quint32_be),
                      "GenericTagData has padding. The following code is a subject to UB.");
        const ParaTagData para = qFromUnaligned<ParaTagData>(data.constData() + tagEntry.offset);
        // re-read first parameter for consistency:
        const auto parametersOffset = tagEntry.offset + sizeof(GenericTagData)
                                      + 2 * sizeof(quint16_be);
        switch (para.curveType) {
        case 0: {
            float g = fromFixedS1516(para.parameter[0]);
            gamma.m_type = QColorTrc::Type::Function;
            gamma.m_fun = QColorTransferFunction::fromGamma(g);
            break;
        }
        case 1: {
            if (tagEntry.size < sizeof(ParaTagData) + 2 * 4)
                return false;
            std::array<quint32_be, 3> parameters =
                qFromUnaligned<decltype(parameters)>(data.constData() + parametersOffset);
            float g = fromFixedS1516(parameters[0]);
            float a = fromFixedS1516(parameters[1]);
            float b = fromFixedS1516(parameters[2]);
            float d = -b / a;
            gamma.m_type = QColorTrc::Type::Function;
            gamma.m_fun = QColorTransferFunction(a, b, 0.0f, d, 0.0f, 0.0f, g);
            break;
        }
        case 2: {
            if (tagEntry.size < sizeof(ParaTagData) + 3 * 4)
                return false;
            std::array<quint32_be, 4> parameters =
                qFromUnaligned<decltype(parameters)>(data.constData() + parametersOffset);
            float g = fromFixedS1516(parameters[0]);
            float a = fromFixedS1516(parameters[1]);
            float b = fromFixedS1516(parameters[2]);
            float c = fromFixedS1516(parameters[3]);
            float d = -b / a;
            gamma.m_type = QColorTrc::Type::Function;
            gamma.m_fun = QColorTransferFunction(a, b, 0.0f, d, c, c, g);
            break;
        }
        case 3: {
            if (tagEntry.size < sizeof(ParaTagData) + 4 * 4)
                return false;
            std::array<quint32_be, 5> parameters =
                qFromUnaligned<decltype(parameters)>(data.constData() + parametersOffset);
            float g = fromFixedS1516(parameters[0]);
            float a = fromFixedS1516(parameters[1]);
            float b = fromFixedS1516(parameters[2]);
            float c = fromFixedS1516(parameters[3]);
            float d = fromFixedS1516(parameters[4]);
            gamma.m_type = QColorTrc::Type::Function;
            gamma.m_fun = QColorTransferFunction(a, b, c, d, 0.0f, 0.0f, g);
            break;
        }
        case 4: {
            if (tagEntry.size < sizeof(ParaTagData) + 6 * 4)
                return false;
            std::array<quint32_be, 7> parameters =
                qFromUnaligned<decltype(parameters)>(data.constData() + parametersOffset);
            float g = fromFixedS1516(parameters[0]);
            float a = fromFixedS1516(parameters[1]);
            float b = fromFixedS1516(parameters[2]);
            float c = fromFixedS1516(parameters[3]);
            float d = fromFixedS1516(parameters[4]);
            float e = fromFixedS1516(parameters[5]);
            float f = fromFixedS1516(parameters[6]);
            gamma.m_type = QColorTrc::Type::Function;
            gamma.m_fun = QColorTransferFunction(a, b, c, d, e, f, g);
            break;
        }
        default:
            qCWarning(lcIcc)  << "Unknown para type" << uint(para.curveType);
            return false;
        }
        return true;
    }
    qCWarning(lcIcc) << "Invalid TRC data type";
    return false;
}

bool parseDesc(const QByteArray &data, const TagEntry &tagEntry, QString &descName)
{
    const GenericTagData tag = qFromUnaligned<GenericTagData>(data.constData() + tagEntry.offset);

    // Either 'desc' (ICCv2) or 'mluc' (ICCv4)
    if (tag.type == quint32(Tag::desc)) {
        if (tagEntry.size < sizeof(DescTagData))
            return false;
        const DescTagData desc = qFromUnaligned<DescTagData>(data.constData() + tagEntry.offset);
        const quint32 len = desc.asciiDescriptionLength;
        if (len < 1)
            return false;
        if (tagEntry.size - 12 < len)
            return false;
        static_assert(sizeof(GenericTagData) == 2 * sizeof(quint32_be),
                      "GenericTagData has padding. The following code is a subject to UB.");
        const char *asciiDescription = data.constData() + tagEntry.offset + sizeof(GenericTagData)
                                       + sizeof(quint32_be);
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
    if (mluc.recordSize < 12)
        return false;
    // We just use the primary record regardless of language or country.
    const quint32 stringOffset = mluc.records[0].offset;
    const quint32 stringSize = mluc.records[0].size;
    if (tagEntry.size < stringOffset || tagEntry.size - stringOffset < stringSize )
        return false;
    if ((stringSize | stringOffset) & 1)
        return false;
    quint32 stringLen = stringSize / 2;
    QVarLengthArray<ushort> utf16hostendian(stringLen);
    qFromBigEndian<ushort>(data.constData() + tagEntry.offset + stringOffset, stringLen,
                             utf16hostendian.data());
    // The given length shouldn't include 0-termination, but might.
    if (stringLen > 1 && utf16hostendian[stringLen - 1] == 0)
        --stringLen;
    descName = QString::fromUtf16(utf16hostendian.data(), stringLen);
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
    if (qsizetype(header.profileSize) > data.size()) {
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
        if (tagTable.size < 12) {
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

    // Check the profile is three-component matrix based (what we currently support):
    if (header.inputColorSpace == uint(ColorSpaceType::Rgb)) {
        if (!tagIndex.contains(Tag::rXYZ) || !tagIndex.contains(Tag::gXYZ) || !tagIndex.contains(Tag::bXYZ) ||
            !tagIndex.contains(Tag::rTRC) || !tagIndex.contains(Tag::gTRC) || !tagIndex.contains(Tag::bTRC) ||
            !tagIndex.contains(Tag::wtpt)) {
            qCWarning(lcIcc) << "fromIccProfile: Unsupported ICC profile - not three component matrix based";
            return false;
        }
    } else {
        Q_ASSERT(header.inputColorSpace == uint(ColorSpaceType::Gray));
        if (!tagIndex.contains(Tag::kTRC) || !tagIndex.contains(Tag::wtpt)) {
            qCWarning(lcIcc) << "fromIccProfile: Invalid ICC profile - not valid gray scale based";
            return false;
        }
    }

    QColorSpacePrivate *colorspaceDPtr = QColorSpacePrivate::getWritable(*colorSpace);

    if (header.inputColorSpace == uint(ColorSpaceType::Rgb)) {
        // Parse XYZ tags
        if (!parseXyzData(data, tagIndex[Tag::rXYZ], colorspaceDPtr->toXyz.r))
            return false;
        if (!parseXyzData(data, tagIndex[Tag::gXYZ], colorspaceDPtr->toXyz.g))
            return false;
        if (!parseXyzData(data, tagIndex[Tag::bXYZ], colorspaceDPtr->toXyz.b))
            return false;
        if (!parseXyzData(data, tagIndex[Tag::wtpt], colorspaceDPtr->whitePoint))
            return false;

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
        }
        if (colorspaceDPtr->toXyz == QColorMatrix::toXyzFromProPhotoRgb()) {
            qCDebug(lcIcc) << "fromIccProfile: ProPhoto RGB primaries detected";
            colorspaceDPtr->primaries = QColorSpace::Primaries::ProPhotoRgb;
        }
    } else {
        // We will use sRGB primaries and fit to match the given white-point if
        // it doesn't match sRGB's.
        QColorVector whitePoint;
        if (!parseXyzData(data, tagIndex[Tag::wtpt], whitePoint))
            return false;
        if (!qFuzzyCompare(whitePoint.y, 1.0f) || (1.0f + whitePoint.z - whitePoint.x) == 0.0f) {
            qCWarning(lcIcc) << "fromIccProfile: Invalid ICC profile - gray white-point not normalized";
            return false;
        }
        if (whitePoint == QColorVector::D65()) {
            colorspaceDPtr->primaries = QColorSpace::Primaries::SRgb;
        } else {
            colorspaceDPtr->primaries = QColorSpace::Primaries::Custom;
            // Calculate chromaticity from xyz (assuming y == 1.0f).
            float y = 1.0f / (1.0f + whitePoint.z - whitePoint.x);
            float x = whitePoint.x * y;
            QColorSpacePrimaries primaries(QColorSpace::Primaries::SRgb);
            primaries.whitePoint = QPointF(x,y);
            if (!primaries.areValid()) {
                qCWarning(lcIcc) << "fromIccProfile: Invalid ICC profile - invalid white-point";
                return false;
            }
            colorspaceDPtr->toXyz = primaries.toXyzMatrix();
        }
    }
    // Reset the matrix to our canonical values:
    if (colorspaceDPtr->primaries != QColorSpace::Primaries::Custom)
        colorspaceDPtr->setToXyzMatrix();

    // Parse TRC tags
    TagEntry rTrc;
    TagEntry gTrc;
    TagEntry bTrc;
    if (header.inputColorSpace == uint(ColorSpaceType::Gray)) {
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
    if (!parseTRC(data, rTrc, rCurve)) {
        qCWarning(lcIcc) << "fromIccProfile: Invalid rTRC";
        return false;
    }
    if (!parseTRC(data, gTrc, gCurve)) {
        qCWarning(lcIcc) << "fromIccProfile: Invalid gTRC";
        return false;
    }
    if (!parseTRC(data, bTrc, bCurve)) {
        qCWarning(lcIcc) << "fromIccProfile: Invalid bTRC";
        return false;
    }
    if (rCurve == gCurve && gCurve == bCurve && rCurve.m_type == QColorTrc::Type::Function) {
        if (rCurve.m_fun.isLinear()) {
            qCDebug(lcIcc) << "fromIccProfile: Linear gamma detected";
            colorspaceDPtr->trc[0] = QColorTransferFunction();
            colorspaceDPtr->transferFunction = QColorSpace::TransferFunction::Linear;
            colorspaceDPtr->gamma = 1.0f;
        } else if (rCurve.m_fun.isGamma()) {
            qCDebug(lcIcc) << "fromIccProfile: Simple gamma detected";
            colorspaceDPtr->trc[0] = QColorTransferFunction::fromGamma(rCurve.m_fun.m_g);
            colorspaceDPtr->transferFunction = QColorSpace::TransferFunction::Gamma;
            colorspaceDPtr->gamma = rCurve.m_fun.m_g;
        } else if (rCurve.m_fun.isSRgb()) {
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

    if (tagIndex.contains(Tag::desc)) {
        if (!parseDesc(data, tagIndex[Tag::desc], colorspaceDPtr->description))
            qCWarning(lcIcc) << "fromIccProfile: Failed to parse description";
        else
            qCDebug(lcIcc) << "fromIccProfile: Description" << colorspaceDPtr->description;
    }

    colorspaceDPtr->identifyColorSpace();
    if (colorspaceDPtr->namedColorSpace)
        qCDebug(lcIcc) << "fromIccProfile: Named colorspace detected: " << QColorSpace::NamedColorSpace(colorspaceDPtr->namedColorSpace);

    colorspaceDPtr->iccProfile = data;

    return true;
}

} // namespace QIcc

QT_END_NAMESPACE
