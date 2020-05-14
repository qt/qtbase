/****************************************************************************
**
** Copyright (C) 2020 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <QtTest/QtTest>
#include <cbor.h>

namespace {
// A QIODevice that supplies a fixed header followed by a large sequence of
// null bytes up until a pre-determined size.
class LargeIODevice final : public QIODevice
{
public:
    qint64 realSize;
    QByteArray start;

    LargeIODevice(const QByteArray &start, qint64 size, QObject *parent = nullptr)
        : QIODevice(parent), realSize(size), start(start)
    {}

    qint64 size() const override { return realSize; }
    bool isSequential() const override { return false; }

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *, qint64) override { return -1; }
};
};

qint64 LargeIODevice::readData(char *data, qint64 maxlen)
{
    qint64 p = pos();
    if (maxlen > realSize - p)
        maxlen = realSize - p;
    memset(data, '\0', maxlen);

    qint64 fromstart = start.size() - p;
    if (fromstart > maxlen)
        fromstart = maxlen;
    else if (fromstart < 0)
        fromstart = 0;
    if (fromstart)
        memcpy(data, start.constData() + p, fromstart);
    return maxlen;
}

void addValidationLargeData(qsizetype minInvalid, qsizetype maxInvalid)
{
    char toolong[1 + sizeof(qsizetype)];
    for (qsizetype v = maxInvalid; v >= minInvalid; --v) {
        // 0x5a for 32-bit, 0x5b for 64-bit
        toolong[0] = sizeof(v) > 4 ? 0x5b : 0x5a;
        qToBigEndian(v, toolong + 1);

        QTest::addRow("bytearray-too-big-for-qbytearray-%llx", v)
                << QByteArray(toolong, sizeof(toolong)) << 0 << CborErrorDataTooLarge;
        QTest::addRow("bytearray-chunked-too-big-for-qbytearray-%llx", v)
                << ('\x5f' + QByteArray(toolong, sizeof(toolong)) + '\xff')
                << 0 << CborErrorDataTooLarge;
        QTest::addRow("bytearray-2chunked-too-big-for-qbytearray-%llx", v)
                << ("\x5f\x40" + QByteArray(toolong, sizeof(toolong)) + '\xff')
                << 0 << CborErrorDataTooLarge;
        toolong[0] |= 0x20;

        // QCborStreamReader::readString copies to a QByteArray first
        QTest::addRow("string-too-big-for-qbytearray-%llx", v)
                << QByteArray(toolong, sizeof(toolong)) << 0 << CborErrorDataTooLarge;
        QTest::addRow("string-chunked-too-big-for-qbytearray-%llx", v)
                << ('\x7f' + QByteArray(toolong, sizeof(toolong)) + '\xff')
                << 0 << CborErrorDataTooLarge;
        QTest::addRow("string-2chunked-too-big-for-qbytearray-%llx", v)
                << ("\x7f\x60" + QByteArray(toolong, sizeof(toolong)) + '\xff')
                << 0 << CborErrorDataTooLarge;
    }
}

void addValidationHugeDevice(qsizetype byteArrayInvalid, qsizetype stringInvalid)
{
    qRegisterMetaType<QSharedPointer<QIODevice>>();
    QTest::addColumn<QSharedPointer<QIODevice>>("device");
    QTest::addColumn<CborError>("expectedError");

    char buf[1 + sizeof(quint64)];
    auto device = [&buf](QCborStreamReader::Type t, quint64 size) {
        buf[0] = quint8(t) | 0x1b;
        qToBigEndian(size, buf + 1);
        size += sizeof(buf);
        QSharedPointer<QIODevice> p =
                QSharedPointer<LargeIODevice>::create(QByteArray(buf, sizeof(buf)), size);
        return p;
    };

    // do the exact limits
    QTest::newRow("bytearray-just-too-big")
            << device(QCborStreamReader::ByteArray, byteArrayInvalid) << CborErrorDataTooLarge;
    QTest::newRow("string-just-too-big")
            << device(QCborStreamReader::String, stringInvalid) << CborErrorDataTooLarge;

    auto addSize = [=](const char *sizename, qint64 size) {
        if (byteArrayInvalid < size)
            QTest::addRow("bytearray-%s", sizename)
                << device(QCborStreamReader::ByteArray, size) << CborErrorDataTooLarge;
        if (stringInvalid < size)
            QTest::addRow("string-%s", sizename)
                << device(QCborStreamReader::String, size) << CborErrorDataTooLarge;
    };
    addSize("1GB", quint64(1) << 30);
    addSize("2GB", quint64(1) << 31);
    addSize("4GB", quint64(1) << 32);
    addSize("max", std::numeric_limits<qint64>::max() - sizeof(buf));
}
