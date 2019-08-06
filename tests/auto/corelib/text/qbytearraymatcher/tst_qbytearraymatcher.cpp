/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qbytearraymatcher.h>

// COM interface
#if defined(Q_OS_WIN) && defined(interface)
#    undef interface
#endif

class tst_QByteArrayMatcher : public QObject
{
    Q_OBJECT

private slots:
    void interface();
    void indexIn();
    void staticByteArrayMatcher();
};

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

    QCOMPARE(matcher1.indexIn(haystack.constData(), haystack.length()), 42);

    QCOMPARE(matcher1.indexIn(haystack, 43), 84);
    QCOMPARE(matcher1.indexIn(haystack.constData(), haystack.length(), 43), 84);
    QCOMPARE(matcher1.indexIn(haystack, 85), -1);
    QCOMPARE(matcher1.indexIn(haystack.constData(), haystack.length(), 85), -1);

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
        static Q_RELAXED_CONSTEXPR auto smatcher = qMakeStaticByteArrayMatcher("Hello");
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
        static Q_RELAXED_CONSTEXPR auto smatcher = qMakeStaticByteArrayMatcher(LONG_STRING_256);
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

#undef LONG_STRING_256
#undef LONG_STRING_128
#undef LONG_STRING__64
#undef LONG_STRING__32

QTEST_APPLESS_MAIN(tst_QByteArrayMatcher)
#include "tst_qbytearraymatcher.moc"
