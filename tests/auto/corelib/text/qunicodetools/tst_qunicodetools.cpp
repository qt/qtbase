// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <qchar.h>
#include <qfile.h>
#include <qstringlist.h>
#include <private/qunicodetables_p.h>
#include <private/qunicodetools_p.h>

class tst_QUnicodeTools : public QObject
{
    Q_OBJECT
private slots:
    void lineBreakClass();
    void graphemeBreakClass_data();
    void graphemeBreakClass();
    void wordBreakClass_data();
    void wordBreakClass();
    void sentenceBreakClass_data();
    void sentenceBreakClass();
};

void tst_QUnicodeTools::lineBreakClass()
{
    QVERIFY(QUnicodeTables::lineBreakClass(0x0029) == QUnicodeTables::LineBreak_CP);
    QVERIFY(QUnicodeTables::lineBreakClass(0x0041) == QUnicodeTables::LineBreak_AL);
    QVERIFY(QUnicodeTables::lineBreakClass(0x0033) == QUnicodeTables::LineBreak_NU);
    QVERIFY(QUnicodeTables::lineBreakClass(0x00ad) == QUnicodeTables::LineBreak_BA);
    QVERIFY(QUnicodeTables::lineBreakClass(0x05d0) == QUnicodeTables::LineBreak_HL);
    QVERIFY(QUnicodeTables::lineBreakClass(0xfffc) == QUnicodeTables::LineBreak_CB);
    QVERIFY(QUnicodeTables::lineBreakClass(0xe0164) == QUnicodeTables::LineBreak_CM);
    QVERIFY(QUnicodeTables::lineBreakClass(0x2f9a4) == QUnicodeTables::LineBreak_ID);
    QVERIFY(QUnicodeTables::lineBreakClass(0x10000) == QUnicodeTables::LineBreak_AL);
    QVERIFY(QUnicodeTables::lineBreakClass(0x1f1e6) == QUnicodeTables::LineBreak_RI);

    // mapped to AL:
    QVERIFY(QUnicodeTables::lineBreakClass(0xfffd) == QUnicodeTables::LineBreak_AL); // AI -> AL
    QVERIFY(QUnicodeTables::lineBreakClass(0x100000) == QUnicodeTables::LineBreak_AL); // XX -> AL
}

static void verifyCharClassPattern(QString str, qulonglong pattern,
                                   QUnicodeTools::CharAttributeOptions type)
{
    QUnicodeTools::ScriptItemArray scriptItems;
    QUnicodeTools::initScripts(str, &scriptItems);
    QCharAttributes cleared;
    memset(&cleared, 0, sizeof(QCharAttributes));
    QList<QCharAttributes> attributes(str.size() + 1, cleared);
    QUnicodeTools::initCharAttributes(str, scriptItems.data(), scriptItems.size(),
                                      attributes.data(), type);

    qulonglong bit = 1ull << str.size();
    Q_ASSERT(str.size() < std::numeric_limits<decltype(bit)>::digits);
    for (qsizetype i = 0; i < str.size(); ++i) {
        bit >>= 1;
        bool test = pattern & bit;
        bool isSet = false;
        switch (type) {
            case QUnicodeTools::GraphemeBreaks:
                isSet = attributes[i].graphemeBoundary;
                break;
            case QUnicodeTools::WordBreaks:
                isSet = attributes[i].wordBreak;
                break;
            case QUnicodeTools::SentenceBreaks:
                isSet = attributes[i].sentenceBoundary;
                break;
            default:
                Q_UNREACHABLE();
                break;
        };
        QVERIFY2(isSet == test,
                 qPrintable(QString("Character #%1: 0x%2, isSet: %3")
                        .arg(i).arg(str[i].unicode(), 0, 16).arg(isSet)));
    }
}

void tst_QUnicodeTools::graphemeBreakClass_data()
{
    QTest::addColumn<QString>("str");
    QTest::addColumn<int>("pattern");

    // A grapheme cluster is a set of unicode code points that is
    // seen as a single character.
    // The pattern has one bit per code point.
    // A pattern bit is set whenever a new grapheme cluster begins.
    // A pattern bit is cleared for every code point that modifies
    // the current graphene cluster.

    QTest::addRow("g and combining diaeresis")
            << u8"g\u0308"
            << 0b10;
    QTest::addRow("hangul gag single")
            << u8"\uAC01"
            << 0b1;
    QTest::addRow("hangul gag cluster")
            << u8"\u1100\u1161\u11A8"
            << 0b100;
    QTest::addRow("thai ko")
            << u8"\u0E01"
            << 0b1;
    QTest::addRow("tamil ni")
            << u8"\u0BA8\u0BBF"
            << 0b10;
    QTest::addRow("thai e")
            << u8"\u0E40"
            << 0b1;
    QTest::addRow("thai kam")
            << u8"\u0E01\u0E33"
            << 0b10;
    QTest::addRow("devanagari ssi")
            << u8"\u0937\u093F"
            << 0b10;
    QTest::addRow("thai am")
            << u8"\u0E33"
            << 0b1;
    QTest::addRow("devanagari ssa")
            << u8"\u0937"
            << 0b1;
    QTest::addRow("devanagari i")
            << u8"\u093F"
            << 0b1;
    QTest::addRow("devanagari kshi")
            << u8"\u0915\u094D\u0937\u093F"
            << 0b1000;
}

void tst_QUnicodeTools::graphemeBreakClass()
{
    QFETCH(QString, str);
    QFETCH(int, pattern);

    verifyCharClassPattern(str, pattern, QUnicodeTools::GraphemeBreaks);
}

void tst_QUnicodeTools::wordBreakClass_data()
{
    QTest::addColumn<QString>("str");
    QTest::addColumn<qulonglong>("pattern");

    // Word boundaries are used for things like selection and whole word search.
    // Typically they are beginning of words, whitespaces and punctuation.

    QTest::addRow("two words")
            <<  "two words"
            << 0b100110000ULL;
            // breaks at beginning of words and space
    QTest::addRow("three words")
            <<  "The quick fox"
            << 0b1001100001100ULL;
            // breaks at beginning of words and spaces
    QTest::addRow("quoted")
            << u8"The quick (\"brown\") fox"
            <<  0b10011000011'110000'111100ULL;
            // as above plus quotes and parentesis
    QTest::addRow("long")
            <<  "The quick (\"brown\") fox can’t jump 32.3 feet, right?"
            << 0b10011000011'110000'11110011000011000110001100011100001ULL;
            // as above plus commma and question mark
            // but decimal separator and apostrophes are not word breaks
}

void tst_QUnicodeTools::wordBreakClass()
{
    QFETCH(QString, str);
    QFETCH(qulonglong, pattern);

    verifyCharClassPattern(str, pattern, QUnicodeTools::WordBreaks);
}

void tst_QUnicodeTools::sentenceBreakClass_data()
{
    QTest::addColumn<QString>("str");
    QTest::addColumn<qulonglong>("pattern");

    // Sentence boundaries are at the beginning of each new sentence

    QTest::addRow("one sentence")
            <<  "One sentence."
            << 0b1000000000000ULL;
    QTest::addRow("two sentences")
            <<  "One sentence. One more."
            << 0b10000000000000100000000ULL;
    QTest::addRow("question")
            <<  "Who said \"Hey you?\" I did."
            << 0b100000000'000000000'00100000ULL;
}

void tst_QUnicodeTools::sentenceBreakClass()
{
    QFETCH(QString, str);
    QFETCH(qulonglong, pattern);

    verifyCharClassPattern(str, pattern, QUnicodeTools::SentenceBreaks);
}

QTEST_APPLESS_MAIN(tst_QUnicodeTools)
#include "tst_qunicodetools.moc"
