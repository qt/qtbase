// Copyright (C) 2017 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/QFile>
#include <QtCore/QByteArrayView>

#include "qedidparser_p.h"
#include "qedidvendortable_p.h"

#define EDID_DESCRIPTOR_ALPHANUMERIC_STRING 0xfe
#define EDID_DESCRIPTOR_PRODUCT_NAME 0xfc
#define EDID_DESCRIPTOR_SERIAL_NUMBER 0xff

#define EDID_DATA_BLOCK_COUNT 4
#define EDID_OFFSET_DATA_BLOCKS 0x36
#define EDID_OFFSET_LAST_BLOCK 0x6c
#define EDID_OFFSET_PNP_ID 0x08
#define EDID_OFFSET_SERIAL 0x0c
#define EDID_PHYSICAL_WIDTH 0x15
#define EDID_OFFSET_PHYSICAL_HEIGHT 0x16
#define EDID_TRANSFER_FUNCTION 0x17
#define EDID_FEATURE_SUPPORT 0x18
#define EDID_CHROMATICITIES_BLOCK 0x19

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QString lookupVendorIdInSystemDatabase(QByteArrayView id)
{
    QString result;

    const QString fileName = "/usr/share/hwdata/pnp.ids"_L1;
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
        return result;

    // On Ubuntu 20.04 the longest line in the file is 85 bytes, so this
    // leaves plenty of room...
    constexpr int MaxLineSize = 512;
    char buf[MaxLineSize];

    while (!file.atEnd()) {
        auto read = file.readLine(buf, MaxLineSize);
        if (read < 0 || read == MaxLineSize) // read error
            break;

        QByteArrayView line(buf, read - 1); // -1 to remove the trailing newline
        if (line.isEmpty())
            continue;

        if (line.startsWith('#'))
            continue;

        auto tabPosition = line.indexOf('\t');
        if (tabPosition <= 0) // no vendor id
            continue;
        if (tabPosition + 1 == line.size()) // no vendor name
            continue;

        if (line.first(tabPosition) == id) {
            auto vendor = line.sliced(tabPosition + 1);
            result = QString::fromUtf8(vendor.data(), vendor.size());
            break;
        }
    }

    return result;
}

bool QEdidParser::parse(const QByteArray &blob)
{
    const quint8 *data = reinterpret_cast<const quint8 *>(blob.constData());
    const size_t length = blob.size();

    // Verify header
    if (length < 128)
        return false;
    if (data[0] != 0x00 || data[1] != 0xff)
        return false;

    /* Decode the PNP ID from three 5 bit words packed into 2 bytes
     * /--08--\/--09--\
     * 7654321076543210
     * |\---/\---/\---/
     * R  C1   C2   C3 */
    char pnpId[3];
    pnpId[0] = 'A' + ((data[EDID_OFFSET_PNP_ID] & 0x7c) / 4) - 1;
    pnpId[1] = 'A' + ((data[EDID_OFFSET_PNP_ID] & 0x3) * 8) + ((data[EDID_OFFSET_PNP_ID + 1] & 0xe0) / 32) - 1;
    pnpId[2] = 'A' + (data[EDID_OFFSET_PNP_ID + 1] & 0x1f) - 1;

    // Clear manufacturer
    manufacturer = QString();

    // Serial number, will be overwritten by an ASCII descriptor
    // when and if it will be found
    quint32 serial = data[EDID_OFFSET_SERIAL]
            + (data[EDID_OFFSET_SERIAL + 1] << 8)
            + (data[EDID_OFFSET_SERIAL + 2] << 16)
            + (data[EDID_OFFSET_SERIAL + 3] << 24);
    if (serial > 0)
        serialNumber = QString::number(serial);
    else
        serialNumber = QString();

    // Parse EDID data
    for (int i = 0; i < EDID_DATA_BLOCK_COUNT; ++i) {
        const uint offset = EDID_OFFSET_DATA_BLOCKS + i * 18;

        if (data[offset] != 0 || data[offset + 1] != 0 || data[offset + 2] != 0)
            continue;

        if (data[offset + 3] == EDID_DESCRIPTOR_PRODUCT_NAME)
            model = parseEdidString(&data[offset + 5]);
        else if (data[offset + 3] == EDID_DESCRIPTOR_ALPHANUMERIC_STRING)
            identifier = parseEdidString(&data[offset + 5]);
        else if (data[offset + 3] == EDID_DESCRIPTOR_SERIAL_NUMBER)
            serialNumber = parseEdidString(&data[offset + 5]);
    }

    // Try to use cache first because it is potentially more updated
    manufacturer = lookupVendorIdInSystemDatabase(pnpId);

    if (manufacturer.isEmpty()) {
        // Find the manufacturer from the vendor lookup table
        const auto compareVendorId = [](const VendorTable &vendor, const char *str)
        {
            return strncmp(vendor.id, str, 3) < 0;
        };

        const auto b = std::begin(q_edidVendorTable);
        const auto e = std::end(q_edidVendorTable);
        auto it = std::lower_bound(b,
                                   e,
                                   pnpId,
                                   compareVendorId);

        if (it != e && strncmp(it->id, pnpId, 3) == 0)
            manufacturer = QString::fromUtf8(it->name);
    }

    // If we don't know the manufacturer, fallback to PNP ID
    if (manufacturer.isEmpty())
        manufacturer = QString::fromUtf8(pnpId, std::size(pnpId));

    // Physical size
    physicalSize = QSizeF(data[EDID_PHYSICAL_WIDTH], data[EDID_OFFSET_PHYSICAL_HEIGHT]) * 10;

    // Gamma and transfer function
    const uint igamma = data[EDID_TRANSFER_FUNCTION];
    if (igamma != 0xff) {
        gamma = 1.0 + (igamma / 100.0f);
        useTables = false;
    } else {
        gamma = 0.0; // Defined in DI-EXT
        useTables = true;
    }
    sRgb = data[EDID_FEATURE_SUPPORT] & 0x04;

    // Chromaticities
    int rx = (data[EDID_CHROMATICITIES_BLOCK] >> 6) & 0x03;
    int ry = (data[EDID_CHROMATICITIES_BLOCK] >> 4) & 0x03;
    int gx = (data[EDID_CHROMATICITIES_BLOCK] >> 2) & 0x03;
    int gy = (data[EDID_CHROMATICITIES_BLOCK] >> 0) & 0x03;
    int bx = (data[EDID_CHROMATICITIES_BLOCK + 1] >> 6) & 0x03;
    int by = (data[EDID_CHROMATICITIES_BLOCK + 1] >> 4) & 0x03;
    int wx = (data[EDID_CHROMATICITIES_BLOCK + 1] >> 2) & 0x03;
    int wy = (data[EDID_CHROMATICITIES_BLOCK + 1] >> 0) & 0x03;
    rx |= data[EDID_CHROMATICITIES_BLOCK + 2] << 2;
    ry |= data[EDID_CHROMATICITIES_BLOCK + 3] << 2;
    gx |= data[EDID_CHROMATICITIES_BLOCK + 4] << 2;
    gy |= data[EDID_CHROMATICITIES_BLOCK + 5] << 2;
    bx |= data[EDID_CHROMATICITIES_BLOCK + 6] << 2;
    by |= data[EDID_CHROMATICITIES_BLOCK + 7] << 2;
    wx |= data[EDID_CHROMATICITIES_BLOCK + 8] << 2;
    wy |= data[EDID_CHROMATICITIES_BLOCK + 9] << 2;

    redChromaticity.setX(rx * (1.0f / 1024.0f));
    redChromaticity.setY(ry * (1.0f / 1024.0f));
    greenChromaticity.setX(gx * (1.0f / 1024.0f));
    greenChromaticity.setY(gy * (1.0f / 1024.0f));
    blueChromaticity.setX(bx * (1.0f / 1024.0f));
    blueChromaticity.setY(by * (1.0f / 1024.0f));
    whiteChromaticity.setX(wx * (1.0f / 1024.0f));
    whiteChromaticity.setY(wy * (1.0f / 1024.0f));

    // Find extensions
    for (uint i = 1; i < length / 128; ++i) {
        uint extensionId = data[i * 128];
        if (extensionId == 0x40) { // DI-EXT
            // 0x0E (sub-pixel layout)
            // 0x20->0x22 (bits per color)
            // 0x51->0x7e Transfer characteristics
            const uchar desc = data[i * 128 + 0x51];
            const uchar len = desc & 0x3f;
            if ((desc & 0xc0) == 0x40) {
                if (len > 45)
                    return false;
                QList<uint16_t> whiteTRC;
                whiteTRC.reserve(len + 1);
                for (uint j = 0; j < len; ++j)
                    whiteTRC[j] = data[0x52 + j] * 0x101;
                whiteTRC[len] = 0xffff;
                tables.append(whiteTRC);
            } else if ((desc & 0xc0) == 0x80) {
                if (len > 15)
                    return false;
                QList<uint16_t> redTRC;
                QList<uint16_t> greenTRC;
                QList<uint16_t> blueTRC;
                blueTRC.reserve(len + 1);
                greenTRC.reserve(len + 1);
                redTRC.reserve(len + 1);
                for (uint j = 0; j < len; ++j)
                    blueTRC[j] = data[0x52 + j] * 0x101;
                blueTRC[len] = 0xffff;
                for (uint j = 0; j < len; ++j)
                    greenTRC[j] = data[0x61 + j] * 0x101;
                greenTRC[len] = 0xffff;
                for (uint j = 0; j < len; ++j)
                    redTRC[j] = data[0x70 + j] * 0x101;
                redTRC[len] = 0xffff;
                tables.append(redTRC);
                tables.append(greenTRC);
                tables.append(blueTRC);
            }
        }
    }

    return true;
}

QString QEdidParser::parseEdidString(const quint8 *data)
{
    QByteArray buffer(reinterpret_cast<const char *>(data), 13);

    for (int i = 0; i < buffer.size(); ++i) {
        // If there are less than 13 characters in the string, the string
        // is terminated with the ASCII code ‘0Ah’ (line feed) and padded
        // with ASCII code ‘20h’ (space). See EDID 1.4, sections 3.10.3.1,
        // 3.10.3.2, and 3.10.3.4.
        if (buffer[i] == '\n') {
            buffer.truncate(i);
            break;
        }

        // Replace non-printable characters with dash
        if (buffer[i] < '\040' || buffer[i] > '\176')
            buffer[i] = '-';
    }

    return QString::fromLatin1(buffer);
}

QT_END_NAMESPACE
