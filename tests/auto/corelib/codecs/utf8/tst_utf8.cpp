/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtTest/QtTest>

#include <qtextcodec.h>
#include <qsharedpointer.h>

//TESTED_CLASS=
//TESTED_FILES=

static const char utf8bom[] = "\xEF\xBB\xBF";

class tst_Utf8 : public QObject
{
    Q_OBJECT

public:
    // test data:
    QTextCodec *codec;
    QString (*from8BitPtr)(const char *, int);
    QByteArray (QString:: *to8Bit)() const;

    inline QString from8Bit(const QByteArray &ba)
    { return from8BitPtr(ba.constData(), ba.length()); }
public slots:
    void initTestCase();
    void init();

private slots:
    void roundTrip_data();
    void roundTrip();

    void charByChar_data();
    void charByChar();

    void invalidUtf8_data();
    void invalidUtf8();

    void nonCharacters_data();
    void nonCharacters();
};

void tst_Utf8::initTestCase()
{
    QTest::addColumn<bool>("useLocale");
    QTest::newRow("utf8codec") << false;

    // is the locale UTF-8?
    if (QString(QChar(QChar::ReplacementCharacter)).toLocal8Bit() == "\xEF\xBF\xBD") {
        QTest::newRow("localecodec") << true;
        qDebug() << "locale is utf8";
    }
}

void tst_Utf8::init()
{
    QFETCH_GLOBAL(bool, useLocale);
    if (useLocale) {
        codec = QTextCodec::codecForLocale();
        from8BitPtr = &QString::fromLocal8Bit;
        to8Bit = &QString::toLocal8Bit;
    } else {
        codec = QTextCodec::codecForMib(106);
        from8BitPtr = &QString::fromUtf8;
        to8Bit = &QString::toUtf8;
    }
}

void tst_Utf8::roundTrip_data()
{
    QTest::addColumn<QByteArray>("utf8");
    QTest::addColumn<QString>("utf16");

    QTest::newRow("empty") << QByteArray() << QString();
    QTest::newRow("nul") << QByteArray("", 1) << QString(QChar(QChar::Null));

    static const char ascii[] = "This is a standard US-ASCII message";
    QTest::newRow("ascii") << QByteArray(ascii) << ascii;

    static const char ascii2[] = "\1This\2is\3an\4US-ASCII\020 message interspersed with control chars";
    QTest::newRow("ascii2") << QByteArray(ascii2) << ascii2;

    static const char utf8_1[] = "\302\240"; // NBSP
    QTest::newRow("utf8_1") << QByteArray(utf8_1) << QString(QChar(QChar::Nbsp));

    static const char utf8_2[] = "\342\202\254"; // Euro symbol
    QTest::newRow("utf8_2") << QByteArray(utf8_2) << QString(QChar(0x20AC));

#if 0
    // Can't test this because QString::fromUtf8 consumes it
    static const char utf8_3[] = "\357\273\277"; // byte order mark
    QTest::newRow("utf8_3") << QByteArray(utf8_3) << QString(QChar(QChar::ByteOrderMark));
#endif

    static const char utf8_4[] = "\357\277\275"; // replacement char
    QTest::newRow("utf8_4") << QByteArray(utf8_4) << QString(QChar(QChar::ReplacementCharacter));

    static const char utf8_5[] = "\360\220\210\203"; // U+010203
    static const uint utf32_5[] = { 0x010203 };
    QTest::newRow("utf8_5") << QByteArray(utf8_5) << QString::fromUcs4(utf32_5, 1);

    static const char utf8_6[] = "\364\217\277\275"; // U+10FFFD
    static const uint utf32_6[] = { 0x10FFFD };
    QTest::newRow("utf8_6") << QByteArray(utf8_6) << QString::fromUcs4(utf32_6, 1);

    static const char utf8_7[] = "abc\302\240\303\241\303\251\307\275 \342\202\254def";
    static const ushort utf16_7[] = { 'a', 'b', 'c', 0x00A0,
                                      0x00E1, 0x00E9, 0x01FD,
                                      ' ', 0x20AC, 'd', 'e', 'f', 0 };
    QTest::newRow("utf8_7") << QByteArray(utf8_7) << QString::fromUtf16(utf16_7);

    static const char utf8_8[] = "abc\302\240\303\241\303\251\307\275 \364\217\277\275 \342\202\254def";
    static const uint utf32_8[] = { 'a', 'b', 'c', 0x00A0,
                                    0x00E1, 0x00E9, 0x01FD,
                                    ' ', 0x10FFFD, ' ',
                                    0x20AC, 'd', 'e', 'f', 0 };
    QTest::newRow("utf8_8") << QByteArray(utf8_8) << QString::fromUcs4(utf32_8);
}

void tst_Utf8::roundTrip()
{
    QFETCH(QByteArray, utf8);
    QFETCH(QString, utf16);

    QCOMPARE((utf16.*to8Bit)(), utf8);
    QCOMPARE(from8Bit(utf8), utf16);

    QCOMPARE((from8Bit(utf8).*to8Bit)(), utf8);
    QCOMPARE(from8Bit((utf16.*to8Bit)()), utf16);
}

void tst_Utf8::charByChar_data()
{
    roundTrip_data();
}

void tst_Utf8::charByChar()
{
    QFETCH(QByteArray, utf8);
    QFETCH(QString, utf16);

    {
        // from utf16 to utf8 char by char:
        QSharedPointer<QTextEncoder> encoder = QSharedPointer<QTextEncoder>(codec->makeEncoder());
        QByteArray encoded;

        for (int i = 0; i < utf16.length(); ++i) {
            encoded += encoder->fromUnicode(utf16.constData() + i, 1);
            QVERIFY(!encoder->hasFailure());
        }

        if (encoded.startsWith(utf8bom))
            encoded = encoded.mid(strlen(utf8bom));
        QCOMPARE(encoded, utf8);
    }
    {
        // from utf8 to utf16 char by char:
        QSharedPointer<QTextDecoder> decoder = QSharedPointer<QTextDecoder>(codec->makeDecoder());
        QString decoded;

        for (int i = 0; i < utf8.length(); ++i) {
            decoded += decoder->toUnicode(utf8.constData() + i, 1);
            QVERIFY(!decoder->hasFailure());
        }

        QCOMPARE(decoded, utf16);
    }
}

void tst_Utf8::invalidUtf8_data()
{
    QTest::addColumn<QByteArray>("utf8");

    QTest::newRow("1char") << QByteArray("\x80");
    QTest::newRow("2chars-1") << QByteArray("\xC2\xC0");
    QTest::newRow("2chars-2") << QByteArray("\xC3\xDF");
    QTest::newRow("2chars-3") << QByteArray("\xC7\xF0");
    QTest::newRow("3chars-1") << QByteArray("\xE0\xA0\xC0");
    QTest::newRow("3chars-2") << QByteArray("\xE0\xC0\xA0");
    QTest::newRow("4chars-1") << QByteArray("\xF0\x90\x80\xC0");
    QTest::newRow("4chars-2") << QByteArray("\xF0\x90\xC0\x80");
    QTest::newRow("4chars-3") << QByteArray("\xF0\xC0\x80\x80");

    // Surrogate pairs must now be present either
    // U+D800:        1101   10 0000   00 0000
    // encoding: xxxz:1101 xz10:0000 xz00:0000
    QTest::newRow("hi-surrogate") << QByteArray("\xED\xA0\x80");
    // U+DC00:        1101   11 0000   00 0000
    // encoding: xxxz:1101 xz11:0000 xz00:0000
    QTest::newRow("lo-surrogate") << QByteArray("\xED\xB0\x80");

    // not even in pair:
    QTest::newRow("surrogate-pair") << QByteArray("\xED\xA0\x80\xED\xB0\x80");

    // Characters outside the Unicode range:
    // 0x110000:   00 0100   01 0000   00 0000   00 0000
    // encoding: xxxx:z100 xz01:0000 xz00:0000 xz00:0000
    QTest::newRow("non-unicode-1") << QByteArray("\xF4\x90\x80\x80");
    // 0x200000:             00 1000   00 0000   00 0000   00 0000
    // encoding: xxxx:xz00 xz00:1000 xz00:0000 xz00:0000 xz00:0000
    QTest::newRow("non-unicode-2") << QByteArray("\xF8\x88\x80\x80\x80");
    // 0x04000000:              0100   00 0000   00 0000   00 0000   00 0000
    // encoding: xxxx:xxz0 xz00:0100 xz00:0000 xz00:0000 xz00:0001 xz00:0001
    QTest::newRow("non-unicode-3") << QByteArray("\xFC\x84\x80\x80\x80\x80");
    // 0x7fffffff:       1   11 1111   11 1111   11 1111   11 1111   11 1111
    // encoding: xxxx:xxz0 xz00:0100 xz00:0000 xz00:0000 xz00:0001 xz00:0001
    QTest::newRow("non-unicode-3") << QByteArray("\xFD\xBF\xBF\xBF\xBF\xBF");

    // As seen above, 0xFE and 0xFF never appear:
    QTest::newRow("fe") << QByteArray("\xFE");
    QTest::newRow("fe-bis") << QByteArray("\xFE\xBF\xBF\xBF\xBF\xBF\xBF");
    QTest::newRow("ff") << QByteArray("\xFF");
    QTest::newRow("ff-bis") << QByteArray("\xFF\xBF\xBF\xBF\xBF\xBF\xBF\xBF");

    // some combinations in UTF-8 are invalid even though they have the proper bits set
    // these are known as overlong sequences

    // "A": U+0041:                                               01   00 0001
    // overlong 2:                                         xxz0:0001 xz00:0001
    QTest::newRow("overlong-1-2") << QByteArray("\xC1\x81");
    // overlong 3:                               xxxz:0000 xz00:0001 xz00:0001
    QTest::newRow("overlong-1-3") << QByteArray("\xE0\x81\x81");
    // overlong 4:                     xxxx:z000 xz00:0000 xz00:0001 xz00:0001
    QTest::newRow("overlong-1-4") << QByteArray("\xF0\x80\x81\x81");
    // overlong 5:           xxxx:xz00 xz00:0000 xz00:0000 xz00:0001 xz00:0001
    QTest::newRow("overlong-1-5") << QByteArray("\xF8\x80\x80\x81\x81");
    // overlong 6: xxxx:xxz0 xz00:0000 xz00:0000 xz00:0000 xz00:0001 xz00:0001
    QTest::newRow("overlong-1-6") << QByteArray("\xFC\x80\x80\x80\x81\x81");

    // NBSP: U+00A0:                                             10    00 0000
    // proper encoding:                                    xxz0:0010 xz00:0000
    // overlong 3:                               xxxz:0000 xz00:0010 xz00:0000
    QTest::newRow("overlong-2-3") << QByteArray("\xC0\x82\x80");
    // overlong 4:                     xxxx:z000 xz00:0000 xz00:0010 xz00:0000
    QTest::newRow("overlong-2-4") << QByteArray("\xF0\x80\x82\x80");
    // overlong 5:           xxxx:xz00 xz00:0000 xz00:0000 xz00:0010 xz00:0000
    QTest::newRow("overlong-2-4") << QByteArray("\xF8\x80\x80\x82\x80");
    // overlong 6: xxxx:xxz0 xz00:0000 xz00:0000 xz00:0000 xz00:0010 xz00:0000
    QTest::newRow("overlong-2-4") << QByteArray("\xFC\x80\x80\x80\x82\x80");

    // U+0800:                                               10 0000   00 0000
    // proper encoding:                          xxxz:0000 xz10:0000 xz00:0000
    // overlong 4:                     xxxx:z000 xz00:0000 xz10:0000 xz00:0000
    QTest::newRow("overlong-3-4") << QByteArray("\xF0\x80\xA0\x80");
    // overlong 5:           xxxx:xz00 xz00:0000 xz00:0000 xz10:0000 xz00:0000
    QTest::newRow("overlong-3-5") << QByteArray("\xF8\x80\x80\xA0\x80");
    // overlong 6: xxxx:xxz0 xz00:0000 xz00:0000 xz00:0000 xz10:0000 xz00:0000
    QTest::newRow("overlong-3-6") << QByteArray("\xFC\x80\x80\x80\xA0\x80");

    // U+010000:                                   00 0100   00 0000   00 0000
    // proper encoding:                xxxx:z000 xz00:0100 xz00:0000 xz00:0000
    // overlong 5:           xxxx:xz00 xz00:0000 xz00:0100 xz00:0000 xz00:0000
    QTest::newRow("overlong-4-5") << QByteArray("\xF8\x80\x84\x80\x80");
    // overlong 6: xxxx:xxz0 xz00:0000 xz00:0000 xz00:0100 xz00:0000 xz00:0000
    QTest::newRow("overlong-4-6") << QByteArray("\xFC\x80\x80\x84\x80\x80");
}

void tst_Utf8::invalidUtf8()
{
    QFETCH(QByteArray, utf8);
    QFETCH_GLOBAL(bool, useLocale);

    QSharedPointer<QTextDecoder> decoder = QSharedPointer<QTextDecoder>(codec->makeDecoder());
    decoder->toUnicode(utf8);

    // Only enforce correctness on our UTF-8 decoder
    // The system's UTF-8 codec is sometimes buggy
    //  GNU libc's iconv is known to accept U+FFFF and U+FFFE encoded as UTF-8
    //  OS X's iconv is known to accept those, plus surrogates and codepoints above U+10FFFF
    if (!useLocale)
        QVERIFY(decoder->hasFailure());
    else if (!decoder->hasFailure())
        qWarning("System codec does not report failure when it should. Should report bug upstream.");
}

void tst_Utf8::nonCharacters_data()
{
    QTest::addColumn<QByteArray>("utf8");
    QTest::addColumn<QString>("utf16");

    // Unicode has a couple of "non-characters" that one can use internally,
    // but are not allowed to be used for text interchange.
    //
    // Those are the last two entries each Unicode Plane (U+FFFE, U+FFFF,
    // U+1FFFE, U+1FFFF, etc.) as well as the entries between U+FDD0 and
    // U+FDEF (inclusive)

    // U+FDD0 through U+FDEF
    for (int i = 0; i < 16; ++i) {
        char utf8[] = { 0357, 0267, 0220 + i, 0 };
        QString utf16 = QChar(0xfdd0 + i);
        QTest::newRow(qPrintable(QString::number(0xfdd0 + i, 16))) << QByteArray(utf8) << utf16;
    }

    // the last two in Planes 1 through 16
    for (uint plane = 1; plane <= 16; ++plane) {
        for (uint lower = 0xfffe; lower < 0x10000; ++lower) {
            uint ucs4 = (plane << 16) | lower;
            char utf8[] = { 0xf0 | uchar(ucs4 >> 18),
                            0x80 | (uchar(ucs4 >> 12) & 0x3f),
                            0x80 | (uchar(ucs4 >> 6) & 0x3f),
                            0x80 | (uchar(ucs4) & 0x3f),
                            0 };
            ushort utf16[] = { QChar::highSurrogate(ucs4), QChar::lowSurrogate(ucs4), 0 };

            QTest::newRow(qPrintable(QString::number(ucs4, 16))) << QByteArray(utf8) << QString::fromUtf16(utf16);
        }
    }

    QTest::newRow("fffe") << QByteArray("\xEF\xBF\xBE") << QString(QChar(0xfffe));
    QTest::newRow("ffff") << QByteArray("\xEF\xBF\xBF") << QString(QChar(0xffff));
}

void tst_Utf8::nonCharacters()
{
    QFETCH(QByteArray, utf8);
    QFETCH(QString, utf16);
    QFETCH_GLOBAL(bool, useLocale);

    QSharedPointer<QTextDecoder> decoder = QSharedPointer<QTextDecoder>(codec->makeDecoder());
    decoder->toUnicode(utf8);

    // Only enforce correctness on our UTF-8 decoder
    // The system's UTF-8 codec is sometimes buggy
    //  GNU libc's iconv is known to accept U+FFFF and U+FFFE encoded as UTF-8
    //  OS X's iconv is known to accept those, plus surrogates and codepoints above U+10FFFF
    if (!useLocale)
        QVERIFY(decoder->hasFailure());
    else if (!decoder->hasFailure())
        qWarning("System codec does not report failure when it should. Should report bug upstream.");

    QSharedPointer<QTextEncoder> encoder(codec->makeEncoder());
    encoder->fromUnicode(utf16);
    if (!useLocale)
        QVERIFY(encoder->hasFailure());
    else if (!encoder->hasFailure())
        qWarning("System codec does not report failure when it should. Should report bug upstream.");
}

QTEST_MAIN(tst_Utf8)
#include "tst_utf8.moc"
