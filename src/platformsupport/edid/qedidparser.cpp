/****************************************************************************
**
** Copyright (C) 2017 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <QtCore/QFile>

#include "qedidparser_p.h"
#include "qedidvendortable_p.h"

#define EDID_DESCRIPTOR_ALPHANUMERIC_STRING 0xfe
#define EDID_DESCRIPTOR_PRODUCT_NAME 0xfc
#define EDID_DESCRIPTOR_SERIAL_NUMBER 0xff

#define EDID_OFFSET_DATA_BLOCKS 0x36
#define EDID_OFFSET_LAST_BLOCK 0x6c
#define EDID_OFFSET_PNP_ID 0x08
#define EDID_OFFSET_SERIAL 0x0c
#define EDID_PHYSICAL_WIDTH 0x15
#define EDID_OFFSET_PHYSICAL_HEIGHT 0x16

QT_BEGIN_NAMESPACE

QEdidParser::QEdidParser()
{
    // Cache vendors list from pnp.ids
    const QString fileName = QLatin1String("/usr/share/hwdata/pnp.ids");
    if (QFile::exists(fileName)) {
        QFile file(fileName);

        if (file.open(QFile::ReadOnly)) {
            while (!file.atEnd()) {
                QString line = QString::fromUtf8(file.readLine()).trimmed();

                if (line.startsWith(QLatin1Char('#')))
                    continue;

                QStringList parts = line.split(QLatin1Char('\t'));
                if (parts.count() > 1) {
                    QString pnpId = parts.at(0);
                    parts.removeFirst();
                    m_vendorCache[pnpId] = parts.join(QLatin1Char(' '));
                }
            }

            file.close();
        }
    }
}

bool QEdidParser::parse(const QByteArray &blob)
{
    const quint8 *data = reinterpret_cast<const quint8 *>(blob.constData());
    const size_t length = blob.length();

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
    QString pnpIdString = QString::fromLatin1(pnpId, 3);

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
    for (int i = 0; i < 5; ++i) {
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
    manufacturer = m_vendorCache.value(pnpIdString);
    if (manufacturer.isEmpty()) {
        // Find the manufacturer from the vendor lookup table
        for (const auto &vendor : q_edidVendorTable) {
            if (strncmp(vendor.id, pnpId, 3) == 0) {
                manufacturer = QString::fromUtf8(vendor.name);
                break;
            }
        }
    }

    // If we don't know the manufacturer, fallback to PNP ID
    if (manufacturer.isEmpty())
        manufacturer = pnpIdString;

    // Physical size
    physicalSize = QSizeF(data[EDID_PHYSICAL_WIDTH], data[EDID_OFFSET_PHYSICAL_HEIGHT]) * 10;

    return true;
}

QString QEdidParser::parseEdidString(const quint8 *data)
{
    QByteArray buffer(reinterpret_cast<const char *>(data), 13);

    // Erase carriage return and line feed
    buffer = buffer.replace('\r', '\0').replace('\n', '\0');

    // Replace non-printable characters with dash
    for (int i = 0; i < buffer.count(); ++i) {
        if (buffer[i] < '\040' || buffer[i] > '\176')
            buffer[i] = '-';
    }

    return QString::fromLatin1(buffer.trimmed());
}

QT_END_NAMESPACE
