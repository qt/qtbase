/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include <QTest>

#include <qstringconverter.h>
#include <qthreadpool.h>

#include <array>

enum CodecLimitation {
    AsciiOnly,
    Latin1Only,
    FullUnicode
};

#ifdef Q_OS_WIN
#  include <qt_windows.h>
static bool localeIsUtf8()
{
    return GetACP() == CP_UTF8;
}
#else
static constexpr bool localeIsUtf8()
{
    return true;
}
#endif

struct Codec
{
    const char name[12];
    QStringConverter::Encoding code;
    CodecLimitation limitation = FullUnicode;
};
static const std::array codes = {
    Codec{ "UTF-8", QStringConverter::Utf8 },
    Codec{ "UTF-16", QStringConverter::Utf16 },
    Codec{ "UTF-16-le", QStringConverter::Utf16LE },
    Codec{ "UTF-16-be", QStringConverter::Utf16BE },
    Codec{ "UTF-32", QStringConverter::Utf32 },
    Codec{ "UTF-32-le", QStringConverter::Utf32LE },
    Codec{ "UTF-32-be", QStringConverter::Utf32BE },
    Codec{ "Latin-1", QStringConverter::Latin1, Latin1Only },
    Codec{ "System", QStringConverter::System, localeIsUtf8() ? FullUnicode : AsciiOnly }
};

struct TestString
{
    const char *description;
    QUtf8StringView utf8;
    QStringView utf16;
    CodecLimitation limitation = FullUnicode;
};
static const std::array testStrings = {
    TestString{ "empty", "", u"", AsciiOnly },
    TestString{ "null-character", QUtf8StringView("", 1), QStringView(u"", 1), AsciiOnly },
    TestString{ "ascii-text",
                "This is a standard US-ASCII message",
                "This is a standard US-ASCII message" u"",
                AsciiOnly
    },
    TestString{ "ascii-with-control",
                "\1This\2is\3an\4US-ASCII\020 message interspersed with control chars",
                "\1This\2is\3an\4US-ASCII\020 message interspersed with control chars" u"",
                AsciiOnly
    },

    TestString{ "nbsp", "\u00a0", u"\u00a0", Latin1Only },
    TestString{ "latin1-text",
                "Hyvää päivää, käyhän että tuon kannettavani saunaan?",
                "Hyvää päivää, käyhän että tuon kannettavani saunaan?" u"",
                Latin1Only
    },

#define ROW(name, string)       TestString{ name, u8"" string, u"" string }
    ROW("euro", "€"),
    //ROW("bom", "\ufeff"),     // Can't test this because QString::fromUtf8 consumes it
    ROW("replacement", "\ufffd"),
    ROW("supplementary-plane", "\U00010203"),
    ROW("mahjong", "\U0001f000\U0001f001\U0001f002\U0001f003\U0001f004\U0001f005"
        "\U0001f006\U0001f007\U0001f008\U0001f009\U0001f00a\U0001f00b\U0001f00c"
        "\U0001f00d\U0001f00e\U0001f00f"),
    ROW("emojis", "😂, 😃, 🧘🏻‍♂️, 🌍, 🌦️, 🍞, 🚗, 📞, 🎉, ❤️, 🏁"),  // https://en.wikipedia.org/wiki/Emoji
    ROW("last-valid", "\U0010fffd"),    // U+10FFFF is the strict last, but it's a non-character
    ROW("mixed-bmp-only", "abc\u00a0\u00e1\u00e9\u01fd \u20acdef"),
    ROW("mixed-full", "abc\u00a0\u00e1\u00e9\u01fd \U0010FFFD \u20acdef"),
    ROW("xml", "<doc>\U00010000\U0010FFFD</doc>\r\n")
#undef ROW
};

class tst_QStringConverter : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void threadSafety();

    void constructByName();

    void convertUtf8_data();
    void convertUtf8();
    void convertUtf8CharByChar_data() { convertUtf8_data(); }
    void convertUtf8CharByChar();
    void roundtrip_data();
    void roundtrip();

    void nonFlaggedCodepointFFFF() const;
    void flagF7808080() const;
    void nonFlaggedEFBFBF() const;
    void decode0D() const;

    void utf8Codec_data();
    void utf8Codec();

    void utf8bom_data();
    void utf8bom();

    void utf8stateful_data();
    void utf8stateful();

    void utfHeaders_data();
    void utfHeaders();

    void encodingForName_data();
    void encodingForName();

    void nameForEncoding_data();
    void nameForEncoding();

    void encodingForData_data();
    void encodingForData();

    void encodingForHtml_data();
    void encodingForHtml();
};

void tst_QStringConverter::constructByName()
{
    QStringDecoder decoder("UTF-8");
    QVERIFY(decoder.isValid());
    QVERIFY(!strcmp(decoder.name(), "UTF-8"));
    decoder = QStringDecoder("XXX");
    QVERIFY(!decoder.isValid());
    decoder = QStringDecoder("ISO-8859-1");
    QVERIFY(decoder.isValid());
    QVERIFY(!strcmp(decoder.name(), "ISO-8859-1"));
    decoder = QStringDecoder("UTF-16LE");
    QVERIFY(decoder.isValid());
    QVERIFY(!strcmp(decoder.name(), "UTF-16LE"));

    decoder = QStringDecoder("utf8");
    QVERIFY(decoder.isValid());
    QVERIFY(!strcmp(decoder.name(), "UTF-8"));
    decoder = QStringDecoder("iso8859-1");
    QVERIFY(decoder.isValid());
    QVERIFY(!strcmp(decoder.name(), "ISO-8859-1"));
    decoder = QStringDecoder("utf-16");
    QVERIFY(decoder.isValid());
    QVERIFY(!strcmp(decoder.name(), "UTF-16"));
}

void tst_QStringConverter::convertUtf8_data()
{
    QTest::addColumn<QStringConverter::Encoding>("encoding");
    QTest::addColumn<QUtf8StringView>("utf8");
    QTest::addColumn<QStringView>("utf16");
    auto addRow = [](const TestString &s) {
        QTest::addRow("Utf8:%s", s.description) << QStringDecoder::Utf8 << s.utf8 << s.utf16;
        if (localeIsUtf8())
            QTest::addRow("System:%s", s.description) << QStringDecoder::System << s.utf8 << s.utf16;
    };

    for (const TestString &s : testStrings)
        addRow(s);
}

void tst_QStringConverter::convertUtf8()
{
    QFETCH(QStringConverter::Encoding, encoding);
    QFETCH(QUtf8StringView, utf8);
    QFETCH(QStringView, utf16);

    QByteArray ba = QByteArray::fromRawData(utf8.data(), utf8.size());

    QStringDecoder decoder(encoding);
    QVERIFY(decoder.isValid());
    QString uniString = decoder(ba);
    QCOMPARE(uniString, utf16);
    QCOMPARE(uniString, QString::fromUtf8(ba));
    QCOMPARE(ba, uniString.toUtf8());

    // do it again (using .decode())
    uniString = decoder.decode(ba);
    QCOMPARE(uniString, utf16);
    QCOMPARE(uniString, QString::fromUtf8(ba));
    QCOMPARE(ba, uniString.toUtf8());

    QStringEncoder encoder(encoding);
    QByteArray reencoded = encoder(utf16);
    QCOMPARE(reencoded, utf8);
    QCOMPARE(reencoded, uniString.toUtf8());

    // do it again (using .encode())
    reencoded = encoder.encode(utf16);
    QCOMPARE(reencoded, utf8);
    QCOMPARE(reencoded, uniString.toUtf8());

    if (utf16.isEmpty())
        return;

    // repeat, with a longer string
    constexpr qsizetype MinSize = 128;
    uniString = utf16.toString();
    while (uniString.size() < MinSize && ba.size() < MinSize) {
        uniString += uniString;
        ba += ba;
    }
    QCOMPARE(decoder(ba), uniString);
    QCOMPARE(encoder(uniString), ba);
}

void tst_QStringConverter::convertUtf8CharByChar()
{
    QFETCH(QStringConverter::Encoding, encoding);
    QFETCH(QUtf8StringView, utf8);
    QFETCH(QStringView, utf16);

    QByteArray ba = QByteArray::fromRawData(utf8.data(), utf8.size());

    QStringDecoder decoder(encoding);
    QVERIFY(decoder.isValid());
    QString uniString;
    for (int i = 0; i < ba.size(); ++i)
        uniString += decoder(QByteArrayView(ba).sliced(i, 1));
    QCOMPARE(uniString, utf16);
    QCOMPARE(uniString, QString::fromUtf8(ba));
    uniString.clear();

    // do it again (using .decode())
    for (int i = 0; i < ba.size(); ++i)
        uniString += decoder.decode(QByteArrayView(ba).sliced(i, 1));
    QCOMPARE(uniString, utf16);
    QCOMPARE(uniString, QString::fromUtf8(ba));

    QStringEncoder encoder(encoding);
    QByteArray reencoded;
    for (int i = 0; i < utf16.size(); ++i)
        reencoded += encoder(utf16.sliced(i, 1));
    QCOMPARE(reencoded, ba);
    reencoded.clear();

    // do it again (using .encode())
    for (int i = 0; i < utf16.size(); ++i)
        reencoded += encoder.encode(utf16.sliced(i, 1));
    QCOMPARE(reencoded, ba);
}

void tst_QStringConverter::roundtrip_data()
{
    QTest::addColumn<QStringView>("utf16");
    QTest::addColumn<QStringConverter::Encoding>("code");

    // TODO: include flag variations, too.

    for (const auto code : codes) {
        for (const TestString &s : testStrings) {
            // rules:
            // 1) don't pass the null character to the System codec
            // 2) only pass operate on a string that will properly convert
            if (code.code == QStringConverter::System && s.utf16.contains(QChar(0)))
                continue;
            if (code.limitation < s.limitation)
                continue;
            QTest::addRow("%s:%s", code.name, s.description) << s.utf16 << code.code;
        }

        if (code.limitation == FullUnicode) {
            const char32_t zeroVal = 0x11136; // Unicode's representation of Chakma zero
            for (int i = 0; i < 10; ++i) {
                QChar data[] = {
                    QChar::highSurrogate(zeroVal + i), QChar::lowSurrogate(zeroVal + i),
                };
                QTest::addRow("%s:Chakma-digit-%d", code.name, i) << QStringView(data) << code.code;
            }
        }
    }
}

void tst_QStringConverter::roundtrip()
{
    QFETCH(QStringView, utf16);
    QFETCH(QStringConverter::Encoding, code);
    QStringEncoder out(code);
    QByteArray encoded = out.encode(utf16);
    QStringDecoder back(code);
    const QString decoded = back.decode(encoded);
    QCOMPARE(decoded, utf16);

    if (utf16.isEmpty())
        return;

    // repeat, with a longer string
    constexpr qsizetype MinSize = 128;
    QString uniString = utf16.toString();
    while (uniString.size() < MinSize && encoded.size() < MinSize) {
        uniString += uniString;
        encoded += encoded;
    }
    QCOMPARE(out.encode(uniString), encoded);
    QCOMPARE(back.decode(encoded), uniString);
}

void tst_QStringConverter::nonFlaggedCodepointFFFF() const
{
    //Check that the code point 0xFFFF (=non-character code 0xEFBFBF) is not flagged
    const QChar ch(0xFFFF);

    QStringEncoder encoder(QStringEncoder::Utf8);
    QVERIFY(encoder.isValid());

    const QByteArray asDecoded = encoder(QStringView(&ch, 1));
    QCOMPARE(asDecoded, QByteArray("\357\277\277"));

    QByteArray ffff("\357\277\277");
    QStringDecoder decoder(QStringEncoder::Utf8, QStringDecoder::Flag::ConvertInvalidToNull);
    QVERIFY(decoder.isValid());
    QVERIFY(decoder(ffff) == QString(1, ch));
}

void tst_QStringConverter::flagF7808080() const
{
    /* This test case stems from test not-wf-sa-170, tests/qxmlstream/XML-Test-Suite/xmlconf/xmltest/not-wf/sa/166.xml,
     * whose description reads:
     *
     * "Four byte UTF-8 encodings can encode UCS-4 characters
     *  which are beyond the range of legal XML characters
     *  (and can't be expressed in Unicode surrogate pairs).
     *  This document holds such a character."
     *
     *  In binary, this is:
     *  11110111100000001000000010000000
     *  *       *       *       *
     *  11110www10xxxxxx10yyyyyy10zzzzzz
     *
     *  With multibyte logic removed it is the codepoint 0x1C0000.
     */
    QByteArray input;
    input.resize(4);
    input[0] = char(0xF7);
    input[1] = char(0x80);
    input[2] = char(0x80);
    input[3] = char(0x80);

    QStringDecoder decoder(QStringEncoder::Utf8, QStringDecoder::Flag::ConvertInvalidToNull);
    QVERIFY(decoder.isValid());

    QCOMPARE(decoder(input), QString(input.size(), QChar(0)));
}

void tst_QStringConverter::nonFlaggedEFBFBF() const
{
    /* Check that the codec does NOT flag EFBFBF.
     * This is a regression test; see QTBUG-33229
     */
    QByteArray validInput;
    validInput.resize(3);
    validInput[0] = char(0xEF);
    validInput[1] = char(0xBF);
    validInput[2] = char(0xBF);

    {
        QStringDecoder decoder(QStringEncoder::Utf8, QStringDecoder::Flag::ConvertInvalidToNull);
        QVERIFY(decoder.isValid());
        QVERIFY(decoder(validInput) == QString::fromUtf8(QByteArray::fromHex("EFBFBF")));
    }

    // Check that 0xEFBFBF is correctly decoded when preceded by an arbitrary character
    {
        QByteArray start("B");
        start.append(validInput);

        QStringDecoder decoder(QStringEncoder::Utf8, QStringDecoder::Flag::ConvertInvalidToNull);
        QVERIFY(decoder.isValid());
        QVERIFY(decoder(start) == QString::fromUtf8(QByteArray("B").append(QByteArray::fromHex("EFBFBF"))));
    }
}

void tst_QStringConverter::decode0D() const
{
    QByteArray input;
    input.resize(3);
    input[0] = 'A';
    input[1] = '\r';
    input[2] = 'B';

    QCOMPARE(QString::fromUtf8(input.constData()).toUtf8(), input);
}

static QString fromInvalidUtf8Sequence(const QByteArray &ba)
{
    return QString().fill(QChar::ReplacementCharacter, ba.size());
}

// copied from tst_QString::fromUtf8_data()
void tst_QStringConverter::utf8Codec_data()
{
    QTest::addColumn<QByteArray>("utf8");
    QTest::addColumn<QString>("res");
    QTest::addColumn<int>("len");
    QString str;

    QTest::newRow("str0") << QByteArray("abcdefgh") << QString("abcdefgh") << -1;
    QTest::newRow("str0-len") << QByteArray("abcdefgh") << QString("abc") << 3;
    QTest::newRow("str1") << QByteArray("\303\266\303\244\303\274\303\226\303\204\303\234\303\270\303\246\303\245\303\230\303\206\303\205")
                          << QString::fromLatin1("\366\344\374\326\304\334\370\346\345\330\306\305") << -1;
    QTest::newRow("str1-len") << QByteArray("\303\266\303\244\303\274\303\226\303\204\303\234\303\270\303\246\303\245\303\230\303\206\303\205")
                              << QString::fromLatin1("\366\344\374\326\304") << 10;

    str += QChar(0x05e9);
    str += QChar(0x05d3);
    str += QChar(0x05d2);
    QTest::newRow("str2") << QByteArray("\327\251\327\223\327\222") << str << -1;

    str = QChar(0x05e9);
    QTest::newRow("str2-len") << QByteArray("\327\251\327\223\327\222") << str << 2;

    str = QChar(0x20ac);
    str += " some text";
    QTest::newRow("str3") << QByteArray("\342\202\254 some text") << str << -1;

    str = QChar(0x20ac);
    str += " some ";
    QTest::newRow("str3-len") << QByteArray("\342\202\254 some text") << str << 9;

    str = "hello";
    str += QChar::ReplacementCharacter;
    str += QChar(0x68);
    str += QChar::ReplacementCharacter;
    str += QChar::ReplacementCharacter;
    str += QChar::ReplacementCharacter;
    str += QChar::ReplacementCharacter;
    str += QChar(0x61);
    str += QChar::ReplacementCharacter;
    QTest::newRow("invalid utf8") << QByteArray("hello\344h\344\344\366\344a\304") << str << -1;
    QTest::newRow("invalid utf8-len") << QByteArray("hello\344h\344\344\366\344a\304") << QString("hello") << 5;

    str = "Prohl";
    str += QChar::ReplacementCharacter;
    str += QChar::ReplacementCharacter;
    str += QLatin1Char('e');
    str += QChar::ReplacementCharacter;
    str += " plugin";
    str += QChar::ReplacementCharacter;
    str += " Netscape";

    QTest::newRow("task28417") << QByteArray("Prohl\355\276e\350 plugin\371 Netscape") << str << -1;
    QTest::newRow("task28417-len") << QByteArray("Prohl\355\276e\350 plugin\371 Netscape") << QString("") << 0;

    QTest::newRow("null-1") << QByteArray() << QString() << -1;
    QTest::newRow("null0") << QByteArray() << QString() << 0;
    // QTest::newRow("null5") << QByteArray() << QString() << 5;
    QTest::newRow("empty-1") << QByteArray("\0abcd", 5) << QString() << -1;
    QTest::newRow("empty0") << QByteArray() << QString() << 0;
    QTest::newRow("empty5") << QByteArray("\0abcd", 5) << QString::fromLatin1("\0abcd", 5) << 5;
    QTest::newRow("other-1") << QByteArray("ab\0cd", 5) << QString::fromLatin1("ab") << -1;
    QTest::newRow("other5") << QByteArray("ab\0cd", 5) << QString::fromLatin1("ab\0cd", 5) << 5;

    str = "Old Italic: ";
    str += QChar(0xd800);
    str += QChar(0xdf00);
    str += QChar(0xd800);
    str += QChar(0xdf01);
    str += QChar(0xd800);
    str += QChar(0xdf02);
    str += QChar(0xd800);
    str += QChar(0xdf03);
    str += QChar(0xd800);
    str += QChar(0xdf04);
    QTest::newRow("surrogate") << QByteArray("Old Italic: \360\220\214\200\360\220\214\201\360\220\214\202\360\220\214\203\360\220\214\204") << str << -1;

    QTest::newRow("surrogate-len") << QByteArray("Old Italic: \360\220\214\200\360\220\214\201\360\220\214\202\360\220\214\203\360\220\214\204") << str.left(16) << 20;

    // from http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html

    // 2.1.1 U+00000000
    QByteArray utf8;
    utf8 += char(0x00);
    str = QChar(QChar::Null);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.1.1") << utf8 << str << 1;

    // 2.1.2 U+00000080
    utf8.clear();
    utf8 += char(0xc2);
    utf8 += char(0x80);
    str = QChar(0x80);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.1.2") << utf8 << str << -1;

    // 2.1.3 U+00000800
    utf8.clear();
    utf8 += char(0xe0);
    utf8 += char(0xa0);
    utf8 += char(0x80);
    str = QChar(0x800);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.1.3") << utf8 << str << -1;

    // 2.1.4 U+00010000
    utf8.clear();
    utf8 += char(0xf0);
    utf8 += char(0x90);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str.clear();
    str += QChar(0xd800);
    str += QChar(0xdc00);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.1.4") << utf8 << str << -1;

    // 2.1.5 U+00200000 (not a valid Unicode character)
    utf8.clear();
    utf8 += char(0xf8);
    utf8 += char(0x88);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.1.5") << utf8 << str << -1;

    // 2.1.6 U+04000000 (not a valid Unicode character)
    utf8.clear();
    utf8 += char(0xfc);
    utf8 += char(0x84);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.1.6") << utf8 << str << -1;

    // 2.2.1 U+0000007F
    utf8.clear();
    utf8 += char(0x7f);
    str = QChar(0x7f);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.2.1") << utf8 << str << -1;

    // 2.2.2 U+000007FF
    utf8.clear();
    utf8 += char(0xdf);
    utf8 += char(0xbf);
    str = QChar(0x7ff);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.2.2") << utf8 << str << -1;

    // 2.2.3 U+000FFFF - non-character code
    utf8.clear();
    utf8 += char(0xef);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = QString::fromUtf8(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.2.3") << utf8 << str << -1;

    // 2.2.4 U+001FFFFF
    utf8.clear();
    utf8 += char(0xf7);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.2.4") << utf8 << str << -1;

    // 2.2.5 U+03FFFFFF (not a valid Unicode character)
    utf8.clear();
    utf8 += char(0xfb);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.2.5") << utf8 << str << -1;

    // 2.2.6 U+7FFFFFFF
    utf8.clear();
    utf8 += char(0xfd);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.2.6") << utf8 << str << -1;

    // 2.3.1 U+0000D7FF
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0x9f);
    utf8 += char(0xbf);
    str = QChar(0xd7ff);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.3.1") << utf8 << str << -1;

    // 2.3.2 U+0000E000
    utf8.clear();
    utf8 += char(0xee);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = QChar(0xe000);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.3.2") << utf8 << str << -1;

    // 2.3.3 U+0000FFFD
    utf8.clear();
    utf8 += char(0xef);
    utf8 += char(0xbf);
    utf8 += char(0xbd);
    str = QChar(QChar::ReplacementCharacter);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.3.3") << utf8 << str << -1;

    // 2.3.4 U+0010FFFD
    utf8.clear();
    utf8 += char(0xf4);
    utf8 += char(0x8f);
    utf8 += char(0xbf);
    utf8 += char(0xbd);
    str.clear();
    str += QChar(0xdbff);
    str += QChar(0xdffd);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.3.4") << utf8 << str << -1;

    // 2.3.5 U+00110000
    utf8.clear();
    utf8 += char(0xf4);
    utf8 += char(0x90);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 2.3.5") << utf8 << str << -1;

    // 3.1.1
    utf8.clear();
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.1.1") << utf8 << str << -1;

    // 3.1.2
    utf8.clear();
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.1.2") << utf8 << str << -1;

    // 3.1.3
    utf8.clear();
    utf8 += char(0x80);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.1.3") << utf8 << str << -1;

    // 3.1.4
    utf8.clear();
    utf8 += char(0x80);
    utf8 += char(0xbf);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.1.4") << utf8 << str << -1;

    // 3.1.5
    utf8.clear();
    utf8 += char(0x80);
    utf8 += char(0xbf);
    utf8 += char(0x80);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.1.5") << utf8 << str << -1;

    // 3.1.6
    utf8.clear();
    utf8 += char(0x80);
    utf8 += char(0xbf);
    utf8 += char(0x80);
    utf8 += char(0xbf);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.1.6") << utf8 << str << -1;

    // 3.1.7
    utf8.clear();
    utf8 += char(0x80);
    utf8 += char(0xbf);
    utf8 += char(0x80);
    utf8 += char(0xbf);
    utf8 += char(0x80);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.1.7") << utf8 << str << -1;

    // 3.1.8
    utf8.clear();
    utf8 += char(0x80);
    utf8 += char(0xbf);
    utf8 += char(0x80);
    utf8 += char(0xbf);
    utf8 += char(0x80);
    utf8 += char(0xbf);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.1.8") << utf8 << str << -1;

    // 3.1.9
    utf8.clear();
    for (uint i = 0x80; i<= 0xbf; ++i)
        utf8 += i;
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.1.9") << utf8 << str << -1;

    // 3.2.1
    utf8.clear();
    str.clear();
    for (uint i = 0xc8; i <= 0xdf; ++i) {
        utf8 += i;
        utf8 += char(0x20);

        str += QChar::ReplacementCharacter;
        str += QChar(0x0020);
    }
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.2.1") << utf8 << str << -1;

    // 3.2.2
    utf8.clear();
    str.clear();
    for (uint i = 0xe0; i <= 0xef; ++i) {
        utf8 += i;
        utf8 += char(0x20);

        str += QChar::ReplacementCharacter;
        str += QChar(0x0020);
    }
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.2.2") << utf8 << str << -1;

    // 3.2.3
    utf8.clear();
    str.clear();
    for (uint i = 0xf0; i <= 0xf7; ++i) {
        utf8 += i;
        utf8 += 0x20;

        str += QChar::ReplacementCharacter;
        str += QChar(0x0020);
    }
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.2.3") << utf8 << str << -1;

    // 3.2.4
    utf8.clear();
    str.clear();
    for (uint i = 0xf8; i <= 0xfb; ++i) {
        utf8 += i;
        utf8 += 0x20;

        str += QChar::ReplacementCharacter;
        str += QChar(0x0020);
    }
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.2.4") << utf8 << str << -1;

    // 3.2.5
    utf8.clear();
    str.clear();
    for (uint i = 0xfc; i <= 0xfd; ++i) {
        utf8 += i;
        utf8 += 0x20;

        str += QChar::ReplacementCharacter;
        str += QChar(0x0020);
    }
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.2.5") << utf8 << str << -1;

    // 3.3.1
    utf8.clear();
    utf8 += char(0xc0);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.1") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.1-1") << utf8 << str << -1;

    // 3.3.2
    utf8.clear();
    utf8 += char(0xe0);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.2-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xe0);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.2-2") << utf8 << str << -1;
    utf8 += 0x30;
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.2-3") << utf8 << str << -1;

    // 3.3.3
    utf8.clear();
    utf8 += char(0xf0);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.3") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.3-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xf0);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.3-2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.3-3") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xf0);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.3-4") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.3-5") << utf8 << str << -1;

    // 3.3.4
    utf8.clear();
    utf8 += char(0xf8);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.4") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.4-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xf8);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.4-2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.4-3") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xf8);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.4-4") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.4-5") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xf8);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.4-6") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.4-7") << utf8 << str << -1;

    // 3.3.5
    utf8.clear();
    utf8 += char(0xfc);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfc);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-3") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfc);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-4") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-5") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfc);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-6") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-7") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfc);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-8") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-9") << utf8 << str << -1;

    // 3.3.6
    utf8.clear();
    utf8 += char(0xdf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.6") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.6-1") << utf8 << str << -1;

    // 3.3.7
    utf8.clear();
    utf8 += char(0xef);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.7") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.7-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xef);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.7-2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.7-3") << utf8 << str << -1;

    // 3.3.8
    utf8.clear();
    utf8 += char(0xf7);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.8") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.8-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xf7);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.8-2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.8-3") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xf7);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.8-4") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.8-5") << utf8 << str << -1;

    // 3.3.9
    utf8.clear();
    utf8 += char(0xfb);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.9") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.9-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfb);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.9-2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.9-3") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfb);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.9-4") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.9-5") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfb);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.9-6") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.9-7") << utf8 << str << -1;

    // 3.3.10
    utf8.clear();
    utf8 += char(0xfd);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfd);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-3") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfd);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-4") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-5") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfd);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-6") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-7") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfd);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-8") << utf8 << str << -1;
    utf8 += char(0x30);
    str += QChar(0x30);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-9") << utf8 << str << -1;

    // 3.4
    utf8.clear();
    utf8 += char(0xc0);
    utf8 += char(0xe0);
    utf8 += char(0x80);
    utf8 += char(0xf0);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0xf8);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0xfc);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0xdf);
    utf8 += char(0xef);
    utf8 += char(0xbf);
    utf8 += char(0xf7);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xfb);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xfd);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.4") << utf8 << str << -1;

    // 3.5.1
    utf8.clear();
    utf8 += char(0xfe);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.5.1") << utf8 << str << -1;

    // 3.5.2
    utf8.clear();
    utf8 += char(0xff);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.5.2") << utf8 << str << -1;

    // 3.5.2
    utf8.clear();
    utf8 += char(0xfe);
    utf8 += char(0xfe);
    utf8 += char(0xff);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.5.2-1") << utf8 << str << -1;

    // 4.1.1
    utf8.clear();
    utf8 += char(0xc0);
    utf8 += char(0xaf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 4.1.1") << utf8 << str << -1;

    // 4.1.2
    utf8.clear();
    utf8 += char(0xe0);
    utf8 += char(0x80);
    utf8 += char(0xaf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 4.1.2") << utf8 << str << -1;

    // 4.1.3
    utf8.clear();
    utf8 += char(0xf0);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0xaf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 4.1.3") << utf8 << str << -1;

    // 4.1.4
    utf8.clear();
    utf8 += char(0xf8);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0xaf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 4.1.4") << utf8 << str << -1;

    // 4.1.5
    utf8.clear();
    utf8 += char(0xfc);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0xaf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 4.1.5") << utf8 << str << -1;

    // 4.2.1
    utf8.clear();
    utf8 += char(0xc1);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 4.2.1") << utf8 << str << -1;

    // 4.2.2
    utf8.clear();
    utf8 += char(0xe0);
    utf8 += char(0x9f);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 4.2.2") << utf8 << str << -1;

    // 4.2.3
    utf8.clear();
    utf8 += char(0xf0);
    utf8 += char(0x8f);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 4.2.3") << utf8 << str << -1;

    // 4.2.4
    utf8.clear();
    utf8 += char(0xf8);
    utf8 += char(0x87);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 4.2.4") << utf8 << str << -1;

    // 4.2.5
    utf8.clear();
    utf8 += char(0xfc);
    utf8 += char(0x83);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 4.2.5") << utf8 << str << -1;

    // 4.3.1
    utf8.clear();
    utf8 += char(0xc0);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 4.3.1") << utf8 << str << -1;

    // 4.3.2
    utf8.clear();
    utf8 += char(0xe0);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 4.3.2") << utf8 << str << -1;

    // 4.3.3
    utf8.clear();
    utf8 += char(0xf0);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 4.3.3") << utf8 << str << -1;

    // 4.3.4
    utf8.clear();
    utf8 += char(0xf8);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 4.3.4") << utf8 << str << -1;

    // 4.3.5
    utf8.clear();
    utf8 += char(0xfc);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 4.3.5") << utf8 << str << -1;

    // 5.1.1
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0xa0);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.1.1") << utf8 << str << -1;

    // 5.1.2
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0xad);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.1.2") << utf8 << str << -1;

    // 5.1.3
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0xae);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.1.3") << utf8 << str << -1;

    // 5.1.4
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0xaf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.1.4") << utf8 << str << -1;

    // 5.1.5
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0xb0);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.1.5") << utf8 << str << -1;

    // 5.1.6
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0xbe);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.1.6") << utf8 << str << -1;

    // 5.1.7
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.1.7") << utf8 << str << -1;

    // 5.2.1
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0xa0);
    utf8 += char(0x80);
    utf8 += char(0xed);
    utf8 += char(0xb0);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.2.1") << utf8 << str << -1;

    // 5.2.2
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0xa0);
    utf8 += char(0x80);
    utf8 += char(0xed);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.2.2") << utf8 << str << -1;

    // 5.2.3
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0xad);
    utf8 += char(0xbf);
    utf8 += char(0xed);
    utf8 += char(0xb0);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.2.3") << utf8 << str << -1;

    // 5.2.4
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0xad);
    utf8 += char(0xbf);
    utf8 += char(0xed);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.2.4") << utf8 << str << -1;

    // 5.2.5
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0xae);
    utf8 += char(0x80);
    utf8 += char(0xed);
    utf8 += char(0xb0);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.2.5") << utf8 << str << -1;

    // 5.2.6
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0xae);
    utf8 += char(0x80);
    utf8 += char(0xed);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.2.6") << utf8 << str << -1;

    // 5.2.7
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0xaf);
    utf8 += char(0xbf);
    utf8 += char(0xed);
    utf8 += char(0xb0);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.2.7") << utf8 << str << -1;

    // 5.2.8
    utf8.clear();
    utf8 += char(0xed);
    utf8 += char(0xaf);
    utf8 += char(0xbf);
    utf8 += char(0xed);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.2.8") << utf8 << str << -1;

    // 5.3.1 - non-character code
    utf8.clear();
    utf8 += char(0xef);
    utf8 += char(0xbf);
    utf8 += char(0xbe);
    //str = QChar(QChar::ReplacementCharacter);
    str = QChar(0xfffe);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.3.1") << utf8 << str << -1;

    // 5.3.2 - non-character code
    utf8.clear();
    utf8 += char(0xef);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    //str = QChar(QChar::ReplacementCharacter);
    str = QChar(0xffff);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 5.3.2") << utf8 << str << -1;
}

void tst_QStringConverter::utf8Codec()
{
    QFETCH(QByteArray, utf8);
    QFETCH(QString, res);
    QFETCH(int, len);

    QStringDecoder decoder(QStringDecoder::Utf8, QStringDecoder::Flag::Stateless);
    QString str = decoder(QByteArrayView(utf8).first(len < 0 ? qstrlen(utf8.constData()) : len));
    QCOMPARE(str, res);

    str = QString::fromUtf8(utf8.isNull() ? 0 : utf8.constData(), len);
    QCOMPARE(str, res);
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
void tst_QStringConverter::utf8bom_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("result");

    QTest::newRow("nobom")
        << QByteArray("\302\240", 2)
        << QString::fromLatin1("\240");

    {
        static const char16_t data[] = { 0x201d };
        QTest::newRow("nobom 2")
            << QByteArray("\342\200\235", 3)
            << QString::fromUtf16(data, std::size(data));
    }

    {
        static const char16_t data[] = { 0xf000 };
        QTest::newRow("bom1")
            << QByteArray("\357\200\200", 3)
            << QString::fromUtf16(data, std::size(data));
    }

    {
        static const char16_t data[] = { 0xfec0 };
        QTest::newRow("bom2")
            << QByteArray("\357\273\200", 3)
            << QString::fromUtf16(data, std::size(data));
    }

    {
        QTest::newRow("normal-bom")
            << QByteArray("\357\273\277a", 4)
            << QString("a");
    }

    { // test the non-SIMD code-path
        static const char16_t data[] = { 0x61, 0xfeff, 0x62 };
        QTest::newRow("middle-bom (non SIMD)")
            << QByteArray("a\357\273\277b")
            << QString::fromUtf16(data, std::size(data));
    }

    { // test the SIMD code-path
        static const char16_t data[] = { 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
                                         0x68, 0x69, 0x6a, 0x6b, 0x6c, 0xfeff, 0x6d };
        QTest::newRow("middle-bom (SIMD)")
            << QByteArray("abcdefghijkl\357\273\277m")
            << QString::fromUtf16(data, std::size(data));
    }
}
QT_WARNING_POP

void tst_QStringConverter::utf8bom()
{
    QFETCH(QByteArray, data);
    QFETCH(QString, result);

    QStringDecoder decoder(QStringDecoder::Utf8);

    QCOMPARE(decoder(data), result);
}

void tst_QStringConverter::utf8stateful_data()
{
    QTest::addColumn<QByteArray>("buffer1");
    QTest::addColumn<QByteArray>("buffer2");
    QTest::addColumn<QString>("result");    // null QString indicates decoder error

    // valid buffer continuations
    QTest::newRow("1of2+valid") << QByteArray("\xc2") << QByteArray("\xa0") << "\xc2\xa0";
    QTest::newRow("1of3+valid") << QByteArray("\xe0") << QByteArray("\xa0\x80") << "\xe0\xa0\x80";
    QTest::newRow("2of3+valid") << QByteArray("\xe0\xa0") << QByteArray("\x80") << "\xe0\xa0\x80";
    QTest::newRow("1of4+valid") << QByteArray("\360") << QByteArray("\220\210\203") << "\360\220\210\203";
    QTest::newRow("2of4+valid") << QByteArray("\360\220") << QByteArray("\210\203") << "\360\220\210\203";
    QTest::newRow("3of4+valid") << QByteArray("\360\220\210") << QByteArray("\203") << "\360\220\210\203";
    QTest::newRow("1ofBom+valid") << QByteArray("\xef") << QByteArray("\xbb\xbf") << "";
    QTest::newRow("2ofBom+valid") << QByteArray("\xef\xbb") << QByteArray("\xbf") << "";

    // invalid continuation
    QTest::newRow("1of2+invalid") << QByteArray("\xc2") << QByteArray("a") << QString();
    QTest::newRow("1of3+invalid") << QByteArray("\xe0") << QByteArray("a") << QString();
    QTest::newRow("2of3+invalid") << QByteArray("\xe0\xa0") << QByteArray("a") << QString();
    QTest::newRow("1of4+invalid") << QByteArray("\360") << QByteArray("a") << QString();
    QTest::newRow("2of4+invalid") << QByteArray("\360\220") << QByteArray("a") << QString();
    QTest::newRow("3of4+invalid") << QByteArray("\360\220\210") << QByteArray("a") << QString();

    // overlong sequence:
    QTest::newRow("overlong-1of2") << QByteArray("\xc1") << QByteArray("\x81") << QString();
    QTest::newRow("overlong-1of3") << QByteArray("\xe0") << QByteArray("\x81\x81") << QString();
    QTest::newRow("overlong-2of3") << QByteArray("\xe0\x81") << QByteArray("\x81") << QString();
    QTest::newRow("overlong-1of4") << QByteArray("\xf0") << QByteArray("\x80\x81\x81") << QString();
    QTest::newRow("overlong-2of4") << QByteArray("\xf0\x80") << QByteArray("\x81\x81") << QString();
    QTest::newRow("overlong-3of4") << QByteArray("\xf0\x80\x81") << QByteArray("\x81") << QString();

    // out of range:
    // leading byte 0xF4 can produce codepoints above U+10FFFF, which aren't valid
    QTest::newRow("outofrange1-1of4") << QByteArray("\xf4") << QByteArray("\x90\x80\x80") << QString();
    QTest::newRow("outofrange1-2of4") << QByteArray("\xf4\x90") << QByteArray("\x80\x80") << QString();
    QTest::newRow("outofrange1-3of4") << QByteArray("\xf4\x90\x80") << QByteArray("\x80") << QString();
    QTest::newRow("outofrange2-1of4") << QByteArray("\xf5") << QByteArray("\x90\x80\x80") << QString();
    QTest::newRow("outofrange2-2of4") << QByteArray("\xf5\x90") << QByteArray("\x80\x80") << QString();
    QTest::newRow("outofrange2-3of4") << QByteArray("\xf5\x90\x80") << QByteArray("\x80") << QString();
    QTest::newRow("outofrange-1of5") << QByteArray("\xf8") << QByteArray("\x88\x80\x80\x80") << QString();
    QTest::newRow("outofrange-2of5") << QByteArray("\xf8\x88") << QByteArray("\x80\x80\x80") << QString();
    QTest::newRow("outofrange-3of5") << QByteArray("\xf8\x88\x80") << QByteArray("\x80\x80") << QString();
    QTest::newRow("outofrange-4of5") << QByteArray("\xf8\x88\x80\x80") << QByteArray("\x80") << QString();
    QTest::newRow("outofrange-1of6") << QByteArray("\xfc") << QByteArray("\x84\x80\x80\x80\x80") << QString();
    QTest::newRow("outofrange-2of6") << QByteArray("\xfc\x84") << QByteArray("\x80\x80\x80\x80") << QString();
    QTest::newRow("outofrange-3of6") << QByteArray("\xfc\x84\x80") << QByteArray("\x80\x80\x80") << QString();
    QTest::newRow("outofrange-4of6") << QByteArray("\xfc\x84\x80\x80") << QByteArray("\x80\x80") << QString();
    QTest::newRow("outofrange-5of6") << QByteArray("\xfc\x84\x80\x80\x80") << QByteArray("\x80") << QString();
}

void tst_QStringConverter::utf8stateful()
{
    QFETCH(QByteArray, buffer1);
    QFETCH(QByteArray, buffer2);
    QFETCH(QString, result);

    {
        QStringDecoder decoder(QStringDecoder::Utf8);
        QVERIFY(decoder.isValid());

        QString decoded = decoder(buffer1);
        if (result.isNull()) {
            if (!decoder.hasError()) {
                // incomplete data
                decoded += decoder(buffer2);
                QVERIFY(decoder.hasError());
            }
        } else {
            QVERIFY(!decoder.hasError());
            decoded += decoder(buffer2);
            QVERIFY(!decoder.hasError());
            QCOMPARE(decoded, result);
        }
    }

    if (!buffer2.isEmpty()) {
        QStringDecoder decoder(QStringDecoder::Utf8);
        QVERIFY(decoder.isValid());

        QString decoded;
        for (char c : buffer1)
            decoded += decoder(QByteArrayView(&c, 1));
        for (char c : buffer2)
            decoded += decoder(QByteArrayView(&c, 1));
        if (result.isNull()) {
            QVERIFY(decoder.hasError());
        } else {
            QVERIFY(!decoder.hasError());
            QCOMPARE(decoded, result);
        }
    }
}

void tst_QStringConverter::utfHeaders_data()
{
    QTest::addColumn<QStringConverter::Encoding>("encoding");
    QTest::addColumn<QStringConverter::Flag>("flags");
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<QString>("unicode");

    QTest::newRow("utf8 bom")
        << QStringConverter::Utf8
        << QStringConverter::Flag::WriteBom
        << QByteArray("\xef\xbb\xbfhello")
        << QString::fromLatin1("hello");
    QTest::newRow("utf8 nobom")
        << QStringConverter::Utf8
        << QStringConverter::Flag::WriteBom
        << QByteArray("hello")
        << QString::fromLatin1("hello");
    QTest::newRow("utf8 bom ignore header")
        << QStringConverter::Utf8
        << QStringConverter::Flag::ConvertInitialBom
        << QByteArray("\xef\xbb\xbfhello")
        << (QString(QChar(0xfeff)) + QString::fromLatin1("hello"));
    QTest::newRow("utf8 nobom ignore header")
        << QStringConverter::Utf8
        << QStringConverter::Flag::ConvertInitialBom
        << QByteArray("hello")
        << QString::fromLatin1("hello");

    QTest::newRow("utf16 bom be")
        << QStringConverter::Utf16
        << QStringConverter::Flag::WriteBom
        << QByteArray("\xfe\xff\0h\0e\0l", 8)
        << QString::fromLatin1("hel");
    QTest::newRow("utf16 bom le")
        << QStringConverter::Utf16
        << QStringConverter::Flag::WriteBom
        << QByteArray("\xff\xfeh\0e\0l\0", 8)
        << QString::fromLatin1("hel");
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        QTest::newRow("utf16 nobom")
            << QStringConverter::Utf16
            << QStringConverter::Flag::WriteBom
            << QByteArray("\0h\0e\0l", 6)
            << QString::fromLatin1("hel");
        QTest::newRow("utf16 bom be ignore header")
            << QStringConverter::Utf16
            << QStringConverter::Flag::ConvertInitialBom
            << QByteArray("\xfe\xff\0h\0e\0l", 8)
            << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"));
    } else {
        QTest::newRow("utf16 nobom")
            << QStringConverter::Utf16
            << QStringConverter::Flag::WriteBom
            << QByteArray("h\0e\0l\0", 6)
            << QString::fromLatin1("hel");
        QTest::newRow("utf16 bom le ignore header")
            << QStringConverter::Utf16
            << QStringConverter::Flag::ConvertInitialBom
            << QByteArray("\xff\xfeh\0e\0l\0", 8)
            << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"));
    }

    QTest::newRow("utf16-be bom be")
        << QStringConverter::Utf16BE
        << QStringConverter::Flag::WriteBom
        << QByteArray("\xfe\xff\0h\0e\0l", 8)
        << QString::fromLatin1("hel");
    QTest::newRow("utf16-be nobom")
        << QStringConverter::Utf16BE
        << QStringConverter::Flag::WriteBom
        << QByteArray("\0h\0e\0l", 6)
        << QString::fromLatin1("hel");
    QTest::newRow("utf16-be bom be ignore header")
        << QStringConverter::Utf16BE
        << QStringConverter::Flag::ConvertInitialBom
        << QByteArray("\xfe\xff\0h\0e\0l", 8)
        << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"));

    QTest::newRow("utf16-le bom le")
        << QStringConverter::Utf16LE
        << QStringConverter::Flag::WriteBom
        << QByteArray("\xff\xfeh\0e\0l\0", 8)
        << QString::fromLatin1("hel");
    QTest::newRow("utf16-le nobom")
        << QStringConverter::Utf16LE
        << QStringConverter::Flag::WriteBom
        << QByteArray("h\0e\0l\0", 6)
        << QString::fromLatin1("hel");
    QTest::newRow("utf16-le bom le ignore header")
        << QStringConverter::Utf16LE
        << QStringConverter::Flag::ConvertInitialBom
        << QByteArray("\xff\xfeh\0e\0l\0", 8)
        << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"));

    QTest::newRow("utf32 bom be")
        << QStringConverter::Utf32
        << QStringConverter::Flag::WriteBom
        << QByteArray("\0\0\xfe\xff\0\0\0h\0\0\0e\0\0\0l", 16)
        << QString::fromLatin1("hel");
    QTest::newRow("utf32 bom le")
        << QStringConverter::Utf32
        << QStringConverter::Flag::WriteBom
        << QByteArray("\xff\xfe\0\0h\0\0\0e\0\0\0l\0\0\0", 16)
        << QString::fromLatin1("hel");
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        QTest::newRow("utf32 nobom")
            << QStringConverter::Utf32
            << QStringConverter::Flag::WriteBom
            << QByteArray("\0\0\0h\0\0\0e\0\0\0l", 12)
            << QString::fromLatin1("hel");
        QTest::newRow("utf32 bom be ignore header")
            << QStringConverter::Utf32
            << QStringConverter::Flag::ConvertInitialBom
            << QByteArray("\0\0\xfe\xff\0\0\0h\0\0\0e\0\0\0l", 16)
            << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"));
    } else {
        QTest::newRow("utf32 nobom")
            << QStringConverter::Utf32
            << QStringConverter::Flag::WriteBom
            << QByteArray("h\0\0\0e\0\0\0l\0\0\0", 12)
            << QString::fromLatin1("hel");
        QTest::newRow("utf32 bom le ignore header")
            << QStringConverter::Utf32
            << QStringConverter::Flag::ConvertInitialBom
            << QByteArray("\xff\xfe\0\0h\0\0\0e\0\0\0l\0\0\0", 16)
            << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"));
    }

    QTest::newRow("utf32-be bom be")
        << QStringConverter::Utf32BE
        << QStringConverter::Flag::WriteBom
        << QByteArray("\0\0\xfe\xff\0\0\0h\0\0\0e\0\0\0l", 16)
        << QString::fromLatin1("hel");
    QTest::newRow("utf32-be nobom")
        << QStringConverter::Utf32BE
        << QStringConverter::Flag::WriteBom
        << QByteArray("\0\0\0h\0\0\0e\0\0\0l", 12)
        << QString::fromLatin1("hel");
    QTest::newRow("utf32-be bom be ignore header")
        << QStringConverter::Utf32BE
        << QStringConverter::Flag::ConvertInitialBom
        << QByteArray("\0\0\xfe\xff\0\0\0h\0\0\0e\0\0\0l", 16)
        << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"));

    QTest::newRow("utf32-le bom le")
        << QStringConverter::Utf32LE
        << QStringConverter::Flag::WriteBom
        << QByteArray("\xff\xfe\0\0h\0\0\0e\0\0\0l\0\0\0", 16)
        << QString::fromLatin1("hel");
    QTest::newRow("utf32-le nobom")
        << QStringConverter::Utf32LE
        << QStringConverter::Flag::WriteBom
        << QByteArray("h\0\0\0e\0\0\0l\0\0\0", 12)
        << QString::fromLatin1("hel");
    QTest::newRow("utf32-le bom le ignore header")
        << QStringConverter::Utf32LE
        << QStringConverter::Flag::ConvertInitialBom
        << QByteArray("\xff\xfe\0\0h\0\0\0e\0\0\0l\0\0\0", 16)
        << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"));
}

void tst_QStringConverter::utfHeaders()
{
    QFETCH(QStringConverter::Encoding, encoding);
    QFETCH(QStringConverter::Flag, flags);
    QFETCH(QByteArray, encoded);
    QFETCH(QString, unicode);

    QLatin1String ignoreReverseTestOn = (QSysInfo::ByteOrder == QSysInfo::BigEndian) ? QLatin1String(" le") : QLatin1String(" be");
    QString rowName(QTest::currentDataTag());

    {
        QStringDecoder decode(encoding, flags);
        QVERIFY(decode.isValid());

        QString result = decode(encoded);
        QCOMPARE(result.length(), unicode.length());
        QCOMPARE(result, unicode);
    }

    {
        QStringDecoder decode(encoding, flags);
        QVERIFY(decode.isValid());

        QString result;
        for (char c : encoded)
            result += decode(QByteArrayView(&c, 1));
        QCOMPARE(result.length(), unicode.length());
        QCOMPARE(result, unicode);
    }

    if (!rowName.endsWith("nobom") && !rowName.contains(ignoreReverseTestOn)) {
        {
            QStringEncoder encode(encoding, flags);
            QVERIFY(encode.isValid());
            QByteArray reencoded = encode(unicode);
            QCOMPARE(reencoded, encoded);
        }

        {
            QStringEncoder encode(encoding, flags);
            QVERIFY(encode.isValid());
            QByteArray reencoded;
            for (QChar c : unicode)
                reencoded += encode(QStringView(&c, 1));
            QCOMPARE(reencoded, encoded);
        }
    }
}

void tst_QStringConverter::encodingForName_data()
{
    QTest::addColumn<QByteArray>("name");
    QTest::addColumn<std::optional<QStringConverter::Encoding>>("encoding");

    QTest::newRow("UTF-8") << QByteArray("UTF-8") << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);
    QTest::newRow("utf8") << QByteArray("utf8") << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);
    QTest::newRow("Utf-8") << QByteArray("Utf-8") << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);
    QTest::newRow("UTF-16") << QByteArray("UTF-16") << std::optional<QStringConverter::Encoding>(QStringConverter::Utf16);
    QTest::newRow("UTF-16le") << QByteArray("UTF-16le") << std::optional<QStringConverter::Encoding>(QStringConverter::Utf16LE);
    QTest::newRow("ISO-8859-1") << QByteArray("ISO-8859-1") << std::optional<QStringConverter::Encoding>(QStringConverter::Latin1);
    QTest::newRow("ISO8859-1") << QByteArray("ISO8859-1") << std::optional<QStringConverter::Encoding>(QStringConverter::Latin1);
    QTest::newRow("iso8859-1") << QByteArray("iso8859-1") << std::optional<QStringConverter::Encoding>(QStringConverter::Latin1);
    QTest::newRow("latin1") << QByteArray("latin1") << std::optional<QStringConverter::Encoding>(QStringConverter::Latin1);
    QTest::newRow("latin2") << QByteArray("latin2") << std::optional<QStringConverter::Encoding>();
    QTest::newRow("latin15") << QByteArray("latin15") << std::optional<QStringConverter::Encoding>();
}

void tst_QStringConverter::encodingForName()
{
    QFETCH(QByteArray, name);
    QFETCH(std::optional<QStringConverter::Encoding>, encoding);

    auto e = QStringConverter::encodingForName(name);
    QCOMPARE(e, encoding);
}

void tst_QStringConverter::nameForEncoding_data()
{
    QTest::addColumn<QByteArray>("name");
    QTest::addColumn<QStringConverter::Encoding>("encoding");

    QTest::newRow("UTF-8") << QByteArray("UTF-8") << QStringConverter::Utf8;
    QTest::newRow("UTF-16") << QByteArray("UTF-16") << QStringConverter::Utf16;
    QTest::newRow("UTF-16LE") << QByteArray("UTF-16LE") << QStringConverter::Utf16LE;
    QTest::newRow("ISO-8859-1") << QByteArray("ISO-8859-1") << QStringConverter::Latin1;
}

void tst_QStringConverter::nameForEncoding()
{
    QFETCH(QByteArray, name);
    QFETCH(QStringConverter::Encoding, encoding);

    QByteArray n = QStringConverter::nameForEncoding(encoding);
    QCOMPARE(n, name);
}

void tst_QStringConverter::encodingForData_data()
{
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<std::optional<QStringConverter::Encoding>>("encoding");


    QTest::newRow("utf8 bom")
        << QByteArray("\xef\xbb\xbfhello")
        << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);
    QTest::newRow("utf8 nobom")
        << QByteArray("hello")
        << std::optional<QStringConverter::Encoding>();

    QTest::newRow("utf16 bom be")
        << QByteArray("\xfe\xff\0h\0e\0l", 8)
        << std::optional<QStringConverter::Encoding>(QStringConverter::Utf16BE);
    QTest::newRow("utf16 bom le")
        << QByteArray("\xff\xfeh\0e\0l\0", 8)
        << std::optional<QStringConverter::Encoding>(QStringConverter::Utf16LE);
    QTest::newRow("utf16 nobom be")
        << QByteArray("\0<\0e\0l", 6)
        << std::optional<QStringConverter::Encoding>(QStringConverter::Utf16BE);
    QTest::newRow("utf16 nobom le")
        << QByteArray("<\0e\0l\0", 6)
        << std::optional<QStringConverter::Encoding>(QStringConverter::Utf16LE);
    QTest::newRow("utf16 nobom no match")
        << QByteArray("h\0e\0l\0", 6)
        << std::optional<QStringConverter::Encoding>();

    QTest::newRow("utf32 bom be")
        << QByteArray("\0\0\xfe\xff\0\0\0h\0\0\0e\0\0\0l", 16)
        << std::optional<QStringConverter::Encoding>(QStringConverter::Utf32BE);
    QTest::newRow("utf32 bom le")
        << QByteArray("\xff\xfe\0\0h\0\0\0e\0\0\0l\0\0\0", 16)
        << std::optional<QStringConverter::Encoding>(QStringConverter::Utf32LE);
    QTest::newRow("utf32 nobom be")
        << QByteArray("\0\0\0<\0\0\0e\0\0\0l", 12)
        << std::optional<QStringConverter::Encoding>(QStringConverter::Utf32BE);
    QTest::newRow("utf32 nobom")
        << QByteArray("<\0\0\0e\0\0\0l\0\0\0", 12)
        << std::optional<QStringConverter::Encoding>(QStringConverter::Utf32LE);
    QTest::newRow("utf32 nobom no match")
        << QByteArray("\0\0\0h\0\0\0e\0\0\0l", 12)
        << std::optional<QStringConverter::Encoding>();
}

void tst_QStringConverter::encodingForData()
{
    QFETCH(QByteArray, encoded);
    QFETCH(std::optional<QStringConverter::Encoding>, encoding);

    auto e = QStringConverter::encodingForData(encoded, char16_t('<'));
    QCOMPARE(e, encoding);
}


void tst_QStringConverter::encodingForHtml_data()
{
    QTest::addColumn<QByteArray>("html");
    QTest::addColumn<std::optional<QStringConverter::Encoding>>("encoding");

    QByteArray html = "<html><head></head><body>blah</body></html>";
    QTest::newRow("no charset") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-15\" /></head></html>";
    QTest::newRow("latin 15") << html << std::optional<QStringConverter::Encoding>();

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-1\" /></head></html>";
    QTest::newRow("latin 1") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Latin1);

    html = "<!DOCTYPE html><html><head><meta charset=\"ISO_8859-1:1987\"><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><title>Test</title></head>";
    QTest::newRow("latin 1 (#2)") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Latin1);

    html = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><title>Test</title></head>";
    QTest::newRow("UTF-8") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);

    html = "<!DOCTYPE html><html><head><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><meta charset=\"utf-8\"><title>Test</title></head>";
    QTest::newRow("UTF-8 (#2)") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8/></head></html>";
    QTest::newRow("UTF-8, no quotes") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset='UTF-8'/></head></html>";
    QTest::newRow("UTF-8, single quotes") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);

    html = "<!DOCTYPE html><html><head><meta charset=utf-8><title>Test</title></head>";
    QTest::newRow("UTF-8, > terminator") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);

    html = "<!DOCTYPE html><html><head><meta charset= utf-8 ><title>Test</title></head>";
    QTest::newRow("UTF-8, > terminator with spaces") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);

    // Test invalid charsets.
    html = "<!DOCTYPE html><html><head><meta charset= utf/8 ><title>Test</title></head>";
    QTest::newRow("utf/8") << html << std::optional<QStringConverter::Encoding>();

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=invalid-foo\" /></head></html>";
    QTest::newRow("invalid charset, no default") << html << std::optional<QStringConverter::Encoding>();

    html = "<!DOCTYPE html><html><head><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><meta charset=\"";
    html.prepend(QByteArray().fill(' ', 512 - html.size()));
    QTest::newRow("invalid charset (large header)") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);


    html = "<!DOCTYPE html><html><head><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><meta charset=\"utf-8";
    QTest::newRow("invalid charset (no closing double quote)") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);


    html = "<!DOCTYPE html><html><head><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><meta charset='utf-8";
    QTest::newRow("invalid charset (no closing single quote)") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);

    html = "<!DOCTYPE html><html><head><meta charset=utf-8 foo=bar><title>Test</title></head>";
    QTest::newRow("invalid (space terminator)") << html << std::optional<QStringConverter::Encoding>();

    html = "<!DOCTYPE html><html><head><meta charset=\" utf' 8 /><title>Test</title></head>";
    QTest::newRow("invalid charset, early terminator (')") << html << std::optional<QStringConverter::Encoding>();

    const char src[] = { char(0xff), char(0xfe), char(0x7a), char(0x03), 0, 0 };
    html = src;
    QTest::newRow("greek text UTF-16LE") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf16LE);


    html = "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><span style=\"color: rgb(0, 0, 0); font-family: "
        "'Galatia SIL'; font-size: 27px; font-style: normal; font-variant: normal; font-weight: normal; letter-spacing: normal; "
        "line-height: normal; orphans: auto; text-align: start; text-indent: 0px; text-transform: none; white-space: normal; widows: "
        "auto; word-spacing: 0px; -webkit-text-size-adjust: auto; -webkit-text-stroke-width: 0px; display: inline !important; float: "
        "none;\">&#x37b</span>\000";
    QTest::newRow("greek text UTF-8") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);

    html = "<!DOCTYPE html><html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=unicode\">"
            "<head/><body><p>bla</p></body></html>"; // QTBUG-41998, ICU will return UTF-16.
    QTest::newRow("legacy unicode UTF-8") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8);
}

void tst_QStringConverter::encodingForHtml()
{
    QFETCH(QByteArray, html);
    QFETCH(std::optional<QStringConverter::Encoding>, encoding);

    QCOMPARE(QStringConverter::encodingForHtml(html), encoding);
}

class LoadAndConvert: public QRunnable
{
public:
    LoadAndConvert(QStringConverter::Encoding encoding, QString *destination)
        : encode(encoding), decode(encoding), target(destination)
    {}
    QStringEncoder encode;
    QStringDecoder decode;
    QString *target;
    void run() override
    {
        QString str = QString::fromLatin1("abcdefghijklmonpqrstufvxyz");
        for (int i = 0; i < 10000; ++i) {
            QByteArray b = encode(str);
            *target = decode(b);
            QCOMPARE(*target, str);
        }
    }
};

void tst_QStringConverter::initTestCase()
{
    if (localeIsUtf8())
        qInfo("System locale is UTF-8");
    else
        qInfo("System locale is not UTF-8");
}

void tst_QStringConverter::threadSafety()
{
    QThreadPool::globalInstance()->setMaxThreadCount(12);

    QList<QString> res;
    res.resize(QStringConverter::LastEncoding + 1);
    for (int i = 0; i < QStringConverter::LastEncoding + 1; ++i) {
        QThreadPool::globalInstance()->start(new LoadAndConvert(QStringConverter::Encoding(i), &res[i]));
    }

    // wait for all threads to finish working
    QThreadPool::globalInstance()->waitForDone();

    for (auto b : res)
        QCOMPARE(b, QString::fromLatin1("abcdefghijklmonpqrstufvxyz"));
}

struct DontCrashAtExit {
    ~DontCrashAtExit() {
        QStringDecoder decoder(QStringDecoder::Utf8);
        QVERIFY(decoder.isValid());
        (void)decoder("azerty");
    }
} dontCrashAtExit;


QTEST_MAIN(tst_QStringConverter)
#include "tst_qstringconverter.moc"
