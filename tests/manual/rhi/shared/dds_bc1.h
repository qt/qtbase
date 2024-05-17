// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
