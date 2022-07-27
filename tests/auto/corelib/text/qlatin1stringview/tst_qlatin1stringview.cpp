// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include <QString>

// Preserve QLatin1StringView-ness (QVariant(QLatin1StringView) creates a QVariant::String):
struct QLatin1StringViewContainer {
    QLatin1StringView l1;
};
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(QLatin1StringViewContainer, Q_RELOCATABLE_TYPE);
QT_END_NAMESPACE
Q_DECLARE_METATYPE(QLatin1StringViewContainer)

class tst_QLatin1StringView : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constExpr();
    void construction();
    void userDefinedLiterals();
    void at();
    void arg() const;
    void midLeftRight();
    void nullString();
    void emptyString();
    void iterators();
    void relationalOperators_data();
    void relationalOperators();
    void count();
    void indexOf_data();
    void indexOf();
};

void tst_QLatin1StringView::constExpr()
{
    // compile-time checks
    {
        constexpr QLatin1StringView l1s;
        static_assert(l1s.size() == 0);
        static_assert(l1s.isNull());
        static_assert(l1s.empty());
        static_assert(l1s.isEmpty());
        static_assert(l1s.latin1() == nullptr);

        constexpr QLatin1StringView l1s2(l1s.latin1(), l1s.latin1() + l1s.size());
        static_assert(l1s2.isNull());
        static_assert(l1s2.empty());
    }
    {
        constexpr QLatin1StringView l1s = nullptr;
        static_assert(l1s.size() == 0);
        static_assert(l1s.isNull());
        static_assert(l1s.empty());
        static_assert(l1s.isEmpty());
        static_assert(l1s.latin1() == nullptr);
    }
    {
        constexpr QLatin1StringView l1s("");
        static_assert(l1s.size() == 0);
        static_assert(!l1s.isNull());
        static_assert(l1s.empty());
        static_assert(l1s.isEmpty());
        static_assert(l1s.latin1() != nullptr);

        constexpr QLatin1StringView l1s2(l1s.latin1(), l1s.latin1() + l1s.size());
        static_assert(!l1s2.isNull());
        static_assert(l1s2.empty());
    }
    {
        static_assert(QLatin1StringView("Hello").size() == 5);
        constexpr QLatin1StringView l1s("Hello");
        static_assert(l1s.size() == 5);
        static_assert(!l1s.empty());
        static_assert(!l1s.isEmpty());
        static_assert(!l1s.isNull());
        static_assert(*l1s.latin1() == 'H');
        static_assert(l1s[0]      == QLatin1Char('H'));
        static_assert(l1s.at(0)   == QLatin1Char('H'));
        static_assert(l1s.front() == QLatin1Char('H'));
        static_assert(l1s.first() == QLatin1Char('H'));
        static_assert(l1s[4]      == QLatin1Char('o'));
        static_assert(l1s.at(4)   == QLatin1Char('o'));
        static_assert(l1s.back()  == QLatin1Char('o'));
        static_assert(l1s.last()  == QLatin1Char('o'));

        constexpr QLatin1StringView l1s2(l1s.latin1(), l1s.latin1() + l1s.size());
        static_assert(!l1s2.isNull());
        static_assert(!l1s2.empty());
        static_assert(l1s2.size() == 5);
    }
}

void tst_QLatin1StringView::construction()
{
    {
        const char str[6] = "hello";
        QLatin1StringView l1s(str);
        QCOMPARE(l1s.size(), 5);
        QCOMPARE(l1s.latin1(), reinterpret_cast<const void *>(&str[0]));
        QCOMPARE(l1s.latin1(), "hello");

        QLatin1StringView s1 = {str, 5};
        QCOMPARE(s1, l1s);
        QLatin1StringView s2 = {str, str + 5};
        QCOMPARE(s2, l1s);

        QByteArrayView helloView(str);
        helloView = helloView.first(4);
        l1s = QLatin1StringView(helloView);
        QCOMPARE(l1s.latin1(), helloView.data());
        QCOMPARE(l1s.latin1(), reinterpret_cast<const void *>(helloView.data()));
        QCOMPARE(l1s.size(), helloView.size());
    }

    {
        const QByteArray helloArray("hello");
        QLatin1StringView l1s(helloArray);
        QCOMPARE(l1s.latin1(), helloArray.data());
        QCOMPARE(l1s.size(), helloArray.size());

        QByteArrayView helloView(helloArray);
        helloView = helloView.first(4);
        l1s = QLatin1StringView(helloView);
        QCOMPARE(l1s.latin1(), helloView.data());
        QCOMPARE(l1s.size(), helloView.size());
    }
}

void tst_QLatin1StringView::userDefinedLiterals()
{
    {
        using namespace Qt::Literals::StringLiterals;

        auto str = "abcd"_L1;
        static_assert(std::is_same_v<decltype(str), QLatin1StringView>);
        QCOMPARE(str.size(), 4);
        QCOMPARE(str, QLatin1StringView("abcd"));
        QCOMPARE(str.latin1(), "abcd");
        QCOMPARE("abcd"_L1, str.latin1());
        QCOMPARE("M\xE5rten"_L1, QLatin1StringView("M\xE5rten"));

        auto ch = 'a'_L1;
        static_assert(std::is_same_v<decltype(ch), QLatin1Char>);
        QCOMPARE(ch, QLatin1Char('a'));
        QCOMPARE(ch.toLatin1(), 'a');
        QCOMPARE('a'_L1, ch.toLatin1());
        QCOMPARE('\xE5'_L1, QLatin1Char('\xE5'));
    }
    {
        using namespace Qt::Literals;

        auto str = "abcd"_L1;
        static_assert(std::is_same_v<decltype(str), QLatin1StringView>);
        QCOMPARE(str, QLatin1StringView("abcd"));

        auto ch = 'a'_L1;
        static_assert(std::is_same_v<decltype(ch), QLatin1Char>);
        QCOMPARE(ch, QLatin1Char('a'));
    }
    {
        using namespace Qt;

        auto str = "abcd"_L1;
        static_assert(std::is_same_v<decltype(str), QLatin1StringView>);
        QCOMPARE(str, QLatin1StringView("abcd"));

        auto ch = 'a'_L1;
        static_assert(std::is_same_v<decltype(ch), QLatin1Char>);
        QCOMPARE(ch, QLatin1Char('a'));
    }
}

void tst_QLatin1StringView::at()
{
    const QLatin1StringView l1("Hello World");
    QCOMPARE(l1.at(0), QLatin1Char('H'));
    QCOMPARE(l1.at(l1.size() - 1), QLatin1Char('d'));
    QCOMPARE(l1[0], QLatin1Char('H'));
    QCOMPARE(l1[l1.size() - 1], QLatin1Char('d'));
}

void tst_QLatin1StringView::arg() const
{
#define CHECK1(pattern, arg1, expected) \
    do { \
        auto p = QLatin1StringView(pattern); \
        QCOMPARE(p.arg(QLatin1StringView(arg1)), expected); \
        QCOMPARE(p.arg(u"" arg1), expected); \
        QCOMPARE(p.arg(QStringLiteral(arg1)), expected); \
        QCOMPARE(p.arg(QString(QLatin1StringView(arg1))), expected); \
    } while (false) \
    /*end*/
#define CHECK2(pattern, arg1, arg2, expected) \
    do { \
        auto p = QLatin1StringView(pattern); \
        QCOMPARE(p.arg(QLatin1StringView(arg1), QLatin1StringView(arg2)), expected); \
        QCOMPARE(p.arg(u"" arg1, QLatin1StringView(arg2)), expected); \
        QCOMPARE(p.arg(QLatin1StringView(arg1), u"" arg2), expected); \
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

    QCOMPARE(QLatin1StringView(" %2 %2 %1 %3 ").arg(QLatin1Char('c'), QChar::CarriageReturn, u'C'),
             " \r \r c C ");
}

void tst_QLatin1StringView::midLeftRight()
{
    const QLatin1StringView l1("Hello World");
    QCOMPARE(l1.mid(0),            l1);
    QCOMPARE(l1.mid(0, l1.size()), l1);
    QCOMPARE(l1.left(l1.size()),   l1);
    QCOMPARE(l1.right(l1.size()),  l1);

    QCOMPARE(l1.mid(6), QLatin1StringView("World"));
    QCOMPARE(l1.mid(6, 5), QLatin1StringView("World"));
    QCOMPARE(l1.right(5), QLatin1StringView("World"));

    QCOMPARE(l1.mid(6, 1), QLatin1StringView("W"));
    QCOMPARE(l1.right(5).left(1), QLatin1StringView("W"));

    QCOMPARE(l1.left(5), QLatin1StringView("Hello"));
}

void tst_QLatin1StringView::nullString()
{
    // default ctor
    {
        QLatin1StringView l1;
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(nullptr));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QVERIFY(s.isNull());
    }

    // from nullptr
    {
        const char *null = nullptr;
        QLatin1StringView l1(null);
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(nullptr));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QVERIFY(s.isNull());
    }

    // from null QByteArray
    {
        const QByteArray null;
        QVERIFY(null.isNull());

        QLatin1StringView l1(null);
        QEXPECT_FAIL("", "null QByteArrays become non-null QLatin1Strings...", Continue);
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(nullptr));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QEXPECT_FAIL("", "null QByteArrays become non-null QLatin1Strings become non-null QStrings...", Continue);
        QVERIFY(s.isNull());
    }
}

void tst_QLatin1StringView::emptyString()
{
    {
        const char *empty = "";
        QLatin1StringView l1(empty);
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(empty));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QVERIFY(s.isEmpty());
        QVERIFY(!s.isNull());
    }

    {
        const char *notEmpty = "foo";
        QLatin1StringView l1(notEmpty, qsizetype(0));
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(notEmpty));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QVERIFY(s.isEmpty());
        QVERIFY(!s.isNull());
    }

    {
        const QByteArray empty = "";
        QLatin1StringView l1(empty);
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(empty.constData()));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QVERIFY(s.isEmpty());
        QVERIFY(!s.isNull());
    }
}

void tst_QLatin1StringView::iterators()
{
    QLatin1StringView hello("hello");
    QLatin1StringView olleh("olleh");

    QVERIFY(std::equal(hello.begin(), hello.end(),
                       olleh.rbegin()));
    QVERIFY(std::equal(hello.rbegin(), hello.rend(),
                       QT_MAKE_CHECKED_ARRAY_ITERATOR(olleh.begin(), olleh.size())));

    QVERIFY(std::equal(hello.cbegin(), hello.cend(),
                       olleh.rbegin()));
    QVERIFY(std::equal(hello.crbegin(), hello.crend(),
                       QT_MAKE_CHECKED_ARRAY_ITERATOR(olleh.begin(), olleh.size())));
}

void tst_QLatin1StringView::relationalOperators_data()
{
    QTest::addColumn<QLatin1StringViewContainer>("lhs");
    QTest::addColumn<int>("lhsOrderNumber");
    QTest::addColumn<QLatin1StringViewContainer>("rhs");
    QTest::addColumn<int>("rhsOrderNumber");

    struct Data {
        QLatin1StringView l1;
        int order;
    } data[] = {
        { QLatin1StringView(),     0 },
        { QLatin1StringView(""),   0 },
        { QLatin1StringView("a"),  1 },
        { QLatin1StringView("aa"), 2 },
        { QLatin1StringView("b"),  3 },
    };

    for (Data *lhs = data; lhs != data + sizeof data / sizeof *data; ++lhs) {
        for (Data *rhs = data; rhs != data + sizeof data / sizeof *data; ++rhs) {
            QLatin1StringViewContainer l = { lhs->l1 }, r = { rhs->l1 };
            QTest::addRow("\"%s\" <> \"%s\"",
                          lhs->l1.data() ? lhs->l1.data() : "nullptr",
                          rhs->l1.data() ? rhs->l1.data() : "nullptr")
                << l << lhs->order << r << rhs->order;
        }
    }
}

void tst_QLatin1StringView::relationalOperators()
{
    QFETCH(QLatin1StringViewContainer, lhs);
    QFETCH(int, lhsOrderNumber);
    QFETCH(QLatin1StringViewContainer, rhs);
    QFETCH(int, rhsOrderNumber);

#define CHECK(op) \
    QCOMPARE(lhs.l1 op rhs.l1, lhsOrderNumber op rhsOrderNumber) \
    /*end*/
    CHECK(==);
    CHECK(!=);
    CHECK(< );
    CHECK(> );
    CHECK(<=);
    CHECK(>=);
#undef CHECK
}

void tst_QLatin1StringView::count()
{
    QLatin1StringView a("ABCDEFGHIEfGEFG");
    QCOMPARE(a.size(), 15);
    QCOMPARE(a.count('A'), 1);
    QCOMPARE(a.count('Z'), 0);
    QCOMPARE(a.count('E'), 3);
    QCOMPARE(a.count('F'), 2);
    QCOMPARE(a.count('F', Qt::CaseInsensitive), 3);
    QCOMPARE(a.count(QLatin1StringView("FG")), 2);
    QCOMPARE(a.count(QLatin1StringView("FG"), Qt::CaseInsensitive), 3);
    QCOMPARE(a.count(QLatin1StringView(), Qt::CaseInsensitive), 16);
    QCOMPARE(a.count(QLatin1StringView(""), Qt::CaseInsensitive), 16);

    QLatin1StringView nullStr;
    QCOMPARE(nullStr.count('A'), 0);
    QCOMPARE(nullStr.count(QLatin1StringView("AB")), 0);
    QCOMPARE(nullStr.count(QLatin1StringView()), 1);
    QCOMPARE(nullStr.count(QLatin1StringView("")), 1);

    QLatin1StringView emptyStr("");
    QCOMPARE(emptyStr.count('A'), 0);
    QCOMPARE(emptyStr.count(QLatin1StringView("AB")), 0);
    QCOMPARE(emptyStr.count(QLatin1StringView()), 1);
    QCOMPARE(emptyStr.count(QLatin1StringView("")), 1);

    using namespace Qt::Literals::StringLiterals;
    QCOMPARE("a\0b"_L1.count(QChar::SpecialCharacter::LineSeparator), 0);
}

void tst_QLatin1StringView::indexOf_data()
{
    using namespace Qt::Literals::StringLiterals;

    QTest::addColumn<QLatin1StringView>("needle");
    QTest::addColumn<QLatin1StringView>("haystack");
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("indexCaseSensitive");
    QTest::addColumn<int>("indexCaseInsensitive");

    // Should never trigger Boyer Moore algorithm
    QTest::newRow("Single letter match start")
            << QLatin1StringView("A") << QLatin1StringView("ABCDEF") << 0 << 0 << 0;
    QTest::newRow("Single letter match second letter")
            << QLatin1StringView("B") << QLatin1StringView("ABCDEF") << 0 << 1 << 1;
    QTest::newRow("Single letter mismatch")
            << QLatin1StringView("G") << QLatin1StringView("ABCDEF") << 0 << -1 << -1;
    QTest::newRow("Single letter case sensitive start")
            << QLatin1StringView("a") << QLatin1StringView("ABCDEF") << 0 << -1 << 0;
    QTest::newRow("Single letter case sensitive end")
            << QLatin1StringView("f") << QLatin1StringView("ABCDEF") << 0 << -1 << 5;
    QTest::newRow("Single letter different match depending on case")
            << QLatin1StringView("a") << QLatin1StringView("ABCabc") << 0 << 3 << 0;
    QTest::newRow("Single letter different match depending on case from 2")
            << QLatin1StringView("a") << QLatin1StringView("ABCABCabc") << 2 << 6 << 3;
    QTest::newRow("Single letter negative from")
            << QLatin1StringView("a") << QLatin1StringView("abcabc") << -3 << 3 << 3;
    QTest::newRow("Single letter non-ASCII") // searching for "ø" in "Øø"
            << "\xf8"_L1
            << "\xd8\xf8"_L1 << 0 << 1 << 0;
    QTest::newRow("Single uppercase letter")
            << QLatin1StringView("A") << QLatin1StringView("aA") << 0 << 1 << 0;

    // Might trigger Boyer Moore algorithm
    QTest::newRow("Small match start")
            << QLatin1StringView("ABC") << QLatin1StringView("ABCDEF") << 0 << 0 << 0;
    QTest::newRow("Small match second letter")
            << QLatin1StringView("BCD") << QLatin1StringView("ABCDEF") << 0 << 1 << 1;
    QTest::newRow("Small mismatch")
            << QLatin1StringView("EFG") << QLatin1StringView("ABCDEF") << 0 << -1 << -1;
    QTest::newRow("Small case sensitive start")
            << QLatin1StringView("abc") << QLatin1StringView("ABCDEF") << 0 << -1 << 0;
    QTest::newRow("Small case sensitive end")
            << QLatin1StringView("DEF") << QLatin1StringView("abcdef") << 0 << -1 << 3;
    QTest::newRow("Small different match depending on case")
            << QLatin1StringView("abcabc") << QLatin1StringView("!!ABCabcabcABC") << 0 << 5 << 2;
    QTest::newRow("Small different match depending on case from 2")
            << QLatin1StringView("abcabc") << QLatin1StringView("ABCABCabcabcABC") << 2 << 6 << 3;
    QTest::newRow("Small negative from") << QLatin1StringView("negative")
                                         << QLatin1StringView("negativenegative") << -8 << 8 << 8;
    QTest::newRow("Small non-ASCII") // searching for "løve" in "LØVEløve"
            << "l\xf8ve"_L1
            << "L\xd8VEl\xf8ve"_L1 << 0 << 4 << 0;
    QTest::newRow("Small skip test")
            << QLatin1StringView("ABBB") << QLatin1StringView("ABABBB") << 0 << 2 << 2;
    QTest::newRow("Small uppercase needle")
            << QLatin1StringView("ABCDEF") << QLatin1StringView("abcdefABCDEF") << 0 << 6 << 0;

    // Should trigger Boyer Moore algorithm
    QTest::newRow("Medium match start")
            << QLatin1StringView("ABCDEFGHIJKLMNOP")
            << QLatin1StringView("ABCDEFGHIJKLMNOPQRSTUVWXYZ") << 0 << 0 << 0;
    QTest::newRow("Medium match second letter")
            << QLatin1StringView("BCDEFGHIJKLMNOPQ")
            << QLatin1StringView("ABCDEFGHIJKLMNOPQRSTUVWXYZ") << 0 << 1 << 1;
    QTest::newRow("Medium mismatch")
            << QLatin1StringView("PONMLKJIHGFEDCBA")
            << QLatin1StringView("ABCDEFGHIJKLMNOPQRSTUVWXYZ") << 0 << -1 << -1;
    QTest::newRow("Medium case sensitive start")
            << QLatin1StringView("abcdefghijklmnopq")
            << QLatin1StringView("ABCDEFGHIJKLMNOPQRSTUVWXYZ") << 0 << -1 << 0;
    QTest::newRow("Medium case sensitive second letter")
            << QLatin1StringView("BCDEFGHIJKLMNOPQR")
            << QLatin1StringView("abcdefghijklmnopqrstuvxyz") << 0 << -1 << 1;
    QTest::newRow("Medium different match depending on case")
            << QLatin1StringView("testingtesting")
            << QLatin1StringView("TESTINGtestingtestingTESTING") << 0 << 7 << 0;
    QTest::newRow("Medium different match depending on case from 2")
            << QLatin1StringView("testingtesting")
            << QLatin1StringView("TESTINGTESTINGtestingtestingTESTING") << 2 << 14 << 7;
    QTest::newRow("Medium negative from")
            << QLatin1StringView("abcdefghijklmnop")
            << QLatin1StringView("abcdefghijklmnopabcdefghijklmnop") << -16 << 16 << 16;
    QTest::newRow("Medium non-ASCII") // searching for "dampfschiffahrtsgesellschaftskapitän"
            << "dampfschiffahrtsgesellschaftskapit\xe4n"_L1
            << "DAMPFSCHIFFAHRTSGESELLSCHAFTSKAPIT\xc4Ndampfschiffahrtsgesellschaftskapit\xe4n"_L1
            << 0 << 36 << 0;
    QTest::newRow("Medium skip test") << QLatin1StringView("ABBBBBBBBBBBBBBB")
                                      << QLatin1StringView("ABABBBBBBBBBBBBBBB") << 0 << 2 << 2;
}

void tst_QLatin1StringView::indexOf()
{
    QFETCH(QLatin1StringView, needle);
    QFETCH(QLatin1StringView, haystack);
    QFETCH(int, from);
    QFETCH(int, indexCaseSensitive);
    QFETCH(int, indexCaseInsensitive);
    QCOMPARE(haystack.indexOf(needle, from, Qt::CaseSensitive), (qsizetype)indexCaseSensitive);
    QCOMPARE(haystack.indexOf(needle, from, Qt::CaseInsensitive), (qsizetype)indexCaseInsensitive);
}

QTEST_APPLESS_MAIN(tst_QLatin1StringView)

#include "tst_qlatin1stringview.moc"
