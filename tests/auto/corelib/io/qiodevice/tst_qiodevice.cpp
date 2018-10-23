/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QCoreApplication>
#include <QtNetwork/QtNetwork>
#include <QtTest/QtTest>

#include "../../../network-settings.h"

class tst_QIODevice : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void getSetCheck();
    void constructing_QTcpSocket();
    void constructing_QFile();
    void read_QByteArray();
    void unget();
    void peek();
    void peekAndRead();

    void readLine_data();
    void readLine();

    void readLine2_data();
    void readLine2();

    void readAllKeepPosition();
    void writeInTextMode();
    void skip_data();
    void skip();
    void skipAfterPeek_data();
    void skipAfterPeek();

    void transaction_data();
    void transaction();

private:
    QSharedPointer<QTemporaryDir> m_tempDir;
    QString m_previousCurrent;
};

void tst_QIODevice::initTestCase()
{
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
    QVERIFY(QFileInfo(QStringLiteral("./tst_qiodevice.cpp")).exists()
            || QFile::copy(QStringLiteral(":/tst_qiodevice.cpp"), QStringLiteral("./tst_qiodevice.cpp")));
#endif
    m_previousCurrent = QDir::currentPath();
    m_tempDir = QSharedPointer<QTemporaryDir>::create();
    QVERIFY2(!m_tempDir.isNull(), qPrintable("Could not create temporary directory."));
    QVERIFY2(QDir::setCurrent(m_tempDir->path()), qPrintable("Could not switch current directory"));
}

void tst_QIODevice::cleanupTestCase()
{
    QDir::setCurrent(m_previousCurrent);
}

// Testing get/set functions
void tst_QIODevice::getSetCheck()
{
    // OpenMode QIODevice::openMode()
    // void QIODevice::setOpenMode(OpenMode)
    class MyIODevice : public QTcpSocket {
    public:
        using QTcpSocket::setOpenMode;
    };
    MyIODevice var1;
    var1.setOpenMode(QIODevice::OpenMode(QIODevice::NotOpen));
    QCOMPARE(QIODevice::OpenMode(QIODevice::NotOpen), var1.openMode());
    var1.setOpenMode(QIODevice::OpenMode(QIODevice::ReadWrite));
    QCOMPARE(QIODevice::OpenMode(QIODevice::ReadWrite), var1.openMode());
}

//----------------------------------------------------------------------------------
void tst_QIODevice::constructing_QTcpSocket()
{
#if defined(Q_OS_WINRT)
    QSKIP("Synchronous socket calls are broken on winrt. See QTBUG-40922");
#endif
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No network test server available");

    QTcpSocket socket;
    QIODevice *device = &socket;

    QVERIFY(!device->isOpen());

    socket.connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY(socket.waitForConnected(30000));
    QVERIFY(device->isOpen());
    QCOMPARE(device->readChannelCount(), 1);
    QCOMPARE(device->writeChannelCount(), 1);

    while (!device->canReadLine())
        QVERIFY(device->waitForReadyRead(30000));

    char buf[1024];
    memset(buf, 0, sizeof(buf));
    qlonglong lineLength = device->readLine(buf, sizeof(buf));
    QVERIFY(lineLength > 0);
    QCOMPARE(socket.pos(), qlonglong(0));

    socket.close();
    QCOMPARE(socket.readChannelCount(), 0);
    QCOMPARE(socket.writeChannelCount(), 0);
    socket.connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY(socket.waitForConnected(30000));
    QVERIFY(device->isOpen());

    while (!device->canReadLine())
        QVERIFY(device->waitForReadyRead(30000));

    char buf2[1024];
    memset(buf2, 0, sizeof(buf2));
    QCOMPARE(socket.readLine(buf2, sizeof(buf2)), lineLength);

    char *c1 = buf;
    char *c2 = buf2;
    while (*c1 && *c2) {
        QCOMPARE(*c1, *c2);
        ++c1;
        ++c2;
    }
    QCOMPARE(*c1, *c2);
}

//----------------------------------------------------------------------------------
void tst_QIODevice::constructing_QFile()
{
    QFile file;
    QIODevice *device = &file;

    QVERIFY(!device->isOpen());

    file.setFileName(QFINDTESTDATA("tst_qiodevice.cpp"));
    QVERIFY(file.open(QFile::ReadOnly));
    QVERIFY(device->isOpen());
    QCOMPARE((int) device->openMode(), (int) QFile::ReadOnly);
    QCOMPARE(device->readChannelCount(), 1);
    QCOMPARE(device->writeChannelCount(), 0);

    char buf[1024];
    memset(buf, 0, sizeof(buf));
    qlonglong lineLength = device->readLine(buf, sizeof(buf));
    QVERIFY(lineLength > 0);
    QCOMPARE(file.pos(), lineLength);

    file.seek(0);
    char buf2[1024];
    memset(buf2, 0, sizeof(buf2));
    QCOMPARE(file.readLine(buf2, sizeof(buf2)), lineLength);

    char *c1 = buf;
    char *c2 = buf2;
    while (*c1 && *c2) {
        QCOMPARE(*c1, *c2);
        ++c1;
        ++c2;
    }
    QCOMPARE(*c1, *c2);
}


void tst_QIODevice::read_QByteArray()
{
    QFile f(QFINDTESTDATA("tst_qiodevice.cpp"));
    f.open(QIODevice::ReadOnly);

    QByteArray b = f.read(10);
    QCOMPARE(b.length(), 10);

    b = f.read(256);
    QCOMPARE(b.length(), 256);

    b = f.read(0);
    QCOMPARE(b.length(), 0);
}

//--------------------------------------------------------------------
void tst_QIODevice::unget()
{
    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);
    buffer.write("ZXCV");
    buffer.seek(0);
    QCOMPARE(buffer.read(4), QByteArray("ZXCV"));
    QCOMPARE(buffer.pos(), qint64(4));

    buffer.ungetChar('a');
    buffer.ungetChar('b');
    buffer.ungetChar('c');
    buffer.ungetChar('d');

    QCOMPARE(buffer.pos(), qint64(0));

    char buf[6];
    QCOMPARE(buffer.readLine(buf, 5), qint64(4));
    QCOMPARE(buffer.pos(), qint64(4));
    QCOMPARE(static_cast<const char*>(buf), "dcba");

    buffer.ungetChar('a');
    buffer.ungetChar('b');
    buffer.ungetChar('c');
    buffer.ungetChar('d');

    QCOMPARE(buffer.pos(), qint64(0));

    for (int i = 0; i < 5; ++i) {
        buf[0] = '@';
        buf[1] = '@';
        QTest::ignoreMessage(QtWarningMsg,
                              "QIODevice::readLine (QBuffer): Called with maxSize < 2");
        QCOMPARE(buffer.readLine(buf, 1), qint64(-1));
        QCOMPARE(buffer.readLine(buf, 2), qint64(i < 4 ? 1 : -1));
        switch (i) {
        case 0: QCOMPARE(buf[0], 'd'); break;
        case 1: QCOMPARE(buf[0], 'c'); break;
        case 2: QCOMPARE(buf[0], 'b'); break;
        case 3: QCOMPARE(buf[0], 'a'); break;
        case 4: QCOMPARE(buf[0], '\0'); break;
        }
        QCOMPARE(buf[1], i < 4 ? '\0' : '@');
    }

    buffer.ungetChar('\n');
    QCOMPARE(buffer.readLine(), QByteArray("\n"));

    buffer.seek(1);
    buffer.readLine(buf, 3);
    QCOMPARE(static_cast<const char*>(buf), "XC");

    buffer.seek(4);
    buffer.ungetChar('Q');
    QCOMPARE(buffer.readLine(buf, 3), qint64(1));

#if defined(Q_OS_WINRT)
    QSKIP("Synchronous socket calls are broken on winrt. See QTBUG-40922");
#endif
    for (int i = 0; i < 2; ++i) {
        QTcpSocket socket;
        QIODevice *dev;
        QByteArray result;
        const char *lineResult;
        if (i == 0) {
            dev = &buffer;
            result = QByteArray("ZXCV");
            lineResult = "ZXCV";
        } else {
            if (!QtNetworkSettings::verifyTestNetworkSettings())
                QSKIP("No network test server available");
            socket.connectToHost(QtNetworkSettings::serverName(), 80);
            socket.write("GET / HTTP/1.0\r\n\r\n");
            QVERIFY(socket.waitForReadyRead());
            dev = &socket;
            result = QByteArray("HTTP");
            lineResult = "Date";
        }
        char ch, ch2;
        dev->seek(0);
        dev->getChar(&ch);
        dev->ungetChar(ch);
        QCOMPARE(dev->peek(4), result);
        dev->getChar(&ch);
        dev->getChar(&ch2);
        dev->ungetChar(ch2);
        dev->ungetChar(ch);
        QCOMPARE(dev->read(1), result.left(1));
        QCOMPARE(dev->read(3), result.right(3));

        if (i == 0)
            dev->seek(0);
        else
            dev->readLine();
        dev->getChar(&ch);
        dev->ungetChar(ch);
        dev->readLine(buf, 5);
        QCOMPARE(static_cast<const char*>(buf), lineResult);

        if (i == 1)
            socket.close();
    }
}

//--------------------------------------------------------------------
void tst_QIODevice::peek()
{
    QBuffer buffer;
    QFile::remove("peektestfile");
    QFile file("peektestfile");

    for (int i = 0; i < 2; ++i) {
        QIODevice *device = i ? (QIODevice *)&file : (QIODevice *)&buffer;

        device->open(QBuffer::ReadWrite);
        device->write("ZXCV");

        device->seek(0);
        QCOMPARE(device->peek(4), QByteArray("ZXCV"));
        QCOMPARE(device->pos(), qint64(0));
        device->write("ABCDE");
        device->seek(3);
        QCOMPARE(device->peek(1), QByteArray("D"));
        QCOMPARE(device->peek(5), QByteArray("DE"));
        device->seek(0);
        QCOMPARE(device->read(4), QByteArray("ABCD"));
        QCOMPARE(device->pos(), qint64(4));

        device->seek(0);
        device->write("ZXCV");
        device->seek(0);
        char buf[5];
        buf[4] = 0;
        device->peek(buf, 4);
        QCOMPARE(static_cast<const char *>(buf), "ZXCV");
        QCOMPARE(device->pos(), qint64(0));
        device->read(buf, 4);
        QCOMPARE(static_cast<const char *>(buf), "ZXCV");
        QCOMPARE(device->pos(), qint64(4));
    }
    QFile::remove("peektestfile");
}

void tst_QIODevice::peekAndRead()
{
    QByteArray originalData;
    for (int i=0;i<1000;i++)
        originalData += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QBuffer buffer;
    QFile::remove("peektestfile");
    QFile file("peektestfile");

    for (int i = 0; i < 2; ++i) {
        QByteArray readData;
        QIODevice *device = i ? (QIODevice *)&file : (QIODevice *)&buffer;
        device->open(QBuffer::ReadWrite);
        device->write(originalData);
        device->seek(0);
        while (!device->atEnd()) {
            char peekIn[26];
            device->peek(peekIn, 26);
            readData += device->read(26);
        }
        QCOMPARE(readData, originalData);
    }
    QFile::remove("peektestfile");
}

void tst_QIODevice::readLine_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("0") << QByteArray("\nAA");
    QTest::newRow("1") << QByteArray("A\nAA");

    QByteArray data(9000, 'A');
    data[8193] = '\n';
    QTest::newRow("8194") << data;
    data[8193] = 'A';
    data[8192] = '\n';
    QTest::newRow("8193") << data;
    data[8192] = 'A';
    data[8191] = '\n';
    QTest::newRow("8192") << data;
    data[8191] = 'A';
    data[8190] = '\n';
    QTest::newRow("8191") << data;

    data[5999] = '\n';
    QTest::newRow("6000") << data;

    data[4095] = '\n';
    QTest::newRow("4096") << data;

    data[4094] = '\n';
    data[4095] = 'A';
    QTest::newRow("4095") << data;
}

void tst_QIODevice::readLine()
{
    QFETCH(QByteArray, data);
    QBuffer buffer(&data);
    QVERIFY(buffer.open(QIODevice::ReadWrite));
    QVERIFY(buffer.canReadLine());

    int linelen = data.indexOf('\n') + 1;
    QByteArray line;
    line.reserve(linelen + 100);

    int result = buffer.readLine(line.data(), linelen + 100);
    QCOMPARE(result, linelen);

    // try the exact length of the line (plus terminating \0)
    QVERIFY(buffer.seek(0));
    result = buffer.readLine(line.data(), linelen + 1);
    QCOMPARE(result, linelen);

    // try with a line length limit
    QVERIFY(buffer.seek(0));
    line = buffer.readLine(linelen + 100);
    QCOMPARE(line.size(), linelen);

    // try without a length limit
    QVERIFY(buffer.seek(0));
    line = buffer.readLine();
    QCOMPARE(line.size(), linelen);
}

void tst_QIODevice::readLine2_data()
{
    QTest::addColumn<QByteArray>("line");

    QTest::newRow("1024 - 4") << QByteArray(1024 - 4, 'x');
    QTest::newRow("1024 - 3") << QByteArray(1024 - 3, 'x');
    QTest::newRow("1024 - 2") << QByteArray(1024 - 2, 'x');
    QTest::newRow("1024 - 1") << QByteArray(1024 - 1, 'x');
    QTest::newRow("1024"    ) << QByteArray(1024    , 'x');
    QTest::newRow("1024 + 1") << QByteArray(1024 + 1, 'x');
    QTest::newRow("1024 + 2") << QByteArray(1024 + 2, 'x');

    QTest::newRow("4096 - 4") << QByteArray(4096 - 4, 'x');
    QTest::newRow("4096 - 3") << QByteArray(4096 - 3, 'x');
    QTest::newRow("4096 - 2") << QByteArray(4096 - 2, 'x');
    QTest::newRow("4096 - 1") << QByteArray(4096 - 1, 'x');
    QTest::newRow("4096"    ) << QByteArray(4096    , 'x');
    QTest::newRow("4096 + 1") << QByteArray(4096 + 1, 'x');
    QTest::newRow("4096 + 2") << QByteArray(4096 + 2, 'x');

    QTest::newRow("8192 - 4") << QByteArray(8192 - 4, 'x');
    QTest::newRow("8192 - 3") << QByteArray(8192 - 3, 'x');
    QTest::newRow("8192 - 2") << QByteArray(8192 - 2, 'x');
    QTest::newRow("8192 - 1") << QByteArray(8192 - 1, 'x');
    QTest::newRow("8192"    ) << QByteArray(8192    , 'x');
    QTest::newRow("8192 + 1") << QByteArray(8192 + 1, 'x');
    QTest::newRow("8192 + 2") << QByteArray(8192 + 2, 'x');

    QTest::newRow("16384 - 4") << QByteArray(16384 - 4, 'x');
    QTest::newRow("16384 - 3") << QByteArray(16384 - 3, 'x');
    QTest::newRow("16384 - 2") << QByteArray(16384 - 2, 'x');
    QTest::newRow("16384 - 1") << QByteArray(16384 - 1, 'x');
    QTest::newRow("16384"    ) << QByteArray(16384    , 'x');
    QTest::newRow("16384 + 1") << QByteArray(16384 + 1, 'x');
    QTest::newRow("16384 + 2") << QByteArray(16384 + 2, 'x');

    QTest::newRow("20000") << QByteArray(20000, 'x');

    QTest::newRow("32768 - 4") << QByteArray(32768 - 4, 'x');
    QTest::newRow("32768 - 3") << QByteArray(32768 - 3, 'x');
    QTest::newRow("32768 - 2") << QByteArray(32768 - 2, 'x');
    QTest::newRow("32768 - 1") << QByteArray(32768 - 1, 'x');
    QTest::newRow("32768"    ) << QByteArray(32768    , 'x');
    QTest::newRow("32768 + 1") << QByteArray(32768 + 1, 'x');
    QTest::newRow("32768 + 2") << QByteArray(32768 + 2, 'x');

    QTest::newRow("40000") << QByteArray(40000, 'x');
}

void tst_QIODevice::readLine2()
{
    QFETCH(QByteArray, line);

    int length = line.size();

    QByteArray data("First line.\r\n");
    data.append(line);
    data.append("\r\n");
    data.append(line);
    data.append("\r\n");
    data.append("\r\n0123456789");

    {
        QBuffer buffer(&data);
        buffer.open(QIODevice::ReadOnly);

        buffer.seek(0);
        QByteArray temp;
        temp.resize(64536);
        QCOMPARE(buffer.readLine(temp.data(), temp.size()), qint64(13));
        QCOMPARE(buffer.readLine(temp.data(), temp.size()), qint64(length + 2));
        QCOMPARE(buffer.readLine(temp.data(), temp.size()), qint64(length + 2));
        QCOMPARE(buffer.readLine(temp.data(), temp.size()), qint64(2));
        QCOMPARE(buffer.readLine(temp.data(), temp.size()), qint64(10));
        QCOMPARE(buffer.readLine(temp.data(), temp.size()), qint64(-1));

        buffer.seek(0);
        QCOMPARE(buffer.readLine().size(), 13);
        QCOMPARE(buffer.readLine().size(), length + 2);
        QCOMPARE(buffer.readLine().size(), length + 2);
        QCOMPARE(buffer.readLine().size(), 2);
        QCOMPARE(buffer.readLine().size(), 10);
        QVERIFY(buffer.readLine().isNull());
    }

    {
        QBuffer buffer(&data);
        buffer.open(QIODevice::ReadOnly | QIODevice::Text);

        buffer.seek(0);
        QByteArray temp;
        temp.resize(64536);
        QCOMPARE(buffer.readLine(temp.data(), temp.size()), qint64(12));
        QCOMPARE(buffer.readLine(temp.data(), temp.size()), qint64(length + 1));
        QCOMPARE(buffer.readLine(temp.data(), temp.size()), qint64(length + 1));
        QCOMPARE(buffer.readLine(temp.data(), temp.size()), qint64(1));
        QCOMPARE(buffer.readLine(temp.data(), temp.size()), qint64(10));
        QCOMPARE(buffer.readLine(temp.data(), temp.size()), qint64(-1));

        buffer.seek(0);
        QCOMPARE(buffer.readLine().size(), 12);
        QCOMPARE(buffer.readLine().size(), length + 1);
        QCOMPARE(buffer.readLine().size(), length + 1);
        QCOMPARE(buffer.readLine().size(), 1);
        QCOMPARE(buffer.readLine().size(), 10);
        QVERIFY(buffer.readLine().isNull());
    }
}

class SequentialReadBuffer : public QIODevice
{
public:
    SequentialReadBuffer(const char *data)
        : QIODevice(), buf(new QByteArray(data)), offset(0), ownbuf(true) { }
    SequentialReadBuffer(QByteArray *byteArray)
        : QIODevice(), buf(byteArray), offset(0), ownbuf(false) { }
    virtual ~SequentialReadBuffer() { if (ownbuf) delete buf; }

    bool isSequential() const override { return true; }
    const QByteArray &buffer() const { return *buf; }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        maxSize = qMin(maxSize, qint64(buf->size() - offset));
        memcpy(data, buf->constData() + offset, maxSize);
        offset += maxSize;
        return maxSize;
    }
    qint64 writeData(const char * /* data */, qint64 /* maxSize */) override
    {
        return -1;
    }

private:
    QByteArray *buf;
    int offset;
    bool ownbuf;
};

// Test readAll() on position change for sequential device
void tst_QIODevice::readAllKeepPosition()
{
    SequentialReadBuffer buffer("Hello world!");
    buffer.open(QIODevice::ReadOnly);
    char c;

    QCOMPARE(buffer.readChannelCount(), 1);
    QCOMPARE(buffer.writeChannelCount(), 0);
    QVERIFY(buffer.getChar(&c));
    QCOMPARE(buffer.pos(), qint64(0));
    buffer.ungetChar(c);
    QCOMPARE(buffer.pos(), qint64(0));

    QByteArray resultArray = buffer.readAll();
    QCOMPARE(buffer.pos(), qint64(0));
    QCOMPARE(resultArray, buffer.buffer());
}

class RandomAccessBuffer : public QIODevice
{
public:
    RandomAccessBuffer(const char *data) : QIODevice(), buf(data) { }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        maxSize = qMin(maxSize, qint64(buf.size() - pos()));
        memcpy(data, buf.constData() + pos(), maxSize);
        return maxSize;
    }
    qint64 writeData(const char *data, qint64 maxSize) override
    {
        maxSize = qMin(maxSize, qint64(buf.size() - pos()));
        memcpy(buf.data() + pos(), data, maxSize);
        return maxSize;
    }

private:
    QByteArray buf;
};

// Test write() on skipping correct number of bytes in read buffer
void tst_QIODevice::writeInTextMode()
{
    // Unlike other platforms, Windows implementation expands '\n' into
    // "\r\n" sequence in write(). Ensure that write() properly works with
    // a read buffer on random-access devices.
#ifndef Q_OS_WIN
    QSKIP("This is a Windows-only test");
#else
    RandomAccessBuffer buffer("one\r\ntwo\r\nthree\r\n");
    buffer.open(QBuffer::ReadWrite | QBuffer::Text);
    QCOMPARE(buffer.readLine(), QByteArray("one\n"));
    QCOMPARE(buffer.write("two\n"), 4);
    QCOMPARE(buffer.readLine(), QByteArray("three\n"));
#endif
}

void tst_QIODevice::skip_data()
{
    QTest::addColumn<bool>("sequential");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<int>("read");
    QTest::addColumn<int>("skip");
    QTest::addColumn<int>("skipped");
    QTest::addColumn<char>("expect");

    QByteArray bigData;
    bigData.fill('a', 20000);
    bigData[10001] = 'x';

    bool sequential = true;
    do {
        QByteArray devName(sequential ? "sequential" : "random-access");

        QTest::newRow(qPrintable(devName + "-small_data")) << true  << QByteArray("abcdefghij")
                                                           << 3 << 6 << 6 << 'j';
        QTest::newRow(qPrintable(devName + "-big_data")) << true  << bigData
                                                         << 1 << 10000 << 10000 << 'x';
        QTest::newRow(qPrintable(devName + "-beyond_the_end")) << true  << bigData
                                                               << 1 << 20000 << 19999 << '\0';

        sequential = !sequential;
    } while (!sequential);
}

void tst_QIODevice::skip()
{
    QFETCH(bool, sequential);
    QFETCH(QByteArray, data);
    QFETCH(int, read);
    QFETCH(int, skip);
    QFETCH(int, skipped);
    QFETCH(char, expect);
    char lastChar = 0;

    QScopedPointer<QIODevice> dev(sequential ? (QIODevice *) new SequentialReadBuffer(&data)
                                             : (QIODevice *) new QBuffer(&data));
    dev->open(QIODevice::ReadOnly);

    for (int i = 0; i < read; ++i)
        dev->getChar(nullptr);

    QCOMPARE(dev->skip(skip), skipped);
    dev->getChar(&lastChar);
    QCOMPARE(lastChar, expect);
}

void tst_QIODevice::skipAfterPeek_data()
{
    QTest::addColumn<bool>("sequential");
    QTest::addColumn<QByteArray>("data");

    QByteArray bigData;
    for (int i = 0; i < 1000; ++i)
        bigData += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    QTest::newRow("sequential") << true  << bigData;
    QTest::newRow("random-access") << false << bigData;
}

void tst_QIODevice::skipAfterPeek()
{
    QFETCH(bool, sequential);
    QFETCH(QByteArray, data);

    QScopedPointer<QIODevice> dev(sequential ? (QIODevice *) new SequentialReadBuffer(&data)
                                             : (QIODevice *) new QBuffer(&data));
    int readSoFar = 0;
    qint64 bytesToSkip = 1;

    dev->open(QIODevice::ReadOnly);
    forever {
        QByteArray chunk = dev->peek(bytesToSkip);
        if (chunk.isEmpty())
            break;

        QCOMPARE(dev->skip(bytesToSkip), qint64(chunk.size()));
        QCOMPARE(chunk, data.mid(readSoFar, chunk.size()));
        readSoFar += chunk.size();
        bytesToSkip <<= 1;
    }
    QCOMPARE(readSoFar, data.size());
}

void tst_QIODevice::transaction_data()
{
    QTest::addColumn<bool>("sequential");
    QTest::addColumn<qint8>("i8Data");
    QTest::addColumn<qint16>("i16Data");
    QTest::addColumn<qint32>("i32Data");
    QTest::addColumn<qint64>("i64Data");
    QTest::addColumn<bool>("bData");
    QTest::addColumn<float>("fData");
    QTest::addColumn<double>("dData");
    QTest::addColumn<QByteArray>("strData");

    bool sequential = true;
    do {
        QByteArray devName(sequential ? "sequential" : "random-access");

        QTest::newRow(qPrintable(devName + '1')) << sequential << qint8(1) << qint16(2)
                                                 << qint32(3) << qint64(4) << true
                                                 << 5.0f << double(6.0)
                                                 << QByteArray("Hello world!");
        QTest::newRow(qPrintable(devName + '2')) << sequential << qint8(1 << 6) << qint16(1 << 14)
                                                 << qint32(1 << 30) << (qint64(1) << 62) << false
                                                 << 123.0f << double(234.0)
                                                 << QByteArray("abcdefghijklmnopqrstuvwxyz");
        QTest::newRow(qPrintable(devName + '3')) << sequential << qint8(-1) << qint16(-2)
                                                 << qint32(-3) << qint64(-4) << true
                                                 << -123.0f << double(-234.0)
                                                 << QByteArray("Qt rocks!");
        sequential = !sequential;
    } while (!sequential);
}

// Test transaction integrity
void tst_QIODevice::transaction()
{
    QByteArray testBuffer;

    QFETCH(bool, sequential);
    QFETCH(qint8, i8Data);
    QFETCH(qint16, i16Data);
    QFETCH(qint32, i32Data);
    QFETCH(qint64, i64Data);
    QFETCH(bool, bData);
    QFETCH(float, fData);
    QFETCH(double, dData);
    QFETCH(QByteArray, strData);

    {
        QDataStream stream(&testBuffer, QIODevice::WriteOnly);

        stream << i8Data << i16Data << i32Data << i64Data
               << bData << fData << dData << strData.constData();
    }

    for (int splitPos = 0; splitPos <= testBuffer.size(); ++splitPos) {
        QByteArray readBuffer(testBuffer.left(splitPos));
        QIODevice *dev = sequential ? (QIODevice *) new SequentialReadBuffer(&readBuffer)
                                    : (QIODevice *) new QBuffer(&readBuffer);
        dev->open(QIODevice::ReadOnly);
        QDataStream stream(dev);

        qint8 i8;
        qint16 i16;
        qint32 i32;
        qint64 i64;
        bool b;
        float f;
        double d;
        char *str;

        forever {
            QVERIFY(!dev->isTransactionStarted());
            dev->startTransaction();
            QVERIFY(dev->isTransactionStarted());

            // Try to read all data in one go. If the status of the data stream
            // indicates an unsuccessful operation, restart a read transaction
            // on the completed buffer.
            stream >> i8 >> i16 >> i32 >> i64 >> b >> f >> d >> str;

            QVERIFY(stream.atEnd());
            if (stream.status() == QDataStream::Ok) {
                dev->commitTransaction();
                break;
            }

            dev->rollbackTransaction();
            QVERIFY(splitPos == 0 || !stream.atEnd());
            QCOMPARE(dev->pos(), Q_INT64_C(0));
            QCOMPARE(dev->bytesAvailable(), qint64(readBuffer.size()));
            QVERIFY(readBuffer.size() < testBuffer.size());
            delete [] str;
            readBuffer.append(testBuffer.right(testBuffer.size() - splitPos));
            stream.resetStatus();
        }

        QVERIFY(!dev->isTransactionStarted());
        QVERIFY(stream.atEnd());
        QCOMPARE(i8, i8Data);
        QCOMPARE(i16, i16Data);
        QCOMPARE(i32, i32Data);
        QCOMPARE(i64, i64Data);
        QCOMPARE(b, bData);
        QCOMPARE(f, fData);
        QCOMPARE(d, dData);
        QVERIFY(strData == str);
        delete [] str;
        stream.setDevice(0);
        delete dev;
    }
}

QTEST_MAIN(tst_QIODevice)
#include "tst_qiodevice.moc"
