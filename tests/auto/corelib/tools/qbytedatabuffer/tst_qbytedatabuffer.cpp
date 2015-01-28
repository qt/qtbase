/****************************************************************************
**
** Copyright (C) 2012 Hewlett-Packard Development Company, L.P.
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
    void readCompleteBuffer_data();
    void readCompleteBuffer();
    void readPartialBuffer_data();
    void readPartialBuffer();
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
    buf.append(QByteArray("\1\2\3"));
    buf.getChar();

    QByteDataBuffer tmp;
    tmp.append(buf);
    QCOMPARE(tmp.readAll(), buf.readAll());
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

QTEST_MAIN(tst_QByteDataBuffer)
#include "tst_qbytedatabuffer.moc"
