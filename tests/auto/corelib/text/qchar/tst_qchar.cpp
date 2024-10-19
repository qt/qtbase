// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <qchar.h>
#include <qfile.h>
#include <qstringlist.h>

#include <type_traits>

template <typename C>
constexpr inline bool implicitly = std::is_convertible_v<C, QChar>;
template <typename C>
constexpr inline bool explicitly = std::is_constructible_v<QChar, C>;
template <typename C>
constexpr inline bool disabled = !explicitly<C>;

//
// Conversion from character types
//
#ifdef QT_NO_CAST_FROM_ASCII
static_assert(disabled<char>);
#else
static_assert(implicitly<char>);
#endif
#ifdef __cpp_char8_t
static_assert(disabled<char8_t>); // ### should this be enabled?
#endif
static_assert(implicitly<char16_t>);
#ifdef Q_OS_WIN
static_assert(implicitly<wchar_t>);
#else
static_assert(explicitly<wchar_t>);
#endif
static_assert(explicitly<char32_t>);

//
// Conversion from others
//
#if defined(QT_RESTRICTED_CAST_FROM_ASCII) || defined(QT_NO_CAST_FROM_ASCII)
static_assert(disabled<uchar>);
#else
static_assert(explicitly<uchar>);
#endif
static_assert(implicitly<short>);
static_assert(implicitly<ushort>);
static_assert(explicitly<int>);
static_assert(explicitly<uint>);

//
// Disabled conversions (from Qt 6.9)
//
static_assert(disabled<bool>);
static_assert(disabled<std::byte>);
static_assert(disabled<signed char>);
static_assert(disabled<long>);
static_assert(disabled<long long>);
static_assert(disabled<unsigned long>);
static_assert(disabled<unsigned long long>);
static_assert(disabled<Qt::Key>);
enum E1 {};
static_assert(disabled<E1>);
enum class E2 {};
static_assert(disabled<E2>);
enum E1C : char16_t {};
static_assert(disabled<E1C>);
enum class E2C : char16_t {};
static_assert(disabled<E2C>);

class tst_QChar : public QObject
{
    Q_OBJECT
private slots:
    void fromChar16_t();
    void fromUcs4_data();
    void fromUcs4();
    void fromWchar_t();
    void operator_eqeq_null();
    void operators_data();
    void operators();
    void qchar_qlatin1char_operators_symmetry_data();
    void qchar_qlatin1char_operators_symmetry();
    void toUpper();
    void toLower();
    void toTitle();
    void toCaseFolded();
    void isDigit_data();
    void isDigit();
    void isLetter_data();
    void isLetter();
    void isLetterOrNumber_data();
    void isLetterOrNumber();
    void isPrint();
    void isUpper();
    void isLower();
    void isTitleCase();
    void isSpace_data();
    void isSpace();
    void isSpaceSpecial();
    void category();
    void direction();
    void joiningType();
    void combiningClass();
    void digitValue();
    void mirroredChar();
    void decomposition();
    void script();
#if !defined(Q_OS_WASM)
    void normalization_data();
    void normalization();
#endif // !defined(Q_OS_WASM)
    void normalization_manual();
    void normalizationCorrections();
    void unicodeVersion();
};

void tst_QChar::fromChar16_t()
{
    QChar aUmlaut = u'\u00E4'; // German small letter a-umlaut
    QCOMPARE(aUmlaut, QChar(0xE4));
    QChar replacementCharacter = u'\uFFFD';
    QCOMPARE(replacementCharacter, QChar(QChar::ReplacementCharacter));
}

void tst_QChar::fromUcs4_data()
{
    QTest::addColumn<uint>("ucs4");
    auto row = [](uint ucs4) {
        QTest::addRow("0x%08X", ucs4) << ucs4;
    };

    row(0x2f868); // a CJK Compatibility Ideograph
    row(0x11139); // Chakma digit 3
    row(0x1D157); // Musical Symbol Void Notehead
}

void tst_QChar::fromUcs4()
{
    QFETCH(const uint, ucs4);

    const auto result = QChar::fromUcs4(ucs4);
    if (QChar::requiresSurrogates(ucs4)) {
        QCOMPARE(result.chars[0], QChar::highSurrogate(ucs4));
        QCOMPARE(result.chars[1], QChar::lowSurrogate(ucs4));
        QCOMPARE(QStringView{result}.size(), 2);
    } else {
        QCOMPARE(result.chars[0], ucs4);
        QCOMPARE(result.chars[1], 0u);
        QCOMPARE(QStringView{result}.size(), 1);
    }
}

void tst_QChar::fromWchar_t()
{
#if defined(Q_OS_WIN)
    QChar aUmlaut(L'\u00E4'); // German small letter a-umlaut
    QCOMPARE(aUmlaut, QChar(0xE4));
    QChar replacementCharacter(L'\uFFFD');
    QCOMPARE(replacementCharacter, QChar(QChar::ReplacementCharacter));
#else
    QSKIP("This is a Windows-only test.");
#endif
}

void tst_QChar::operator_eqeq_null()
{
    {
        const QChar ch = QLatin1Char(' ');
#define CHECK(NUL) \
        do { \
            QVERIFY(!(ch  == NUL)); \
            QVERIFY(  ch  != NUL ); \
            QVERIFY(!(ch  <  NUL)); \
            QVERIFY(  ch  >  NUL ); \
            QVERIFY(!(ch  <= NUL)); \
            QVERIFY(  ch  >= NUL ); \
            QVERIFY(!(NUL == ch )); \
            QVERIFY(  NUL != ch  ); \
            QVERIFY(  NUL <  ch  ); \
            QVERIFY(!(NUL >  ch )); \
            QVERIFY(  NUL <= ch  ); \
            QVERIFY(!(NUL >= ch )); \
        } while (0)

        CHECK(0);
#ifndef QT_NO_CAST_FROM_ASCII
        CHECK('\0');
#endif
        CHECK(u'\0');
#undef CHECK
    }
    {
        const QChar ch = QLatin1Char('\0');
#define CHECK(NUL) \
        do { \
            QVERIFY(  ch  == NUL ); \
            QVERIFY(!(ch  != NUL)); \
            QVERIFY(!(ch  <  NUL)); \
            QVERIFY(!(ch  >  NUL)); \
            QVERIFY(  ch  <= NUL ); \
            QVERIFY(  ch  >= NUL ); \
            QVERIFY(  NUL == ch  ); \
            QVERIFY(!(NUL != ch )); \
            QVERIFY(!(NUL <  ch )); \
            QVERIFY(!(NUL >  ch )); \
            QVERIFY(  NUL <= ch  ); \
            QVERIFY(  NUL >= ch  ); \
        } while (0)

        CHECK(0);
#ifndef QT_NO_CAST_FROM_ASCII
        CHECK('\0');
#endif
        CHECK(u'\0');
#undef CHECK
    }
}

void tst_QChar::operators_data()
{
    QTest::addColumn<QChar>("lhs");
    QTest::addColumn<QChar>("rhs");

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            QTest::addRow("'\\%d' (op) '\\%d'", i, j)
                    << QChar(ushort(i)) << QChar(ushort(j));
    }
}

void tst_QChar::operators()
{
    QFETCH(QChar, lhs);
    QFETCH(QChar, rhs);

#define CHECK(op) QCOMPARE((lhs op rhs), (lhs.unicode() op rhs.unicode()))
    CHECK(==);
    CHECK(!=);
    CHECK(< );
    CHECK(> );
    CHECK(<=);
    CHECK(>=);
#undef CHECK
}

void tst_QChar::qchar_qlatin1char_operators_symmetry_data()
{
    QTest::addColumn<char>("lhs");
    QTest::addColumn<char>("rhs");

    const uchar values[] = {0x00, 0x3a, 0x7f, 0x80, 0xab, 0xff};

    for (uchar i : values) {
        for (uchar j : values)
            QTest::addRow("'\\x%02x'_op_'\\x%02x'", i, j) << char(i) << char(j);
    }
}

void tst_QChar::qchar_qlatin1char_operators_symmetry()
{
    QFETCH(char, lhs);
    QFETCH(char, rhs);

    const QLatin1Char l1lhs(lhs);
    const QLatin1Char l1rhs(rhs);
#define CHECK(op) QCOMPARE((l1lhs op l1rhs), (QChar(l1lhs) op QChar(l1rhs)))
    CHECK(==);
    CHECK(!=);
    CHECK(< );
    CHECK(> );
    CHECK(<=);
    CHECK(>=);
#undef CHECK
}

void tst_QChar::toUpper()
{
    QVERIFY(QChar(u'a').toUpper() == u'A');
    QVERIFY(QChar(u'A').toUpper() == u'A');
    QVERIFY(QChar(0x1c7).toUpper().unicode() == 0x1c7);
    QVERIFY(QChar(0x1c8).toUpper().unicode() == 0x1c7);
    QVERIFY(QChar(0x1c9).toUpper().unicode() == 0x1c7);
    QVERIFY(QChar(0x25c).toUpper().unicode() == 0xa7ab);
    QVERIFY(QChar(0x29e).toUpper().unicode() == 0xa7b0);
    QVERIFY(QChar(0x1d79).toUpper().unicode() == 0xa77d);
    QVERIFY(QChar(0x0265).toUpper().unicode() == 0xa78d);

    QVERIFY(QChar::toUpper(u'a') == u'A');
    QVERIFY(QChar::toUpper(u'A') == u'A');
    QVERIFY(QChar::toUpper(0xdf) == 0xdf); // german sharp s
    QVERIFY(QChar::toUpper(0x1c7) == 0x1c7);
    QVERIFY(QChar::toUpper(0x1c8) == 0x1c7);
    QVERIFY(QChar::toUpper(0x1c9) == 0x1c7);
    QVERIFY(QChar::toUpper(0x25c) == 0xa7ab);
    QVERIFY(QChar::toUpper(0x29e) == 0xa7b0);
    QVERIFY(QChar::toUpper(0x1d79) == 0xa77d);
    QVERIFY(QChar::toUpper(0x0265) == 0xa78d);

    QVERIFY(QChar::toUpper(0x10400) == 0x10400);
    QVERIFY(QChar::toUpper(0x10428) == 0x10400);
}

void tst_QChar::toLower()
{
    QVERIFY(QChar(u'A').toLower() == u'a');
    QVERIFY(QChar(u'a').toLower() == u'a');
    QVERIFY(QChar(0x1c7).toLower().unicode() == 0x1c9);
    QVERIFY(QChar(0x1c8).toLower().unicode() == 0x1c9);
    QVERIFY(QChar(0x1c9).toLower().unicode() == 0x1c9);
    QVERIFY(QChar(0xa77d).toLower().unicode() == 0x1d79);
    QVERIFY(QChar(0xa78d).toLower().unicode() == 0x0265);
    QVERIFY(QChar(0xa7ab).toLower().unicode() == 0x25c);
    QVERIFY(QChar(0xa7b1).toLower().unicode() == 0x287);

    QVERIFY(QChar::toLower(u'a') == u'a');
    QVERIFY(QChar::toLower(u'A') == u'a');
    QVERIFY(QChar::toLower(0x1c7) == 0x1c9);
    QVERIFY(QChar::toLower(0x1c8) == 0x1c9);
    QVERIFY(QChar::toLower(0x1c9) == 0x1c9);
    QVERIFY(QChar::toLower(0xa77d) == 0x1d79);
    QVERIFY(QChar::toLower(0xa78d) == 0x0265);
    QVERIFY(QChar::toLower(0xa7ab) == 0x25c);
    QVERIFY(QChar::toLower(0xa7b1) == 0x287);

    QVERIFY(QChar::toLower(0x10400) == 0x10428);
    QVERIFY(QChar::toLower(0x10428) == 0x10428);
}

void tst_QChar::toTitle()
{
    QVERIFY(QChar(u'a').toTitleCase() == u'A');
    QVERIFY(QChar(u'A').toTitleCase() == u'A');
    QVERIFY(QChar(0x1c7).toTitleCase().unicode() == 0x1c8);
    QVERIFY(QChar(0x1c8).toTitleCase().unicode() == 0x1c8);
    QVERIFY(QChar(0x1c9).toTitleCase().unicode() == 0x1c8);
    QVERIFY(QChar(0x1d79).toTitleCase().unicode() == 0xa77d);
    QVERIFY(QChar(0x0265).toTitleCase().unicode() == 0xa78d);

    QVERIFY(QChar::toTitleCase(u'a') == u'A');
    QVERIFY(QChar::toTitleCase(u'A') == u'A');
    QVERIFY(QChar::toTitleCase(0xdf) == 0xdf); // german sharp s
    QVERIFY(QChar::toTitleCase(0x1c7) == 0x1c8);
    QVERIFY(QChar::toTitleCase(0x1c8) == 0x1c8);
    QVERIFY(QChar::toTitleCase(0x1c9) == 0x1c8);
    QVERIFY(QChar::toTitleCase(0x1d79) == 0xa77d);
    QVERIFY(QChar::toTitleCase(0x0265) == 0xa78d);

    QVERIFY(QChar::toTitleCase(0x10400) == 0x10400);
    QVERIFY(QChar::toTitleCase(0x10428) == 0x10400);
}

void tst_QChar::toCaseFolded()
{
    QVERIFY(QChar(u'a').toCaseFolded() == u'a');
    QVERIFY(QChar(u'A').toCaseFolded() == u'a');
    QVERIFY(QChar(0x1c7).toCaseFolded().unicode() == 0x1c9);
    QVERIFY(QChar(0x1c8).toCaseFolded().unicode() == 0x1c9);
    QVERIFY(QChar(0x1c9).toCaseFolded().unicode() == 0x1c9);
    QVERIFY(QChar(0xa77d).toCaseFolded().unicode() == 0x1d79);
    QVERIFY(QChar(0xa78d).toCaseFolded().unicode() == 0x0265);
    QVERIFY(QChar(0xa7ab).toCaseFolded().unicode() == 0x25c);
    QVERIFY(QChar(0xa7b1).toCaseFolded().unicode() == 0x287);

    QVERIFY(QChar::toCaseFolded(u'a') == u'a');
    QVERIFY(QChar::toCaseFolded(u'A') == u'a');
    QVERIFY(QChar::toCaseFolded(0x1c7) == 0x1c9);
    QVERIFY(QChar::toCaseFolded(0x1c8) == 0x1c9);
    QVERIFY(QChar::toCaseFolded(0x1c9) == 0x1c9);
    QVERIFY(QChar::toCaseFolded(0xa77d) == 0x1d79);
    QVERIFY(QChar::toCaseFolded(0xa78d) == 0x0265);
    QVERIFY(QChar::toCaseFolded(0xa7ab) == 0x25c);
    QVERIFY(QChar::toCaseFolded(0xa7b1) == 0x287);

    QVERIFY(QChar::toCaseFolded(0x10400) == 0x10428);
    QVERIFY(QChar::toCaseFolded(0x10428) == 0x10428);

    QVERIFY(QChar::toCaseFolded(0xb5) == 0x3bc);
}

void tst_QChar::isDigit_data()
{
    QTest::addColumn<ushort>("ucs");
    QTest::addColumn<bool>("expected");

    for (ushort ucs = 0; ucs < 256; ++ucs) {
        bool isDigit = (ucs <= '9' && ucs >= '0');
        const QByteArray tag = "0x" + QByteArray::number(ucs, 16);
        QTest::newRow(tag.constData()) << ucs << isDigit;
    }
}

void tst_QChar::isDigit()
{
    QFETCH(ushort, ucs);
    QFETCH(bool, expected);
    QCOMPARE(QChar(ucs).isDigit(), expected);
}

static bool isExpectedLetter(ushort ucs)
{
    return (ucs >= 'a' && ucs <= 'z') || (ucs >= 'A' && ucs <= 'Z')
            || ucs == 0xAA || ucs == 0xB5 || ucs == 0xBA
            || (ucs >= 0xC0 && ucs <= 0xD6)
            || (ucs >= 0xD8 && ucs <= 0xF6)
            || (ucs >= 0xF8 && ucs <= 0xFF);
}

void tst_QChar::isLetter_data()
{
    QTest::addColumn<ushort>("ucs");
    QTest::addColumn<bool>("expected");

    for (ushort ucs = 0; ucs < 256; ++ucs) {
        const QByteArray tag = "0x" + QByteArray::number(ucs, 16);
        QTest::newRow(tag.constData()) << ucs << isExpectedLetter(ucs);
    }
}

void tst_QChar::isLetter()
{
    QFETCH(ushort, ucs);
    QFETCH(bool, expected);
    QCOMPARE(QChar(ucs).isLetter(), expected);
}

void tst_QChar::isLetterOrNumber_data()
{
    QTest::addColumn<ushort>("ucs");
    QTest::addColumn<bool>("expected");

    for (ushort ucs = 0; ucs < 256; ++ucs) {
        bool isLetterOrNumber = isExpectedLetter(ucs)
                || (ucs >= '0' && ucs <= '9')
                || ucs == 0xB2 || ucs == 0xB3 || ucs == 0xB9
                || (ucs >= 0xBC && ucs <= 0xBE);
        const QByteArray tag = "0x" + QByteArray::number(ucs, 16);
        QTest::newRow(tag.constData()) << ucs << isLetterOrNumber;
    }
}

void tst_QChar::isLetterOrNumber()
{
    QFETCH(ushort, ucs);
    QFETCH(bool, expected);
    QCOMPARE(QChar(ucs).isLetterOrNumber(), expected);
}

void tst_QChar::isPrint()
{
    // noncharacters, reserved (General_Gategory =Cn)
    QVERIFY(!QChar(0x2064).isPrint());
    QVERIFY(!QChar(0x2069).isPrint());
    QVERIFY(!QChar(0xfdd0).isPrint());
    QVERIFY(!QChar(0xfdef).isPrint());
    QVERIFY(!QChar(0xfff0).isPrint());
    QVERIFY(!QChar(0xfff8).isPrint());
    QVERIFY(!QChar(0xfffe).isPrint());
    QVERIFY(!QChar(0xffff).isPrint());
    QVERIFY(!QChar::isPrint(0xe0000));
    QVERIFY(!QChar::isPrint(0xe0002));
    QVERIFY(!QChar::isPrint(0xe001f));
    QVERIFY(!QChar::isPrint(0xe0080));
    QVERIFY(!QChar::isPrint(0xe00ff));

    // Other_Default_Ignorable_Code_Point, Variation_Selector
    QVERIFY(QChar(0x034f).isPrint());
    QVERIFY(QChar(0x115f).isPrint());
    QVERIFY(QChar(0x180b).isPrint());
    QVERIFY(QChar(0x180d).isPrint());
    QVERIFY(QChar(0x3164).isPrint());
    QVERIFY(QChar(0xfe00).isPrint());
    QVERIFY(QChar(0xfe0f).isPrint());
    QVERIFY(QChar(0xffa0).isPrint());
    QVERIFY(QChar::isPrint(0xe0100));
    QVERIFY(QChar::isPrint(0xe01ef));

    // Cf, Cs, Cc, White_Space, Annotation Characters
    QVERIFY(!QChar(0x0008).isPrint());
    QVERIFY(!QChar(0x000a).isPrint());
    QVERIFY(QChar(0x0020).isPrint());
    QVERIFY(QChar(0x00a0).isPrint());
    QVERIFY(!QChar(0x00ad).isPrint());
    QVERIFY(!QChar(0x0085).isPrint());
    QVERIFY(!QChar(0xd800).isPrint());
    QVERIFY(!QChar(0xdc00).isPrint());
    QVERIFY(!QChar(0xfeff).isPrint());
    QVERIFY(!QChar::isPrint(0x1d173));

    QVERIFY(QChar(u'0').isPrint());
    QVERIFY(QChar(u'A').isPrint());
    QVERIFY(QChar(u'a').isPrint());

    QVERIFY(QChar(0x0370).isPrint()); // assigned in 5.1
    QVERIFY(QChar(0x0524).isPrint()); // assigned in 5.2
    QVERIFY(QChar(0x0526).isPrint()); // assigned in 6.0
    QVERIFY(QChar(0x08a0).isPrint()); // assigned in 6.1
    QVERIFY(!QChar(0x1aff).isPrint()); // not assigned
    QVERIFY(QChar(0x1e9e).isPrint()); // assigned in 5.1
    QVERIFY(QChar::isPrint(0x1b000)); // assigned in 6.0
    QVERIFY(QChar::isPrint(0x110d0)); // assigned in 5.1
    QVERIFY(!QChar::isPrint(0x1bca0)); // assigned in 7.0
}

void tst_QChar::isUpper()
{
    QVERIFY(QChar(u'A').isUpper());
    QVERIFY(QChar(u'Z').isUpper());
    QVERIFY(!QChar(u'a').isUpper());
    QVERIFY(!QChar(u'z').isUpper());
    QVERIFY(!QChar(u'?').isUpper());
    QVERIFY(QChar(0xC2).isUpper());   // A with ^
    QVERIFY(!QChar(0xE2).isUpper());  // a with ^

    for (uint codepoint = 0; codepoint <= QChar::LastValidCodePoint; ++codepoint) {
        if (QChar::isUpper(codepoint))
            QVERIFY(codepoint == QChar::toUpper(codepoint));
    }
}

void tst_QChar::isLower()
{
    QVERIFY(!QChar(u'A').isLower());
    QVERIFY(!QChar(u'Z').isLower());
    QVERIFY(QChar(u'a').isLower());
    QVERIFY(QChar(u'z').isLower());
    QVERIFY(!QChar(u'?').isLower());
    QVERIFY(!QChar(0xC2).isLower());   // A with ^
    QVERIFY(QChar(0xE2).isLower());  // a with ^

    for (uint codepoint = 0; codepoint <= QChar::LastValidCodePoint; ++codepoint) {
        if (QChar::isLower(codepoint))
            QVERIFY(codepoint == QChar::toLower(codepoint));
    }
}

void tst_QChar::isTitleCase()
{
    for (uint codepoint = 0; codepoint <= QChar::LastValidCodePoint; ++codepoint) {
        if (QChar::isTitleCase(codepoint))
            QVERIFY(codepoint == QChar::toTitleCase(codepoint));
    }
}

void tst_QChar::isSpace_data()
{
    QTest::addColumn<ushort>("ucs");
    QTest::addColumn<bool>("expected");

    for (ushort ucs = 0; ucs < 256; ++ucs) {
        bool isSpace = (ucs <= 0x0D && ucs >= 0x09) || ucs == 0x20 || ucs == 0xA0 || ucs == 0x85;
        const QByteArray tag = "0x" + QByteArray::number(ucs, 16);
        QTest::newRow(tag.constData()) << ucs << isSpace;
    }
}

void tst_QChar::isSpace()
{
    QFETCH(ushort, ucs);
    QFETCH(bool, expected);
    QCOMPARE(QChar(ucs).isSpace(), expected);
}

void tst_QChar::isSpaceSpecial()
{
    QVERIFY(!QChar(QChar::Null).isSpace());
    QVERIFY(QChar(QChar::Nbsp).isSpace());
    QVERIFY(QChar(QChar::ParagraphSeparator).isSpace());
    QVERIFY(QChar(QChar::LineSeparator).isSpace());
    QVERIFY(QChar(0x1680).isSpace());
}

void tst_QChar::category()
{
    QVERIFY(QChar(u'a').category() == QChar::Letter_Lowercase);
    QVERIFY(QChar(u'A').category() == QChar::Letter_Uppercase);

    QVERIFY(QChar::category('a') == QChar::Letter_Lowercase);
    QVERIFY(QChar::category('A') == QChar::Letter_Uppercase);

    QVERIFY(QChar::category(0xe0100) == QChar::Mark_NonSpacing);
    QVERIFY(QChar::category(0xeffff) != QChar::Other_PrivateUse);
    QVERIFY(QChar::category(0xf0000) == QChar::Other_PrivateUse);
    QVERIFY(QChar::category(0xf0001) == QChar::Other_PrivateUse);

    QVERIFY(QChar::category(0xd900) == QChar::Other_Surrogate);
    QVERIFY(QChar::category(0xdc00) == QChar::Other_Surrogate);
    QVERIFY(QChar::category(0xdc01) == QChar::Other_Surrogate);

    QVERIFY(QChar::category(0x1aff) == QChar::Other_NotAssigned);
    QVERIFY(QChar::category(0x10fffd) == QChar::Other_PrivateUse);
    QVERIFY(QChar::category(0x10ffff) == QChar::Other_NotAssigned);
    QVERIFY(QChar::category(0x110000) == QChar::Other_NotAssigned);
}

void tst_QChar::direction()
{
    QVERIFY(QChar::direction(0x200E) == QChar::DirL);
    QVERIFY(QChar::direction(0x200F) == QChar::DirR);
    QVERIFY(QChar::direction(0x202A) == QChar::DirLRE);
    QVERIFY(QChar::direction(0x202B) == QChar::DirRLE);
    QVERIFY(QChar::direction(0x202C) == QChar::DirPDF);
    QVERIFY(QChar::direction(0x202D) == QChar::DirLRO);
    QVERIFY(QChar::direction(0x202E) == QChar::DirRLO);
    QVERIFY(QChar::direction(0x2066) == QChar::DirLRI);
    QVERIFY(QChar::direction(0x2067) == QChar::DirRLI);
    QVERIFY(QChar::direction(0x2068) == QChar::DirFSI);
    QVERIFY(QChar::direction(0x2069) == QChar::DirPDI);

    QVERIFY(QChar(u'a').direction() == QChar::DirL);
    QVERIFY(QChar(u'0').direction() == QChar::DirEN);
    QVERIFY(QChar(0x627).direction() == QChar::DirAL);
    QVERIFY(QChar(0x5d0).direction() == QChar::DirR);

    QVERIFY(QChar::direction('a') == QChar::DirL);
    QVERIFY(QChar::direction('0') == QChar::DirEN);
    QVERIFY(QChar::direction(0x627) == QChar::DirAL);
    QVERIFY(QChar::direction(0x5d0) == QChar::DirR);

    QVERIFY(QChar::direction(0xE01DA) == QChar::DirNSM);
    QVERIFY(QChar::direction(0xf0000) == QChar::DirL);
    QVERIFY(QChar::direction(0xE0030) == QChar::DirBN);
    QVERIFY(QChar::direction(0x2FA17) == QChar::DirL);
}

void tst_QChar::joiningType()
{
    QVERIFY(QChar(u'a').joiningType() == QChar::Joining_None);
    QVERIFY(QChar(u'0').joiningType() == QChar::Joining_None);
    QVERIFY(QChar(0x0627).joiningType() == QChar::Joining_Right);
    QVERIFY(QChar(0x05d0).joiningType() == QChar::Joining_None);
    QVERIFY(QChar(0x00ad).joiningType() == QChar::Joining_Transparent);
    QVERIFY(QChar(0xA872).joiningType() == QChar::Joining_Left);

    QVERIFY(QChar::joiningType('a') == QChar::Joining_None);
    QVERIFY(QChar::joiningType('0') == QChar::Joining_None);
    QVERIFY(QChar::joiningType(0x0627) == QChar::Joining_Right);
    QVERIFY(QChar::joiningType(0x05d0) == QChar::Joining_None);
    QVERIFY(QChar::joiningType(0x00ad) == QChar::Joining_Transparent);

    QVERIFY(QChar::joiningType(0xE01DA) == QChar::Joining_Transparent);
    QVERIFY(QChar::joiningType(0xf0000) == QChar::Joining_None);
    QVERIFY(QChar::joiningType(0xE0030) == QChar::Joining_Transparent);
    QVERIFY(QChar::joiningType(0x2FA17) == QChar::Joining_None);

    QVERIFY(QChar::joiningType(0xA872) == QChar::Joining_Left);
    QVERIFY(QChar::joiningType(0x10ACD) == QChar::Joining_Left);
    QVERIFY(QChar::joiningType(0x10AD7) == QChar::Joining_Left);
}

void tst_QChar::combiningClass()
{
    QVERIFY(QChar(u'a').combiningClass() == 0);
    QVERIFY(QChar(u'0').combiningClass() == 0);
    QVERIFY(QChar(0x627).combiningClass() == 0);
    QVERIFY(QChar(0x5d0).combiningClass() == 0);

    QVERIFY(QChar::combiningClass('a') == 0);
    QVERIFY(QChar::combiningClass('0') == 0);
    QVERIFY(QChar::combiningClass(0x627) == 0);
    QVERIFY(QChar::combiningClass(0x5d0) == 0);

    QVERIFY(QChar::combiningClass(0xE01DA) == 0);
    QVERIFY(QChar::combiningClass(0xf0000) == 0);
    QVERIFY(QChar::combiningClass(0xE0030) == 0);
    QVERIFY(QChar::combiningClass(0x2FA17) == 0);

    QVERIFY(QChar::combiningClass(0x300) == 230);

    QVERIFY(QChar::combiningClass(0x1d244) == 230);

}

void tst_QChar::unicodeVersion()
{
    QVERIFY(QChar(u'a').unicodeVersion() == QChar::Unicode_1_1);
    QVERIFY(QChar(u'0').unicodeVersion() == QChar::Unicode_1_1);
    QVERIFY(QChar(0x627).unicodeVersion() == QChar::Unicode_1_1);
    QVERIFY(QChar(0x5d0).unicodeVersion() == QChar::Unicode_1_1);

    QVERIFY(QChar::unicodeVersion('a') == QChar::Unicode_1_1);
    QVERIFY(QChar::unicodeVersion('0') == QChar::Unicode_1_1);
    QVERIFY(QChar::unicodeVersion(0x627) == QChar::Unicode_1_1);
    QVERIFY(QChar::unicodeVersion(0x5d0) == QChar::Unicode_1_1);

    QVERIFY(QChar(0x0591).unicodeVersion() == QChar::Unicode_2_0);
    QVERIFY(QChar::unicodeVersion(0x0591) == QChar::Unicode_2_0);

    QVERIFY(QChar(0x20AC).unicodeVersion() == QChar::Unicode_2_1_2);
    QVERIFY(QChar::unicodeVersion(0x20AC) == QChar::Unicode_2_1_2);
    QVERIFY(QChar(0xfffc).unicodeVersion() == QChar::Unicode_2_1_2);
    QVERIFY(QChar::unicodeVersion(0xfffc) == QChar::Unicode_2_1_2);

    QVERIFY(QChar(0x01f6).unicodeVersion() == QChar::Unicode_3_0);
    QVERIFY(QChar::unicodeVersion(0x01f6) == QChar::Unicode_3_0);

    QVERIFY(QChar(0x03F4).unicodeVersion() == QChar::Unicode_3_1);
    QVERIFY(QChar::unicodeVersion(0x03F4) == QChar::Unicode_3_1);
    QVERIFY(QChar::unicodeVersion(0x10300) == QChar::Unicode_3_1);

    QVERIFY(QChar(0x0220).unicodeVersion() == QChar::Unicode_3_2);
    QVERIFY(QChar::unicodeVersion(0x0220) == QChar::Unicode_3_2);
    QVERIFY(QChar::unicodeVersion(0xFF5F) == QChar::Unicode_3_2);

    QVERIFY(QChar(0x0221).unicodeVersion() == QChar::Unicode_4_0);
    QVERIFY(QChar::unicodeVersion(0x0221) == QChar::Unicode_4_0);
    QVERIFY(QChar::unicodeVersion(0x10000) == QChar::Unicode_4_0);

    QVERIFY(QChar(0x0237).unicodeVersion() == QChar::Unicode_4_1);
    QVERIFY(QChar::unicodeVersion(0x0237) == QChar::Unicode_4_1);
    QVERIFY(QChar::unicodeVersion(0x10140) == QChar::Unicode_4_1);

    QVERIFY(QChar(0x0242).unicodeVersion() == QChar::Unicode_5_0);
    QVERIFY(QChar::unicodeVersion(0x0242) == QChar::Unicode_5_0);
    QVERIFY(QChar::unicodeVersion(0x12000) == QChar::Unicode_5_0);

    QVERIFY(QChar(0x0370).unicodeVersion() == QChar::Unicode_5_1);
    QVERIFY(QChar::unicodeVersion(0x0370) == QChar::Unicode_5_1);
    QVERIFY(QChar::unicodeVersion(0x1f093) == QChar::Unicode_5_1);

    QVERIFY(QChar(0x0524).unicodeVersion() == QChar::Unicode_5_2);
    QVERIFY(QChar::unicodeVersion(0x0524) == QChar::Unicode_5_2);
    QVERIFY(QChar::unicodeVersion(0x2b734) == QChar::Unicode_5_2);

    QVERIFY(QChar(0x26ce).unicodeVersion() == QChar::Unicode_6_0);
    QVERIFY(QChar::unicodeVersion(0x26ce) == QChar::Unicode_6_0);
    QVERIFY(QChar::unicodeVersion(0x1f618) == QChar::Unicode_6_0);

    QVERIFY(QChar(0xa69f).unicodeVersion() == QChar::Unicode_6_1);
    QVERIFY(QChar::unicodeVersion(0xa69f) == QChar::Unicode_6_1);
    QVERIFY(QChar::unicodeVersion(0x1f600) == QChar::Unicode_6_1);

    QVERIFY(QChar(0x20ba).unicodeVersion() == QChar::Unicode_6_2);
    QVERIFY(QChar::unicodeVersion(0x20ba) == QChar::Unicode_6_2);

    QVERIFY(QChar(0x061c).unicodeVersion() == QChar::Unicode_6_3);
    QVERIFY(QChar::unicodeVersion(0x061c) == QChar::Unicode_6_3);

    QVERIFY(QChar(0x20bd).unicodeVersion() == QChar::Unicode_7_0);
    QVERIFY(QChar::unicodeVersion(0x20bd) == QChar::Unicode_7_0);
    QVERIFY(QChar::unicodeVersion(0x16b00) == QChar::Unicode_7_0);

    QVERIFY(QChar(0x08b3).unicodeVersion() == QChar::Unicode_8_0);
    QVERIFY(QChar::unicodeVersion(0x08b3) == QChar::Unicode_8_0);
    QVERIFY(QChar::unicodeVersion(0x108e0) == QChar::Unicode_8_0);

    QVERIFY(QChar(0x09ff).unicodeVersion() == QChar::Unicode_Unassigned);
    QVERIFY(QChar::unicodeVersion(0x09ff) == QChar::Unicode_Unassigned);
    QVERIFY(QChar::unicodeVersion(0x110000) == QChar::Unicode_Unassigned);
}

void tst_QChar::digitValue()
{
    QVERIFY(QChar(u'9').digitValue() == 9);
    QVERIFY(QChar(u'0').digitValue() == 0);
    QVERIFY(QChar(u'a').digitValue() == -1);

    QVERIFY(QChar::digitValue('9') == 9);
    QVERIFY(QChar::digitValue('0') == 0);

    QVERIFY(QChar::digitValue(0x1049) == 9);
    QVERIFY(QChar::digitValue(0x1040) == 0);

    QVERIFY(QChar::digitValue(0xd800) == -1);
    QVERIFY(QChar::digitValue(0x110000) == -1);
}

void tst_QChar::mirroredChar()
{
    QVERIFY(QChar(0x169B).hasMirrored());
    QVERIFY(QChar(0x169B).mirroredChar() == QChar(0x169C));
    QVERIFY(QChar(0x169C).hasMirrored());
    QVERIFY(QChar(0x169C).mirroredChar() == QChar(0x169B));

    QVERIFY(QChar(0x301A).hasMirrored());
    QVERIFY(QChar(0x301A).mirroredChar() == QChar(0x301B));
    QVERIFY(QChar(0x301B).hasMirrored());
    QVERIFY(QChar(0x301B).mirroredChar() == QChar(0x301A));
}

void tst_QChar::decomposition()
{
    // Hangul syllables
    for (uint ucs = 0xac00; ucs <= 0xd7af; ++ucs) {
        QChar::Decomposition expected = QChar::unicodeVersion(ucs) > QChar::Unicode_Unassigned ? QChar::Canonical : QChar::NoDecomposition;
        QString desc = QString::fromLatin1("ucs = 0x%1, tag = %2, expected = %3")
                .arg(QString::number(ucs, 16)).arg(QChar::decompositionTag(ucs)).arg(expected);
        QVERIFY2(QChar::decompositionTag(ucs) == expected, desc.toLatin1());
    }

    QVERIFY(QChar(0xa0).decompositionTag() == QChar::NoBreak);
    QVERIFY(QChar(0xa8).decompositionTag() == QChar::Compat);
    QVERIFY(QChar(0x41).decompositionTag() == QChar::NoDecomposition);

    QVERIFY(QChar::decompositionTag(0xa0) == QChar::NoBreak);
    QVERIFY(QChar::decompositionTag(0xa8) == QChar::Compat);
    QVERIFY(QChar::decompositionTag(0x41) == QChar::NoDecomposition);

    QVERIFY(QChar::decomposition(0xa0) == QString(QChar(0x20)));
    QVERIFY(QChar::decomposition(0xc0) == (QString(QChar(0x41)) + QString(QChar(0x300))));

    {
        QString str;
        str += QChar(QChar::highSurrogate(0x1D157));
        str += QChar(QChar::lowSurrogate(0x1D157));
        str += QChar(QChar::highSurrogate(0x1D165));
        str += QChar(QChar::lowSurrogate(0x1D165));
        QVERIFY(QChar::decomposition(0x1D15e) == str);
    }

    {
        QString str;
        str += QChar(0x1100);
        str += QChar(0x1161);
        QVERIFY(QChar::decomposition(0xac00) == str);
    }
    {
        QString str;
        str += QChar(0x110c);
        str += QChar(0x1165);
        str += QChar(0x11b7);
        QVERIFY(QChar::decomposition(0xc810) == str);
    }
}

void tst_QChar::script()
{
    QVERIFY(QChar::script(0x0020) == QChar::Script_Common);
    QVERIFY(QChar::script(0x0041) == QChar::Script_Latin);
    QVERIFY(QChar::script(0x0375) == QChar::Script_Greek);
    QVERIFY(QChar::script(0x0400) == QChar::Script_Cyrillic);
    QVERIFY(QChar::script(0x0531) == QChar::Script_Armenian);
    QVERIFY(QChar::script(0x0591) == QChar::Script_Hebrew);
    QVERIFY(QChar::script(0x0600) == QChar::Script_Arabic);
    QVERIFY(QChar::script(0x0700) == QChar::Script_Syriac);
    QVERIFY(QChar::script(0x0780) == QChar::Script_Thaana);
    QVERIFY(QChar::script(0x07c0) == QChar::Script_Nko);
    QVERIFY(QChar::script(0x0900) == QChar::Script_Devanagari);
    QVERIFY(QChar::script(0x0981) == QChar::Script_Bengali);
    QVERIFY(QChar::script(0x0a01) == QChar::Script_Gurmukhi);
    QVERIFY(QChar::script(0x0a81) == QChar::Script_Gujarati);
    QVERIFY(QChar::script(0x0b01) == QChar::Script_Oriya);
    QVERIFY(QChar::script(0x0b82) == QChar::Script_Tamil);
    QVERIFY(QChar::script(0x0c01) == QChar::Script_Telugu);
    QVERIFY(QChar::script(0x0c82) == QChar::Script_Kannada);
    QVERIFY(QChar::script(0x0d02) == QChar::Script_Malayalam);
    QVERIFY(QChar::script(0x0d82) == QChar::Script_Sinhala);
    QVERIFY(QChar::script(0x0e01) == QChar::Script_Thai);
    QVERIFY(QChar::script(0x0e81) == QChar::Script_Lao);
    QVERIFY(QChar::script(0x0f00) == QChar::Script_Tibetan);
    QVERIFY(QChar::script(0x1000) == QChar::Script_Myanmar);
    QVERIFY(QChar::script(0x10a0) == QChar::Script_Georgian);
    QVERIFY(QChar::script(0x1100) == QChar::Script_Hangul);
    QVERIFY(QChar::script(0x1680) == QChar::Script_Ogham);
    QVERIFY(QChar::script(0x16a0) == QChar::Script_Runic);
    QVERIFY(QChar::script(0x1780) == QChar::Script_Khmer);
    QVERIFY(QChar::script(0x200c) == QChar::Script_Inherited);
    QVERIFY(QChar::script(0x200d) == QChar::Script_Inherited);
    QVERIFY(QChar::script(0x1018a) == QChar::Script_Greek);
    QVERIFY(QChar::script(0x1f130) == QChar::Script_Common);
    QVERIFY(QChar::script(0xe0100) == QChar::Script_Inherited);
}

// wasm is limited in reading filesystems, so omit this test for now
#if !defined(Q_OS_WASM)
void tst_QChar::normalization_data()
{
    QTest::addColumn<QStringList>("columns");
    QTest::addColumn<int>("part");

    int linenum = 0;
    int part = 0;

    QString testFile = QFINDTESTDATA("data/NormalizationTest.txt");
    QVERIFY2(!testFile.isEmpty(), "data/NormalizationTest.txt not found!");
    QFile f(testFile);
    QVERIFY(f.open(QIODevice::ReadOnly));

    while (!f.atEnd()) {
        linenum++;

        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);

        if (line.startsWith('@')) {
            if (line.startsWith("@Part") && line.size() > 5 && QChar::isDigit(line.at(5)))
                part = QChar::digitValue(line.at(5));
            continue;
        }

        if (line.isEmpty())
            continue;

        line = line.trimmed();
        if (line.endsWith(';'))
            line.truncate(line.size()-1);

        QList<QByteArray> l = line.split(';');

        QCOMPARE(l.size(), 5);

        QStringList columns;
        for (int i = 0; i < 5; ++i) {
            columns.append(QString());

            QList<QByteArray> c = l.at(i).split(' ');
            QVERIFY(!c.isEmpty());

            for (int j = 0; j < c.size(); ++j) {
                bool ok;
                uint uc = c.at(j).toInt(&ok, 16);
                columns[i].append(QChar::fromUcs4(uc));
            }
        }


        const QByteArray nm = "line #" + QByteArray::number(linenum) + " (part "
            + QByteArray::number(part);
        QTest::newRow(nm.constData()) << columns << part;
    }
}

void tst_QChar::normalization()
{
    QFETCH(QStringList, columns);
    QFETCH(int, part);

    Q_UNUSED(part);

        // CONFORMANCE:
        // 1. The following invariants must be true for all conformant implementations
        //
        //    NFC
        //      c2 ==  NFC(c1) ==  NFC(c2) ==  NFC(c3)
        //      c4 ==  NFC(c4) ==  NFC(c5)

        QVERIFY(columns[1] == columns[0].normalized(QString::NormalizationForm_C));
        QVERIFY(columns[1] == columns[1].normalized(QString::NormalizationForm_C));
        QVERIFY(columns[1] == columns[2].normalized(QString::NormalizationForm_C));
        QVERIFY(columns[3] == columns[3].normalized(QString::NormalizationForm_C));
        QVERIFY(columns[3] == columns[4].normalized(QString::NormalizationForm_C));

        //    NFD
        //      c3 ==  NFD(c1) ==  NFD(c2) ==  NFD(c3)
        //      c5 ==  NFD(c4) ==  NFD(c5)

        QVERIFY(columns[2] == columns[0].normalized(QString::NormalizationForm_D));
        QVERIFY(columns[2] == columns[1].normalized(QString::NormalizationForm_D));
        QVERIFY(columns[2] == columns[2].normalized(QString::NormalizationForm_D));
        QVERIFY(columns[4] == columns[3].normalized(QString::NormalizationForm_D));
        QVERIFY(columns[4] == columns[4].normalized(QString::NormalizationForm_D));

        //    NFKC
        //      c4 == NFKC(c1) == NFKC(c2) == NFKC(c3) == NFKC(c4) == NFKC(c5)

        QVERIFY(columns[3] == columns[0].normalized(QString::NormalizationForm_KC));
        QVERIFY(columns[3] == columns[1].normalized(QString::NormalizationForm_KC));
        QVERIFY(columns[3] == columns[2].normalized(QString::NormalizationForm_KC));
        QVERIFY(columns[3] == columns[3].normalized(QString::NormalizationForm_KC));
        QVERIFY(columns[3] == columns[4].normalized(QString::NormalizationForm_KC));

        //    NFKD
        //      c5 == NFKD(c1) == NFKD(c2) == NFKD(c3) == NFKD(c4) == NFKD(c5)

        QVERIFY(columns[4] == columns[0].normalized(QString::NormalizationForm_KD));
        QVERIFY(columns[4] == columns[1].normalized(QString::NormalizationForm_KD));
        QVERIFY(columns[4] == columns[2].normalized(QString::NormalizationForm_KD));
        QVERIFY(columns[4] == columns[3].normalized(QString::NormalizationForm_KD));
        QVERIFY(columns[4] == columns[4].normalized(QString::NormalizationForm_KD));

        // 2. For every code point X assigned in this version of Unicode that is not specifically
        //    listed in Part 1, the following invariants must be true for all conformant
        //    implementations:
        //
        //      X == NFC(X) == NFD(X) == NFKC(X) == NFKD(X)

        // #################

}
#endif // !defined(Q_OS_WASM)

void tst_QChar::normalization_manual()
{
    {
        QString decomposed;
        decomposed += QChar(0x41);
        decomposed += QChar(0x0221); // assigned in 4.0
        decomposed += QChar(0x300);

        QVERIFY(decomposed.normalized(QString::NormalizationForm_C, QChar::Unicode_3_2) == decomposed);

        decomposed[1] = QChar(0x037f); // unassigned in 6.1

        QVERIFY(decomposed.normalized(QString::NormalizationForm_C) == decomposed);
    }
    {
        QString composed;
        composed += QChar(0xc0);
        QString decomposed;
        decomposed += QChar(0x41);
        decomposed += QChar(0x300);

        QVERIFY(composed.normalized(QString::NormalizationForm_D) == decomposed);
        QVERIFY(composed.normalized(QString::NormalizationForm_C) == composed);
        QVERIFY(composed.normalized(QString::NormalizationForm_KD) == decomposed);
        QVERIFY(composed.normalized(QString::NormalizationForm_KC) == composed);
    }
    {
        QString composed;
        composed += QChar(0xa0);
        QString decomposed;
        decomposed += QChar(0x20);

        QVERIFY(composed.normalized(QString::NormalizationForm_D) == composed);
        QVERIFY(composed.normalized(QString::NormalizationForm_C) == composed);
        QVERIFY(composed.normalized(QString::NormalizationForm_KD) == decomposed);
        QVERIFY(composed.normalized(QString::NormalizationForm_KC) == decomposed);
    }
    {
        QString composed;
        composed += QChar(0x0061);
        composed += QChar(0x00f2);
        QString decomposed;
        decomposed += QChar(0x0061);
        decomposed += QChar(0x006f);
        decomposed += QChar(0x0300);

        QVERIFY(decomposed.normalized(QString::NormalizationForm_D) == decomposed);
        QVERIFY(decomposed.normalized(QString::NormalizationForm_C) == composed);
        QVERIFY(decomposed.normalized(QString::NormalizationForm_KD) == decomposed);
        QVERIFY(decomposed.normalized(QString::NormalizationForm_KC) == composed);
    }
    {   // hangul
        QString composed;
        composed += QChar(0xc154);
        composed += QChar(0x11f0);
        QString decomposed;
        decomposed += QChar(0x1109);
        decomposed += QChar(0x1167);
        decomposed += QChar(0x11f0);

        QVERIFY(composed.normalized(QString::NormalizationForm_D) == decomposed);
        QVERIFY(composed.normalized(QString::NormalizationForm_C) == composed);
        QVERIFY(composed.normalized(QString::NormalizationForm_KD) == decomposed);
        QVERIFY(composed.normalized(QString::NormalizationForm_KC) == composed);

        QVERIFY(decomposed.normalized(QString::NormalizationForm_D) == decomposed);
        QVERIFY(decomposed.normalized(QString::NormalizationForm_C) == composed);
        QVERIFY(decomposed.normalized(QString::NormalizationForm_KD) == decomposed);
        QVERIFY(decomposed.normalized(QString::NormalizationForm_KC) == composed);
    }
    // QTBUG-71894 - erratum fixed in Unicode 4.1.0; SCount bounds are < not <=
    {
        // Hangul compose, test 0x11a7:
        const QChar c[] = { QChar(0xae30), QChar(0x11a7), {} };
        const QChar d[] = { QChar(0x1100), QChar(0x1175), QChar(0x11a7), {} };
        const QString composed(c, 2);
        const QString decomposed(d, 3);

        QCOMPARE(decomposed.normalized(QString::NormalizationForm_C), composed);
    }
    {
        // Hangul compose, test 0x11c3:
        const QChar c[] = { QChar(0xae30), QChar(0x11c3), {} };
        const QChar d[] = { QChar(0x1100), QChar(0x1175), QChar(0x11c3), {} };
        const QString composed(c, 2);
        const QString decomposed(d, 3);

        QCOMPARE(decomposed.normalized(QString::NormalizationForm_C), composed);
    }
}

void tst_QChar::normalizationCorrections()
{
    QString s;
    s.append(QChar(0xf951));

    QString n = s.normalized(QString::NormalizationForm_D);
    QString res;
    res.append(QChar(0x964b));
    QCOMPARE(n, res);

    n = s.normalized(QString::NormalizationForm_D, QChar::Unicode_3_1);
    res.clear();
    res.append(QChar(0x96fb));
    QCOMPARE(n, res);

    s.clear();
    s += QChar(QChar::highSurrogate(0x2f868));
    s += QChar(QChar::lowSurrogate(0x2f868));

    n = s.normalized(QString::NormalizationForm_C);
    res.clear();
    res += QChar(0x36fc);
    QCOMPARE(n, res);

    n = s.normalized(QString::NormalizationForm_C, QChar::Unicode_3_1);
    res.clear();
    res += QChar(0xd844);
    res += QChar(0xdf6a);
    QCOMPARE(n, res);

    n = s.normalized(QString::NormalizationForm_C, QChar::Unicode_3_2);
    QCOMPARE(n, res);
}


QTEST_APPLESS_MAIN(tst_QChar)
#include "tst_qchar.moc"
