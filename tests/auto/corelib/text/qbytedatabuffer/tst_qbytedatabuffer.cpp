// Copyright (C) 2012 Hewlett-Packard Development Company, L.P.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <private/qbytedata_p.h>
// for QIODEVICE_BUFFERSIZE macro (== 16384):
#include <private/qiodevice_p.h>

class tst_QByteDataBuffer : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void canReadLine();
    void positionHandling();
    void appendBuffer();
    void moveAppendBuffer();
    void readCompleteBuffer_data();
    void readCompleteBuffer();
    void readPartialBuffer_data();
    void readPartialBuffer();
    void readPointer();
private:
    void readBuffer(int size, int readSize);
};

void tst_QByteDataBuffer::canReadLine()
{
    QByteDataBuffer buf;
    buf.append(QByteArray("a"));
    buf.append(QByteArray("\nb"));
    QVERIFY(buf.canReadLine());
    QVERIFY(buf.getChar() == 'a');
    QVERIFY(buf.canReadLine());
    QVERIFY(buf.getChar() == '\n');
    QVERIFY(!buf.canReadLine());
}

void tst_QByteDataBuffer::positionHandling()
{
    QByteDataBuffer buf;
    buf.append(QByteArray("abc"));
    buf.append(QByteArray("def"));

    QCOMPARE(buf.byteAmount(), (qlonglong)6);
    QCOMPARE(buf.sizeNextBlock(), (qlonglong)3);

    QCOMPARE(buf.getChar(), 'a');
    QCOMPARE(buf.byteAmount(), (qlonglong)5);
    QCOMPARE(buf.sizeNextBlock(), (qlonglong)2);

    QVERIFY(!strcmp(buf[0].constData(), "bc"));
    QCOMPARE(buf.getChar(), 'b');
    QCOMPARE(buf.byteAmount(), (qlonglong)4);
    QCOMPARE(buf.sizeNextBlock(), (qlonglong)1);

    QByteArray tmp("ab");
    buf.prepend(tmp);
    QCOMPARE(buf.byteAmount(), (qlonglong)6);
    QVERIFY(!strcmp(buf.readAll().constData(), "abcdef"));
    QCOMPARE(buf.byteAmount(), (qlonglong)0);

    QByteDataBuffer buf2;
    buf2.append(QByteArray("abc"));
    buf2.getChar();
    QCOMPARE(buf2.read(), QByteArray("bc"));
}

void tst_QByteDataBuffer::appendBuffer()
{
    QByteDataBuffer buf;
    QByteArray local("\1\2\3");
    buf.append(local);
    buf.getChar();

    QByteDataBuffer tmp;
    tmp.append(buf);
    QCOMPARE(tmp.readAll(), buf.readAll());
}

void tst_QByteDataBuffer::moveAppendBuffer()
{
    QByteDataBuffer buf;
    buf.append(QByteArray("hello world"));
    QCOMPARE(buf.getChar(), 'h');

    QByteDataBuffer tmp;
    tmp.append(std::move(buf));
    QCOMPARE(tmp.readAll(), "ello world");
}

static QByteArray makeByteArray(int size)
{
    QByteArray array;
    array.resize(size);
    char *data = array.data();
    for (int i = 0; i < size; ++i)
        data[i] = i % 256;
    return array;
}


void tst_QByteDataBuffer::readBuffer(int size, int readSize)
{
    QByteArray data = makeByteArray(size);

    QByteDataBuffer buf;
    buf.append(data);

    QByteArray tmp;
    tmp.resize(size);

    QBENCHMARK_ONCE {
        for (int i = 0; i < (size - 1) / readSize + 1; ++i)
            buf.read(tmp.data() + i * readSize, readSize);
    }

    QCOMPARE(data.size(), tmp.size());
    QCOMPARE(data, tmp);
}

void tst_QByteDataBuffer::readCompleteBuffer_data()
{
    QTest::addColumn<int>("size");
    QTest::newRow("10B") << (int)10;
    QTest::newRow("1MB") << (int)1e6;
    QTest::newRow("5MB") << (int)5e6;
    QTest::newRow("10MB") << (int)10e6;
}

void tst_QByteDataBuffer::readCompleteBuffer()
{
    QFETCH(int, size);
    readBuffer(size, size);
}

void tst_QByteDataBuffer::readPartialBuffer_data()
{
    readCompleteBuffer_data();
}

void tst_QByteDataBuffer::readPartialBuffer()
{
    QFETCH(int, size);
    // QIODevice::readAll() reads in QIODEVICE_BUFFERSIZE size
    // increments.
    readBuffer(size, QIODEVICE_BUFFERSIZE);
}

void tst_QByteDataBuffer::readPointer()
{
    QByteDataBuffer buffer;

    auto view = buffer.readPointer();
    QCOMPARE(view.size(), 0);
    QCOMPARE(view, "");

    buffer.append("Hello");
    buffer.append("World");

    qint64 initialSize = buffer.byteAmount();
    view = buffer.readPointer();

    QCOMPARE(initialSize, buffer.byteAmount());
    QCOMPARE(view.size(), 5);
    QCOMPARE(view, "Hello");

    buffer.advanceReadPointer(2);
    view = buffer.readPointer();

    QCOMPARE(initialSize - 2, buffer.byteAmount());
    QCOMPARE(view.size(), 3);
    QCOMPARE(view, "llo");

    buffer.advanceReadPointer(3);
    view = buffer.readPointer();

    QCOMPARE(initialSize - 5, buffer.byteAmount());
    QCOMPARE(view.size(), 5);
    QCOMPARE(view, "World");

    buffer.advanceReadPointer(5);
    view = buffer.readPointer();

    QVERIFY(buffer.isEmpty());
    QCOMPARE(view.size(), 0);
    QCOMPARE(view, "");

    // Advance past the current view's size
    buffer.append("Hello");
    buffer.append("World");

    buffer.advanceReadPointer(6);
    view = buffer.readPointer();
    QCOMPARE(view, "orld");
    QCOMPARE(buffer.byteAmount(), 4);

    // Advance past the end of all contained data
    buffer.advanceReadPointer(6);
    view = buffer.readPointer();
    QCOMPARE(view, "");
}

QTEST_MAIN(tst_QByteDataBuffer)
#include "tst_qbytedatabuffer.moc"
