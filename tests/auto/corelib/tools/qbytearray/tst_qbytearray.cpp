/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <qbytearray.h>
#include <qfile.h>
#include <qhash.h>
#include <limits.h>
#include <private/qtools_p.h>
#if defined(Q_OS_WINCE)
#include <qcoreapplication.h>
#endif

class tst_QByteArray : public QObject
{
    Q_OBJECT

public:
    tst_QByteArray();
private slots:
    void swap();
    void qCompress_data();
#ifndef QT_NO_COMPRESS
    void qCompress();
    void qUncompressCorruptedData_data();
    void qUncompressCorruptedData();
    void qCompressionZeroTermination();
#endif
    void constByteArray();
    void leftJustified();
    void rightJustified();
    void setNum();
    void startsWith_data();
    void startsWith();
    void startsWith_char();
    void endsWith_data();
    void endsWith();
    void endsWith_char();
    void split_data();
    void split();
    void base64_data();
    void base64();
    void fromBase64_data();
    void fromBase64();
    void qvsnprintf();
    void qstrlen();
    void qstrnlen();
    void qstrcpy();
    void qstrncpy();
    void qstricmp_data();
    void qstricmp();
    void qstricmp_singularities();
    void qstrnicmp_singularities();
    void chop_data();
    void chop();
    void prepend();
    void prependExtended_data();
    void prependExtended();
    void append();
    void appendExtended_data();
    void appendExtended();
    void insert();
    void insertExtended_data();
    void insertExtended();
    void remove_data();
    void remove();
    void replace_data();
    void replace();
    void replaceWithSpecifiedLength();
    void indexOf_data();
    void indexOf();
    void lastIndexOf_data();
    void lastIndexOf();
    void toULong_data();
    void toULong();
    void toULongLong_data();
    void toULongLong();

    void number();
    void toInt_data();
    void toInt();
    void qAllocMore();

    void resizeAfterFromRawData();
    void appendAfterFromRawData();
    void toFromHex_data();
    void toFromHex();
    void toFromPercentEncoding();
    void fromPercentEncoding_data();
    void fromPercentEncoding();
    void toPercentEncoding_data();
    void toPercentEncoding();
    void toPercentEncoding2_data();
    void toPercentEncoding2();

    void compare_data();
    void compare();
    void compareCharStar_data();
    void compareCharStar();

    void repeatedSignature() const;
    void repeated() const;
    void repeated_data() const;

    void byteRefDetaching() const;

    void reserve();
    void reserveExtended_data();
    void reserveExtended();
    void movablity_data();
    void movablity();
#if defined(Q_COMPILER_LAMBDA)
    void literals();
#endif

    void macTypes();
};

static const struct StaticByteArrays {
    struct Standard {
        QByteArrayData data;
        const char string[8];
    } standard;
    struct NotNullTerminated {
        QByteArrayData data;
        const char string[8];
    } notNullTerminated;
    struct Shifted {
        QByteArrayData data;
        const char dummy;  // added to change offset of string
        const char string[8];
    } shifted;
    struct ShiftedNotNullTerminated {
        QByteArrayData data;
        const char dummy;  // added to change offset of string
        const char string[8];
    } shiftedNotNullTerminated;

} statics = {{Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER(4), "data"}
             ,{Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER(4), "dataBAD"}
             ,{Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(4, sizeof(QByteArrayData) + sizeof(char)), 0, "data"}
             ,{Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(4, sizeof(QByteArrayData) + sizeof(char)), 0, "dataBAD"}
            };

static const QByteArrayDataPtr staticStandard = { const_cast<QByteArrayData *>(&statics.standard.data) };
static const QByteArrayDataPtr staticNotNullTerminated = { const_cast<QByteArrayData *>(&statics.notNullTerminated.data) };
static const QByteArrayDataPtr staticShifted = { const_cast<QByteArrayData *>(&statics.shifted.data) };
static const QByteArrayDataPtr staticShiftedNotNullTerminated = { const_cast<QByteArrayData *>(&statics.shiftedNotNullTerminated.data) };

template <class T> const T &verifyZeroTermination(const T &t) { return t; }

QByteArray verifyZeroTermination(const QByteArray &ba)
{
    // This test does some evil stuff, it's all supposed to work.

    QByteArray::DataPtr baDataPtr = const_cast<QByteArray &>(ba).data_ptr();

    // Skip if isStatic() or fromRawData(), as those offer no guarantees
    if (baDataPtr->ref.isStatic()
            || baDataPtr->offset != QByteArray().data_ptr()->offset)
        return ba;

    int baSize = ba.size();
    char baTerminator = ba.constData()[baSize];
    if ('\0' != baTerminator)
        return QString::fromLatin1(
            "*** Result ('%1') not null-terminated: 0x%2 ***").arg(QString::fromLatin1(ba))
                .arg(baTerminator, 2, 16, QChar('0')).toLatin1();

    // Skip mutating checks on shared strings
    if (baDataPtr->ref.isShared())
        return ba;

    const char *baData = ba.constData();
    const QByteArray baCopy(baData, baSize); // Deep copy

    const_cast<char *>(baData)[baSize] = 'x';
    if ('x' != ba.constData()[baSize]) {
        return QString::fromLatin1("*** Failed to replace null-terminator in "
                "result ('%1') ***").arg(QString::fromLatin1(ba)).toLatin1();
    }
    if (ba != baCopy) {
        return QString::fromLatin1( "*** Result ('%1') differs from its copy "
                "after null-terminator was replaced ***").arg(QString::fromLatin1(ba)).toLatin1();
    }
    const_cast<char *>(baData)[baSize] = '\0'; // Restore sanity

    return ba;
}

// Overriding QTest's QCOMPARE, to check QByteArray for null termination
#undef QCOMPARE
#define QCOMPARE(actual, expected)                                      \
    do {                                                                \
        if (!QTest::qCompare(verifyZeroTermination(actual), expected,   \
                #actual, #expected, __FILE__, __LINE__))                \
            return;                                                     \
    } while (0)                                                         \
    /**/
#undef QTEST
#define QTEST(actual, testElement)                                      \
    do {                                                                \
        if (!QTest::qTest(verifyZeroTermination(actual), testElement,   \
                #actual, #testElement, __FILE__, __LINE__))             \
            return;                                                     \
    } while (0)                                                         \
    /**/

tst_QByteArray::tst_QByteArray()
{
}

void tst_QByteArray::qCompress_data()
{
    QTest::addColumn<QByteArray>("ba");

#ifndef Q_OS_WINCE
    const int size1 = 1024*1024;
#else
    const int size1 = 1024;
#endif
    QByteArray ba1( size1, 0 );

    QTest::newRow( "00" ) << QByteArray();

    int i;
    for ( i=0; i<size1; i++ )
        ba1[i] = (char)( i / 1024 );
    QTest::newRow( "01" ) << ba1;

    for ( i=0; i<size1; i++ )
        ba1[i] = (char)( i % 256 );
    QTest::newRow( "02" ) << ba1;

    ba1.fill( 'A' );
    QTest::newRow( "03" ) << ba1;

#if defined(Q_OS_WINCE)
    int tmpArgc = 0;
    char** tmpArgv = 0;
    QCoreApplication app(tmpArgc, tmpArgv);
#endif
    QFile file( QFINDTESTDATA("rfc3252.txt") );
    QVERIFY( file.open(QIODevice::ReadOnly) );
    QTest::newRow( "04" ) << file.readAll();
}

#ifndef QT_NO_COMPRESS
void tst_QByteArray::qCompress()
{
    QFETCH( QByteArray, ba );
    QByteArray compressed = ::qCompress( ba );
    QTEST( ::qUncompress( compressed ), "ba" );
}

void tst_QByteArray::qUncompressCorruptedData_data()
{
    QTest::addColumn<QByteArray>("in");

    QTest::newRow("0x00000000") << QByteArray("\x00\x00\x00\x00", 4);
    QTest::newRow("0x000000ff") << QByteArray("\x00\x00\x00\xff", 4);
    QTest::newRow("0x3f000000") << QByteArray("\x3f\x00\x00\x00", 4);
    QTest::newRow("0x3fffffff") << QByteArray("\x3f\xff\xff\xff", 4);
    QTest::newRow("0x7fffff00") << QByteArray("\x7f\xff\xff\x00", 4);
    QTest::newRow("0x7fffffff") << QByteArray("\x7f\xff\xff\xff", 4);
    QTest::newRow("0x80000000") << QByteArray("\x80\x00\x00\x00", 4);
    QTest::newRow("0x800000ff") << QByteArray("\x80\x00\x00\xff", 4);
    QTest::newRow("0xcf000000") << QByteArray("\xcf\x00\x00\x00", 4);
    QTest::newRow("0xcfffffff") << QByteArray("\xcf\xff\xff\xff", 4);
    QTest::newRow("0xffffff00") << QByteArray("\xff\xff\xff\x00", 4);
    QTest::newRow("0xffffffff") << QByteArray("\xff\xff\xff\xff", 4);
}

// Corrupt data causes this test to lock up on HP-UX / PA-RISC with gcc,
// SOLARIS, and Windows.
// This test is expected to produce some warning messages in the test output.
void tst_QByteArray::qUncompressCorruptedData()
{
#if !(defined(Q_OS_HPUX) && !defined(__ia64) && defined(Q_CC_GNU)) && !defined(Q_OS_SOLARIS) && !defined(Q_OS_WIN)
    QFETCH(QByteArray, in);

    QByteArray res;
    res = ::qUncompress(in);
    QCOMPARE(res, QByteArray());

    res = ::qUncompress(in + "blah");
    QCOMPARE(res, QByteArray());
#else
    QSKIP("This test freezes on this platform");
#endif
}

void tst_QByteArray::qCompressionZeroTermination()
{
    QString s = "Hello, I'm a string.";
    QByteArray ba = ::qUncompress(::qCompress(s.toLocal8Bit()));
    QVERIFY((int) *(ba.data() + ba.size()) == 0);
}

#endif

void tst_QByteArray::constByteArray()
{
    const char *ptr = "abc";
    QByteArray cba = QByteArray::fromRawData(ptr, 3);
    QVERIFY(cba.constData() == ptr);
    cba.squeeze();
    QVERIFY(cba.constData() == ptr);
    cba.detach();
    QVERIFY(cba.size() == 3);
    QVERIFY(cba.capacity() == 3);
    QVERIFY(cba.constData() != ptr);
    QVERIFY(cba.constData()[0] == 'a');
    QVERIFY(cba.constData()[1] == 'b');
    QVERIFY(cba.constData()[2] == 'c');
    QVERIFY(cba.constData()[3] == '\0');
}

void tst_QByteArray::leftJustified()
{
    QByteArray a;
    a = "ABC";
    QCOMPARE(a.leftJustified(5,'-'), QByteArray("ABC--"));
    QCOMPARE(a.leftJustified(4,'-'), QByteArray("ABC-"));
    QCOMPARE(a.leftJustified(4), QByteArray("ABC "));
    QCOMPARE(a.leftJustified(3), QByteArray("ABC"));
    QCOMPARE(a.leftJustified(2), QByteArray("ABC"));
    QCOMPARE(a.leftJustified(1), QByteArray("ABC"));
    QCOMPARE(a.leftJustified(0), QByteArray("ABC"));

    QByteArray n;
    QVERIFY(!n.leftJustified(3).isNull());    // I expected true
    QCOMPARE(a.leftJustified(4,' ',true), QByteArray("ABC "));
    QCOMPARE(a.leftJustified(3,' ',true), QByteArray("ABC"));
    QCOMPARE(a.leftJustified(2,' ',true), QByteArray("AB"));
    QCOMPARE(a.leftJustified(1,' ',true), QByteArray("A"));
    QCOMPARE(a.leftJustified(0,' ',true), QByteArray(""));
}

void tst_QByteArray::rightJustified()
{
    QByteArray a;
    a="ABC";
    QCOMPARE(a.rightJustified(5,'-'),QByteArray("--ABC"));
    QCOMPARE(a.rightJustified(4,'-'),QByteArray("-ABC"));
    QCOMPARE(a.rightJustified(4),QByteArray(" ABC"));
    QCOMPARE(a.rightJustified(3),QByteArray("ABC"));
    QCOMPARE(a.rightJustified(2),QByteArray("ABC"));
    QCOMPARE(a.rightJustified(1),QByteArray("ABC"));
    QCOMPARE(a.rightJustified(0),QByteArray("ABC"));

    QByteArray n;
    QVERIFY(!n.rightJustified(3).isNull());  // I expected true
    QCOMPARE(a.rightJustified(4,'-',true),QByteArray("-ABC"));
    QCOMPARE(a.rightJustified(4,' ',true),QByteArray(" ABC"));
    QCOMPARE(a.rightJustified(3,' ',true),QByteArray("ABC"));
    QCOMPARE(a.rightJustified(2,' ',true),QByteArray("AB"));
    QCOMPARE(a.rightJustified(1,' ',true),QByteArray("A"));
    QCOMPARE(a.rightJustified(0,' ',true),QByteArray(""));
    QCOMPARE(a,QByteArray("ABC"));
}

void tst_QByteArray::setNum()
{
    QByteArray a;
    QCOMPARE(a.setNum(-1), QByteArray("-1"));
    QCOMPARE(a.setNum(0), QByteArray("0"));
    QCOMPARE(a.setNum(0, 2), QByteArray("0"));
    QCOMPARE(a.setNum(0, 36), QByteArray("0"));
    QCOMPARE(a.setNum(1), QByteArray("1"));
    QCOMPARE(a.setNum(35, 36), QByteArray("z"));
    QCOMPARE(a.setNum(37, 2), QByteArray("100101"));
    QCOMPARE(a.setNum(37, 36), QByteArray("11"));

    // Negative numbers are only properly supported for base 10.
    QCOMPARE(a.setNum(short(-1), 16), QByteArray("ffff"));
    QCOMPARE(a.setNum(int(-1), 16), QByteArray("ffffffff"));
    QCOMPARE(a.setNum(qlonglong(-1), 16), QByteArray("ffffffffffffffff"));

    QCOMPARE(a.setNum(short(-1), 10), QByteArray("-1"));
    QCOMPARE(a.setNum(int(-1), 10), QByteArray("-1"));
    QCOMPARE(a.setNum(qlonglong(-1), 10), QByteArray("-1"));

    QCOMPARE(a.setNum(-123), QByteArray("-123"));
    QCOMPARE(a.setNum(0x123,16), QByteArray("123"));
    QCOMPARE(a.setNum((short)123), QByteArray("123"));

    QCOMPARE(a.setNum(1.23), QByteArray("1.23"));
    QCOMPARE(a.setNum(1.234567), QByteArray("1.23457"));

    // Note that there are no 'long' overloads, so not all of the
    // QString::setNum() tests can be re-used.
    QCOMPARE(a.setNum(Q_INT64_C(123)), QByteArray("123"));
    // 2^40 == 1099511627776
    QCOMPARE(a.setNum(Q_INT64_C(-1099511627776)), QByteArray("-1099511627776"));
    QCOMPARE(a.setNum(Q_UINT64_C(1099511627776)), QByteArray("1099511627776"));
    QCOMPARE(a.setNum(Q_INT64_C(9223372036854775807)), // LLONG_MAX
            QByteArray("9223372036854775807"));
    QCOMPARE(a.setNum(-Q_INT64_C(9223372036854775807) - Q_INT64_C(1)),
            QByteArray("-9223372036854775808"));
    QCOMPARE(a.setNum(Q_UINT64_C(18446744073709551615)), // ULLONG_MAX
            QByteArray("18446744073709551615"));
    QCOMPARE(a.setNum(0.000000000931322574615478515625), QByteArray("9.31323e-10"));
}

void tst_QByteArray::startsWith_data()
{
    QTest::addColumn<QByteArray>("ba");
    QTest::addColumn<QByteArray>("sw");
    QTest::addColumn<bool>("result");

    QTest::newRow("01") << QByteArray() << QByteArray() << true;
    QTest::newRow("02") << QByteArray() << QByteArray("") << true;
    QTest::newRow("03") << QByteArray() << QByteArray("hallo") << false;

    QTest::newRow("04") << QByteArray("") << QByteArray() << true;
    QTest::newRow("05") << QByteArray("") << QByteArray("") << true;
    QTest::newRow("06") << QByteArray("") << QByteArray("h") << false;

    QTest::newRow("07") << QByteArray("hallo") << QByteArray("h") << true;
    QTest::newRow("08") << QByteArray("hallo") << QByteArray("hallo") << true;
    QTest::newRow("09") << QByteArray("hallo") << QByteArray("") << true;
    QTest::newRow("10") << QByteArray("hallo") << QByteArray("hallohallo") << false;
    QTest::newRow("11") << QByteArray("hallo") << QByteArray() << true;
}

void tst_QByteArray::startsWith()
{
    QFETCH(QByteArray, ba);
    QFETCH(QByteArray, sw);
    QFETCH(bool, result);

    QVERIFY(ba.startsWith(sw) == result);

    if (sw.isNull()) {
        QVERIFY(ba.startsWith((char*)0) == result);
    } else {
        QVERIFY(ba.startsWith(sw.data()) == result);
    }
}

void tst_QByteArray::startsWith_char()
{
    QVERIFY(QByteArray("hallo").startsWith('h'));
    QVERIFY(!QByteArray("hallo").startsWith('\0'));
    QVERIFY(!QByteArray("hallo").startsWith('o'));
    QVERIFY(QByteArray("h").startsWith('h'));
    QVERIFY(!QByteArray("h").startsWith('\0'));
    QVERIFY(!QByteArray("h").startsWith('o'));
    QVERIFY(!QByteArray("hallo").startsWith('l'));
    QVERIFY(!QByteArray("").startsWith('\0'));
    QVERIFY(!QByteArray("").startsWith('a'));
    QVERIFY(!QByteArray().startsWith('a'));
    QVERIFY(!QByteArray().startsWith('\0'));
}

void tst_QByteArray::endsWith_data()
{
    QTest::addColumn<QByteArray>("ba");
    QTest::addColumn<QByteArray>("sw");
    QTest::addColumn<bool>("result");

    QTest::newRow("01") << QByteArray() << QByteArray() << true;
    QTest::newRow("02") << QByteArray() << QByteArray("") << true;
    QTest::newRow("03") << QByteArray() << QByteArray("hallo") << false;

    QTest::newRow("04") << QByteArray("") << QByteArray() << true;
    QTest::newRow("05") << QByteArray("") << QByteArray("") << true;
    QTest::newRow("06") << QByteArray("") << QByteArray("h") << false;

    QTest::newRow("07") << QByteArray("hallo") << QByteArray("o") << true;
    QTest::newRow("08") << QByteArray("hallo") << QByteArray("hallo") << true;
    QTest::newRow("09") << QByteArray("hallo") << QByteArray("") << true;
    QTest::newRow("10") << QByteArray("hallo") << QByteArray("hallohallo") << false;
    QTest::newRow("11") << QByteArray("hallo") << QByteArray() << true;
}

void tst_QByteArray::endsWith()
{
    QFETCH(QByteArray, ba);
    QFETCH(QByteArray, sw);
    QFETCH(bool, result);

    QVERIFY(ba.endsWith(sw) == result);

    if (sw.isNull()) {
        QVERIFY(ba.endsWith((char*)0) == result);
    } else {
        QVERIFY(ba.endsWith(sw.data()) == result);
    }
}

void tst_QByteArray::endsWith_char()
{
    QVERIFY(QByteArray("hallo").endsWith('o'));
    QVERIFY(!QByteArray("hallo").endsWith('\0'));
    QVERIFY(!QByteArray("hallo").endsWith('h'));
    QVERIFY(QByteArray("h").endsWith('h'));
    QVERIFY(!QByteArray("h").endsWith('\0'));
    QVERIFY(!QByteArray("h").endsWith('o'));
    QVERIFY(!QByteArray("hallo").endsWith('l'));
    QVERIFY(!QByteArray("").endsWith('\0'));
    QVERIFY(!QByteArray("").endsWith('a'));
    QVERIFY(!QByteArray().endsWith('a'));
    QVERIFY(!QByteArray().endsWith('\0'));
}

void tst_QByteArray::split_data()
{
    QTest::addColumn<QByteArray>("sample");
    QTest::addColumn<int>("size");

    QTest::newRow("1") << QByteArray("-rw-r--r--  1 0  0  519240 Jul  9  2002 bigfile") << 14;
    QTest::newRow("2") << QByteArray("abcde") << 1;
    QTest::newRow("one empty") << QByteArray("") << 1;
    QTest::newRow("two empty") << QByteArray(" ") << 2;
    QTest::newRow("three empty") << QByteArray("  ") << 3;

}

void tst_QByteArray::split()
{
    QFETCH(QByteArray, sample);
    QFETCH(int, size);

    QList<QByteArray> list = sample.split(' ');
    QCOMPARE(list.count(), size);
}

void tst_QByteArray::swap()
{
    QByteArray b1 = "b1", b2 = "b2";
    b1.swap(b2);
    QCOMPARE(b1, QByteArray("b2"));
    QCOMPARE(b2, QByteArray("b1"));
}

void tst_QByteArray::base64_data()
{
    QTest::addColumn<QByteArray>("rawdata");
    QTest::addColumn<QByteArray>("base64");

    QTest::newRow("1") << QByteArray("") << QByteArray("");
    QTest::newRow("2") << QByteArray("1") << QByteArray("MQ==");
    QTest::newRow("3") << QByteArray("12") << QByteArray("MTI=");
    QTest::newRow("4") << QByteArray("123") << QByteArray("MTIz");
    QTest::newRow("5") << QByteArray("1234") << QByteArray("MTIzNA==");
    QTest::newRow("6") << QByteArray("\n") << QByteArray("Cg==");
    QTest::newRow("7") << QByteArray("a\n") << QByteArray("YQo=");
    QTest::newRow("8") << QByteArray("ab\n") << QByteArray("YWIK");
    QTest::newRow("9") << QByteArray("abc\n") << QByteArray("YWJjCg==");
    QTest::newRow("a") << QByteArray("abcd\n") << QByteArray("YWJjZAo=");
    QTest::newRow("b") << QByteArray("abcde\n") << QByteArray("YWJjZGUK");
    QTest::newRow("c") << QByteArray("abcdef\n") << QByteArray("YWJjZGVmCg==");
    QTest::newRow("d") << QByteArray("abcdefg\n") << QByteArray("YWJjZGVmZwo=");
    QTest::newRow("e") << QByteArray("abcdefgh\n") << QByteArray("YWJjZGVmZ2gK");

    QByteArray ba;
    ba.resize(256);
    for (int i = 0; i < 256; ++i)
        ba[i] = i;
    QTest::newRow("f") << ba << QByteArray("AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==");

    QTest::newRow("g") << QByteArray("foo\0bar", 7) << QByteArray("Zm9vAGJhcg==");
    QTest::newRow("h") << QByteArray("f\xd1oo\x9ctar") << QByteArray("ZtFvb5x0YXI=");
    QTest::newRow("i") << QByteArray("\"\0\0\0\0\0\0\"", 8) << QByteArray("IgAAAAAAACI=");
}


void tst_QByteArray::base64()
{
    QFETCH(QByteArray, rawdata);
    QFETCH(QByteArray, base64);

    QByteArray arr = QByteArray::fromBase64(base64);
    QCOMPARE(arr, rawdata);

    QByteArray arr64 = rawdata.toBase64();
    QCOMPARE(arr64, base64);

    arr64 = rawdata.toBase64(QByteArray::Base64Encoding);
    QCOMPARE(arr64, base64);

    QByteArray base64noequals = base64;
    base64noequals.replace('=', "");
    arr64 = rawdata.toBase64(QByteArray::Base64Encoding | QByteArray::OmitTrailingEquals);
    QCOMPARE(arr64, base64noequals);

    QByteArray base64url = base64;
    base64url.replace('/', '_').replace('+', '-');
    arr64 = rawdata.toBase64(QByteArray::Base64UrlEncoding);
    QCOMPARE(arr64, base64url);

    QByteArray base64urlnoequals = base64url;
    base64urlnoequals.replace('=', "");
    arr64 = rawdata.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QCOMPARE(arr64, base64urlnoequals);
}

//different from the previous test as the input are invalid
void tst_QByteArray::fromBase64_data()
{
    QTest::addColumn<QByteArray>("rawdata");
    QTest::addColumn<QByteArray>("base64");

    QTest::newRow("1") << QByteArray("") << QByteArray("  ");
    QTest::newRow("2") << QByteArray("1") << QByteArray("MQ");
    QTest::newRow("3") << QByteArray("12") << QByteArray("MTI       ");
    QTest::newRow("4") << QByteArray("123") << QByteArray("M=TIz");
    QTest::newRow("5") << QByteArray("1234") << QByteArray("MTI zN A ");
    QTest::newRow("6") << QByteArray("\n") << QByteArray("Cg");
    QTest::newRow("7") << QByteArray("a\n") << QByteArray("======YQo=");
    QTest::newRow("8") << QByteArray("ab\n") << QByteArray("Y\nWIK");
    QTest::newRow("9") << QByteArray("abc\n") << QByteArray("YWJjCg==");
    QTest::newRow("a") << QByteArray("abcd\n") << QByteArray("YWJ\1j\x9cZAo=");
    QTest::newRow("b") << QByteArray("abcde\n") << QByteArray("YW JjZ\n G\tUK");
    QTest::newRow("c") << QByteArray("abcdef\n") << QByteArray("YWJjZGVmCg=");
    QTest::newRow("d") << QByteArray("abcdefg\n") << QByteArray("YWJ\rjZGVmZwo");
    QTest::newRow("e") << QByteArray("abcdefgh\n") << QByteArray("YWJjZGVmZ2gK");

    QByteArray ba;
    ba.resize(256);
    for (int i = 0; i < 256; ++i)
        ba[i] = i;
    QTest::newRow("f") << ba << QByteArray("AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Nj\n"
                                           "c4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG1u\n"
                                           "b3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqOkpa\n"
                                           "anqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd\n"
                                           "3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==                            ");


    QTest::newRow("g") << QByteArray("foo\0bar", 7) << QByteArray("Zm9vAGJhcg");
    QTest::newRow("h") << QByteArray("f\xd1oo\x9ctar") << QByteArray("ZtFvb5x0YXI=");
    QTest::newRow("i") << QByteArray("\"\0\0\0\0\0\0\"", 8) << QByteArray("IgAAAAAAACI");

}


void tst_QByteArray::fromBase64()
{
    QFETCH(QByteArray, rawdata);
    QFETCH(QByteArray, base64);

    QByteArray arr = QByteArray::fromBase64(base64);
    QCOMPARE(arr, rawdata);

    arr = QByteArray::fromBase64(base64, QByteArray::Base64Encoding);
    QCOMPARE(arr, rawdata);

    // try "base64url" encoding
    QByteArray base64url = base64;
    base64url.replace('/', '_').replace('+', '-');
    arr = QByteArray::fromBase64(base64url, QByteArray::Base64UrlEncoding);
    QCOMPARE(arr, rawdata);

    if (base64 != base64url) {
        // check that the invalid decodings fail
        arr = QByteArray::fromBase64(base64, QByteArray::Base64UrlEncoding);
        QVERIFY(arr != rawdata);
        arr = QByteArray::fromBase64(base64url, QByteArray::Base64Encoding);
        QVERIFY(arr != rawdata);
    }
}

void tst_QByteArray::qvsnprintf()
{
    char buf[20];
    memset(buf, 42, sizeof(buf));

    QCOMPARE(::qsnprintf(buf, 10, "%s", "bubu"), 4);
    QCOMPARE(static_cast<const char *>(buf), "bubu");
#ifndef Q_CC_MSVC
    // MSVC implementation of vsnprintf overwrites bytes after null terminator so this would fail.
    QCOMPARE(buf[5], char(42));
#endif

    memset(buf, 42, sizeof(buf));
    QCOMPARE(::qsnprintf(buf, 5, "%s", "bubu"), 4);
    QCOMPARE(static_cast<const char *>(buf), "bubu");
    QCOMPARE(buf[5], char(42));

    memset(buf, 42, sizeof(buf));
#ifdef Q_OS_WIN
    // VS 2005 uses the Qt implementation of vsnprintf.
# if defined(_MSC_VER) && _MSC_VER >= 1400 && !defined(Q_OS_WINCE)
    QCOMPARE(::qsnprintf(buf, 3, "%s", "bubu"), -1);
    QCOMPARE(static_cast<const char*>(buf), "bu");
# else
    // windows has to do everything different, of course.
    QCOMPARE(::qsnprintf(buf, 3, "%s", "bubu"), -1);
    buf[19] = '\0';
    QCOMPARE(static_cast<const char *>(buf), "bub****************");
# endif
#else
#ifdef Q_OS_IRIX
    // Irix reports back the amount of characters written without the \0
    QCOMPARE(::qsnprintf(buf, 3, "%s", "bubu"), 2);
#else
    // Every other system in this world reports the amount of data that could have been written
    QCOMPARE(::qsnprintf(buf, 3, "%s", "bubu"), 4);
#endif
    QCOMPARE(static_cast<const char*>(buf), "bu");
#endif
    QCOMPARE(buf[4], char(42));

#ifndef Q_OS_WIN
    memset(buf, 42, sizeof(buf));
    QCOMPARE(::qsnprintf(buf, 10, ""), 0);
#endif
}


void tst_QByteArray::qstrlen()
{
    const char *src = "Something about ... \0 a string.";
    QCOMPARE(::qstrlen((char*)0), (uint)0);
    QCOMPARE(::qstrlen(src), (uint)20);
}

void tst_QByteArray::qstrnlen()
{
    const char *src = "Something about ... \0 a string.";
    QCOMPARE(::qstrnlen((char*)0, 1), (uint)0);
    QCOMPARE(::qstrnlen(src, 31), (uint)20);
    QCOMPARE(::qstrnlen(src, 19), (uint)19);
    QCOMPARE(::qstrnlen(src, 21), (uint)20);
    QCOMPARE(::qstrnlen(src, 20), (uint)20);
}

void tst_QByteArray::qstrcpy()
{
    const char *src = "Something about ... \0 a string.";
    const char *expected = "Something about ... ";
    char dst[128];

    QCOMPARE(::qstrcpy(0, 0), (char*)0);
    QCOMPARE(::qstrcpy(dst, 0), (char*)0);

    QCOMPARE(::qstrcpy(dst ,src), (char *)dst);
    QCOMPARE((char *)dst, const_cast<char *>(expected));
}

void tst_QByteArray::qstrncpy()
{
    QByteArray src(1024, 'a'), dst(1024, 'b');

    // singularities
    QCOMPARE(::qstrncpy(0, 0,0), (char*)0);
    QCOMPARE(::qstrncpy(dst.data(), 0, 0), (char*)0);

    // normal copy
    QCOMPARE(::qstrncpy(dst.data(), src.data(), src.size()), dst.data());

    src = QByteArray( "Tumdelidum" );
    QCOMPARE(QByteArray(::qstrncpy(dst.data(), src.data(), src.size())),
            QByteArray("Tumdelidu"));

    // normal copy with length is longer than necessary
    src = QByteArray( "Tumdelidum\0foo" );
    dst.resize(128*1024);
    QCOMPARE(QByteArray(::qstrncpy(dst.data(), src.data(), dst.size())),
            QByteArray("Tumdelidum"));
}

void tst_QByteArray::qstricmp_data()
{
    QTest::addColumn<QString>("str1");
    QTest::addColumn<QString>("str2");

    QTest::newRow("equal   1") << "abcEdb"  << "abcEdb";
    QTest::newRow("equal   2") << "abcEdb"  << "ABCeDB";
    QTest::newRow("equal   3") << "ABCEDB"  << "abcedb";
    QTest::newRow("less    1") << "abcdef"  << "abcdefg";
    QTest::newRow("less    2") << "abcdeF"  << "abcdef";
    QTest::newRow("greater 1") << "abcdef"  << "abcdeF";
    QTest::newRow("greater 2") << "abcdefg" << "abcdef";
}

void tst_QByteArray::qstricmp()
{
    QFETCH(QString, str1);
    QFETCH(QString, str2);

    int expected = strcmp(str1.toUpper().toLatin1(),
                          str2.toUpper().toLatin1());
    if ( expected != 0 ) {
        expected = (expected < 0 ? -1 : 1);
    }
    int actual = ::qstricmp(str1.toLatin1(), str2.toLatin1());
    if ( actual != 0 ) {
        actual = (actual < 0 ? -1 : 1);
    }
    QCOMPARE(expected, actual);
}

void tst_QByteArray::qstricmp_singularities()
{
    QCOMPARE(::qstricmp(0, 0), 0);
    QVERIFY(::qstricmp(0, "a") != 0);
    QVERIFY(::qstricmp("a", 0) != 0);
    QCOMPARE(::qstricmp("", ""), 0);
}

void tst_QByteArray::qstrnicmp_singularities()
{
    QCOMPARE(::qstrnicmp(0, 0, 123), 0);
    QVERIFY(::qstrnicmp(0, "a", 123) != 0);
    QVERIFY(::qstrnicmp("a", 0, 123) != 0);
    QCOMPARE(::qstrnicmp("", "", 123), 0);
    QCOMPARE(::qstrnicmp("a", "B", 0), 0);
}

void tst_QByteArray::chop_data()
{
    QTest::addColumn<QByteArray>("src");
    QTest::addColumn<int>("choplength");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("1") << QByteArray("short1") << 128 << QByteArray();
    QTest::newRow("2") << QByteArray("short2") << int(strlen("short2"))
                    << QByteArray();
    QTest::newRow("3") << QByteArray("abcdef\0foo", 10) << 2
                    << QByteArray("abcdef\0f", 8);
    QTest::newRow("4") << QByteArray("STARTTLS\r\n") << 2
                    << QByteArray("STARTTLS");
    QTest::newRow("5") << QByteArray("") << 1 << QByteArray();
    QTest::newRow("6") << QByteArray("foo") << 0 << QByteArray("foo");
    QTest::newRow("7") << QByteArray(0) << 28 << QByteArray();
}

void tst_QByteArray::chop()
{
    QFETCH(QByteArray, src);
    QFETCH(int, choplength);
    QFETCH(QByteArray, expected);

    src.chop(choplength);
    QCOMPARE(src, expected);
}

void tst_QByteArray::prepend()
{
    QByteArray ba("foo");
    QCOMPARE(ba.prepend((char*)0), QByteArray("foo"));
    QCOMPARE(ba.prepend(QByteArray()), QByteArray("foo"));
    QCOMPARE(ba.prepend("1"), QByteArray("1foo"));
    QCOMPARE(ba.prepend(QByteArray("2")), QByteArray("21foo"));
    QCOMPARE(ba.prepend('3'), QByteArray("321foo"));
    QCOMPARE(ba.prepend("\0 ", 2), QByteArray::fromRawData("\0 321foo", 8));
}

void tst_QByteArray::prependExtended_data()
{
    QTest::addColumn<QByteArray>("array");
    QTest::newRow("literal") << QByteArray(QByteArrayLiteral("data"));
    QTest::newRow("standard") << QByteArray(staticStandard);
    QTest::newRow("shifted") << QByteArray(staticShifted);
    QTest::newRow("notNullTerminated") << QByteArray(staticNotNullTerminated);
    QTest::newRow("shiftedNotNullTerminated") << QByteArray(staticShiftedNotNullTerminated);
    QTest::newRow("non static data") << QByteArray("data");
    QTest::newRow("from raw data") << QByteArray::fromRawData("data", 4);
    QTest::newRow("from raw data not terminated") << QByteArray::fromRawData("dataBAD", 4);
}

void tst_QByteArray::prependExtended()
{
    QFETCH(QByteArray, array);

    QCOMPARE(QByteArray().prepend(array), QByteArray("data"));
    QCOMPARE(QByteArray("").prepend(array), QByteArray("data"));

    QCOMPARE(array.prepend((char*)0), QByteArray("data"));
    QCOMPARE(array.prepend(QByteArray()), QByteArray("data"));
    QCOMPARE(array.prepend("1"), QByteArray("1data"));
    QCOMPARE(array.prepend(QByteArray("2")), QByteArray("21data"));
    QCOMPARE(array.prepend('3'), QByteArray("321data"));
    QCOMPARE(array.prepend("\0 ", 2), QByteArray::fromRawData("\0 321data", 9));
    QCOMPARE(array.size(), 9);
}

void tst_QByteArray::append()
{
    QByteArray ba("foo");
    QCOMPARE(ba.append((char*)0), QByteArray("foo"));
    QCOMPARE(ba.append(QByteArray()), QByteArray("foo"));
    QCOMPARE(ba.append("1"), QByteArray("foo1"));
    QCOMPARE(ba.append(QByteArray("2")), QByteArray("foo12"));
    QCOMPARE(ba.append('3'), QByteArray("foo123"));
    QCOMPARE(ba.append("\0"), QByteArray("foo123"));
    QCOMPARE(ba.append("\0", 1), QByteArray::fromRawData("foo123\0", 7));
    QCOMPARE(ba.size(), 7);
}

void tst_QByteArray::appendExtended_data()
{
    prependExtended_data();
}

void tst_QByteArray::appendExtended()
{
    QFETCH(QByteArray, array);

    QCOMPARE(QByteArray().append(array), QByteArray("data"));
    QCOMPARE(QByteArray("").append(array), QByteArray("data"));

    QCOMPARE(array.append((char*)0), QByteArray("data"));
    QCOMPARE(array.append(QByteArray()), QByteArray("data"));
    QCOMPARE(array.append("1"), QByteArray("data1"));
    QCOMPARE(array.append(QByteArray("2")), QByteArray("data12"));
    QCOMPARE(array.append('3'), QByteArray("data123"));
    QCOMPARE(array.append("\0"), QByteArray("data123"));
    QCOMPARE(array.append("\0", 1), QByteArray::fromRawData("data123\0", 8));
    QCOMPARE(array.size(), 8);
}

void tst_QByteArray::insert()
{
    QByteArray ba("Meal");
    QCOMPARE(ba.insert(1, QByteArray("ontr")), QByteArray("Montreal"));
    QCOMPARE(ba.insert(ba.size(), "foo"), QByteArray("Montrealfoo"));

    ba = QByteArray("13");
    QCOMPARE(ba.insert(1, QByteArray("2")), QByteArray("123"));

    ba = "ac";
    QCOMPARE(ba.insert(1, 'b'), QByteArray("abc"));
    QCOMPARE(ba.size(), 3);

    ba = "ikl";
    QCOMPARE(ba.insert(1, "j"), QByteArray("ijkl"));
    QCOMPARE(ba.size(), 4);

    ba = "ab";
    QCOMPARE(ba.insert(1, "\0X\0", 3), QByteArray::fromRawData("a\0X\0b", 5));
    QCOMPARE(ba.size(), 5);
}

void tst_QByteArray::insertExtended_data()
{
    prependExtended_data();
}

void tst_QByteArray::insertExtended()
{
    QFETCH(QByteArray, array);
    QCOMPARE(array.insert(1, "i"), QByteArray("diata"));
    QCOMPARE(array.size(), 5);
}

void tst_QByteArray::remove_data()
{
    QTest::addColumn<QByteArray>("src");
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("length");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("1") << QByteArray("Montreal") << 1 << 4
                    << QByteArray("Meal");
    QTest::newRow("2") << QByteArray() << 10 << 10 << QByteArray();
    QTest::newRow("3") << QByteArray("hi") << 0 << 10 << QByteArray();
    QTest::newRow("4") << QByteArray("Montreal") << 4 << 100
                    << QByteArray("Mont");

    // index out of range
    QTest::newRow("5") << QByteArray("Montreal") << 8 << 1
                    << QByteArray("Montreal");
    QTest::newRow("6") << QByteArray("Montreal") << 18 << 4
                    << QByteArray("Montreal");
}

void tst_QByteArray::remove()
{
    QFETCH(QByteArray, src);
    QFETCH(int, position);
    QFETCH(int, length);
    QFETCH(QByteArray, expected);
    QCOMPARE(src.remove(position, length), expected);
}

void tst_QByteArray::replace_data()
{
    QTest::addColumn<QByteArray>("src");
    QTest::addColumn<int>("pos");
    QTest::addColumn<int>("len");
    QTest::addColumn<QByteArray>("after");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("1") << QByteArray("Say yes!") << 4 << 3
                    << QByteArray("no") << QByteArray("Say no!");
    QTest::newRow("2") << QByteArray("rock and roll") << 5 << 3
                    << QByteArray("&") << QByteArray("rock & roll");
    QTest::newRow("3") << QByteArray("foo") << 3 << 0 << QByteArray("bar")
                    << QByteArray("foobar");
    QTest::newRow("4") << QByteArray() << 0 << 0 << QByteArray() << QByteArray();
    // index out of range
    QTest::newRow("5") << QByteArray() << 3 << 0 << QByteArray("hi")
                    << QByteArray("   hi");
    // Optimized path
    QTest::newRow("6") << QByteArray("abcdef") << 3 << 12 << QByteArray("abcdefghijkl") << QByteArray("abcabcdefghijkl");
    QTest::newRow("7") << QByteArray("abcdef") << 3 << 4  << QByteArray("abcdefghijkl") << QByteArray("abcabcdefghijkl");
    QTest::newRow("8") << QByteArray("abcdef") << 3 << 3  << QByteArray("abcdefghijkl") << QByteArray("abcabcdefghijkl");
    QTest::newRow("9") << QByteArray("abcdef") << 3 << 2  << QByteArray("abcdefghijkl") << QByteArray("abcabcdefghijklf");
    QTest::newRow("10") << QByteArray("abcdef") << 2 << 2  << QByteArray("xx") << QByteArray("abxxef");
}

void tst_QByteArray::replace()
{
    QFETCH(QByteArray, src);
    QFETCH(int, pos);
    QFETCH(int, len);
    QFETCH(QByteArray, after);
    QFETCH(QByteArray, expected);

    QByteArray str1 = src;
    QByteArray str2 = src;

    QCOMPARE(str1.replace(pos, len, after).constData(), expected.constData());
    QCOMPARE(str2.replace(pos, len, after.data()), expected);
}

void tst_QByteArray::replaceWithSpecifiedLength()
{
    const char after[] = "zxc\0vbnmqwert";
    int lenAfter = 6;
    QByteArray ba("abcdefghjk");
    ba.replace(0,2,after,lenAfter);

    const char _expected[] = "zxc\0vbcdefghjk";
    QByteArray expected(_expected,sizeof(_expected)-1);
    QCOMPARE(ba,expected);
}

void tst_QByteArray::indexOf_data()
{
    QTest::addColumn<QByteArray>("haystack");
    QTest::addColumn<QByteArray>("needle");
    QTest::addColumn<int>("startpos");
    QTest::addColumn<int>("expected");

    QTest::newRow( "1" ) << QByteArray("abc") << QByteArray("a") << 0 << 0;
    QTest::newRow( "2" ) << QByteArray("abc") << QByteArray("A") << 0 << -1;
    QTest::newRow( "3" ) << QByteArray("abc") << QByteArray("a") << 1 << -1;
    QTest::newRow( "4" ) << QByteArray("abc") << QByteArray("A") << 1 << -1;
    QTest::newRow( "5" ) << QByteArray("abc") << QByteArray("b") << 0 << 1;
    QTest::newRow( "6" ) << QByteArray("abc") << QByteArray("B") << 0 << -1;
    QTest::newRow( "7" ) << QByteArray("abc") << QByteArray("b") << 1 << 1;
    QTest::newRow( "8" ) << QByteArray("abc") << QByteArray("B") << 1 << -1;
    QTest::newRow( "9" ) << QByteArray("abc") << QByteArray("b") << 2 << -1;
    QTest::newRow( "10" ) << QByteArray("abc") << QByteArray("c") << 0 << 2;
    QTest::newRow( "11" ) << QByteArray("abc") << QByteArray("C") << 0 << -1;
    QTest::newRow( "12" ) << QByteArray("abc") << QByteArray("c") << 1 << 2;
    QTest::newRow( "13" ) << QByteArray("abc") << QByteArray("C") << 1 << -1;
    QTest::newRow( "14" ) << QByteArray("abc") << QByteArray("c") << 2 << 2;
    QTest::newRow( "15" ) << QByteArray("aBc") << QByteArray("bc") << 0 << -1;
    QTest::newRow( "16" ) << QByteArray("aBc") << QByteArray("Bc") << 0 << 1;
    QTest::newRow( "17" ) << QByteArray("aBc") << QByteArray("bC") << 0 << -1;
    QTest::newRow( "18" ) << QByteArray("aBc") << QByteArray("BC") << 0 << -1;

    static const char h19[] = {'x', 0x00, (char)0xe7, 0x25, 0x1c, 0x0a};
    static const char n19[] = {0x00, 0x00, 0x01, 0x00};
    QTest::newRow( "19" ) << QByteArray(h19, sizeof(h19))
                          << QByteArray(n19, sizeof(n19)) << 0 << -1;

    QTest::newRow( "empty" ) << QByteArray("") << QByteArray("x") << 0 << -1;
    QTest::newRow( "null" ) << QByteArray() << QByteArray("x") << 0 << -1;
    QTest::newRow( "null-in-null") << QByteArray() << QByteArray() << 0 << 0;
    QTest::newRow( "empty-in-null") << QByteArray() << QByteArray("") << 0 << 0;
    QTest::newRow( "null-in-empty") << QByteArray("") << QByteArray() << 0 << 0;
    QTest::newRow( "empty-in-empty") << QByteArray("") << QByteArray("") << 0 << 0;

    QByteArray veryBigHaystack(500, 'a');
    veryBigHaystack += 'B';
    QTest::newRow("BoyerMooreStressTest") << veryBigHaystack << veryBigHaystack << 0 << 0;
    QTest::newRow("BoyerMooreStressTest2") << QByteArray(veryBigHaystack + 'c') <<  QByteArray(veryBigHaystack) << 0 << 0;
    QTest::newRow("BoyerMooreStressTest3") << QByteArray('c' + veryBigHaystack) <<  QByteArray(veryBigHaystack) << 0 << 1;
    QTest::newRow("BoyerMooreStressTest4") << QByteArray(veryBigHaystack) <<  QByteArray(veryBigHaystack + 'c') << 0 << -1;
    QTest::newRow("BoyerMooreStressTest5") << QByteArray(veryBigHaystack) <<  QByteArray('c' + veryBigHaystack) << 0 << -1;
    QTest::newRow("BoyerMooreStressTest6") << QByteArray('d' + veryBigHaystack) <<  QByteArray('c' + veryBigHaystack) << 0 << -1;
    QTest::newRow("BoyerMooreStressTest7") << QByteArray(veryBigHaystack + 'c') <<  QByteArray('c' + veryBigHaystack) << 0 << -1;
}

void tst_QByteArray::indexOf()
{
    QFETCH( QByteArray, haystack );
    QFETCH( QByteArray, needle );
    QFETCH( int, startpos );
    QFETCH( int, expected );

    bool hasNull = needle.contains('\0');

    QCOMPARE( haystack.indexOf(needle, startpos), expected );
    if (!hasNull)
        QCOMPARE( haystack.indexOf(needle.data(), startpos), expected );
    if (needle.size() == 1)
        QCOMPARE( haystack.indexOf(needle.at(0), startpos), expected );

    if (startpos == 0) {
        QCOMPARE( haystack.indexOf(needle), expected );
        if (!hasNull)
            QCOMPARE( haystack.indexOf(needle.data()), expected );
        if (needle.size() == 1)
            QCOMPARE( haystack.indexOf(needle.at(0)), expected );
    }
}

void tst_QByteArray::lastIndexOf_data()
{
    QTest::addColumn<QByteArray>("haystack");
    QTest::addColumn<QByteArray>("needle");
    QTest::addColumn<int>("startpos");
    QTest::addColumn<int>("expected");

    QTest::newRow( "1" ) << QByteArray("abc") << QByteArray("a") << 0 << 0;
    QTest::newRow( "2" ) << QByteArray("abc") << QByteArray("A") << 0 << -1;
    QTest::newRow( "3" ) << QByteArray("abc") << QByteArray("a") << 1 << 0;
    QTest::newRow( "4" ) << QByteArray("abc") << QByteArray("A") << 1 << -1;
    QTest::newRow( "5" ) << QByteArray("abc") << QByteArray("a") << -1 << 0;
    QTest::newRow( "6" ) << QByteArray("abc") << QByteArray("b") << 0 << -1;
    QTest::newRow( "7" ) << QByteArray("abc") << QByteArray("B") << 0 << -1;
    QTest::newRow( "8" ) << QByteArray("abc") << QByteArray("b") << 1 << 1;
    QTest::newRow( "9" ) << QByteArray("abc") << QByteArray("B") << 1 << -1;
    QTest::newRow( "10" ) << QByteArray("abc") << QByteArray("b") << 2 << 1;
    QTest::newRow( "11" ) << QByteArray("abc") << QByteArray("b") << -1 << 1;
    QTest::newRow( "12" ) << QByteArray("abc") << QByteArray("c") << 0 << -1;
    QTest::newRow( "13" ) << QByteArray("abc") << QByteArray("C") << 0 << -1;
    QTest::newRow( "14" ) << QByteArray("abc") << QByteArray("c") << 1 << -1;
    QTest::newRow( "15" ) << QByteArray("abc") << QByteArray("C") << 1 << -1;
    QTest::newRow( "16" ) << QByteArray("abc") << QByteArray("c") << 2 << 2;
    QTest::newRow( "17" ) << QByteArray("abc") << QByteArray("c") << -1 << 2;
    QTest::newRow( "18" ) << QByteArray("aBc") << QByteArray("bc") << 0 << -1;
    QTest::newRow( "19" ) << QByteArray("aBc") << QByteArray("Bc") << 0 << -1;
    QTest::newRow( "20" ) << QByteArray("aBc") << QByteArray("Bc") << 2 << 1;
    QTest::newRow( "21" ) << QByteArray("aBc") << QByteArray("Bc") << 1 << 1;
    QTest::newRow( "22" ) << QByteArray("aBc") << QByteArray("Bc") << -1 << 1;
    QTest::newRow( "23" ) << QByteArray("aBc") << QByteArray("bC") << 0 << -1;
    QTest::newRow( "24" ) << QByteArray("aBc") << QByteArray("BC") << 0 << -1;

    static const char h25[] = {0x00, (char)0xbc, 0x03, 0x10, 0x0a };
    static const char n25[] = {0x00, 0x00, 0x01, 0x00};
    QTest::newRow( "25" ) << QByteArray(h25, sizeof(h25))
                          << QByteArray(n25, sizeof(n25)) << 0 << -1;

    QTest::newRow( "empty" ) << QByteArray("") << QByteArray("x") << -1 << -1;
    QTest::newRow( "null" ) << QByteArray() << QByteArray("x") << -1 << -1;
    QTest::newRow( "null-in-null") << QByteArray() << QByteArray() << -1 << 0;
    QTest::newRow( "empty-in-null") << QByteArray() << QByteArray("") << -1 << 0;
    QTest::newRow( "null-in-empty") << QByteArray("") << QByteArray() << -1 << 0;
    QTest::newRow( "empty-in-empty") << QByteArray("") << QByteArray("") << -1 << 0;
}

void tst_QByteArray::lastIndexOf()
{
    QFETCH( QByteArray, haystack );
    QFETCH( QByteArray, needle );
    QFETCH( int, startpos );
    QFETCH( int, expected );

    bool hasNull = needle.contains('\0');

    QCOMPARE( haystack.lastIndexOf(needle, startpos), expected );
    if (!hasNull)
        QCOMPARE( haystack.lastIndexOf(needle.data(), startpos), expected );
    if (needle.size() == 1)
        QCOMPARE( haystack.lastIndexOf(needle.at(0), startpos), expected );

    if (startpos == -1) {
        QCOMPARE( haystack.lastIndexOf(needle), expected );
        if (!hasNull)
            QCOMPARE( haystack.lastIndexOf(needle.data()), expected );
        if (needle.size() == 1)
            QCOMPARE( haystack.lastIndexOf(needle.at(0)), expected );
    }
}

void tst_QByteArray::number()
{
    QCOMPARE(QString(QByteArray::number((quint64) 0)),
             QString(QByteArray("0")));
    QCOMPARE(QString(QByteArray::number(Q_UINT64_C(0xFFFFFFFFFFFFFFFF))),
             QString(QByteArray("18446744073709551615")));
    QCOMPARE(QString(QByteArray::number(Q_INT64_C(0xFFFFFFFFFFFFFFFF))),
             QString(QByteArray("-1")));
    QCOMPARE(QString(QByteArray::number(qint64(0))),
             QString(QByteArray("0")));
    QCOMPARE(QString(QByteArray::number(Q_INT64_C(0x7FFFFFFFFFFFFFFF))),
             QString(QByteArray("9223372036854775807")));
    QCOMPARE(QString(QByteArray::number(Q_INT64_C(0x8000000000000000))),
             QString(QByteArray("-9223372036854775808")));
}

// defined later
extern const char globalChar;

void tst_QByteArray::toInt_data()
{
    QTest::addColumn<QByteArray>("string");
    QTest::addColumn<int>("base");
    QTest::addColumn<int>("expectednumber");
    QTest::addColumn<bool>("expectedok");

    QTest::newRow("base 10") << QByteArray("100") << 10 << int(100) << true;
    QTest::newRow("base 16-1") << QByteArray("100") << 16 << int(256) << true;
    QTest::newRow("base 16-2") << QByteArray("0400") << 16 << int(1024) << true;
    QTest::newRow("base 2") << QByteArray("1111") << 2 << int(15) << true;
    QTest::newRow("base 8") << QByteArray("100") << 8 << int(64) << true;
    QTest::newRow("base 0-1") << QByteArray("0x10") << 0 << int(16) << true;
    QTest::newRow("base 0-2") << QByteArray("10") << 0 << int(10) << true;
    QTest::newRow("base 0-3") << QByteArray("010") << 0 << int(8) << true;
    QTest::newRow("empty") << QByteArray() << 0 << int(0) << false;

    // using fromRawData
    QTest::newRow("raw1") << QByteArray::fromRawData("1", 1) << 10 << 1 << true;
    QTest::newRow("raw2") << QByteArray::fromRawData("1foo", 1) << 10 << 1 << true;
    QTest::newRow("raw3") << QByteArray::fromRawData("12", 1) << 10 << 1 << true;
    QTest::newRow("raw4") << QByteArray::fromRawData("123456789", 1) << 10 << 1 << true;
    QTest::newRow("raw5") << QByteArray::fromRawData("123456789", 2) << 10 << 12 << true;

    QTest::newRow("raw-static") << QByteArray::fromRawData(&globalChar, 1) << 10 << 1 << true;
}

void tst_QByteArray::toInt()
{
    QFETCH( QByteArray, string );
    QFETCH( int, base );
    QFETCH( int, expectednumber );
    QFETCH( bool, expectedok );

    bool ok;
    int number = string.toInt(&ok, base);

    QCOMPARE( ok, expectedok );
    QCOMPARE( number, expectednumber );
}

void tst_QByteArray::toULong_data()
{
    QTest::addColumn<QByteArray>("str");
    QTest::addColumn<int>("base");
    QTest::addColumn<ulong>("result");
    QTest::addColumn<bool>("ok");

    ulong LongMaxPlusOne = (ulong)LONG_MAX + 1;
    QTest::newRow("LONG_MAX+1") << QString::number(LongMaxPlusOne).toLatin1() << 10 << LongMaxPlusOne << true;
    QTest::newRow("default") << QByteArray() << 10 << 0UL << false;
    QTest::newRow("empty") << QByteArray("") << 10 << 0UL << false;
    QTest::newRow("ulong1") << QByteArray("3234567890") << 10 << 3234567890UL << true;
    QTest::newRow("ulong2") << QByteArray("fFFfFfFf") << 16 << 0xFFFFFFFFUL << true;
}

void tst_QByteArray::toULong()
{
    QFETCH(QByteArray, str);
    QFETCH(int, base);
    QFETCH(ulong, result);
    QFETCH(bool, ok);

    bool b;
    QCOMPARE(str.toULong(0, base), result);
    QCOMPARE(str.toULong(&b, base), result);
    QCOMPARE(b, ok);
}

void tst_QByteArray::toULongLong_data()
{
    QTest::addColumn<QByteArray>("str");
    QTest::addColumn<int>("base");
    QTest::addColumn<qulonglong>("result");
    QTest::addColumn<bool>("ok");

    QTest::newRow("default") << QByteArray() << 10 << (qulonglong)0 << false;
    QTest::newRow("out of base bound") << QByteArray("c") << 10 << (qulonglong)0 << false;

}

void tst_QByteArray::toULongLong()
{
    QFETCH(QByteArray, str);
    QFETCH(int, base);
    QFETCH(qulonglong, result);
    QFETCH(bool, ok);

    bool b;
    QCOMPARE(str.toULongLong(0, base), result);
    QCOMPARE(str.toULongLong(&b, base), result);
    QCOMPARE(b, ok);
}

// global function defined in qbytearray.cpp
void tst_QByteArray::qAllocMore()
{
    using QT_PREPEND_NAMESPACE(qAllocMore);

    // Not very important, but please behave :-)
    QVERIFY(qAllocMore(0, 0) >= 0);

    for (int i = 1; i < 1 << 8; i <<= 1)
        QVERIFY(qAllocMore(i, 0) >= i);

    for (int i = 1 << 8; i < 1 << 30; i <<= 1) {
        const int alloc = qAllocMore(i, 0);

        QVERIFY(alloc >= i);
        QCOMPARE(qAllocMore(i - 8, 8), alloc - 8);
        QCOMPARE(qAllocMore(i - 16, 16), alloc - 16);
        QCOMPARE(qAllocMore(i - 24, 24), alloc - 24);
        QCOMPARE(qAllocMore(i - 32, 32), alloc - 32);

        QVERIFY(qAllocMore(i - 1, 0) >= i - 1);
        QVERIFY(qAllocMore(i + 1, 0) >= i + 1);
    }
}

void tst_QByteArray::resizeAfterFromRawData()
{
    QByteArray buffer("hello world");

    QByteArray array = QByteArray::fromRawData(buffer.constData(), buffer.size());
    QVERIFY(array.constData() == buffer.constData());
    array.resize(5);
    QVERIFY(array.constData() == buffer.constData());
}

void tst_QByteArray::appendAfterFromRawData()
{
    QByteArray arr;
    {
        char data[] = "X";
        arr += QByteArray::fromRawData(data, sizeof(data));
        data[0] = 'Y';
    }
    QCOMPARE(arr.at(0), 'X');
}

void tst_QByteArray::toFromHex_data()
{
    QTest::addColumn<QByteArray>("str");
    QTest::addColumn<QByteArray>("hex");
    QTest::addColumn<QByteArray>("hex_alt1");

    QTest::newRow("Qt is great!")
        << QByteArray("Qt is great!")
        << QByteArray("517420697320677265617421")
        << QByteArray("51 74 20 69 73 20 67 72 65 61 74 21");

    QTest::newRow("Qt is so great!")
        << QByteArray("Qt is so great!")
        << QByteArray("517420697320736f20677265617421")
        << QByteArray("51 74 20 69 73 20 73 6f 20 67 72 65 61 74 21");

    QTest::newRow("default-constructed")
        << QByteArray()
        << QByteArray()
        << QByteArray();

    QTest::newRow("empty")
        << QByteArray("")
        << QByteArray("")
        << QByteArray("");

    QTest::newRow("array-of-null")
        << QByteArray("\0", 1)
        << QByteArray("00")
        << QByteArray("0");

    QTest::newRow("no-leading-zero")
        << QByteArray("\xf")
        << QByteArray("0f")
        << QByteArray("f");

    QTest::newRow("single-byte")
        << QByteArray("\xaf")
        << QByteArray("af")
        << QByteArray("xaf");

    QTest::newRow("no-leading-zero")
        << QByteArray("\xd\xde\xad\xc0\xde")
        << QByteArray("0ddeadc0de")
        << QByteArray("ddeadc0de");

    QTest::newRow("garbage")
        << QByteArray("\xC\xde\xeC\xea\xee\xDe\xee\xee")
        << QByteArray("0cdeeceaeedeeeee")
        << QByteArray("Code less. Create more. Deploy everywhere.");

    QTest::newRow("under-defined-1")
        << QByteArray("\x1\x23")
        << QByteArray("0123")
        << QByteArray("x123");

    QTest::newRow("under-defined-2")
        << QByteArray("\x12\x34")
        << QByteArray("1234")
        << QByteArray("x1234");
}

void tst_QByteArray::toFromHex()
{
    QFETCH(QByteArray, str);
    QFETCH(QByteArray, hex);
    QFETCH(QByteArray, hex_alt1);

    {
        const QByteArray th = str.toHex();
        QCOMPARE(th.size(), hex.size());
        QCOMPARE(th, hex);
    }

    {
        const QByteArray fh = QByteArray::fromHex(hex);
        QCOMPARE(fh.size(), str.size());
        QCOMPARE(fh, str);
    }

    QCOMPARE(QByteArray::fromHex(hex_alt1), str);
}

void tst_QByteArray::toFromPercentEncoding()
{
    QByteArray arr("Qt is great!");

    QByteArray data = arr.toPercentEncoding();
    QCOMPARE(QString(data), QString("Qt%20is%20great%21"));
    QCOMPARE(QByteArray::fromPercentEncoding(data), arr);

    data = arr.toPercentEncoding("! ", "Qt");
    QCOMPARE(QString(data), QString("%51%74 is grea%74!"));
    QCOMPARE(QByteArray::fromPercentEncoding(data), arr);

    data = arr.toPercentEncoding(QByteArray(), "abcdefghijklmnopqrstuvwxyz", 'Q');
    QCOMPARE(QString(data), QString("Q51Q74Q20Q69Q73Q20Q67Q72Q65Q61Q74Q21"));
    QCOMPARE(QByteArray::fromPercentEncoding(data, 'Q'), arr);

    // verify that to/from percent encoding preserves nullity
    arr = "";
    QVERIFY(arr.isEmpty());
    QVERIFY(!arr.isNull());
    QVERIFY(arr.toPercentEncoding().isEmpty());
    QVERIFY(!arr.toPercentEncoding().isNull());
    QVERIFY(QByteArray::fromPercentEncoding("").isEmpty());
    QVERIFY(!QByteArray::fromPercentEncoding("").isNull());

    arr = QByteArray();
    QVERIFY(arr.isEmpty());
    QVERIFY(arr.isNull());
    QVERIFY(arr.toPercentEncoding().isEmpty());
    QVERIFY(arr.toPercentEncoding().isNull());
    QVERIFY(QByteArray::fromPercentEncoding(QByteArray()).isEmpty());
    QVERIFY(QByteArray::fromPercentEncoding(QByteArray()).isNull());
}

void tst_QByteArray::fromPercentEncoding_data()
{
    QTest::addColumn<QByteArray>("encodedString");
    QTest::addColumn<QByteArray>("decodedString");

    QTest::newRow("NormalString") << QByteArray("filename") << QByteArray("filename");
    QTest::newRow("NormalStringEncoded") << QByteArray("file%20name") << QByteArray("file name");
    QTest::newRow("JustEncoded") << QByteArray("%20") << QByteArray(" ");
    QTest::newRow("HTTPUrl") << QByteArray("http://qt-project.org") << QByteArray("http://qt-project.org");
    QTest::newRow("HTTPUrlEncoded") << QByteArray("http://qt-project%20org") << QByteArray("http://qt-project org");
    QTest::newRow("EmptyString") << QByteArray("") << QByteArray("");
    QTest::newRow("Task27166") << QByteArray("Fran%C3%A7aise") << QByteArray("Française");
}

void tst_QByteArray::fromPercentEncoding()
{
    QFETCH(QByteArray, encodedString);
    QFETCH(QByteArray, decodedString);

    QCOMPARE(QByteArray::fromPercentEncoding(encodedString), decodedString);
}

void tst_QByteArray::toPercentEncoding_data()
{
    QTest::addColumn<QByteArray>("decodedString");
    QTest::addColumn<QByteArray>("encodedString");

    QTest::newRow("NormalString") << QByteArray("filename") << QByteArray("filename");
    QTest::newRow("NormalStringEncoded") << QByteArray("file name") << QByteArray("file%20name");
    QTest::newRow("JustEncoded") << QByteArray(" ") << QByteArray("%20");
    QTest::newRow("HTTPUrl") << QByteArray("http://qt-project.org") << QByteArray("http%3A//qt-project.org");
    QTest::newRow("HTTPUrlEncoded") << QByteArray("http://qt-project org") << QByteArray("http%3A//qt-project%20org");
    QTest::newRow("EmptyString") << QByteArray("") << QByteArray("");
    QTest::newRow("Task27166") << QByteArray("Française") << QByteArray("Fran%C3%A7aise");
}

void tst_QByteArray::toPercentEncoding()
{
    QFETCH(QByteArray, decodedString);
    QFETCH(QByteArray, encodedString);

    QCOMPARE(decodedString.toPercentEncoding("/.").constData(), encodedString.constData());
}

void tst_QByteArray::toPercentEncoding2_data()
{
    QTest::addColumn<QByteArray>("original");
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<QByteArray>("excludeInEncoding");
    QTest::addColumn<QByteArray>("includeInEncoding");

    QTest::newRow("test_01") << QByteArray("abcdevghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012345678-._~")
                          << QByteArray("abcdevghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012345678-._~")
                          << QByteArray("")
                          << QByteArray("");
    QTest::newRow("test_02") << QByteArray("{\t\n\r^\"abc}")
                          << QByteArray("%7B%09%0A%0D%5E%22abc%7D")
                          << QByteArray("")
                          << QByteArray("");
    QTest::newRow("test_03") << QByteArray("://?#[]@!$&'()*+,;=")
                          << QByteArray("%3A%2F%2F%3F%23%5B%5D%40%21%24%26%27%28%29%2A%2B%2C%3B%3D")
                          << QByteArray("")
                          << QByteArray("");
    QTest::newRow("test_04") << QByteArray("://?#[]@!$&'()*+,;=")
                          << QByteArray("%3A%2F%2F%3F%23%5B%5D%40!$&'()*+,;=")
                          << QByteArray("!$&'()*+,;=")
                          << QByteArray("");
    QTest::newRow("test_05") << QByteArray("abcd")
                          << QByteArray("a%62%63d")
                          << QByteArray("")
                          << QByteArray("bc");
}

void tst_QByteArray::toPercentEncoding2()
{
    QFETCH(QByteArray, original);
    QFETCH(QByteArray, encoded);
    QFETCH(QByteArray, excludeInEncoding);
    QFETCH(QByteArray, includeInEncoding);

    QByteArray encodedData = original.toPercentEncoding(excludeInEncoding, includeInEncoding);
    QCOMPARE(encodedData.constData(), encoded.constData());
    QCOMPARE(original, QByteArray::fromPercentEncoding(encodedData));
}

void tst_QByteArray::compare_data()
{
    QTest::addColumn<QByteArray>("str1");
    QTest::addColumn<QByteArray>("str2");
    QTest::addColumn<int>("result");

    QTest::newRow("null")      << QByteArray() << QByteArray() << 0;
    QTest::newRow("null-empty")<< QByteArray() << QByteArray("") << 0;
    QTest::newRow("empty-null")<< QByteArray("") << QByteArray() << 0;
    QTest::newRow("null-full") << QByteArray() << QByteArray("abc") << -1;
    QTest::newRow("full-null") << QByteArray("abc") << QByteArray() << +1;
    QTest::newRow("empty-full")<< QByteArray("") << QByteArray("abc") << -1;
    QTest::newRow("full-empty")<< QByteArray("abc") << QByteArray("") << +1;
    QTest::newRow("rawempty-full") << QByteArray::fromRawData("abc", 0) << QByteArray("abc") << -1;
    QTest::newRow("full-rawempty") << QByteArray("abc") << QByteArray::fromRawData("abc", 0) << +1;

    QTest::newRow("equal   1") << QByteArray("abc") << QByteArray("abc") << 0;
    QTest::newRow("equal   2") << QByteArray::fromRawData("abc", 3) << QByteArray("abc") << 0;
    QTest::newRow("equal   3") << QByteArray::fromRawData("abcdef", 3) << QByteArray("abc") << 0;
    QTest::newRow("equal   4") << QByteArray("abc") << QByteArray::fromRawData("abc", 3) << 0;
    QTest::newRow("equal   5") << QByteArray("abc") << QByteArray::fromRawData("abcdef", 3) << 0;
    QTest::newRow("equal   6") << QByteArray("a\0bc", 4) << QByteArray("a\0bc", 4) << 0;
    QTest::newRow("equal   7") << QByteArray::fromRawData("a\0bcdef", 4) << QByteArray("a\0bc", 4) << 0;
    QTest::newRow("equal   8") << QByteArray("a\0bc", 4) << QByteArray::fromRawData("a\0bcdef", 4) << 0;

    QTest::newRow("less    1") << QByteArray("000") << QByteArray("abc") << -1;
    QTest::newRow("less    2") << QByteArray::fromRawData("00", 3) << QByteArray("abc") << -1;
    QTest::newRow("less    3") << QByteArray("000") << QByteArray::fromRawData("abc", 3) << -1;
    QTest::newRow("less    4") << QByteArray("abc", 3) << QByteArray("abc", 4) << -1;
    QTest::newRow("less    5") << QByteArray::fromRawData("abc\0", 3) << QByteArray("abc\0", 4) << -1;
    QTest::newRow("less    6") << QByteArray("a\0bc", 4) << QByteArray("a\0bd", 4) << -1;

    QTest::newRow("greater 1") << QByteArray("abc") << QByteArray("000") << +1;
    QTest::newRow("greater 2") << QByteArray("abc") << QByteArray::fromRawData("00", 3) << +1;
    QTest::newRow("greater 3") << QByteArray("abcd") << QByteArray::fromRawData("abcd", 3) << +1;
    QTest::newRow("greater 4") << QByteArray("a\0bc", 4) << QByteArray("a\0bb", 4) << +1;
}

void tst_QByteArray::compare()
{
    QFETCH(QByteArray, str1);
    QFETCH(QByteArray, str2);
    QFETCH(int, result);

    const bool isEqual   = result == 0;
    const bool isLess    = result < 0;
    const bool isGreater = result > 0;

    // basic tests:
    QCOMPARE(str1 == str2, isEqual);
    QCOMPARE(str1 < str2, isLess);
    QCOMPARE(str1 > str2, isGreater);

    // composed tests:
    QCOMPARE(str1 <= str2, isLess || isEqual);
    QCOMPARE(str1 >= str2, isGreater || isEqual);
    QCOMPARE(str1 != str2, !isEqual);

    // inverted tests:
    QCOMPARE(str2 == str1, isEqual);
    QCOMPARE(str2 < str1, isGreater);
    QCOMPARE(str2 > str1, isLess);

    // composed, inverted tests:
    QCOMPARE(str2 <= str1, isGreater || isEqual);
    QCOMPARE(str2 >= str1, isLess || isEqual);
    QCOMPARE(str2 != str1, !isEqual);

    if (isEqual)
        QVERIFY(qHash(str1) == qHash(str2));
}

void tst_QByteArray::compareCharStar_data()
{
    QTest::addColumn<QByteArray>("str1");
    QTest::addColumn<QString>("string2");
    QTest::addColumn<int>("result");

    QTest::newRow("null-null") << QByteArray() << QString() << 0;
    QTest::newRow("null-empty") << QByteArray() << "" << 0;
    QTest::newRow("null-full") << QByteArray() << "abc" << -1;
    QTest::newRow("empty-null") << QByteArray("") << QString() << 0;
    QTest::newRow("empty-empty") << QByteArray("") << "" << 0;
    QTest::newRow("empty-full") << QByteArray("") << "abc" << -1;
    QTest::newRow("raw-null") << QByteArray::fromRawData("abc", 0) << QString() << 0;
    QTest::newRow("raw-empty") << QByteArray::fromRawData("abc", 0) << QString("") << 0;
    QTest::newRow("raw-full") << QByteArray::fromRawData("abc", 0) << "abc" << -1;

    QTest::newRow("full-null") << QByteArray("abc") << QString() << +1;
    QTest::newRow("full-empty") << QByteArray("abc") << "" << +1;

    QTest::newRow("equal1") << QByteArray("abc") << "abc" << 0;
    QTest::newRow("equal2") << QByteArray("abcd", 3) << "abc" << 0;
    QTest::newRow("equal3") << QByteArray::fromRawData("abcd", 3) << "abc" << 0;

    QTest::newRow("less1") << QByteArray("ab") << "abc" << -1;
    QTest::newRow("less2") << QByteArray("abb") << "abc" << -1;
    QTest::newRow("less3") << QByteArray::fromRawData("abc", 2) << "abc" << -1;
    QTest::newRow("less4") << QByteArray("", 1) << "abc" << -1;
    QTest::newRow("less5") << QByteArray::fromRawData("", 1) << "abc" << -1;
    QTest::newRow("less6") << QByteArray("a\0bc", 4) << "a.bc" << -1;

    QTest::newRow("greater1") << QByteArray("ac") << "abc" << +1;
    QTest::newRow("greater2") << QByteArray("abd") << "abc" << +1;
    QTest::newRow("greater3") << QByteArray("abcd") << "abc" << +1;
    QTest::newRow("greater4") << QByteArray::fromRawData("abcd", 4) << "abc" << +1;
}

void tst_QByteArray::compareCharStar()
{
    QFETCH(QByteArray, str1);
    QFETCH(QString, string2);
    QFETCH(int, result);

    const bool isEqual   = result == 0;
    const bool isLess    = result < 0;
    const bool isGreater = result > 0;
    QByteArray qba = string2.toLatin1();
    const char *str2 = qba.constData();
    if (string2.isNull())
        str2 = 0;

    // basic tests:
    QCOMPARE(str1 == str2, isEqual);
    QCOMPARE(str1 < str2, isLess);
    QCOMPARE(str1 > str2, isGreater);

    // composed tests:
    QCOMPARE(str1 <= str2, isLess || isEqual);
    QCOMPARE(str1 >= str2, isGreater || isEqual);
    QCOMPARE(str1 != str2, !isEqual);

    // inverted tests:
    QCOMPARE(str2 == str1, isEqual);
    QCOMPARE(str2 < str1, isGreater);
    QCOMPARE(str2 > str1, isLess);

    // composed, inverted tests:
    QCOMPARE(str2 <= str1, isGreater || isEqual);
    QCOMPARE(str2 >= str1, isLess || isEqual);
    QCOMPARE(str2 != str1, !isEqual);
}

void tst_QByteArray::repeatedSignature() const
{
    /* repated() should be a const member. */
    const QByteArray string;
    string.repeated(3);
}

void tst_QByteArray::repeated() const
{
    QFETCH(QByteArray, string);
    QFETCH(QByteArray, expected);
    QFETCH(int, count);

    QCOMPARE(string.repeated(count), expected);
}

void tst_QByteArray::repeated_data() const
{
    QTest::addColumn<QByteArray>("string" );
    QTest::addColumn<QByteArray>("expected" );
    QTest::addColumn<int>("count" );

    /* Empty strings. */
    QTest::newRow("data1")
        << QByteArray()
        << QByteArray()
        << 0;

    QTest::newRow("data2")
        << QByteArray()
        << QByteArray()
        << -1004;

    QTest::newRow("data3")
        << QByteArray()
        << QByteArray()
        << 1;

    QTest::newRow("data4")
        << QByteArray()
        << QByteArray()
        << 5;

    /* On simple string. */
    QTest::newRow("data5")
        << QByteArray("abc")
        << QByteArray()
        << -1004;

    QTest::newRow("data6")
        << QByteArray("abc")
        << QByteArray()
        << -1;

    QTest::newRow("data7")
        << QByteArray("abc")
        << QByteArray()
        << 0;

    QTest::newRow("data8")
        << QByteArray("abc")
        << QByteArray("abc")
        << 1;

    QTest::newRow("data9")
        << QByteArray(("abc"))
        << QByteArray(("abcabc"))
        << 2;

    QTest::newRow("data10")
        << QByteArray(("abc"))
        << QByteArray(("abcabcabc"))
        << 3;

    QTest::newRow("data11")
        << QByteArray(("abc"))
        << QByteArray(("abcabcabcabc"))
        << 4;

    QTest::newRow("static not null terminated")
        << QByteArray(staticNotNullTerminated)
        << QByteArray("datadatadatadata")
        << 4;
    QTest::newRow("static standard")
        << QByteArray(staticStandard)
        << QByteArray("datadatadatadata")
        << 4;
    QTest::newRow("static shifted not null terminated")
        << QByteArray(staticShiftedNotNullTerminated)
        << QByteArray("datadatadatadata")
        << 4;
    QTest::newRow("static shifted")
        << QByteArray(staticShifted)
        << QByteArray("datadatadatadata")
        << 4;
}

void tst_QByteArray::byteRefDetaching() const
{
    {
        QByteArray str = "str";
        QByteArray copy;
        copy[0] = 'S';

        QCOMPARE(str, QByteArray("str"));
    }

    {
        char buf[] = { 's', 't', 'r' };
        QByteArray str = QByteArray::fromRawData(buf, 3);
        str[0] = 'S';

        QCOMPARE(buf[0], char('s'));
    }

    {
        static const char buf[] = { 's', 't', 'r' };
        QByteArray str = QByteArray::fromRawData(buf, 3);

        // this causes a crash in most systems if the detaching doesn't work
        str[0] = 'S';

        QCOMPARE(buf[0], char('s'));
    }
}

void tst_QByteArray::reserve()
{
    int capacity = 100;
    QByteArray qba;
    qba.reserve(capacity);
    QVERIFY(qba.capacity() == capacity);
    char *data = qba.data();

    for (int i = 0; i < capacity; i++) {
        qba.resize(i);
        QVERIFY(capacity == qba.capacity());
        QVERIFY(data == qba.data());
    }

    qba.resize(capacity);

    QByteArray copy = qba;
    qba.reserve(capacity / 2);
    QCOMPARE(qba.size(), capacity); // we didn't shrink the size!
    QCOMPARE(qba.capacity(), capacity);
    QCOMPARE(copy.capacity(), capacity);

    qba = copy;
    qba.reserve(capacity * 2);
    QCOMPARE(qba.size(), capacity);
    QCOMPARE(qba.capacity(), capacity * 2);
    QCOMPARE(copy.capacity(), capacity);
    QVERIFY(qba.constData() != data);

    QByteArray nil1, nil2;
    nil1.reserve(0);
    nil2.squeeze();
    nil1.squeeze();
    nil2.reserve(0);
}

void tst_QByteArray::reserveExtended_data()
{
    prependExtended_data();
}

void tst_QByteArray::reserveExtended()
{
    QFETCH(QByteArray, array);
    array.reserve(1024);
    QVERIFY(array.capacity() == 1024);
    QCOMPARE(array, QByteArray("data"));
    array.squeeze();
    QCOMPARE(array, QByteArray("data"));
    QCOMPARE(array.capacity(), array.size());
}

void tst_QByteArray::movablity_data()
{
    QTest::addColumn<QByteArray>("array");

    QTest::newRow("0x00000000") << QByteArray("\x00\x00\x00\x00", 4);
    QTest::newRow("0x000000ff") << QByteArray("\x00\x00\x00\xff", 4);
    QTest::newRow("0xffffffff") << QByteArray("\xff\xff\xff\xff", 4);
    QTest::newRow("empty") << QByteArray("");
    QTest::newRow("null") << QByteArray();
    QTest::newRow("sss") << QByteArray(3, 's');

    prependExtended_data();
}

void tst_QByteArray::movablity()
{
    QFETCH(QByteArray, array);

    QVERIFY(!QTypeInfo<QByteArray>::isStatic);

    const int size = array.size();
    const bool isEmpty = array.isEmpty();
    const bool isNull = array.isNull();
    const int capacity = array.capacity();

    QByteArray memSpace;

    // we need only memory space not the instance
    memSpace.~QByteArray();
    // move array -> memSpace
    memcpy(&memSpace, &array, sizeof(QByteArray));
    // reconstruct empty QByteArray
    new (&array) QByteArray;

    QCOMPARE(memSpace.size(), size);
    QCOMPARE(memSpace.isEmpty(), isEmpty);
    QCOMPARE(memSpace.isNull(), isNull);
    QCOMPARE(memSpace.capacity(), capacity);

    // try to not crash
    memSpace.toLower();
    memSpace.toUpper();
    memSpace.prepend('a');
    memSpace.append("b", 1);
    memSpace.squeeze();
    memSpace.reserve(array.size() + 16);

    QByteArray copy(memSpace);

    // reinitialize base values
    const int newSize = size + 2;
    const bool newIsEmpty = false;
    const bool newIsNull = false;
    const int newCapacity = 16;

    // move back memSpace -> array
    array.~QByteArray();
    memcpy(&array, &memSpace, sizeof(QByteArray));
    // reconstruct empty QByteArray
    new (&memSpace) QByteArray;

    QCOMPARE(array.size(), newSize);
    QCOMPARE(array.isEmpty(), newIsEmpty);
    QCOMPARE(array.isNull(), newIsNull);
    QCOMPARE(array.capacity(), newCapacity);
    QVERIFY(array.startsWith("a"));
    QVERIFY(array.endsWith("b"));

    QCOMPARE(copy.size(), newSize);
    QCOMPARE(copy.isEmpty(), newIsEmpty);
    QCOMPARE(copy.isNull(), newIsNull);
    QCOMPARE(copy.capacity(), newCapacity);
    QVERIFY(copy.startsWith("a"));
    QVERIFY(copy.endsWith("b"));

    // try to not crash
    array.squeeze();
    array.reserve(array.size() + 3);
    QVERIFY(true);
}

#if defined(Q_COMPILER_LAMBDA)
// Only tested on c++0x compliant compiler or gcc
void tst_QByteArray::literals()
{
    QByteArray str(QByteArrayLiteral("abcd"));

    QVERIFY(str.length() == 4);
    QVERIFY(str == "abcd");
    QVERIFY(str.data_ptr()->ref.isStatic());
    QVERIFY(str.data_ptr()->offset == sizeof(QByteArrayData));

    const char *s = str.constData();
    QByteArray str2 = str;
    QVERIFY(str2.constData() == s);

    // detach on non const access
    QVERIFY(str.data() != s);

    QVERIFY(str2.constData() == s);
    QVERIFY(str2.data() != s);
}
#endif

void tst_QByteArray::macTypes()
{
#ifndef Q_OS_MAC
    QSKIP("This is a Apple-only test");
#else
    extern void tst_QByteArray_macTypes(); // in qbytearray_mac.mm
    tst_QByteArray_macTypes();
#endif
}

const char globalChar = '1';

QTEST_MAIN(tst_QByteArray)
#include "tst_qbytearray.moc"
