// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifdef QT_NO_QSNPRINTF
# undef QT_NO_QSNPRINTF // test of the function
#endif

#include <QTest>

#include <qbytearray.h>
#include <qfile.h>
#include <qhash.h>
#include <limits.h>
#include <private/qtools_p.h>

#include "../shared/test_number_shared.h"

#include <QtCore/q20iterator.h>
#include <sstream>

using namespace Qt::StringLiterals;

class tst_QByteArray : public QObject
{
    Q_OBJECT

public:
    tst_QByteArray();
private slots:
    // Note: much of the shared API is tested in ../qbytearrayapisymmetry/
    void swap();
    void qChecksum_data();
    void qChecksum();
    void constByteArray();
    void leftJustified();
    void rightJustified();
    void setNum();
    void iterators();
    void reverseIterators();
    void split_data();
    void split();
    void base64_data();
    void base64();
    void fromBase64_data();
    void fromBase64();
#if QT_DEPRECATED_SINCE(6, 9)
    void qvsnprintf();
#endif
    void qstrlen();
    void qstrnlen();
    void qstrcpy();
    void qstrncpy();
    void chop_data();
    void chop();
    void prepend();
    void prependExtended_data();
    void prependExtended();
    void append();
    void appendFromRawData();
    void appendExtended_data();
    void appendExtended();
    void nullTerminated();
    void appendEmptyNull();
    void assign();
    void assignShared();
    void assignUsesPrependBuffer();
    void insert();
    void insertExtended_data();
    void insertExtended();
    void remove_data();
    void remove();
    void remove_extra();
    void removeIf();
    void erase();
    void erase_single_arg();

    void replace_pos_len_data();
    void replace_pos_len();
    void replace_before_after_data();
    void replace_before_after();
    void replace_after_points_into_this_data();
    void replace_after_points_into_this();
    void replace_view_view_data();
    void replace_view_view();
    void replaceWithSpecifiedLength();
    void replaceWithEmptyNeedleInsertsBeforeEachChar_data();
    void replaceWithEmptyNeedleInsertsBeforeEachChar();
    void replaceDoesNotReplaceTheTerminatingNull();

    void number();
    void number_double_data();
    void number_double();
    void number_base_data();
    void number_base();
    void nullness();
    void blockSizeCalculations();

    void resizeAfterFromRawData();
    void toFromHex_data();
    void toFromHex();
    void toFromPercentEncoding();
    void fromPercentEncoding_data();
    void fromPercentEncoding();
    void toPercentEncoding_data();
    void toPercentEncoding();
    void pecentEncodingRoundTrip_data();
    void pecentEncodingRoundTrip();

    void qstrcmp_data();
    void qstrcmp();
    void compare_singular();
    void compareCharStar_data();
    void compareCharStar();

    void repeatedSignature() const;
    void repeated() const;
    void repeated_data() const;

    void byteRefDetaching() const;

    void reserve();
    void reserveExtended_data();
    void reserveExtended();
    void resize();
    void movability_data();
    void movability();
    void literals();
    void userDefinedLiterals();
    void toUpperLower_data();
    void toUpperLower();
    void isUpper();
    void isLower();

    void indexOf_data();
    void indexOf();
    void lastIndexOf_data();
    void lastIndexOf();

    void macTypes();

    void stdString();

    void emptyAndClear();
    void fill();
    void dataPointers();
    void truncate();
    void trimmed_data();
    void trimmed();
    void simplified();
    void simplified_data();
    void left();
    void right();
    void mid();
    void length();
    void length_data();
    void slice() const;
    void std_stringview_conversion();
};

static const QByteArray::DataPointer staticStandard = {
    nullptr,
    const_cast<char *>("data"),
    4
};
static const QByteArray::DataPointer staticNotNullTerminated = {
    nullptr,
    const_cast<char *>("dataBAD"),
    4
};

template <typename String> String detached(String s)
{
    if (!s.isNull()) { // detaching loses nullness, but we need to preserve it
        auto d = s.data();
        Q_UNUSED(d);
    }
    return s;
}

template <class T> const T &verifyZeroTermination(const T &t) { return t; }

QByteArray verifyZeroTermination(const QByteArray &ba)
{
    // This test does some evil stuff, it's all supposed to work.

    QByteArray::DataPointer baDataPtr = const_cast<QByteArray &>(ba).data_ptr();

    // Skip if !isMutable() as those offer no guarantees
    if (!baDataPtr->isMutable())
        return ba;

    qsizetype baSize = ba.size();
    char baTerminator = ba.constData()[baSize];
    if ('\0' != baTerminator)
        return QString::fromUtf8(
            "*** Result ('%1') not null-terminated: 0x%2 ***").arg(QString::fromUtf8(ba))
                .arg(int(baTerminator), 2, 16, QChar('0')).toUtf8();

    // Skip mutating checks on shared strings
    if (baDataPtr->isShared())
        return ba;

    const char *baData = ba.constData();
    const QByteArray baCopy(baData, baSize); // Deep copy

    const_cast<char *>(baData)[baSize] = 'x';
    if ('x' != ba.constData()[baSize]) {
        return "*** Failed to replace null-terminator in "
                "result ('" + ba + "') ***";
    }
    if (ba != baCopy) {
        return  "*** Result ('" + ba + "') differs from its copy "
                "after null-terminator was replaced ***";
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

Q_DECLARE_METATYPE(QByteArray::Base64DecodingStatus);

tst_QByteArray::tst_QByteArray()
{
}

void tst_QByteArray::qChecksum_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<uint>("len");
    QTest::addColumn<Qt::ChecksumType>("standard");
    QTest::addColumn<uint>("checksum");

    // Examples from ISO 14443-3
    QTest::newRow("1") << QByteArray("\x00\x00", 2)         << 2U << Qt::ChecksumItuV41  << 0x1EA0U;
    QTest::newRow("2") << QByteArray("\x12\x34", 2)         << 2U << Qt::ChecksumItuV41  << 0xCF26U;
    QTest::newRow("3") << QByteArray("\x00\x00\x00", 3)     << 3U << Qt::ChecksumIso3309 << 0xC6CCU;
    QTest::newRow("4") << QByteArray("\x0F\xAA\xFF", 3)     << 3U << Qt::ChecksumIso3309 << 0xD1FCU;
    QTest::newRow("5") << QByteArray("\x0A\x12\x34\x56", 4) << 4U << Qt::ChecksumIso3309 << 0xF62CU;
}

void tst_QByteArray::qChecksum()
{
    QFETCH(QByteArray, data);
    QFETCH(uint, len);
    QFETCH(Qt::ChecksumType, standard);
    QFETCH(uint, checksum);

    QCOMPARE(data.size(), int(len));
    if (standard == Qt::ChecksumIso3309) {
        QCOMPARE(::qChecksum(QByteArrayView(data.constData(), len)), static_cast<quint16>(checksum));
    }
    QCOMPARE(::qChecksum(QByteArrayView(data.constData(), len), standard), static_cast<quint16>(checksum));
}


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

    QCOMPARE(a.leftJustified(3, '-'), QByteArray("---"));
    QCOMPARE(a.leftJustified(2, ' '), QByteArray("  "));
    QVERIFY(!a.isDetached());

    a = "ABC";
    QCOMPARE(a.leftJustified(5,'-'), QByteArray("ABC--"));
    QCOMPARE(a.leftJustified(4,'-'), QByteArray("ABC-"));
    QCOMPARE(a.leftJustified(4), QByteArray("ABC "));
    QCOMPARE(a.leftJustified(3), QByteArray("ABC"));
    QCOMPARE(a.leftJustified(2), QByteArray("ABC"));
    QCOMPARE(a.leftJustified(1), QByteArray("ABC"));
    QCOMPARE(a.leftJustified(0), QByteArray("ABC"));

    QCOMPARE(a.leftJustified(4,' ',true), QByteArray("ABC "));
    QCOMPARE(a.leftJustified(3,' ',true), QByteArray("ABC"));
    QCOMPARE(a.leftJustified(2,' ',true), QByteArray("AB"));
    QCOMPARE(a.leftJustified(1,' ',true), QByteArray("A"));
    QCOMPARE(a.leftJustified(0,' ',true), QByteArray(""));
}

void tst_QByteArray::rightJustified()
{
    QByteArray a;

    QCOMPARE(a.rightJustified(3, '-'), QByteArray("---"));
    QCOMPARE(a.rightJustified(2, ' '), QByteArray("  "));
    QVERIFY(!a.isDetached());

    a="ABC";
    QCOMPARE(a.rightJustified(5,'-'),QByteArray("--ABC"));
    QCOMPARE(a.rightJustified(4,'-'),QByteArray("-ABC"));
    QCOMPARE(a.rightJustified(4),QByteArray(" ABC"));
    QCOMPARE(a.rightJustified(3),QByteArray("ABC"));
    QCOMPARE(a.rightJustified(2),QByteArray("ABC"));
    QCOMPARE(a.rightJustified(1),QByteArray("ABC"));
    QCOMPARE(a.rightJustified(0),QByteArray("ABC"));

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

    QCOMPARE(a.setNum(short(-1), 16), QByteArray("-1"));
    QCOMPARE(a.setNum(int(-1), 16), QByteArray("-1"));
    QCOMPARE(a.setNum(qlonglong(-1), 16), QByteArray("-1"));

    QCOMPARE(a.setNum(short(-1), 10), QByteArray("-1"));
    QCOMPARE(a.setNum(int(-1), 10), QByteArray("-1"));
    QCOMPARE(a.setNum(qlonglong(-1), 10), QByteArray("-1"));

    QCOMPARE(a.setNum(-123), QByteArray("-123"));
    QCOMPARE(a.setNum(0x123, 16), QByteArray("123"));
    QCOMPARE(a.setNum(short(123)), QByteArray("123"));

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

void tst_QByteArray::iterators()
{
    QByteArray emptyArr;
    QCOMPARE(emptyArr.constBegin(), emptyArr.constEnd());
    QCOMPARE(emptyArr.cbegin(), emptyArr.cend());
    QVERIFY(!emptyArr.isDetached());
    QCOMPARE(emptyArr.begin(), emptyArr.end());

    QByteArray a("0123456789");

    auto it = a.begin();
    auto constIt = a.cbegin();
    qsizetype idx = 0;

    QCOMPARE(*it, a[idx]);
    QCOMPARE(*constIt, a[idx]);

    it++;
    constIt++;
    idx++;
    QCOMPARE(*it, a[idx]);
    QCOMPARE(*constIt, a[idx]);

    it += 5;
    constIt += 5;
    idx += 5;
    QCOMPARE(*it, a[idx]);
    QCOMPARE(*constIt, a[idx]);

    it -= 3;
    constIt -= 3;
    idx -= 3;
    QCOMPARE(*it, a[idx]);
    QCOMPARE(*constIt, a[idx]);

    it--;
    constIt--;
    idx--;
    QCOMPARE(*it, a[idx]);
    QCOMPARE(*constIt, a[idx]);
}

void tst_QByteArray::reverseIterators()
{
    QByteArray emptyArr;
    QCOMPARE(emptyArr.crbegin(), emptyArr.crend());
    QVERIFY(!emptyArr.isDetached());
    QCOMPARE(emptyArr.rbegin(), emptyArr.rend());

    QByteArray s = "1234";
    QByteArray sr = s;
    std::reverse(sr.begin(), sr.end());
    const QByteArray &csr = sr;
    QVERIFY(std::equal(s.begin(), s.end(), sr.rbegin()));
    QVERIFY(std::equal(s.begin(), s.end(), sr.crbegin()));
    QVERIFY(std::equal(s.begin(), s.end(), csr.rbegin()));
    QVERIFY(std::equal(sr.rbegin(), sr.rend(), s.begin()));
    QVERIFY(std::equal(sr.crbegin(), sr.crend(), s.begin()));
    QVERIFY(std::equal(csr.rbegin(), csr.rend(), s.begin()));
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
    QTest::newRow("null") << QByteArray() << 1;
}

void tst_QByteArray::split()
{
    QFETCH(QByteArray, sample);
    QFETCH(int, size);

    QList<QByteArray> list = sample.split(' ');
    QCOMPARE(list.size(), size);
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

    QTest::newRow("null") << QByteArray() << QByteArray();
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
    QByteArray::FromBase64Result result;

    result = QByteArray::fromBase64Encoding(base64, QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(result);
    QCOMPARE(result.decoded, rawdata);

    QByteArray arr = base64;
    result = QByteArray::fromBase64Encoding(std::move(arr), QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(result);
    QCOMPARE(result.decoded, rawdata);

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
    QTest::addColumn<QByteArray::Base64DecodingStatus>("status");

    QTest::newRow("1") << QByteArray("") << QByteArray("  ") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("2") << QByteArray("1") << QByteArray("MQ=") << QByteArray::Base64DecodingStatus::IllegalInputLength;
    QTest::newRow("3") << QByteArray("12") << QByteArray("MTI       ") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("4") << QByteArray("123") << QByteArray("M=TIz") << QByteArray::Base64DecodingStatus::IllegalInputLength;
    QTest::newRow("5") << QByteArray("1234") << QByteArray("MTI zN A ") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("6") << QByteArray("\n") << QByteArray("Cg@") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("7") << QByteArray("a\n") << QByteArray("======YQo=") << QByteArray::Base64DecodingStatus::IllegalInputLength;
    QTest::newRow("8") << QByteArray("ab\n") << QByteArray("Y\nWIK ") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("9") << QByteArray("abc\n") << QByteArray("YWJjCg=") << QByteArray::Base64DecodingStatus::IllegalInputLength;
    QTest::newRow("a") << QByteArray("abcd\n") << QByteArray("YWJ\1j\x9cZAo=") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("b") << QByteArray("abcde\n") << QByteArray("YW JjZ\n G\tUK") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("c") << QByteArray("abcdef\n") << QByteArray("YWJjZGVmCg=") << QByteArray::Base64DecodingStatus::IllegalInputLength;
    QTest::newRow("d") << QByteArray("abcdefg\n") << QByteArray("YWJ\rjZGVmZwo") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("e") << QByteArray("abcdefgh\n") << QByteArray("YWJjZGVmZ2gK====") << QByteArray::Base64DecodingStatus::IllegalPadding;

    QByteArray ba;
    ba.resize(256);
    for (int i = 0; i < 256; ++i)
        ba[i] = i;
    QTest::newRow("f") << ba << QByteArray("AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Nj\n"
                                           "c4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG1u\n"
                                           "b3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqOkpa\n"
                                           "anqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd\n"
                                           "3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==                            ") << QByteArray::Base64DecodingStatus::IllegalCharacter;


    QTest::newRow("g") << QByteArray("foo\0bar", 7) << QByteArray("Zm9vAGJhcg=") << QByteArray::Base64DecodingStatus::IllegalInputLength;
    QTest::newRow("h") << QByteArray("f\xd1oo\x9ctar") << QByteArray("ZtFvb5x 0YXI") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("i") << QByteArray("\"\0\0\0\0\0\0\"", 8) << QByteArray("IgAAAAAAACI ") << QByteArray::Base64DecodingStatus::IllegalCharacter;
}


void tst_QByteArray::fromBase64()
{
    QFETCH(QByteArray, rawdata);
    QFETCH(QByteArray, base64);
    QFETCH(QByteArray::Base64DecodingStatus, status);

    QByteArray::FromBase64Result result;

    result = QByteArray::fromBase64Encoding(base64);
    QVERIFY(result);
    QCOMPARE(result.decoded, rawdata);

    result = QByteArray::fromBase64Encoding(base64, QByteArray::Base64Encoding);
    QVERIFY(result);
    QCOMPARE(result.decoded, rawdata);

    result = QByteArray::fromBase64Encoding(base64, QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(!result);
    QCOMPARE(result.decodingStatus, status);
    QVERIFY(result.decoded.isEmpty());

    QByteArray arr = base64;
    QVERIFY(!arr.isDetached());
    result = QByteArray::fromBase64Encoding(std::move(arr), QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(!arr.isEmpty());
    QVERIFY(!result);
    QCOMPARE(result.decodingStatus, status);
    QVERIFY(result.decoded.isEmpty());

    arr.detach();
    QVERIFY(arr.isDetached());
    result = QByteArray::fromBase64Encoding(std::move(arr), QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(arr.isEmpty());
    QVERIFY(!result);
    QCOMPARE(result.decodingStatus, status);
    QVERIFY(result.decoded.isEmpty());

    // try "base64url" encoding
    QByteArray base64url = base64;
    base64url.replace('/', '_').replace('+', '-');
    result = QByteArray::fromBase64Encoding(base64url, QByteArray::Base64UrlEncoding);
    QVERIFY(result);
    QCOMPARE(result.decoded, rawdata);

    result = QByteArray::fromBase64Encoding(base64url, QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(!result);
    QCOMPARE(result.decodingStatus, status);
    QVERIFY(result.decoded.isEmpty());

    arr = base64url;
    arr.detach();
    result = QByteArray::fromBase64Encoding(std::move(arr), QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(arr.isEmpty());
    QVERIFY(!result);
    QCOMPARE(result.decodingStatus, status);
    QVERIFY(result.decoded.isEmpty());

    if (base64 != base64url) {
        // check that the invalid decodings fail
        result = QByteArray::fromBase64Encoding(base64, QByteArray::Base64UrlEncoding);
        QVERIFY(result);
        QVERIFY(result.decoded != rawdata);
        result = QByteArray::fromBase64Encoding(base64url, QByteArray::Base64Encoding);
        QVERIFY(result);
        QVERIFY(result.decoded != rawdata);

        result = QByteArray::fromBase64Encoding(base64, QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
        QVERIFY(!result);
        QVERIFY(result.decoded.isEmpty());

        arr = base64;
        arr.detach();
        result = QByteArray::fromBase64Encoding(std::move(arr), QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
        QVERIFY(arr.isEmpty());
        QVERIFY(!result);
        QVERIFY(result.decoded.isEmpty());

        result = QByteArray::fromBase64Encoding(base64url, QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
        QVERIFY(!result);
        QVERIFY(result.decoded.isEmpty());

        arr = base64url;
        arr.detach();
        result = QByteArray::fromBase64Encoding(std::move(arr), QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
        QVERIFY(arr.isEmpty());
        QVERIFY(!result);
        QVERIFY(result.decoded.isEmpty());
    }

    // also remove padding, if any, and test again. note that by doing
    // that we might be sanitizing the illegal input, so we can't assume now
    // that result will be invalid in all cases
    {
        auto rightmostNotEqualSign = std::find_if_not(base64url.rbegin(), base64url.rend(), [](char c) { return c == '='; });
        base64url.chop(std::distance(base64url.rbegin(), rightmostNotEqualSign)); // no QByteArray::erase...
    }

    result = QByteArray::fromBase64Encoding(base64url, QByteArray::Base64UrlEncoding);
    QVERIFY(result);
    QCOMPARE(result.decoded, rawdata);

    result = QByteArray::fromBase64Encoding(base64url, QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
    if (result) {
        QCOMPARE(result.decoded, rawdata);
    } else {
        QCOMPARE(result.decodingStatus, status);
        QVERIFY(result.decoded.isEmpty());
    }

    arr = base64url;
    arr.detach();
    result = QByteArray::fromBase64Encoding(std::move(arr), QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(arr.isEmpty());
    if (result) {
        QCOMPARE(result.decoded, rawdata);
    } else {
        QCOMPARE(result.decodingStatus, status);
        QVERIFY(result.decoded.isEmpty());
    }
}

#if QT_DEPRECATED_SINCE(6, 9)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
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
# if defined(_MSC_VER)
    QCOMPARE(::qsnprintf(buf, 3, "%s", "bubu"), -1);
    QCOMPARE(static_cast<const char*>(buf), "bu");
# else
    // windows has to do everything different, of course.
    QCOMPARE(::qsnprintf(buf, 3, "%s", "bubu"), -1);
    buf[19] = '\0';
    QCOMPARE(static_cast<const char *>(buf), "bub****************");
# endif
#else
    QCOMPARE(::qsnprintf(buf, 3, "%s", "bubu"), 4);
    QCOMPARE(static_cast<const char*>(buf), "bu");
#endif
    QCOMPARE(buf[4], char(42));

#ifndef Q_OS_WIN
    memset(buf, 42, sizeof(buf));
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wformat-zero-length")
    QT_WARNING_DISABLE_CLANG("-Wformat-zero-length")
    QCOMPARE(::qsnprintf(buf, 10, ""), 0);
    QT_WARNING_POP
#endif
}
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 9)


void tst_QByteArray::qstrlen()
{
    const char *src = "Something about ... \0 a string.";
    QCOMPARE(::qstrlen(nullptr), size_t(0));
    QCOMPARE(::qstrlen(src), size_t(20));
}

void tst_QByteArray::qstrnlen()
{
    const char *src = "Something about ... \0 a string.";
    QCOMPARE(::qstrnlen(nullptr, 1), size_t(0));
    QCOMPARE(::qstrnlen(src, 31), size_t(20));
    QCOMPARE(::qstrnlen(src, 19), size_t(19));
    QCOMPARE(::qstrnlen(src, 21), size_t(20));
    QCOMPARE(::qstrnlen(src, 20), size_t(20));
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

    // dst == nullptr
    QCOMPARE(::qstrncpy(0, src.data(),  0), (char*)0);
    QCOMPARE(::qstrncpy(0, src.data(), 10), (char*)0);

    // src == nullptr
    QCOMPARE(::qstrncpy(dst.data(), 0,  0), (char*)0);
    QCOMPARE(*dst.data(), 'b'); // must not have written to dst
    QCOMPARE(::qstrncpy(dst.data(), 0, 10), (char*)0);
    QCOMPARE(*dst.data(), '\0'); // must have written to dst
    *dst.data() = 'b'; // restore

    // valid pointers, but len == 0
    QCOMPARE(::qstrncpy(dst.data(), src.data(), 0), dst.data());
    QCOMPARE(*dst.data(), 'b'); // must not have written to dst

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
    QTest::newRow("null 0") << QByteArray() << 0 << QByteArray();
    QTest::newRow("null 10") << QByteArray() << 10 << QByteArray();
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
    const char data[] = "data";

    QCOMPARE(QByteArray().prepend(QByteArray()), QByteArray());
    QCOMPARE(QByteArray().prepend('a'), QByteArray("a"));
    QCOMPARE(QByteArray().prepend(2, 'a'), QByteArray("aa"));
    QCOMPARE(QByteArray().prepend(QByteArray("data")), QByteArray("data"));
    QCOMPARE(QByteArray().prepend(data), QByteArray("data"));
    QCOMPARE(QByteArray().prepend(data, 2), QByteArray("da"));
    QCOMPARE(QByteArray().prepend(QByteArrayView(data)), QByteArray("data"));

    QByteArray ba("foo");
    QCOMPARE(ba.prepend((char*)0), QByteArray("foo"));
    QCOMPARE(ba.prepend(QByteArray()), QByteArray("foo"));
    QCOMPARE(ba.prepend("1"), QByteArray("1foo"));
    QCOMPARE(ba.prepend(QByteArray("2")), QByteArray("21foo"));
    QCOMPARE(ba.prepend('3'), QByteArray("321foo"));
    QCOMPARE(ba.prepend(-1, 'x'), QByteArray("321foo"));
    QCOMPARE(ba.prepend(3, 'x'), QByteArray("xxx321foo"));
    QCOMPARE(ba.prepend("\0 ", 2), QByteArray::fromRawData("\0 xxx321foo", 11));

    QByteArray tenChars;
    tenChars.reserve(10);
    QByteArray twoChars("ab");
    tenChars.prepend(twoChars);
    QCOMPARE(tenChars.capacity(), 10);
}

void tst_QByteArray::prependExtended_data()
{
    QTest::addColumn<QByteArray>("array");
    QTest::newRow("literal") << QByteArray(QByteArrayLiteral("data"));
    QTest::newRow("standard") << QByteArray(staticStandard);
    QTest::newRow("notNullTerminated") << QByteArray(staticNotNullTerminated);
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
    QCOMPARE(array.prepend(-1, 'x'), QByteArray("321data"));
    QCOMPARE(array.prepend(3, 'x'), QByteArray("xxx321data"));
    QCOMPARE(array.prepend("\0 ", 2), QByteArray::fromRawData("\0 xxx321data", 12));
    QCOMPARE(array.size(), 12);
}

void tst_QByteArray::append()
{
    const char data[] = "data";

    QCOMPARE(QByteArray().append(QByteArray()), QByteArray());
    QCOMPARE(QByteArray().append('a'), QByteArray("a"));
    QCOMPARE(QByteArray().append(2, 'a'), QByteArray("aa"));
    QCOMPARE(QByteArray().append(QByteArray("data")), QByteArray("data"));
    QCOMPARE(QByteArray().append(data), QByteArray("data"));
    QCOMPARE(QByteArray().append(data, -1), QByteArray("data"));
    QCOMPARE(QByteArray().append(data, 2), QByteArray("da"));
    QCOMPARE(QByteArray().append(QByteArrayView(data)), QByteArray("data"));

    QByteArray ba("foo");
    QCOMPARE(ba.append((char*)0), QByteArray("foo"));
    QCOMPARE(ba.append(QByteArray()), QByteArray("foo"));
    QCOMPARE(ba.append("1"), QByteArray("foo1"));
    QCOMPARE(ba.append(QByteArray("2")), QByteArray("foo12"));
    QCOMPARE(ba.append('3'), QByteArray("foo123"));
    QCOMPARE(ba.append(-1, 'x'), QByteArray("foo123"));
    QCOMPARE(ba.append(3, 'x'), QByteArray("foo123xxx"));
    QCOMPARE(ba.append("\0"), QByteArray("foo123xxx"));
    QCOMPARE(ba.append("\0", 1), QByteArray::fromRawData("foo123xxx\0", 10));
    QCOMPARE(ba.size(), 10);

    QByteArray tenChars;
    tenChars.reserve(10);
    QByteArray twoChars("ab");
    tenChars.append(twoChars);
    QCOMPARE(tenChars.capacity(), 10);

    {
        QByteArray prepended("abcd");
        prepended.prepend('a');
        const qsizetype freeAtEnd = prepended.data_ptr()->freeSpaceAtEnd();
        QVERIFY(prepended.size() + freeAtEnd < prepended.capacity());
        prepended += QByteArray(freeAtEnd, 'b');
        prepended.append('c');
        QCOMPARE(prepended, QByteArray("aabcd") + QByteArray(freeAtEnd, 'b') + QByteArray("c"));
    }

    {
        QByteArray prepended2("aaaaaaaaaa");
        while (prepended2.size())
            prepended2.remove(0, 1);
        QVERIFY(prepended2.data_ptr()->freeSpaceAtBegin() > 0);
        QByteArray array(prepended2.data_ptr()->freeSpaceAtEnd(), 'a');
        prepended2 += array;
        prepended2.append('b');
        QCOMPARE(prepended2, array + QByteArray("b"));
    }
}

void tst_QByteArray::appendFromRawData()
{
    char rawData[] = "Hello World!";
    QByteArray ba = QByteArray::fromRawData(rawData, std::size(rawData) - 1);

    QByteArray copy;
    copy.append(ba);
    QCOMPARE(copy, ba);
    // We make an _actual_ copy, because appending a byte array
    // created with fromRawData() might be optimized to copy the DataPointer,
    // which means we may point to temporary stack data.
    QCOMPARE_NE((void *)copy.constData(), (void *)ba.constData());
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
    QCOMPARE(array.append(-1, 'x'), QByteArray("data123"));
    QCOMPARE(array.append(3, 'x'), QByteArray("data123xxx"));
    QCOMPARE(array.append("\0"), QByteArray("data123xxx"));
    QCOMPARE(array.append("\0", 1), QByteArray::fromRawData("data123xxx\0", 11));
    QCOMPARE(array.size(), 11);
}

void tst_QByteArray::nullTerminated()
{
    const char ptr[] = {'A', 'B', 'C'};

    QTest::ThrowOnFailEnabler throwOnFail;

    auto check = [&ptr](const QByteArray &ba) {
        QCOMPARE_NE(reinterpret_cast<const void *>(ba.constData()), ptr);
        QCOMPARE(ba.constData()[0], ptr[0]);
        QCOMPARE(ba.constData()[1], ptr[1]);
        QCOMPARE(ba.constData()[2], '\0');
        QCOMPARE(ba.size(), 2);
    };

    {
        auto ba = QByteArray::fromRawData(ptr, 2);
        QCOMPARE_EQ(reinterpret_cast<const void *>(ba.constData()), ptr);
        QCOMPARE(ba.constData()[0], ptr[0]);
        QCOMPARE(ba.constData()[1], ptr[1]);
        QCOMPARE(ba.size(), 2);

        check(ba.nullTerminated());
        check(QByteArray::fromRawData(ptr, 2).nullTerminated()); // rvalue
    }

    {
        auto ba = QByteArray::fromRawData(ptr, 2);
        QCOMPARE_EQ(reinterpret_cast<const void *>(ba.constData()), ptr);
        QCOMPARE(ba.constData()[0], ptr[0]);
        QCOMPARE(ba.constData()[1], ptr[1]);
        QCOMPARE(ba.size(), 2);

        check(ba.nullTerminate());
    }
}

void tst_QByteArray::appendEmptyNull()
{
    QByteArray a;
    QVERIFY(a.isEmpty());
    QVERIFY(a.isNull());

    QByteArray b("");
    QVERIFY(b.isEmpty());
    QVERIFY(!b.isNull());

    // Concatenating a null and an empty-but-not-null byte arrays results in
    // an empty but not null byte array
    QByteArray r = a + b;
    QVERIFY(r.isEmpty());
    QVERIFY(!r.isNull());
}

void tst_QByteArray::assign()
{
    // QByteArray &assign(QByteArrayView)
    {
        QByteArray ba;
        QByteArray test("data");
        QCOMPARE(ba.assign(test), test);
        QCOMPARE(ba.size(), test.size());
        test = "data\0data";
        QCOMPARE(ba.assign(test), test);
        QCOMPARE(ba.size(), test.size());
        test = "data\0data"_ba;
        QCOMPARE(ba.assign(test), test);
        QCOMPARE(ba.size(), test.size());
    }
    // QByteArray &assign(qsizetype, char);
    {
        QByteArray ba;
        QByteArray test("ddd");
        QCOMPARE(ba.assign(3, 'd'), test);
        QCOMPARE(ba.size(), test.size());
        test = "xx";
        QCOMPARE(ba.assign(20, 'd').assign(2, 'x'), test);
        QCOMPARE(ba.size(), test.size());
        test = "ddddd";
        QCOMPARE(ba.assign(0, 'x').assign(5, 'd'), test);
        QCOMPARE(ba.size(), test.size());
        test = "\0\0\0"_ba;
        QCOMPARE(ba.assign(0, 'x').assign(3, '\0'), test);
        QCOMPARE(ba.size(), test.size());
    }
    // QByteArray &assign(InputIterator, InputIterator)
    {
        QByteArray ba;
        QByteArrayView test;

        QList<char> l = {'\0', 'T', 'E', 'S', 'T'};

        ba.assign(l.begin(), l.begin());
        QVERIFY(ba.isEmpty());
        QCOMPARE(*ba.constData(), '\0');

        ba.assign(l.begin(), l.end());
        test = "\0TEST"_ba;
        QCOMPARE(ba, test);
        QCOMPARE(ba.size(), test.size());

        const std::byte bytes[] = {std::byte('T'), std::byte(0), std::byte('S'), std::byte('T')};
        test = QByteArrayView::fromArray(bytes);
        QCOMPARE(ba.assign(test.begin(), test.end()), test);
        QCOMPARE(ba.size(), test.size());

        std::stringstream ss;
        ss << "T " << '\0' << ' ' << "S " << "T ";
        ba.assign(std::istream_iterator<char>{ss}, std::istream_iterator<char>{});
        test = "T\0ST"_ba;
        QCOMPARE(ba, test);
        QCOMPARE(ba.size(), test.size());

        ba.assign(l.begin(), l.begin());
        QVERIFY(ba.isEmpty());
        QCOMPARE(*ba.constData(), '\0');
    }
    // Test chaining
    {
        QByteArray ba;
        QByteArray test("TTTTT");
        char arr[] = {'T', 'E', 'S', 'T'};
        ba.assign(std::begin(arr), std::end(arr)).assign({"Hello World!"}).assign(5, 'T');
        QCOMPARE(ba, test);
        QCOMPARE(ba.size(), test.size());
        test = "DATA";
        QCOMPARE(ba.assign(300, 'T').assign({"DATA"}), test);
        QCOMPARE(ba.size(), test.size());
        test = QByteArray(arr, q20::ssize(arr));
        QCOMPARE(ba.assign(10, 'c').assign(std::begin(arr), std::end(arr)), test);
        QCOMPARE(ba.size(), test.size());
        test = "TTT";
        QCOMPARE(ba.assign("data").assign(QByteArrayView::fromArray(
                         {std::byte('T'), std::byte('T'), std::byte('T')})), test);
        QCOMPARE(ba.size(), test.size());
        test = "\0data";
        QCOMPARE(ba.assign("data").assign("\0data"), test);
        QCOMPARE(ba.size(), test.size());
    }
}

void tst_QByteArray::assignShared()
{
    {
        QByteArray ba;
        ba.assign({"DATA"});
        QVERIFY(ba.isDetached());
        QCOMPARE(ba, QByteArray("DATA"));

        auto baCopy = ba;
        QVERIFY(!ba.isDetached());
        QVERIFY(!baCopy.isDetached());
        QVERIFY(ba.isSharedWith(baCopy));
        QVERIFY(baCopy.isSharedWith(ba));

        ba.assign(10, 'D');
        QVERIFY(ba.isDetached());
        QVERIFY(baCopy.isDetached());
        QVERIFY(!ba.isSharedWith(baCopy));
        QVERIFY(!baCopy.isSharedWith(ba));
        QCOMPARE(ba, QByteArray("DDDDDDDDDD"));
        QCOMPARE(baCopy, QByteArray("DATA"));
    }
    {
        QByteArray ba("START");
        QByteArrayView bav("DATA");
        QVERIFY(ba.isDetached());
        QCOMPARE(ba, QByteArray("START"));

        auto copyForwardIt = ba;
        QVERIFY(!ba.isDetached());
        QVERIFY(!copyForwardIt.isDetached());
        QVERIFY(ba.isSharedWith(copyForwardIt));
        QVERIFY(copyForwardIt.isSharedWith(ba));

        ba.assign(bav.begin(), bav.end());
        QVERIFY(ba.isDetached());
        QVERIFY(copyForwardIt.isDetached());
        QVERIFY(!ba.isSharedWith(copyForwardIt));
        QVERIFY(!copyForwardIt.isSharedWith(ba));
        QCOMPARE(ba, QByteArray("DATA"));
        QCOMPARE(copyForwardIt, QByteArray("START"));

        auto copyInputIt = ba;
        QVERIFY(!ba.isDetached());
        QVERIFY(!copyInputIt.isDetached());
        QVERIFY(ba.isSharedWith(copyInputIt));
        QVERIFY(copyInputIt.isSharedWith(ba));

        std::stringstream ss("1 2 3 4 5 6 ");
        ba.assign(std::istream_iterator<char>{ss}, std::istream_iterator<char>{});
        QVERIFY(ba.isDetached());
        QVERIFY(copyInputIt.isDetached());
        QVERIFY(!ba.isSharedWith(copyInputIt));
        QVERIFY(!copyInputIt.isSharedWith(ba));
        QCOMPARE(ba, QByteArray("123456"));
        QCOMPARE(copyInputIt, QByteArray("DATA"));
    }
}

void tst_QByteArray::assignUsesPrependBuffer()
{
    const auto capBegin = [](const QByteArray &ba) {
        return ba.begin() - ba.d.freeSpaceAtBegin();
    };
    const auto capEnd = [](const QByteArray &ba) {
        return ba.end() + ba.d.freeSpaceAtEnd();
    };
    // QByteArray &assign(QByteArrayView)
    {
        QByteArray withFreeSpaceAtBegin;
        for (int i = 0; i < 100 && withFreeSpaceAtBegin.d.freeSpaceAtBegin() < 2; ++i)
            withFreeSpaceAtBegin.prepend("data");
        QCOMPARE_GT(withFreeSpaceAtBegin.d.freeSpaceAtBegin(), 1);

        const auto oldCapBegin = capBegin(withFreeSpaceAtBegin);
        const auto oldCapEnd = capEnd(withFreeSpaceAtBegin);

        std::string test(withFreeSpaceAtBegin.d.freeSpaceAtBegin(), 'd');
        withFreeSpaceAtBegin.assign(test);

        QCOMPARE_EQ(withFreeSpaceAtBegin.d.freeSpaceAtBegin(), 0); // we used the prepend buffer
        QCOMPARE_EQ(capBegin(withFreeSpaceAtBegin), oldCapBegin);
        QCOMPARE_EQ(capEnd(withFreeSpaceAtBegin), oldCapEnd);
        QCOMPARE(withFreeSpaceAtBegin, test.data());
    }
    // QByteArray &assign(InputIterator, InputIterator)
    {
        QByteArray withFreeSpaceAtBegin;
        for (int i = 0; i < 100 && withFreeSpaceAtBegin.d.freeSpaceAtBegin() < 2; ++i)
            withFreeSpaceAtBegin.prepend("data");
        QCOMPARE_GT(withFreeSpaceAtBegin.d.freeSpaceAtBegin(), 1);

        const auto oldCapBegin = capBegin(withFreeSpaceAtBegin);
        const auto oldCapEnd = capEnd(withFreeSpaceAtBegin);

        std::stringstream ss;
        for (qsizetype i = 0; i < withFreeSpaceAtBegin.d.freeSpaceAtBegin(); ++i)
            ss << "d ";

        withFreeSpaceAtBegin.assign(std::istream_iterator<char>{ss}, std::istream_iterator<char>{});
        QCOMPARE_EQ(withFreeSpaceAtBegin.d.freeSpaceAtBegin(), 0); // we used the prepend buffer
        QCOMPARE_EQ(capBegin(withFreeSpaceAtBegin), oldCapBegin);
        QCOMPARE_EQ(capEnd(withFreeSpaceAtBegin), oldCapEnd);
    }
}

void tst_QByteArray::insert()
{
    const char data[] = "data";

    QCOMPARE(QByteArray().insert(0, QByteArray()), QByteArray());
    QCOMPARE(QByteArray().insert(0, 'a'), QByteArray("a"));
    QCOMPARE(QByteArray().insert(0, 2, 'a'), QByteArray("aa"));
    QCOMPARE(QByteArray().insert(0, QByteArray("data")), QByteArray("data"));
    QCOMPARE(QByteArray().insert(0, data), QByteArray("data"));
    QCOMPARE(QByteArray().insert(0, data, 2), QByteArray("da"));
    QCOMPARE(QByteArray().insert(0, QByteArrayView(data)), QByteArray("data"));

    // insert into empty with offset
    QCOMPARE(QByteArray().insert(2, QByteArray()), QByteArray());
    QCOMPARE(QByteArray().insert(2, 'a'), QByteArray("  a"));
    QCOMPARE(QByteArray().insert(2, 2, 'a'), QByteArray("  aa"));
    QCOMPARE(QByteArray().insert(2, QByteArray("data")), QByteArray("  data"));
    QCOMPARE(QByteArray().insert(2, data), QByteArray("  data"));
    QCOMPARE(QByteArray().insert(2, data, 2), QByteArray("  da"));
    QCOMPARE(QByteArray().insert(2, QByteArrayView(data)), QByteArray("  data"));

    QByteArray ba("Meal");
    QCOMPARE(ba.insert(1, QByteArray("ontr")), QByteArray("Montreal"));
    QCOMPARE(ba.insert(ba.size(), "foo"), QByteArray("Montrealfoo"));

    ba = QByteArray("13");
    QCOMPARE(ba.insert(1, QByteArray("2")), QByteArray("123"));

    ba = "ac";
    QCOMPARE(ba.insert(1, 'b'), QByteArray("abc"));
    QCOMPARE(ba.size(), 3);

    ba = "ac";
    QCOMPARE(ba.insert(-1, 3, 'x'), QByteArray("ac"));
    QCOMPARE(ba.insert(1, 3, 'x'), QByteArray("axxxc"));
    QCOMPARE(ba.insert(6, 3, 'x'), QByteArray("axxxc xxx"));
    QCOMPARE(ba.size(), 9);

    ba = "ikl";
    QCOMPARE(ba.insert(1, "j"), QByteArray("ijkl"));
    QCOMPARE(ba.size(), 4);

    ba = "ab";
    QCOMPARE(ba.insert(1, "\0X\0", 3), QByteArray::fromRawData("a\0X\0b", 5));
    QCOMPARE(ba.size(), 5);

    ba = "Hello World";
    QCOMPARE(ba.insert(5, QByteArrayView(",")), QByteArray("Hello, World"));
    QCOMPARE(ba.size(), 12);

    ba = "one";
    QCOMPARE(ba.insert(1, ba), QByteArray("oonene"));
    QCOMPARE(ba.size(), 6);

    ba = "one";
    QCOMPARE(ba.insert(1, QByteArrayView(ba)), QByteArray("oonene"));
    QCOMPARE(ba.size(), 6);

    {
        ba = "one";
        ba.prepend('a');
        QByteArray b(ba.data_ptr()->freeSpaceAtEnd(), 'b');
        QCOMPARE(ba.insert(ba.size() + 1, QByteArrayView(b)), QByteArray("aone ") + b);
    }

    {
        ba = "onetwothree";
        while (ba.size() - 1)
            ba.remove(0, 1);
        QByteArray b(ba.data_ptr()->freeSpaceAtEnd() + 1, 'b');
        QCOMPARE(ba.insert(ba.size() + 1, QByteArrayView(b)), QByteArray("e ") + b);
    }

    {
        ba = "one";
        ba.prepend('a');
        const qsizetype freeAtEnd = ba.data_ptr()->freeSpaceAtEnd();
        QCOMPARE(ba.insert(ba.size() + 1, freeAtEnd + 1, 'b'),
                 QByteArray("aone ") + QByteArray(freeAtEnd + 1, 'b'));
    }

    {
        ba = "onetwothree";
        while (ba.size() - 1)
            ba.remove(0, 1);
        const qsizetype freeAtEnd = ba.data_ptr()->freeSpaceAtEnd();
        QCOMPARE(ba.insert(ba.size() + 1, freeAtEnd + 1, 'b'),
                 QByteArray("e ") + QByteArray(freeAtEnd + 1, 'b'));
    }
}

void tst_QByteArray::insertExtended_data()
{
    prependExtended_data();
}

void tst_QByteArray::insertExtended()
{
    QFETCH(QByteArray, array);
    QCOMPARE(array.insert(1, "i"), QByteArray("diata"));
    QCOMPARE(array.insert(1, 3, 'x'), QByteArray("dxxxiata"));
    QCOMPARE(array.size(), 8);
}

void tst_QByteArray::remove_data()
{
    QTest::addColumn<QByteArray>("src");
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("length");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("null 0 0") << QByteArray() << 0 << 0 << QByteArray();
    QTest::newRow("null 0 5") << QByteArray() << 0 << 5 << QByteArray();
    QTest::newRow("null 3 5") << QByteArray() << 3 << 5 << QByteArray();
    QTest::newRow("null -1 5") << QByteArray() << -1 << 5 << QByteArray();

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
    // Test when it's shared
    QByteArray ba1 = src;
    QCOMPARE(ba1.remove(position, length), expected);

    // Test when it's not shared
    QByteArray ba2 = src;
    ba2.detach();
    QCOMPARE(ba2.remove(position, length), expected);
}

void tst_QByteArray::remove_extra()
{
    QByteArray ba = "Clock";
    ba.removeFirst();
    QCOMPARE(ba, "lock");
    ba.removeLast();
    QCOMPARE(ba, "loc");
    ba.removeAt(ba.indexOf('o'));
    QCOMPARE(ba, "lc");
    ba.clear();
    // No crash on empty byte arrays
    ba.removeFirst();
    ba.removeLast();
    ba.removeAt(2);
}

void tst_QByteArray::removeIf()
{
    auto removeA = [](const char c) { return c == 'a' || c == 'A'; };

    QByteArray a;
    QCOMPARE(a.removeIf(removeA), QByteArray());
    QVERIFY(!a.isDetached());

    a = QByteArray("aBcAbC");
    // Test when it's not shared
    QVERIFY(a.isDetached());
    QCOMPARE(a.removeIf(removeA), QByteArray("BcbC"));

    a = QByteArray("aBcAbC");
    QByteArray b = a;
    // Test when it's shared
    QVERIFY(!b.isDetached());
    QCOMPARE(b.removeIf(removeA), QByteArray("BcbC"));
}

void tst_QByteArray::erase()
{
    {
        QByteArray ba = "kittens";
        auto it = ba.erase(ba.cbegin(), ba.cbegin() + 2);
        QCOMPARE(ba, "ttens");
        QCOMPARE(it, ba.cbegin());
    }

    {
        QByteArray ba = "kittens";
        auto it = ba.erase(ba.cbegin(), ba.cend());
        QCOMPARE(ba, "");
        QCOMPARE(it, ba.cbegin());
        QCOMPARE(ba.cbegin(), ba.cend());
    }

    {
        QByteArray ba = "kite";
        auto it = ba.erase(ba.cbegin(), ba.cbegin());
        // erase() should return an iterator (not const_iterator)
        *it = 'Z';
        QCOMPARE(ba, "Zite");
        QCOMPARE(it, ba.cbegin());
    }
}

void tst_QByteArray::erase_single_arg()
{
    QByteArray ba = "abcdefg";
    ba.erase(ba.cend());
    auto it = ba.erase(ba.cbegin());
    QCOMPARE_EQ(ba, "bcdefg");
    QCOMPARE(it, ba.cbegin());

    it = ba.erase(std::prev(ba.end()));
    QCOMPARE_EQ(ba, "bcdef");
    QCOMPARE(it, ba.cend());

    it = ba.erase(std::find(ba.begin(), ba.end(), QChar('d')));
    QCOMPARE(it, ba.begin() + 2);
}

void tst_QByteArray::replace_pos_len_data()
{
    QTest::addColumn<QByteArray>("src");
    QTest::addColumn<int>("pos");
    QTest::addColumn<int>("len");
    QTest::addColumn<QByteArray>("after");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("1") << QByteArray("Say yes!") << 4 << 3 << QByteArray("no")
                       << QByteArray("Say no!");
    QTest::newRow("2") << QByteArray("rock and roll") << 5 << 3 << QByteArray("&")
                       << QByteArray("rock & roll");
    QTest::newRow("3") << QByteArray("foo") << 3 << 0 << QByteArray("bar")
                       << QByteArray("foobar");
    QTest::newRow("4") << QByteArray() << 0 << 0 << QByteArray() << QByteArray();
    // index out of range
    QTest::newRow("5") << QByteArray() << 3 << 0 << QByteArray("hi")
                       << QByteArray();

    QTest::newRow("negative-before-len-1") << QByteArray("yyyy") << 3 << -1
                                           << QByteArray("ZZZZ") << QByteArray("yyyZZZZy");
    QTest::newRow("negative-before-len-2") << QByteArray("yyyy") << 3 << -2
                                           << QByteArray("ZZZZ") << QByteArray("yyyZZZZy");

    // Optimized path
    QTest::newRow("6") << QByteArray("abcdef") << 3 << 12
                       << QByteArray("abcdefghijkl") << QByteArray("abcabcdefghijkl");
    QTest::newRow("7") << QByteArray("abcdef") << 3 << 4
                       << QByteArray("abcdefghijkl") << QByteArray("abcabcdefghijkl");
    QTest::newRow("8") << QByteArray("abcdef") << 3 << 3
                       << QByteArray("abcdefghijkl") << QByteArray("abcabcdefghijkl");
    QTest::newRow("9") << QByteArray("abcdef") << 3 << 2
                       << QByteArray("abcdefghijkl") << QByteArray("abcabcdefghijklf");
    QTest::newRow("10") << QByteArray("abcdef") << 2 << 2 << QByteArray("xx")
                        << QByteArray("abxxef");
}

void tst_QByteArray::replace_pos_len()
{
    QFETCH(QByteArray, src);
    QFETCH(int, pos);
    QFETCH(int, len);
    QFETCH(QByteArray, after);
    QFETCH(QByteArray, expected);

    QByteArray copy = src;
    QCOMPARE(copy.replace(pos, len, after), expected);
    copy = src;
    QCOMPARE(copy.replace(pos, len, after.data(), after.size()), expected);
}

void tst_QByteArray::replace_before_after_data()
{
    QTest::addColumn<QByteArray>("src");
    QTest::addColumn<QByteArray>("before");
    QTest::addColumn<QByteArray>("after");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("null") << QByteArray() << QByteArray("abc") << QByteArray()
                          << QByteArray();

    QTest::newRow("text to text") << QByteArray("abcdefghbcd") << QByteArray("bcd")
                                  << QByteArray("1234") << QByteArray("a1234efgh1234");

    QTest::newRow("char to text") << QByteArray("abcdefgch") << QByteArray("c")
                                  << QByteArray("1234") << QByteArray("ab1234defg1234h");

    QTest::newRow("char to char") << QByteArray("abcdefgch") << QByteArray("c")
                                  << QByteArray("1") << QByteArray("ab1defg1h");
}

void tst_QByteArray::replace_before_after()
{
    QFETCH(QByteArray, src);
    QFETCH(QByteArray, before);
    QFETCH(QByteArray, after);
    QFETCH(QByteArray, expected);

    QByteArray copy = src;
    if (before.size() == 1) {
        if (after.size() == 1)
            QCOMPARE(copy.replace(before.front(), after.front()), expected);
        QCOMPARE(copy.replace(before.front(), after), expected);
    }
    copy = src;
    QCOMPARE(copy.replace(before, after), expected);
    copy = src;
    QCOMPARE(copy.replace(before.constData(), before.size(), after.constData(), after.size()), expected);
}

void tst_QByteArray::replace_after_points_into_this_data()
{
    QTest::addColumn<QByteArray>("ba");
    QTest::addColumn<int>("before_index");
    QTest::addColumn<int>("before_len");
    QTest::addColumn<int>("after_index");
    QTest::addColumn<int>("after_len");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("both-point-into-this") << "abcdefghibcdefghij"_ba
                                          << 1 << 6
                                          << 9 << 3
                                          << "abcdhibcdhij"_ba;

    QTest::newRow("before-points-into-after-too") << "abcdefghibcdefghij"_ba
                                           << 1 << 6
                                           << 1 << 5
                                           << "abcdefhibcdefhij"_ba;

    QTest::newRow("nothing-with-nothing") << "abcdefghibcdefghij"_ba
                                          << 0 << 0
                                          << 0 << 0
                                          << "abcdefghibcdefghij"_ba;

    QTest::newRow("all-null") << QByteArray{}
                              << 0 << 0
                              << 0 << 0
                              << QByteArray{};

}

void tst_QByteArray::replace_after_points_into_this()
{
    QFETCH(QByteArray, ba);
    QFETCH(int, before_index);
    QFETCH(int, before_len);
    QFETCH(int, after_index);
    QFETCH(int, after_len);
    QFETCH(QByteArray, expected);

    { // When it's shared
        QByteArray src = ba;
        auto before = QByteArrayView{src}.sliced(before_index, before_len);
        auto after = QByteArrayView{src}.sliced(after_index, after_len);
        src.replace(before, after);
        QCOMPARE(src, expected);
    }

    { // When it's detached
        QByteArray src = ba;
        src.detach();
        auto before = QByteArrayView{src}.sliced(before_index, before_len);
        auto after = QByteArrayView{src}.sliced(after_index, after_len);
        src.replace(before, after);
        QCOMPARE(src, expected);
    }
}

void tst_QByteArray::replace_view_view_data()
{
    QTest::addColumn<QByteArray>("src");
    QTest::addColumn<QByteArray>("before");
    QTest::addColumn<QByteArray>("after");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("null-src-1") << QByteArray() << QByteArray() << QByteArray() << QByteArray();
    QTest::newRow("null-src-2") << QByteArray() << ""_ba        << QByteArray() << QByteArray();
    QTest::newRow("null-src-3") << QByteArray() << ""_ba        << ""_ba << QByteArray();
    QTest::newRow("null-src-4") << QByteArray() << QByteArray() << ""_ba << QByteArray();

    QTest::newRow("empty-src-1") << ""_ba << QByteArray() << QByteArray() << ""_ba;
    QTest::newRow("empty-src-2") << ""_ba << ""_ba        << QByteArray() << ""_ba;
    QTest::newRow("empty-src-3") << ""_ba << ""_ba        << ""_ba        << ""_ba;
    QTest::newRow("empty-src-4") << ""_ba << QByteArray() << ""_ba        << ""_ba;

    QTest::newRow("null-char-1") << QByteArray() << "a"_ba        << QByteArray() << QByteArray();
    QTest::newRow("null-char-2") << QByteArray() << "a"_ba        << "b"_ba       << QByteArray();
    QTest::newRow("null-char-3") << QByteArray() << QByteArray()  << "b"_ba       << "b"_ba;

    QTest::newRow("null-str-1") << QByteArray() << "abc"_ba     << QByteArray() << QByteArray();
    QTest::newRow("null-str-2") << QByteArray() << "abc"_ba     << "gfh"_ba     << QByteArray();
    QTest::newRow("null-str-3") << QByteArray() << QByteArray() << "gfh"_ba     << "gfh"_ba;

    QTest::newRow("empty-char-1") << ""_ba << "a"_ba       << QByteArray() << ""_ba;
    QTest::newRow("empty-char-2") << ""_ba << "a"_ba       << "b"_ba       << ""_ba;
    QTest::newRow("empty-char-3") << ""_ba << QByteArray() << "b"_ba       << "b"_ba;

    QTest::newRow("empty-str-1") << ""_ba << "abc"_ba     << QByteArray() << ""_ba;
    QTest::newRow("empty-str-2") << ""_ba << "abc"_ba     << "gfh"_ba     << ""_ba;
    QTest::newRow("empty-str-3") << ""_ba << QByteArray() << "gfh"_ba     << "gfh"_ba;

    QTest::newRow("before-longer-1") << "Say yes!"_ba
                                     << "yes"_ba << "no"_ba
                                     << "Say no!"_ba;

    QTest::newRow("before-longer-2") << "rock and roll"_ba
                                     << "and"_ba << "&"_ba
                                     << "rock & roll"_ba;

    QTest::newRow("equal-length-1") << "Say yes!"_ba
                                    << "yes"_ba << "yep"_ba
                                    << "Say yep!"_ba;

    QTest::newRow("equal-length-2") << "kite mite"_ba
                                    << "te"_ba << "NN"_ba
                                    << "kiNN miNN"_ba;

    QTest::newRow("after-longer-1") << "Say yes!"_ba
                                    << "yes"_ba << "affirmative"_ba
                                    << "Say affirmative!"_ba;

    QTest::newRow("after-longer-2") << "foo"_ba
                                    << "f"_ba << "bar"_ba
                                    << "baroo"_ba;

    QTest::newRow("after-longer-3") << "the quality of mercy"_ba
                                    << "t"_ba << "XYZ"_ba
                                    << "XYZhe qualiXYZy of mercy"_ba;

    QTest::newRow("after-longer-4") << "the quality of mercy"_ba
                                    << "of"_ba << "BAR"_ba
                                    << "the quality BAR mercy"_ba;

    QTest::newRow("replace-with-nothing-1") << "the quality of mercy"_ba
                                            << "the"_ba << ""_ba
                                            << " quality of mercy"_ba;

    QTest::newRow("replace-with-nothing-2") << "the quality of mercy"_ba
                                            << "of"_ba << ""_ba
                                            << "the quality  mercy"_ba;

    QTest::newRow("one-char-with-one-char-1") << "the quality of mercy"_ba
                                              << "t"_ba << "Z"_ba
                                              << "Zhe qualiZy of mercy"_ba;

    QTest::newRow("one-char-with-one-char-2") << "the quality of mercy"_ba
                                              << "e"_ba << "R"_ba
                                              << "thR quality of mRrcy"_ba;

    QTest::newRow("nothing-with-one-char") << "quality"_ba
                                           << ""_ba << "A"_ba
                                           << "AqAuAaAlAiAtAyA"_ba;

    QTest::newRow("one-char-with-nothing") << "E"_ba
                                           << "E"_ba << ""_ba
                                           << ""_ba;
}

void tst_QByteArray::replace_view_view()
{
    QFETCH(QByteArray, src);
    QFETCH(QByteArray, before);
    QFETCH(QByteArray, after);
    QFETCH(QByteArray, expected);

    QByteArray copy = src;
    {
        copy.replace(before, after);
        QCOMPARE(copy, expected);
    }

    { // When it's detaches
        copy = src;
        copy.detach();
        copy.replace(before, after);
        QCOMPARE(copy, expected);
    }

    { // detached and doesn't need to reallocate
        if (before.size() < after.size()) {
            copy = src;
            copy.detach();
            copy.reserve(copy.size() + 30);
            copy.replace(before, after);
            QCOMPARE(copy, expected);
        }
    }
}

void tst_QByteArray::replaceWithSpecifiedLength()
{
    const char after[] = "zxc\0vbnmqwert";
    qsizetype lenAfter = 6;
    QByteArray ba("abcdefghjk");
    ba.replace(qsizetype(0), 2, after, lenAfter);

    const char _expected[] = "zxc\0vbcdefghjk";
    QByteArray expected(_expected,sizeof(_expected)-1);
    QCOMPARE(ba,expected);
}

void tst_QByteArray::replaceWithEmptyNeedleInsertsBeforeEachChar_data()
{
    QTest::addColumn<QByteArray>("haystack");
    QTest::addColumn<QByteArray>("needle");
    QTest::addColumn<QByteArray>("replacement");
    QTest::addColumn<QByteArray>("result");

    const QByteArray null;
    const QByteArray empty = "";
    const QByteArray a = "a";
    const QByteArray aa = "aa";
    const QByteArray b = "b";
    const QByteArray bb = "bb";
    const QByteArray bab = "bab";
    const QByteArray babab = "babab";

    auto row = [](const QByteArray &haystack, const QByteArray &needle,
                  const QByteArray &replacement, const QByteArray &result)
    {
        auto protect = [](const QByteArray &ba) { return ba.isNull() ? "<null>" : ba.data(); };
        QTest::addRow("/%s/%s/%s/", protect(haystack), protect(needle), protect(replacement))
                << haystack << needle << replacement << result;
    };
    row(null,  null,  a, a);
    row(null,  empty, a, a);
    row(null,  a,     a, null);
    row(null,  a,     b, null);
    row(null,  aa,    b, null);

    row(empty, null,  a, a);
    row(empty, empty, a, a);
    row(empty, a,     a, empty);
    row(empty, aa,    b, empty);

    row(a,     null,  b, bab);
    row(a,     empty, b, bab);
    row(a,     a,     b, b);
    row(a,     aa,    b, a);

    row(aa,    null,  b, babab);
    row(aa,    empty, b, babab);
    row(aa,    a,     b, bb);
    row(aa,    aa,    b, b);
}

void tst_QByteArray::replaceWithEmptyNeedleInsertsBeforeEachChar()
{
    QFETCH(const QByteArray, haystack);
    QFETCH(const QByteArray, needle);
    QFETCH(const QByteArray, replacement);
    QFETCH(const QByteArray, result);

    const auto check = [](auto haystack, auto needle, auto replacement, auto result) {
        {
            // shared
            auto copy = haystack;
            copy.replace(needle, replacement);
            QCOMPARE(copy.isNull(), result.isNull());
            QCOMPARE(copy, result);
        }
        {
            // unshared
            auto copy = haystack;
            copy.detach();
            copy.replace(needle, replacement);
            // isNull() check pointless, as copy is never isNull() after detach()
            QCOMPARE(copy, result);
        }
    };

    check(haystack, needle, replacement, result);
    if (QTest::currentTestFailed())
        return;

    {
        // compared with QString::replace()
        const auto h = QString(haystack);
        QCOMPARE(h.isNull(), haystack.isNull());
        const auto n = QString(needle);
        QCOMPARE(n.isNull(), needle.isNull());
        const auto rep = QString(replacement);
        QCOMPARE(rep.isNull(), replacement.isNull());
        const auto res = QString(result);
        QCOMPARE(res.isNull(), result.isNull());

        check(h, n, rep, res);
        if (QTest::currentTestFailed())
            return;
    }

    {
        // compared with QStringTokenizer
        QByteArray alt;
        for (auto part : qTokenize(QLatin1StringView{haystack}, QLatin1StringView{needle})) {
            alt += QByteArrayView{part};
            alt += replacement;
        }
        if (!alt.isEmpty())
            alt.chop(replacement.size()); // this destroys null'ness
        else
            QCOMPARE(alt.isNull(), result.isNull()); // so this only makes sense if we didn't chop
        QCOMPARE(alt, result);
    }
}

void tst_QByteArray::replaceDoesNotReplaceTheTerminatingNull()
{
    // Try really hard to replace the implicit terminating '\0' byte:
    constexpr char content[] = "Hello, World!";
#define CHECK(...) do { \
        QByteArray ba(content); \
        QCOMPARE(std::as_const(ba).data()[ba.size()], '\0'); \
        ba.replace(__VA_ARGS__); \
        QCOMPARE(ba, content); \
        QCOMPARE(std::as_const(ba).data()[ba.size()], '\0'); \
    } while (false)
    CHECK('\0', 'a');
    CHECK(QByteArrayView{"!", 2}, // including \0, matches end of `ba`
          QByteArrayView{"!!"});
#undef CHECK
}

void tst_QByteArray::number()
{
    QCOMPARE(QByteArray::number(quint64(0)), QByteArray("0"));
    QCOMPARE(QByteArray::number(Q_UINT64_C(0xFFFFFFFFFFFFFFFF)),
             QByteArray("18446744073709551615"));
    QCOMPARE(QByteArray::number(Q_INT64_C(0xFFFFFFFFFFFFFFFF)), QByteArray("-1"));
    QCOMPARE(QByteArray::number(qint64(0)), QByteArray("0"));
    QCOMPARE(QByteArray::number(Q_INT64_C(0x7FFFFFFFFFFFFFFF)),
             QByteArray("9223372036854775807"));
    QCOMPARE(QByteArray::number(Q_INT64_C(0x8000000000000000)),
             QByteArray("-9223372036854775808"));
}

void tst_QByteArray::number_double_data()
{
    QTest::addColumn<double>("value");
    QTest::addColumn<char>("format");
    QTest::addColumn<int>("precision");
    QTest::addColumn<QByteArray>("expected");

    // This function is implemented in ../shared/test_number_shared.h
    add_number_double_shared_data([](NumberDoubleTestData datum) {
        QByteArray ba(datum.expected.data(), datum.expected.size());
        const char *title = !datum.optTitle.isEmpty() ? datum.optTitle.data() : ba.data();
        QTest::addRow("%s, format '%c', precision %d", title, datum.f, datum.p)
                << datum.d << datum.f << datum.p << ba;
        if (datum.f != 'f') { // Also test uppercase format
            datum.f = QtMiscUtils::toAsciiUpper(datum.f);
            QByteArray upper = ba.toUpper();
            QByteArray upperTitle = QByteArray(title);
            if (!datum.optTitle.isEmpty())
                upperTitle += ", uppercase";
            else
                upperTitle = upperTitle.toUpper();
            QTest::addRow("%s, format '%c', precision %d", upperTitle.data(), datum.f, datum.p)
                    << datum.d << datum.f << datum.p << upper;
        }
    });
}

void tst_QByteArray::number_double()
{
    QFETCH(double, value);
    QFETCH(char, format);
    QFETCH(int, precision);

    QT_IGNORE_DEPRECATIONS(constexpr bool has_denorm = std::numeric_limits<double>::has_denorm != std::denorm_present;)
    if constexpr (has_denorm) {
        if (::qstrcmp(QTest::currentDataTag(), "Very small number, very high precision, format 'f', precision 350") == 0) {
            QSKIP("Skipping 'denorm' as this type lacks denormals on this system");
        }
    }
    QTEST(QByteArray::number(value, format, precision), "expected");
}

void tst_QByteArray::number_base_data()
{
    QTest::addColumn<qlonglong>("n");
    QTest::addColumn<int>("base");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("base 10") << 12346LL << 10 << QByteArray("12346");
    QTest::newRow("base  2") << 12346LL <<  2 << QByteArray("11000000111010");
    QTest::newRow("base  8") << 12346LL <<  8 << QByteArray("30072");
    QTest::newRow("base 16") << 12346LL << 16 << QByteArray("303a");
    QTest::newRow("base 17") << 12346LL << 17 << QByteArray("28c4");
    QTest::newRow("base 36") << 2181789482LL << 36 << QByteArray("102zbje");

    QTest::newRow("largeint, base 10")
            << 123456789012LL << 10 << QByteArray("123456789012");
    QTest::newRow("largeint, base  2")
            << 123456789012LL <<  2 << QByteArray("1110010111110100110010001101000010100");
    QTest::newRow("largeint, base  8")
            << 123456789012LL <<  8 << QByteArray("1627646215024");
    QTest::newRow("largeint, base 16")
            << 123456789012LL << 16 << QByteArray("1cbe991a14");
    QTest::newRow("largeint, base 17")
            << 123456789012LL << 17 << QByteArray("10bec2b629");
}

void tst_QByteArray::number_base()
{
    QFETCH( qlonglong, n );
    QFETCH( int, base );
    QFETCH( QByteArray, expected );
    QCOMPARE(QByteArray::number(n, base), expected);
    QCOMPARE(QByteArray::number(-n, base), '-' + expected);

    // check qlonglong->QByteArray->qlonglong round trip
    for (int ibase = 2; ibase <= 36; ++ibase) {
        auto stringrep = QByteArray::number(n, ibase);
        QCOMPARE(QByteArray::number(-n, ibase), '-' + stringrep);
        bool ok(false);
        auto result = stringrep.toLongLong(&ok, ibase);
        QVERIFY(ok);
        QCOMPARE(n, result);
    }
    if (n <= std::numeric_limits<int>::max()) {
        QCOMPARE(QByteArray::number(int(n), base), expected);
        QCOMPARE(QByteArray::number(int(-n), base), '-' + expected);
    } else if (n <= std::numeric_limits<long>::max()) {
        QCOMPARE(QByteArray::number(long(n), base), expected);
        QCOMPARE(QByteArray::number(long(-n), base), '-' + expected);
    }
}

void tst_QByteArray::nullness()
{
    {
        QByteArray ba;
        QVERIFY(ba.isNull());
    }
    {
        QByteArray ba = nullptr;
        QVERIFY(ba.isNull());
    }
    {
        const char *ptr = nullptr;
        QByteArray ba = ptr;
        QVERIFY(ba.isNull());
    }
    {
        QByteArray ba(nullptr, 0);
        QVERIFY(ba.isNull());
    }
    {
        const char *ptr = nullptr;
        QByteArray ba(ptr, 0);
        QVERIFY(ba.isNull());
    }
    {
        QByteArrayView bav;
        QVERIFY(bav.isNull());
        QByteArray ba = bav.toByteArray();
        QVERIFY(ba.isNull());
    }
}

static bool checkSize(qsizetype value, qsizetype min)
{
    return value >= min && value <= std::numeric_limits<qsizetype>::max();
}

// global functions defined in qbytearray.cpp
void tst_QByteArray::blockSizeCalculations()
{
    qsizetype MaxAllocSize = std::numeric_limits<qsizetype>::max();

    // Not very important, but please behave :-)
    QCOMPARE(qCalculateBlockSize(0, 1), qsizetype(0));
    QVERIFY(qCalculateGrowingBlockSize(0, 1).size <= MaxAllocSize);
    QVERIFY(qCalculateGrowingBlockSize(0, 1).elementCount <= MaxAllocSize);

    // boundary condition
    QCOMPARE(qCalculateBlockSize(MaxAllocSize, 1), qsizetype(MaxAllocSize));
    QCOMPARE(qCalculateBlockSize(MaxAllocSize/2, 2), qsizetype(MaxAllocSize) - 1);
    QCOMPARE(qCalculateBlockSize(MaxAllocSize/2, 2, 1), qsizetype(MaxAllocSize));
    QCOMPARE(qCalculateGrowingBlockSize(MaxAllocSize, 1).size, qsizetype(MaxAllocSize));
    QCOMPARE(qCalculateGrowingBlockSize(MaxAllocSize, 1).elementCount, qsizetype(MaxAllocSize));
    QCOMPARE(qCalculateGrowingBlockSize(MaxAllocSize/2, 2, 1).size, qsizetype(MaxAllocSize));
    QCOMPARE(qCalculateGrowingBlockSize(MaxAllocSize/2, 2, 1).elementCount, qsizetype(MaxAllocSize)/2);

    // error conditions
    QCOMPARE(qCalculateBlockSize(quint64(MaxAllocSize) + 1, 1), qsizetype(-1));
    QCOMPARE(qCalculateBlockSize(qsizetype(-1), 1), qsizetype(-1));
    QCOMPARE(qCalculateBlockSize(MaxAllocSize, 1, 1), qsizetype(-1));
    QCOMPARE(qCalculateBlockSize(MaxAllocSize/2 + 1, 2), qsizetype(-1));
    QCOMPARE(qCalculateGrowingBlockSize(quint64(MaxAllocSize) + 1, 1).size, qsizetype(-1));
    QCOMPARE(qCalculateGrowingBlockSize(MaxAllocSize/2 + 1, 2).size, qsizetype(-1));

    // overflow conditions
#if QT_POINTER_SIZE == 4
    // on 32-bit platforms, (1 << 16) * (1 << 16) = (1 << 32) which is zero
    QCOMPARE(qCalculateBlockSize(1 << 16, 1 << 16), qsizetype(-1));
    QCOMPARE(qCalculateBlockSize(MaxAllocSize/4, 16), qsizetype(-1));
    // on 32-bit platforms, (1 << 30) * 3 + (1 << 30) would overflow to zero
    QCOMPARE(qCalculateBlockSize(1U << 30, 3, 1U << 30), qsizetype(-1));
#else
    // on 64-bit platforms, (1 << 32) * (1 << 32) = (1 << 64) which is zero
    QCOMPARE(qCalculateBlockSize(1LL << 32, 1LL << 32), qsizetype(-1));
    QCOMPARE(qCalculateBlockSize(MaxAllocSize/4, 16), qsizetype(-1));
    // on 64-bit platforms, (1 << 30) * 3 + (1 << 30) would overflow to zero
    QCOMPARE(qCalculateBlockSize(1ULL << 62, 3, 1ULL << 62), qsizetype(-1));
#endif
    // exact block sizes
    for (int i = 1; i < 1 << 31; i <<= 1) {
        QCOMPARE(qCalculateBlockSize(0, 1, i), qsizetype(i));
        QCOMPARE(qCalculateBlockSize(i, 1), qsizetype(i));
        QCOMPARE(qCalculateBlockSize(i + i/2, 1), qsizetype(i + i/2));
    }
    for (int i = 1; i < 1 << 30; i <<= 1) {
        QCOMPARE(qCalculateBlockSize(i, 2), 2 * qsizetype(i));
        QCOMPARE(qCalculateBlockSize(i, 2, 1), 2 * qsizetype(i) + 1);
        QCOMPARE(qCalculateBlockSize(i, 2, 16), 2 * qsizetype(i) + 16);
    }

    // growing sizes
    for (int i = 1; i < 1 << 31; i <<= 1) {
        QVERIFY(checkSize(qCalculateGrowingBlockSize(i, 1).size, i));
        QVERIFY(checkSize(qCalculateGrowingBlockSize(i, 1).elementCount, i));
        QVERIFY(checkSize(qCalculateGrowingBlockSize(i, 1, 16).size, i));
        QVERIFY(checkSize(qCalculateGrowingBlockSize(i, 1, 16).elementCount, i));
        QVERIFY(checkSize(qCalculateGrowingBlockSize(i, 1, 24).size, i));
        QVERIFY(checkSize(qCalculateGrowingBlockSize(i, 1, 16).elementCount, i));
    }

    // growth should be limited
    for (int elementSize = 1; elementSize < (1<<8); elementSize <<= 1) {
        qsizetype alloc = 1;
        forever {
            QVERIFY(checkSize(qCalculateGrowingBlockSize(alloc, elementSize).size, alloc * elementSize));
            qsizetype newAlloc = qCalculateGrowingBlockSize(alloc, elementSize).elementCount;
            QVERIFY(checkSize(newAlloc, alloc));
            if (newAlloc == alloc)
                break;  // no growth, we're at limit
            alloc = newAlloc;
        }
        QVERIFY(checkSize(alloc, qsizetype(MaxAllocSize) / elementSize));

        // the next allocation should be invalid
        if (alloc < MaxAllocSize) // lest alloc + 1 overflows (= UB)
            QCOMPARE(qCalculateGrowingBlockSize(alloc + 1, elementSize).size, qsizetype(-1));
    }
}

void tst_QByteArray::resizeAfterFromRawData()
{
    QByteArray buffer("hello world");

    QByteArray array = QByteArray::fromRawData(buffer.constData(), buffer.size());
    QVERIFY(array.constData() == buffer.constData());
    array.resize(5);
    QVERIFY(array.constData() != buffer.constData());
    // check null termination
    QVERIFY(array.constData()[5] == 0);
}

void tst_QByteArray::toFromHex_data()
{
    QTest::addColumn<QByteArray>("str");
    QTest::addColumn<char>("sep");
    QTest::addColumn<QByteArray>("hex");
    QTest::addColumn<QByteArray>("hex_alt1");

    QTest::newRow("Qt is great! (default)")
        << QByteArray("Qt is great!")
        << '\0'
        << QByteArray("517420697320677265617421")
        << QByteArray("51 74 20 69 73 20 67 72 65 61 74 21");

    QTest::newRow("Qt is great! (with space)")
        << QByteArray("Qt is great!")
        << ' '
        << QByteArray("51 74 20 69 73 20 67 72 65 61 74 21")
        << QByteArray("51 74 20 69 73 20 67 72 65 61 74 21");

    QTest::newRow("Qt is great! (with minus)")
        << QByteArray("Qt is great!")
        << '-'
        << QByteArray("51-74-20-69-73-20-67-72-65-61-74-21")
        << QByteArray("51-74-20-69-73-20-67-72-65-61-74-21");

    QTest::newRow("Qt is so great!")
        << QByteArray("Qt is so great!")
        << '\0'
        << QByteArray("517420697320736f20677265617421")
        << QByteArray("51 74 20 69 73 20 73 6f 20 67 72 65 61 74 21");

    QTest::newRow("default-constructed")
        << QByteArray()
        << '\0'
        << QByteArray()
        << QByteArray();

    QTest::newRow("default-constructed (with space)")
        << QByteArray()
        << ' '
        << QByteArray()
        << QByteArray();

    QTest::newRow("empty")
        << QByteArray("")
        << '\0'
        << QByteArray("")
        << QByteArray("");

    QTest::newRow("null")
        << QByteArray()
        << '\0'
        << QByteArray()
        << QByteArray();

    QTest::newRow("empty (with space)")
        << QByteArray("")
        << ' '
        << QByteArray("")
        << QByteArray("");

    QTest::newRow("array-of-null")
        << QByteArray("\0", 1)
        << '\0'
        << QByteArray("00")
        << QByteArray("0");

    QTest::newRow("no-leading-zero")
        << QByteArray("\xf")
        << '\0'
        << QByteArray("0f")
        << QByteArray("f");

    QTest::newRow("single-byte")
        << QByteArray("\xaf")
        << '\0'
        << QByteArray("af")
        << QByteArray("xaf");

    QTest::newRow("no-leading-zero-long")
        << QByteArray("\xd\xde\xad\xc0\xde")
        << '\0'
        << QByteArray("0ddeadc0de")
        << QByteArray("ddeadc0de");

    QTest::newRow("garbage")
        << QByteArray("\xC\xde\xeC\xea\xee\xDe\xee\xee")
        << '\0'
        << QByteArray("0cdeeceaeedeeeee")
        << QByteArray("Code less. Create more. Deploy everywhere.");

    QTest::newRow("under-defined-1")
        << QByteArray("\x1\x23")
        << '\0'
        << QByteArray("0123")
        << QByteArray("x123");

    QTest::newRow("under-defined-2")
        << QByteArray("\x12\x34")
        << '\0'
        << QByteArray("1234")
        << QByteArray("x1234");
}

void tst_QByteArray::toFromHex()
{
    QFETCH(QByteArray, str);
    QFETCH(char,       sep);
    QFETCH(QByteArray, hex);
    QFETCH(QByteArray, hex_alt1);

    if (sep == 0) {
        const QByteArray th = str.toHex();
        QCOMPARE(th.size(), hex.size());
        QCOMPARE(th, hex);
    }

    {
        const QByteArray th = str.toHex(sep);
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

    QCOMPARE(QByteArray().toPercentEncoding(), QByteArray());
    QCOMPARE(QByteArray("").toPercentEncoding(), QByteArray(""));

    QByteArray data = arr.toPercentEncoding();
    QCOMPARE(data, QByteArray("Qt%20is%20great%21"));
    QCOMPARE(data.percentDecoded(), arr);

    data = arr.toPercentEncoding("! ", "Qt");
    QCOMPARE(data, QByteArray("%51%74 is grea%74!"));
    QCOMPARE(data.percentDecoded(), arr);

    data = arr.toPercentEncoding(QByteArray(), "abcdefghijklmnopqrstuvwxyz", 'Q');
    QCOMPARE(data, QByteArray("Q51Q74Q20Q69Q73Q20Q67Q72Q65Q61Q74Q21"));
    QCOMPARE(data.percentDecoded('Q'), arr);

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
    QVERIFY(QByteArray().percentDecoded().isEmpty());
    QVERIFY(QByteArray().percentDecoded().isNull());

    // Verify that literal % in the string to be encoded does round-trip:
    arr = "Qt%20is%20great%21";
    data = arr.toPercentEncoding();
    QCOMPARE(data.percentDecoded(), arr);
    arr = "87% of all statistics are made up!";
    data = arr.toPercentEncoding();
    QCOMPARE(data.percentDecoded(), arr);
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

    QCOMPARE(encodedString.percentDecoded(), decodedString);
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

void tst_QByteArray::pecentEncodingRoundTrip_data()
{
    QTest::addColumn<QByteArray>("original");
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<QByteArray>("excludeInEncoding");
    QTest::addColumn<QByteArray>("includeInEncoding");

    QTest::newRow("unchanged")
        << QByteArray("abcdevghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012345678-._~")
        << QByteArray("abcdevghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012345678-._~")
        << QByteArray("") << QByteArray("");
    QTest::newRow("enclosed-space-quote")
        << QByteArray("{\t\n\r^\"abc}")
        << QByteArray("%7B%09%0A%0D%5E%22abc%7D")
        << QByteArray("") << QByteArray("");
    QTest::newRow("punctuate")
        << QByteArray("://?#[]@!$&'()*+,;=")
        << QByteArray("%3A%2F%2F%3F%23%5B%5D%40%21%24%26%27%28%29%2A%2B%2C%3B%3D")
        << QByteArray("") << QByteArray("");
    QTest::newRow("punctuate-exclude")
        << QByteArray("://?#[]@!$&'()*+,;=")
        << QByteArray("%3A%2F%2F%3F%23%5B%5D%40!$&'()*+,;=")
        << QByteArray("!$&'()*+,;=") << QByteArray("");
    QTest::newRow("text-include")
        << QByteArray("abcd") << QByteArray("a%62%63d") << QByteArray("") << QByteArray("bc");
}

void tst_QByteArray::pecentEncodingRoundTrip()
{
    QFETCH(QByteArray, original);
    QFETCH(QByteArray, encoded);
    QFETCH(QByteArray, excludeInEncoding);
    QFETCH(QByteArray, includeInEncoding);

    QByteArray encodedData = original.toPercentEncoding(excludeInEncoding, includeInEncoding);
    QCOMPARE(encodedData, encoded);
    QCOMPARE(encodedData.percentDecoded(), original);
}

struct StringComparisonData
{
    const char *const left;
    const char *const right;
    const unsigned int clip;
    const int cmp, icmp, ncmp, nicmp;
    static int sign(int val) { return val < 0 ? -1 : val > 0 ? +1 : 0; }
};
Q_DECLARE_METATYPE(StringComparisonData);

void tst_QByteArray::qstrcmp_data()
{
    QTest::addColumn<StringComparisonData>("data");

    QTest::newRow("equal")
        << StringComparisonData{"abcEdb", "abcEdb", 3, 0, 0, 0, 0};
    QTest::newRow("upper")
        << StringComparisonData{"ABCedb", "ABCEDB", 3, 1, 0, 0, 0};
    QTest::newRow("lower")
        << StringComparisonData{"ABCEDB", "abcedb", 3, -1, 0, -1, 0};
    QTest::newRow("upper-late")
        << StringComparisonData{"abcEdb", "abcEDB", 3, 1, 0, 0, 0};
    QTest::newRow("lower-late")
        << StringComparisonData{"ABCEDB", "ABCedb", 3, -1, 0, 0, 0};
    QTest::newRow("longer")
        << StringComparisonData{"abcdef", "abcdefg", 6, -1, -1, 0, 0};
    QTest::newRow("long-up")
        << StringComparisonData{"abcdef", "abcdeFg", 6, 1, -1, 1, 0};
    QTest::newRow("long-down")
        << StringComparisonData{"abcdeF", "abcdefg", 6, -1, -1, -1, 0};
    QTest::newRow("shorter")
        << StringComparisonData{"abcdefg", "abcdef", 6, 1, 1, 0, 0};
    QTest::newRow("short-up")
        << StringComparisonData{"abcdefg", "abcdeF", 6, 1, 1, 1, 0};
    QTest::newRow("short-down")
        << StringComparisonData{"abcdeFg", "abcdef", 6, -1, 1, -1, 0};
    QTest::newRow("zero-length")
        << StringComparisonData{"abcdefg", "T", 0, 1, -1, 0, 0};
    QTest::newRow("null-null")
        << StringComparisonData{nullptr, nullptr, 6, 0, 0, 0, 0};
    QTest::newRow("null-empty")
        << StringComparisonData{nullptr, "", 0, -1, -1, -1, -1};
    QTest::newRow("empty-null")
        << StringComparisonData{"", nullptr, 0, 1, 1, 1, 1};
    QTest::newRow("empty-empty")
        << StringComparisonData{"", "", 0, 0, 0, 0, 0};
    QTest::newRow("null-some")
        << StringComparisonData{nullptr, "some", 0, -1, -1, -1, -1};
    QTest::newRow("some-null")
        << StringComparisonData{"some", nullptr, 0, 1, 1, 1, 1};
    QTest::newRow("empty-some")
        << StringComparisonData{"", "some", 0, -1, -1, 0, 0};
    QTest::newRow("some-empty")
        << StringComparisonData{"some", "", 0, 1, 1, 0, 0};
}

void tst_QByteArray::qstrcmp()
{
    QFETCH(StringComparisonData, data);
    QCOMPARE(data.sign(::qstrcmp(data.left, data.right)), data.cmp);
    QCOMPARE(data.sign(::qstricmp(data.left, data.right)), data.icmp);
    QCOMPARE(data.sign(::qstrncmp(data.left, data.right, data.clip)), data.ncmp);
    QCOMPARE(data.sign(::qstrnicmp(data.left, data.right, data.clip)), data.nicmp);
}

void tst_QByteArray::compare_singular()
{
    QCOMPARE(QByteArray().compare(nullptr, Qt::CaseInsensitive), 0);
    QCOMPARE(QByteArray().compare("", Qt::CaseInsensitive), 0);
    QVERIFY(QByteArray("a").compare(nullptr, Qt::CaseInsensitive) > 0);
    QVERIFY(QByteArray("a").compare("", Qt::CaseInsensitive) > 0);
    QVERIFY(QByteArray().compare("a", Qt::CaseInsensitive) < 0);
    QCOMPARE(QByteArray().compare(QByteArray(), Qt::CaseInsensitive), 0);
    QVERIFY(QByteArray().compare(QByteArray("a"), Qt::CaseInsensitive) < 0);
    QVERIFY(QByteArray("a").compare(QByteArray(), Qt::CaseInsensitive) > 0);
}

void tst_QByteArray::compareCharStar_data()
{
    QTest::addColumn<QByteArray>("str1");
    QTest::addColumn<QByteArray>("string2");
    QTest::addColumn<int>("result");

    QTest::newRow("null-null") << QByteArray() << QByteArray() << 0;
    QTest::newRow("null-empty") << QByteArray() << QByteArray("") << 0;
    QTest::newRow("null-full") << QByteArray() << QByteArray("abc") << -1;
    QTest::newRow("empty-null") << QByteArray("") << QByteArray() << 0;
    QTest::newRow("empty-empty") << QByteArray("") << QByteArray("") << 0;
    QTest::newRow("empty-full") << QByteArray("") << QByteArray("abc") << -1;
    QTest::newRow("raw-null") << QByteArray::fromRawData("abc", 0) << QByteArray() << 0;
    QTest::newRow("raw-empty") << QByteArray::fromRawData("abc", 0) << QByteArray("") << 0;
    QTest::newRow("raw-full") << QByteArray::fromRawData("abc", 0) << QByteArray("abc") << -1;

    QTest::newRow("full-null") << QByteArray("abc") << QByteArray() << +1;
    QTest::newRow("full-empty") << QByteArray("abc") << QByteArray("") << +1;

    QTest::newRow("equal1") << QByteArray("abc") << QByteArray("abc") << 0;
    QTest::newRow("equal2") << QByteArray("abcd", 3) << QByteArray("abc") << 0;
    QTest::newRow("equal3") << QByteArray::fromRawData("abcd", 3) << QByteArray("abc") << 0;

    QTest::newRow("less1") << QByteArray("ab") << QByteArray("abc") << -1;
    QTest::newRow("less2") << QByteArray("abb") << QByteArray("abc") << -1;
    QTest::newRow("less3") << QByteArray::fromRawData("abc", 2) << QByteArray("abc") << -1;
    QTest::newRow("less4") << QByteArray("", 1) << QByteArray("abc") << -1;
    QTest::newRow("less5") << QByteArray::fromRawData("", 1) << QByteArray("abc") << -1;
    QTest::newRow("less6") << QByteArray("a\0bc", 4) << QByteArray("a.bc") << -1;

    QTest::newRow("greater1") << QByteArray("ac") << QByteArray("abc") << +1;
    QTest::newRow("greater2") << QByteArray("abd") << QByteArray("abc") << +1;
    QTest::newRow("greater3") << QByteArray("abcd") << QByteArray("abc") << +1;
    QTest::newRow("greater4") << QByteArray::fromRawData("abcd", 4) << QByteArray("abc") << +1;
}

void tst_QByteArray::compareCharStar()
{
    QFETCH(QByteArray, str1);
    QFETCH(QByteArray, string2);
    QFETCH(int, result);

    const bool isEqual   = result == 0;
    const bool isLess    = result < 0;
    const bool isGreater = result > 0;
    const char *str2 = string2.isNull() ? nullptr : string2.constData();

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
    (void)string.repeated(3);
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
}

void tst_QByteArray::byteRefDetaching() const
{
    {
        QByteArray str = "str";
        QByteArray copy = str;
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
    QCOMPARE(nil1.capacity(), 0);
    QCOMPARE(nil2.capacity(), 0);

    nil1.resize(5);
    QVERIFY(nil1.capacity() >= 5);
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

void tst_QByteArray::resize()
{
    QByteArray ba;
    ba.resize(15);
    QCOMPARE(ba.size(), qsizetype(15));
    ba.resize(10);
    QCOMPARE(ba.size(), 10);
    ba.resize(0);
    QCOMPARE(ba.size(), 0);
    ba.resize(5, 'a');
    QCOMPARE(ba.size(), 5);
    QCOMPARE(ba, "aaaaa");
    ba.resize(10, 'b');
    QCOMPARE(ba.size(), 10);
    QCOMPARE(ba, "aaaaabbbbb");
}

void tst_QByteArray::movability_data()
{
    prependExtended_data();

    QTest::newRow("0x00000000") << QByteArray("\x00\x00\x00\x00", 4);
    QTest::newRow("0x000000ff") << QByteArray("\x00\x00\x00\xff", 4);
    QTest::newRow("0xffffffff") << QByteArray("\xff\xff\xff\xff", 4);
    QTest::newRow("empty") << QByteArray("");
    QTest::newRow("null") << QByteArray();
    QTest::newRow("sss") << QByteArray(3, 's');
}

void tst_QByteArray::movability()
{
    QFETCH(QByteArray, array);

    static_assert(QTypeInfo<QByteArray>::isRelocatable);

    const int size = array.size();
    const bool isEmpty = array.isEmpty();
    const bool isNull = array.isNull();
    const int capacity = array.capacity();

    QByteArray memSpace;

    // we need only memory space not the instance
    memSpace.~QByteArray();
    // move array -> memSpace
    memcpy((void *)&memSpace, (const void *)&array, sizeof(QByteArray));
    // reconstruct empty QByteArray
    new (&array) QByteArray;

    QCOMPARE(memSpace.size(), size);
    QCOMPARE(memSpace.isEmpty(), isEmpty);
    QCOMPARE(memSpace.isNull(), isNull);
    QCOMPARE(memSpace.capacity(), capacity);

    // try to not crash
    (void)memSpace.toLower();
    (void)memSpace.toUpper();
    memSpace.prepend('a');
    memSpace.append("b", 1);
    memSpace.squeeze();
    memSpace.reserve(array.size() + 16);

    QByteArray copy(memSpace);

    // reinitialize base values
    const int newSize = size + 2;
    const bool newIsEmpty = false;
    const bool newIsNull = false;
    const int newCapacity = memSpace.capacity();

    // move back memSpace -> array
    array.~QByteArray();
    memcpy((void *)&array, (const void *)&memSpace, sizeof(QByteArray));
    // reconstruct empty QByteArray
    new (&memSpace) QByteArray;

    QCOMPARE(array.size(), newSize);
    QCOMPARE(array.isEmpty(), newIsEmpty);
    QCOMPARE(array.isNull(), newIsNull);
    QCOMPARE(array.capacity(), newCapacity);
    QVERIFY(array.startsWith('a'));
    QVERIFY(array.endsWith('b'));

    QCOMPARE(copy.size(), newSize);
    QCOMPARE(copy.isEmpty(), newIsEmpty);
    QCOMPARE(copy.isNull(), newIsNull);
    QCOMPARE(copy.capacity(), newCapacity);
    QVERIFY(copy.startsWith('a'));
    QVERIFY(copy.endsWith('b'));

    // try to not crash
    array.squeeze();
    array.reserve(array.size() + 3);
    QVERIFY(true);
}

void tst_QByteArray::literals()
{
    QByteArray str(QByteArrayLiteral("abcd"));

    QVERIFY(str.size() == 4);
    QCOMPARE(str.capacity(), 0);
    QVERIFY(str == "abcd");
    QVERIFY(!str.data_ptr()->isMutable());

    const char *s = str.constData();
    QByteArray str2 = str;
    QVERIFY(str2.constData() == s);
    QCOMPARE(str2.capacity(), 0);

    // detach on non const access
    QVERIFY(str.data() != s);
    QVERIFY(str.capacity() >= str.size());

    QVERIFY(str2.constData() == s);
    QVERIFY(str2.data() != s);
    QVERIFY(str2.capacity() >= str2.size());
}

void tst_QByteArray::userDefinedLiterals()
{
    {
        QByteArray str = "abcd"_ba;

        QVERIFY(str.size() == 4);
        QCOMPARE(str.capacity(), 0);
        QVERIFY(str == "abcd");
        QVERIFY(!str.data_ptr()->isMutable());

        const char *s = str.constData();
        QByteArray str2 = str;
        QVERIFY(str2.constData() == s);
        QCOMPARE(str2.capacity(), 0);

        // detach on non const access
        QVERIFY(str.data() != s);
        QVERIFY(str.capacity() >= str.size());

        QVERIFY(str2.constData() == s);
        QVERIFY(str2.data() != s);
        QVERIFY(str2.capacity() >= str2.size());
    }

#if QT_DEPRECATED_SINCE(6, 8)
    {
        QT_IGNORE_DEPRECATIONS(QByteArray str = "abcd"_qba;)

        QVERIFY(str.size() == 4);
        QCOMPARE(str.capacity(), 0);
        QVERIFY(str == "abcd");
        QVERIFY(!str.data_ptr()->isMutable());

        const char *s = str.constData();
        QByteArray str2 = str;
        QVERIFY(str2.constData() == s);
        QCOMPARE(str2.capacity(), 0);

        // detach on non const access
        QVERIFY(str.data() != s);
        QVERIFY(str.capacity() >= str.size());

        QVERIFY(str2.constData() == s);
        QVERIFY(str2.data() != s);
        QVERIFY(str2.capacity() >= str2.size());
    }
#endif // QT_DEPRECATED_SINCE(6, 8)
}

void tst_QByteArray::toUpperLower_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("upper");
    QTest::addColumn<QByteArray>("lower");

    {
        QByteArray nonAscii(128, Qt::Uninitialized);
        char *data = nonAscii.data();
        for (unsigned char i = 0; i < 128; ++i)
            data[i] = i + 128;
        QTest::newRow("non-ASCII") << nonAscii << nonAscii << nonAscii;
    }

    QTest::newRow("null") << QByteArray() << QByteArray() << QByteArray();
    QTest::newRow("empty") << QByteArray("") << QByteArray("") << QByteArray("");
    QTest::newRow("literal") << QByteArrayLiteral("Hello World")
                             << QByteArrayLiteral("HELLO WORLD")
                             << QByteArrayLiteral("hello world");
    QTest::newRow("ascii") << QByteArray("Hello World, this is a STRING")
                           << QByteArray("HELLO WORLD, THIS IS A STRING")
                           << QByteArray("hello world, this is a string");
    QTest::newRow("nul") << QByteArray("a\0B", 3) << QByteArray("A\0B", 3) << QByteArray("a\0b", 3);
}

void tst_QByteArray::toUpperLower()
{
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, upper);
    QFETCH(QByteArray, lower);
    QVERIFY(upper.isUpper());
    QVERIFY(lower.isLower());
    QCOMPARE(lower.toLower(), lower);
    QVERIFY(lower.toLower().isLower());
    QCOMPARE(upper.toUpper(), upper);
    QVERIFY(upper.toUpper().isUpper());
    QCOMPARE(input.toUpper(), upper);
    QVERIFY(input.toUpper().isUpper());
    QCOMPARE(input.toLower(), lower);
    QVERIFY(input.toLower().isLower());

    QByteArray copy = input;
    QCOMPARE(std::move(copy).toUpper(), upper);
    copy = input;
    copy.detach();
    QCOMPARE(std::move(copy).toUpper(), upper);

    copy = input;
    QCOMPARE(std::move(copy).toLower(), lower);
    copy = input;
    copy.detach();
    QCOMPARE(std::move(copy).toLower(), lower);

    copy = lower;
    QCOMPARE(std::move(copy).toLower(), lower);
    copy = lower;
    copy.detach();
    QCOMPARE(std::move(copy).toLower(), lower);

    copy = upper;
    QCOMPARE(std::move(copy).toUpper(), upper);
    copy = upper;
    copy.detach();
    QCOMPARE(std::move(copy).toUpper(), upper);
}

void tst_QByteArray::isUpper()
{
    QVERIFY(QByteArray().isUpper());
    QVERIFY(QByteArray("").isUpper());
    QVERIFY(QByteArray("TEXT").isUpper());
    QVERIFY(QByteArray("\xD0\xDE").isUpper());
    QVERIFY(QByteArray("\xD7").isUpper());
    QVERIFY(QByteArray("\xDF").isUpper());
    QVERIFY(!QByteArray("text").isUpper());
    QVERIFY(!QByteArray("Text").isUpper());
    QVERIFY(!QByteArray("tExt").isUpper());
    QVERIFY(!QByteArray("teXt").isUpper());
    QVERIFY(!QByteArray("texT").isUpper());
    QVERIFY(!QByteArray("TExt").isUpper());
    QVERIFY(!QByteArray("teXT").isUpper());
    QVERIFY(!QByteArray("tEXt").isUpper());
    QVERIFY(!QByteArray("tExT").isUpper());
    QVERIFY(QByteArray("@ABYZ[").isUpper());
    QVERIFY(!QByteArray("@abyz[").isUpper());
    QVERIFY(QByteArray("`ABYZ{").isUpper());
    QVERIFY(!QByteArray("`abyz{").isUpper());
}

void tst_QByteArray::isLower()
{
    QVERIFY(QByteArray().isLower());
    QVERIFY(QByteArray("").isLower());
    QVERIFY(QByteArray("text").isLower());
    QVERIFY(QByteArray("\xE0\xFF").isLower());
    QVERIFY(QByteArray("\xF7").isLower());
    QVERIFY(!QByteArray("Text").isLower());
    QVERIFY(!QByteArray("tExt").isLower());
    QVERIFY(!QByteArray("teXt").isLower());
    QVERIFY(!QByteArray("texT").isLower());
    QVERIFY(!QByteArray("TExt").isLower());
    QVERIFY(!QByteArray("teXT").isLower());
    QVERIFY(!QByteArray("tEXt").isLower());
    QVERIFY(!QByteArray("tExT").isLower());
    QVERIFY(!QByteArray("TEXT").isLower());
    QVERIFY(!QByteArray("@ABYZ[").isLower());
    QVERIFY(QByteArray("@abyz[").isLower());
    QVERIFY(!QByteArray("`ABYZ{").isLower());
    QVERIFY(QByteArray("`abyz{").isLower());
}

using ByteArrayOrChar = std::variant<QByteArray, char>;
void tst_QByteArray::indexOf_data()
{
    qRegisterMetaType<ByteArrayOrChar>();
    QTest::addColumn<QByteArray>("haystack");
    QTest::addColumn<ByteArrayOrChar>("needle");
    QTest::addColumn<int>("expectedIndex");
    QTest::addColumn<int>("from");

    const QByteArray haystack = "abc 123 cba \x80 \x08 \x00 \x01 \x81"_ba;

    QTest::newRow("not_found_1_char_string") << haystack << ByteArrayOrChar("d"_ba) << -1 << 0;
    QTest::newRow("not_found_char") << haystack << ByteArrayOrChar('d') << -1 << 0;

    QTest::newRow("not_found_string") << haystack << ByteArrayOrChar("abcd"_ba) << -1 << 0;
    QTest::newRow("found_1_char_string") << haystack << ByteArrayOrChar("a"_ba) << 0 << 0;

    QTest::newRow("found_char") << haystack << ByteArrayOrChar('a') << 0 << 0;
    QTest::newRow("found_string") << haystack << ByteArrayOrChar("cba"_ba) << 8 << 0;

    QTest::newRow("found_empty_string") << haystack << ByteArrayOrChar(""_ba) << 0 << 0;

    QTest::newRow("found_embedded_null") << haystack << ByteArrayOrChar("\x00"_ba) << 16 << 0;
    QTest::newRow("not_found_terminating_null") << haystack << ByteArrayOrChar("\x00"_ba) << -1 << 17;
    QTest::newRow("found_char_star_0x80") << haystack << ByteArrayOrChar("\x80"_ba) << 12 << 0;
    QTest::newRow("found_char_0x80") << haystack << ByteArrayOrChar('\x80') << 12 << 0;
    QTest::newRow("found_char_0x81") << haystack << ByteArrayOrChar('\x81') << 20 << 0;

    // Make the needle sufficiently large to try to potentially trip boundary guards:
    QTest::newRow("needle_larger_than_haystack") << "b"_ba << ByteArrayOrChar(":"_ba.repeated(4096)) << -1 << 0;
}

void tst_QByteArray::indexOf()
{
    QFETCH(QByteArray, haystack);
    QFETCH(ByteArrayOrChar, needle);
    QFETCH(int, expectedIndex);
    QFETCH(int, from);

    if (auto *qba = std::get_if<QByteArray>(&needle)) {
        QCOMPARE(haystack.indexOf(*qba, from), expectedIndex);
    } else {
        char c = std::get<char>(needle);
        QCOMPARE(haystack.indexOf(c, from), expectedIndex);
    }
}

void tst_QByteArray::lastIndexOf_data()
{
    qRegisterMetaType<ByteArrayOrChar>();
    QTest::addColumn<QByteArray>("haystack");
    QTest::addColumn<ByteArrayOrChar>("needle");
    QTest::addColumn<int>("expectedIndex");
    QTest::addColumn<int>("from");

    const QByteArray haystack = "abc 123 cba \x80 \x08 \x00 \x01 \x81"_ba;

    QTest::newRow("not_found_1_char_string") << haystack << ByteArrayOrChar("d"_ba) << -1 << -1;
    QTest::newRow("not_found_char") << haystack << ByteArrayOrChar('d') << -1 << -1;

    QTest::newRow("not_found_string") << haystack << ByteArrayOrChar("abcd"_ba) << -1 << -1;
    QTest::newRow("found_1_char_string") << haystack << ByteArrayOrChar("a"_ba) << 10 << -1;

    QTest::newRow("found_char") << haystack << ByteArrayOrChar('a') << 10 << -1;
    QTest::newRow("found_string") << haystack << ByteArrayOrChar("cba"_ba) << 8 << -1;

    QTest::newRow("found_empty_string") << haystack << ByteArrayOrChar(""_ba) << haystack.size() << -1;

    QTest::newRow("found_embedded_null") << haystack << ByteArrayOrChar("\x00"_ba) << 16 << -1;
    QTest::newRow("not_found_leading_null") << haystack << ByteArrayOrChar("\x00"_ba) << -1 << 15;
    QTest::newRow("found_char_star_0x80") << haystack << ByteArrayOrChar("\x80"_ba) << 12 << -1;
    QTest::newRow("found_char_0x80") << haystack << ByteArrayOrChar('\x80') << 12 << -1;
    QTest::newRow("found_char_0x81") << haystack << ByteArrayOrChar('\x81') << 20 << -1;

    // Make the needle sufficiently large to try to potentially trip boundary guards:
    QTest::newRow("needle_larger_than_haystack") << "b"_ba << ByteArrayOrChar(":"_ba.repeated(4096)) << -1 << 0;
}

void tst_QByteArray::lastIndexOf()
{
    QFETCH(QByteArray, haystack);
    QFETCH(ByteArrayOrChar, needle);
    QFETCH(int, expectedIndex);
    QFETCH(int, from);

    if (auto *qba = std::get_if<QByteArray>(&needle)) {
        QCOMPARE(haystack.lastIndexOf(*qba, from), expectedIndex);
    } else {
        char c = std::get<char>(needle);
        QCOMPARE(haystack.lastIndexOf(c, from), expectedIndex);
    }
}

void tst_QByteArray::macTypes()
{
#ifndef Q_OS_DARWIN
    QSKIP("This is a Apple-only test");
#else
    extern void tst_QByteArray_macTypes(); // in qbytearray_mac.mm
    tst_QByteArray_macTypes();
#endif
}

void tst_QByteArray::stdString()
{
    std::string stdstr( "QByteArray" );

    const QByteArray stlqt = QByteArray::fromStdString(stdstr);
    QCOMPARE(stlqt.size(), int(stdstr.length()));
    QCOMPARE(stlqt.data(), stdstr.c_str());
    QCOMPARE(stlqt.toStdString(), stdstr);

    std::string utf8str( "Nøt æscii" );
    const QByteArray u8 = QByteArray::fromStdString(utf8str);
    const QByteArray l1 = QString::fromUtf8(u8).toLatin1();
    std::string l1str = l1.toStdString();
    QVERIFY(l1str.length() < utf8str.length());
}

void tst_QByteArray::emptyAndClear()
{
    QByteArray a;
    QVERIFY(a.isEmpty());
    a.clear();
    QVERIFY(a.isEmpty());
    QVERIFY(!a.isDetached());

    a.append("data");
    QVERIFY(!a.isEmpty());

    a.clear();
    QVERIFY(a.isEmpty());
}

void tst_QByteArray::fill()
{
    QByteArray a;
    QVERIFY(a.isEmpty());
    QVERIFY(!a.isDetached());

    // filling an empty QByteArray does nothing
    a.fill('a');
    QVERIFY(a.isEmpty());
    QVERIFY(!a.isDetached());

    // filling empty QByteArray to 0 length does nothing
    a.fill('a', 0);
    QVERIFY(a.isEmpty());
    QVERIFY(!a.isDetached());

    a.fill('b', 5);
    QCOMPARE(a, QByteArray("bbbbb"));

    a.fill('c');
    QCOMPARE(a, QByteArray("ccccc"));

    a.fill('d', 2);
    QCOMPARE(a, QByteArray("dd"));

    // filling to 0 length empties the QByteArray
    a.fill('a', 0);
    QVERIFY(a.isEmpty());
}

void tst_QByteArray::dataPointers()
{
    QByteArray a;
    const char *constPtr = a.constData();
    QCOMPARE(a.data(), constPtr); // does not detach on empty QBA.

    a = "abc"; // detaches
    const char *dataConstPtr = a.constData();
    QVERIFY(dataConstPtr != constPtr);

    QByteArray copy = a;
    QCOMPARE(copy.constData(), dataConstPtr);

    char *dataPtr = copy.data(); // detaches, as the QBA is not empty
    QVERIFY(dataPtr != dataConstPtr);

    *dataPtr = 'd';
    QCOMPARE(copy, QByteArray("dbc"));
    QCOMPARE(a, QByteArray("abc"));
}

void tst_QByteArray::truncate()
{
    QByteArray a;
    a.truncate(0);
    a.truncate(10);
    QVERIFY(a.isEmpty());
    QVERIFY(!a.isDetached());

    a = QByteArray("abcdef");
    a.truncate(4);
    QCOMPARE(a, QByteArray("abcd"));
    a.truncate(5);
    QCOMPARE(a, QByteArray("abcd"));

    a.truncate(-5);
    QVERIFY(a.isEmpty());
}

void tst_QByteArray::trimmed_data()
{
    QTest::addColumn<QByteArray>("full" );
    QTest::addColumn<QByteArray>("trimmed" );

    QTest::addRow("null") << QByteArray() << QByteArray();
    QTest::addRow("simple") << "Text"_ba << "Text"_ba;
    QTest::addRow("single-space") << " "_ba << ""_ba;
    QTest::addRow("single-char") << " a   "_ba << "a"_ba;
    QTest::addRow("mixed") << " a \n\t\v b   "_ba << "a \n\t\v b"_ba;
}

void tst_QByteArray::trimmed()
{
    QFETCH(QByteArray, full);
    QFETCH(QByteArray, trimmed);

    // Shared
    if (!full.isNull())
        QVERIFY(!full.isDetached());
    QCOMPARE(full.trimmed(), trimmed); // lvalue
    QCOMPARE(QByteArray(full).trimmed(), trimmed); // rvalue
    QCOMPARE(full.isNull(), trimmed.isNull());

    // Not shared
    full = QByteArrayView(full).toByteArray();
    if (!full.isNull())
        QVERIFY(full.isDetached());
    QCOMPARE(full.trimmed(), trimmed); // lvalue
    QCOMPARE(QByteArray(full).trimmed(), trimmed); // rvalue
    QCOMPARE(full.isNull(), trimmed.isNull());
}

void tst_QByteArray::simplified()
{
    QFETCH(QByteArray, source);
    QFETCH(QByteArray, expected);

    QCOMPARE(source.simplified(), expected);
    QByteArray copy = source;
    QCOMPARE(std::move(copy).simplified(), expected);

    if (source.isEmpty())
        QVERIFY(!source.isDetached());
}

void tst_QByteArray::simplified_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("null") << QByteArray() << QByteArray();
    QTest::newRow("empty") << QByteArray("") << QByteArray("");
    QTest::newRow("no extra spaces") << QByteArray("a bc d") << QByteArray("a bc d");
    QTest::newRow("with spaces") << QByteArray("\t \v a  b\r\nc\td \r\n\f")
                                 << QByteArray("a b c d");
    QTest::newRow("all spaces") << QByteArray("\t \r \n \v \f") << QByteArray("");
}

void tst_QByteArray::left()
{
    QByteArray a;
    QCOMPARE(QByteArray().left(0), QByteArray());
    QCOMPARE(QByteArray().left(10), QByteArray());
    QCOMPARE(a.left(0), QByteArray());
    QCOMPARE(a.left(10), QByteArray());
    QVERIFY(!a.isDetached());
    QCOMPARE(QByteArray(a).left(0), QByteArray());
    QCOMPARE(QByteArray(a).left(10), QByteArray());
    QCOMPARE(detached(a).left(0), QByteArray());
    QCOMPARE(detached(a).left(10), QByteArray());

    a = QByteArray("abcdefgh");
    const char *ptr = a.constData();

    // lvalue
    QCOMPARE(a.left(5), QByteArray("abcde"));
    QCOMPARE(a.left(20), a);
    QCOMPARE(a.left(-5), QByteArray());
    // calling left() does not modify the source array
    QCOMPARE(a.constData(), ptr);

    // rvalue, not detached
    QCOMPARE(QByteArray(a).left(5), QByteArray("abcde"));
    QCOMPARE(QByteArray(a).left(20), a);
    QCOMPARE(QByteArray(a).left(-5), QByteArray());
    // calling left() does not modify the source array
    QCOMPARE(a.constData(), ptr);

    // rvalue, detached
    QCOMPARE(detached(a).left(5), QByteArray("abcde"));
    QCOMPARE(detached(a).left(20), a);
    QCOMPARE(detached(a).left(-5), QByteArray());
    // calling left() does not modify the source array
    QCOMPARE(a.constData(), ptr);
}

void tst_QByteArray::right()
{
    QByteArray a;
    QCOMPARE(QByteArray().right(0), QByteArray());
    QCOMPARE(QByteArray().right(10), QByteArray());
    QCOMPARE(a.right(0), QByteArray());
    QCOMPARE(a.right(10), QByteArray());
    QVERIFY(!a.isDetached());
    QCOMPARE(QByteArray(a).right(0), QByteArray());
    QCOMPARE(QByteArray(a).right(10), QByteArray());
    QCOMPARE(detached(a).right(0), QByteArray());
    QCOMPARE(detached(a).right(10), QByteArray());

    a = QByteArray("abcdefgh");
    const char *ptr = a.constData();

    // lvalue
    QCOMPARE(a.right(5), QByteArray("defgh"));
    QCOMPARE(a.right(20), a);
    QCOMPARE(a.right(-5), QByteArray());
    // calling right() does not modify the source array
    QCOMPARE(a.constData(), ptr);

    // rvalue, not detached
    QCOMPARE(QByteArray(a).right(5), QByteArray("defgh"));
    QCOMPARE(QByteArray(a).right(20), a);
    QCOMPARE(QByteArray(a).right(-5), QByteArray());
    // calling right() does not modify the source array
    QCOMPARE(a.constData(), ptr);

    // rvalue, detached
    QCOMPARE(detached(a).right(5), QByteArray("defgh"));
    QCOMPARE(detached(a).right(20), a);
    QCOMPARE(detached(a).right(-5), QByteArray());
    // calling right() does not modify the source array
    QCOMPARE(a.constData(), ptr);
}

void tst_QByteArray::mid()
{
    QByteArray a;
    QCOMPARE(QByteArray().mid(0), QByteArray());
    QCOMPARE(a.mid(0, 10), QByteArray());
    QCOMPARE(a.mid(0), QByteArray());
    QCOMPARE(a.mid(0, 10), QByteArray());
    QCOMPARE(a.mid(10), QByteArray());
    QVERIFY(!a.isDetached());
    QCOMPARE(QByteArray(a).mid(0), QByteArray());
    QCOMPARE(QByteArray(a).mid(0, 10), QByteArray());
    QCOMPARE(QByteArray(a).mid(10), QByteArray());
    QCOMPARE(detached(a).mid(0), QByteArray());
    QCOMPARE(detached(a).mid(0, 10), QByteArray());
    QCOMPARE(detached(a).mid(10), QByteArray());

    a = QByteArray("abcdefgh");
    const char *ptr = a.constData();

    // lvalue
    QCOMPARE(a.mid(2), QByteArray("cdefgh"));
    QCOMPARE(a.mid(2, 3), QByteArray("cde"));
    QCOMPARE(a.mid(20), QByteArray());
    QCOMPARE(a.mid(-5), QByteArray("abcdefgh"));
    QCOMPARE(a.mid(-5, 8), QByteArray("abc"));
    // calling mid() does not modify the source array
    QCOMPARE(a.constData(), ptr);

    // rvalue, not detached
    QCOMPARE(QByteArray(a).mid(2), QByteArray("cdefgh"));
    QCOMPARE(QByteArray(a).mid(2, 3), QByteArray("cde"));
    QCOMPARE(QByteArray(a).mid(20), QByteArray());
    QCOMPARE(QByteArray(a).mid(-5), QByteArray("abcdefgh"));
    QCOMPARE(QByteArray(a).mid(-5, 8), QByteArray("abc"));
    // calling mid() does not modify the source array
    QCOMPARE(a.constData(), ptr);

    // rvalue, detached
    QCOMPARE(detached(a).mid(2), QByteArray("cdefgh"));
    QCOMPARE(detached(a).mid(2, 3), QByteArray("cde"));
    QCOMPARE(detached(a).mid(20), QByteArray());
    QCOMPARE(detached(a).mid(-5), QByteArray("abcdefgh"));
    QCOMPARE(detached(a).mid(-5, 8), QByteArray("abc"));
    // calling mid() does not modify the source array
    QCOMPARE(a.constData(), ptr);
}

void tst_QByteArray::length()
{
    QFETCH(QByteArray, src);
    QFETCH(qsizetype, res);

    QCOMPARE(src.size(), res);
    QCOMPARE(src.size(), res);
#if QT_DEPRECATED_SINCE(6, 4)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    QCOMPARE(src.size(), res);
QT_WARNING_POP
#endif
}

void tst_QByteArray::length_data()
{
    QTest::addColumn<QByteArray>("src");
    QTest::addColumn<qsizetype>("res");

    QTest::newRow("null") << QByteArray() << qsizetype(0);
    QTest::newRow("empty") << QByteArray("") << qsizetype(0);
    QTest::newRow("letters and digits") << QByteArray("abc123") << qsizetype(6);
    QTest::newRow("with space chars") << QByteArray(" abc\r\n123\t\v") << qsizetype(11);
    QTest::newRow("with '\\0'") << QByteArray("abc\0def", 7) << qsizetype(7);
    QTest::newRow("with '\\0' no size") << QByteArray("abc\0def") << qsizetype(3);
}

void tst_QByteArray::slice() const
{
    QByteArray a;

    a.slice(0);
    QVERIFY(a.isEmpty());
    QVERIFY(a.isNull());
    a.slice(0, 0);
    QVERIFY(a.isEmpty());
    QVERIFY(a.isNull());

    a = "Five pineapples";

    a.slice(5);
    QCOMPARE_EQ(a, "pineapples");

    a.slice(4, 3);
    QCOMPARE_EQ(a, "app");

    a.slice(a.size());
    QVERIFY(a.isEmpty());

    a.slice(0, 0);
    QVERIFY(a.isEmpty());
}

void tst_QByteArray::std_stringview_conversion()
{
#ifdef QT_BYTEARRAY_CONVERTS_TO_STD_STRING_VIEW
    static_assert(std::is_convertible_v<QByteArray, std::string_view>);

    QByteArray ba;
    std::string_view sv = ba;
    QCOMPARE(sv, std::string_view());
    sv = std::string_view(ba);
    QCOMPARE(sv, std::string_view());

    ba = ""_ba;
    sv = ba;
    QCOMPARE(ba.size(), 0);
    QCOMPARE(sv.size(), size_t(0));
    QCOMPARE(sv, std::string_view());

    ba = "Hello"_ba;
    sv = ba;
    QCOMPARE(ba.size(), 5);
    QCOMPARE(sv.size(), size_t(5));
    QCOMPARE(sv, std::string_view("Hello"));

    ba = "Hello\0world"_ba;
    sv = ba;
    QCOMPARE(ba.size(), 11);
    QCOMPARE(sv.size(), size_t(11));
    QCOMPARE(sv, std::string_view("Hello\0world", 11));
#else
    QSKIP("This compiler does not support QByteArray -> std::string_view implicit conversions");
#endif
}

QTEST_MAIN(tst_QByteArray)
#include "tst_qbytearray.moc"
