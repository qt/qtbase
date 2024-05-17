// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtCore/QLatin1StringMatcher>
#include <QtCore/QStaticLatin1StringMatcher>

#include <numeric>
#include <string>

#include <thread>

// COM interface
#if defined(interface)
#    undef interface
#endif

using namespace Qt::Literals::StringLiterals;

class tst_QLatin1StringMatcher : public QObject
{
    Q_OBJECT

private slots:
    void overloads();
    void staticOverloads();
    void staticOverloads_QStringViewHaystack();
    void interface();
    void indexIn();
    void haystacksWithMoreThan4GiBWork();
    void staticLatin1StringMatcher();
};

void tst_QLatin1StringMatcher::overloads()
{
    QLatin1StringView hello = "hello"_L1;
    QByteArray hello2B = QByteArrayView(hello).toByteArray().repeated(2);
    QLatin1StringView hello2(hello2B);
    {
        QLatin1StringMatcher m("hello"_L1, Qt::CaseSensitive);
        QCOMPARE(m.pattern(), "hello"_L1);
        QCOMPARE(m.indexIn("hello"_L1), 0);
        QCOMPARE(m.indexIn("Hello"_L1), -1);
        QCOMPARE(m.indexIn("Hellohello"_L1), 5);
        QCOMPARE(m.indexIn("helloHello"_L1), 0);
        QCOMPARE(m.indexIn("helloHello"_L1, 1), -1);

        QCOMPARE(m.indexIn(u"hello"), 0);
        QCOMPARE(m.indexIn(u"Hello"), -1);
        QCOMPARE(m.indexIn(u"Hellohello"), 5);
        QCOMPARE(m.indexIn(u"helloHello"), 0);
        QCOMPARE(m.indexIn(u"helloHello", 1), -1);
    }
    {
        QLatin1StringMatcher m("Hello"_L1, Qt::CaseSensitive);
        QCOMPARE(m.pattern(), "Hello"_L1);
        QCOMPARE(m.indexIn("hello"_L1), -1);
        QCOMPARE(m.indexIn("Hello"_L1), 0);
        QCOMPARE(m.indexIn("Hellohello"_L1), 0);
        QCOMPARE(m.indexIn("helloHello"_L1), 5);
        QCOMPARE(m.indexIn("helloHello"_L1, 6), -1);

        QCOMPARE(m.indexIn(u"hello"), -1);
        QCOMPARE(m.indexIn(u"Hello"), 0);
        QCOMPARE(m.indexIn(u"Hellohello"), 0);
        QCOMPARE(m.indexIn(u"helloHello"), 5);
        QCOMPARE(m.indexIn(u"helloHello", 6), -1);
    }
    {
        QLatin1StringMatcher m("hello"_L1, Qt::CaseInsensitive);
        QCOMPARE(m.pattern(), "hello"_L1);
        QCOMPARE(m.indexIn("hello"_L1), 0);
        QCOMPARE(m.indexIn("Hello"_L1), 0);
        QCOMPARE(m.indexIn("Hellohello"_L1), 0);
        QCOMPARE(m.indexIn("helloHello"_L1), 0);
        QCOMPARE(m.indexIn("helloHello"_L1, 1), 5);
        QCOMPARE(m.indexIn("helloHello"_L1, 6), -1);

        QCOMPARE(m.indexIn(u"hello"), 0);
        QCOMPARE(m.indexIn(u"Hello"), 0);
        QCOMPARE(m.indexIn(u"Hellohello"), 0);
        QCOMPARE(m.indexIn(u"helloHello"), 0);
        QCOMPARE(m.indexIn(u"helloHello", 1), 5);
        QCOMPARE(m.indexIn(u"helloHello", 6), -1);
    }
    {
        QLatin1StringMatcher m("Hello"_L1, Qt::CaseInsensitive);
        QCOMPARE(m.pattern(), "Hello"_L1);
        QCOMPARE(m.indexIn("hello"_L1), 0);
        QCOMPARE(m.indexIn("Hello"_L1), 0);
        QCOMPARE(m.indexIn("Hellohello"_L1), 0);
        QCOMPARE(m.indexIn("helloHello"_L1), 0);
        QCOMPARE(m.indexIn("helloHello"_L1, 1), 5);
        QCOMPARE(m.indexIn("helloHello"_L1, 6), -1);

        QCOMPARE(m.indexIn(u"hello"), 0);
        QCOMPARE(m.indexIn(u"Hello"), 0);
        QCOMPARE(m.indexIn(u"Hellohello"), 0);
        QCOMPARE(m.indexIn(u"helloHello"), 0);
        QCOMPARE(m.indexIn(u"helloHello", 1), 5);
        QCOMPARE(m.indexIn(u"helloHello", 6), -1);
    }
    {
        QLatin1StringMatcher m(hello, Qt::CaseSensitive);
        QCOMPARE(m.pattern(), "hello"_L1);
        QCOMPARE(m.indexIn(hello), 0);
        QCOMPARE(m.indexIn(hello, 1), -1);
        QCOMPARE(m.indexIn(hello2, 1), hello.size());
        QCOMPARE(m.indexIn(hello2, 6), -1);

        QCOMPARE(m.indexIn(QString::fromLatin1(hello)), 0);
        QCOMPARE(m.indexIn(QString::fromLatin1(hello), 1), -1);
        QCOMPARE(m.indexIn(QString::fromLatin1(hello2), 1), hello.size());
        QCOMPARE(m.indexIn(QString::fromLatin1(hello2), 6), -1);
    }
}

void tst_QLatin1StringMatcher::staticOverloads()
{
#ifdef QT_STATIC_BOYER_MOORE_NOT_SUPPORTED
    QSKIP("Test is only valid on an OS that supports static latin1 string matcher");
#else
    constexpr QLatin1StringView hello = "hello"_L1;
    QByteArray hello2B = QByteArrayView(hello).toByteArray().repeated(2);
    QLatin1StringView hello2(hello2B);
    {
        static constexpr auto m = qMakeStaticCaseSensitiveLatin1StringMatcher("hel");
        QCOMPARE(m.indexIn("hello"_L1), 0);
        QCOMPARE(m.indexIn("Hello"_L1), -1);
        QCOMPARE(m.indexIn("Hellohello"_L1), 5);
        QCOMPARE(m.indexIn("helloHello"_L1), 0);
        QCOMPARE(m.indexIn("he"_L1), -1);
        QCOMPARE(m.indexIn("hel"_L1), 0);
        QCOMPARE(m.indexIn(hello), 0);
        QCOMPARE(m.indexIn(hello, 1), -1); // from is 1
        QCOMPARE(m.indexIn(hello2, 2), hello.size()); // from is 2
        QCOMPARE(m.indexIn(hello2, 3), hello.size()); // from is 3
        QCOMPARE(m.indexIn(hello2, 6), -1); // from is 6
        static_assert(m.indexIn("hello"_L1) == 0);
        static_assert(m.indexIn("Hello"_L1) == -1);
        static_assert(m.indexIn("Hellohello"_L1) == 5);
        static_assert(m.indexIn("helloHello"_L1) == 0);
        static_assert(m.indexIn("he"_L1) == -1);
        static_assert(m.indexIn("hel"_L1) == 0);
        static_assert(m.indexIn("hellohello"_L1, 2) == 5); // from is 2
        static_assert(m.indexIn("hellohello"_L1, 3) == 5); // from is 3
        static_assert(m.indexIn("hellohello"_L1, 6) == -1); // from is 6
    }
    {
        static constexpr auto m = qMakeStaticCaseSensitiveLatin1StringMatcher("Hel");
        QCOMPARE(m.indexIn("hello"_L1), -1);
        QCOMPARE(m.indexIn("Hello"_L1), 0);
        QCOMPARE(m.indexIn("Hellohello"_L1), 0);
        QCOMPARE(m.indexIn("helloHello"_L1), 5);
        QCOMPARE(m.indexIn("helloHello"_L1, 6), -1);
        QCOMPARE(m.indexIn("He"_L1), -1);
        QCOMPARE(m.indexIn("Hel"_L1), 0);
        QCOMPARE(m.indexIn(hello), -1);
        QCOMPARE(m.indexIn(hello2, 2), -1); // from is 2
        QCOMPARE(m.indexIn(hello2, 6), -1); // from is 6
        static_assert(m.indexIn("hello"_L1) == -1);
        static_assert(m.indexIn("Hello"_L1) == 0);
        static_assert(m.indexIn("Hellohello"_L1) == 0);
        static_assert(m.indexIn("helloHello"_L1) == 5);
        static_assert(m.indexIn("helloHello"_L1, 6) == -1);
        static_assert(m.indexIn("He"_L1) == -1);
        static_assert(m.indexIn("Hel"_L1) == 0);
        static_assert(m.indexIn("hellohello"_L1, 2) == -1); // from is 2
        static_assert(m.indexIn("hellohello"_L1, 6) == -1); // from is 6
    }
    {
        static constexpr auto m = qMakeStaticCaseInsensitiveLatin1StringMatcher("hel");
        QCOMPARE(m.indexIn("hello"_L1), 0);
        QCOMPARE(m.indexIn("Hello"_L1), 0);
        QCOMPARE(m.indexIn("Hellohello"_L1), 0);
        QCOMPARE(m.indexIn("helloHello"_L1), 0);
        QCOMPARE(m.indexIn("he"_L1), -1);
        QCOMPARE(m.indexIn("hel"_L1), 0);
        QCOMPARE(m.indexIn(hello), 0);
        QCOMPARE(m.indexIn(hello, 1), -1);
        QCOMPARE(m.indexIn(hello2, 2), hello.size()); // from is 2
        QCOMPARE(m.indexIn(hello2, 3), hello.size()); // from is 3
        QCOMPARE(m.indexIn(hello2, 6), -1); // from is 6
        static_assert(m.indexIn("hello"_L1) == 0);
        static_assert(m.indexIn("Hello"_L1) == 0);
        static_assert(m.indexIn("Hellohello"_L1) == 0);
        static_assert(m.indexIn("helloHello"_L1) == 0);
        static_assert(m.indexIn("he"_L1) == -1);
        static_assert(m.indexIn("hel"_L1) == 0);
        static_assert(m.indexIn("hellohello"_L1, 2) == 5); // from is 2
        static_assert(m.indexIn("hellohello"_L1, 3) == 5); // from is 3
        static_assert(m.indexIn("hellohello"_L1, 6) == -1); // from is 6
    }
    {
        static constexpr auto m = qMakeStaticCaseInsensitiveLatin1StringMatcher("Hel");
        QCOMPARE(m.indexIn("hello"_L1), 0);
        QCOMPARE(m.indexIn("Hello"_L1), 0);
        QCOMPARE(m.indexIn("Hellohello"_L1), 0);
        QCOMPARE(m.indexIn("helloHello"_L1), 0);
        QCOMPARE(m.indexIn("he"_L1), -1);
        QCOMPARE(m.indexIn("hel"_L1), 0);
        QCOMPARE(m.indexIn(hello), 0);
        QCOMPARE(m.indexIn(hello, 1), -1);
        QCOMPARE(m.indexIn(hello2, 2), hello.size()); // from is 2
        QCOMPARE(m.indexIn(hello2, 3), hello.size()); // from is 3
        QCOMPARE(m.indexIn(hello2, 6), -1); // from is 6
        static_assert(m.indexIn("hello"_L1) == 0);
        static_assert(m.indexIn("Hello"_L1) == 0);
        static_assert(m.indexIn("Hellohello"_L1) == 0);
        static_assert(m.indexIn("helloHello"_L1) == 0);
        static_assert(m.indexIn("he"_L1) == -1);
        static_assert(m.indexIn("hel"_L1) == 0);
        static_assert(m.indexIn("hellohello"_L1, 2) == 5); // from is 2
        static_assert(m.indexIn("hellohello"_L1, 3) == 5); // from is 3
        static_assert(m.indexIn("hellohello"_L1, 6) == -1); // from is 6
    }
    {
        static constexpr auto m = qMakeStaticCaseInsensitiveLatin1StringMatcher("b\xF8");
        QCOMPARE(m.indexIn("B\xD8"_L1), 0);
        QCOMPARE(m.indexIn("B\xF8"_L1), 0);
        QCOMPARE(m.indexIn("b\xD8"_L1), 0);
        QCOMPARE(m.indexIn("b\xF8"_L1), 0);
        QCOMPARE(m.indexIn("b\xF8lle"_L1), 0);
        QCOMPARE(m.indexIn("m\xF8lle"_L1), -1);
        QCOMPARE(m.indexIn("Si b\xF8"_L1), 3);
    }
    {
        static constexpr auto m = qMakeStaticCaseSensitiveLatin1StringMatcher("b\xF8");
        QCOMPARE(m.indexIn("B\xD8"_L1), -1);
        QCOMPARE(m.indexIn("B\xF8"_L1), -1);
        QCOMPARE(m.indexIn("b\xD8"_L1), -1);
        QCOMPARE(m.indexIn("b\xF8"_L1), 0);
        QCOMPARE(m.indexIn("b\xF8lle"_L1), 0);
        QCOMPARE(m.indexIn("m\xF8lle"_L1), -1);
        QCOMPARE(m.indexIn("Si b\xF8"_L1), 3);
    }
#endif
}

void tst_QLatin1StringMatcher::staticOverloads_QStringViewHaystack()
{
#ifdef QT_STATIC_BOYER_MOORE_NOT_SUPPORTED
    QSKIP("Test is only valid on an OS that supports static latin1 string matcher");
#else
    constexpr QStringView hello = u"hello";
    QString hello2B = QStringView(hello).toString().repeated(2);
    hello2B += QStringView(u"🍉");
    QStringView hello2(hello2B);
    {
        static constexpr auto m = qMakeStaticCaseSensitiveLatin1StringMatcher("hel");
        QCOMPARE(m.indexIn(QStringView(u"hello🍉")), 0);
        QCOMPARE(m.indexIn(QStringView(u"Hello🍉")), -1);
        QCOMPARE(m.indexIn(QStringView(u"Hellohello🍉")), 5);
        QCOMPARE(m.indexIn(QStringView(u"helloHello🍉")), 0);
        QCOMPARE(m.indexIn(QStringView(u"he🍉")), -1);
        QCOMPARE(m.indexIn(QStringView(u"hel🍉")), 0);
        QCOMPARE(m.indexIn(hello), 0);
        QCOMPARE(m.indexIn(hello, 1), -1); // from is 1
        QCOMPARE(m.indexIn(hello2, 2), hello.size()); // from is 2
        QCOMPARE(m.indexIn(hello2, 3), hello.size()); // from is 3
        QCOMPARE(m.indexIn(hello2, 6), -1); // from is 6
        static_assert(m.indexIn(QStringView(u"hello🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"Hello🍉")) == -1);
        static_assert(m.indexIn(QStringView(u"Hellohello🍉")) == 5);
        static_assert(m.indexIn(QStringView(u"helloHello🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"he🍉")) == -1);
        static_assert(m.indexIn(QStringView(u"hel🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"hellohello🍉"), 2) == 5); // from is 2
        static_assert(m.indexIn(QStringView(u"hellohello🍉"), 3) == 5); // from is 3
        static_assert(m.indexIn(QStringView(u"hellohello🍉"), 6) == -1); // from is 6
    }
    {
        static constexpr auto m = qMakeStaticCaseSensitiveLatin1StringMatcher("Hel");
        QCOMPARE(m.indexIn(QStringView(u"hello🍉")), -1);
        QCOMPARE(m.indexIn(QStringView(u"Hello🍉")), 0);
        QCOMPARE(m.indexIn(QStringView(u"Hellohello🍉")), 0);
        QCOMPARE(m.indexIn(QStringView(u"helloHello🍉")), 5);
        QCOMPARE(m.indexIn(QStringView(u"helloHello🍉"), 6), -1);
        QCOMPARE(m.indexIn(QStringView(u"He🍉")), -1);
        QCOMPARE(m.indexIn(QStringView(u"Hel🍉")), 0);
        QCOMPARE(m.indexIn(hello), -1);
        QCOMPARE(m.indexIn(hello2, 2), -1); // from is 2
        QCOMPARE(m.indexIn(hello2, 6), -1); // from is 6
        static_assert(m.indexIn(QStringView(u"hello🍉")) == -1);
        static_assert(m.indexIn(QStringView(u"Hello🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"Hellohello🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"helloHello🍉")) == 5);
        static_assert(m.indexIn(QStringView(u"helloHello🍉"), 6) == -1);
        static_assert(m.indexIn(QStringView(u"He🍉")) == -1);
        static_assert(m.indexIn(QStringView(u"Hel🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"hellohello🍉"), 2) == -1); // from is 2
        static_assert(m.indexIn(QStringView(u"hellohello🍉"), 6) == -1); // from is 6
    }
    {
        static constexpr auto m = qMakeStaticCaseInsensitiveLatin1StringMatcher("hel");
        QCOMPARE(m.indexIn(QStringView(u"hello🍉")), 0);
        QCOMPARE(m.indexIn(QStringView(u"Hello🍉")), 0);
        QCOMPARE(m.indexIn(QStringView(u"Hellohello🍉")), 0);
        QCOMPARE(m.indexIn(QStringView(u"helloHello🍉")), 0);
        QCOMPARE(m.indexIn(QStringView(u"he🍉")), -1);
        QCOMPARE(m.indexIn(QStringView(u"hel🍉")), 0);
        QCOMPARE(m.indexIn(hello), 0);
        QCOMPARE(m.indexIn(hello, 1), -1);
        QCOMPARE(m.indexIn(hello2, 2), hello.size()); // from is 2
        QCOMPARE(m.indexIn(hello2, 3), hello.size()); // from is 3
        QCOMPARE(m.indexIn(hello2, 6), -1); // from is 6
        static_assert(m.indexIn(QStringView(u"hello🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"Hello🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"Hellohello🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"helloHello🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"he🍉")) == -1);
        static_assert(m.indexIn(QStringView(u"hel🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"hellohello🍉"), 2) == 5); // from is 2
        static_assert(m.indexIn(QStringView(u"hellohello🍉"), 3) == 5); // from is 3
        static_assert(m.indexIn(QStringView(u"hellohello🍉"), 6) == -1); // from is 6
    }
    {
        static constexpr auto m = qMakeStaticCaseInsensitiveLatin1StringMatcher("Hel");
        QCOMPARE(m.indexIn(QStringView(u"hello🍉")), 0);
        QCOMPARE(m.indexIn(QStringView(u"Hello🍉")), 0);
        QCOMPARE(m.indexIn(QStringView(u"Hellohello🍉")), 0);
        QCOMPARE(m.indexIn(QStringView(u"helloHello🍉")), 0);
        QCOMPARE(m.indexIn(QStringView(u"he🍉")), -1);
        QCOMPARE(m.indexIn(QStringView(u"hel🍉")), 0);
        QCOMPARE(m.indexIn(hello), 0);
        QCOMPARE(m.indexIn(hello, 1), -1);
        QCOMPARE(m.indexIn(hello2, 2), hello.size()); // from is 2
        QCOMPARE(m.indexIn(hello2, 3), hello.size()); // from is 3
        QCOMPARE(m.indexIn(hello2, 6), -1); // from is 6
        static_assert(m.indexIn(QStringView(u"hello🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"Hello🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"Hellohello🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"helloHello🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"he🍉")) == -1);
        static_assert(m.indexIn(QStringView(u"hel🍉")) == 0);
        static_assert(m.indexIn(QStringView(u"hellohello🍉"), 2) == 5); // from is 2
        static_assert(m.indexIn(QStringView(u"hellohello🍉"), 3) == 5); // from is 3
        static_assert(m.indexIn(QStringView(u"hellohello🍉"), 6) == -1); // from is 6
    }
    {
        static constexpr auto m = qMakeStaticCaseInsensitiveLatin1StringMatcher("b\xF8");
        QCOMPARE(m.indexIn(QStringView(u"B\xD8")), 0);
        QCOMPARE(m.indexIn(QStringView(u"B\xF8")), 0);
        QCOMPARE(m.indexIn(QStringView(u"b\xD8")), 0);
        QCOMPARE(m.indexIn(QStringView(u"b\xF8")), 0);
        QCOMPARE(m.indexIn(QStringView(u"b\xF8lle")), 0);
        QCOMPARE(m.indexIn(QStringView(u"m\xF8lle")), -1);
        QCOMPARE(m.indexIn(QStringView(u"Si b\xF8")), 3);
    }
    {
        static constexpr auto m = qMakeStaticCaseSensitiveLatin1StringMatcher("b\xF8");
        QCOMPARE(m.indexIn(QStringView(u"B\xD8")), -1);
        QCOMPARE(m.indexIn(QStringView(u"B\xF8")), -1);
        QCOMPARE(m.indexIn(QStringView(u"b\xD8")), -1);
        QCOMPARE(m.indexIn(QStringView(u"b\xF8")), 0);
        QCOMPARE(m.indexIn(QStringView(u"b\xF8lle")), 0);
        QCOMPARE(m.indexIn(QStringView(u"m\xF8lle")), -1);
        QCOMPARE(m.indexIn(QStringView(u"Si b\xF8")), 3);
    }
#endif
}

void tst_QLatin1StringMatcher::interface()
{
    QLatin1StringView needle = "abc123"_L1;
    QByteArray haystackT(500, 'a');
    haystackT.insert(6, "123");
    haystackT.insert(31, "abc");
    haystackT.insert(42, "abc123");
    haystackT.insert(84, "abc123");
    QLatin1StringView haystack(haystackT);

    QLatin1StringMatcher matcher1;

    matcher1 = QLatin1StringMatcher(needle, Qt::CaseSensitive);
    QLatin1StringMatcher matcher2;
    matcher2.setPattern(needle);

    QLatin1StringMatcher matcher3 = QLatin1StringMatcher(needle, Qt::CaseSensitive);
    QLatin1StringMatcher matcher4;
    matcher4 = matcher3;

    QCOMPARE(matcher1.indexIn(haystack), 42);
    QCOMPARE(matcher2.indexIn(haystack), 42);
    QCOMPARE(matcher3.indexIn(haystack), 42);
    QCOMPARE(matcher4.indexIn(haystack), 42);

    QCOMPARE(matcher1.indexIn(haystack, 43), 84);
    QCOMPARE(matcher1.indexIn(haystack, 85), -1);

    QLatin1StringMatcher matcher5("123"_L1, Qt::CaseSensitive);
    QCOMPARE(matcher5.indexIn(haystack), 6);

    matcher5 = QLatin1StringMatcher("abc"_L1, Qt::CaseSensitive);
    QCOMPARE(matcher5.indexIn(haystack), 31);

    matcher5.setPattern(matcher4.pattern());
    QCOMPARE(matcher5.indexIn(haystack), 42);

    QLatin1StringMatcher matcher6 = matcher5;
    QCOMPARE(matcher6.indexIn(haystack), 42);

    QLatin1StringMatcher matcher7 = std::move(matcher5);
    QCOMPARE(matcher7.indexIn(haystack), 42);

    matcher1.setPattern("123"_L1);
    matcher7 = std::move(matcher1);
    QCOMPARE(matcher7.indexIn(haystack), 6);
}

#define LONG_STRING__32 "abcdefghijklmnopqrstuvwxyz012345"
#define LONG_STRING__64 LONG_STRING__32 LONG_STRING__32
#define LONG_STRING_128 LONG_STRING__64 LONG_STRING__64
#define LONG_STRING_256 LONG_STRING_128 LONG_STRING_128
#define LONG_STRING_512 LONG_STRING_256 LONG_STRING_256

void tst_QLatin1StringMatcher::indexIn()
{
    const char p_data[] = { 0x0, 0x0, 0x1 };
    QLatin1StringView pattern(p_data, sizeof(p_data));

    QByteArray haystackT(8, '\0');
    haystackT[7] = 0x1;
    QLatin1StringView haystack(haystackT);

    QLatin1StringMatcher matcher;

    matcher = QLatin1StringMatcher(pattern, Qt::CaseSensitive);
    QCOMPARE(matcher.indexIn(haystack, 0), 5);
    QCOMPARE(matcher.indexIn(haystack, 1), 5);
    QCOMPARE(matcher.indexIn(haystack, 2), 5);

    matcher.setPattern(pattern);
    QCOMPARE(matcher.indexIn(haystack, 0), 5);
    QCOMPARE(matcher.indexIn(haystack, 1), 5);
    QCOMPARE(matcher.indexIn(haystack, 2), 5);

    std::array<char, 256> allChars;
    for (int i = 0; i < 256; ++i)
        allChars[i] = char(i);

    matcher = QLatin1StringMatcher(QLatin1StringView(allChars), Qt::CaseSensitive);
    haystackT = LONG_STRING__32 "x";
    haystackT += matcher.pattern();
    haystack = QLatin1StringView(haystackT);
    QCOMPARE(matcher.indexIn(haystack, 0), 33);
    QCOMPARE(matcher.indexIn(haystack, 1), 33);
    QCOMPARE(matcher.indexIn(haystack, 2), 33);
    QCOMPARE(matcher.indexIn(haystack, 33), 33);
    QCOMPARE(matcher.indexIn(haystack, 34), -1);

    matcher = QLatin1StringMatcher(QLatin1StringView(LONG_STRING_256), Qt::CaseSensitive);
    haystackT = QByteArray(LONG_STRING__32 "x");
    haystackT += matcher.pattern();
    haystackT += QByteArrayView("Just junk at the end");
    haystack = QLatin1StringView(haystackT);
    QCOMPARE(matcher.indexIn(haystack, 0), 33);
    QCOMPARE(matcher.indexIn(haystack, 1), 33);
    QCOMPARE(matcher.indexIn(haystack, 2), 33);
    QCOMPARE(matcher.indexIn(haystack, 33), 33);
    QCOMPARE(matcher.indexIn(haystack, 34), -1);
    matcher.setCaseSensitivity(Qt::CaseInsensitive);
    QCOMPARE(matcher.indexIn(haystack, 0), 33);
    QCOMPARE(matcher.indexIn(haystack, 1), 33);
    QCOMPARE(matcher.indexIn(haystack, 2), 33);
    QCOMPARE(matcher.indexIn(haystack, 33), 33);
    QCOMPARE(matcher.indexIn(haystack, 34), -1);

    matcher = QLatin1StringMatcher(QLatin1StringView(LONG_STRING_512), Qt::CaseInsensitive);
    haystackT = QByteArray(LONG_STRING__32 "x");
    haystackT += matcher.pattern();
    haystackT += QByteArrayView("Just junk at the end");
    haystack = QLatin1StringView(haystackT);
    QCOMPARE(matcher.indexIn(haystack, 0), 33);
    QCOMPARE(matcher.indexIn(haystack, 1), 33);
    QCOMPARE(matcher.indexIn(haystack, 2), 33);
    QCOMPARE(matcher.indexIn(haystack, 33), 33);
    QCOMPARE(matcher.indexIn(haystack, 34), -1);
    matcher.setCaseSensitivity(Qt::CaseSensitive);
    QCOMPARE(matcher.indexIn(haystack, 0), 33);
    QCOMPARE(matcher.indexIn(haystack, 1), 33);
    QCOMPARE(matcher.indexIn(haystack, 2), 33);
    QCOMPARE(matcher.indexIn(haystack, 33), 33);
    QCOMPARE(matcher.indexIn(haystack, 34), -1);

    matcher = QLatin1StringMatcher(QLatin1StringView(""), Qt::CaseSensitive);
    haystackT = QByteArray(LONG_STRING__32 "x");
    haystack = QLatin1StringView(haystackT);
    QCOMPARE(matcher.indexIn(haystack, 0), 0);
    QCOMPARE(matcher.indexIn(haystack, 1), 1);
    QCOMPARE(matcher.indexIn(haystack, 2), 2);
    QCOMPARE(matcher.indexIn(haystack, 33), 33);

    matcher = QLatin1StringMatcher(QLatin1StringView(""), Qt::CaseInsensitive);
    haystackT = QByteArray(LONG_STRING__32 "x");
    haystack = QLatin1StringView(haystackT);
    QCOMPARE(matcher.indexIn(haystack, 0), 0);
    QCOMPARE(matcher.indexIn(haystack, 1), 1);
    QCOMPARE(matcher.indexIn(haystack, 2), 2);
    QCOMPARE(matcher.indexIn(haystack, 33), 33);

    matcher = QLatin1StringMatcher(QLatin1StringView("m\xF8"), Qt::CaseInsensitive);
    haystackT = QByteArray("M\xF8m\xF8");
    haystack = QLatin1StringView(haystackT);
    QCOMPARE(matcher.indexIn(haystack, 0), 0);
    QCOMPARE(matcher.indexIn(haystack, 1), 2);
    QCOMPARE(matcher.indexIn(haystack, 2), 2);
    QCOMPARE(matcher.indexIn(haystack, 3), -1);
    matcher.setCaseSensitivity(Qt::CaseSensitive);
    QCOMPARE(matcher.indexIn(haystack, 0), 2);
    QCOMPARE(matcher.indexIn(haystack, 1), 2);
    QCOMPARE(matcher.indexIn(haystack, 2), 2);
    QCOMPARE(matcher.indexIn(haystack, 3), -1);
}

void tst_QLatin1StringMatcher::haystacksWithMoreThan4GiBWork()
{
#if QT_POINTER_SIZE > 4
    // use a large needle to trigger long skips in the Boyer-Moore algorithm
    // (to speed up the test)
    constexpr std::string_view needle = LONG_STRING_256;

    //
    // GIVEN: a haystack with more than 4 GiB of data
    //

    // don't use QByteArray because freeSpaceAtEnd() may break reserve()
    // semantics and a realloc is the last thing we need here
    std::string large;
    QElapsedTimer timer;
    timer.start();
    constexpr size_t GiB = 1024 * 1024 * 1024;
    constexpr size_t BaseSize = 4 * GiB + 1;
    try {
        large.reserve(BaseSize + needle.size());
        large.resize(BaseSize, '\0');
        large.append(needle);
    } catch (const std::bad_alloc &) {
        QSKIP("Could not allocate 4GiB plus a couple hundred bytes of RAM.");
    }
    QCOMPARE(large.size(), BaseSize + needle.size());
    qDebug("created dataset in %lld ms", timer.elapsed());

    {
        //
        // WHEN: trying to match an occurrence past the 4GiB mark
        //
        qsizetype dynamicResult;
        auto t = std::thread{ [&] {
            QLatin1StringMatcher m(QLatin1StringView(needle), Qt::CaseSensitive);
            dynamicResult = m.indexIn(QLatin1StringView(large));
        } };
        t.join();

        //
        // THEN: the result index is not truncated
        //

        QCOMPARE(dynamicResult, qsizetype(BaseSize));
    }

    {
        qsizetype dynamicResult;
        auto t = std::thread{ [&] {
            QLatin1StringMatcher m(QLatin1StringView(needle), Qt::CaseSensitive);
            dynamicResult = m.indexIn(QStringView(QString::fromLatin1(large)));
        } };
        t.join();

        QCOMPARE(dynamicResult, qsizetype(BaseSize));
    }

#else
    QSKIP("This test is 64-bit only.");
#endif
}

void tst_QLatin1StringMatcher::staticLatin1StringMatcher()
{
#ifdef QT_STATIC_BOYER_MOORE_NOT_SUPPORTED
    QSKIP("Test is only valid on an OS that supports static latin1 string matcher");
#else
    {
        static constexpr auto smatcher = qMakeStaticCaseSensitiveLatin1StringMatcher("Hello");
        QCOMPARE(smatcher.indexIn("Hello"_L1), 0);
        QCOMPARE(smatcher.indexIn("Hello, World!"_L1), 0);
        QCOMPARE(smatcher.indexIn("Hello, World!"_L1, 0), 0);
        QCOMPARE(smatcher.indexIn("Hello, World!"_L1, 1), -1);
        QCOMPARE(smatcher.indexIn("aHello, World!"_L1), 1);
        QCOMPARE(smatcher.indexIn("aaHello, World!"_L1), 2);
        QCOMPARE(smatcher.indexIn("aaaHello, World!"_L1), 3);
        QCOMPARE(smatcher.indexIn("aaaaHello, World!"_L1), 4);
        QCOMPARE(smatcher.indexIn("aaaaaHello, World!"_L1), 5);
        QCOMPARE(smatcher.indexIn("aaaaaaHello, World!"_L1), 6);
        QCOMPARE(smatcher.indexIn("HHello, World!"_L1), 1);
        QCOMPARE(smatcher.indexIn("HeHello, World!"_L1), 2);
        QCOMPARE(smatcher.indexIn("HelHello, World!"_L1), 3);
        QCOMPARE(smatcher.indexIn("HellHello, World!"_L1), 4);
        QCOMPARE(smatcher.indexIn("HellaHello, World!"_L1), 5);
        QCOMPARE(smatcher.indexIn("HellauHello, World!"_L1), 6);
        QCOMPARE(smatcher.indexIn("aHella, World!"_L1), -1);
        QCOMPARE(smatcher.indexIn("aaHella, World!"_L1), -1);
        QCOMPARE(smatcher.indexIn("aaaHella, World!"_L1), -1);
        QCOMPARE(smatcher.indexIn("aaaaHella, World!"_L1), -1);
        QCOMPARE(smatcher.indexIn("aaaaaHella, World!"_L1), -1);
        QCOMPARE(smatcher.indexIn("aaaaaaHella, World!"_L1), -1);

        QCOMPARE(smatcher.indexIn("aHello"_L1), 1);
        QCOMPARE(smatcher.indexIn("aaHello"_L1), 2);
        QCOMPARE(smatcher.indexIn("aaaHello"_L1), 3);
        QCOMPARE(smatcher.indexIn("aaaaHello"_L1), 4);
        QCOMPARE(smatcher.indexIn("aaaaaHello"_L1), 5);
        QCOMPARE(smatcher.indexIn("aaaaaaHello"_L1), 6);
        QCOMPARE(smatcher.indexIn("HHello"_L1), 1);
        QCOMPARE(smatcher.indexIn("HeHello"_L1), 2);
        QCOMPARE(smatcher.indexIn("HelHello"_L1), 3);
        QCOMPARE(smatcher.indexIn("HellHello"_L1), 4);
        QCOMPARE(smatcher.indexIn("HellaHello"_L1), 5);
        QCOMPARE(smatcher.indexIn("HellauHello"_L1), 6);
        QCOMPARE(smatcher.indexIn("aHella"_L1), -1);
        QCOMPARE(smatcher.indexIn("aaHella"_L1), -1);
        QCOMPARE(smatcher.indexIn("aaaHella"_L1), -1);
        QCOMPARE(smatcher.indexIn("aaaaHella"_L1), -1);
        QCOMPARE(smatcher.indexIn("aaaaaHella"_L1), -1);
        QCOMPARE(smatcher.indexIn("aaaaaaHella"_L1), -1);

        constexpr qsizetype found = smatcher.indexIn("Oh Hello"_L1);
        static_assert(found == 3);

        static_assert(smatcher.indexIn("Hello"_L1) == 0);
        static_assert(smatcher.indexIn("Hello, World!"_L1) == 0);
        static_assert(smatcher.indexIn("Hello, World!"_L1, 0) == 0);
        static_assert(smatcher.indexIn("Hello, World!"_L1, 1) == -1);
        static_assert(smatcher.indexIn("aHello, World!"_L1) == 1);
        static_assert(smatcher.indexIn("aaHello, World!"_L1) == 2);
        static_assert(smatcher.indexIn("aaaHello, World!"_L1) == 3);
        static_assert(smatcher.indexIn("aaaaHello, World!"_L1) == 4);
        static_assert(smatcher.indexIn("aaaaaHello, World!"_L1) == 5);
        static_assert(smatcher.indexIn("aaaaaaHello, World!"_L1) == 6);
        static_assert(smatcher.indexIn("HHello, World!"_L1) == 1);
        static_assert(smatcher.indexIn("HeHello, World!"_L1) == 2);
        static_assert(smatcher.indexIn("HelHello, World!"_L1) == 3);
        static_assert(smatcher.indexIn("HellHello, World!"_L1) == 4);
        static_assert(smatcher.indexIn("HellaHello, World!"_L1) == 5);
        static_assert(smatcher.indexIn("HellauHello, World!"_L1) == 6);
        static_assert(smatcher.indexIn("aHella, World!"_L1) == -1);
        static_assert(smatcher.indexIn("aaHella, World!"_L1) == -1);
        static_assert(smatcher.indexIn("aaaHella, World!"_L1) == -1);
        static_assert(smatcher.indexIn("aaaaHella, World!"_L1) == -1);
        static_assert(smatcher.indexIn("aaaaaHella, World!"_L1) == -1);
        static_assert(smatcher.indexIn("aaaaaaHella, World!"_L1) == -1);

        static_assert(smatcher.indexIn("aHello"_L1) == 1);
        static_assert(smatcher.indexIn("aaHello"_L1) == 2);
        static_assert(smatcher.indexIn("aaaHello"_L1) == 3);
        static_assert(smatcher.indexIn("aaaaHello"_L1) == 4);
        static_assert(smatcher.indexIn("aaaaaHello"_L1) == 5);
        static_assert(smatcher.indexIn("aaaaaaHello"_L1) == 6);
        static_assert(smatcher.indexIn("HHello"_L1) == 1);
        static_assert(smatcher.indexIn("HeHello"_L1) == 2);
        static_assert(smatcher.indexIn("HelHello"_L1) == 3);
        static_assert(smatcher.indexIn("HellHello"_L1) == 4);
        static_assert(smatcher.indexIn("HellaHello"_L1) == 5);
        static_assert(smatcher.indexIn("HellauHello"_L1) == 6);
        static_assert(smatcher.indexIn("aHella"_L1) == -1);
        static_assert(smatcher.indexIn("aaHella"_L1) == -1);
        static_assert(smatcher.indexIn("aaaHella"_L1) == -1);
        static_assert(smatcher.indexIn("aaaaHella"_L1) == -1);
        static_assert(smatcher.indexIn("aaaaaHella"_L1) == -1);
        static_assert(smatcher.indexIn("aaaaaaHella"_L1) == -1);

        static_assert(smatcher.indexIn("aHello"_L1) == 1);
        static_assert(smatcher.indexIn("no"_L1) == -1);
        static_assert(smatcher.indexIn("miss"_L1) == -1);
        static_assert(smatcher.indexIn("amiss"_L1) == -1);
        static_assert(smatcher.indexIn("olleH"_L1) == -1);
        static_assert(smatcher.indexIn("HellNo"_L1) == -1);
        static_assert(smatcher.indexIn("lloHello"_L1) == 3);
        static_assert(smatcher.indexIn("lHello"_L1) == 1);
        static_assert(smatcher.indexIn("oHello"_L1) == 1);
    }
    {
        static constexpr auto smatcher =
                qMakeStaticCaseSensitiveLatin1StringMatcher(LONG_STRING_256);

        QCOMPARE(smatcher.indexIn(QLatin1StringView("a" LONG_STRING_256)), 1);
        QCOMPARE(smatcher.indexIn(QLatin1StringView("aa" LONG_STRING_256)), 2);
        QCOMPARE(smatcher.indexIn(QLatin1StringView("aaa" LONG_STRING_256)), 3);
        QCOMPARE(smatcher.indexIn(QLatin1StringView("aaaa" LONG_STRING_256)), 4);
        QCOMPARE(smatcher.indexIn(QLatin1StringView("aaaaa" LONG_STRING_256)), 5);
        QCOMPARE(smatcher.indexIn(QLatin1StringView("aaaaaa" LONG_STRING_256)), 6);
        QCOMPARE(smatcher.indexIn(QLatin1StringView("a" LONG_STRING_256 "a")), 1);
        QCOMPARE(smatcher.indexIn(QLatin1StringView("aa" LONG_STRING_256 "a")), 2);
        QCOMPARE(smatcher.indexIn(QLatin1StringView("aaa" LONG_STRING_256 "a")), 3);
        QCOMPARE(smatcher.indexIn(QLatin1StringView("aaaa" LONG_STRING_256 "a")), 4);
        QCOMPARE(smatcher.indexIn(QLatin1StringView("aaaaa" LONG_STRING_256 "a")), 5);
        QCOMPARE(smatcher.indexIn(QLatin1StringView("aaaaaa" LONG_STRING_256 "a")), 6);
        QCOMPARE(smatcher.indexIn(QLatin1StringView(LONG_STRING__32 "x" LONG_STRING_256)), 33);
        QCOMPARE(smatcher.indexIn(QLatin1StringView(LONG_STRING__64 "x" LONG_STRING_256)), 65);
        QCOMPARE(smatcher.indexIn(QLatin1StringView(LONG_STRING_128 "x" LONG_STRING_256)), 129);

        static_assert(smatcher.indexIn(QLatin1StringView("a" LONG_STRING_256)) == 1);
        static_assert(smatcher.indexIn(QLatin1StringView("aa" LONG_STRING_256)) == 2);
        static_assert(smatcher.indexIn(QLatin1StringView("aaa" LONG_STRING_256)) == 3);
        static_assert(smatcher.indexIn(QLatin1StringView("aaaa" LONG_STRING_256)) == 4);
        static_assert(smatcher.indexIn(QLatin1StringView("aaaaa" LONG_STRING_256)) == 5);
        static_assert(smatcher.indexIn(QLatin1StringView("aaaaaa" LONG_STRING_256)) == 6);
        static_assert(smatcher.indexIn(QLatin1StringView("a" LONG_STRING_256 "a")) == 1);
        static_assert(smatcher.indexIn(QLatin1StringView("aa" LONG_STRING_256 "a")) == 2);
        static_assert(smatcher.indexIn(QLatin1StringView("aaa" LONG_STRING_256 "a")) == 3);
        static_assert(smatcher.indexIn(QLatin1StringView("aaaa" LONG_STRING_256 "a")) == 4);
        static_assert(smatcher.indexIn(QLatin1StringView("aaaaa" LONG_STRING_256 "a")) == 5);
        static_assert(smatcher.indexIn(QLatin1StringView("aaaaaa" LONG_STRING_256 "a")) == 6);
        static_assert(smatcher.indexIn(QLatin1StringView(LONG_STRING__32 "x" LONG_STRING_256))
                      == 33);
        static_assert(smatcher.indexIn(QLatin1StringView(LONG_STRING__64 "x" LONG_STRING_256))
                      == 65);
        static_assert(smatcher.indexIn(QLatin1StringView(LONG_STRING_128 "x" LONG_STRING_256))
                      == 129);
    }
#endif
}

#undef LONG_STRING_512
#undef LONG_STRING_256
#undef LONG_STRING_128
#undef LONG_STRING__64
#undef LONG_STRING__32

QTEST_APPLESS_MAIN(tst_QLatin1StringMatcher)
#include "tst_qlatin1stringmatcher.moc"
