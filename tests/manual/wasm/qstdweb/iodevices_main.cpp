// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QtCore>
#include <QtCore/private/qstdweb_p.h>

#include <qtwasmtestlib.h>

#include "emscripten.h"

using qstdweb::ArrayBuffer;
using qstdweb::Uint8Array;
using qstdweb::Blob;
using qstdweb::BlobIODevice;
using qstdweb::Uint8ArrayIODevice;

class WasmIoDevicesTest: public QObject
{
    Q_OBJECT

private slots:
    void blobIODevice();
    void uint8ArrayIODevice();
};

// Creates a test arraybuffer with byte values [0..size] % 256 * 2
char testByteValue(int i) { return (i % 256) * 2; }
ArrayBuffer createTestArrayBuffer(int size)
{
    ArrayBuffer buffer(size);
    Uint8Array array(buffer);
    for (int i = 0; i < size; ++i)
        array.val().set(i, testByteValue(i));
    return buffer;
}

void WasmIoDevicesTest::blobIODevice()
{
    if (!qstdweb::canBlockCallingThread()) {
        QtWasmTest::completeTestFunction(QtWasmTest::TestResult::Skip, "requires asyncify");
        return;
    }

    // Create test buffer and BlobIODevice
    const int bufferSize = 16;
    BlobIODevice blobDevice(Blob::fromArrayBuffer(createTestArrayBuffer(bufferSize)));

    // Read back byte for byte from the device
    QWASMVERIFY(blobDevice.open(QIODevice::ReadOnly));
    for (int i = 0; i < bufferSize; ++i) {
        char byte;
        blobDevice.seek(i);
        blobDevice.read(&byte, 1);
        QWASMCOMPARE(byte, testByteValue(i));
    }

    blobDevice.close();
    QWASMVERIFY(!blobDevice.open(QIODevice::WriteOnly));
    QWASMSUCCESS();
}

void WasmIoDevicesTest::uint8ArrayIODevice()
{
    // Create test buffer and Uint8ArrayIODevice
    const int bufferSize = 1024;
    Uint8Array array(createTestArrayBuffer(bufferSize));
    Uint8ArrayIODevice arrayDevice(array);

    // Read back byte for byte from the device
    QWASMVERIFY(arrayDevice.open(QIODevice::ReadWrite));
    for (int i = 0; i < bufferSize; ++i) {
        char byte;
        arrayDevice.seek(i);
        arrayDevice.read(&byte, 1);
        QWASMCOMPARE(byte, testByteValue(i));
    }

    // Write a different set of bytes
    QWASMCOMPARE(arrayDevice.seek(0), true);
    for (int i = 0; i < bufferSize; ++i) {
        char byte = testByteValue(i + 1);
        arrayDevice.seek(i);
        QWASMCOMPARE(arrayDevice.write(&byte, 1), 1);
    }

    // Verify that the original array was updated
    QByteArray copy = QByteArray::fromEcmaUint8Array(array.val());
    for (int i = 0; i < bufferSize; ++i)
        QWASMCOMPARE(copy.at(i), testByteValue(i + 1));

    arrayDevice.close();
    QWASMSUCCESS();
}

int main(int argc, char **argv)
{
    auto testObject = std::make_shared<WasmIoDevicesTest>();
    QtWasmTest::initTestCase<QCoreApplication>(argc, argv, testObject);
    return 0;
}

#include "iodevices_main.moc"

