// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtNetwork/qhttpheaders.h>

#include <QtTest/qtest.h>

#include <QtCore/qbuffer.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>
#include <optional>

using namespace Qt::StringLiterals;

class tst_QHttpHeaders : public QObject
{
    Q_OBJECT

private slots:
    void constructors();
    void accessors();
    void wellKnownHeader();
    void headerNameField();
    void headerValueField();
    void valueEncoding();
    void replaceOrAppend();
    void intValues();
    void dateTimeValues();
    void rangeValues();
    void data_stream_data();
    void data_stream();
    void data_stream_invalid_data();
    void data_stream_invalid();

private:
    static constexpr QAnyStringView n1{"name1"};
    static constexpr QAnyStringView n2{"name2"};
    static constexpr QAnyStringView n3{"name3"};
    static constexpr QAnyStringView v1{"value1"};
    static constexpr QAnyStringView v2{"value2"};
    static constexpr QAnyStringView v3{"value3"};
    static constexpr QAnyStringView N1{"NAME1"};
    static constexpr QAnyStringView N2{"NAME2"};
    static constexpr QAnyStringView N3{"NAME3"};
    static constexpr QAnyStringView V1{"VALUE1"};
    static constexpr QAnyStringView V2{"VALUE2"};
    static constexpr QAnyStringView V3{"VALUE3"};
};

void tst_QHttpHeaders::constructors()
{
    // Default ctor
    QHttpHeaders h1;
    QVERIFY(h1.isEmpty());

    // Copy ctor
    QHttpHeaders h2(h1);
    QCOMPARE(h2.toListOfPairs(), h1.toListOfPairs());

    // Copy assignment
    QHttpHeaders h3;
    h3 = h1;
    QCOMPARE(h3.toListOfPairs(), h1.toListOfPairs());

    // Move assignment
    QHttpHeaders h4;
    h4 = std::move(h2);
    QCOMPARE(h4.toListOfPairs(), h1.toListOfPairs());

    // Move ctor
    QHttpHeaders h5(std::move(h4));
    QCOMPARE(h5.toListOfPairs(), h1.toListOfPairs());

    // Constructors that are counterparts to 'toXXX()' conversion getters
    const QByteArray nb1{"name1"};
    const QByteArray nb2{"name2"};
    const QByteArray nv1{"value1"};
    const QByteArray nv2{"value2"};
    // Initialize three QHttpHeaders with similar content, and verify that they have
    // similar header entries
#define CONTAINS_HEADER(NAME, VALUE) \
    QVERIFY(hlist.contains(NAME) && hmap.contains(NAME) && hhash.contains(NAME)); \
    QCOMPARE(hlist.combinedValue(NAME), VALUE); \
    QCOMPARE(hmap.combinedValue(NAME), VALUE);  \
    QCOMPARE(hhash.combinedValue(NAME), VALUE); \

    QList<std::pair<QByteArray, QByteArray>> list{{nb1, nv1}, {nb2, nv2}, {nb2, nv2}};
    QMultiMap<QByteArray, QByteArray> map{{nb1, nv1}, {nb2, nv2}, {nb2, nv2}};
    QMultiHash<QByteArray, QByteArray> hash{{nb1, nv1}, {nb2, nv2}, {nb2, nv2}};
    QHttpHeaders hlist = QHttpHeaders::fromListOfPairs(list);
    QHttpHeaders hmap = QHttpHeaders::fromMultiMap(map);
    QHttpHeaders hhash = QHttpHeaders::fromMultiHash(hash);
    CONTAINS_HEADER(nb1, v1);
    CONTAINS_HEADER(nb2, nv2 + ", " + nv2)
#undef CONTAINS_HEADER
}

void tst_QHttpHeaders::accessors()
{
    QHttpHeaders h1;

    // isEmpty(), clear(), size()
    h1.append(n1,v1);
    QVERIFY(!h1.isEmpty());
    QCOMPARE(h1.size(), 1);
    QVERIFY(h1.append(n1, v1));
    QCOMPARE(h1.size(), 2);
    h1.insert(0, n1, v1);
    QCOMPARE(h1.size(), 3);
    h1.clear();
    QVERIFY(h1.isEmpty());

    // contains()
    h1.append(n1, v1);
    QVERIFY(h1.contains(n1));
    QVERIFY(h1.contains(N1));
    QVERIFY(!h1.contains(n2));
    QVERIFY(!h1.contains(QHttpHeaders::WellKnownHeader::Allow));
    h1.append(QHttpHeaders::WellKnownHeader::Accept, "nothing");
    QVERIFY(h1.contains(QHttpHeaders::WellKnownHeader::Accept));
    QVERIFY(h1.contains("accept"));

    // values()/value()
#define EXISTS_NOT(H, N) do {                       \
        QVERIFY(!H.contains(N));                    \
        QCOMPARE(H.value(N, "ENOENT"), "ENOENT");   \
        const auto values = H.values(N);            \
        QVERIFY(values.isEmpty());                  \
        QVERIFY(H.combinedValue(N).isNull());       \
    } while (false)

#define EXISTS_N_TIMES(X, H, N, ...) do {                           \
        const std::array expected = { __VA_ARGS__ };                \
        static_assert(std::tuple_size_v<decltype(expected)> == X);  \
        QVERIFY(H.contains(N));                                     \
        QCOMPARE(H.value(N, "ENOENT"), expected.front());           \
        const auto values = H.values(N);                            \
        QCOMPARE(values.size(), X);                                 \
        QCOMPARE(values.front(), expected.front());                 \
        /* ignore in-between */                                     \
        QCOMPARE(values.back(), expected.back());                   \
        QCOMPARE(H.combinedValue(N), values.join(", "));            \
    } while (false)

#define EXISTS_ONCE(H, N, V) EXISTS_N_TIMES(1, H, N, V)

    EXISTS_ONCE(h1, n1, v1);
    EXISTS_ONCE(h1, N1, v1);
    EXISTS_ONCE(h1, QHttpHeaders::WellKnownHeader::Accept, "nothing");
    EXISTS_ONCE(h1, "Accept", "nothing");

    EXISTS_NOT(h1, N2);
    EXISTS_NOT(h1, QHttpHeaders::WellKnownHeader::Allow);

    h1.clear();

    EXISTS_NOT(h1, n1);

    h1.append(n1, v1);
    h1.append(n1, v2);
    h1.append(n1, v3);
    h1.append(n2, v2);
    h1.append(n3, ""); // empty value

    EXISTS_N_TIMES(3, h1, n1, v1, v2, v3);
    EXISTS_N_TIMES(3, h1, N1, v1, v2, v3);
    EXISTS_ONCE(h1, n3, ""); // empty value

    h1.append(QHttpHeaders::WellKnownHeader::Accept, "nothing");
    h1.append(QHttpHeaders::WellKnownHeader::Accept, "ever");

    EXISTS_N_TIMES(2, h1, QHttpHeaders::WellKnownHeader::Accept, "nothing", "ever");
    EXISTS_NOT(h1, "nonexistent");

#undef EXISTS_ONCE
#undef EXISTS_N_TIMES
#undef EXISTS_NOT

    // valueAt()
    h1.clear();
    h1.append(n1, v1);
    h1.append(n2, v2);
    h1.append(n3, v3);
    QCOMPARE(h1.valueAt(0), v1);
    QCOMPARE(h1.valueAt(1), v2);
    QCOMPARE(h1.valueAt(2), v3);

    // nameAt()
    h1.clear();
    h1.append(n1, v1);
    h1.append(n2, v2);
    h1.append(n3, v3);
    QCOMPARE(h1.nameAt(0), n1);
    QCOMPARE(h1.nameAt(1), n2);
    QCOMPARE(h1.nameAt(2), n3);

    // removeAll()
    h1.clear();
    QVERIFY(h1.append(n1, v1));
    QVERIFY(h1.append(QHttpHeaders::WellKnownHeader::Accept, "nothing"));
    QVERIFY(h1.append(n1, v1));
    QCOMPARE(h1.size(), 3);
    h1.removeAll(n1);
    QVERIFY(!h1.contains(n1));
    QCOMPARE(h1.size(), 1);
    QVERIFY(h1.contains("accept"));
    h1.removeAll(QHttpHeaders::WellKnownHeader::Accept);
    QVERIFY(!h1.contains(QHttpHeaders::WellKnownHeader::Accept));

    // removeAt()
    h1.clear();
    h1.append(n1, v1);
    h1.append(n2, v2);
    h1.append(n3, v3);

    // Valid removals
    QVERIFY(h1.contains(n3));
    h1.removeAt(2);
    QVERIFY(!h1.contains(n3));
    QVERIFY(h1.contains(n1));
    h1.removeAt(0);
    QVERIFY(!h1.contains(n1));
    QVERIFY(h1.contains(n2));
    h1.removeAt(0);
    QVERIFY(!h1.contains(n2));
    QVERIFY(h1.isEmpty());

    // toListOfPairs()
    h1.clear();
    h1.append(n1, v1);
    h1.append(n2, v2);
    h1.append(N3, V3); // uppercase of n3
    auto list = h1.toListOfPairs();
    QCOMPARE(list.size(), h1.size());
    QCOMPARE(list.at(0).first, n1);
    QCOMPARE(list.at(0).second, v1);
    QCOMPARE(list.at(1).first, n2);
    QCOMPARE(list.at(1).second, v2);
    QCOMPARE(list.at(2).first, n3);
    QCOMPARE(list.at(2).second, V3);

    // toMultiMap()
    auto map = h1.toMultiMap();
    QCOMPARE(map.size(), h1.size());
    QCOMPARE(map.value(n1.toString().toLatin1()), v1);
    QCOMPARE(map.value(n2.toString().toLatin1()), v2);
    QCOMPARE(map.value(n3.toString().toLatin1()), V3);

    // toMultiHash()
    auto hash = h1.toMultiHash();
    QCOMPARE(hash.size(), h1.size());
    QCOMPARE(hash.value(n1.toString().toLatin1()), v1);
    QCOMPARE(hash.value(n2.toString().toLatin1()), v2);
    QCOMPARE(hash.value(n3.toString().toLatin1()), V3);

    // insert()
    h1.clear();
    h1.append(n3, v3);
    QVERIFY(h1.insert(0, n1, v1));
    list = h1.toListOfPairs();
    QCOMPARE(list.size(), 2);
    QCOMPARE(list.at(0).first, n1);
    QCOMPARE(list.at(0).second, v1);
    QCOMPARE(list.at(1).first, n3);
    QCOMPARE(list.at(1).second, v3);
    QVERIFY(h1.insert(1, n2, v2));
    list = h1.toListOfPairs();
    QCOMPARE(list.size(), 3);
    QCOMPARE(list.at(0).first, n1);
    QCOMPARE(list.at(0).second, v1);
    QCOMPARE(list.at(1).first, n2);
    QCOMPARE(list.at(1).second, v2);
    QCOMPARE(list.at(2).first, n3);
    QCOMPARE(list.at(2).second, v3);
    QVERIFY(h1.insert(1, QHttpHeaders::WellKnownHeader::Accept, "nothing"));
    QCOMPARE(h1.size(), 4);
    list = h1.toListOfPairs();
    QCOMPARE(list.at(1).first, "accept");
    QCOMPARE(list.at(1).second, "nothing");
    QVERIFY(h1.insert(list.size(), "LastName", "lastValue"));
    QCOMPARE(h1.size(), 5);
    list = h1.toListOfPairs();
    QCOMPARE(list.last().first, "lastname");
    QCOMPARE(list.last().second, "lastValue");
    // Failed insert
    QRegularExpression re("HTTP header name contained*");
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, re);
    QVERIFY(!h1.insert(0, "a€", "b"));

    // replace
    h1.clear();
    h1.append(n1, v1);
    h1.append(n2, v2);
    QCOMPARE(h1.size(), 2);
    QVERIFY(h1.replace(0, n3, v3));
    QVERIFY(h1.replace(1, QHttpHeaders::WellKnownHeader::Accept, "nothing"));
    QCOMPARE(h1.size(), 2);
    list = h1.toListOfPairs();
    QCOMPARE(list.at(0).first, n3);
    QCOMPARE(list.at(0).second, v3);
    QCOMPARE(list.at(1).first, "accept");
    QCOMPARE(list.at(1).second, "nothing");
    QVERIFY(h1.replace(1, "ACCEPT", "NOTHING"));
    QCOMPARE(h1.size(), 2);
    list = h1.toListOfPairs();
    QCOMPARE(list.at(0).first, n3);
    QCOMPARE(list.at(0).second, v3);
    QCOMPARE(list.at(1).first, "accept");
    QCOMPARE(list.at(1).second, "NOTHING");
    // Failed replace
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, re);
    QVERIFY(!h1.replace(0, "a€", "b"));

}

void tst_QHttpHeaders::wellKnownHeader()
{
    QByteArrayView view = QHttpHeaders::wellKnownHeaderName(QHttpHeaders::WellKnownHeader::AIM);
    QCOMPARE(view, "A-IM");
}

#define TEST_ILLEGAL_HEADER_NAME_CHARACTER(NAME)       \
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, re); \
    QVERIFY(!h1.append(NAME, v1));                     \
    QVERIFY(h1.isEmpty());                             \

void tst_QHttpHeaders::headerNameField()
{
    QHttpHeaders h1;

    // All allowed characters in different encodings and types
    // const char[]
    h1.append("abcdefghijklmnopqrstuvwyxzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#$%&'*+-.^_`|~", v1);
    QCOMPARE(h1.size(), 1);
    // UTF-8
    h1.append(u8"abcdefghijklmnopqrstuvwyxzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#$%&'*+-.^_`|~",
              v1);
    QCOMPARE(h1.size(), 2);
    // UTF-16
    h1.append(u"abcdefghijklmnopqrstuvwyxzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#$%&'*+-.^_`|~", v1);
    QCOMPARE(h1.size(), 3);
    // QString (UTF-16)
    h1.append(u"abcdefghijklmnopqrstuvwyxzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#$%&'*+-.^_`|~"_s,
              v1);
    QCOMPARE(h1.size(), 4);
    QCOMPARE(h1.nameAt(0), h1.nameAt(1));
    QCOMPARE(h1.nameAt(1), h1.nameAt(2));
    QCOMPARE(h1.nameAt(2), h1.nameAt(3));
    h1.clear();

    // Error cases
    // Header name must contain at least 1 character
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "HTTP header name cannot be empty");
    h1.append("", v1);
    QVERIFY(h1.isEmpty());
    // Disallowed ASCII/extended ASCII characters (not exhaustive list)
    QRegularExpression re("HTTP header name contained illegal character*");
    TEST_ILLEGAL_HEADER_NAME_CHARACTER("foo\x08" "bar"); // BS
    TEST_ILLEGAL_HEADER_NAME_CHARACTER("foo\x7F" "bar"); // DEL
    TEST_ILLEGAL_HEADER_NAME_CHARACTER("foo()" "bar");   // parantheses
    TEST_ILLEGAL_HEADER_NAME_CHARACTER("foobar" "¿");    // extended ASCII
    TEST_ILLEGAL_HEADER_NAME_CHARACTER("©" "foobar");    // extended ASCII
    TEST_ILLEGAL_HEADER_NAME_CHARACTER("foo,bar");       // comma
    // Disallowed UTF-8 characters
    TEST_ILLEGAL_HEADER_NAME_CHARACTER(u8"€");
    TEST_ILLEGAL_HEADER_NAME_CHARACTER(u8"𝒜𝒴𝟘𝟡𐎀𐎜𐒀𐒐𝓐𝓩𝔸𝔹𝕀𝕁𝕌𝕍𓂀𓂁𓃀𓃁𓇋𓇌𓉐𓉑𓋴𓋵𓎡𓎢𓎣𓏏");
    // Disallowed UTF-16 characters
    TEST_ILLEGAL_HEADER_NAME_CHARACTER(u"€");
    TEST_ILLEGAL_HEADER_NAME_CHARACTER(u"𝒜𝒴𝟘𝟡𐎀𐎜𐒀𐒐𝓐𝓩𝔸𝔹𝕀𝕁𝕌𝕍𓂀𓂁𓃀𓃁𓇋𓇌𓉐𓉑𓋴𓋵𓎡𓎢𓎣𓏏");

    // Non-null-terminated name. The 'x' below is to make sure the strings don't
    // null-terminate by happenstance
    h1.clear();
    constexpr char L1Array[] = {'a','b','c','x'};
    const QLatin1StringView nonNullLatin1{L1Array, sizeof(L1Array) - 1}; // abc

    constexpr char UTF8Array[] = {0x64, 0x65, 0x66, 0x78};
    const QUtf8StringView nonNullUTF8(UTF8Array, sizeof(UTF8Array) - 1); // def

    constexpr QChar UTF16Array[] = {'g', 'h', 'i', 'x'};
    QStringView nonNullUTF16(UTF16Array, sizeof(UTF16Array) / sizeof(QChar) - 1); // ghi

    h1.append(nonNullLatin1, v1);
    QCOMPARE(h1.size(), 1);
    QVERIFY(h1.contains(nonNullLatin1));
    QCOMPARE(h1.combinedValue(nonNullLatin1), v1);

    h1.append(nonNullUTF8, v2);
    QCOMPARE(h1.size(), 2);
    QVERIFY(h1.contains(nonNullUTF8));
    QCOMPARE(h1.combinedValue(nonNullUTF8), v2);

    h1.append(nonNullUTF16, v3);
    QCOMPARE(h1.size(), 3);
    QVERIFY(h1.contains(nonNullUTF16));
    QCOMPARE(h1.combinedValue(nonNullUTF16), v3);
}

#define TEST_ILLEGAL_HEADER_VALUE_CHARACTER(VALUE) \
QTest::ignoreMessage(QtMsgType::QtWarningMsg, re); \
    QVERIFY(!h1.append(n1, VALUE));                \
    QVERIFY(h1.isEmpty());                         \

void tst_QHttpHeaders::headerValueField()
{
    QHttpHeaders h1;

    // Visible ASCII characters and space and horizontal tab
    // const char[]
    h1.append(n1, "!\"#$%&'()*+,-./0123456789:; \t<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                  "`abcdefghijklmnopqrstuvwxyz{|}~");
    QCOMPARE(h1.size(), 1);
    // UTF-8
    h1.append(n1, u8"!\"#$%&'()*+,-./0123456789:; \t<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                    "`abcdefghijklmnopqrstuvwxyz{|}~");
    QCOMPARE(h1.size(), 2);
    // UTF-16
    h1.append(n1, u"!\"#$%&'()*+,-./0123456789:; \t<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                   "`abcdefghijklmnopqrstuvwxyz{|}~");
    QCOMPARE(h1.size(), 3);
    // QString / UTF-16
    h1.append(n1, u"!\"#$%&'()*+,-./0123456789:; \t<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                   "`abcdefghijklmnopqrstuvwxyz{|}~"_s);
    QCOMPARE(h1.size(), 4);
    const auto values = h1.values(n1);
    QVERIFY(!values.isEmpty() && values.size() == 4);
    QVERIFY(values[0] == values[1]
            && values[1] == values[2]
            && values[2] == values[3]);
    // Extended ASCII (explicit on Latin-1 to avoid UTF-8 interpretation)
    h1.append(n1, "\x80\x09\xB2\xFF"_L1);
    QCOMPARE(h1.size(), 5);
    // Empty value
    h1.append(n1, "");
    QCOMPARE(h1.size(), 6);
    // Leading and trailing space
    h1.clear();
    h1.append(n1, " foo ");
    QCOMPARE(h1.combinedValue(n1), "foo");
    h1.append(n1, "\tbar\t");
    QCOMPARE(h1.combinedValue(n1), "foo, bar");
    QCOMPARE(h1.size(), 2);

    h1.clear();
    QRegularExpression re("HTTP header value contained illegal character*");
    TEST_ILLEGAL_HEADER_VALUE_CHARACTER("foo\x08" "bar"); // BS
    TEST_ILLEGAL_HEADER_VALUE_CHARACTER("foo\x1B" "bar"); // ESC
    // Disallowed UTF-16 characters
    TEST_ILLEGAL_HEADER_VALUE_CHARACTER(u"€");
    TEST_ILLEGAL_HEADER_VALUE_CHARACTER(u"𝒜𝒴𝟘𝟡𐎀𐎜𐒀𐒐𝓐𝓩𝔸𝔹𝕀𝕁𝕌𝕍𓂀𓂁𓃀𓃁𓇋𓇌𓉐𓉑𓋴𓋵𓎡𓎢𓎣𓏏");

    // Non-null-terminated value. The 'x' below is to make sure the strings don't
    // null-terminate by happenstance
    h1.clear();
    constexpr char L1Array[] = {'a','b','c','x'};
    const QLatin1StringView nonNullLatin1{L1Array, sizeof(L1Array) - 1}; // abc

    constexpr char UTF8Array[] = {0x64, 0x65, 0x66, 0x78};
    const QUtf8StringView nonNullUTF8(UTF8Array, sizeof(UTF8Array) - 1); // def

    constexpr QChar UTF16Array[] = {'g', 'h', 'i', 'x'};
    QStringView nonNullUTF16(UTF16Array, sizeof(UTF16Array) / sizeof(QChar) - 1); // ghi

    h1.append(n1, nonNullLatin1);
    QCOMPARE(h1.size(), 1);
    QVERIFY(h1.contains(n1));
    QCOMPARE(h1.combinedValue(n1), "abc");

    h1.append(n2, nonNullUTF8);
    QCOMPARE(h1.size(), 2);
    QVERIFY(h1.contains(n2));
    QCOMPARE(h1.combinedValue(n2), "def");

    h1.append(n3, nonNullUTF16);
    QCOMPARE(h1.size(), 3);
    QVERIFY(h1.contains(n3));
    QCOMPARE(h1.combinedValue(n3), "ghi");
}

void tst_QHttpHeaders::valueEncoding()
{
    // Test that common encodings are possible to set and not blocked by
    // header value character filter (ie. don't contain disallowed characters as per RFC 9110)
    QHttpHeaders h1;
    // Within visible ASCII range
    QVERIFY(h1.append(n1, "foo"_ba.toBase64()));
    QCOMPARE(h1.values(n1).at(0), "Zm9v");
    h1.replace(0, n1, "foo"_ba.toPercentEncoding());
    QCOMPARE(h1.values(n1).at(0), "foo");

    // Outside of ASCII/Latin-1 range (€)
    h1.replace(0, n1, "foo€"_ba.toBase64());
    QCOMPARE(h1.values(n1).at(0), "Zm9v4oKs");
    h1.replace(0, n1, "foo€"_ba.toPercentEncoding());
    QCOMPARE(h1.values(n1).at(0), "foo%E2%82%AC");
}

void tst_QHttpHeaders::replaceOrAppend()
{
    QHttpHeaders h1;

#define REPLACE_OR_APPEND(NAME, VALUE, INDEX, TOTALSIZE) \
    do {                                                 \
        QVERIFY(h1.replaceOrAppend(NAME, VALUE));        \
        QCOMPARE(h1.size(), TOTALSIZE);                  \
        QCOMPARE(h1.nameAt(INDEX), NAME);                \
        QCOMPARE(h1.valueAt(INDEX), VALUE);              \
    } while (false)

    // Append to empty container and replace it
    REPLACE_OR_APPEND(n1, v1, 0, 1); // Appends
    REPLACE_OR_APPEND(n1, v2, 0, 1); // Replaces

    // Replace at beginning, middle, and end
    h1.clear();
    REPLACE_OR_APPEND(n1, v1, 0, 1); // Appends
    REPLACE_OR_APPEND(n2, v2, 1, 2); // Appends
    REPLACE_OR_APPEND(n3, v3, 2, 3); // Appends
    REPLACE_OR_APPEND(n1, V1, 0, 3); // Replaces at beginning
    REPLACE_OR_APPEND(n2, V2, 1, 3); // Replaces at middle
    REPLACE_OR_APPEND(n3, V3, 2, 3); // Replaces at end

    // Pre-existing multiple values (n2) are removed
    h1.clear();
    h1.append(n1, v1);
    h1.append(n2, v2); // First n2 is at index 1
    h1.append(n2, v2);
    h1.append(n3, v3);
    h1.append(n2, v2);
    QCOMPARE(h1.size(), 5);
    QCOMPARE(h1.combinedValue(n2), "value2, value2, value2");
    REPLACE_OR_APPEND(n2, V2, 1, 3); // Replaces value at index 1, and removes the rest
    QCOMPARE(h1.combinedValue(n2), "VALUE2");
#undef REPLACE_OR_APPEND

    // Implicit sharing / detaching
    h1.clear();
    h1.append(n1, v1);
    QHttpHeaders h2 = h1;
    QCOMPARE(h1.size(), h2.size());
    QCOMPARE(h1.valueAt(0), h2.valueAt(0)); // Iniially values are equal
    h1.replaceOrAppend(n1, v2);  // Change value in h1 => detaches h1
    QCOMPARE_NE(h1.valueAt(0), h2.valueAt(0)); // Values are no more equal
    QCOMPARE(h1.valueAt(0), v2); // Value in h1 changed
    QCOMPARE(h2.valueAt(0), v1); // Value in h2 remained

    // Failed attempts
    h1.clear();
    h1.append(n1, v1);
    QRegularExpression re("HTTP header*");
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, re);
    QVERIFY(!h1.replaceOrAppend("", V1));
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, re);
    QVERIFY(!h1.replaceOrAppend(v1, "foo\x08"));
}

void tst_QHttpHeaders::intValues()
{
    QHttpHeaders h1;
    h1.append("Content-Length", "12345");
    h1.append(QHttpHeaders::WellKnownHeader::ContentLength, "67890");

    std::optional<qint64> intValueString = h1.intValue("content-length");
    QCOMPARE(intValueString, 12345);

    std::optional<qint64> intValueWKH =
            h1.intValue(QHttpHeaders::WellKnownHeader::ContentLength);
    QCOMPARE(intValueWKH, 12345);

    std::optional<QList<qint64>> intStringValuesList = h1.intValues("content-length");
    QVERIFY(intStringValuesList);
    QCOMPARE(intStringValuesList->size(), 2);
    QCOMPARE(intStringValuesList->at(0), 12345);
    QCOMPARE(intStringValuesList->at(1), 67890);

    std::optional<QList<qint64>> intWKHValuesList =
            h1.intValues(QHttpHeaders::WellKnownHeader::ContentLength);
    QVERIFY(intWKHValuesList);
    QCOMPARE(intWKHValuesList->size(), 2);
    QCOMPARE(intWKHValuesList->at(0), 12345);
    QCOMPARE(intWKHValuesList->at(1), 67890);

    std::optional<qint64> intValueAtIndex = h1.intValueAt(1);
    QCOMPARE(intValueAtIndex, 67890);

    h1.clear();
    h1.append("Content-Length", "Invalid Number");
    h1.append("Content-Length", "");
    QCOMPARE(h1.intValueAt(0), std::nullopt);
    QCOMPARE(h1.intValue("content-length"), std::nullopt);
    QCOMPARE(h1.intValue("non-existing-header"), std::nullopt);
    QCOMPARE(h1.intValues("content-length"), std::nullopt);
}

void tst_QHttpHeaders::dateTimeValues()
{
    QHttpHeaders h1;
    h1.append("Date", "Tue, 25 Feb 2025 10:10:10 GMT");
    h1.append(QHttpHeaders::WellKnownHeader::Date, "Mon, 24 Feb 2025 11:11:11 GMT");

    std::optional<QDateTime> dateTimeString = h1.dateTimeValue("date");
    QVERIFY(dateTimeString);
    QCOMPARE(dateTimeString->date(), QDate(2025, 2, 25));
    QCOMPARE(dateTimeString->time(), QTime(10, 10, 10, 0));
    std::optional<QDateTime> dateTimeWKH =
            h1.dateTimeValue(QHttpHeaders::WellKnownHeader::Date);
    QVERIFY(dateTimeWKH);
    QCOMPARE(dateTimeWKH->date(), QDate(2025, 2, 25));
    QCOMPARE(dateTimeWKH->time(), QTime(10, 10, 10, 0));

    std::optional<QList<QDateTime>> dateTimeStringValueList = h1.dateTimeValues("date");
    QVERIFY(dateTimeStringValueList);
    QCOMPARE(dateTimeStringValueList->size(), 2);
    QCOMPARE(dateTimeStringValueList->at(0).date(), QDate(2025, 2, 25));
    QCOMPARE(dateTimeStringValueList->at(0).time(), QTime(10, 10, 10, 0));
    QCOMPARE(dateTimeStringValueList->at(1).date(), QDate(2025, 2, 24));
    QCOMPARE(dateTimeStringValueList->at(1).time(), QTime(11, 11, 11, 0));

    std::optional<QList<QDateTime>> dateTimeWHKValueList =
            h1.dateTimeValues(QHttpHeaders::WellKnownHeader::Date);
    QVERIFY(dateTimeWHKValueList);
    QCOMPARE(dateTimeWHKValueList->size(), 2);
    QCOMPARE(dateTimeWHKValueList->at(0).date(), QDate(2025, 2, 25));
    QCOMPARE(dateTimeWHKValueList->at(0).time(), QTime(10, 10, 10, 0));
    QCOMPARE(dateTimeWHKValueList->at(1).date(), QDate(2025, 2, 24));
    QCOMPARE(dateTimeWHKValueList->at(1).time(), QTime(11, 11, 11, 0));

    std::optional<QDateTime> dateTimeValueAtIndex = h1.dateTimeValueAt(1);
    QVERIFY(dateTimeValueAtIndex);
    QCOMPARE(dateTimeValueAtIndex->date(), QDate(2025, 2, 24));
    QCOMPARE(dateTimeValueAtIndex->time(), QTime(11, 11, 11, 0));

    QDateTime dateTimeValue3{QDate{2049, 4, 3}, QTime{12, 30, 00, 0}};
    dateTimeValue3.setTimeZone(QTimeZone::UTC);
    h1.setDateTimeValue("date", dateTimeValue3);
    std::optional<QDateTime> setDateTimeValue = h1.dateTimeValue("date");
    QVERIFY(setDateTimeValue);
    QCOMPARE(setDateTimeValue->date(), QDate(2049, 4, 3));
    QCOMPARE(setDateTimeValue->time(), QTime(12, 30, 00, 0));

    h1.clear();
    h1.append("Date", "InvalidDateFormat");
    h1.append("Date", "");
    QCOMPARE(h1.dateTimeValueAt(0), std::nullopt);
    QCOMPARE(h1.dateTimeValue("date"), std::nullopt);
    QCOMPARE(h1.dateTimeValue("non-existing-header"), std::nullopt);
    QCOMPARE(h1.dateTimeValues("date"), std::nullopt);
}

void tst_QHttpHeaders::rangeValues()
{
    QHttpHeaders h1;
    bool ok = false;

    // Test set range
    QList<QHttpHeaderRange> ranges = {
        QHttpHeaderRange(0, 499),
        QHttpHeaderRange(500, std::nullopt),
        QHttpHeaderRange(std::nullopt, 100)
    };
    h1.setRangeValues(ranges);
    QCOMPARE(h1.value(QHttpHeaders::WellKnownHeader::Range), "bytes=0-499, 500-, -100");

    // Parse range
    auto parsedRanges = h1.rangeValues(&ok);
    QCOMPARE(ok, true);
    QCOMPARE(parsedRanges.size(), 3);
    QCOMPARE(*parsedRanges.at(0).start(), 0);
    QCOMPARE(*parsedRanges.at(0).end(), 499);
    QCOMPARE(*parsedRanges.at(1).start(), 500);
    QVERIFY(!parsedRanges.at(1).end().has_value());
    QVERIFY(!parsedRanges.at(2).start().has_value());
    QCOMPARE(*parsedRanges.at(2).end(), 100);

    // Use empty range to clear (also not count as failure)
    h1.setRangeValues({});
    QVERIFY(!h1.contains(QHttpHeaders::WellKnownHeader::Range));
    QVERIFY(h1.rangeValues(&ok).isEmpty());
    QCOMPARE(ok, true);

    // Test parsing invalid range
    h1.clear();
    h1.append(QHttpHeaders::WellKnownHeader::Range, "bytes= 0-100 , invalid, 500-");
    auto complexRanges = h1.rangeValues(&ok);
    QCOMPARE(ok, false);
    QVERIFY(complexRanges.isEmpty());

    h1.clear();
    h1.append(QHttpHeaders::WellKnownHeader::Range, "bytes=100");
    h1.rangeValues(&ok);
    QCOMPARE(ok, false);

    // Test unknown range unit (should ignore instead of failure)
    h1.clear();
    h1.append(QHttpHeaders::WellKnownHeader::Range, "items=0-10");
    auto itemRanges = h1.rangeValues(&ok);
    QCOMPARE(ok, true);
    QVERIFY(itemRanges.isEmpty());

    // Test multiple Range header
    h1.clear();
    h1.append(QHttpHeaders::WellKnownHeader::Range, "bytes=-100");
    h1.append(QHttpHeaders::WellKnownHeader::Range, "bytes=114-514, 2000-");

    auto multiHeaderRanges = h1.rangeValues(&ok);
    QCOMPARE(ok, true);
    QCOMPARE(multiHeaderRanges.size(), 3);
    QVERIFY(!multiHeaderRanges.at(0).start().has_value());
    QCOMPARE(*multiHeaderRanges.at(0).end(), 100);
    QCOMPARE(*multiHeaderRanges.at(1).start(), 114);
    QCOMPARE(*multiHeaderRanges.at(1).end(), 514);
    QCOMPARE(*multiHeaderRanges.at(2).start(), 2000);
    QVERIFY(!multiHeaderRanges.at(2).end().has_value());

    // Multiple Range headers with one of them should be skipped
    h1.clear();
    h1.append(QHttpHeaders::WellKnownHeader::Range, "bytes=0-499");
    h1.append(QHttpHeaders::WellKnownHeader::Range, "unknown-unit=1-2");
    h1.append(QHttpHeaders::WellKnownHeader::Range, "bytes=1000-");

    auto mixedHeaderRanges = h1.rangeValues(&ok);
    QCOMPARE(ok, true);
    QCOMPARE(mixedHeaderRanges.size(), 2);
    QCOMPARE(*mixedHeaderRanges.at(0).start(), 0);
    QCOMPARE(*mixedHeaderRanges.at(0).end(), 499);
    QCOMPARE(*mixedHeaderRanges.at(1).start(), 1000);
    QVERIFY(!mixedHeaderRanges.at(1).end().has_value());

    // Test multiple Range header with invalid one
    h1.clear();
    h1.append(QHttpHeaders::WellKnownHeader::Range, "bytes=0-499");
    h1.append(QHttpHeaders::WellKnownHeader::Range, "bytes=bad-format");

    h1.rangeValues(&ok);
    QCOMPARE(ok, false);

    // Test isValid() method
    QHttpHeaderRange valid1(0, 499);
    QHttpHeaderRange valid2(500, std::nullopt);
    QHttpHeaderRange valid3(std::nullopt, 100);
    QHttpHeaderRange invalid1(std::nullopt, std::nullopt);
    QHttpHeaderRange invalid2(500, 400);
    QHttpHeaderRange invalid3(-100, std::nullopt);
    QHttpHeaderRange invalid4(std::nullopt, -100);
    QHttpHeaderRange invalid5(-100, -1);

    QVERIFY(valid1.isValid());
    QVERIFY(valid2.isValid());
    QVERIFY(valid3.isValid());
    QVERIFY(!invalid1.isValid());
    QVERIFY(!invalid2.isValid());
    QVERIFY(!invalid3.isValid());
    QVERIFY(!invalid4.isValid());
    QVERIFY(!invalid5.isValid());

    // Test setRangeValue with QSpan
    h1.clear();
    QList<QHttpHeaderRange> rangeList = {QHttpHeaderRange(0, 99), QHttpHeaderRange(200, std::nullopt)};
    h1.setRangeValues(rangeList);
    QCOMPARE(h1.value(QHttpHeaders::WellKnownHeader::Range), "bytes=0-99, 200-");
}

void tst_QHttpHeaders::data_stream_data()
{
    QHttpHeaders h1;
    h1.append(n1, v1);
    h1.append(n1, v2);
    h1.append(n2, v3);

    QHttpHeaders h2;
    h2.append(n1, v1);
    h2.removeAll(n1); // makes it empty, but not default-constructed

    QHttpHeaders h3;

    QTest::addColumn<QHttpHeaders>("http_headers");

    QTest::newRow("non-empty") << h1;
    QTest::newRow("empty") << h2;
    QTest::newRow("null") << h3;
}

void tst_QHttpHeaders::data_stream()
{
    QFETCH(const QHttpHeaders, http_headers);

    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    {
        QDataStream stream(&buffer);
        stream << http_headers;
        QCOMPARE_EQ(stream.status(), QDataStream::Status::Ok);
    }

    buffer.seek(0);

    {
        QDataStream stream(&buffer);
        QHttpHeaders h2;
        stream >> h2;
        QCOMPARE_EQ(stream.status(), QDataStream::Status::Ok);
        QCOMPARE(http_headers.toListOfPairs(), h2.toListOfPairs());
    }
}

void tst_QHttpHeaders::data_stream_invalid_data()
{
    QList<std::pair<QByteArray, QByteArray>> raw;
    raw.append(std::pair<QByteArray, QByteArray>("Invalid:Header", "value"));

    QTest::addColumn<QList<std::pair<QByteArray, QByteArray>>>("raw");
    QTest::addColumn<QDataStream::Status>("stream_out_status");

    QTest::newRow("invalid-char-colon") << raw << QDataStream::Status::ReadCorruptData;
}

void tst_QHttpHeaders::data_stream_invalid()
{
    using RawHeaders = QList<std::pair<QByteArray, QByteArray>>;

    QFETCH(const RawHeaders, raw);
    QFETCH(const QDataStream::Status, stream_out_status);

    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);

    {
        QDataStream out(&buffer);

        out << raw;
        QCOMPARE(out.status(), QDataStream::Status::Ok);
    }

    buffer.seek(0);

    {
        QDataStream in(&buffer);

        QHttpHeaders headers;
        headers.append(n1, v1);
        in >> headers;
        QCOMPARE(in.status(), stream_out_status);
        QVERIFY(headers.isEmpty());
    }
}

QTEST_MAIN(tst_QHttpHeaders)
#include "tst_qhttpheaders.moc"
