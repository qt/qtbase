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

#include <QtTest/QtTest>

#include <QBuffer>
#include <QByteArray>

class tst_QBuffer : public QObject
{
    Q_OBJECT
private slots:
    void open();
    void getSetCheck();
    void readBlock();
    void readBlockPastEnd();
    void writeBlock_data();
    void writeBlock();
    void seek();
    void seekTest_data();
    void seekTest();
    void read_rawdata();
    void isSequential();
    void signalTest_data();
    void signalTest();
    void isClosedAfterClose();
    void readLine_data();
    void readLine();
    void canReadLine_data();
    void canReadLine();
    void atEnd();
    void readLineBoundaries();
    void getAndUngetChar();
    void writeAfterQByteArrayResize();
    void read_null();

protected slots:
    void readyReadSlot();
    void bytesWrittenSlot(qint64 written);

private:
    qint64 totalBytesWritten;
    bool gotReadyRead;
};

// Testing get/set functions
void tst_QBuffer::getSetCheck()
{
    QBuffer obj1;
    // const QByteArray & QBuffer::data()
    // void QBuffer::setData(const QByteArray &)
    QByteArray var1("Bogus data");
    obj1.setData(var1);
    QCOMPARE(var1, obj1.data());
    obj1.setData(QByteArray());
    QCOMPARE(QByteArray(), obj1.data());
}

void tst_QBuffer::open()
{
    QByteArray data(10, 'f');

    QBuffer b;

    QTest::ignoreMessage(QtWarningMsg, "QBuffer::open: Buffer access not specified");
    QVERIFY(!b.open(QIODevice::NotOpen));
    QVERIFY(!b.isOpen());
    b.close();

    QTest::ignoreMessage(QtWarningMsg, "QBuffer::open: Buffer access not specified");
    QVERIFY(!b.open(QIODevice::Text));
    QVERIFY(!b.isOpen());
    b.close();

    QTest::ignoreMessage(QtWarningMsg, "QBuffer::open: Buffer access not specified");
    QVERIFY(!b.open(QIODevice::Unbuffered));
    QVERIFY(!b.isOpen());
    b.close();

    QVERIFY(b.open(QIODevice::ReadOnly));
    QVERIFY(b.isReadable());
    b.close();

    QVERIFY(b.open(QIODevice::WriteOnly));
    QVERIFY(b.isWritable());
    b.close();

    b.setData(data);
    QVERIFY(b.open(QIODevice::Append));
    QVERIFY(b.isWritable());
    QCOMPARE(b.size(), qint64(10));
    QCOMPARE(b.pos(), b.size());
    b.close();

    b.setData(data);
    QVERIFY(b.open(QIODevice::Truncate));
    QVERIFY(b.isWritable());
    QCOMPARE(b.size(), qint64(0));
    QCOMPARE(b.pos(), qint64(0));
    b.close();

    QVERIFY(b.open(QIODevice::ReadWrite));
    QVERIFY(b.isReadable());
    QVERIFY(b.isWritable());
    b.close();
}

// some status() tests, too
void tst_QBuffer::readBlock()
{
    const int arraySize = 10;
    char a[arraySize];
    QBuffer b;
    QCOMPARE(b.bytesAvailable(), (qint64) 0); // no data
    QCOMPARE(b.read(a, arraySize), (qint64) -1); // not opened
    QVERIFY(b.atEnd());

    QByteArray ba;
    ba.resize(arraySize);
    b.setBuffer(&ba);
    QCOMPARE(b.bytesAvailable(), (qint64) arraySize);
    b.open(QIODevice::WriteOnly);
    QCOMPARE(b.bytesAvailable(), (qint64) arraySize);
    QTest::ignoreMessage(QtWarningMsg, "QIODevice::read (QBuffer): WriteOnly device");
    QCOMPARE(b.read(a, arraySize), (qint64) -1); // no read access
    b.close();

    b.open(QIODevice::ReadOnly);
    QCOMPARE(b.bytesAvailable(), (qint64) arraySize);
    QCOMPARE(b.read(a, arraySize), (qint64) arraySize);
    QVERIFY(b.atEnd());
    QCOMPARE(b.bytesAvailable(), (qint64) 0);

    // up to 3.0.x reading beyond the end was an error while ok
    // this has been made consistent with other QIODevice sub classes in 3.1
    QCOMPARE(b.read(a, 1), qint64(0));
    QVERIFY(b.atEnd());

    // read in two chunks
    b.close();
    b.open(QIODevice::ReadOnly);
    QCOMPARE(b.bytesAvailable(), (qint64) arraySize);
    QCOMPARE(b.read(a, arraySize/2), (qint64) arraySize/2);
    QCOMPARE(b.bytesAvailable(), (qint64) arraySize/2);
    QCOMPARE(b.read(a + arraySize/2, arraySize - arraySize/2),
            (qint64)(arraySize - arraySize/2));
    QVERIFY(b.atEnd());
    QCOMPARE(b.bytesAvailable(), (qint64) 0);
}

void tst_QBuffer::readBlockPastEnd()
{
    QByteArray arr(4096 + 3616, 'd');
    QBuffer buf(&arr);

    buf.open(QIODevice::ReadOnly);
    char dummy[4096];

    buf.read(1);

    QCOMPARE(buf.read(dummy, 4096), qint64(4096));
    QCOMPARE(buf.read(dummy, 4096), qint64(3615));
    QVERIFY(buf.atEnd());
}

void tst_QBuffer::writeBlock_data()
{
    QTest::addColumn<QString>("str");

    QTest::newRow( "small_bytearray" ) << QString("Test");
    QTest::newRow( "large_bytearray" ) << QString(
        "The QBuffer class is an I/O device that operates on a QByteArray.\n"
        "QBuffer is used to read and write to a memory buffer. It is normally "
        "used with a QTextStream or a QDataStream. QBuffer has an associated "
        "QByteArray which holds the buffer data. The size() of the buffer is "
        "automatically adjusted as data is written.\n"
        "The constructor QBuffer(QByteArray) creates a QBuffer using an existing "
        "byte array. The byte array can also be set with setBuffer(). Writing to "
        "the QBuffer will modify the original byte array because QByteArray is "
        "explicitly shared.\n"
        "Use open() to open the buffer before use and to set the mode (read-only, "
        "write-only, etc.). close() closes the buffer. The buffer must be closed "
        "before reopening or calling setBuffer().\n"
        "A common way to use QBuffer is through QDataStream or QTextStream, which "
        "have constructors that take a QBuffer parameter. For convenience, there "
        "are also QDataStream and QTextStream constructors that take a QByteArray "
        "parameter. These constructors create and open an internal QBuffer.\n"
        "Note that QTextStream can also operate on a QString (a Unicode string); a "
        "QBuffer cannot.\n"
        "You can also use QBuffer directly through the standard QIODevice functions "
        "readBlock(), writeBlock() readLine(), at(), getch(), putch() and ungetch().\n"
        "See also QFile, QDataStream, QTextStream, QByteArray, Shared Classes, Collection "
        "Classes and Input/Output and Networking.\n\n"
        "The QBuffer class is an I/O device that operates on a QByteArray.\n"
        "QBuffer is used to read and write to a memory buffer. It is normally "
        "used with a QTextStream or a QDataStream. QBuffer has an associated "
        "QByteArray which holds the buffer data. The size() of the buffer is "
        "automatically adjusted as data is written.\n"
        "The constructor QBuffer(QByteArray) creates a QBuffer using an existing "
        "byte array. The byte array can also be set with setBuffer(). Writing to "
        "the QBuffer will modify the original byte array because QByteArray is "
        "explicitly shared.\n"
        "Use open() to open the buffer before use and to set the mode (read-only, "
        "write-only, etc.). close() closes the buffer. The buffer must be closed "
        "before reopening or calling setBuffer().\n"
        "A common way to use QBuffer is through QDataStream or QTextStream, which "
        "have constructors that take a QBuffer parameter. For convenience, there "
        "are also QDataStream and QTextStream constructors that take a QByteArray "
        "parameter. These constructors create and open an internal QBuffer.\n"
        "Note that QTextStream can also operate on a QString (a Unicode string); a "
        "QBuffer cannot.\n"
        "You can also use QBuffer directly through the standard QIODevice functions "
        "readBlock(), writeBlock() readLine(), at(), getch(), putch() and ungetch().\n"
        "See also QFile, QDataStream, QTextStream, QByteArray, Shared Classes, Collection "
        "Classes and Input/Output and Networking.\n\n"
        "The QBuffer class is an I/O device that operates on a QByteArray.\n"
        "QBuffer is used to read and write to a memory buffer. It is normally "
        "used with a QTextStream or a QDataStream. QBuffer has an associated "
        "QByteArray which holds the buffer data. The size() of the buffer is "
        "automatically adjusted as data is written.\n"
        "The constructor QBuffer(QByteArray) creates a QBuffer using an existing "
        "byte array. The byte array can also be set with setBuffer(). Writing to "
        "the QBuffer will modify the original byte array because QByteArray is "
        "explicitly shared.\n"
        "Use open() to open the buffer before use and to set the mode (read-only, "
        "write-only, etc.). close() closes the buffer. The buffer must be closed "
        "before reopening or calling setBuffer().\n"
        "A common way to use QBuffer is through QDataStream or QTextStream, which "
        "have constructors that take a QBuffer parameter. For convenience, there "
        "are also QDataStream and QTextStream constructors that take a QByteArray "
        "parameter. These constructors create and open an internal QBuffer.\n"
        "Note that QTextStream can also operate on a QString (a Unicode string); a "
        "QBuffer cannot.\n"
        "You can also use QBuffer directly through the standard QIODevice functions "
        "readBlock(), writeBlock() readLine(), at(), getch(), putch() and ungetch().\n"
        "See also QFile, QDataStream, QTextStream, QByteArray, Shared Classes, Collection "
        "Classes and Input/Output and Networking.");
}

void tst_QBuffer::writeBlock()
{
    QFETCH( QString, str );

    QByteArray ba;
    QBuffer buf( &ba );
    buf.open(QIODevice::ReadWrite);
    QByteArray data = str.toLatin1();
    QCOMPARE(buf.write( data.constData(), data.size() ), qint64(data.size()));

    QCOMPARE(buf.data(), str.toLatin1());
}

void tst_QBuffer::seek()
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QCOMPARE(buffer.size(), qint64(0));
    QCOMPARE(buffer.pos(), qint64(0));
    const qint64 pos = 10;
    QVERIFY(buffer.seek(pos));
    QCOMPARE(buffer.size(), pos);
}

void tst_QBuffer::seekTest_data()
{
    writeBlock_data();
}

#define DO_VALID_SEEK(position) {                                            \
    char c;                                                                  \
    QVERIFY(buf.seek(qint64(position)));                                      \
    QCOMPARE(buf.pos(), qint64(position));                                    \
    QVERIFY(buf.getChar(&c));                                                 \
    QCOMPARE(QChar(c), str.at(qint64(position)));                             \
}
#define DO_INVALID_SEEK(position) {                                          \
    qint64 prev_pos = buf.pos();                                             \
    QVERIFY(!buf.seek(qint64(position)));                                     \
    QCOMPARE(buf.pos(), prev_pos); /* position should not be changed */                  \
}

void tst_QBuffer::seekTest()
{
    QFETCH(QString, str);

    QByteArray ba;
    QBuffer buf(&ba);
    QCOMPARE(buf.pos(), qint64(0));

    buf.open(QIODevice::ReadWrite);
    QCOMPARE(buf.pos(), qint64(0));
    QCOMPARE(buf.bytesAvailable(), qint64(0));

    QByteArray data = str.toLatin1();
    QCOMPARE(buf.write( data.constData(), data.size() ), qint64(data.size()));
    QCOMPARE(buf.bytesAvailable(), qint64(0)); // we're at the end
    QCOMPARE(buf.size(), qint64(data.size()));

    QTest::ignoreMessage(QtWarningMsg, "QBuffer::seek: Invalid pos: -1");
    DO_INVALID_SEEK(-1);

    DO_VALID_SEEK(0);
    DO_VALID_SEEK(str.size() - 1);
    QVERIFY(buf.atEnd());
    DO_VALID_SEEK(str.size() / 2);

    // Special case: valid to seek one position past the buffer.
    // Its then legal to write, but not read.
    {
        char c = 'a';
        QVERIFY(buf.seek(qint64(str.size())));
        QCOMPARE(buf.bytesAvailable(), qint64(0));
        QCOMPARE(buf.read(&c, qint64(1)), qint64(0));
        QCOMPARE(c, 'a');
        QCOMPARE(buf.write(&c, qint64(1)), qint64(1));
    }

    // Special case 2: seeking to an arbitrary position beyond the buffer auto-expands it
    {
        char c;
        const int offset = 1; // any positive integer will do
        const qint64 pos = buf.size() + offset;
        QVERIFY(buf.seek(pos));
        QCOMPARE(buf.bytesAvailable(), qint64(0));
        QCOMPARE(buf.pos(), pos);
        QVERIFY(!buf.getChar(&c));
        QVERIFY(buf.seek(pos - 1));
        QVERIFY(buf.getChar(&c));
        QCOMPARE(c, buf.data().at(pos - 1));
        QVERIFY(buf.seek(pos));
        QVERIFY(buf.putChar(c));
    }
}

void tst_QBuffer::read_rawdata()
{
    static const unsigned char mydata[] = {
        0x01, 0x00, 0x03, 0x84, 0x78, 0x9c, 0x3b, 0x76,
        0xec, 0x18, 0xc3, 0x31, 0x0a, 0xf1, 0xcc, 0x99,
        0x6d, 0x5b
    };

    QByteArray data = QByteArray::fromRawData((const char *)mydata, sizeof(mydata));
    QBuffer buffer(&data);
    buffer.open(QIODevice::ReadOnly);
    QDataStream in(&buffer);
    quint8 ch;
    for (int i = 0; i < (int)sizeof(mydata); ++i) {
        QVERIFY(!buffer.atEnd());
        in >> ch;
        QCOMPARE(ch, (quint8)mydata[i]);
    }
    QVERIFY(buffer.atEnd());
}

void tst_QBuffer::isSequential()
{
    QBuffer buf;
    QVERIFY(!buf.isSequential());
}

void tst_QBuffer::signalTest_data()
{
    QTest::addColumn<QByteArray>("sample");

    QTest::newRow("empty") << QByteArray();
    QTest::newRow("size 1") << QByteArray("1");
    QTest::newRow("size 2") << QByteArray("11");
    QTest::newRow("size 100") << QByteArray(100, '1');
}

void tst_QBuffer::signalTest()
{
    QFETCH(QByteArray, sample);

    totalBytesWritten = 0;

    QBuffer buf;
    buf.open(QIODevice::WriteOnly);

    buf.buffer().resize(sample.size() * 10);
    connect(&buf, SIGNAL(readyRead()), this, SLOT(readyReadSlot()));
    connect(&buf, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWrittenSlot(qint64)));

    for (int i = 0; i < 10; ++i) {
        gotReadyRead = false;
        QCOMPARE(buf.write(sample), qint64(sample.size()));
        if (sample.size() > 0) {
            QTestEventLoop::instance().enterLoop(5);
            if (QTestEventLoop::instance().timeout())
                QFAIL("Timed out when waiting for readyRead()");
            QCOMPARE(totalBytesWritten, qint64(sample.size() * (i + 1)));
            QVERIFY(gotReadyRead);
        } else {
            QCOMPARE(totalBytesWritten, qint64(0));
            QVERIFY(!gotReadyRead);
        }
    }
}

void tst_QBuffer::readyReadSlot()
{
    gotReadyRead = true;
    QTestEventLoop::instance().exitLoop();
}

void tst_QBuffer::bytesWrittenSlot(qint64 written)
{
    totalBytesWritten += written;
}

void tst_QBuffer::isClosedAfterClose()
{
    QBuffer buffer;
    buffer.open(QBuffer::ReadOnly);
    QVERIFY(buffer.isOpen());
    buffer.close();
    QVERIFY(!buffer.isOpen());
}

void tst_QBuffer::readLine_data()
{
    QTest::addColumn<QByteArray>("src");
    QTest::addColumn<int>("maxlen");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("1") << QByteArray("line1\nline2\n") << 1024
                    << QByteArray("line1\n");
    QTest::newRow("2") << QByteArray("hi there") << 1024
                    << QByteArray("hi there");
    QTest::newRow("3") << QByteArray("l\n") << 3 << QByteArray("l\n");
    QTest::newRow("4") << QByteArray("l\n") << 2 << QByteArray("l");
}

void tst_QBuffer::readLine()
{
    QFETCH(QByteArray, src);
    QFETCH(int, maxlen);
    QFETCH(QByteArray, expected);

    QBuffer buf;
    buf.setBuffer(&src);
    char *result = new char[maxlen + 1];
    result[maxlen] = '\0';

    QVERIFY(buf.open(QIODevice::ReadOnly));

    qint64 bytes_read = buf.readLine(result, maxlen);

    QCOMPARE(bytes_read, qint64(expected.size()));
    QCOMPARE(QByteArray(result), expected);

    buf.close();
    delete[] result;

}

void tst_QBuffer::canReadLine_data()
{
    QTest::addColumn<QByteArray>("src");
    QTest::addColumn<bool>("expected");

    QTest::newRow("1") << QByteArray("no newline") << false;
    QTest::newRow("2") << QByteArray("two \n lines\n") << true;
    QTest::newRow("3") << QByteArray("\n") << true;
    QTest::newRow("4") << QByteArray() << false;
}

void tst_QBuffer::canReadLine()
{
    QFETCH(QByteArray, src);
    QFETCH(bool, expected);

    QBuffer buf;
    buf.setBuffer(&src);
    QVERIFY(!buf.canReadLine());
    QVERIFY(buf.open(QIODevice::ReadOnly));
    QCOMPARE(buf.canReadLine(), expected);
}

void tst_QBuffer::atEnd()
{
    QBuffer buffer;
    buffer.open(QBuffer::Append);
    buffer.write("heisann");
    buffer.close();

    buffer.open(QBuffer::ReadOnly);
    buffer.seek(buffer.size());
    char c;
    QVERIFY(!buffer.getChar(&c));
    QCOMPARE(buffer.read(&c, 1), qint64(0));
}

// Test that reading data out of a QBuffer a line at a time gives the same
// result as reading the whole buffer at once.
void tst_QBuffer::readLineBoundaries()
{
    QByteArray line = "This is a line\n";
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    while (buffer.size() < 16384)
        buffer.write(line);

    buffer.seek(0);
    QByteArray lineByLine;
    while (!buffer.atEnd())
        lineByLine.append(buffer.readLine());

    buffer.seek(0);
    QCOMPARE(buffer.bytesAvailable(), lineByLine.size());

    QByteArray all = buffer.readAll();
    QCOMPARE(all.size(), lineByLine.size());
    QCOMPARE(all, lineByLine);
}

// Test that any character in a buffer can be read and pushed back.
void tst_QBuffer::getAndUngetChar()
{
    // Create some data in a buffer
    QByteArray line = "This is a line\n";
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    while (buffer.size() < 16384)
        buffer.write(line);

    // Take a copy of the data held in the buffer
    buffer.seek(0);
    QCOMPARE(buffer.bytesAvailable(), buffer.size());
    QByteArray data = buffer.readAll();
    QCOMPARE(buffer.bytesAvailable(), qint64(0));

    // Get and unget each character in order
    for (qint64 i = 0; i < buffer.size(); ++i) {
        buffer.seek(i);
        char c;
        QVERIFY(buffer.getChar(&c));
        QCOMPARE(c, data.at((uint)i));
        buffer.ungetChar(c);
    }

    // Get and unget each character in reverse order
    for (qint64 i = buffer.size() - 1; i >= 0; --i) {
        buffer.seek(i);
        char c;
        QVERIFY(buffer.getChar(&c));
        QCOMPARE(c, data.at((uint)i));
        buffer.ungetChar(c);
    }

    // Verify that the state of the buffer still matches the original data.
    buffer.seek(0);
    QCOMPARE(buffer.bytesAvailable(), data.size());
    QCOMPARE(buffer.readAll(), data);
    QCOMPARE(buffer.bytesAvailable(), qint64(0));
}

void tst_QBuffer::writeAfterQByteArrayResize()
{
    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::WriteOnly));

    buffer.write(QByteArray().fill('a', 1000));
    QCOMPARE(buffer.buffer().size(), 1000);

    // resize the QByteArray behind QBuffer's back
    buffer.buffer().clear();
    buffer.seek(0);
    QCOMPARE(buffer.buffer().size(), 0);

    buffer.write(QByteArray().fill('b', 1000));
    QCOMPARE(buffer.buffer().size(), 1000);
}

void tst_QBuffer::read_null()
{
    QByteArray buffer;
    buffer.resize(32000);
    for (int i = 0; i < buffer.size(); ++i)
        buffer[i] = char(i & 0xff);

    QBuffer in(&buffer);
    in.open(QIODevice::ReadOnly);

    QByteArray chunk;

    chunk.resize(16380);
    in.read(chunk.data(), 16380);

    QCOMPARE(chunk, buffer.mid(0, chunk.size()));

    in.read(chunk.data(), 0);

    chunk.resize(8);
    in.read(chunk.data(), chunk.size());

    QCOMPARE(chunk, buffer.mid(16380, chunk.size()));
}

QTEST_MAIN(tst_QBuffer)
#include "tst_qbuffer.moc"
