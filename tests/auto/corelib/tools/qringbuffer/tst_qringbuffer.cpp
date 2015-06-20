/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <private/qringbuffer_p.h>

class tst_QRingBuffer : public QObject
{
    Q_OBJECT
private slots:
    void readPointerAtPositionWriteRead();
    void readPointerAtPositionEmptyRead();
    void readPointerAtPositionWithHead();
    void readPointerAtPositionReadTooMuch();
    void sizeWhenEmpty();
    void sizeWhenReservedAndChopped();
    void sizeWhenReserved();
    void free();
    void reserveAndRead();
    void reserveFrontAndRead();
    void chop();
    void ungetChar();
    void indexOf();
    void appendAndRead();
    void peek();
    void readLine();
};

void tst_QRingBuffer::sizeWhenReserved()
{
    QRingBuffer ringBuffer;
    ringBuffer.reserve(5);

    QCOMPARE(ringBuffer.size(), Q_INT64_C(5));
}

void tst_QRingBuffer::sizeWhenReservedAndChopped()
{
    QRingBuffer ringBuffer;
    ringBuffer.reserve(31337);
    ringBuffer.chop(31337);

    QCOMPARE(ringBuffer.size(), Q_INT64_C(0));
}

void tst_QRingBuffer::sizeWhenEmpty()
{
    QRingBuffer ringBuffer;

    QCOMPARE(ringBuffer.size(), Q_INT64_C(0));
}

void tst_QRingBuffer::readPointerAtPositionReadTooMuch()
{
    QRingBuffer ringBuffer;

    qint64 length;
    const char *buf = ringBuffer.readPointerAtPosition(42, length);
    QVERIFY(buf == 0);
    QCOMPARE(length, Q_INT64_C(0));
}

void tst_QRingBuffer::readPointerAtPositionWithHead()
{
    QRingBuffer ringBuffer;
    char *buf = ringBuffer.reserve(4);
    memcpy (buf, "0123", 4);
    ringBuffer.free(2);

    // ringBuffer should have stayed the same except
    // its head it had moved to position 2
    qint64 length;
    const char* buf2 = ringBuffer.readPointerAtPosition(0, length);

    QCOMPARE(length, Q_INT64_C(2));
    QCOMPARE(*buf2, '2');
    QCOMPARE(*(buf2 + 1), '3');

    // advance 2 more, ringBuffer should be empty then
    ringBuffer.free(2);
    buf2 = ringBuffer.readPointerAtPosition(0, length);
    QCOMPARE(length, Q_INT64_C(0));
    QVERIFY(buf2 == 0);

    // check buffer with 2 blocks
    memcpy(ringBuffer.reserve(4), "0123", 4);
    ringBuffer.append(QByteArray("45678", 5));
    ringBuffer.free(3);
    buf2 = ringBuffer.readPointerAtPosition(Q_INT64_C(1), length);
    QCOMPARE(length, Q_INT64_C(5));
}

void tst_QRingBuffer::readPointerAtPositionEmptyRead()
{
    QRingBuffer ringBuffer;

    qint64 length;
    const char *buf = ringBuffer.readPointerAtPosition(0, length);
    QVERIFY(buf == 0);
    QCOMPARE(length, Q_INT64_C(0));
}

void tst_QRingBuffer::readPointerAtPositionWriteRead()
{
    //create some data
    QBuffer inData;
    inData.open(QIODevice::ReadWrite);
    inData.putChar(0x42);
    inData.putChar(0x23);
    inData.write("Qt rocks!");
    for (int i = 0; i < 5000; i++)
        inData.write(QString("Number %1").arg(i).toUtf8());
    inData.reset();
    QVERIFY(inData.size() > 0);

    //put the inData in the QRingBuffer
    QRingBuffer ringBuffer;
    qint64 remaining = inData.size();
    while (remaining > 0) {
        // write in chunks of 50 bytes
        // this ensures there will be multiple QByteArrays inside the QRingBuffer
        // since QRingBuffer is then only using individual arrays of around 4000 bytes
        qint64 thisWrite = qMin(remaining, Q_INT64_C(50));
        char *pos = ringBuffer.reserve(thisWrite);
        inData.read(pos, thisWrite);
        remaining -= thisWrite;
    }
    // was data put into it?
    QVERIFY(ringBuffer.size() > 0);
    QCOMPARE(ringBuffer.size(), inData.size());

    //read from the QRingBuffer in loop, put back into another QBuffer
    QBuffer outData;
    outData.open(QIODevice::ReadWrite);
    remaining = ringBuffer.size();
    while (remaining > 0) {
        qint64 thisRead;
        // always try to read as much as possible
        const char *buf = ringBuffer.readPointerAtPosition(ringBuffer.size() - remaining, thisRead);
        outData.write(buf, thisRead);
        remaining -= thisRead;
    }
    outData.reset();

    QVERIFY(outData.size() > 0);

    // was the data read from the QRingBuffer the same as the one written into it?
    QCOMPARE(outData.size(), inData.size());
    QVERIFY(outData.buffer().startsWith(inData.buffer()));
}

void tst_QRingBuffer::free()
{
    QRingBuffer ringBuffer;
    // make three byte arrays with different sizes
    ringBuffer.reserve(4096);
    ringBuffer.reserve(2048);
    ringBuffer.append(QByteArray("01234", 5));

    ringBuffer.free(1);
    QCOMPARE(ringBuffer.size(), Q_INT64_C(4095 + 2048 + 5));
    ringBuffer.free(4096);
    QCOMPARE(ringBuffer.size(), Q_INT64_C(2047 + 5));
    ringBuffer.free(48);
    ringBuffer.free(2000);
    QCOMPARE(ringBuffer.size(), Q_INT64_C(4));
    QVERIFY(memcmp(ringBuffer.readPointer(), "1234", 4) == 0);
}

void tst_QRingBuffer::reserveAndRead()
{
    QRingBuffer ringBuffer;
    // fill buffer with an arithmetic progression
    for (int i = 1; i < 256; ++i) {
        QByteArray ba(i, char(i));
        char *ringPos = ringBuffer.reserve(i);
        QVERIFY(ringPos);
        memcpy(ringPos, ba.constData(), i);
    }

    // readback and check stored data
    for (int i = 1; i < 256; ++i) {
        QByteArray ba;
        ba.resize(i);
        qint64 thisRead = ringBuffer.read(ba.data(), i);
        QCOMPARE(thisRead, qint64(i));
        QCOMPARE(ba.count(char(i)), i);
    }
    QCOMPARE(ringBuffer.size(), Q_INT64_C(0));
}

void tst_QRingBuffer::reserveFrontAndRead()
{
    QRingBuffer ringBuffer;
    // fill buffer with an arithmetic progression
    for (int i = 1; i < 256; ++i) {
        QByteArray ba(i, char(i));
        char *ringPos = ringBuffer.reserveFront(i);
        QVERIFY(ringPos);
        memcpy(ringPos, ba.constData(), i);
    }

    // readback and check stored data
    for (int i = 255; i > 0; --i) {
        QByteArray ba;
        ba.resize(i);
        qint64 thisRead = ringBuffer.read(ba.data(), i);
        QCOMPARE(thisRead, qint64(i));
        QCOMPARE(ba.count(char(i)), i);
    }
    QCOMPARE(ringBuffer.size(), Q_INT64_C(0));
}

void tst_QRingBuffer::chop()
{
    QRingBuffer ringBuffer;
    // make three byte arrays with different sizes
    ringBuffer.append(QByteArray("01234", 5));
    ringBuffer.reserve(2048);
    ringBuffer.reserve(4096);

    ringBuffer.chop(1);
    QCOMPARE(ringBuffer.size(), Q_INT64_C(5 + 2048 + 4095));
    ringBuffer.chop(4096);
    QCOMPARE(ringBuffer.size(), Q_INT64_C(5 + 2047));
    ringBuffer.chop(48);
    ringBuffer.chop(2000);
    QCOMPARE(ringBuffer.size(), Q_INT64_C(4));
    QVERIFY(memcmp(ringBuffer.readPointer(), "0123", 4) == 0);
}

void tst_QRingBuffer::ungetChar()
{
    QRingBuffer ringBuffer(16);
    for (int i = 1; i < 32; ++i)
        ringBuffer.putChar(char(i));

    for (int i = 1; i < 31; ++i) {
        int c = ringBuffer.getChar();
        QCOMPARE(c, 1);
        ringBuffer.getChar();
        ringBuffer.ungetChar(char(c)); // unget first char
    }
    QCOMPARE(ringBuffer.size(), Q_INT64_C(1));
}

void tst_QRingBuffer::indexOf()
{
    QRingBuffer ringBuffer(16);
    for (int i = 1; i < 256; ++i)
        ringBuffer.putChar(char(i));

    for (int i = 1; i < 256; ++i) {
        qint64 index = ringBuffer.indexOf(char(i));
        QCOMPARE(index, qint64(i - 1));
        QCOMPARE(ringBuffer.indexOf(char(i), i), index);
        QCOMPARE(ringBuffer.indexOf(char(i), i - 1), -1); // test for absent char
    }
}

void tst_QRingBuffer::appendAndRead()
{
    QRingBuffer ringBuffer;
    QByteArray ba1("Hello world!");
    QByteArray ba2("Test string.");
    QByteArray ba3("0123456789");
    ringBuffer.append(ba1);
    ringBuffer.append(ba2);
    ringBuffer.append(ba3);

    QCOMPARE(ringBuffer.read(), ba1);
    QCOMPARE(ringBuffer.read(), ba2);
    QCOMPARE(ringBuffer.read(), ba3);
}

void tst_QRingBuffer::peek()
{
    QRingBuffer ringBuffer;
    QByteArray testBuffer;
    // fill buffer with an arithmetic progression
    for (int i = 1; i < 256; ++i) {
        char *ringPos = ringBuffer.reserve(i);
        QVERIFY(ringPos);
        memset(ringPos, i, i);
        testBuffer.append(ringPos, i);
    }

    // check stored data
    QByteArray resultBuffer;
    int peekPosition = testBuffer.size();
    for (int i = 1; i < 256; ++i) {
        QByteArray ba(i, 0);
        peekPosition -= i;
        qint64 thisPeek = ringBuffer.peek(ba.data(), i, peekPosition);
        QCOMPARE(thisPeek, qint64(i));
        resultBuffer.prepend(ba);
    }
    QCOMPARE(resultBuffer, testBuffer);
}

void tst_QRingBuffer::readLine()
{
    QRingBuffer ringBuffer;
    QByteArray ba1("Hello world!\n", 13);
    QByteArray ba2("\n", 1);
    QByteArray ba3("Test string.", 12);
    QByteArray ba4("0123456789", 10);
    ringBuffer.append(ba1);
    ringBuffer.append(ba2);
    ringBuffer.append(ba3 + ba4 + ba2);

    char stringBuf[102];
    stringBuf[101] = 0; // non-crash terminator
    QCOMPARE(ringBuffer.readLine(stringBuf, sizeof(stringBuf) - 2), qint64(ba1.size()));
    QCOMPARE(QByteArray(stringBuf, int(strlen(stringBuf))), ba1);

    // check first empty string reading
    stringBuf[0] = char(0xFF);
    QCOMPARE(ringBuffer.readLine(stringBuf, int(sizeof(stringBuf)) - 2), qint64(ba2.size()));
    QCOMPARE(stringBuf[0], ba2.at(0));

    QCOMPARE(ringBuffer.readLine(stringBuf, int(sizeof(stringBuf)) - 2),
             qint64(ba3.size() + ba4.size() + ba2.size()));
    QCOMPARE(QByteArray(stringBuf, int(strlen(stringBuf))), ba3 + ba4 + ba2);
    QCOMPARE(ringBuffer.size(), Q_INT64_C(0));
}

QTEST_APPLESS_MAIN(tst_QRingBuffer)
#include "tst_qringbuffer.moc"
