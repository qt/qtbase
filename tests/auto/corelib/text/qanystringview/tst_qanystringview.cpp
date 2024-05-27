// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QAnyStringView>
#include <QChar>
#include <QDebug>
#include <QList>
#include <QString>
#include <QStringBuilder>
#include <QVarLengthArray>
#if QT_CONFIG(cpp_winrt)
#  include <private/qt_winrtbase_p.h>
#endif
#include <private/qxmlstream_p.h>

#include <QTest>

#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <algorithm>
#include <memory>
#include <q20iterator.h>

// for negative testing (can't convert from)
#include <deque>
#include <list>

#ifdef __cpp_char8_t
#  define ONLY_IF_CHAR_8_T(expr) expr
#else
#  define ONLY_IF_CHAR_8_T(expr) \
    QSKIP("This test requires C++20 char8_t support enabled in the compiler.")
#endif

#ifdef __cpp_lib_char8_t
#  define ONLY_IF_LIB_CHAR_8_T(expr) expr
#else
#  define ONLY_IF_LIB_CHAR_8_T(expr) \
    QSKIP("This test requires C++20 char8_t support enabled in the standard library.")
#endif

#ifdef Q_OS_WIN
#  define ONLY_WIN(expr) expr
#else
#  define ONLY_WIN(expr) QSKIP("This is a Windows-only test")
#endif

#ifdef __cpp_impl_three_way_comparison
#  define ONLY_3WAY(expr) expr
#else
#  define ONLY_3WAY(expr) \
    QSKIP("This test requires C++20 spaceship operator (<=>) " \
          "support enabled in the standard library.")
#endif

using namespace Qt::StringLiterals;

template <typename T>
constexpr inline bool CanConvert = std::is_convertible_v<T, QAnyStringView>;

static_assert(CanConvert<QLatin1String>);
static_assert(CanConvert<const char*>);
static_assert(CanConvert<QByteArray>);

template <typename T>
struct ImplicitlyConvertibleTo
{
    operator T() const;
};

static_assert(CanConvert<ImplicitlyConvertibleTo<QString>>);
static_assert(CanConvert<ImplicitlyConvertibleTo<QByteArray>>);
static_assert(!CanConvert<ImplicitlyConvertibleTo<QLatin1StringView>>);

// QAnyStringView qchar_does_not_compile() { return QAnyStringView(QChar('a')); }
// QAnyStringView qlatin1string_does_not_compile() { return QAnyStringView(QLatin1String("a")); }
// QAnyStringView const_char_star_does_not_compile() { return QAnyStringView("a"); }
// QAnyStringView qbytearray_does_not_compile() { return QAnyStringView(QByteArray("a")); }

//
// QChar
//

static_assert(CanConvert<QChar>);

static_assert(CanConvert<QChar[123]>);

static_assert(CanConvert<      QString >);
static_assert(CanConvert<const QString >);
static_assert(CanConvert<      QString&>);
static_assert(CanConvert<const QString&>);

//
// ushort
//

static_assert(CanConvert<ushort>);

static_assert(CanConvert<ushort[123]>);

static_assert(CanConvert<      ushort*>);
static_assert(CanConvert<const ushort*>);

static_assert(CanConvert<QList<ushort>>);
static_assert(CanConvert<QVarLengthArray<ushort>>);
static_assert(CanConvert<std::vector<ushort>>);
static_assert(CanConvert<std::array<ushort, 123>>);
static_assert(!CanConvert<std::deque<ushort>>);
static_assert(!CanConvert<std::list<ushort>>);

#ifdef __cpp_char8_t

//
// char8_t
//

static_assert(CanConvert<char8_t>);

static_assert(CanConvert<      char8_t*>);
static_assert(CanConvert<const char8_t*>);

#ifdef __cpp_lib_char8_t

static_assert(CanConvert<      std::u8string >);
static_assert(CanConvert<const std::u8string >);
static_assert(CanConvert<      std::u8string&>);
static_assert(CanConvert<const std::u8string&>);

static_assert(CanConvert<      std::u8string_view >);
static_assert(CanConvert<const std::u8string_view >);
static_assert(CanConvert<      std::u8string_view&>);
static_assert(CanConvert<const std::u8string_view&>);

#endif // __cpp_lib_char8_t

static_assert(CanConvert<QList<char8_t>>);
static_assert(CanConvert<QVarLengthArray<char8_t>>);
static_assert(CanConvert<std::vector<char8_t>>);
static_assert(CanConvert<std::array<char8_t, 123>>);
static_assert(!CanConvert<std::deque<char8_t>>);
static_assert(!CanConvert<std::list<char8_t>>);

#endif // __cpp_char8_t

//
// char16_t
//

static_assert(CanConvert<char16_t>);

static_assert(CanConvert<      char16_t*>);
static_assert(CanConvert<const char16_t*>);

static_assert(CanConvert<      std::u16string >);
static_assert(CanConvert<const std::u16string >);
static_assert(CanConvert<      std::u16string&>);
static_assert(CanConvert<const std::u16string&>);

static_assert(CanConvert<      std::u16string_view >);
static_assert(CanConvert<const std::u16string_view >);
static_assert(CanConvert<      std::u16string_view&>);
static_assert(CanConvert<const std::u16string_view&>);

static_assert(CanConvert<QList<char16_t>>);
static_assert(CanConvert<QVarLengthArray<char16_t>>);
static_assert(CanConvert<std::vector<char16_t>>);
static_assert(CanConvert<std::array<char16_t, 123>>);
static_assert(!CanConvert<std::deque<char16_t>>);
static_assert(!CanConvert<std::list<char16_t>>);

static_assert(CanConvert<QtPrivate::XmlStringRef>);

//
// char32_t
//

// Qt Policy: char32_t isn't supported

static_assert(CanConvert<char32_t>); // ... except here

static_assert(!CanConvert<      char32_t*>);
static_assert(!CanConvert<const char32_t*>);

static_assert(!CanConvert<      std::u32string >);
static_assert(!CanConvert<const std::u32string >);
static_assert(!CanConvert<      std::u32string&>);
static_assert(!CanConvert<const std::u32string&>);

static_assert(!CanConvert<      std::u32string_view >);
static_assert(!CanConvert<const std::u32string_view >);
static_assert(!CanConvert<      std::u32string_view&>);
static_assert(!CanConvert<const std::u32string_view&>);

static_assert(!CanConvert<QList<char32_t>>);
static_assert(!CanConvert<QVarLengthArray<char32_t>>);
static_assert(!CanConvert<std::vector<char32_t>>);
static_assert(!CanConvert<std::array<char32_t, 123>>);
static_assert(!CanConvert<std::deque<char32_t>>);
static_assert(!CanConvert<std::list<char32_t>>);

//
// wchar_t
//

constexpr bool CanConvertFromWCharT =
#ifdef Q_OS_WIN
        true
#else
        false
#endif
        ;

static_assert(CanConvert<wchar_t> == CanConvertFromWCharT); // ### FIXME: should work everywhere

static_assert(CanConvert<      wchar_t*> == CanConvertFromWCharT);
static_assert(CanConvert<const wchar_t*> == CanConvertFromWCharT);

static_assert(CanConvert<      std::wstring > == CanConvertFromWCharT);
static_assert(CanConvert<const std::wstring > == CanConvertFromWCharT);
static_assert(CanConvert<      std::wstring&> == CanConvertFromWCharT);
static_assert(CanConvert<const std::wstring&> == CanConvertFromWCharT);

static_assert(CanConvert<      std::wstring_view > == CanConvertFromWCharT);
static_assert(CanConvert<const std::wstring_view > == CanConvertFromWCharT);
static_assert(CanConvert<      std::wstring_view&> == CanConvertFromWCharT);
static_assert(CanConvert<const std::wstring_view&> == CanConvertFromWCharT);

static_assert(CanConvert<QList<wchar_t>> == CanConvertFromWCharT);
static_assert(CanConvert<QVarLengthArray<wchar_t>> == CanConvertFromWCharT);
static_assert(CanConvert<std::vector<wchar_t>> == CanConvertFromWCharT);
static_assert(CanConvert<std::array<wchar_t, 123>> == CanConvertFromWCharT);
static_assert(!CanConvert<std::deque<wchar_t>>);
static_assert(!CanConvert<std::list<wchar_t>>);

//
// QStringBuilder
//

static_assert(CanConvert<QStringBuilder<QString, QString>>);

#if QT_CONFIG(cpp_winrt)

//
// winrt::hstring (QTBUG-111886)
//

static_assert(CanConvert<      winrt::hstring >);
static_assert(CanConvert<const winrt::hstring >);
static_assert(CanConvert<      winrt::hstring&>);
static_assert(CanConvert<const winrt::hstring&>);

#endif // QT_CONFIG(cpp_winrt)

// In bootstrapped build and in Qt 7+, two lower bits of size() are used as a
// mask, so check that it is handled correctly, and the mask does not break the
// actual size
template <typename Char> struct SampleStrings
{
    static constexpr char emptyString[] = "";
    static constexpr char oneChar[] = "a";
    static constexpr char twoChars[] = "ab";
    static constexpr char threeChars[] = "abc";
    static constexpr char regularString[] = "Hello World!";
    static constexpr char regularLongString[] = R"(Lorem ipsum dolor sit amet, consectetur
adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna
aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi
ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in
voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint
occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim
id est laborum.)";
    static constexpr char stringWithNulls[] = "Hello\0World\0!";
    static constexpr qsizetype stringWithNullsLength = std::size(stringWithNulls) -1;
};

template <> struct SampleStrings<char16_t>
{
    static constexpr char16_t emptyString[] = u"";
    static constexpr char16_t oneChar[] = u"a";
    static constexpr char16_t twoChars[] = u"ab";
    static constexpr char16_t threeChars[] = u"abc";
    static constexpr char16_t regularString[] = u"Hello World!";
    static constexpr char16_t regularLongString[] = uR"(Lorem ipsum dolor sit amet, consectetur
adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna
aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi
ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in
voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint
occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim
id est laborum.)";
    static constexpr char16_t stringWithNulls[] = u"Hello\0World\0!";
    static constexpr qsizetype stringWithNullsLength = std::size(stringWithNulls) -1;
};

template <> struct SampleStrings<QChar>
{
    static constexpr QChar emptyString[] = { {} };  // this one is easy
    static const QChar *const oneChar;
    static const QChar *const twoChars;
    static const QChar *const threeChars;
    static const QChar *const regularString;
    static const QChar *const regularLongString;
    static const QChar *const stringWithNulls;
    static constexpr qsizetype stringWithNullsLength = SampleStrings<char16_t>::stringWithNullsLength;
};
const QChar *const SampleStrings<QChar>::oneChar =
        reinterpret_cast<const QChar *>(SampleStrings<char16_t>::oneChar);
const QChar *const SampleStrings<QChar>::twoChars =
        reinterpret_cast<const QChar *>(SampleStrings<char16_t>::twoChars);
const QChar *const SampleStrings<QChar>::threeChars =
        reinterpret_cast<const QChar *>(SampleStrings<char16_t>::threeChars);
const QChar *const SampleStrings<QChar>::regularString =
        reinterpret_cast<const QChar *>(SampleStrings<char16_t>::regularString);
const QChar *const SampleStrings<QChar>::regularLongString =
        reinterpret_cast<const QChar *>(SampleStrings<char16_t>::regularLongString);
const QChar *const SampleStrings<QChar>::stringWithNulls =
        reinterpret_cast<const QChar *>(SampleStrings<char16_t>::stringWithNulls);

class tst_QAnyStringView : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constExpr() const;
    void basics() const;
    void debug() const;
    void asciiLiteralIsLatin1() const;

    void fromQString() const { fromQStringOrByteArray<QString>(); }
    void fromQByteArray() const { fromQStringOrByteArray<QByteArray>(); }
    void fromQStringView() const { fromQStringOrByteArray<QStringView>(); }
    void fromQUtf8StringView() const { fromQStringOrByteArray<QUtf8StringView>(); }
    void fromQLatin1StringView() const { fromQStringOrByteArray<QLatin1StringView>(); }

    void fromCharArray() const { fromArray<char>(); }
    void fromChar8Array() const { ONLY_IF_CHAR_8_T(fromArray<char8_t>()); }
    void fromChar16Array() const { fromArray<char16_t>(); }
    void fromQCharArray() const { fromArray<QChar>(); }
    void fromWCharTArray() const { ONLY_WIN(fromArray<wchar_t>()); }

    void fromQCharStar() const
    {
        const QChar str[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!', '\0' };
        fromLiteral(str);
    }

    void fromUShortStar() const
    {
        const ushort str[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!', '\0' };
        fromLiteral(str);
    }

    void fromChar8TStar() const
    {
        fromLiteral(u8"Hello, World!"); // char[] in <= C++17, char8_t[] in >= C++20
    }

    void fromChar() const { fromCharacter('\xE4', 1); }
    void fromUChar() const { fromCharacter(static_cast<unsigned char>('\xE4'), 1); }
    void fromSChar() const { fromCharacter(static_cast<signed char>('\xE4'), 1); }
    void fromChar16T() const { fromCharacter(u'ä', 1); }
    void fromUShort() const { fromCharacter(ushort(0xE4), 1); }
    void fromChar32T() const {
        fromCharacter(U'ä', 1);
        if (QTest::currentTestFailed())
            return;
        fromCharacter(U'\x1F0A0', 2); // U+1F0A0: PLAYING CARD BACK
    }
    void fromWCharT() const {
        ONLY_WIN(fromCharacter(L'ä', 1)); // should work on Unix, too (char32_t does)
    }
    void fromQChar() const { fromCharacter(QChar(u'ä'), 1); }
    void fromQLatin1Char() const { fromCharacter(QLatin1Char('\xE4'), 1); }
    void fromQCharSpecialCharacter() const {
        fromCharacter(QChar::ReplacementCharacter, 1);
        if (QTest::currentTestFailed())
            return;
        fromCharacter(QChar::LastValidCodePoint, 1);
    }
    void fromCharacterSpecial() const;

    void fromChar16TStar() const { fromLiteral(u"Hello, World!"); }
    void fromWCharTStar() const { ONLY_WIN(fromLiteral(L"Hello, World!")); }

    void fromCharRange() const { fromRange<char>(); }
    void fromChar8TRange() const { ONLY_IF_CHAR_8_T(fromRange<char8_t>()); }
    void fromQCharRange() const { fromRange<QChar>(); }
    void fromUShortRange() const { fromRange<ushort>(); }
    void fromChar16TRange() const { fromRange<char16_t>(); }
    void fromWCharTRange() const { ONLY_WIN(fromRange<wchar_t>()); }

    // std::basic_string
    void fromStdStringChar() const { fromStdString<char>(); }
    void fromStdStringChar8T() const { ONLY_IF_LIB_CHAR_8_T(fromStdString<char8_t>()); }
    void fromStdStringWCharT() const { ONLY_WIN(fromStdString<wchar_t>()); }
    void fromStdStringChar16T() const { fromStdString<char16_t>(); }

    void fromUShortContainers() const { fromContainers<ushort>(); }
    void fromQCharContainers() const { fromContainers<QChar>(); }
    void fromChar16TContainers() const { fromContainers<char16_t>(); }
    void fromWCharTContainers() const { ONLY_WIN(fromContainers<wchar_t>()); }

    void fromQStringBuilder_QString_QString() const { fromQStringBuilder(u"1"_s % u"2"_s, u"12"); }

    void comparison();
    void compare3way();

private:
    template <typename StringBuilder>
    void fromQStringBuilder(StringBuilder &&sb, QStringView expected) const;
    template <typename Char>
    void fromArray() const;
    template <typename String>
    void conversion_tests(String arg) const;
    template <typename Char>
    void fromLiteral(const Char *arg) const;
    template <typename Char>
    void fromCharacter(Char arg, qsizetype expectedSize) const;
    template <typename Char>
    void fromRange() const;
    template <typename Char, typename Container>
    void fromContainer() const;
    template <typename Char>
    void fromContainers() const;
    template <typename Char>
    void fromStdString() const { fromContainer<Char, std::basic_string<Char> >(); }
    template <typename QStringOrByteArray>
    void fromQStringOrByteArray() const;
};

void tst_QAnyStringView::constExpr() const
{
#define IS_NULL(sv) \
    do { \
        static_assert(sv.size() == 0); \
        static_assert(sv.isNull()); \
        static_assert(sv.empty()); \
        static_assert(sv.isEmpty()); \
        static_assert(sv.data() == nullptr); \
    } while (false) \
    /*end*/
#define IS_EMPTY(sv) \
    do { \
        static_assert(sv.size() == 0); \
        static_assert(!sv.isNull()); \
        static_assert(sv.empty()); \
        static_assert(sv.isEmpty()); \
        static_assert(sv.data() != nullptr); \
    } while (false) \
    /*end*/
#define IS_OF_SIZE(sv, sz) \
    do { \
        static_assert(sv.size() == sz); \
        static_assert(!sv.isNull()); \
        static_assert(!sv.empty()); \
        static_assert(!sv.isEmpty()); \
        static_assert(sv.data() != nullptr); \
    } while (false) \
    /*end*/

    // compile-time checks
    {
        constexpr QAnyStringView sv;
        IS_NULL(sv);
    }
    {
        constexpr const char *nul = nullptr;
        constexpr QAnyStringView sv(nul, 0);
        IS_NULL(sv);
    }
    {
        constexpr const char16_t *nul = nullptr;
        constexpr QAnyStringView sv(nul, 0);
        IS_NULL(sv);
    }
#ifdef __cpp_char8_t
    {
        constexpr const char8_t *nul = nullptr;
        constexpr QAnyStringView sv(nul, 0);
        IS_NULL(sv);
    }
#endif // __cpp_char8_t
    {
        constexpr QAnyStringView sv = nullptr;
        IS_NULL(sv);
    }
    {
        constexpr QAnyStringView sv = "";
        IS_EMPTY(sv);
    }
    {
        constexpr QAnyStringView sv = u8"";
        IS_EMPTY(sv);
    }
    {
        constexpr QAnyStringView sv = u"";
        IS_EMPTY(sv);
    }
    {
        constexpr QAnyStringView sv = u"Hello";
        IS_OF_SIZE(sv, 5);

        constexpr QAnyStringView sv2 = sv;
        IS_OF_SIZE(sv2, 5);
    }
#undef IS_OF_SIZE
#undef IS_EMPTY
#undef IS_NULL
}

void tst_QAnyStringView::basics() const
{
    QAnyStringView sv1;

    // a default-constructed QAnyStringView is null:
    QVERIFY(sv1.isNull());
    // which implies it's empty();
    QVERIFY(sv1.isEmpty());

    QAnyStringView sv2;

    QVERIFY(sv2 == sv1);
    QVERIFY(!(sv2 != sv1));
}

void tst_QAnyStringView::debug() const
{
    #ifdef QT_SUPPORTS_IS_CONSTANT_EVALUATED
    #  define MAYBE_L1(str) str "_L1"
    #  define VERIFY_L1(s) QVERIFY(s.isLatin1())
    #else
    #  define MAYBE_L1(str) "u8" str
    #  define VERIFY_L1(s) QVERIFY(s.isUtf8())
    #endif
    #define CHECK1(s, mod, expected) do { \
            QString result; \
            QDebug(&result) mod << "X"_L1 << s << "Y"_L1; \
            /* QDebug appends an eager ' ', so trim before comparison */ \
            /* We use X and Y affixes so we can still check spacing   */ \
            /* around the QAnyStringView itself.                      */ \
            QCOMPARE(result.trimmed(), expected); \
        } while (false)
    #define CHECK(init, esq, eq, es, e) do { \
            QAnyStringView s = init; \
            CHECK1(s, ,                   esq); \
            CHECK1(s, .nospace(),          eq); \
            CHECK1(s, .noquote(),          es); \
            CHECK1(s, .nospace().noquote(), e); \
        } while (false)

    CHECK(nullptr,
          R"("X" u8"" "Y")",
          R"("X"u8"""Y")",
          R"(X  Y)",
          R"(XY)");
    CHECK(QLatin1StringView(nullptr),
          R"("X" ""_L1 "Y")",
          R"("X"""_L1"Y")",
          R"(X  Y)",
          R"(XY)");
    CHECK(QUtf8StringView(nullptr),
          R"("X" u8"" "Y")",
          R"("X"u8"""Y")",
          R"(X  Y)",
          R"(XY)");
    CHECK(QStringView(nullptr),
          R"("X" u"" "Y")",
          R"("X"u"""Y")",
          R"(X  Y)",
          R"(XY)");
    {
        constexpr QAnyStringView asv = "hello";
        VERIFY_L1(asv); // ### fails when asv isn't constexpr
        CHECK(asv,
              R"("X" )" MAYBE_L1(R"("hello")") R"( "Y")",
              R"("X")" MAYBE_L1(R"("hello")") R"("Y")",
              R"(X hello Y)",
              R"(XhelloY)");
    }
    CHECK(u8"hällo",
          R"("X" u8"h\xC3\xA4llo" "Y")",
          R"("X"u8"h\xC3\xA4llo""Y")",
          R"(X hällo Y)",
          R"(XhälloY)");
    CHECK(u"hällo",
          R"("X" u"hällo" "Y")",
          R"("X"u"hällo""Y")",
          R"(X hällo Y)",
          R"(XhälloY)");

    #undef CHECK
    #undef CHECK1
    #undef VERIFY_L1
    #undef MAYBE_L1
}

void tst_QAnyStringView::asciiLiteralIsLatin1() const
{
    if constexpr (QAnyStringView::detects_US_ASCII_at_compile_time) {
        constexpr bool asciiCstringIsLatin1 = QAnyStringView("Hello, World").isLatin1();
        QVERIFY(asciiCstringIsLatin1);
        constexpr bool asciiUtf8stringIsLatin1 = QAnyStringView(u8"Hello, World").isLatin1();
        QVERIFY(asciiUtf8stringIsLatin1);
        constexpr bool utf8StringIsNotLatin1 = !QAnyStringView(u8"Tørrfisk").isLatin1();
        QVERIFY(utf8StringIsNotLatin1);
        constexpr bool asciiCstringArrayIsLatin1 =
                QAnyStringView::fromArray("Hello, World").isLatin1();
        QVERIFY(asciiCstringArrayIsLatin1);
        constexpr bool asciiUtfstringArrayIsLatin1 =
                QAnyStringView::fromArray(u8"Hello, World").isLatin1();
        QVERIFY(asciiUtfstringArrayIsLatin1);
        constexpr bool utf8StringArrayIsNotLatin1 =
                !QAnyStringView::fromArray(u8"Tørrfisk").isLatin1();
        QVERIFY(utf8StringArrayIsNotLatin1);
    } else {
        QSKIP("Compile-detection of US-ASCII strings not possible with this compiler");
    }
}

void tst_QAnyStringView::fromCharacterSpecial() const
{
    QEXPECT_FAIL("", "QTBUG-125730", Continue);
    // Treating 'ä' as a UTF-8 sequence doesn't make sense, as it would be
    // invalid. And this is not how legacy Qt APIs handled it, either:
    QCOMPARE_NE(QAnyStringView('\xE4').tag(), QAnyStringView::Tag::Utf8);
}

template <typename StringBuilder>
void tst_QAnyStringView::fromQStringBuilder(StringBuilder &&sb, QStringView expected) const
{
    auto toAnyStringView = [](QAnyStringView sv) { return sv; };
    QCOMPARE(toAnyStringView(std::forward<StringBuilder>(sb)), expected);
}

template <typename Char>
void tst_QAnyStringView::fromArray() const
{
    constexpr Char hello[] = {'H', 'e', 'l', 'l', 'o', '\0', 'a', 'b', 'c', '\0', '\0', '.', '\0'};

    QAnyStringView sv = QAnyStringView::fromArray(hello);
    QCOMPARE(sv.size(), 13);
    QVERIFY(!sv.empty());
    QVERIFY(!sv.isEmpty());
    QVERIFY(!sv.isNull());
    QCOMPARE(sv.front(), 'H');
    QCOMPARE(sv.back(),  '\0');

    const Char bytes[] = {'a', 'b', 'c'};
    QAnyStringView sv2 = QAnyStringView::fromArray(bytes);
    QCOMPARE(sv2.data(), static_cast<const void *>(bytes + 0));
    QCOMPARE(sv2.size(), 3);
    QCOMPARE(sv2.back(), u'c');
}


template <typename QStringOrByteArray>
void tst_QAnyStringView::fromQStringOrByteArray() const
{
    using Char = std::remove_cv_t<typename QStringOrByteArray::value_type>;
    using Strings = SampleStrings<Char>;

    QStringOrByteArray null;
    QStringOrByteArray empty(Strings::emptyString);

    QVERIFY( QAnyStringView(null).isNull());
    QVERIFY( QAnyStringView(null).isEmpty());
    QVERIFY( QAnyStringView(empty).isEmpty());
    QVERIFY(!QAnyStringView(empty).isNull());

    conversion_tests(QStringOrByteArray(Strings::oneChar));
    if (QTest::currentTestFailed())
        return;
    conversion_tests(QStringOrByteArray(Strings::twoChars));
    if (QTest::currentTestFailed())
        return;
    conversion_tests(QStringOrByteArray(Strings::threeChars));
    if (QTest::currentTestFailed())
        return;
    conversion_tests(QStringOrByteArray(Strings::regularString));
    if (QTest::currentTestFailed())
        return;
    conversion_tests(QStringOrByteArray(Strings::regularLongString));
    if (QTest::currentTestFailed())
        return;
    conversion_tests(QStringOrByteArray(Strings::stringWithNulls, Strings::stringWithNullsLength));
}

template <typename Char>
void tst_QAnyStringView::fromLiteral(const Char *arg) const
{
    const Char *null = nullptr;
    const Char empty[] = { Char{} };

    QCOMPARE(QAnyStringView(null).size(), qsizetype(0));
    QCOMPARE(QAnyStringView(null).data(), nullptr);
    QCOMPARE(QAnyStringView(empty).size(), qsizetype(0));
    QCOMPARE(static_cast<const void*>(QAnyStringView(empty).data()),
             static_cast<const void*>(empty));

    QVERIFY( QAnyStringView(null).isNull());
    QVERIFY( QAnyStringView(null).isEmpty());
    QVERIFY( QAnyStringView(empty).isEmpty());
    QVERIFY(!QAnyStringView(empty).isNull());

    conversion_tests(arg);
}

template<typename Char>
void tst_QAnyStringView::fromCharacter(Char arg, qsizetype expectedSize) const
{
    // Need to re-create a new QASV(arg) each time, QASV(Char).data() dangles
    // after the end of the Full Expression:

    static_assert(noexcept(QAnyStringView(arg)),
                  "If this fails, we may be creating a temporary QString/QByteArray");

    QCOMPARE(QAnyStringView(arg).size(), expectedSize);

    // QCOMPARE(QAnyStringView(arg), arg); // not all pairs compile, so do it manually:

    // Check implicit conversion:
    const QChar chars[] = {
        [](QAnyStringView v) { return v.front(); }(arg),
        [](QAnyStringView v) { return v.back();  }(arg),
    };

    switch (expectedSize) {
    case 1:
        if constexpr (std::is_same_v<Char, signed char>) // QChar doesn't have a ctor for this
            QCOMPARE(chars[0], QChar(uchar(arg)));
        else
            QCOMPARE(chars[0], QChar(arg));
        break;
    case 2:
        QCOMPARE_EQ(QAnyStringView(arg), QStringView::fromArray(chars));
        if constexpr (std::is_convertible_v<Char, char32_t>)
            QCOMPARE_EQ(QAnyStringView(arg), QStringView(QChar::fromUcs4(arg)));
        break;
    default:
        QFAIL("Don't know how to compare this type to QAnyStringView");
    }

    // conversion_tests() would produce dangling references
}

template <typename Char>
void tst_QAnyStringView::fromRange() const
{
    auto doTest = [](const Char *first, const Char *last) {
        QCOMPARE(QAnyStringView(first, first).size(), 0);
        QCOMPARE(static_cast<const void*>(QAnyStringView(first, first).data()),
                 static_cast<const void*>(first));

        const auto sv = QAnyStringView(first, last);
        QCOMPARE(sv.size(), last - first);
        QCOMPARE(static_cast<const void*>(sv.data()),
                 static_cast<const void*>(first));

        // can't call conversion_tests() here, as it requires a single object
    };
    const Char *null = nullptr;
    using RealChar = std::conditional_t<sizeof(Char) == 1, char, char16_t>;
    using Strings = SampleStrings<RealChar>;

    QCOMPARE(QAnyStringView(null, null).size(), 0);
    QCOMPARE(QAnyStringView(null, null).data(), nullptr);

    doTest(reinterpret_cast<const Char *>(std::begin(Strings::regularString)),
           reinterpret_cast<const Char *>(std::end(Strings::regularString)));
    if (QTest::currentTestFailed())
        return;

    doTest(reinterpret_cast<const Char *>(std::begin(Strings::regularLongString)),
           reinterpret_cast<const Char *>(std::end(Strings::regularLongString)));
    if (QTest::currentTestFailed())
        return;

    doTest(reinterpret_cast<const Char *>(std::begin(Strings::stringWithNulls)),
           reinterpret_cast<const Char *>(std::end(Strings::stringWithNulls)));
    if (QTest::currentTestFailed())
        return;
}

template <typename Char, typename Container>
void tst_QAnyStringView::fromContainer() const
{
    const std::string s = "Hello World!";
    const std::string n(SampleStrings<char>::stringWithNulls, SampleStrings<char>::stringWithNullsLength);

    Container c;
    // unspecified whether empty containers make null QAnyStringViews
    QVERIFY(QAnyStringView(c).isEmpty());

    std::copy(s.begin(), s.end(), std::back_inserter(c));
    conversion_tests(std::move(c));
    if (QTest::currentTestFailed())
        return;

    // repeat with nulls
    c = {};
    std::copy(n.begin(), n.end(), std::back_inserter(c));
    conversion_tests(std::move(c));
}

template <typename Char>
void tst_QAnyStringView::fromContainers() const
{
    fromContainer<Char, QList<Char>>();
    fromContainer<Char, QVarLengthArray<Char>>();
    fromContainer<Char, std::vector<Char>>();
}

namespace help {
    template <typename T>
    auto ssize(T &t) { return q20::ssize(t); }

    template <typename T>
    qsizetype ssize(const T *t)
    {
        qsizetype result = 0;
        if (t) {
            while (*t++)
                ++result;
        }
        return result;
    }

    qsizetype ssize(const QChar *t)
    {
        qsizetype result = 0;
        if (t) {
            while (!t++->isNull())
                ++result;
        }
        return result;
    }
}

template <typename String>
void tst_QAnyStringView::conversion_tests(String string) const
{
    // copy-construct:
    {
        QAnyStringView sv = string;

        QCOMPARE(help::ssize(sv), help::ssize(string));

        QCOMPARE(sv, string);
    }

    QAnyStringView sv;

    // copy-assign:
    {
        sv = string;

        QCOMPARE(help::ssize(sv), help::ssize(string));

        // check relational operators:

        QCOMPARE(sv, string);
        QCOMPARE(string, sv);

        QVERIFY(!(sv != string));
        QVERIFY(!(string != sv));

        QVERIFY(!(sv < string));
        QVERIFY(sv <= string);
        QVERIFY(!(sv > string));
        QVERIFY(sv >= string);

        QVERIFY(!(string < sv));
        QVERIFY(string <= sv);
        QVERIFY(!(string > sv));
        QVERIFY(string >= sv);
    }

    // copy-construct from rvalue (QAnyStringView never assumes ownership):
    {
        QAnyStringView sv2 = std::move(string);
        QCOMPARE(sv2, sv);
        QCOMPARE(sv2, string);
    }

    // copy-assign from rvalue (QAnyStringView never assumes ownership):
    {
        QAnyStringView sv2;
        sv2 = std::move(string);
        QCOMPARE(sv2, sv);
        QCOMPARE(sv2, string);
    }
}

void tst_QAnyStringView::comparison()
{
    const QAnyStringView aa = u"aa";
    const QAnyStringView upperAa = u"AA";
    const QAnyStringView bb = u"bb";

    QVERIFY(aa == aa);
    QVERIFY(aa != bb);
    QVERIFY(aa < bb);
    QVERIFY(bb > aa);

    QCOMPARE(QAnyStringView::compare(aa, aa), 0);
    QVERIFY(QAnyStringView::compare(aa, upperAa) != 0);
    QCOMPARE(QAnyStringView::compare(aa, upperAa, Qt::CaseInsensitive), 0);
    QVERIFY(QAnyStringView::compare(aa, bb) < 0);
    QVERIFY(QAnyStringView::compare(bb, aa) > 0);
}

void tst_QAnyStringView::compare3way()
{
#define COMPARE_3WAY(lhs, rhs, res) \
    do { \
        const auto qt_3way_cmp_res = (lhs) <=> (rhs); \
        static_assert(std::is_same_v<decltype(qt_3way_cmp_res), decltype(res)>); \
        QCOMPARE(std::is_eq(qt_3way_cmp_res), std::is_eq(res)); \
        QCOMPARE(std::is_lt(qt_3way_cmp_res), std::is_lt(res)); \
        QCOMPARE(std::is_gt(qt_3way_cmp_res), std::is_gt(res)); \
    } while (false)

    ONLY_3WAY(
    const QAnyStringView aa = u"aa";
    const QAnyStringView upperAa = u"AA";
    const QAnyStringView bb = u"bb";
    COMPARE_3WAY(aa, aa, std::strong_ordering::equal);
    COMPARE_3WAY(aa, bb, std::strong_ordering::less);
    COMPARE_3WAY(bb, aa, std::strong_ordering::greater);
    COMPARE_3WAY(upperAa, aa, std::strong_ordering::less);
    COMPARE_3WAY(aa, upperAa, std::strong_ordering::greater);
    );
#undef COMPARE_3WAY
}

QTEST_APPLESS_MAIN(tst_QAnyStringView)
#include "tst_qanystringview.moc"
