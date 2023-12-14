// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtNetwork/qhttpheaders.h>

#include <QtTest/qtest.h>

#include <QtCore/qset.h>

using namespace Qt::StringLiterals;

class tst_QHttpHeaders : public QObject
{
    Q_OBJECT

private slots:
    void comparison();
    void constructors();
    void accessors();
    void headerNameField();
    void headerValueField();
    void valueEncoding();

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

void tst_QHttpHeaders::comparison()
{
    // Basic comparisons
    QHttpHeaders h1;
    QHttpHeaders h2;
    QVERIFY(h1.equals(h2)); // empties
    h1.append(n1, v1);
    QVERIFY(h1.equals(h1)); // self
    h2.append(n1, v1);
    QVERIFY(h1.equals(h2));
    h1.append(n2, v2);
    QVERIFY(!h1.equals(h2));
    h1.removeAll(n2);
    QVERIFY(h1.equals(h2));

    // 'name' case-insensitivity and 'value' case-sensitivity
    h1.removeAll(n1);
    QVERIFY(h1.isEmpty());
    h1.append(N1, v1);
    QVERIFY(h1.equals(h2));
    h1.removeAll(n1);
    QVERIFY(h1.isEmpty());
    h1.append(n1, V1);
    QVERIFY(!h1.equals(h2));

    // Order-insensitivity
    h1.clear();
    h2.clear();
    QVERIFY(h1.isEmpty() && h2.isEmpty());
    // Same headers but in different order
    h1.append(n1, v1);
    h1.append(n2, v2);
    h2.append(n2, v2);
    h2.append(n1, v1);
    QVERIFY(h1.equals(h2));
    // Add header with different name casing
    h1.insert(0, n1, v1);
    h2.append(N1, v1);
    QVERIFY(h1.equals(h2));
    // Add header with different value casing
    h1.insert(0, n1, v1);
    h2.append(n1, V1);
    QVERIFY(!h1.equals(h2));

    // Order-sensitivity
    h1.clear();
    h2.clear();
    QVERIFY(h1.equals(h2, QHttpHeaders::CompareOption::OrderSensitive));
    // Same headers but in different order
    h1.append(n1, v1);
    h1.append(n2, v2);
    h2.append(n2, v2);
    h2.append(n1, v1);
    QVERIFY(!h1.equals(h2, QHttpHeaders::CompareOption::OrderSensitive));

    // Different number of headers
    h1.clear();
    h2.clear();
    h1.append(n1, v1);
    h2.append(n1, v1);
    h2.append(n2, v2);
    QVERIFY(!h1.equals(h2));
    QVERIFY(!h1.equals(h2, QHttpHeaders::CompareOption::OrderSensitive));
}

void tst_QHttpHeaders::constructors()
{
    // Default ctor
    QHttpHeaders h1;
    QVERIFY(h1.isEmpty());

    // Copy ctor
    QHttpHeaders h2(h1);
    QVERIFY(h2.equals(h1));

    // Copy assignment
    QHttpHeaders h3;
    h3 = h1;
    QVERIFY(h3.equals(h1));

    // Move assignment
    QHttpHeaders h4;
    h4 = std::move(h2);
    QVERIFY(h4.equals(h1));

    // Move ctor
    QHttpHeaders h5(std::move(h4));
    QVERIFY(h5.equals(h1));

    // Constructors that are counterparts to 'toXXX()' conversion getters
    const QByteArray nb1{"name1"};
    const QByteArray nb2{"name2"};
    const QByteArray nv1{"value1"};
    const QByteArray nv2{"value2"};
    // Initialize three QHttpHeaders with same content and verify they match
    QList<std::pair<QByteArray, QByteArray>> list{{nb1, nv1}, {nb2, nv2}, {nb2, nv2}};
    QMultiMap<QByteArray, QByteArray> map{{nb1, nv1}, {nb2, nv2}, {nb2, nv2}};
    QMultiHash<QByteArray, QByteArray> hash{{nb1, nv1}, {nb2, nv2}, {nb2, nv2}};
    QHttpHeaders hlist = QHttpHeaders::fromListOfPairs(list);
    QHttpHeaders hmap = QHttpHeaders::fromMultiMap(map);
    QHttpHeaders hhash = QHttpHeaders::fromMultiHash(hash);
    QVERIFY(hlist.has(nb1) && hmap.has(nb1) && hhash.has(nb1));
    QVERIFY(!hlist.has(n3) && !hmap.has(n3) && !hhash.has(n3));
    QVERIFY(hlist.equals(hmap));
    QVERIFY(hmap.equals(hhash));
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

    // has()
    h1.append(n1, v1);
    QVERIFY(h1.has(n1));
    QVERIFY(h1.has(N1));
    QVERIFY(!h1.has(n2));
    QVERIFY(!h1.has(QHttpHeaders::WellKnownHeader::Allow));
    h1.append(QHttpHeaders::WellKnownHeader::Accept, "nothing");
    QVERIFY(h1.has(QHttpHeaders::WellKnownHeader::Accept));
    QVERIFY(h1.has("accept"));

    // values()/value()
#define EXISTS_NOT(H, N) do {                       \
        QVERIFY(!H.has(N));                         \
        QCOMPARE(H.value(N, "ENOENT"), "ENOENT");   \
        const auto values = H.values(N);            \
        QVERIFY(values.isEmpty());                  \
        QVERIFY(H.combinedValue(N).isNull());       \
    } while (false)

#define EXISTS_N_TIMES(X, H, N, ...) do {                           \
        const std::array expected = { __VA_ARGS__ };                \
        static_assert(std::tuple_size_v<decltype(expected)> == X);  \
        QVERIFY(H.has(N));                                          \
        QCOMPARE(H.value(N, "ENOENT"), expected.front());           \
        const auto values = H.values(N);                            \
        QCOMPARE(values.size(), X);                                 \
        QCOMPARE(values.front(), expected.front());                 \
        /* ignore in-between */                                     \
        QCOMPARE(values.back(), expected.back());                   \
        QCOMPARE(H.combinedValue(N), values.join(','));             \
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

    // names()
    h1.clear();
    QVERIFY(h1.names().isEmpty());
    h1.append(n1, v1);
    QCOMPARE(h1.names().size(), 1);
    QCOMPARE(h1.size(), 1);
    QVERIFY(h1.names().contains(n1));
    h1.append(n2, v2);
    QCOMPARE(h1.names().size(), 2);
    QCOMPARE(h1.size(), 2);
    QVERIFY(h1.names().contains(n1));
    QVERIFY(h1.names().contains(n2));
    h1.append(n1, v1);
    h1.append(n1, v1);
    QCOMPARE(h1.size(), 4);
    QCOMPARE(h1.names().size(), 2);
    h1.append(N1, v1); // uppercase of n1
    QCOMPARE(h1.size(), 5);
    QCOMPARE(h1.names().size(), 2);

    // removeAll()
    h1.clear();
    QVERIFY(h1.append(n1, v1));
    QVERIFY(h1.append(QHttpHeaders::WellKnownHeader::Accept, "nothing"));
    QVERIFY(h1.append(n1, v1));
    QCOMPARE(h1.size(), 3);
    h1.removeAll(n1);
    QVERIFY(!h1.has(n1));
    QCOMPARE(h1.size(), 1);
    QVERIFY(h1.has("accept"));
    h1.removeAll(QHttpHeaders::WellKnownHeader::Accept);
    QVERIFY(!h1.has(QHttpHeaders::WellKnownHeader::Accept));

    // removeAt()
    h1.clear();
    h1.append(n1, v1);
    h1.append(n2, v2);
    h1.append(n3, v3);

    // Valid removals
    QVERIFY(h1.has(n3));
    h1.removeAt(2);
    QVERIFY(!h1.has(n3));
    QVERIFY(h1.has(n1));
    h1.removeAt(0);
    QVERIFY(!h1.has(n1));
    QVERIFY(h1.has(n2));
    h1.removeAt(0);
    QVERIFY(!h1.has(n2));
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
    QCOMPARE(list.at(2).first, n3); // N3 has been lower-cased
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
    QCOMPARE(h1.names().size(), 1);
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
    QVERIFY(h1.has(nonNullLatin1));
    QCOMPARE(h1.combinedValue(nonNullLatin1), v1);

    h1.append(nonNullUTF8, v2);
    QCOMPARE(h1.size(), 2);
    QVERIFY(h1.has(nonNullUTF8));
    QCOMPARE(h1.combinedValue(nonNullUTF8), v2);

    h1.append(nonNullUTF16, v3);
    QCOMPARE(h1.size(), 3);
    QVERIFY(h1.has(nonNullUTF16));
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
    QCOMPARE(h1.combinedValue(n1), "foo,bar");
    QCOMPARE(h1.size(), 2);

    h1.clear();
    QRegularExpression re("HTTP header value contained illegal character*");
    TEST_ILLEGAL_HEADER_VALUE_CHARACTER("foo\x08" "bar"); // BS
    TEST_ILLEGAL_HEADER_VALUE_CHARACTER("foo\x1B" "bar"); // ESC
    // Disallowed UTF-8 characters
    TEST_ILLEGAL_HEADER_VALUE_CHARACTER(u8"€");
    TEST_ILLEGAL_HEADER_VALUE_CHARACTER(u8"𝒜𝒴𝟘𝟡𐎀𐎜𐒀𐒐𝓐𝓩𝔸𝔹𝕀𝕁𝕌𝕍𓂀𓂁𓃀𓃁𓇋𓇌𓉐𓉑𓋴𓋵𓎡𓎢𓎣𓏏");
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
    QVERIFY(h1.has(n1));
    QCOMPARE(h1.combinedValue(n1), "abc");

    h1.append(n2, nonNullUTF8);
    QCOMPARE(h1.size(), 2);
    QVERIFY(h1.has(n2));
    QCOMPARE(h1.combinedValue(n2), "def");

    h1.append(n3, nonNullUTF16);
    QCOMPARE(h1.size(), 3);
    QVERIFY(h1.has(n3));
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

QTEST_MAIN(tst_QHttpHeaders)
#include "tst_qhttpheaders.moc"
