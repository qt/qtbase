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
#include <QtCore/QBuffer>
#include <QtCore/QDataStream>

#include "qbitarray.h"

/**
 * Helper function to initialize a bitarray from a string
 */
static QBitArray QStringToQBitArray(const QString &str)
{
    QBitArray ba;
    ba.resize(str.length());
    int i;
    QChar tru('1');
    for (i = 0; i < str.length(); i++)
    {
        if (str.at(i) == tru)
        {
            ba.setBit(i, true);
        }
    }
    return ba;
}

class tst_QBitArray : public QObject
{
    Q_OBJECT
private slots:
    void size_data();
    void size();
    void countBits_data();
    void countBits();
    void countBits2();
    void isEmpty();
    void swap();
    void fill();
    void toggleBit_data();
    void toggleBit();
    // operator &=
    void operator_andeq_data();
    void operator_andeq();
    // operator |=
    void operator_oreq_data();
    void operator_oreq();
    // operator ^=
    void operator_xoreq_data();
    void operator_xoreq();
    // operator ~
    void operator_neg_data();
    void operator_neg();
    void datastream_data();
    void datastream();
    void invertOnNull() const;
    void operator_noteq_data();
    void operator_noteq();

    void resize();
    void fromBits_data();
    void fromBits();
};

void tst_QBitArray::size_data()
{
    //create the testtable instance and define the elements
    QTest::addColumn<int>("count");
    QTest::addColumn<QString>("res");

    //next we fill it with data
    QTest::newRow( "data0" )  << 1 << QString("1");
    QTest::newRow( "data1" )  << 2 << QString("11");
    QTest::newRow( "data2" )  << 3 << QString("111");
    QTest::newRow( "data3" )  << 9 << QString("111111111");
    QTest::newRow( "data4" )  << 10 << QString("1111111111");
    QTest::newRow( "data5" )  << 17 << QString("11111111111111111");
    QTest::newRow( "data6" )  << 18 << QString("111111111111111111");
    QTest::newRow( "data7" )  << 19 << QString("1111111111111111111");
    QTest::newRow( "data8" )  << 20 << QString("11111111111111111111");
    QTest::newRow( "data9" )  << 21 << QString("111111111111111111111");
    QTest::newRow( "data10" )  << 22 << QString("1111111111111111111111");
    QTest::newRow( "data11" )  << 23 << QString("11111111111111111111111");
    QTest::newRow( "data12" )  << 24 << QString("111111111111111111111111");
    QTest::newRow( "data13" )  << 25 << QString("1111111111111111111111111");
    QTest::newRow( "data14" )  << 32 << QString("11111111111111111111111111111111");
}

void tst_QBitArray::size()
{
    QFETCH(int,count);

    QString S;
    QBitArray a(count);
    a.fill(1);
    int len = a.size();
    for (int j=0; j<len; j++) {
        bool b = a[j];
        if (b)
            S+= QLatin1Char('1');
        else
            S+= QLatin1Char('0');
    }
    QTEST(S,"res");
}

void tst_QBitArray::countBits_data()
{
    QTest::addColumn<QString>("bitField");
    QTest::addColumn<int>("numBits");
    QTest::addColumn<int>("onBits");

    QTest::newRow("empty") << QString() << 0 << 0;
    QTest::newRow("1") << QString("1") << 1 << 1;
    QTest::newRow("101") << QString("101") << 3 << 2;
    QTest::newRow("101100001") << QString("101100001") << 9 << 4;
    QTest::newRow("101100001101100001") << QString("101100001101100001") << 18 << 8;
    QTest::newRow("101100001101100001101100001101100001") << QString("101100001101100001101100001101100001") << 36 << 16;
    QTest::newRow("00000000000000000000000000000000000") << QString("00000000000000000000000000000000000") << 35 << 0;
    QTest::newRow("11111111111111111111111111111111111") << QString("11111111111111111111111111111111111") << 35 << 35;
    QTest::newRow("11111111111111111111111111111111") << QString("11111111111111111111111111111111") << 32 << 32;
    QTest::newRow("11111111111111111111111111111111111111111111111111111111")
        << QString("11111111111111111111111111111111111111111111111111111111") << 56 << 56;
    QTest::newRow("00000000000000000000000000000000000") << QString("00000000000000000000000000000000000") << 35 << 0;
    QTest::newRow("00000000000000000000000000000000") << QString("00000000000000000000000000000000") << 32 << 0;
    QTest::newRow("00000000000000000000000000000000000000000000000000000000")
        << QString("00000000000000000000000000000000000000000000000000000000") << 56 << 0;
}

void tst_QBitArray::countBits()
{
    QFETCH(QString, bitField);
    QFETCH(int, numBits);
    QFETCH(int, onBits);

    QBitArray bits(bitField.size());
    for (int i = 0; i < bitField.size(); ++i) {
        if (bitField.at(i) == QLatin1Char('1'))
            bits.setBit(i);
    }

    QCOMPARE(bits.count(), numBits);
    QCOMPARE(bits.count(true), onBits);
    QCOMPARE(bits.count(false), numBits - onBits);
}

void tst_QBitArray::countBits2()
{
    QBitArray bitArray;
    for (int i = 0; i < 4017; ++i) {
        bitArray.resize(i);
        bitArray.fill(true);
        QCOMPARE(bitArray.count(true), i);
        QCOMPARE(bitArray.count(false), 0);
        bitArray.fill(false);
        QCOMPARE(bitArray.count(true), 0);
        QCOMPARE(bitArray.count(false), i);
    }
}

void tst_QBitArray::isEmpty()
{
    QBitArray a1;
    QVERIFY(a1.isEmpty());
    QVERIFY(a1.isNull());
    QVERIFY(a1.size() == 0);

    QBitArray a2(0, true);
    QVERIFY(a2.isEmpty());
    QVERIFY(!a2.isNull());
    QVERIFY(a2.size() == 0);

    QBitArray a3(1, true);
    QVERIFY(!a3.isEmpty());
    QVERIFY(!a3.isNull());
    QVERIFY(a3.size() == 1);

    a1.resize(0);
    QVERIFY(a1.isEmpty());
    QVERIFY(!a1.isNull());
    QVERIFY(a1.size() == 0);

    a2.resize(0);
    QVERIFY(a2.isEmpty());
    QVERIFY(!a2.isNull());
    QVERIFY(a2.size() == 0);

    a1.resize(1);
    QVERIFY(!a1.isEmpty());
    QVERIFY(!a1.isNull());
    QVERIFY(a1.size() == 1);

    a1.resize(2);
    QVERIFY(!a1.isEmpty());
    QVERIFY(!a1.isNull());
    QVERIFY(a1.size() == 2);
}

void tst_QBitArray::swap()
{
    QBitArray b1 = QStringToQBitArray("1"), b2 = QStringToQBitArray("10");
    b1.swap(b2);
    QCOMPARE(b1,QStringToQBitArray("10"));
    QCOMPARE(b2,QStringToQBitArray("1"));
}

void tst_QBitArray::fill()
{
    int N = 64;
    int M = 17;
    QBitArray a(N, false);
    int i, j;

    for (i = 0; i < N-M; ++i) {
        a.fill(true, i, i + M);
        for (j = 0; j < N; ++j) {
            if (j >= i && j < i + M) {
                QVERIFY(a.at(j));
            } else {
                QVERIFY(!a.at(j));
            }
        }
        a.fill(false, i, i + M);
    }
    for (i = 0; i < N; ++i)
        a.fill(i % 2 == 0, i, i + 1);
    for (i = 0; i < N; ++i) {
        QVERIFY(a.at(i) == (i % 2 == 0));
    }
}

void tst_QBitArray::toggleBit_data()
{
    QTest::addColumn<int>("index");
    QTest::addColumn<QBitArray>("input");
    QTest::addColumn<QBitArray>("res");
    // 8 bits, toggle first bit
    QTest::newRow( "data0" )  << 0 << QStringToQBitArray(QString("11111111")) << QStringToQBitArray(QString("01111111"));
    // 8 bits
    QTest::newRow( "data1" )  << 1 << QStringToQBitArray(QString("11111111")) << QStringToQBitArray(QString("10111111"));
    // 11 bits, toggle last bit
    QTest::newRow( "data2" )  << 10 << QStringToQBitArray(QString("11111111111")) << QStringToQBitArray(QString("11111111110"));

}

void tst_QBitArray::toggleBit()
{
    QFETCH(int,index);
    QFETCH(QBitArray, input);
    QFETCH(QBitArray, res);

    input.toggleBit(index);

    QCOMPARE(input, res);
}

void tst_QBitArray::operator_andeq_data()
{
    QTest::addColumn<QBitArray>("input1");
    QTest::addColumn<QBitArray>("input2");
    QTest::addColumn<QBitArray>("res");

    QTest::newRow( "data0" )   << QStringToQBitArray(QString("11111111"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("00101100"));


    QTest::newRow( "data1" )   << QStringToQBitArray(QString("11011011"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("00001000"));

    QTest::newRow( "data2" )   << QStringToQBitArray(QString("11011011111"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("00001000000"));

    QTest::newRow( "data3" )   << QStringToQBitArray(QString("11011011"))
                            << QStringToQBitArray(QString("00101100111"))
                            << QStringToQBitArray(QString("00001000000"));

    QTest::newRow( "data4" )   << QStringToQBitArray(QString())
                            << QStringToQBitArray(QString("00101100111"))
                            << QStringToQBitArray(QString("00000000000"));

    QTest::newRow( "data5" ) << QStringToQBitArray(QString("00101100111"))
                             << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString("00000000000"));

    QTest::newRow( "data6" ) << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString());
}

void tst_QBitArray::operator_andeq()
{
    QFETCH(QBitArray, input1);
    QFETCH(QBitArray, input2);
    QFETCH(QBitArray, res);

    input1&=input2;

    QCOMPARE(input1, res);
}

void tst_QBitArray::operator_oreq_data()
{
    QTest::addColumn<QBitArray>("input1");
    QTest::addColumn<QBitArray>("input2");
    QTest::addColumn<QBitArray>("res");

    QTest::newRow( "data0" )   << QStringToQBitArray(QString("11111111"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("11111111"));


    QTest::newRow( "data1" )   << QStringToQBitArray(QString("11011011"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("11111111"));

    QTest::newRow( "data2" )   << QStringToQBitArray(QString("01000010"))
                            << QStringToQBitArray(QString("10100001"))
                            << QStringToQBitArray(QString("11100011"));

    QTest::newRow( "data3" )   << QStringToQBitArray(QString("11011011"))
                            << QStringToQBitArray(QString("00101100000"))
                            << QStringToQBitArray(QString("11111111000"));

    QTest::newRow( "data4" )   << QStringToQBitArray(QString("11011011111"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("11111111111"));

    QTest::newRow( "data5" )   << QStringToQBitArray(QString())
                            << QStringToQBitArray(QString("00101100111"))
                            << QStringToQBitArray(QString("00101100111"));

    QTest::newRow( "data6" ) << QStringToQBitArray(QString("00101100111"))
                             << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString("00101100111"));

    QTest::newRow( "data7" ) << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString());
}

void tst_QBitArray::operator_oreq()
{
    QFETCH(QBitArray, input1);
    QFETCH(QBitArray, input2);
    QFETCH(QBitArray, res);

    input1|=input2;

    QCOMPARE(input1, res);
}

void tst_QBitArray::operator_xoreq_data()
{
    QTest::addColumn<QBitArray>("input1");
    QTest::addColumn<QBitArray>("input2");
    QTest::addColumn<QBitArray>("res");
    QTest::newRow( "data0" )   << QStringToQBitArray(QString("11111111"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("11010011"));

    QTest::newRow( "data1" )   << QStringToQBitArray(QString("11011011"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("11110111"));

    QTest::newRow( "data2" )   << QStringToQBitArray(QString("01000010"))
                            << QStringToQBitArray(QString("10100001"))
                            << QStringToQBitArray(QString("11100011"));

    QTest::newRow( "data3" )   << QStringToQBitArray(QString("01000010"))
                            << QStringToQBitArray(QString("10100001101"))
                            << QStringToQBitArray(QString("11100011101"));

    QTest::newRow( "data4" )   << QStringToQBitArray(QString("01000010111"))
                            << QStringToQBitArray(QString("101000011"))
                            << QStringToQBitArray(QString("11100011011"));

    QTest::newRow( "data5" )   << QStringToQBitArray(QString())
                            << QStringToQBitArray(QString("00101100111"))
                            << QStringToQBitArray(QString("00101100111"));

    QTest::newRow( "data6" ) << QStringToQBitArray(QString("00101100111"))
                             << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString("00101100111"));

    QTest::newRow( "data7" ) << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString());
}

void tst_QBitArray::operator_xoreq()
{
    QFETCH(QBitArray, input1);
    QFETCH(QBitArray, input2);
    QFETCH(QBitArray, res);

    input1^=input2;

    QCOMPARE(input1, res);
}


void tst_QBitArray::operator_neg_data()
{
    QTest::addColumn<QBitArray>("input");
    QTest::addColumn<QBitArray>("res");

    QTest::newRow( "data0" )   << QStringToQBitArray(QString("11111111"))
                               << QStringToQBitArray(QString("00000000"));

    QTest::newRow( "data1" )   << QStringToQBitArray(QString("11011011"))
                               << QStringToQBitArray(QString("00100100"));

    QTest::newRow( "data2" )   << QStringToQBitArray(QString("00000000"))
                               << QStringToQBitArray(QString("11111111"));

    QTest::newRow( "data3" )   << QStringToQBitArray(QString())
                               << QStringToQBitArray(QString());

    QTest::newRow( "data4" )   << QStringToQBitArray("1")
                               << QStringToQBitArray("0");

    QTest::newRow( "data5" )   << QStringToQBitArray("0")
                               << QStringToQBitArray("1");

    QTest::newRow( "data6" )   << QStringToQBitArray("01")
                               << QStringToQBitArray("10");

    QTest::newRow( "data7" )   << QStringToQBitArray("1110101")
                               << QStringToQBitArray("0001010");

    QTest::newRow( "data8" )   << QStringToQBitArray("01110101")
                               << QStringToQBitArray("10001010");

    QTest::newRow( "data9" )   << QStringToQBitArray("011101010")
                               << QStringToQBitArray("100010101");

    QTest::newRow( "data10" )   << QStringToQBitArray("0111010101111010")
                                << QStringToQBitArray("1000101010000101");
}

void tst_QBitArray::operator_neg()
{
    QFETCH(QBitArray, input);
    QFETCH(QBitArray, res);

    input = ~input;

    QCOMPARE(input, res);
}

void tst_QBitArray::datastream_data()
{
    QTest::addColumn<QString>("bitField");
    QTest::addColumn<int>("numBits");
    QTest::addColumn<int>("onBits");

    QTest::newRow("empty") << QString() << 0 << 0;
    QTest::newRow("1") << QString("1") << 1 << 1;
    QTest::newRow("101") << QString("101") << 3 << 2;
    QTest::newRow("101100001") << QString("101100001") << 9 << 4;
    QTest::newRow("101100001101100001") << QString("101100001101100001") << 18 << 8;
    QTest::newRow("101100001101100001101100001101100001") << QString("101100001101100001101100001101100001") << 36 << 16;
    QTest::newRow("00000000000000000000000000000000000") << QString("00000000000000000000000000000000000") << 35 << 0;
    QTest::newRow("11111111111111111111111111111111111") << QString("11111111111111111111111111111111111") << 35 << 35;
    QTest::newRow("11111111111111111111111111111111") << QString("11111111111111111111111111111111") << 32 << 32;
    QTest::newRow("11111111111111111111111111111111111111111111111111111111")
        << QString("11111111111111111111111111111111111111111111111111111111") << 56 << 56;
    QTest::newRow("00000000000000000000000000000000000") << QString("00000000000000000000000000000000000") << 35 << 0;
    QTest::newRow("00000000000000000000000000000000") << QString("00000000000000000000000000000000") << 32 << 0;
    QTest::newRow("00000000000000000000000000000000000000000000000000000000")
        << QString("00000000000000000000000000000000000000000000000000000000") << 56 << 0;
}

void tst_QBitArray::datastream()
{
    QFETCH(QString, bitField);
    QFETCH(int, numBits);
    QFETCH(int, onBits);

    QBuffer buffer;
    QVERIFY(buffer.open(QBuffer::ReadWrite));
    QDataStream stream(&buffer);

    QBitArray bits(bitField.size());
    for (int i = 0; i < bitField.size(); ++i) {
        if (bitField.at(i) == QLatin1Char('1'))
            bits.setBit(i);
    }

    QCOMPARE(bits.count(), numBits);
    QCOMPARE(bits.count(true), onBits);
    QCOMPARE(bits.count(false), numBits - onBits);

    stream << bits << bits << bits;
    buffer.close();

    QCOMPARE(stream.status(), QDataStream::Ok);

    QVERIFY(buffer.open(QBuffer::ReadWrite));
    QDataStream stream2(&buffer);

    QBitArray array1, array2, array3;
    stream2 >> array1 >> array2 >> array3;

    QCOMPARE(array1.count(), numBits);
    QCOMPARE(array1.count(true), onBits);
    QCOMPARE(array1.count(false), numBits - onBits);

    QCOMPARE(array1, bits);
    QCOMPARE(array2, bits);
    QCOMPARE(array3, bits);
}

void tst_QBitArray::invertOnNull() const
{
    QBitArray a;
    QCOMPARE(a = ~a, QBitArray());
}

void tst_QBitArray::operator_noteq_data()
{
    QTest::addColumn<QBitArray>("input1");
    QTest::addColumn<QBitArray>("input2");
    QTest::addColumn<bool>("res");

    QTest::newRow("data0") << QStringToQBitArray(QString("11111111"))
                           << QStringToQBitArray(QString("00101100"))
                           << true;

    QTest::newRow("data1") << QStringToQBitArray(QString("11011011"))
                           << QStringToQBitArray(QString("11011011"))
                           << false;

    QTest::newRow("data2") << QStringToQBitArray(QString())
                           << QStringToQBitArray(QString("00101100111"))
                           << true;

    QTest::newRow("data3") << QStringToQBitArray(QString())
                           << QStringToQBitArray(QString())
                           << false;

    QTest::newRow("data4") << QStringToQBitArray(QString("00101100"))
                           << QStringToQBitArray(QString("11111111"))
                           << true;

    QTest::newRow("data5") << QStringToQBitArray(QString("00101100111"))
                           << QStringToQBitArray(QString())
                           << true;
}

void tst_QBitArray::operator_noteq()
{
    QFETCH(QBitArray, input1);
    QFETCH(QBitArray, input2);
    QFETCH(bool, res);

    bool b = input1 != input2;
    QCOMPARE(b, res);
}

void tst_QBitArray::resize()
{
    // -- check that a resize handles the bits correctly
    QBitArray a = QStringToQBitArray(QString("11"));
    a.resize(10);
    QVERIFY(a.size() == 10);
    QCOMPARE( a, QStringToQBitArray(QString("1100000000")) );

    a.setBit(9);
    a.resize(9);
    // now the bit in a should have been gone:
    QCOMPARE( a, QStringToQBitArray(QString("110000000")) );

    // grow the array back and check the new bit
    a.resize(10);
    QCOMPARE( a, QStringToQBitArray(QString("1100000000")) );

    // other test with and
    a.resize(9);
    QBitArray b = QStringToQBitArray(QString("1111111111"));
    b &= a;
    QCOMPARE( b, QStringToQBitArray(QString("1100000000")) );

}

void tst_QBitArray::fromBits_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<int>("size");
    QTest::addColumn<QBitArray>("expected");

    QTest::newRow("empty") << QByteArray() << 0 << QBitArray();

    auto add = [](const QByteArray &tag, const char *data) {
        QTest::newRow(tag) << QByteArray(data, (tag.size() + 7) / 8) << tag.size()
                           << QStringToQBitArray(tag);
    };

    // "0" to "0000000000000000"
    for (int i = 1; i < 16; ++i) {
        char zero[2] = { 0, 0 };
        QByteArray pattern(i, '0');
        add(pattern, zero);
    }

    // "1" to "1111111111111111"
    for (int i = 1; i < 16; ++i) {
        char one[2] = { '\xff', '\xff' };
        QByteArray pattern(i, '1');
        add(pattern, one);
    }

    // trailing 0 and 1
    char zero = 1;
    char one = 0;
    QByteArray pzero = "1";
    QByteArray pone = "0";
    for (int i = 2; i < 8; ++i) {
        zero <<= 1;
        pzero.prepend('0');
        add(pzero, &zero);

        one = (one << 1) | 1;
        pone.prepend('1');
        add(pone, &one);
    }
}

void tst_QBitArray::fromBits()
{
    QFETCH(QByteArray, data);
    QFETCH(int, size);
    QFETCH(QBitArray, expected);

    QBitArray fromBits = QBitArray::fromBits(data, size);
    QCOMPARE(fromBits, expected);

    QCOMPARE(QBitArray::fromBits(fromBits.bits(), fromBits.size()), expected);
}

QTEST_APPLESS_MAIN(tst_QBitArray)
#include "tst_qbitarray.moc"
