// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

#include <qbytearraymatcher.h>

#include <numeric>
#include <string>

#if QT_CONFIG(cxx11_future)
# include <thread>
#endif

// COM interface
#if defined(Q_OS_WIN) && defined(interface)
#    undef interface
#endif

class tst_QByteArrayMatcher : public QObject
{
    Q_OBJECT

private slots:
    void overloads();
    void interface();
    void indexIn();
    void staticByteArrayMatcher();
    void haystacksWithMoreThan4GiBWork();
};

void tst_QByteArrayMatcher::overloads()
{
    QByteArray hello = QByteArrayLiteral("hello");
    QByteArray hello2 = hello.repeated(2);
    {
        QByteArrayMatcher m("hello");
        QCOMPARE(m.pattern(), "hello");
        QCOMPARE(m.indexIn("hello"), 0);
    }
    {
        QByteArrayMatcher m("hello", qsizetype(3));
        QCOMPARE(m.pattern(), "hel");
        QCOMPARE(m.indexIn("hellohello", qsizetype(2)), -1); // haystack is "he", not: from is 2
        QCOMPARE(m.indexIn("hellohello", qsizetype(3)), 0); // haystack is "hel", not: from is 3
    }
    {
        QByteArrayMatcher m(hello);
        QCOMPARE(m.pattern(), "hello");
        QCOMPARE(m.indexIn(hello), 0);
        QCOMPARE(m.indexIn(hello2, qsizetype(1)), hello.size());
    }
    {
        QStaticByteArrayMatcher m("hel");
        QCOMPARE(m.pattern(), "hel");
        QCOMPARE(m.indexIn("hello"), qsizetype(0));
        QCOMPARE(m.indexIn("hellohello", qsizetype(2)), -1); // haystack is "he", not: from is 2
        QCOMPARE(m.indexIn("hellohello", qsizetype(3)), 0); // haystack is "hel", not: from is 3
        QCOMPARE(m.indexIn(hello), 0);
        QCOMPARE(m.indexIn(hello2, qsizetype(2)), hello.size()); // from is 2
        QCOMPARE(m.indexIn(hello2, qsizetype(3)), hello.size()); // from is 3
    }
}

void tst_QByteArrayMatcher::interface()
{
    const char needle[] = "abc123";
    QByteArray haystack(500, 'a');
    haystack.insert(6, "123");
    haystack.insert(31, "abc");
    haystack.insert(42, "abc123");
    haystack.insert(84, "abc123");

    QByteArrayMatcher matcher1;

    matcher1 = QByteArrayMatcher(QByteArray(needle));
    QByteArrayMatcher matcher2;
    matcher2.setPattern(QByteArray(needle));

    QByteArrayMatcher matcher3 = QByteArrayMatcher(QByteArray(needle));
    QByteArrayMatcher matcher4(needle, sizeof(needle) - 1);
    QByteArrayMatcher matcher5(matcher2);
    QByteArrayMatcher matcher6;
    matcher6 = matcher3;

    QCOMPARE(matcher1.indexIn(haystack), 42);
    QCOMPARE(matcher2.indexIn(haystack), 42);
    QCOMPARE(matcher3.indexIn(haystack), 42);
    QCOMPARE(matcher4.indexIn(haystack), 42);
    QCOMPARE(matcher5.indexIn(haystack), 42);
    QCOMPARE(matcher6.indexIn(haystack), 42);

    QCOMPARE(matcher1.indexIn(haystack.constData(), haystack.size()), 42);

    QCOMPARE(matcher1.indexIn(haystack, 43), 84);
    QCOMPARE(matcher1.indexIn(haystack.constData(), haystack.size(), 43), 84);
    QCOMPARE(matcher1.indexIn(haystack, 85), -1);
    QCOMPARE(matcher1.indexIn(haystack.constData(), haystack.size(), 85), -1);

    QByteArrayMatcher matcher7(QByteArray("123"));
    QCOMPARE(matcher7.indexIn(haystack), 6);

    matcher7 = QByteArrayMatcher(QByteArray("abc"));
    QCOMPARE(matcher7.indexIn(haystack), 31);

    matcher7.setPattern(matcher4.pattern());
    QCOMPARE(matcher7.indexIn(haystack), 42);
}

#define LONG_STRING__32 "abcdefghijklmnopqrstuvwxyz012345"
#define LONG_STRING__64 LONG_STRING__32 LONG_STRING__32
#define LONG_STRING_128 LONG_STRING__64 LONG_STRING__64
#define LONG_STRING_256 LONG_STRING_128 LONG_STRING_128

void tst_QByteArrayMatcher::indexIn()
{
    const char p_data[] = { 0x0, 0x0, 0x1 };
    QByteArray pattern(p_data, sizeof(p_data));

    QByteArray haystack(8, '\0');
    haystack[7] = 0x1;

    QByteArrayMatcher matcher;

    matcher = QByteArrayMatcher(pattern);
    QCOMPARE(matcher.indexIn(haystack, 0), 5);
    QCOMPARE(matcher.indexIn(haystack, 1), 5);
    QCOMPARE(matcher.indexIn(haystack, 2), 5);

    matcher.setPattern(pattern);
    QCOMPARE(matcher.indexIn(haystack, 0), 5);
    QCOMPARE(matcher.indexIn(haystack, 1), 5);
    QCOMPARE(matcher.indexIn(haystack, 2), 5);

    QByteArray allChars(256, Qt::Uninitialized);
    for (int i = 0; i < 256; ++i)
        allChars[i] = char(i);

    matcher = QByteArrayMatcher(allChars);
    haystack = LONG_STRING__32 "x" + matcher.pattern();
    QCOMPARE(matcher.indexIn(haystack,  0), 33);
    QCOMPARE(matcher.indexIn(haystack,  1), 33);
    QCOMPARE(matcher.indexIn(haystack,  2), 33);
    QCOMPARE(matcher.indexIn(haystack, 33), 33);
    QCOMPARE(matcher.indexIn(haystack, 34), -1);

    matcher = QByteArrayMatcher(LONG_STRING_256);
    haystack = LONG_STRING__32 "x" + matcher.pattern();
    QCOMPARE(matcher.indexIn(haystack,  0), 33);
    QCOMPARE(matcher.indexIn(haystack,  1), 33);
    QCOMPARE(matcher.indexIn(haystack,  2), 33);
    QCOMPARE(matcher.indexIn(haystack, 33), 33);
    QCOMPARE(matcher.indexIn(haystack, 34), -1);
}

void tst_QByteArrayMatcher::staticByteArrayMatcher()
{
    {
        static constexpr auto smatcher = qMakeStaticByteArrayMatcher("Hello");
        QCOMPARE(smatcher.pattern(), QByteArrayLiteral("Hello"));

        QCOMPARE(smatcher.indexIn(QByteArray("Hello, World!")),     0);
        QCOMPARE(smatcher.indexIn(QByteArray("Hello, World!"), 0),  0);
        QCOMPARE(smatcher.indexIn(QByteArray("Hello, World!"), 1), -1);
        QCOMPARE(smatcher.indexIn(QByteArray("aHello, World!")),        1);
        QCOMPARE(smatcher.indexIn(QByteArray("aaHello, World!")),       2);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaHello, World!")),      3);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaHello, World!")),     4);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaaHello, World!")),    5);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaaaHello, World!")),   6);
        QCOMPARE(smatcher.indexIn(QByteArray("HHello, World!")),        1);
        QCOMPARE(smatcher.indexIn(QByteArray("HeHello, World!")),       2);
        QCOMPARE(smatcher.indexIn(QByteArray("HelHello, World!")),      3);
        QCOMPARE(smatcher.indexIn(QByteArray("HellHello, World!")),     4);
        QCOMPARE(smatcher.indexIn(QByteArray("HellaHello, World!")),    5);
        QCOMPARE(smatcher.indexIn(QByteArray("HellauHello, World!")),   6);
        QCOMPARE(smatcher.indexIn(QByteArray("aHella, World!")),       -1);
        QCOMPARE(smatcher.indexIn(QByteArray("aaHella, World!")),      -1);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaHella, World!")),     -1);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaHella, World!")),    -1);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaaHella, World!")),   -1);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaaaHella, World!")),  -1);

        QCOMPARE(smatcher.indexIn(QByteArray("aHello")),        1);
        QCOMPARE(smatcher.indexIn(QByteArray("aaHello")),       2);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaHello")),      3);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaHello")),     4);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaaHello")),    5);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaaaHello")),   6);
        QCOMPARE(smatcher.indexIn(QByteArray("HHello")),        1);
        QCOMPARE(smatcher.indexIn(QByteArray("HeHello")),       2);
        QCOMPARE(smatcher.indexIn(QByteArray("HelHello")),      3);
        QCOMPARE(smatcher.indexIn(QByteArray("HellHello")),     4);
        QCOMPARE(smatcher.indexIn(QByteArray("HellaHello")),    5);
        QCOMPARE(smatcher.indexIn(QByteArray("HellauHello")),   6);
        QCOMPARE(smatcher.indexIn(QByteArray("aHella")),       -1);
        QCOMPARE(smatcher.indexIn(QByteArray("aaHella")),      -1);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaHella")),     -1);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaHella")),    -1);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaaHella")),   -1);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaaaHella")),  -1);
    }

    {
        static constexpr auto smatcher = qMakeStaticByteArrayMatcher(LONG_STRING_256);
        QCOMPARE(smatcher.pattern(), QByteArrayLiteral(LONG_STRING_256));

        QCOMPARE(smatcher.indexIn(QByteArray("a" LONG_STRING_256)),        1);
        QCOMPARE(smatcher.indexIn(QByteArray("aa" LONG_STRING_256)),       2);
        QCOMPARE(smatcher.indexIn(QByteArray("aaa" LONG_STRING_256)),      3);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaa" LONG_STRING_256)),     4);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaa" LONG_STRING_256)),    5);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaaa" LONG_STRING_256)),   6);
        QCOMPARE(smatcher.indexIn(QByteArray("a" LONG_STRING_256 "a")),        1);
        QCOMPARE(smatcher.indexIn(QByteArray("aa" LONG_STRING_256 "a")),       2);
        QCOMPARE(smatcher.indexIn(QByteArray("aaa" LONG_STRING_256 "a")),      3);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaa" LONG_STRING_256 "a")),     4);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaa" LONG_STRING_256 "a")),    5);
        QCOMPARE(smatcher.indexIn(QByteArray("aaaaaa" LONG_STRING_256 "a")),   6);
        QCOMPARE(smatcher.indexIn(QByteArray(LONG_STRING__32 "x" LONG_STRING_256)),  33);
        QCOMPARE(smatcher.indexIn(QByteArray(LONG_STRING__64 "x" LONG_STRING_256)),  65);
        QCOMPARE(smatcher.indexIn(QByteArray(LONG_STRING_128 "x" LONG_STRING_256)), 129);
    }

}

void tst_QByteArrayMatcher::haystacksWithMoreThan4GiBWork()
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

# if QT_CONFIG(cxx11_future)
    using MaybeThread = std::thread;
# else
    struct MaybeThread {
        std::function<void()> func;
        void join() { func(); }
    };
# endif

    //
    // WHEN: trying to match an occurrence past the 4GiB mark
    //

    qsizetype dynamicResult, staticResult;

    auto t = MaybeThread{[&]{
        QByteArrayMatcher m(needle);
        dynamicResult = m.indexIn(large);
    }};
    {
        static_assert(needle == LONG_STRING_256); // need a string literal in the following line:
        QStaticByteArrayMatcher m(LONG_STRING_256);
        staticResult = m.indexIn(large.data(), large.size());
    }
    t.join();

    //
    // THEN: the result index is not trucated
    //

    QCOMPARE(staticResult, qsizetype(BaseSize));
    QCOMPARE(dynamicResult, qsizetype(BaseSize));
#else
    QSKIP("This test is 64-bit only.");
#endif

}

#undef LONG_STRING_256
#undef LONG_STRING_128
#undef LONG_STRING__64
#undef LONG_STRING__32

QTEST_APPLESS_MAIN(tst_QByteArrayMatcher)
#include "tst_qbytearraymatcher.moc"
