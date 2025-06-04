// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtCore/QString>

#include <private/qcomparisontesthelper_p.h>

#ifdef __cpp_char8_t
#  define ONLY_IF_CHAR_8_T(expr) expr
#else
#  define ONLY_IF_CHAR_8_T(expr) \
    QSKIP("This test requires C++20 char8_t support enabled in the compiler.")
#endif

// QTBUG-112746
namespace {
extern const char string_array[];
static void from_array_of_unknown_size()
{
    auto sv = QUtf8StringView{string_array};
    QCOMPARE(sv.size(), 3);
}
const char string_array[] = "abc\0def";

#ifdef __cpp_char8_t
extern const char8_t string_array_8t[];
static void from_array_of_unknown_size_8t()
{
    auto sv = QUtf8StringView{string_array_8t};
    QCOMPARE(sv.size(), 3);
}
const char8_t string_array_8t[] = u8"abc\0def";
#endif
} // unnamed namespace

template <typename T>
const void *as_const_void_star(T *p) { return p; }

class tst_QUtf8StringView : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fromArraysOfUnknownSize();
    void constExpr();
    void basics();
    void literalsChar8();
    void literalsChar();
    void construction();
    void at();
    void arg() const;
    void fromQString();
    void comparison();
    void std_stringview_conversion();
    void overloadResolution();
    void midLeftRight();
    void nullString();
    void emptyString();
    void iterators();
    void relationalOperators_data();
    void relationalOperators();
};

void tst_QUtf8StringView::fromArraysOfUnknownSize()
{
    from_array_of_unknown_size();
    ONLY_IF_CHAR_8_T(from_array_of_unknown_size_8t());
}

void tst_QUtf8StringView::constExpr()
{
    // compile-time checks
    {
        constexpr QUtf8StringView utf8s;
        static_assert(utf8s.size() == 0);
        static_assert(utf8s.isNull());
        static_assert(utf8s.empty());
        static_assert(utf8s.isEmpty());
        static_assert(utf8s.data() == nullptr);

        constexpr QUtf8StringView utf8s2(utf8s.data(), utf8s.data() + utf8s.size());
        static_assert(utf8s2.isNull());
        static_assert(utf8s2.empty());
    }
    {
        constexpr QUtf8StringView utf8s = nullptr;
        static_assert(utf8s.size() == 0);
        static_assert(utf8s.isNull());
        static_assert(utf8s.empty());
        static_assert(utf8s.isEmpty());
        static_assert(utf8s.data() == nullptr);
    }
    {
        constexpr QUtf8StringView utf8s("");
        static_assert(utf8s.size() == 0);
        static_assert(!utf8s.isNull());
        static_assert(utf8s.empty());
        static_assert(utf8s.isEmpty());

        constexpr QUtf8StringView utf8s2(utf8s.data(), utf8s.data() + utf8s.size());
        static_assert(!utf8s2.isNull());
        static_assert(utf8s2.empty());
    }
    {
        static_assert(QUtf8StringView("Hello").size() == 5);
        constexpr QUtf8StringView utf8s("Hello");
        static_assert(utf8s.size() == 5);
        static_assert(!utf8s.empty());
        static_assert(!utf8s.isEmpty());
        static_assert(!utf8s.isNull());
        static_assert(*utf8s.data() == 'H');

        static_assert(utf8s[0]      == char('H'));
        static_assert(utf8s.at(0)   == char('H'));
        static_assert(utf8s.front() == char('H'));
        static_assert(utf8s[4]      == char('o'));
        static_assert(utf8s.at(4)   == char('o'));
        static_assert(utf8s.back()  == char('o'));

#ifdef __cpp_char8_t
        static_assert(utf8s[0]      == char8_t('H'));
        static_assert(utf8s.at(0)   == char8_t('H'));
        static_assert(utf8s.front() == char8_t('H'));
        static_assert(utf8s[4]      == char8_t('o'));
        static_assert(utf8s.at(4)   == char8_t('o'));
        static_assert(utf8s.back()  == char8_t('o'));
#endif

        constexpr QUtf8StringView utf8s2(utf8s.data(), utf8s.data() + utf8s.size());
        static_assert(!utf8s2.isNull());
        static_assert(!utf8s2.empty());
        static_assert(utf8s2.size() == 5);
    }
}

void tst_QUtf8StringView::basics()
{
    QUtf8StringView sv1;

    // a default-constructed QStringView is null:
    QVERIFY(sv1.isNull());
    // which implies it's empty();
    QVERIFY(sv1.isEmpty());

    QUtf8StringView sv2;
    QT_TEST_ALL_COMPARISON_OPS(sv2, sv1, Qt::strong_ordering::equal);
}

template <typename Char, size_t HSize, size_t LHSize, size_t WNSize>
static void testLiteralsWithType(const Char (&hello)[HSize], const Char (&longhello)[LHSize], const Char (&withnull)[WNSize])
{
    QCOMPARE(QUtf8StringView(hello).size(), 5);
    QCOMPARE(QUtf8StringView(hello + 0).size(), 5); // forces decay to pointer
    QUtf8StringView sv = hello;
    QCOMPARE(sv.size(), 5);
    QVERIFY(!sv.empty());
    QVERIFY(!sv.isEmpty());
    QVERIFY(!sv.isNull());

#ifdef __cpp_char8_t
    QCOMPARE(*sv.utf8(), u8'H');
#endif
    QCOMPARE(sv[0],       u8'H');
    QCOMPARE(sv.at(0),    u8'H');
    QCOMPARE(sv.front(),  u8'H');
    QCOMPARE(sv.first(1), u8'H');
    QCOMPARE(sv[4],       u8'o');
    QCOMPARE(sv.at(4),    u8'o');
    QCOMPARE(sv.back(),   u8'o');
    QCOMPARE(sv.last(1),  u8'o');

    QCOMPARE(*sv.data(),  'H');
    QCOMPARE(sv[0],       'H');
    QCOMPARE(sv.at(0),    'H');
    QCOMPARE(sv.front(),  'H');
    QCOMPARE(sv.first(1), 'H');
    QCOMPARE(sv[4],       'o');
    QCOMPARE(sv.at(4),    'o');
    QCOMPARE(sv.back(),   'o');
    QCOMPARE(sv.last(1),  'o');

#ifdef __cpp_char8_t
    {
        QUtf8StringView sv2(sv.utf8(), sv.utf8() + sv.size());
        QVERIFY(!sv2.isNull());
        QVERIFY(!sv2.empty());
        QCOMPARE(sv2.size(), 5);
        QCOMPARE(sv, sv2);
        QCOMPARE(sv.data(), sv2.data());
    }
#endif

    QUtf8StringView sv2(sv.data(), sv.data() + sv.size());
    QVERIFY(!sv2.isNull());
    QVERIFY(!sv2.empty());
    QCOMPARE(sv2.size(), 5);
    QCOMPARE(sv, sv2);
    QCOMPARE(sv.data(), sv2.data());

    QUtf8StringView sv3(longhello);
    QCOMPARE(size_t(sv3.size()), sizeof(longhello)/sizeof(longhello[0]) - 1);
    QCOMPARE(sv3.last(1), u'.');
    sv3 = longhello;
    QCOMPARE(size_t(sv3.size()), sizeof(longhello)/sizeof(longhello[0]) - 1);

    for (int i = 0; i < sv3.size(); ++i) {
        QUtf8StringView sv4(longhello + i);
        QCOMPARE(size_t(sv4.size()), sizeof(longhello)/sizeof(longhello[0]) - 1 - i);
        QCOMPARE(sv4.last(1), u'.');
        sv4 = longhello + i;
        QCOMPARE(size_t(sv4.size()), sizeof(longhello)/sizeof(longhello[0]) - 1 - i);
    }

    // these are different results
    QCOMPARE(size_t(QUtf8StringView(withnull).size()), size_t(1));
    QCOMPARE(size_t(QUtf8StringView::fromArray(withnull).size()), sizeof(withnull)/sizeof(withnull[0]));
    QCOMPARE(QUtf8StringView(withnull + 0).size(), qsizetype(1));
};


void tst_QUtf8StringView::literalsChar8()
{
#ifdef __cpp_char8_t
    const char8_t hello[] = u8"Hello";
    const char8_t longhello[] =
            u8"Hello World. This is a much longer message, to exercise qustrlen.";
    const char8_t withnull[] = u8"a\0zzz";
    static_assert(sizeof(longhello) >= 16);

    testLiteralsWithType(hello, longhello, withnull);
#endif
}

void tst_QUtf8StringView::literalsChar()
{
    const char hello[] = "Hello";
    const char longhello[] =
            "Hello World. This is a much longer message, to exercise qustrlen.";
    const char withnull[] = "a\0zzz";
    static_assert(sizeof(longhello) >= 16);

    testLiteralsWithType(hello, longhello, withnull);
}

void tst_QUtf8StringView::construction()
{
    {
        auto str = u8"hello";
        QUtf8StringView utf8s(str);
        QCOMPARE(utf8s.size(), 5);
        QCOMPARE(utf8s.data(), as_const_void_star(&str[0]));
        ONLY_IF_CHAR_8_T(QCOMPARE(as_const_void_star(utf8s.utf8()), as_const_void_star(&str[0])));
        ONLY_IF_CHAR_8_T(QCOMPARE(as_const_void_star(utf8s.utf8()), utf8s.data()));

        QUtf8StringView s1 = {str, 5};
        QCOMPARE(s1, utf8s);
        QUtf8StringView s2 = {str, str + 5};
        QCOMPARE(s2, utf8s);

#ifdef __cpp_char8_t
        QByteArrayView helloView(reinterpret_cast<const char *>(str));
#else
        QByteArrayView helloView(str);
#endif
        helloView = helloView.first(4);
        utf8s = QUtf8StringView(helloView);
        QCOMPARE(utf8s.data(), helloView.data());
        QCOMPARE(utf8s.data(), as_const_void_star(helloView.data()));
        QCOMPARE(utf8s.size(), helloView.size());
        ONLY_IF_CHAR_8_T(QCOMPARE(utf8s.utf8(), as_const_void_star(helloView.data())));
        ONLY_IF_CHAR_8_T(QCOMPARE(utf8s.size(), helloView.size()));
    }

    {
        const char str[6] = "hello";
        QUtf8StringView utf8s(str);
        QCOMPARE(utf8s.size(), 5);
        QCOMPARE(utf8s.data(), as_const_void_star(&str[0]));
        QCOMPARE(utf8s.data(), "hello");
        ONLY_IF_CHAR_8_T(QCOMPARE(as_const_void_star(utf8s.utf8()), as_const_void_star(&str[0])));
        ONLY_IF_CHAR_8_T(QCOMPARE(as_const_void_star(utf8s.utf8()), utf8s.data()));

        QUtf8StringView s1 = {str, 5};
        QCOMPARE(s1, utf8s);
        QUtf8StringView s2 = {str, str + 5};
        QCOMPARE(s2, utf8s);

        QByteArrayView helloView(str);
        helloView = helloView.first(4);
        utf8s = QUtf8StringView(helloView);
        QCOMPARE(utf8s.data(), helloView.data());
        QCOMPARE(utf8s.data(), as_const_void_star(helloView.data()));
        QCOMPARE(utf8s.size(), helloView.size());
        ONLY_IF_CHAR_8_T(QCOMPARE(utf8s.utf8(), as_const_void_star(helloView.data())));
        ONLY_IF_CHAR_8_T(QCOMPARE(utf8s.size(), helloView.size()));
    }

    {
        const QByteArray helloArray("hello");
        QUtf8StringView utf8s(helloArray);
        QCOMPARE(utf8s.data(), helloArray.data());
        ONLY_IF_CHAR_8_T(QCOMPARE(utf8s.utf8(), as_const_void_star(helloArray.data())));
        QCOMPARE(utf8s.size(), helloArray.size());

        QByteArrayView helloView(helloArray);
        helloView = helloView.first(4);
        utf8s = QUtf8StringView(helloView);
        QCOMPARE(utf8s.data(), helloView.data());
        ONLY_IF_CHAR_8_T(QCOMPARE(utf8s.utf8(), as_const_void_star(helloView.data())));
        QCOMPARE(utf8s.size(), helloView.size());
    }
}

void tst_QUtf8StringView::at()
{
    const QUtf8StringView utf8("Hello World");
    QCOMPARE(utf8.at(0), 'H');
    QCOMPARE(utf8.at(utf8.size() - 1), 'd');
    QCOMPARE(utf8[0], 'H');
    QCOMPARE(utf8[utf8.size() - 1], 'd');
}

void tst_QUtf8StringView::arg() const
{
    // nullness checks
    QCOMPARE(QUtf8StringView().arg(QUtf8StringView()), "");
    QCOMPARE(QUtf8StringView("%1").arg(nullptr), "");
    QCOMPARE(QUtf8StringView(u8"%1").arg(nullptr), "");

#define CHECK1(pattern, arg1, expected) \
    do { \
        auto p = QUtf8StringView(pattern); \
        QCOMPARE(p.arg(QUtf8StringView(arg1)), expected); \
        QCOMPARE(p.arg(u"" arg1), expected); \
        QCOMPARE(p.arg(QStringLiteral(arg1)), expected); \
        QCOMPARE(p.arg(QLatin1StringView(arg1).toString()), expected); \
        QCOMPARE(p.arg(u8"" arg1), expected); \
    } while (false) \
    /*end*/
#define CHECK2(pattern, arg1, arg2, expected) \
    do { \
        auto p = QUtf8StringView(pattern); \
        QCOMPARE(p.arg(QUtf8StringView(arg1), QUtf8StringView(arg2)), expected); \
        QCOMPARE(p.arg(u"" arg1, QUtf8StringView(arg2)), expected); \
        QCOMPARE(p.arg(QUtf8StringView(arg1), u"" arg2), expected); \
        QCOMPARE(p.arg(u"" arg1, u"" arg2), expected); \
        QCOMPARE(p.arg(u8"" arg1, u8"" arg2), expected); \
        QCOMPARE(p.arg(u"" arg1, u8"" arg2), expected); \
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

    QCOMPARE(QUtf8StringView(" %2 %2 %1 %3 ").arg('c', QChar::CarriageReturn, u'C'),
             " \r \r c C ");
}

void tst_QUtf8StringView::fromQString()
{
    QString null;
    QString empty = "";
    QString normal = "hello";

    QVERIFY( QUtf8StringView(null.toUtf8()).isNull());
    QVERIFY( QUtf8StringView(null.toUtf8()).isEmpty());
    QVERIFY( QUtf8StringView(empty.toUtf8()).isEmpty());
    QVERIFY(!QUtf8StringView(empty.toUtf8()).isNull());
    QVERIFY(!QUtf8StringView(normal.toUtf8()).isEmpty());
    QVERIFY(!QUtf8StringView(normal.toUtf8()).isNull());
    QCOMPARE(QUtf8StringView(normal.toUtf8()), u"hello");
}

void tst_QUtf8StringView::comparison()
{
    const QUtf8StringView aa = u8"aa";
    const QUtf8StringView upperAa = u8"AA";
    const QUtf8StringView bb = u8"bb";

    QVERIFY(aa == aa);
    QVERIFY(aa != bb);
    QVERIFY(aa < bb);
    QVERIFY(bb > aa);

    QT_TEST_ALL_COMPARISON_OPS(aa, aa, Qt::strong_ordering::equal);
    QT_TEST_ALL_COMPARISON_OPS(aa, bb, Qt::strong_ordering::less);

    QCOMPARE(aa.compare(aa), 0);
    QVERIFY(aa.compare(upperAa) != 0);
    QCOMPARE(aa.compare(upperAa, Qt::CaseInsensitive), 0);
    QVERIFY(aa.compare(bb) < 0);
    QVERIFY(bb.compare(aa) > 0);
}

void tst_QUtf8StringView::std_stringview_conversion()
{
    {
        QUtf8StringView s;
        std::basic_string_view<QUtf8StringView::storage_type> sv(s);
        QCOMPARE(sv, std::basic_string_view<QUtf8StringView::storage_type>());

        s = u8"";
        sv = s;
        QCOMPARE(s.size(), 0);
        QCOMPARE(sv.size(), size_t(0));
        QCOMPARE(sv, std::basic_string_view<QUtf8StringView::storage_type>());

        s = u8"Hello";
        sv = s;
        QCOMPARE(sv, std::basic_string_view<QUtf8StringView::storage_type>("Hello"));

        s = QUtf8StringView::fromArray(u8"Hello\0world");
        sv = s;
        QCOMPARE(s.size(), 12);
        QCOMPARE(sv.size(), size_t(12));
        QCOMPARE(sv, std::basic_string_view<QUtf8StringView::storage_type>("Hello\0world\0", 12));
    }

#ifdef __cpp_lib_char8_t
    {
        QUtf8StringView s;
        std::u8string_view sv(s);
        QCOMPARE(sv, std::u8string_view());

        s = u8"";
        sv = s;
        QCOMPARE(s.size(), 0);
        QCOMPARE(sv.size(), size_t(0));
        QCOMPARE(sv, std::u8string_view());

        s = u8"Hello";
        sv = s;
        QCOMPARE(sv, std::u8string_view(u8"Hello"));

        s = QUtf8StringView::fromArray(u8"Hello\0world");
        sv = s;
        QCOMPARE(s.size(), 12);
        QCOMPARE(sv.size(), size_t(12));
        QCOMPARE(sv, std::u8string_view(u8"Hello\0world\0", 12));
    }
#endif
}

namespace QUtf8StringViewOverloadResolution {
// QString and QUtf8StringView overloads are ambiguous
// static void test(QString) { }
// but QStringView and QUtf8StringView work
static void test(QStringView) = delete;
static void test(QUtf8StringView) { }
}
#ifdef __cpp_char8_t
extern const char8_t char8ArrayOfUnknownSize[];
#endif
extern const char charArrayOfUnknownSize[];

void tst_QUtf8StringView::overloadResolution()
{
#ifdef __cpp_char8_t
    {
        char8_t char8Array[] = u8"test";
        QUtf8StringViewOverloadResolution::test(char8Array);
        char8_t *char8Pointer = char8Array;
        QUtf8StringViewOverloadResolution::test(char8Pointer);
        QUtf8StringViewOverloadResolution::test(char8ArrayOfUnknownSize);
    }

    {
        std::u8string string;
        QUtf8StringViewOverloadResolution::test(string);
        QUtf8StringViewOverloadResolution::test(std::as_const(string));
        QUtf8StringViewOverloadResolution::test(std::move(string));
    }
#endif

    {
        char charArray[] = "test";
        QUtf8StringViewOverloadResolution::test(charArray);
        char *charPointer = charArray;
        QUtf8StringViewOverloadResolution::test(charPointer);
        QUtf8StringViewOverloadResolution::test(charArrayOfUnknownSize);
    }

    {
        std::string string;
        QUtf8StringViewOverloadResolution::test(string);
        QUtf8StringViewOverloadResolution::test(std::as_const(string));
        QUtf8StringViewOverloadResolution::test(std::move(string));
    }
}

#ifdef __cpp_char8_t
const char8_t char8ArrayOfUnknownSize[] = u8"abc\0def";
#endif
const char charArrayOfUnknownSize[] = "abc\0def";

void tst_QUtf8StringView::midLeftRight()
{
    const QUtf8StringView utf8("Hello World");
    QCOMPARE(utf8.mid(0),              utf8);
    QCOMPARE(utf8.mid(0, utf8.size()), utf8);
    QCOMPARE(utf8.left(utf8.size()),   utf8);
    QCOMPARE(utf8.right(utf8.size()),  utf8);

    QCOMPARE(utf8.mid(6), QUtf8StringView("World"));
    QCOMPARE(utf8.mid(6, 5), QUtf8StringView("World"));
    QCOMPARE(utf8.right(5), QUtf8StringView("World"));

    QCOMPARE(utf8.mid(6, 1), QUtf8StringView("W"));
    QCOMPARE(utf8.right(5).left(1), QUtf8StringView("W"));

    QCOMPARE(utf8.left(5), QUtf8StringView("Hello"));
}

void tst_QUtf8StringView::nullString()
{
    // default ctor
    {
        QUtf8StringView utf8;
        QCOMPARE(as_const_void_star(utf8.data()), nullptr);
        QCOMPARE(utf8.size(), 0);

        QString s = utf8.toString(); //no operator=
        QVERIFY(s.isNull());
    }

    // from nullptr
    {
        const char *null = nullptr;
        QUtf8StringView utf8(null);
        QCOMPARE(as_const_void_star(utf8.data()), nullptr);
        QCOMPARE(utf8.size(), 0);

        QString s = utf8.toString(); //no operator=
        QVERIFY(s.isNull());
    }

    // from null QByteArray
    {
        const QByteArray null;
        QVERIFY(null.isNull());

        QUtf8StringView utf8(null);
        QCOMPARE(as_const_void_star(utf8.data()), nullptr);
        QCOMPARE(utf8.size(), 0);

        QString s = utf8.toString(); //no operator=
        QVERIFY(s.isNull());
    }
}

void tst_QUtf8StringView::emptyString()
{
    {
        const char *empty = "";
        QUtf8StringView utf8(empty);
        QCOMPARE(as_const_void_star(utf8.data()), as_const_void_star(empty));
        QCOMPARE(utf8.size(), 0);

        QString s = utf8.toString(); //no operator=
        QVERIFY(s.isEmpty());
        QVERIFY(!s.isNull());
    }

    {
        const char *notEmpty = "foo";
        QUtf8StringView utf8(notEmpty, qsizetype(0));
        QCOMPARE(as_const_void_star(utf8.data()), as_const_void_star(notEmpty));
        QCOMPARE(utf8.size(), 0);

        QString s = utf8.toString(); //no operator=
        QVERIFY(s.isEmpty());
        QVERIFY(!s.isNull());
    }

    {
        const QByteArray empty = "";
        QUtf8StringView utf8(empty);
        QCOMPARE(as_const_void_star(utf8.data()), as_const_void_star(empty.constData()));
        QCOMPARE(utf8.size(), 0);

        QString s = utf8.toString(); //no operator=
        QVERIFY(s.isEmpty());
        QVERIFY(!s.isNull());
    }
}

void tst_QUtf8StringView::iterators()
{
    QUtf8StringView hello("hello");
    QUtf8StringView olleh("olleh");

    QVERIFY(std::equal(hello.begin(), hello.end(),
                       olleh.rbegin()));
    QVERIFY(std::equal(hello.rbegin(), hello.rend(),
                       QT_MAKE_CHECKED_ARRAY_ITERATOR(olleh.begin(), olleh.size())));

    QVERIFY(std::equal(hello.cbegin(), hello.cend(),
                       olleh.rbegin()));
    QVERIFY(std::equal(hello.crbegin(), hello.crend(),
                       QT_MAKE_CHECKED_ARRAY_ITERATOR(olleh.begin(), olleh.size())));
}

void tst_QUtf8StringView::relationalOperators_data()
{
    QTest::addColumn<QUtf8StringView>("lhs");
    QTest::addColumn<int>("lhsOrderNumber");
    QTest::addColumn<QUtf8StringView>("rhs");
    QTest::addColumn<int>("rhsOrderNumber");

    struct Data {
        QUtf8StringView l1;
        int order;
    } data[] = {
        { QUtf8StringView(),     0 },
        { QUtf8StringView(""),   0 },
        { QUtf8StringView("a"),  1 },
        { QUtf8StringView("aa"), 2 },
        { QUtf8StringView("b"),  3 },
    };

    for (Data *lhs = data; lhs != data + sizeof data / sizeof *data; ++lhs) {
        for (Data *rhs = data; rhs != data + sizeof data / sizeof *data; ++rhs) {
            QUtf8StringView l = { lhs->l1 }, r = { rhs->l1 };
            QTest::addRow("\"%s\" <> \"%s\"",
                          lhs->l1.data() ? lhs->l1.data() : "nullptr",
                          rhs->l1.data() ? rhs->l1.data() : "nullptr")
                << l << lhs->order << r << rhs->order;
        }
    }
}

void tst_QUtf8StringView::relationalOperators()
{
    QFETCH(QUtf8StringView, lhs);
    QFETCH(int, lhsOrderNumber);
    QFETCH(QUtf8StringView, rhs);
    QFETCH(int, rhsOrderNumber);

#define CHECK(op) \
    QCOMPARE(lhs op rhs, lhsOrderNumber op rhsOrderNumber) \
    /*end*/
    CHECK(==);
    CHECK(!=);
    CHECK(< );
    CHECK(> );
    CHECK(<=);
    CHECK(>=);
#undef CHECK
}

QTEST_APPLESS_MAIN(tst_QUtf8StringView)

#include "tst_qutf8stringview.moc"
