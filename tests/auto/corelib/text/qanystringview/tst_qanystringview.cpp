// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QAnyStringView>
#include <QChar>
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


class tst_QAnyStringView : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constExpr() const;
    void basics() const;
    void asciiLiteralIsLatin1() const;

    void fromQString() const { fromQStringOrByteArray<QString>(); }
    void fromQByteArray() const { fromQStringOrByteArray<QByteArray>(); }

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

    void fromChar16TStar() const { fromLiteral(u"Hello, World!"); }
    void fromWCharTStar() const { ONLY_WIN(fromLiteral(L"Hello, World!")); }

    void fromQCharRange() const
    {
        const QChar str[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
        fromRange(std::begin(str), std::end(str));
    }

    void fromUShortRange() const
    {
        const ushort str[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
        fromRange(std::begin(str), std::end(str));
    }

    void fromChar16TRange() const
    {
        const char16_t str[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
        fromRange(std::begin(str), std::end(str));
    }

    void fromWCharTRange() const
    {
        [[maybe_unused]] const wchar_t str[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
        ONLY_WIN(fromRange(std::begin(str), std::end(str)));
    }

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
    void fromRange(const Char *first, const Char *last) const;
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
    }
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
    QStringOrByteArray null;
    QStringOrByteArray empty = "";

    QVERIFY( QAnyStringView(null).isNull());
    QVERIFY( QAnyStringView(null).isEmpty());
    QVERIFY( QAnyStringView(empty).isEmpty());
    QVERIFY(!QAnyStringView(empty).isNull());

    conversion_tests(QStringOrByteArray("Hello World!"));
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

template <typename Char>
void tst_QAnyStringView::fromRange(const Char *first, const Char *last) const
{
    const Char *null = nullptr;
    QCOMPARE(QAnyStringView(null, null).size(), 0);
    QCOMPARE(QAnyStringView(null, null).data(), nullptr);
    QCOMPARE(QAnyStringView(first, first).size(), 0);
    QCOMPARE(static_cast<const void*>(QAnyStringView(first, first).data()),
             static_cast<const void*>(first));

    const auto sv = QAnyStringView(first, last);
    QCOMPARE(sv.size(), last - first);
    QCOMPARE(static_cast<const void*>(sv.data()),
             static_cast<const void*>(first));

    // can't call conversion_tests() here, as it requires a single object
}

template <typename Char, typename Container>
void tst_QAnyStringView::fromContainer() const
{
    const std::string s = "Hello World!";

    Container c;
    // unspecified whether empty containers make null QAnyStringViews
    QVERIFY(QAnyStringView(c).isEmpty());

    std::copy(s.begin(), s.end(), std::back_inserter(c));
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
