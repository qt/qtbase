// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include <QtCore/private/qglobal_p.h>
#include <qstringconverter.h>
#include <private/qstringconverter_p.h>
#include <qthreadpool.h>

#include <array>
#include <numeric>

using namespace Qt::StringLiterals;

static constexpr bool IsBigEndian = QSysInfo::ByteOrder == QSysInfo::BigEndian;
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

static const std::array encodedBoms = {
    QByteArrayView("\xef\xbb\xbf"),                         // Utf8,
    QByteArrayView(IsBigEndian ? "\xfe\xff" : "\xff\xfe"),  // Utf16,
    QByteArrayView("\xff\xfe"),                             // Utf16LE,
    QByteArrayView("\xfe\xff"),                             // Utf16BE,
    QByteArrayView(IsBigEndian ? "\0\0\xfe\xff" : "\xff\xfe\0", 4), // Utf32,
    QByteArrayView("\xff\xfe\0", 4),                        // Utf32LE,
    QByteArrayView("\0\0\xfe\xff", 4),                      // Utf32BE,
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
    TestString{ "ascii-with-carriage-return", "a\rb", u"a\rb", AsciiOnly },
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
    ROW("character+bom", "b\ufeff"),
    /* Check that the codec does NOT flag EFBFBF.
     * This is a regression test; see QTBUG-33229
     */
    ROW("last-bmp", "\uffff"),
    ROW("character+last-bmp", "b\uffff"),
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

    void invalidConverter();

    void convertUtf8_data();
    void convertUtf8();
    void convertUtf8CharByChar_data() { convertUtf8_data(); }
    void convertUtf8CharByChar();
    void roundtrip_data();
    void roundtrip();

    void convertL1U8();

    void convertL1U16();

#if QT_CONFIG(icu)
    void roundtripIcu_data();
    void roundtripIcu();
    void icuInvalidCharacter_data();
    void icuInvalidCharacter();
    void icuEncodeEdgeCases_data();
    void icuEncodeEdgeCases();
    void icuUsableAfterMove();
    void charByCharConsistency_data();
    void charByCharConsistency();
    void byteByByteConsistency_data();
    void byteByByteConsistency();
    void statefulPieceWise();
#endif

    void flagF7808080() const;

    void utf8Codec_data();
    void utf8Codec();

    void utf8bom_data();
    void utf8bom();
    void roundtripBom_data();
    void roundtripBom();

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

#ifdef Q_OS_WIN
    // On all other systems local 8-bit encoding is UTF-8
    void fromLocal8Bit_data();
    void fromLocal8Bit();
    void fromLocal8Bit_special_cases();
    void toLocal8Bit_data();
    void toLocal8Bit();
    void toLocal8Bit_special_cases();
#endif
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

void tst_QStringConverter::invalidConverter()
{
    // QStringEncoder tests
    {
        QStringEncoder encoder;
        QVERIFY(!encoder.isValid());
        QVERIFY(!encoder.name());
        QByteArray encoded = encoder(u"Some text");
        QVERIFY(encoded.isEmpty());
        QVERIFY(encoder.hasError());

        encoder.resetState();
        QVERIFY(!encoder.hasError());

        encoded = encoder.encode(u"More text");
        QVERIFY(encoded.isEmpty());
        QVERIFY(encoder.hasError());
        QCOMPARE(encoder.requiredSpace(42), 0);

        encoder.resetState();
        QVERIFY(!encoder.hasError());
        char buffer[100];
        char *position = encoder.appendToBuffer(buffer, u"Even more");
        QCOMPARE(position, buffer);
        QVERIFY(encoder.hasError());
    }

    // QStringDecoder tests
    {
        QStringDecoder decoder;
        QVERIFY(!decoder.name());
        QVERIFY(!decoder.isValid());
        QString decoded = decoder("Some text");
        QVERIFY(decoded.isEmpty());
        QVERIFY(decoder.hasError());

        decoder.resetState();
        QVERIFY(!decoder.hasError());

        decoded = decoder.decode("More text");
        QVERIFY(decoded.isEmpty());
        QVERIFY(decoder.hasError());

        QCOMPARE(decoder.requiredSpace(42), 0);

        decoder.resetState();
        QVERIFY(!decoder.hasError());
        char16_t buffer[100];
        char16_t *position = decoder.appendToBuffer(buffer, "Even more");
        QCOMPARE(position, buffer);
        QVERIFY(decoder.hasError());
    }
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

void tst_QStringConverter::convertL1U16()
{
    const QLatin1StringView latin1("some plain latin1 text");
    const QString qstr(latin1);

    QStringDecoder decoder(QStringConverter::Latin1);
    QVERIFY(decoder.isValid());
    QString uniString = decoder(latin1);
    QCOMPARE(uniString, qstr);
    QCOMPARE(latin1, uniString.toLatin1());

    // do it again (using .decode())
    uniString = decoder.decode(latin1);
    QCOMPARE(uniString, qstr);
    QCOMPARE(latin1, uniString.toLatin1());

    QStringEncoder encoder(QStringConverter::Latin1);
    QByteArray reencoded = encoder(uniString);
    QCOMPARE(reencoded, QByteArrayView(latin1));
    QCOMPARE(reencoded, uniString.toLatin1());

    // do it again (using .encode())
    reencoded = encoder.encode(uniString);
    QCOMPARE(reencoded, QByteArrayView(latin1));
    QCOMPARE(reencoded, uniString.toLatin1());
}

void tst_QStringConverter::roundtrip_data()
{
    QTest::addColumn<QStringView>("utf16");
    QTest::addColumn<QStringConverter::Encoding>("code");

    for (const auto &code : codes) {
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
            using Digits = std::array<QChar, 2>;
            using DigitsArray = std::array<Digits, 10>;
            static constexpr DigitsArray chakmaDigits = []() {
                const char32_t zeroVal = 0x11136; // Unicode's representation of Chakma zero
                DigitsArray r;
                for (int i = 0; i < int(r.size()); ++i)
                    r[i] = { QChar::highSurrogate(zeroVal + i), QChar::lowSurrogate(zeroVal + i) };
                return r;
            }();
            for (int i = 0; i < int(chakmaDigits.size()); ++i)
                QTest::addRow("%s:Chakma-digit-%d", code.name, i) << QStringView(chakmaDigits[i]) << code.code;
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
    QString decoded = back.decode(encoded);
    QCOMPARE(decoded, utf16);

    // test some flags
    QStringConverter::Flags flag = QStringEncoder::Flag::Stateless;
    {
        QStringEncoder out2(code, flag);
        QStringDecoder back2(code, flag);
        decoded = back2.decode(out2.encode(utf16));
        QCOMPARE(decoded, utf16);
    }
    flag |= QStringConverter::Flag::ConvertInvalidToNull;
    {
        QStringEncoder out2(code, flag);
        QStringDecoder back2(code, flag);
        decoded = back2.decode(out2.encode(utf16));
        QCOMPARE(decoded, utf16);
    }

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

    QStringEncoder out2(code, flag);
    QStringDecoder back2(code, flag);
    decoded = back2.decode(out2.encode(uniString));
    QCOMPARE(decoded, uniString);
}

void tst_QStringConverter::convertL1U8()
{
    {
        std::array<char, 256> latin1;
        std::iota(latin1.data(), latin1.data() + latin1.size(), uchar(0));
        std::array<char, 512> utf8;
        auto out = QUtf8::convertFromLatin1(utf8.data(), QLatin1StringView{latin1.data(), latin1.size()});
        QCOMPARE(QString::fromLatin1(latin1.data(), latin1.size()),
                 QString::fromUtf8(utf8.data(), out - utf8.data()));
    }
}

#if QT_CONFIG(icu)

void tst_QStringConverter::roundtripIcu_data()
{
    QTest::addColumn<QString>("original");
    QTest::addColumn<QByteArray>("codec");

    QTest::addRow("shift_jis") << u"古池や　蛙飛び込む　水の音"_s << QByteArray("shift_jis");
    QTest::addRow("UTF7") << u"Übermäßig: čçö"_s << QByteArray("UTF-7");
}

void tst_QStringConverter::roundtripIcu()
{
    QFETCH(QString, original);
    QFETCH(QByteArray, codec);
    QStringEncoder fromUtf16(codec);
    if (!fromUtf16.isValid())
        QSKIP("Unsupported codec");
    QStringDecoder toUtf16(codec);
    QByteArray asShiftJIS = fromUtf16(original);
    QString roundTripped = toUtf16(asShiftJIS);
    QCOMPARE(roundTripped, original);
}

void tst_QStringConverter::icuEncodeEdgeCases_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QByteArray>("expected") ;
    QTest::addColumn<QByteArray>("codec");

    QTest::addRow("empty") << QString() << QByteArray() << QByteArray("ISO-2022-CN");
    QTest::addRow("BOMonly") << QString(QChar(QChar::ByteOrderMark)) << QByteArray() << QByteArray("ISO-2022-CN");
    QTest::addRow("1to6") << u"좋"_s << QByteArray::fromHex("1b2428434141") << QByteArray("ISO-2022-JP-2");
    QTest::addRow("1to7") << u"漢"_s << QByteArray::fromHex("1b2429470e6947") << QByteArray("ISO-2022-CN");
    QTest::addRow("1to8") << u"墎"_s << QByteArray::fromHex("1b242a481b4e4949")  << QByteArray("ISO-2022-CN");
    QTest::addRow("utf7") << u"Übergröße"_s << QByteArray("+ANw-bergr+APYA3w-e") << QByteArray("UTF-7");
}

void tst_QStringConverter::icuEncodeEdgeCases()
{
    QFETCH(QString, source);
    QFETCH(QByteArray, expected);
    QFETCH(QByteArray, codec);
    QStringEncoder encoder(codec);
    if (!encoder.isValid())
        QSKIP("Unsupported codec");
    QVERIFY(encoder.isValid());
    QByteArray encoded = encoder.encode(source);
    QCOMPARE(encoded, expected);
}

void tst_QStringConverter::charByCharConsistency_data()
{
    QTest::addColumn<QStringView>("source");
    QTest::addColumn<QByteArray>("codec");

    auto addRow = [](const TestString &s) {
        QTest::addRow("%s_shift_jis", s.description) << s.utf16 << QByteArray("shift_jis");
        QTest::addRow("%s_EUC-CN", s.description) << s.utf16 << QByteArray("EUC-CN");
    };

    for (const TestString &s : testStrings) {
        if (s.utf16.isEmpty())
            continue;
        addRow(s);
    }
}

void tst_QStringConverter::charByCharConsistency()
{
    QFETCH(QStringView, source);
    QFETCH(QByteArray, codec);

    {
        QStringEncoder encoder(codec);
        if (!encoder.isValid())
            QSKIP("Unsupported codec");

        QByteArray fullyConverted = encoder.encode(source);
        encoder.resetState();
        QByteArray stepByStepConverted;
        for (const auto& codeUnit: source) {
            stepByStepConverted += encoder.encode(codeUnit);
        }
        QCOMPARE(stepByStepConverted, fullyConverted);
    }

    {
        QStringEncoder encoder(codec, QStringConverter::Flag::ConvertInvalidToNull);

        QByteArray fullyConverted = encoder.encode(source);
        encoder.resetState();
        QByteArray stepByStepConverted;
        for (const auto& codeUnit: source) {
            stepByStepConverted += encoder.encode(codeUnit);
        }
        QCOMPARE(stepByStepConverted, fullyConverted);
    }
}

void tst_QStringConverter::byteByByteConsistency_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<QByteArray>("codec");

    QTest::addRow("plain_ascii_utf7") << QByteArray("Hello, world!") << QByteArray("UTF-7");
    QFile eucKr(":/euc_kr.txt");
    if (eucKr.open(QFile::ReadOnly))
        QTest::addRow("euc_kr_storing_jp") << eucKr.readAll() << QByteArray("EUC-KR");
    QTest::addRow("incomplete_euc_jp") << QByteArrayLiteral("test\x8Ftest") << QByteArray("EUC-JP");
}

void tst_QStringConverter::byteByByteConsistency()
{
    QFETCH(QByteArray, source);
    QFETCH(QByteArray, codec);

    {
        QStringDecoder decoder(codec);
        if (!decoder.isValid())
            QSKIP("Unsupported codec");

        QString fullyConverted = decoder.decode(source);
        decoder.resetState();
        QString stepByStepConverted;
        for (const auto& byte: source) {
            QByteArray singleChar;
            singleChar.append(byte);
            stepByStepConverted += decoder.decode(singleChar);
        }
        QCOMPARE(stepByStepConverted, fullyConverted);
    }

    {
        QStringDecoder decoder(codec, QStringConverter::Flag::ConvertInvalidToNull);
        if (!decoder.isValid())
            QSKIP("Unsupported codec");

        QString fullyConverted = decoder.decode(source);
        decoder.resetState();
        QString stepByStepConverted;
        for (const auto& byte: source) {
            QByteArray singleChar;
            singleChar.append(byte);
            stepByStepConverted += decoder.decode(singleChar);
        }
        QCOMPARE(stepByStepConverted, fullyConverted);
    }
}

void tst_QStringConverter::statefulPieceWise()
{
    QStringDecoder decoder("HZ");
    if (!decoder.isValid())
        QSKIP("Unsupported codec");
    QString start = decoder.decode("pure ASCII");
    QCOMPARE(start, u"pure ASCII");
    QString shifted = decoder.decode("~{");
    // shift out changes the state, but won't create any output
    QCOMPARE(shifted, "");
    QString continuation = decoder.decode("\x42\x43");
    QCOMPARE(continuation, "旅");
    decoder.resetState();
    // after resetting the state we're in N0 again
    QString afterReset = decoder.decode("\x42\x43");
    QCOMPARE(afterReset, "BC");
}

void tst_QStringConverter::icuUsableAfterMove()
{
    {
        QStringDecoder decoder("EUC-JP");
        QVERIFY(decoder.isValid());
        QString partial = decoder.decode("Test\x8E");
        QCOMPARE(partial, u"Test"_s);
        QStringDecoder moved(std::move(decoder));
        QString complete = partial + moved.decode("\xA1Test");
        QCOMPARE(complete, u"Test\uFF61Test"_s);
    }
    {
        QStringEncoder encoder("Big5");
        QVERIFY(encoder.isValid());
        QByteArray encoded = encoder.encode("hello"_L1);
        QCOMPARE(encoded, "hello");
        QStringEncoder moved(std::move(encoder));
        encoded = moved.encode("bye");
        QCOMPARE(encoded, "bye");
    }
}

void tst_QStringConverter::icuInvalidCharacter_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QByteArray>("bytearray");
    QTest::addColumn<QByteArray>("codec");
    QTest::addColumn<QStringConverter::Flags>("flags");
    QTest::addColumn<bool>("shouldDecode");

    using Flags = QStringConverter::Flags;
    using Flag = QStringConverter::Flag;
    QTest::addRow("encode")
            << u"Test👪Test"_s
            << QByteArrayLiteral("\xE3\x85\xA2\xA3\x3F\xE3\x85\xA2\xA3")
            << QByteArray("IBM-037") << Flags(Flag::Default)
            << false;
    QTest::addRow("encode_null")
            << u"Test👪Test"_s
            << QByteArrayLiteral("\xE3\x85\xA2\xA3\0\xE3\x85\xA2\xA3")
            << QByteArray("IBM-037") << Flags(Flag::ConvertInvalidToNull)
            << false;
    QTest::addRow("decode_incomplete_EUC-JP")
            << u"test"_s
            << QByteArrayLiteral("test\x8F")
            << QByteArray("EUC-JP") << Flags(Flag::Stateless)
            << true;
    QTest::addRow("decode_invalid_EUC-JP_sequence")
            << u"test\0test"_s
            << QByteArrayLiteral("test\x8Ftest")
            << QByteArray("EUC-JP") << Flags(Flag::ConvertInvalidToNull)
            << true;
    QTest::addRow("encode_incomplete_surrogate")
            << u"test"_s + QChar::highSurrogate(0x11136)
            << QByteArray("test")
            << QByteArray("EUC-JP") << Flags(Flag::Stateless)
            << false;
}

void tst_QStringConverter::icuInvalidCharacter()
{
    QFETCH(QString, string);
    QFETCH(QByteArray, bytearray);
    QFETCH(QByteArray, codec);
    QFETCH(QStringConverter::Flags, flags);
    QFETCH(bool, shouldDecode);
    if (shouldDecode) {
        QStringDecoder decoder(codec.data(), flags);
        QVERIFY(decoder.isValid());
        QString decoded = decoder.decode(bytearray);
        QVERIFY(decoder.hasError());
        QCOMPARE(decoded, string);
    } else {
        QStringEncoder encoder(codec.data(), flags);
        QVERIFY(encoder.isValid());
        QByteArray encoded = encoder.encode(string);
        QVERIFY(encoder.hasError());
        QCOMPARE(encoded, bytearray);
    }
}

#endif

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

// someone set us up the BOM!
void tst_QStringConverter::roundtripBom_data()
{
    QTest::addColumn<QStringView>("utf16");
    QTest::addColumn<QStringConverter::Encoding>("code");

    for (const auto &code : codes) {
        if (size_t(code.code) >= encodedBoms.size())
            break;
        if (code.limitation != FullUnicode)
            continue;           // can't represent BOM

        for (const TestString &s : testStrings) {
            if (s.utf16.isEmpty())
                continue;
            QTest::addRow("%s:%s", code.name, s.description) << s.utf16 << code.code;
        }
    }
}

void tst_QStringConverter::roundtripBom()
{
    QFETCH(QStringView, utf16);
    QFETCH(QStringConverter::Encoding, code);
    QByteArray encodedBom = encodedBoms[code].toByteArray();
    QChar bom = QChar::ByteOrderMark;

    // QStringConverter defaults to producing no BOM, but interpreting it if it
    // is there

    QStringEncoder encoderWithoutBom(code);
    QStringEncoder encoderWithBom(code, QStringEncoder::Flag::WriteBom);
    QByteArray encodedWithoutBom = encoderWithoutBom(utf16);
    QByteArray encodedWithBom = encoderWithBom(utf16);
    QCOMPARE(encodedWithBom, encodedBom + encodedWithoutBom);

    QStringDecoder decoderWithoutBom(code, QStringDecoder::Flag::ConvertInitialBom);
    QStringDecoder decoderWithBom(code);
    QString decoded = decoderWithBom(encodedWithBom);
    QCOMPARE(decoded, utf16);

    decoded = decoderWithoutBom(encodedWithBom);
    QCOMPARE(decoded, bom + utf16.toString());
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
        QCOMPARE(result.size(), unicode.size());
        QCOMPARE(result, unicode);
    }

    {
        QStringDecoder decode(encoding, flags);
        QVERIFY(decode.isValid());

        QString result;
        for (char c : encoded)
            result += decode(QByteArrayView(&c, 1));
        QCOMPARE(result.size(), unicode.size());
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
    QTest::newRow("<empty>") << QByteArray("") << std::optional<QStringConverter::Encoding>();
    QTest::newRow("<nullptr>") << QByteArray(nullptr) << std::optional<QStringConverter::Encoding>();
}

void tst_QStringConverter::encodingForName()
{
    QFETCH(const QByteArray, name);
    QFETCH(const std::optional<QStringConverter::Encoding>, encoding);

    const auto *ptr = name.isNull() ? nullptr : name.data();

    const auto e = QStringConverter::encodingForName(ptr);
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
    QTest::addColumn<QByteArray>("name"); // ICU name if we have ICU support

    QByteArray html = "<html><head></head><body>blah</body></html>";
    QTest::newRow("no charset") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8) << QByteArray("UTF-8");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-15\" /></head></html>";
    QTest::newRow("latin 15") << html << std::optional<QStringConverter::Encoding>() << QByteArray("ISO-8859-15");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=SJIS\" /></head></html>";
    QTest::newRow("sjis") << html << std::optional<QStringConverter::Encoding>() << QByteArray("Shift_JIS");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-2022-JP\" /></head></html>";
    QTest::newRow("ISO-2022-JP") << html << std::optional<QStringConverter::Encoding>() << QByteArray("ISO-2022-JP");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-2022\" /></head></html>";
    QTest::newRow("ISO-2022") << html << std::optional<QStringConverter::Encoding>() << QByteArray("ISO-2022-JP");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=GB2312\" /></head></html>";
    QTest::newRow("GB2312") << html << std::optional<QStringConverter::Encoding>() << QByteArray("GB2312");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=Big5\" /></head></html>";
    QTest::newRow("Big5") << html << std::optional<QStringConverter::Encoding>() << QByteArray("Big5");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=GB18030\" /></head></html>";
    QTest::newRow("GB18030") << html << std::optional<QStringConverter::Encoding>() << QByteArray("GB18030");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=GB2312-HKSCS\" /></head></html>";
    QTest::newRow("GB2312-HKSCS") << html << std::optional<QStringConverter::Encoding>() << QByteArray("GB2312-HKSCS");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=Big5-HKSCS\" /></head></html>";
    QTest::newRow("Big5-HKSCS") << html << std::optional<QStringConverter::Encoding>() << QByteArray("Big5-HKSCS");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=EucJP\" /></head></html>";
    QTest::newRow("EucJP") << html << std::optional<QStringConverter::Encoding>() << QByteArray("EUC-JP");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=EucKR\" /></head></html>";
    QTest::newRow("EucKR") << html << std::optional<QStringConverter::Encoding>() << QByteArray("EUC-KR");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=KOI8-R\" /></head></html>";
    QTest::newRow("KOI8-R") << html << std::optional<QStringConverter::Encoding>() << QByteArray("KOI8-R");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=KOI8-U\" /></head></html>";
    QTest::newRow("KOI8-U") << html << std::optional<QStringConverter::Encoding>() << QByteArray("KOI8-U");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-1\" /></head></html>";
    QTest::newRow("latin 1") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Latin1) << QByteArray("ISO-8859-1");

    html = "<!DOCTYPE html><html><head><meta charset=\"ISO_8859-1:1987\"><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><title>Test</title></head>";
    QTest::newRow("latin 1 (#2)") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Latin1) << QByteArray("ISO-8859-1");

    html = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><title>Test</title></head>";
    QTest::newRow("UTF-8") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8) << QByteArray("UTF-8");

    html = "<!DOCTYPE html><html><head><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><meta charset=\"utf-8\"><title>Test</title></head>";
    QTest::newRow("UTF-8 (#2)") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8) << QByteArray("UTF-8");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8/></head></html>";
    QTest::newRow("UTF-8, no quotes") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8) << QByteArray("UTF-8");

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset='UTF-8'/></head></html>";
    QTest::newRow("UTF-8, single quotes") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8) << QByteArray("UTF-8");

    html = "<!DOCTYPE html><html><head><meta charset=utf-8><title>Test</title></head>";
    QTest::newRow("UTF-8, > terminator") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8) << QByteArray("UTF-8");

    html = "<!DOCTYPE html><html><head><meta charset= utf-8 ><title>Test</title></head>";
    QTest::newRow("UTF-8, > terminator with spaces") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8) << QByteArray("UTF-8");

    // Test invalid charsets.
    html = "<!DOCTYPE html><html><head><meta charset= utf/8 ><title>Test</title></head>";
    QTest::newRow("utf/8") << html << std::optional<QStringConverter::Encoding>()  << QByteArray();

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=invalid-foo\" /></head></html>";
    QTest::newRow("invalid charset, no default") << html << std::optional<QStringConverter::Encoding>() << QByteArray("UTF-8");

    html = "<!DOCTYPE html><html><head><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><meta charset=\"";
    html.prepend(QByteArray().fill(' ', 512 - html.size()));
    QTest::newRow("invalid charset (large header)") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8) << QByteArray("UTF-8");


    html = "<!DOCTYPE html><html><head><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><meta charset=\"utf-8";
    QTest::newRow("invalid charset (no closing double quote)") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8) << QByteArray("UTF-8");


    html = "<!DOCTYPE html><html><head><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><meta charset='utf-8";
    QTest::newRow("invalid charset (no closing single quote)") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8) << QByteArray("UTF-8");

    html = "<!DOCTYPE html><html><head><meta charset=utf-8 foo=bar><title>Test</title></head>";
    QTest::newRow("invalid (space terminator)") << html << std::optional<QStringConverter::Encoding>() << QByteArray();

    html = "<!DOCTYPE html><html><head><meta charset=\" utf' 8 /><title>Test</title></head>";
    QTest::newRow("invalid charset, early terminator (')") << html << std::optional<QStringConverter::Encoding>() << QByteArray();

    const char src[] = { char(0xff), char(0xfe), char(0x7a), char(0x03), 0, 0 };
    html = src;
    QTest::newRow("greek text UTF-16LE") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf16LE)  << QByteArray("UTF-16LE");


    html = "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><span style=\"color: rgb(0, 0, 0); font-family: "
        "'Galatia SIL'; font-size: 27px; font-style: normal; font-variant: normal; font-weight: normal; letter-spacing: normal; "
        "line-height: normal; orphans: auto; text-align: start; text-indent: 0px; text-transform: none; white-space: normal; widows: "
        "auto; word-spacing: 0px; -webkit-text-size-adjust: auto; -webkit-text-stroke-width: 0px; display: inline !important; float: "
        "none;\">&#x37b</span>\000";
    QTest::newRow("greek text UTF-8") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8) << QByteArray("UTF-8");

    html = "<!DOCTYPE html><html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=unicode\">"
            "<head/><body><p>bla</p></body></html>"; // QTBUG-41998, ICU will return UTF-16.
    QTest::newRow("legacy unicode UTF-8") << html << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8) << QByteArray("UTF-8");
}

void tst_QStringConverter::encodingForHtml()
{
    QFETCH(QByteArray, html);
    QFETCH(std::optional<QStringConverter::Encoding>, encoding);
    QFETCH(QByteArray, name);

    QCOMPARE(QStringConverter::encodingForHtml(html), encoding);

    QStringDecoder decoder = QStringDecoder::decoderForHtml(html);
    if (encoding || // we should have a valid decoder independent of ICU support
        decoder.isValid()) { // we got a valid decoder through ICU
        QCOMPARE(decoder.name(), name);
    }
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
#if defined(Q_OS_WASM)
    QSKIP("This test misbehaves on WASM. Investigation needed (QTBUG-110067)");
#endif

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

#ifdef Q_OS_WIN
void tst_QStringConverter::fromLocal8Bit_data()
{
    QTest::addColumn<QByteArray>("eightBit");
    QTest::addColumn<QString>("utf16");
    QTest::addColumn<quint32>("codePage");

    constexpr uint WINDOWS_1252 = 1252u;
    QTest::newRow("windows-1252") << "Hello, world!"_ba << u"Hello, world!"_s << WINDOWS_1252;
    constexpr uint SHIFT_JIS = 932u;
    // Mostly two byte characters, but the comma is a single byte character (0xa4)
    QTest::newRow("shiftJIS")
            << "\x82\xb1\x82\xf1\x82\xc9\x82\xbf\x82\xcd\xa4\x90\xa2\x8a\x45\x81\x49"_ba
            << u"こんにちは､世界！"_s << SHIFT_JIS;

    constexpr uint GB_18030 = 54936u;
    QTest::newRow("GB-18030") << "\xc4\xe3\xba\xc3\xca\xc0\xbd\xe7\xa3\xa1"_ba << u"你好世界！"_s
                              << GB_18030;
}

void tst_QStringConverter::fromLocal8Bit()
{
    QFETCH(const QByteArray, eightBit);
    QFETCH(const QString, utf16);
    QFETCH(const quint32, codePage);

    QStringConverter::State state;

    QString result = QLocal8Bit::convertToUnicode_sys(eightBit, codePage, &state);
    QCOMPARE(result, utf16);
    QCOMPARE(state.remainingChars, 0);

    result.clear();
    state.clear();
    for (char c : eightBit)
        result += QLocal8Bit::convertToUnicode_sys({&c, 1}, codePage, &state);
    QCOMPARE(result, utf16);
    QCOMPARE(state.remainingChars, 0);

    result.clear();
    state.clear();
    // Decode the full string again, this time without state
    state.flags |= QStringConverter::Flag::Stateless;
    result = QLocal8Bit::convertToUnicode_sys(eightBit, codePage, &state);
    QCOMPARE(result, utf16);
    QCOMPARE(state.remainingChars, 0);
}

void tst_QStringConverter::fromLocal8Bit_special_cases()
{
    QStringConverter::State state;
    constexpr uint SHIFT_JIS = 932u;
    // Decode a 2-octet character, but only provide 1 octet at first:
    QString result = QLocal8Bit::convertToUnicode_sys("\x82", SHIFT_JIS, &state);
    QCOMPARE(result, QString());
    QVERIFY(result.isNull());
    QCOMPARE_GT(state.remainingChars, 0);
    // Then provide the second octet:
    result = QLocal8Bit::convertToUnicode_sys("\xb1", SHIFT_JIS, &state);
    QCOMPARE(result, u"こ");
    QCOMPARE(state.remainingChars, 0);

    // And without state:
    result.clear();
    QStringConverter::State statelessState;
    statelessState.flags |= QStringConverter::Flag::Stateless;
    result = QLocal8Bit::convertToUnicode_sys("\x82", SHIFT_JIS, &statelessState);
    result += QLocal8Bit::convertToUnicode_sys("\xb1", SHIFT_JIS, &statelessState);
    // 0xb1 is a valid single-octet character in Shift-JIS, so the output
    // isn't really what you would expect:
    QCOMPARE(result, QString(QChar::ReplacementCharacter) + u'ｱ');
    QCOMPARE(statelessState.remainingChars, 0);

    // Now try a 3-octet UTF-8 sequence:
    result.clear();
    state.clear();
    constexpr uint UTF8 = 65001u;
    // First the first 2 octets:
    result = QLocal8Bit::convertToUnicode_sys("\xe4\xbd", UTF8, &state);
    QCOMPARE(result, QString());
    QVERIFY(result.isNull());
    QCOMPARE_GT(state.remainingChars, 0);
    // Then provide the remaining octet:
    result = QLocal8Bit::convertToUnicode_sys("\xa0", UTF8, &state);
    QCOMPARE(result, u"你");
    QCOMPARE(state.remainingChars, 0);

    // Now try a 4-octet GB 18030 sequence:
    result.clear();
    state.clear();
    constexpr uint GB_18030 = 54936u;
    const char sequence[] = "\x95\x32\x90\x31";
    // Repeat the sequence multiple times to test handling of exhaustion of
    // internal buffer
    QByteArray repeated = QByteArray(sequence).repeated(2049);
    QByteArrayView octets = QByteArrayView(repeated);
    result = QLocal8Bit::convertToUnicode_sys(octets.first(2), GB_18030, &state);
    QCOMPARE(result, QString());
    QVERIFY(result.isNull());
    QCOMPARE_GT(state.remainingChars, 0);
    // Then provide one more octet:
    result = QLocal8Bit::convertToUnicode_sys(octets.sliced(2, 1), GB_18030, &state);
    QCOMPARE(result, QString());
    QVERIFY(result.isNull());
    QCOMPARE_GT(state.remainingChars, 0);
    // Then provide the last octet + the rest of the string
    result = QLocal8Bit::convertToUnicode_sys(octets.sliced(3), GB_18030, &state);
    QCOMPARE(result.first(2), u"𠂇");
    QCOMPARE(state.remainingChars, 0);
}

void tst_QStringConverter::toLocal8Bit_data()
{
    fromLocal8Bit_data();
}

void tst_QStringConverter::toLocal8Bit()
{
    QFETCH(const QByteArray, eightBit);
    QFETCH(const QString, utf16);
    QFETCH(const quint32, codePage);

    QStringConverter::State state;

    QByteArray result = QLocal8Bit::convertFromUnicode_sys(utf16, codePage, &state);
    QCOMPARE(result, eightBit);
    QCOMPARE(state.remainingChars, 0);

    result.clear();
    state.clear();
    for (QChar c : utf16)
        result += QLocal8Bit::convertFromUnicode_sys(QStringView(&c, 1), codePage, &state);
    QCOMPARE(result, eightBit);
    QCOMPARE(state.remainingChars, 0);

    result.clear();
    state.clear();
    // Decode the full string again, this time without state
    state.flags |= QStringConverter::Flag::Stateless;
    result = QLocal8Bit::convertFromUnicode_sys(utf16, codePage, &state);
    QCOMPARE(result, eightBit);
    QCOMPARE(state.remainingChars, 0);
}

void tst_QStringConverter::toLocal8Bit_special_cases()
{
    QStringConverter::State state;
    // Normally utf8 goes through a different code path, but we can force it here
    constexpr uint UTF8 = 65001u;
    // Decode a 2-code unit character, but only provide 1 code unit at first:
    const char16_t a[] = u"𬽦";
    QStringView codeUnits = a;
    QByteArray result = QLocal8Bit::convertFromUnicode_sys(codeUnits.first(1), UTF8, &state);
    QCOMPARE(result, QString());
    QVERIFY(result.isNull());
    QCOMPARE_GT(state.remainingChars, 0);
    // Then provide the second code unit:
    result = QLocal8Bit::convertFromUnicode_sys(codeUnits.sliced(1), UTF8, &state);
    QCOMPARE(result, "\xf0\xac\xbd\xa6"_ba);
    QCOMPARE(state.remainingChars, 0);

    // Retain compat with the behavior for toLocal8Bit:
    QCOMPARE(codeUnits.first(1).toLocal8Bit(), "?");

    // QString::toLocal8Bit is already stateless, but test stateless handling
    // explicitly anyway:
    result.clear();
    QStringConverter::State statelessState;
    statelessState.flags |= QStringConverter::Flag::Stateless;
    result = QLocal8Bit::convertFromUnicode_sys(codeUnits.first(1), UTF8, &statelessState);
    result += QLocal8Bit::convertFromUnicode_sys(codeUnits.sliced(1), UTF8, &statelessState);
    // Windows uses the replacement character for invalid characters:
    QCOMPARE(result, "\ufffd\ufffd");

    // Now do the same, but the second time we feed in a character, we also
    // provide many more so the internal stack buffer is not large enough.
    result.clear();
    state.clear();
    QString str = QStringView(a).toString().repeated(2048);
    codeUnits = str;
    result = QLocal8Bit::convertFromUnicode_sys(codeUnits.first(1), UTF8, &state);
    QCOMPARE(result, QString());
    QVERIFY(result.isNull());
    QCOMPARE_GT(state.remainingChars, 0);
    // Then we provide the rest of the string:
    result = QLocal8Bit::convertFromUnicode_sys(codeUnits.sliced(1), UTF8, &state);
    QCOMPARE(result.first(4), "\xf0\xac\xbd\xa6"_ba);
    QCOMPARE(state.remainingChars, 0);
}
#endif // Q_OS_WIN

struct DontCrashAtExit {
    ~DontCrashAtExit() {
        QStringDecoder decoder(QStringDecoder::Utf8);
        QVERIFY(decoder.isValid());
        (void)decoder("azerty");
    }
} dontCrashAtExit;


QTEST_MAIN(tst_QStringConverter)
#include "tst_qstringconverter.moc"
