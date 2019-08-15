/****************************************************************************
**
** Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
#include <QtCore/QString>
#include <private/qstringiterator_p.h>

class tst_QStringIterator : public QObject
{
    Q_OBJECT
private slots:
    void sweep_data();
    void sweep();

    void position();
};

void tst_QStringIterator::sweep_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<int>("count");

    QTest::newRow("sweep_00") << QString::fromUtf8("", 0) << true << 0;
    QTest::newRow("sweep_01") << QString::fromUtf8("a", 1) << true << 1;
    QTest::newRow("sweep_02") << QString::fromUtf8("a string", 8) << true << 8;
    QTest::newRow("sweep_03") << QString::fromUtf8("\xc3\xa0\xc3\xa8\xc3\xac\xc3\xb2\xc3\xb9", 10) << true << 5;
    QTest::newRow("sweep_04") << QString::fromUtf8("\xc3\x9f\xe2\x80\x94\xc2\xa1", 7) << true << 3;
    QTest::newRow("sweep_05") << QString::fromUtf8("\xe6\xb0\xb4\xe6\xb0\xb5\xe6\xb0\xb6\xe6\xb0\xb7\xe6\xb0\xb8\xe6\xb0\xb9", 18) << true << 6;
    QTest::newRow("sweep_06") << QString::fromUtf8("\xf0\x9f\x98\x81\xf0\x9f\x98\x82\x61\x62\x63\xf0\x9f\x98\x83\xc4\x91\xc3\xa8\xef\xac\x80\xf0\x9f\x98\x84\xf0\x9f\x98\x85", 30) << true << 11;
    QTest::newRow("sweep_07") << QString::fromUtf8("\xf0\x9f\x82\xaa\xf0\x9f\x82\xab\xf0\x9f\x82\xad\xf0\x9f\x82\xae\xf0\x9f\x82\xa1\x20\x52\x4f\x59\x41\x4c\x20\x46\x4c\x55\x53\x48\x20\x4f\x46\x20\x53\x50\x41\x44\x45\x53", 42) << true << 27;
    QTest::newRow("sweep_08") << QString::fromUtf8("abc\0def", 7) << true << 7;
    QTest::newRow("sweep_09") << QString::fromUtf8("\xc3\xa0\xce\xb2\xc3\xa7\xf0\x9f\x80\xb9\xf0\x9f\x80\xb8\x00\xf0\x9f\x80\xb1\x00\xf0\x9f\x80\xb3\xf0\x9f\x81\x85\xe1\xb8\x8a\xc4\x99\xc6\x92", 35) << true << 13;

    QTest::newRow("sweep_invalid_00") << QString(QChar(0xd800)) << false << 1;
    QTest::newRow("sweep_invalid_01") << QString(QChar(0xdc00)) << false << 1;
    QTest::newRow("sweep_invalid_02") << QString(QChar(0xdbff)) << false << 1;
    QTest::newRow("sweep_invalid_03") << QString(QChar(0xdfff)) << false << 1;

#define QSTRING_FROM_QCHARARRAY(x) (QString((x), sizeof(x)/sizeof((x)[0])))

    static const QChar invalid_04[] = {
        QLatin1Char('i'), QLatin1Char('n'), QLatin1Char('v'),
        QLatin1Char('a'), QLatin1Char('l'), QLatin1Char('i'),
        QLatin1Char('d'), QChar(0xd800)
    };
    QTest::newRow("sweep_invalid_04") << QSTRING_FROM_QCHARARRAY(invalid_04) << false << 8;

    static const QChar invalid_05[] = {
        QLatin1Char('i'), QLatin1Char('n'), QLatin1Char('v'),
        QLatin1Char('a'), QLatin1Char('l'), QLatin1Char('i'),
        QLatin1Char('d'), QChar(0xd800), QLatin1Char('x')
    };
    QTest::newRow("sweep_invalid_05") << QSTRING_FROM_QCHARARRAY(invalid_05) << false << 9;

    static const QChar invalid_06[] = {
        QLatin1Char('i'), QLatin1Char('n'), QLatin1Char('v'),
        QLatin1Char('a'), QLatin1Char('l'), QLatin1Char('i'),
        QLatin1Char('d'), QChar(0xdc00)
    };
    QTest::newRow("sweep_invalid_06") << QSTRING_FROM_QCHARARRAY(invalid_06) << false << 8;

    static const QChar invalid_07[] = {
        QLatin1Char('i'), QLatin1Char('n'), QLatin1Char('v'),
        QLatin1Char('a'), QLatin1Char('l'), QLatin1Char('i'),
        QLatin1Char('d'), QChar(0xdc00), QLatin1Char('x')
    };
    QTest::newRow("sweep_invalid_07") << QSTRING_FROM_QCHARARRAY(invalid_07) << false << 9;

    static const QChar invalid_08[] = {
        QChar(0xd800),
        QLatin1Char('i'), QLatin1Char('n'), QLatin1Char('v'),
        QLatin1Char('a'), QLatin1Char('l'), QLatin1Char('i'),
        QLatin1Char('d')
    };
    QTest::newRow("sweep_invalid_08") << QSTRING_FROM_QCHARARRAY(invalid_08) << false << 8;

    static const QChar invalid_09[] = {
        QChar(0xdc00),
        QLatin1Char('i'), QLatin1Char('n'), QLatin1Char('v'),
        QLatin1Char('a'), QLatin1Char('l'), QLatin1Char('i'),
        QLatin1Char('d')
    };
    QTest::newRow("sweep_invalid_09") << QSTRING_FROM_QCHARARRAY(invalid_09) << false << 8;

    static const QChar invalid_10[] = {
        QChar(0xd800), QChar(0xd800),
        QLatin1Char('i'), QLatin1Char('n'), QLatin1Char('v'),
        QLatin1Char('a'), QLatin1Char('l'), QLatin1Char('i'),
        QLatin1Char('d')
    };
    QTest::newRow("sweep_invalid_10") << QSTRING_FROM_QCHARARRAY(invalid_10) << false << 9;

    static const QChar invalid_11[] = {
        QChar(0xdc00), QChar(0xd800),
        QLatin1Char('i'), QLatin1Char('n'), QLatin1Char('v'),
        QLatin1Char('a'), QLatin1Char('l'), QLatin1Char('i'),
        QLatin1Char('d')
    };
    QTest::newRow("sweep_invalid_11") << QSTRING_FROM_QCHARARRAY(invalid_11) << false << 9;

    static const QChar invalid_12[] = {
        QChar(0xdc00), QChar(0xdc00),
        QLatin1Char('i'), QLatin1Char('n'), QLatin1Char('v'),
        QLatin1Char('a'), QLatin1Char('l'), QLatin1Char('i'),
        QLatin1Char('d')
    };
    QTest::newRow("sweep_invalid_12") << QSTRING_FROM_QCHARARRAY(invalid_12) << false << 9;

    static const QChar invalid_13[] = {
        QLatin1Char('i'), QLatin1Char('n'), QLatin1Char('v'),
        QChar(0xd800), QChar(0xdf00), // U+10300 OLD ITALIC LETTER A
        QLatin1Char('a'), QLatin1Char('l'), QLatin1Char('i'),
        QLatin1Char('d'), QChar(0xd800)
    };
    QTest::newRow("sweep_invalid_13") << QSTRING_FROM_QCHARARRAY(invalid_13) << false << 9;

    static const QChar invalid_14[] = {
        QLatin1Char('i'), QLatin1Char('n'), QLatin1Char('v'),
        QChar(0xd800), QChar(0xdf00), // U+10300 OLD ITALIC LETTER A
        QLatin1Char('a'), QLatin1Char('l'), QLatin1Char('i'),
        QLatin1Char('d'), QChar(0xd800), QLatin1Char('x')
    };
    QTest::newRow("sweep_invalid_14") << QSTRING_FROM_QCHARARRAY(invalid_14) << false << 10;

    static const QChar invalid_15[] = {
        QLatin1Char('i'), QLatin1Char('n'), QLatin1Char('v'),
        QChar(0xd800), QChar(0xdf00), // U+10300 OLD ITALIC LETTER A
        QLatin1Char('a'), QLatin1Char('l'), QLatin1Char('i'),
        QLatin1Char('d'), QChar(0xdc00)
    };
    QTest::newRow("sweep_invalid_15") << QSTRING_FROM_QCHARARRAY(invalid_15) << false << 9;

    static const QChar invalid_16[] = {
        QLatin1Char('i'), QLatin1Char('n'), QLatin1Char('v'),
        QChar(0xd800), QChar(0xdf00), // U+10300 OLD ITALIC LETTER A
        QLatin1Char('a'), QLatin1Char('l'), QLatin1Char('i'),
        QLatin1Char('d'), QChar(0xdc00), QLatin1Char('x')
    };
    QTest::newRow("sweep_invalid_16") << QSTRING_FROM_QCHARARRAY(invalid_16) << false << 10;

    static const QChar invalid_17[] = {
        QChar(0xd800),
        QLatin1Char('i'), QLatin1Char('n'), QLatin1Char('v'),
        QChar(0xd800), QChar(0xdf00), // U+10300 OLD ITALIC LETTER A
        QLatin1Char('a'), QLatin1Char('l'), QLatin1Char('i'),
        QLatin1Char('d')
    };
    QTest::newRow("sweep_invalid_17") << QSTRING_FROM_QCHARARRAY(invalid_17) << false << 9;

    static const QChar invalid_18[] = {
        QChar(0xdc00),
        QLatin1Char('i'), QLatin1Char('n'), QLatin1Char('v'),
        QChar(0xd800), QChar(0xdf00), // U+10300 OLD ITALIC LETTER A
        QLatin1Char('a'), QLatin1Char('l'), QLatin1Char('i'),
        QLatin1Char('d')
    };
    QTest::newRow("sweep_invalid_18") << QSTRING_FROM_QCHARARRAY(invalid_18) << false << 9;

#undef QSTRING_FROM_QCHARARRAY
}

void tst_QStringIterator::sweep()
{
    QFETCH(QString, string);
    QFETCH(bool, valid);

    QStringIterator i(string);
    int count = 0;
    QString rebuiltString;

    while (i.hasNext()) {
        const uint peekedCodePoint = i.peekNext(~0u);
        const uint codePoint = i.next(~0u);

        QVERIFY(peekedCodePoint == codePoint);

        if (codePoint == ~0u)
            rebuiltString += *(i.position() - 1);
        else
            rebuiltString += QString::fromUcs4(&codePoint, 1);

        ++count;
    }

    QTEST(count, "count");
    QTEST(rebuiltString, "string");
    rebuiltString.clear();

    while (i.hasPrevious()) {
        const uint peekedCodePoint = i.peekPrevious(~0u);
        const uint codePoint = i.previous(~0u);

        QVERIFY(peekedCodePoint == codePoint);

        --count;
    }

    QCOMPARE(count, 0);

    while (i.hasNext()) {
        i.advance();
        ++count;
    }

    QTEST(count, "count");

    while (i.hasPrevious()) {
        i.recede();
        --count;
    }

    QCOMPARE(count, 0);

    if (valid) {
        while (i.hasNext()) {
            const uint peekedCodePoint = i.peekNextUnchecked();
            const uint codePoint = i.nextUnchecked();

            QVERIFY(peekedCodePoint == codePoint);
            QVERIFY(codePoint <= 0x10FFFFu);
            rebuiltString += QString::fromUcs4(&codePoint, 1);
            ++count;
        }

        QTEST(count, "count");
        QTEST(rebuiltString, "string");

        while (i.hasPrevious()) {
            const uint peekedCodePoint = i.peekPreviousUnchecked();
            const uint codePoint = i.previousUnchecked();

            QVERIFY(peekedCodePoint == codePoint);

            --count;
        }

        QCOMPARE(count, 0);

        while (i.hasNext()) {
            i.advanceUnchecked();
            ++count;
        }

        QTEST(count, "count");

        while (i.hasPrevious()) {
            i.recedeUnchecked();
            --count;
        }

        QCOMPARE(count, 0);
    }
}

void tst_QStringIterator::position()
{
    static const QChar stringData[] =
    {
        // codeunit count: 0
        QLatin1Char('a'), QLatin1Char('b'), QLatin1Char('c'),
        // codeunit count: 3
        QChar(0x00A9), // U+00A9 COPYRIGHT SIGN
        // codeunit count: 4
        QChar(0x00AE), // U+00AE REGISTERED SIGN
        // codeunit count: 5
        QLatin1Char('d'), QLatin1Char('e'), QLatin1Char('f'),
        // codeunit count: 8
        QLatin1Char('\0'),
        // codeunit count: 9
        QLatin1Char('g'), QLatin1Char('h'), QLatin1Char('i'),
        // codeunit count: 12
        QChar(0xD834), QChar(0xDD1E), // U+1D11E MUSICAL SYMBOL G CLEF
        // codeunit count: 14
        QChar(0xD834), QChar(0xDD21), // U+1D121 MUSICAL SYMBOL C CLEF
        // codeunit count: 16
        QLatin1Char('j'),
        // codeunit count: 17
        QChar(0xD800), // stray high surrogate
        // codeunit count: 18
        QLatin1Char('k'),
        // codeunit count: 19
        QChar(0xDC00), // stray low surrogate
        // codeunit count: 20
        QLatin1Char('l'),
        // codeunit count: 21
        QChar(0xD800), QChar(0xD800), // two high surrogates
        // codeunit count: 23
        QLatin1Char('m'),
        // codeunit count: 24
        QChar(0xDC00), QChar(0xDC00), // two low surrogates
        // codeunit count: 26
        QLatin1Char('n'),
        // codeunit count: 27
        QChar(0xD800), QChar(0xD800), QChar(0xDC00), // stray high surrogate followed by valid pair
        // codeunit count: 30
        QLatin1Char('o'),
        // codeunit count: 31
        QChar(0xDC00), QChar(0xD800), QChar(0xDC00), // stray low surrogate followed by valid pair
        // codeunit count: 34
        QLatin1Char('p')
        // codeunit count: 35
    };
    static const int stringDataSize = sizeof(stringData) / sizeof(stringData[0]);

    QStringIterator i(QStringView(stringData, stringDataSize));

    QCOMPARE(i.position(), stringData);
    QVERIFY(i.hasNext());
    QVERIFY(!i.hasPrevious());

    i.setPosition(stringData + stringDataSize);
    QCOMPARE(i.position(), stringData + stringDataSize);
    QVERIFY(!i.hasNext());
    QVERIFY(i.hasPrevious());

#define QCHAR_UNICODE_VALUE(x) ((uint)(QChar(x).unicode()))

    const QChar *begin = stringData;
    i.setPosition(begin);
    QCOMPARE(i.position(), begin);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(QLatin1Char('a')));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(QLatin1Char('a')));

    QCOMPARE(i.position(), begin + 1);

    i.setPosition(begin + 2);
    QCOMPARE(i.position(), begin + 2);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(QLatin1Char('c')));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(QLatin1Char('c')));

    QCOMPARE(i.position(), begin + 3);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(0x00A9));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(0x00A9));

    QCOMPARE(i.position(), begin + 4);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(0x00AE));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(0x00AE));

    QCOMPARE(i.position(), begin + 5);
    QCOMPARE(i.peekPrevious(), QCHAR_UNICODE_VALUE(0x00AE));
    QCOMPARE(i.previous(), QCHAR_UNICODE_VALUE(0x00AE));

    QCOMPARE(i.position(), begin + 4);
    QCOMPARE(i.peekPrevious(), QCHAR_UNICODE_VALUE(0x00A9));
    QCOMPARE(i.previous(), QCHAR_UNICODE_VALUE(0x00A9));

    QCOMPARE(i.position(), begin + 3);

    i.setPosition(begin + 8);
    QCOMPARE(i.position(), begin + 8);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(QLatin1Char('\0')));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(QLatin1Char('\0')));

    QCOMPARE(i.position(), begin + 9);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(QLatin1Char('g')));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(QLatin1Char('g')));

    QCOMPARE(i.position(), begin + 10);
    QCOMPARE(i.peekPrevious(), QCHAR_UNICODE_VALUE(QLatin1Char('g')));
    QCOMPARE(i.previous(), QCHAR_UNICODE_VALUE(QLatin1Char('g')));

    QCOMPARE(i.position(), begin + 9);
    QCOMPARE(i.peekPrevious(), QCHAR_UNICODE_VALUE(QLatin1Char('\0')));
    QCOMPARE(i.previous(), QCHAR_UNICODE_VALUE(QLatin1Char('\0')));

    QCOMPARE(i.position(), begin + 8);
    QCOMPARE(i.peekPrevious(), QCHAR_UNICODE_VALUE(QLatin1Char('f')));
    QCOMPARE(i.previous(), QCHAR_UNICODE_VALUE(QLatin1Char('f')));

    QCOMPARE(i.position(), begin + 7);

    i.advanceUnchecked();
    i.advanceUnchecked();
    i.advanceUnchecked();
    i.advanceUnchecked();
    i.advanceUnchecked();

    QCOMPARE(i.position(), begin + 12);
    QCOMPARE(i.peekNext(), 0x1D11Eu);
    QCOMPARE(i.next(), 0x1D11Eu);

    QCOMPARE(i.position(), begin + 14);
    QCOMPARE(i.peekNext(), 0x1D121u);
    QCOMPARE(i.next(), 0x1D121u);

    QCOMPARE(i.position(), begin + 16);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(QLatin1Char('j')));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(QLatin1Char('j')));

    QCOMPARE(i.position(), begin + 17);
    QCOMPARE(i.peekPrevious(), QCHAR_UNICODE_VALUE(QLatin1Char('j')));
    QCOMPARE(i.previous(), QCHAR_UNICODE_VALUE(QLatin1Char('j')));

    QCOMPARE(i.position(), begin + 16);
    QCOMPARE(i.peekPrevious(), 0x1D121u);
    QCOMPARE(i.previous(), 0x1D121u);

    QCOMPARE(i.position(), begin + 14);
    QCOMPARE(i.peekPrevious(), 0x1D11Eu);
    QCOMPARE(i.previous(), 0x1D11Eu);

    QCOMPARE(i.position(), begin + 12);


    i.setPosition(begin + 13);
    QCOMPARE(i.position(), begin + 13);

    QCOMPARE(i.peekNext(), 0xFFFDu);
    QCOMPARE(i.next(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 14);
    QCOMPARE(i.peekNext(), 0x1D121u);
    QCOMPARE(i.next(), 0x1D121u);

    QCOMPARE(i.position(), begin + 16);


    i.setPosition(begin + 15);
    QCOMPARE(i.position(), begin + 15);

    QCOMPARE(i.peekPrevious(), 0xFFFDu);
    QCOMPARE(i.previous(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 14);
    QCOMPARE(i.peekPrevious(), 0x1D11Eu);
    QCOMPARE(i.previous(), 0x1D11Eu);

    QCOMPARE(i.position(), begin + 12);

    i.advanceUnchecked();
    i.advanceUnchecked();

    QCOMPARE(i.position(), begin + 16);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(QLatin1Char('j')));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(QLatin1Char('j')));

    QCOMPARE(i.position(), begin + 17);
    QCOMPARE(i.peekNext(), 0xFFFDu);
    QCOMPARE(i.next(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 18);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(QLatin1Char('k')));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(QLatin1Char('k')));

    QCOMPARE(i.position(), begin + 19);
    QCOMPARE(i.peekNext(), 0xFFFDu);
    QCOMPARE(i.next(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 20);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(QLatin1Char('l')));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(QLatin1Char('l')));

    QCOMPARE(i.position(), begin + 21);
    QCOMPARE(i.peekNext(), 0xFFFDu);
    QCOMPARE(i.next(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 22);
    QCOMPARE(i.peekNext(), 0xFFFDu);
    QCOMPARE(i.next(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 23);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(QLatin1Char('m')));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(QLatin1Char('m')));

    QCOMPARE(i.position(), begin + 24);
    QCOMPARE(i.peekNext(), 0xFFFDu);
    QCOMPARE(i.next(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 25);
    QCOMPARE(i.peekNext(), 0xFFFDu);
    QCOMPARE(i.next(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 26);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(QLatin1Char('n')));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(QLatin1Char('n')));

    QCOMPARE(i.position(), begin + 27);
    QCOMPARE(i.peekNext(), 0xFFFDu);
    QCOMPARE(i.next(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 28);
    QCOMPARE(i.peekNext(), 0x10000u);
    QCOMPARE(i.next(), 0x10000u);

    QCOMPARE(i.position(), begin + 30);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(QLatin1Char('o')));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(QLatin1Char('o')));

    QCOMPARE(i.position(), begin + 31);
    QCOMPARE(i.peekNext(), 0xFFFDu);
    QCOMPARE(i.next(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 32);
    QCOMPARE(i.peekNext(), 0x10000u);
    QCOMPARE(i.next(), 0x10000u);

    QCOMPARE(i.position(), begin + 34);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(QLatin1Char('p')));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(QLatin1Char('p')));

    QVERIFY(!i.hasNext());

    QCOMPARE(i.position(), begin + 35);
    QCOMPARE(i.peekPrevious(), QCHAR_UNICODE_VALUE(QLatin1Char('p')));
    QCOMPARE(i.previous(), QCHAR_UNICODE_VALUE(QLatin1Char('p')));

    QCOMPARE(i.position(), begin + 34);
    QCOMPARE(i.peekPrevious(), 0x10000u);
    QCOMPARE(i.previous(), 0x10000u);

    QCOMPARE(i.position(), begin + 32);
    QCOMPARE(i.peekPrevious(), 0xFFFDu);
    QCOMPARE(i.previous(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 31);
    QCOMPARE(i.peekPrevious(), QCHAR_UNICODE_VALUE(QLatin1Char('o')));
    QCOMPARE(i.previous(), QCHAR_UNICODE_VALUE(QLatin1Char('o')));

    QCOMPARE(i.position(), begin + 30);
    QCOMPARE(i.peekPrevious(), 0x10000u);
    QCOMPARE(i.previous(), 0x10000u);

    QCOMPARE(i.position(), begin + 28);
    QCOMPARE(i.peekPrevious(), 0xFFFDu);
    QCOMPARE(i.previous(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 27);
    QCOMPARE(i.peekPrevious(), QCHAR_UNICODE_VALUE(QLatin1Char('n')));
    QCOMPARE(i.previous(), QCHAR_UNICODE_VALUE(QLatin1Char('n')));

    QCOMPARE(i.position(), begin + 26);
    QCOMPARE(i.peekPrevious(), 0xFFFDu);
    QCOMPARE(i.previous(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 25);
    QCOMPARE(i.peekPrevious(), 0xFFFDu);
    QCOMPARE(i.previous(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 24);
    QCOMPARE(i.peekPrevious(), QCHAR_UNICODE_VALUE(QLatin1Char('m')));
    QCOMPARE(i.previous(), QCHAR_UNICODE_VALUE(QLatin1Char('m')));

    QCOMPARE(i.position(), begin + 23);
    QCOMPARE(i.peekPrevious(), 0xFFFDu);
    QCOMPARE(i.previous(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 22);
    QCOMPARE(i.peekPrevious(), 0xFFFDu);
    QCOMPARE(i.previous(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 21);
    QCOMPARE(i.peekPrevious(), QCHAR_UNICODE_VALUE(QLatin1Char('l')));
    QCOMPARE(i.previous(), QCHAR_UNICODE_VALUE(QLatin1Char('l')));

    QCOMPARE(i.position(), begin + 20);
    QCOMPARE(i.peekPrevious(), 0xFFFDu);
    QCOMPARE(i.previous(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 19);
    QCOMPARE(i.peekPrevious(), QCHAR_UNICODE_VALUE(QLatin1Char('k')));
    QCOMPARE(i.previous(), QCHAR_UNICODE_VALUE(QLatin1Char('k')));

    QCOMPARE(i.position(), begin + 18);
    QCOMPARE(i.peekPrevious(), 0xFFFDu);
    QCOMPARE(i.previous(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 17);
    QCOMPARE(i.peekPrevious(), QCHAR_UNICODE_VALUE(QLatin1Char('j')));
    QCOMPARE(i.previous(), QCHAR_UNICODE_VALUE(QLatin1Char('j')));

    i.setPosition(begin + 29);
    QCOMPARE(i.position(), begin + 29);
    QCOMPARE(i.peekNext(), 0xFFFDu);
    QCOMPARE(i.next(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 30);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(QLatin1Char('o')));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(QLatin1Char('o')));

    QCOMPARE(i.position(), begin + 31);
    QCOMPARE(i.peekPrevious(), QCHAR_UNICODE_VALUE(QLatin1Char('o')));
    QCOMPARE(i.previous(), QCHAR_UNICODE_VALUE(QLatin1Char('o')));

    QCOMPARE(i.position(), begin + 30);
    QCOMPARE(i.peekPrevious(), 0x10000u);
    QCOMPARE(i.previous(), 0x10000u);

    QCOMPARE(i.position(), begin + 28);

    i.setPosition(begin + 33);
    QCOMPARE(i.position(), begin + 33);
    QCOMPARE(i.peekNext(), 0xFFFDu);
    QCOMPARE(i.next(), 0xFFFDu);

    QCOMPARE(i.position(), begin + 34);
    QCOMPARE(i.peekNext(), QCHAR_UNICODE_VALUE(QLatin1Char('p')));
    QCOMPARE(i.next(), QCHAR_UNICODE_VALUE(QLatin1Char('p')));

    QCOMPARE(i.position(), begin + 35);
    QCOMPARE(i.peekPrevious(), QCHAR_UNICODE_VALUE(QLatin1Char('p')));
    QCOMPARE(i.previous(), QCHAR_UNICODE_VALUE(QLatin1Char('p')));

    QCOMPARE(i.position(), begin + 34);
    QCOMPARE(i.peekPrevious(), 0x10000u);
    QCOMPARE(i.previous(), 0x10000u);

    QCOMPARE(i.position(), begin + 32);


    i.setPosition(begin + 16);
    QCOMPARE(i.position(), begin + 16);

    i.recedeUnchecked();
    i.recedeUnchecked();
    QCOMPARE(i.position(), begin + 12);

    i.recedeUnchecked();
    i.recedeUnchecked();
    i.recedeUnchecked();
    i.recedeUnchecked();
    QCOMPARE(i.position(), begin + 8);

    i.recedeUnchecked();
    i.recedeUnchecked();
    i.recedeUnchecked();
    i.recedeUnchecked();
    i.recedeUnchecked();
    i.recedeUnchecked();
    QCOMPARE(i.position(), begin + 2);

#undef QCHAR_UNICODE_VALUE
}

QTEST_APPLESS_MAIN(tst_QStringIterator)

#include "tst_qstringiterator.moc"
