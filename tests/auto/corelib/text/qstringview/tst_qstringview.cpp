// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QStringView>
#include <QStringTokenizer>
#include <QString>
#include <QChar>
#include <QVarLengthArray>
#include <QList>
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

// for negative testing (can't convert from)
#include <deque>
#include <list>

template <typename T>
using CanConvert = std::is_convertible<T, QStringView>;

static_assert(!CanConvert<QLatin1String>::value);
static_assert(!CanConvert<const char*>::value);
static_assert(!CanConvert<QByteArray>::value);

// QStringView qchar_does_not_compile() { return QStringView(QChar('a')); }
// QStringView qlatin1string_does_not_compile() { return QStringView(QLatin1String("a")); }
// QStringView const_char_star_does_not_compile() { return QStringView("a"); }
// QStringView qbytearray_does_not_compile() { return QStringView(QByteArray("a")); }

//
// QChar
//

static_assert(!CanConvert<QChar>::value);

static_assert(CanConvert<QChar[123]>::value);

static_assert(CanConvert<      QString >::value);
static_assert(CanConvert<const QString >::value);
static_assert(CanConvert<      QString&>::value);
static_assert(CanConvert<const QString&>::value);

//
// ushort
//

static_assert(!CanConvert<ushort>::value);

static_assert(CanConvert<ushort[123]>::value);

static_assert(CanConvert<      ushort*>::value);
static_assert(CanConvert<const ushort*>::value);

static_assert(CanConvert<QList<ushort>>::value);
static_assert(CanConvert<QVarLengthArray<ushort>>::value);
static_assert(CanConvert<std::vector<ushort>>::value);
static_assert(CanConvert<std::array<ushort, 123>>::value);
static_assert(!CanConvert<std::deque<ushort>>::value);
static_assert(!CanConvert<std::list<ushort>>::value);

//
// char16_t
//

static_assert(!CanConvert<char16_t>::value);

static_assert(CanConvert<      char16_t*>::value);
static_assert(CanConvert<const char16_t*>::value);

static_assert(CanConvert<      std::u16string >::value);
static_assert(CanConvert<const std::u16string >::value);
static_assert(CanConvert<      std::u16string&>::value);
static_assert(CanConvert<const std::u16string&>::value);

static_assert(CanConvert<      std::u16string_view >::value);
static_assert(CanConvert<const std::u16string_view >::value);
static_assert(CanConvert<      std::u16string_view&>::value);
static_assert(CanConvert<const std::u16string_view&>::value);

static_assert(CanConvert<QList<char16_t>>::value);
static_assert(CanConvert<QVarLengthArray<char16_t>>::value);
static_assert(CanConvert<std::vector<char16_t>>::value);
static_assert(CanConvert<std::array<char16_t, 123>>::value);
static_assert(!CanConvert<std::deque<char16_t>>::value);
static_assert(!CanConvert<std::list<char16_t>>::value);

static_assert(CanConvert<QtPrivate::XmlStringRef>::value);

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

static_assert(!CanConvert<wchar_t>::value);

static_assert(CanConvert<      wchar_t*>::value == CanConvertFromWCharT);
static_assert(CanConvert<const wchar_t*>::value == CanConvertFromWCharT);

static_assert(CanConvert<      std::wstring >::value == CanConvertFromWCharT);
static_assert(CanConvert<const std::wstring >::value == CanConvertFromWCharT);
static_assert(CanConvert<      std::wstring&>::value == CanConvertFromWCharT);
static_assert(CanConvert<const std::wstring&>::value == CanConvertFromWCharT);

static_assert(CanConvert<      std::wstring_view >::value == CanConvertFromWCharT);
static_assert(CanConvert<const std::wstring_view >::value == CanConvertFromWCharT);
static_assert(CanConvert<      std::wstring_view&>::value == CanConvertFromWCharT);
static_assert(CanConvert<const std::wstring_view&>::value == CanConvertFromWCharT);

static_assert(CanConvert<QList<wchar_t>>::value == CanConvertFromWCharT);
static_assert(CanConvert<QVarLengthArray<wchar_t>>::value == CanConvertFromWCharT);
static_assert(CanConvert<std::vector<wchar_t>>::value == CanConvertFromWCharT);
static_assert(CanConvert<std::array<wchar_t, 123>>::value == CanConvertFromWCharT);
static_assert(!CanConvert<std::deque<wchar_t>>::value);
static_assert(!CanConvert<std::list<wchar_t>>::value);

#if QT_CONFIG(cpp_winrt)

//
// winrt::hstring (QTBUG-111886)
//

static_assert(CanConvert<      winrt::hstring >::value);
static_assert(CanConvert<const winrt::hstring >::value);
static_assert(CanConvert<      winrt::hstring&>::value);
static_assert(CanConvert<const winrt::hstring&>::value);

#endif // QT_CONFIG(cpp_winrt)

class tst_QStringView : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constExpr() const;
    void basics() const;
    void literals() const;
    void fromArray() const;
    void at() const;

    void arg() const;

    void fromQString() const;

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

    void fromChar16TStar() const
    {
        fromLiteral(u"Hello, World!");
    }

    void fromWCharTStar() const
    {
#ifdef Q_OS_WIN
        fromLiteral(L"Hello, World!");
#else
        QSKIP("This is a Windows-only test");
#endif
    }

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
#ifdef Q_OS_WIN
        const wchar_t str[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
        fromRange(std::begin(str), std::end(str));
#else
        QSKIP("This is a Windows-only test");
#endif
    }

    // std::basic_string
    void fromStdStringWCharT() const
    {
#ifdef Q_OS_WIN
        fromStdString<wchar_t>();
#else
        QSKIP("This is a Windows-only test");
#endif
    }
    void fromStdStringChar16T() const
    {
        fromStdString<char16_t>();
    }

    void fromUShortContainers() const
    {
        fromContainers<ushort>();
    }

    void fromQCharContainers() const
    {
        fromContainers<QChar>();
    }

    void fromChar16TContainers() const
    {
        fromContainers<char16_t>();
    }

    void fromWCharTContainers() const
    {
#ifdef Q_OS_WIN
        fromContainers<wchar_t>();
#else
        QSKIP("This is a Windows-only test");
#endif
    }

    void comparison();

    void overloadResolution();

    void tokenize_data() const;
    void tokenize() const;

private:
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
};

void tst_QStringView::constExpr() const
{
    // compile-time checks
    {
        constexpr QStringView sv;
        static_assert(sv.size() == 0);
        static_assert(sv.isNull());
        static_assert(sv.empty());
        static_assert(sv.isEmpty());
        static_assert(sv.utf16() == nullptr);

        constexpr QStringView sv2(sv.utf16(), sv.utf16() + sv.size());
        static_assert(sv2.isNull());
        static_assert(sv2.empty());
    }
    {
        constexpr QStringView sv = nullptr;
        Q_STATIC_ASSERT(sv.size() == 0);
        Q_STATIC_ASSERT(sv.isNull());
        Q_STATIC_ASSERT(sv.empty());
        Q_STATIC_ASSERT(sv.isEmpty());
        Q_STATIC_ASSERT(sv.utf16() == nullptr);
    }
    {
        constexpr QStringView sv = u"";
        static_assert(sv.size() == 0);
        static_assert(!sv.isNull());
        static_assert(sv.empty());
        static_assert(sv.isEmpty());
        static_assert(sv.utf16() != nullptr);

        constexpr QStringView sv2(sv.utf16(), sv.utf16() + sv.size());
        static_assert(!sv2.isNull());
        static_assert(sv2.empty());
    }
    {
        constexpr QStringView sv = u"Hello";
        static_assert(sv.size() == 5);
        static_assert(!sv.empty());
        static_assert(!sv.isEmpty());
        static_assert(!sv.isNull());
        static_assert(*sv.utf16() == 'H');
        static_assert(sv[0]      == QLatin1Char('H'));
        static_assert(sv.at(0)   == QLatin1Char('H'));
        static_assert(sv.front() == QLatin1Char('H'));
        static_assert(sv.first() == QLatin1Char('H'));
        static_assert(sv[4]      == QLatin1Char('o'));
        static_assert(sv.at(4)   == QLatin1Char('o'));
        static_assert(sv.back()  == QLatin1Char('o'));
        static_assert(sv.last()  == QLatin1Char('o'));

        constexpr QStringView sv2(sv.utf16(), sv.utf16() + sv.size());
        static_assert(!sv2.isNull());
        static_assert(!sv2.empty());
        static_assert(sv2.size() == 5);
    }
    {
        static_assert(QStringView(u"Hello").size() == 5);
        constexpr QStringView sv = u"Hello";
        static_assert(sv.size() == 5);
        static_assert(!sv.empty());
        static_assert(!sv.isEmpty());
        static_assert(!sv.isNull());
        static_assert(*sv.utf16() == 'H');
        static_assert(sv[0]      == QLatin1Char('H'));
        static_assert(sv.at(0)   == QLatin1Char('H'));
        static_assert(sv.front() == QLatin1Char('H'));
        static_assert(sv.first() == QLatin1Char('H'));
        static_assert(sv[4]      == QLatin1Char('o'));
        static_assert(sv.at(4)   == QLatin1Char('o'));
        static_assert(sv.back()  == QLatin1Char('o'));
        static_assert(sv.last()  == QLatin1Char('o'));

        constexpr QStringView sv2(sv.utf16(), sv.utf16() + sv.size());
        static_assert(!sv2.isNull());
        static_assert(!sv2.empty());
        static_assert(sv2.size() == 5);

        constexpr char16_t *null = nullptr;
        constexpr QStringView sv3(null);
        static_assert(sv3.isNull());
        static_assert(sv3.isEmpty());
        static_assert(sv3.size() == 0);
    }
}

void tst_QStringView::basics() const
{
    QStringView sv1;

    // a default-constructed QStringView is null:
    QVERIFY(sv1.isNull());
    // which implies it's empty();
    QVERIFY(sv1.isEmpty());

    QStringView sv2;

    QVERIFY(sv2 == sv1);
    QVERIFY(!(sv2 != sv1));
}

void tst_QStringView::literals() const
{
    const char16_t hello[] = u"Hello";
    const char16_t longhello[] =
            u"Hello World. This is a much longer message, to exercise qustrlen.";
    const char16_t withnull[] = u"a\0zzz";
    static_assert(sizeof(longhello) >= 16);

    QCOMPARE(QStringView(hello).size(), 5);
    QCOMPARE(QStringView(hello + 0).size(), 5); // forces decay to pointer
    QStringView sv = hello;
    QCOMPARE(sv.size(), 5);
    QVERIFY(!sv.empty());
    QVERIFY(!sv.isEmpty());
    QVERIFY(!sv.isNull());
    QCOMPARE(*sv.utf16(), 'H');
    QCOMPARE(sv[0],      QLatin1Char('H'));
    QCOMPARE(sv.at(0),   QLatin1Char('H'));
    QCOMPARE(sv.front(), QLatin1Char('H'));
    QCOMPARE(sv.first(), QLatin1Char('H'));
    QCOMPARE(sv[4],      QLatin1Char('o'));
    QCOMPARE(sv.at(4),   QLatin1Char('o'));
    QCOMPARE(sv.back(),  QLatin1Char('o'));
    QCOMPARE(sv.last(),  QLatin1Char('o'));

    QStringView sv2(sv.utf16(), sv.utf16() + sv.size());
    QVERIFY(!sv2.isNull());
    QVERIFY(!sv2.empty());
    QCOMPARE(sv2.size(), 5);

    QStringView sv3(longhello);
    QCOMPARE(size_t(sv3.size()), sizeof(longhello)/sizeof(longhello[0]) - 1);
    QCOMPARE(sv3.last(), QLatin1Char('.'));
    sv3 = longhello;
    QCOMPARE(size_t(sv3.size()), sizeof(longhello)/sizeof(longhello[0]) - 1);

    for (int i = 0; i < sv3.size(); ++i) {
        QStringView sv4(longhello + i);
        QCOMPARE(size_t(sv4.size()), sizeof(longhello)/sizeof(longhello[0]) - 1 - i);
        QCOMPARE(sv4.last(), QLatin1Char('.'));
        sv4 = longhello + i;
        QCOMPARE(size_t(sv4.size()), sizeof(longhello)/sizeof(longhello[0]) - 1 - i);
    }

    // these are different results
    QCOMPARE(size_t(QStringView(withnull).size()), size_t(1));
    QCOMPARE(size_t(QStringView::fromArray(withnull).size()), sizeof(withnull)/sizeof(withnull[0]));
    QCOMPARE(QStringView(withnull + 0).size(), qsizetype(1));
}

void tst_QStringView::fromArray() const
{
    static constexpr char16_t hello[] = u"Hello\0abc\0\0.";

    constexpr QStringView sv = QStringView::fromArray(hello);
    QCOMPARE(sv.size(), 13);
    QVERIFY(!sv.empty());
    QVERIFY(!sv.isEmpty());
    QVERIFY(!sv.isNull());
    QCOMPARE(*sv.data(), 'H');
    QCOMPARE(sv[0],      'H');
    QCOMPARE(sv.at(0),   'H');
    QCOMPARE(sv.front(), 'H');
    QCOMPARE(sv.first(), 'H');
    QCOMPARE(sv[4],      'o');
    QCOMPARE(sv.at(4),   'o');
    QCOMPARE(sv[5],      '\0');
    QCOMPARE(sv.at(5),   '\0');
    QCOMPARE(*(sv.data() + sv.size() - 2),  '.');
    QCOMPARE(sv.back(),  '\0');
    QCOMPARE(sv.last(),  '\0');

    const char16_t bytes[] = {u'a', u'b', u'c'};
    QStringView sv2 = QStringView::fromArray(bytes);
    QCOMPARE(sv2.data(), reinterpret_cast<const QChar *>(bytes + 0));
    QCOMPARE(sv2.size(), 3);
    QCOMPARE(sv2.first(), u'a');
    QCOMPARE(sv2.last(), u'c');
}

void tst_QStringView::at() const
{
    QString hello("Hello");
    QStringView sv(hello);
    QCOMPARE(sv.at(0), QChar('H')); QCOMPARE(sv[0], QChar('H'));
    QCOMPARE(sv.at(1), QChar('e')); QCOMPARE(sv[1], QChar('e'));
    QCOMPARE(sv.at(2), QChar('l')); QCOMPARE(sv[2], QChar('l'));
    QCOMPARE(sv.at(3), QChar('l')); QCOMPARE(sv[3], QChar('l'));
    QCOMPARE(sv.at(4), QChar('o')); QCOMPARE(sv[4], QChar('o'));
}

void tst_QStringView::arg() const
{
#define CHECK1(pattern, arg1, expected) \
    do { \
        auto p = QStringView(u"" pattern); \
        QCOMPARE(p.arg(QLatin1String(arg1)), expected); \
        QCOMPARE(p.arg(u"" arg1), expected); \
        QCOMPARE(p.arg(QStringLiteral(arg1)), expected); \
        QCOMPARE(p.arg(QString(QLatin1String(arg1))), expected); \
    } while (false) \
    /*end*/
#define CHECK2(pattern, arg1, arg2, expected) \
    do { \
        auto p = QStringView(u"" pattern); \
        QCOMPARE(p.arg(QLatin1String(arg1), QLatin1String(arg2)), expected); \
        QCOMPARE(p.arg(u"" arg1, QLatin1String(arg2)), expected); \
        QCOMPARE(p.arg(QLatin1String(arg1), u"" arg2), expected); \
        QCOMPARE(p.arg(u"" arg1, u"" arg2), expected); \
    } while (false) \
    /*end*/

    CHECK1("", "World", "");
    CHECK1("%1", "World", "World");
    CHECK1("!%1?", "World", "!World?");
    CHECK1("%1%1", "World", "WorldWorld");
    CHECK1("%1%2", "World", "World%2");
    CHECK1("%2%1", "World", "%2World");

    CHECK2("", "Hello", "World", "");
    CHECK2("%1", "Hello", "World", "Hello");
    CHECK2("!%1, %2?", "Hello", "World", "!Hello, World?");
    CHECK2("%1%1", "Hello", "World", "HelloHello");
    CHECK2("%1%2", "Hello", "World", "HelloWorld");
    CHECK2("%2%1", "Hello", "World", "WorldHello");

#undef CHECK2
#undef CHECK1

    QCOMPARE(QStringView(u" %2 %2 %1 %3 ").arg(QLatin1Char('c'), QChar::CarriageReturn, u'C'), " \r \r c C ");
}

void tst_QStringView::fromQString() const
{
    QString null;
    QString empty = "";

    QVERIFY( QStringView(null).isNull());
    QVERIFY( QStringView(null).isEmpty());
    QVERIFY( QStringView(empty).isEmpty());
    QVERIFY(!QStringView(empty).isNull());

    conversion_tests(QString("Hello World!"));
}

void tst_QStringView::tokenize_data() const
{
    // copied from tst_QString
    QTest::addColumn<QString>("str");
    QTest::addColumn<QString>("sep");
    QTest::addColumn<QStringList>("result");

    QTest::newRow("1") << "a,b,c" << "," << (QStringList() << "a" << "b" << "c");
    QTest::newRow("2") << QString("-rw-r--r--  1 0  0  519240 Jul  9  2002 bigfile")
                       << " "
                       << (QStringList() << "-rw-r--r--" << "" << "1" << "0" << "" << "0" << ""
                                         << "519240" << "Jul" << "" << "9" << "" << "2002"
                                         << "bigfile");
    QTest::newRow("one-empty") << "" << " " << (QStringList() << "");
    QTest::newRow("two-empty") << " " << " " << (QStringList() << "" << "");
    QTest::newRow("three-empty") << "  " << " " << (QStringList() << "" << "" << "");

    QTest::newRow("all-empty") << "" << "" << (QStringList() << "" << "");
    QTest::newRow("sep-empty") << "abc" << "" << (QStringList() << "" << "a" << "b" << "c" << "");
}

void tst_QStringView::tokenize() const
{
    QFETCH(const QString, str);
    QFETCH(const QString, sep);
    QFETCH(const QStringList, result);

    // lvalue QString
    {
        auto rit = result.cbegin();
        for (auto sv : QStringTokenizer{str, sep})
            QCOMPARE(sv, *rit++);
    }
    {
        auto rit = result.cbegin();
        for (auto sv : QStringView{str}.tokenize(sep))
            QCOMPARE(sv, *rit++);
    }

    // rvalue QString
    {
        auto rit = result.cbegin();
        for (auto sv : QStringTokenizer{str, QString{sep}})
            QCOMPARE(sv, *rit++);
    }
    {
        auto rit = result.cbegin();
        for (auto sv : QStringView{str}.tokenize(QString{sep}))
            QCOMPARE(sv, *rit++);
    }

    // (rvalue) QChar
    if (sep.size() == 1) {
        auto rit = result.cbegin();
        for (auto sv : QStringTokenizer{str, sep.front()})
            QCOMPARE(sv, *rit++);
    }
    if (sep.size() == 1) {
        auto rit = result.cbegin();
        for (auto sv : QStringView{str}.tokenize(sep.front()))
            QCOMPARE(sv, *rit++);
    }

    // (rvalue) char16_t
    if (sep.size() == 1) {
        auto rit = result.cbegin();
        for (auto sv : QStringTokenizer{str, *qToStringViewIgnoringNull(sep).utf16()})
            QCOMPARE(sv, *rit++);
    }
    if (sep.size() == 1) {
        auto rit = result.cbegin();
        for (auto sv : QStringView{str}.tokenize(*qToStringViewIgnoringNull(sep).utf16()))
            QCOMPARE(sv, *rit++);
    }

    // char16_t literal
    const auto make_literal = [](const QString &sep) {
        auto literal = std::make_unique<char16_t[]>(sep.size() + 1);
        const auto to_char16_t = [](QChar c) { return char16_t{c.unicode()}; };
        std::transform(sep.cbegin(), sep.cend(), literal.get(), to_char16_t);
        return literal;
    };
    const std::unique_ptr<const char16_t[]> literal = make_literal(sep);
    {
        auto rit = result.cbegin();
        for (auto sv : QStringTokenizer{str, literal.get()})
            QCOMPARE(sv, *rit++);
    }
    {
        auto rit = result.cbegin();
        for (auto sv : QStringView{str}.tokenize(literal.get()))
            QCOMPARE(sv, *rit++);
    }

#ifdef __cpp_lib_ranges
    // lvalue QString
    {
        QStringList actual;
        const QStringTokenizer tok{str, sep};
        std::ranges::transform(tok, std::back_inserter(actual),
                               [](auto sv) { return sv.toString(); });
        QCOMPARE(result, actual);
    }

    // rvalue QString
    {
        QStringList actual;
        const QStringTokenizer tok{str, QString{sep}};
        std::ranges::transform(tok, std::back_inserter(actual),
                               [](auto sv) { return sv.toString(); });
        QCOMPARE(result, actual);
    }

    // (rvalue) QChar
    if (sep.size() == 1) {
        QStringList actual;
        const QStringTokenizer tok{str, sep.front()};
        std::ranges::transform(tok, std::back_inserter(actual),
                               [](auto sv) { return sv.toString(); });
        QCOMPARE(result, actual);
    }
#endif // __cpp_lib_ranges
}

template <typename Char>
void tst_QStringView::fromLiteral(const Char *arg) const
{
    const Char *null = nullptr;
    const Char empty[] = { Char{} };

    QCOMPARE(QStringView(null).size(), qsizetype(0));
    QCOMPARE(QStringView(null).data(), nullptr);
    QCOMPARE(QStringView(empty).size(), qsizetype(0));
    QCOMPARE(static_cast<const void*>(QStringView(empty).data()),
             static_cast<const void*>(empty));

    QVERIFY( QStringView(null).isNull());
    QVERIFY( QStringView(null).isEmpty());
    QVERIFY( QStringView(empty).isEmpty());
    QVERIFY(!QStringView(empty).isNull());

    conversion_tests(arg);
}

template <typename Char>
void tst_QStringView::fromRange(const Char *first, const Char *last) const
{
    const Char *null = nullptr;
    QCOMPARE(QStringView(null, null).size(), 0);
    QCOMPARE(QStringView(null, null).data(), nullptr);
    QCOMPARE(QStringView(first, first).size(), 0);
    QCOMPARE(static_cast<const void*>(QStringView(first, first).data()),
             static_cast<const void*>(first));

    const auto sv = QStringView(first, last);
    QCOMPARE(sv.size(), last - first);
    QCOMPARE(static_cast<const void*>(sv.data()),
             static_cast<const void*>(first));

    // can't call conversion_tests() here, as it requires a single object
}

template <typename Char, typename Container>
void tst_QStringView::fromContainer() const
{
    const QString s = "Hello World!";

    Container c;
    // unspecified whether empty containers make null QStringViews
    QVERIFY(QStringView(c).isEmpty());

    QCOMPARE(sizeof(Char), sizeof(QChar));

    const auto *data = reinterpret_cast<const Char *>(s.utf16());
    std::copy(data, data + s.size(), std::back_inserter(c));
    conversion_tests(std::move(c));
}

template <typename Char>
void tst_QStringView::fromContainers() const
{
    fromContainer<Char, QList<Char>>();
    fromContainer<Char, QVarLengthArray<Char>>();
    fromContainer<Char, std::vector<Char>>();
}

namespace help {
template <typename T>
size_t size(const T &t) { return size_t(t.size()); }
template <typename T>
size_t size(const T *t)
{
    size_t result = 0;
    if (t) {
        while (*t++)
            ++result;
    }
    return result;
}
size_t size(const QChar *t)
{
    size_t result = 0;
    if (t) {
        while (!t++->isNull())
            ++result;
    }
    return result;
}

template <typename T>
decltype(auto)             cbegin(const T &t) { return t.begin(); }
template <typename T>
const T *                  cbegin(const T *t) { return t; }

template <typename T>
decltype(auto)             cend(const T &t) { return t.end(); }
template <typename T>
const T *                  cend(const T *t) { return t + size(t); }

template <typename T>
decltype(auto)                     crbegin(const T &t) { return t.rbegin(); }
template <typename T>
std::reverse_iterator<const T*>    crbegin(const T *t) { return std::reverse_iterator<const T*>(cend(t)); }

template <typename T>
decltype(auto)                     crend(const T &t) { return t.rend(); }
template <typename T>
std::reverse_iterator<const T*>    crend(const T *t) { return std::reverse_iterator<const T*>(cbegin(t)); }

} // namespace help

template <typename String>
void tst_QStringView::conversion_tests(String string) const
{
    // copy-construct:
    {
        QStringView sv = string;

        QCOMPARE(help::size(sv), help::size(string));

        // check iterators:

        QVERIFY(std::equal(help::cbegin(string), help::cend(string),
                           QT_MAKE_CHECKED_ARRAY_ITERATOR(sv.cbegin(), sv.size())));
        QVERIFY(std::equal(help::cbegin(string), help::cend(string),
                           QT_MAKE_CHECKED_ARRAY_ITERATOR(sv.begin(), sv.size())));
        QVERIFY(std::equal(help::crbegin(string), help::crend(string),
                           sv.crbegin()));
        QVERIFY(std::equal(help::crbegin(string), help::crend(string),
                           sv.rbegin()));

        QCOMPARE(sv, string);
    }

    QStringView sv;

    // copy-assign:
    {
        sv = string;

        QCOMPARE(help::size(sv), help::size(string));

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

    // copy-construct from rvalue (QStringView never assumes ownership):
    {
        QStringView sv2 = std::move(string);
        QCOMPARE(sv2, sv);
        QCOMPARE(sv2, string);
    }

    // copy-assign from rvalue (QStringView never assumes ownership):
    {
        QStringView sv2;
        sv2 = std::move(string);
        QCOMPARE(sv2, sv);
        QCOMPARE(sv2, string);
    }
}

void tst_QStringView::comparison()
{
    const QStringView aa = u"aa";
    const QStringView upperAa = u"AA";
    const QStringView bb = u"bb";

    QVERIFY(aa == aa);
    QVERIFY(aa != bb);
    QVERIFY(aa < bb);
    QVERIFY(bb > aa);

    QCOMPARE(aa.compare(aa), 0);
    QVERIFY(aa.compare(upperAa) != 0);
    QCOMPARE(aa.compare(upperAa, Qt::CaseInsensitive), 0);
    QVERIFY(aa.compare(bb) < 0);
    QVERIFY(bb.compare(aa) > 0);
}

namespace QStringViewOverloadResolution {
static void test(QString) = delete;
static void test(QStringView) {}
}

// Compile-time only test: overload resolution prefers QStringView over QString
void tst_QStringView::overloadResolution()
{
    {
        QChar qcharArray[42] = {};
        QStringViewOverloadResolution::test(qcharArray);
        QChar *qcharPointer = qcharArray;
        QStringViewOverloadResolution::test(qcharPointer);
    }

    {
        ushort ushortArray[42] = {};
        QStringViewOverloadResolution::test(ushortArray);
        ushort *ushortPointer = ushortArray;
        QStringViewOverloadResolution::test(ushortPointer);
    }

#if defined(Q_OS_WIN)
    {
        wchar_t wchartArray[42] = {};
        QStringViewOverloadResolution::test(wchartArray);
        QStringViewOverloadResolution::test(L"test");
    }
#endif

    {
        char16_t char16Array[] = u"test";
        QStringViewOverloadResolution::test(char16Array);
        char16_t *char16Pointer = char16Array;
        QStringViewOverloadResolution::test(char16Pointer);
    }

    {
        std::u16string string;
        QStringViewOverloadResolution::test(string);
        QStringViewOverloadResolution::test(std::as_const(string));
        QStringViewOverloadResolution::test(std::move(string));
    }
}

QTEST_APPLESS_MAIN(tst_QStringView)
#include "tst_qstringview.moc"
