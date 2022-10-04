// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include <QtCore/QLatin1StringMatcher>

#include <numeric>
#include <string>

#if QT_CONFIG(cxx11_future)
#    include <thread>
#endif

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
    void interface();
    void indexIn();
    void haystacksWithMoreThan4GiBWork();
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
    }
    {
        QLatin1StringMatcher m("Hello"_L1, Qt::CaseSensitive);
        QCOMPARE(m.pattern(), "Hello"_L1);
        QCOMPARE(m.indexIn("hello"_L1), -1);
        QCOMPARE(m.indexIn("Hello"_L1), 0);
        QCOMPARE(m.indexIn("Hellohello"_L1), 0);
        QCOMPARE(m.indexIn("helloHello"_L1), 5);
        QCOMPARE(m.indexIn("helloHello"_L1, 6), -1);
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
    }
    {
        QLatin1StringMatcher m(hello, Qt::CaseSensitive);
        QCOMPARE(m.pattern(), "hello"_L1);
        QCOMPARE(m.indexIn(hello), 0);
        QCOMPARE(m.indexIn(hello, 1), -1);
        QCOMPARE(m.indexIn(hello2, 1), hello.size());
        QCOMPARE(m.indexIn(hello2, 6), -1);
    }
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

#    if QT_CONFIG(cxx11_future)
    using MaybeThread = std::thread;
#    else
    struct MaybeThread
    {
        std::function<void()> func;
        void join() { func(); }
    };
#    endif

    //
    // WHEN: trying to match an occurrence past the 4GiB mark
    //

    qsizetype dynamicResult;

    auto t = MaybeThread{ [&] {
        QLatin1StringMatcher m(QLatin1StringView(needle), Qt::CaseSensitive);
        dynamicResult = m.indexIn(QLatin1StringView(large));
    } };
    t.join();

    //
    // THEN: the result index is not trucated
    //

    QCOMPARE(dynamicResult, qsizetype(BaseSize));
#else
    QSKIP("This test is 64-bit only.");
#endif
}

#undef LONG_STRING_512
#undef LONG_STRING_256
#undef LONG_STRING_128
#undef LONG_STRING__64
#undef LONG_STRING__32

QTEST_APPLESS_MAIN(tst_QLatin1StringMatcher)
#include "tst_qlatin1stringmatcher.moc"
