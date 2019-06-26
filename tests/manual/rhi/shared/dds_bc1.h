/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

static const quint32 DDS_MAGIC = 0x20534444; // 'DDS '
static const quint32 DDS_FOURCC = 4;

#define FOURCC(c0, c1, c2, c3) ((c0) | ((c1) << 8) | ((c2) << 16) | ((c3 << 24)))

struct DDS_PIXELFORMAT {
    quint32 size;
    quint32 flags;
    quint32 fourCC;
    quint32 rgbBitCount;
    quint32 rBitMask;
    quint32 gBitMask;
    quint32 bBitMask;
    quint32 aBitMask;
};

struct DDS_HEADER {
    quint32 size;
    quint32 flags;
    quint32 height;
    quint32 width;
    quint32 pitch;
    quint32 depth;
    quint32 mipMapCount;
    quint32 reserved1[11];
    DDS_PIXELFORMAT pixelFormat;
    quint32 caps;
    quint32 caps2;
    quint32 caps3;
    quint32 caps4;
    quint32 reserved2;
};

static quint32 bc1size(const QSize &size)
{
    static const quint32 blockSize = 8; // 8 bytes for BC1
    const quint32 bytesPerLine = qMax<quint32>(1, (size.width() + 3) / 4) * blockSize;
    const quint32 ySize = qMax<quint32>(1, (size.height() + 3) / 4);
    return bytesPerLine * ySize;
}

static QByteArrayList loadBC1(const QString &filename, QSize *size)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("Failed to open %s", qPrintable(filename));
        return QByteArrayList();
    }

    quint32 magic = 0;
    f.read(reinterpret_cast<char *>(&magic), sizeof(magic));
    if (magic != DDS_MAGIC) {
        qWarning("%s is not a DDS file", qPrintable(filename));
        return QByteArrayList();
    }
    DDS_HEADER header;
    f.read(reinterpret_cast<char *>(&header), sizeof(header));
    if (header.size != sizeof(DDS_HEADER)) {
        qWarning("Invalid DDS header size");
        return QByteArrayList();
    }
    if (header.pixelFormat.size != sizeof(DDS_PIXELFORMAT)) {
        qWarning("Invalid DDS pixel format size");
        return QByteArrayList();
    }
    if (!(header.pixelFormat.flags & DDS_FOURCC)) {
        qWarning("Invalid DDS pixel format");
        return QByteArrayList();
    }
    if (header.pixelFormat.fourCC != FOURCC('D', 'X', 'T', '1')) {
        qWarning("Only DXT1 (BC1) is supported");
        return QByteArrayList();
    }

    QByteArrayList data;
    QSize sz(header.width, header.height);
    for (quint32 level = 0; level < header.mipMapCount; ++level) {
        data.append(f.read(bc1size(sz)));
        sz.setWidth(qMax(1, sz.width() / 2));
        sz.setHeight(qMax(1, sz.height() / 2));
    }

    if (size)
        *size = QSize(header.width, header.height);

    return data;
}
