/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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


#include <QtTest/QtTest>

#include <qtextcodec.h>
#include <qfile.h>
#include <time.h>
#include <qprocess.h>
#include <QThreadPool>

class tst_QTextCodec : public QObject
{
    Q_OBJECT

private slots:
    void threadSafety();

    void toUnicode_data();
    void toUnicode();
    void codecForName_data();
    void codecForName();
    void fromUnicode_data();
    void fromUnicode();
    void toUnicode_codecForHtml();
    void toUnicode_incremental();
    void codecForLocale();

    void asciiToIscii() const;
    void nonFlaggedCodepointFFFF() const;
    void flagF7808080() const;
    void nonFlaggedEFBFBF() const;
    void decode0D() const;
    void aliasForUTF16() const;
    void mibForTSCII() const;
    void codecForTSCII() const;
    void iso8859_16() const;

    void utf8Codec_data();
    void utf8Codec();

    void utf8bom_data();
    void utf8bom();

    void utf8stateful_data();
    void utf8stateful();

    void utfHeaders_data();
    void utfHeaders();

    void codecForHtml_data();
    void codecForHtml();

    void codecForUtfText_data();
    void codecForUtfText();

#if defined(Q_OS_UNIX)
    void toLocal8Bit();
#endif

    void invalidNames();
    void checkAliases_data();
    void checkAliases();

    void moreToFromUnicode_data();
    void moreToFromUnicode();

    void shiftJis();
    void userCodec();
};

void tst_QTextCodec::toUnicode_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("codecName");

    QTest::newRow( "korean-eucKR" ) << QFINDTESTDATA("korean.txt") << "eucKR";
    QTest::newRow( "UTF-8" ) << QFINDTESTDATA("utf8.txt") << "UTF-8";
}

void tst_QTextCodec::toUnicode()
{
    QFETCH( QString, fileName );
    QFETCH( QString, codecName );

    QFile file( fileName );

    if ( file.open( QIODevice::ReadOnly ) ) {
        QByteArray ba = file.readAll();
        QVERIFY(!ba.isEmpty());
        QTextCodec *c = QTextCodec::codecForName( codecName.toLatin1() );
        QVERIFY(c != 0);
        QString uniString = c->toUnicode( ba );
        if (codecName == QLatin1String("UTF-8")) {
            QCOMPARE(uniString, QString::fromUtf8(ba));
            QCOMPARE(ba, uniString.toUtf8());
        }
        QVERIFY(!uniString.isEmpty());
        QCOMPARE( ba, c->fromUnicode( uniString ) );

        char ch = '\0';
        QVERIFY(c->toUnicode(&ch, 1).length() == 1);
        QVERIFY(c->toUnicode(&ch, 1).at(0).unicode() == 0);
    } else {
        QFAIL(qPrintable("File could not be opened: " + file.errorString()));
    }
}

void tst_QTextCodec::codecForName_data()
{
    QTest::addColumn<QString>("hint");
    QTest::addColumn<QString>("actualCodecName");

    QTest::newRow("data1") << "iso88591" << "ISO-8859-1";
    QTest::newRow("data2") << "iso88592" << "ISO-8859-2";
    QTest::newRow("data3") << " IsO(8)8/5*9-2 " << "ISO-8859-2";
    QTest::newRow("data4") << " IsO(8)8/5*2-9 " << "";
    QTest::newRow("data5") << "latin2" << "ISO-8859-2";
}

void tst_QTextCodec::codecForName()
{
    QFETCH(QString, hint);
    QFETCH(QString, actualCodecName);

    QTextCodec *codec = QTextCodec::codecForName(hint.toLatin1());
    if (actualCodecName.isEmpty()) {
        QVERIFY(!codec);
    } else {
        QVERIFY(codec != 0);
        QCOMPARE(QString(codec->name()), actualCodecName);
    }
}

void tst_QTextCodec::fromUnicode_data()
{
    QTest::addColumn<QString>("codecName");
    QTest::addColumn<bool>("eightBit");

    QTest::newRow("ISO-8859-1") << "ISO-8859-1" << true;
    QTest::newRow("ISO-8859-2") << "ISO-8859-2" << true;
    QTest::newRow("ISO-8859-3") << "ISO-8859-3" << true;
    QTest::newRow("ISO-8859-4") << "ISO-8859-4" << true;
    QTest::newRow("ISO-8859-5") << "ISO-8859-5" << true;
    QTest::newRow("ISO-8859-6") << "ISO-8859-6" << true;
    QTest::newRow("ISO-8859-7") << "ISO-8859-7" << true;
    QTest::newRow("ISO-8859-8") << "ISO-8859-8" << true;
    QTest::newRow("ISO-8859-9") << "ISO-8859-9" << true;
    QTest::newRow("ISO-8859-10") << "ISO-8859-10" << true;
    QTest::newRow("ISO-8859-13") << "ISO-8859-13" << true;
    QTest::newRow("ISO-8859-14") << "ISO-8859-14" << true;
    QTest::newRow("ISO-8859-15") << "ISO-8859-15" << true;
//    QTest::newRow("ISO-8859-16") << "ISO-8859-16" << true;

    QTest::newRow("IBM850") << "IBM850" << true;
    QTest::newRow("IBM874") << "IBM874" << true;
    QTest::newRow("IBM866") << "IBM866" << true;

    QTest::newRow("windows-1250") << "windows-1250" << true;
    QTest::newRow("windows-1251") << "windows-1251" << true;
    QTest::newRow("windows-1252") << "windows-1252" << true;
    QTest::newRow("windows-1253") << "windows-1253" << true;
    QTest::newRow("windows-1254") << "windows-1254" << true;
    QTest::newRow("windows-1255") << "windows-1255" << true;
    QTest::newRow("windows-1256") << "windows-1256" << true;
    QTest::newRow("windows-1257") << "windows-1257" << true;
    QTest::newRow("windows-1258") << "windows-1258" << true;

    QTest::newRow("Apple Roman") << "Apple Roman" << true;
    //QTest::newRow("WINSAMI2") << "WINSAMI2" << true;
    QTest::newRow("TIS-620") << "TIS-620" << true;
    QTest::newRow("SJIS") << "SJIS" << false;

    // all codecs from documentation
    QTest::newRow("Big5") << "Big5" << false;
    QTest::newRow("Big5-HKSCS") << "Big5-HKSCS" << false;
    QTest::newRow("CP949") << "CP949" << false;
    QTest::newRow("windows-949") << "windows-949" << false;
    QTest::newRow("EUC-JP") << "EUC-JP" << false;
    QTest::newRow("EUC-KR") << "EUC-KR" << false;
    QTest::newRow("GB18030") << "GB18030" << false;
    QTest::newRow("HP-ROMAN8") << "HP-ROMAN8" << false;
    QTest::newRow("IBM 850") << "IBM 850" << false;
    QTest::newRow("IBM 866") << "IBM 866" << false;
    QTest::newRow("IBM 874") << "IBM 874" << false;
    QTest::newRow("ISO 2022-JP") << "ISO 2022-JP" << false;
    //ISO 8859-1 to 10 and  ISO 8859-13 to 16 tested previously
    // Iscii-Bng, Dev, Gjr, Knd, Mlm, Ori, Pnj, Tlg, and Tml  tested in Iscii test
    QTest::newRow("KOI8-R") << "KOI8-R" << false;
    QTest::newRow("KOI8-U") << "KOI8-U" << false;
    QTest::newRow("Macintosh") << "Macintosh" << true;
    QTest::newRow("Shift-JIS") << "Shift-JIS" << false;
    QTest::newRow("TIS-620") << "TIS-620" << false;
    QTest::newRow("TSCII") << "TSCII" << false;
    QTest::newRow("UTF-8") << "UTF-8" << false;
    QTest::newRow("UTF-16") << "UTF-16" << false;
    QTest::newRow("UTF-16BE") << "UTF-16BE" << false;
    QTest::newRow("UTF-16LE") << "UTF-16LE" << false;
    QTest::newRow("UTF-32") << "UTF-32" << false;
    QTest::newRow("UTF-32BE") << "UTF-32BE" << false;
    QTest::newRow("UTF-32LE") << "UTF-32LE" << false;
    //Windows-1250 to 1258 tested previously
}

void tst_QTextCodec::fromUnicode()
{
    QFETCH(QString, codecName);
    QFETCH(bool, eightBit);

    QTextCodec *codec = QTextCodec::codecForName(codecName.toLatin1());
    QVERIFY(codec != 0);

    // Check if the reverse lookup is what we expect
    if (eightBit) {
        char chars[128];
        for (int i = 0; i < 128; ++i)
            chars[i] = i + 128;
        QString s = codec->toUnicode(chars, 128);
        QByteArray c = codec->fromUnicode(s);
        QCOMPARE(c.size(), 128);

        int numberOfQuestionMarks = 0;
        for (int i = 0; i < 128; ++i) {
            if (c.at(i) == '?')
                ++numberOfQuestionMarks;
            else
                QCOMPARE(c.at(i), char(i + 128));
        }
        QVERIFY(numberOfQuestionMarks != 128);
    }

    /*
        If the encoding is a superset of ASCII, test that the byte
        array is correct (no off by one, no trailing '\0').
    */
    QByteArray result = codec->fromUnicode(QString("abc"));
    if (result.startsWith('a')) {
        QCOMPARE(result.size(), 3);
        QCOMPARE(result, QByteArray("abc"));
    } else {
        QVERIFY(true);
    }
}

void tst_QTextCodec::toUnicode_codecForHtml()
{
    QFile file(QFINDTESTDATA("QT4-crashtest.txt"));
    QVERIFY(file.open(QFile::ReadOnly));

    QByteArray data = file.readAll();
    QTextCodec *codec = QTextCodec::codecForHtml(data);
    codec->toUnicode(data); // this line crashes
}


void tst_QTextCodec::toUnicode_incremental()
{
    QByteArray ba;
    ba += char(0xf0);
    ba += char(0x90);
    ba += char(0x80);
    ba += char(0x80);
    ba += char(0xf4);
    ba += char(0x8f);
    ba += char(0xbf);
    ba += char(0xbd);

    QString expected = QString::fromUtf8(ba);

    QString incremental;
    QTextDecoder *utf8Decoder = QTextCodec::codecForMib(106)->makeDecoder();

    QString actual;
    for (int i = 0; i < ba.size(); ++i)
        utf8Decoder->toUnicode(&actual, ba.constData() + i, 1);

    QCOMPARE(actual, expected);


    delete utf8Decoder;
}

void tst_QTextCodec::codecForLocale()
{
    QTextCodec *codec = QTextCodec::codecForLocale();
    QVERIFY(codec != 0);

    // The rest of this test is for Unix only
#if defined(Q_OS_UNIX)
    // get a time string that is locale-encoded
    QByteArray originalLocaleEncodedTimeString;
    originalLocaleEncodedTimeString.resize(1024);
    time_t t;
    time(&t);
    int r = strftime(originalLocaleEncodedTimeString.data(),
                     originalLocaleEncodedTimeString.size(),
                     "%A%a%B%b%Z",
                     localtime(&t));
    QVERIFY(r != 0);
    originalLocaleEncodedTimeString.resize(r);

    QString unicodeTimeString = codec->toUnicode(originalLocaleEncodedTimeString);
    QByteArray localeEncodedTimeString = codec->fromUnicode(unicodeTimeString);
    QCOMPARE(localeEncodedTimeString, originalLocaleEncodedTimeString);

    // find a codec that is not the codecForLocale()
    QTextCodec *codec2 = 0;
    const auto availableMibs = QTextCodec::availableMibs();
    for (int mib : availableMibs ) {
        if (mib != codec->mibEnum()) {
            codec2 = QTextCodec::codecForMib(mib);
            if (codec2)
                break;
        }
    }

    // Only run the rest of the test if we could find a codec that is not
    // already the codecForLocale().
    if (codec2) {
        // set it, codecForLocale() should return it now
        QTextCodec::setCodecForLocale(codec2);
        QCOMPARE(QTextCodec::codecForLocale(), codec2);

        // reset back to the default
        QTextCodec::setCodecForLocale(0);
        QCOMPARE(QTextCodec::codecForLocale(), codec);
    }
#endif
}

void tst_QTextCodec::asciiToIscii() const
{
    /* Add all low, 7-bit ASCII characters. */
    QString ascii;
    const int len = 0xA0 - 1;
    ascii.resize(len);

    for(int i = 0; i < len; ++i)
        ascii[i] = QChar(i + 1);

    static const char *const isciiCodecs[] =
    {
        "Iscii-Mlm",
        "Iscii-Knd",
        "Iscii-Tlg",
        "Iscii-Tml",
        "Iscii-Ori",
        "Iscii-Gjr",
        "Iscii-Pnj",
        "Iscii-Bng",
        "Iscii-Dev"
    };
    const int isciiCodecsLen = sizeof(isciiCodecs) / sizeof(const char *);

    for(int i = 0; i < isciiCodecsLen; ++i) {
        /* For each codec. */

        const QTextCodec *const textCodec = QTextCodec::codecForName(isciiCodecs[i]);
        if (!textCodec)
            QSKIP("No ISCII codecs available.");

        for(int i2 = 0; i2 < len; ++i2) {
            /* For each character in ascii. */
            const QChar c(ascii[i2]);
            QVERIFY2(textCodec->canEncode(c), qPrintable(QString::fromLatin1("Failed to encode %1 with encoding %2")
                                                         .arg(QString::number(c.unicode()), QString::fromLatin1(textCodec->name().constData()))));
        }

        QVERIFY2(textCodec->canEncode(ascii), qPrintable(QString::fromLatin1("Failed for full string with encoding %1")
                                                         .arg(QString::fromLatin1(textCodec->name().constData()))));
    }
}

void tst_QTextCodec::nonFlaggedCodepointFFFF() const
{
    //Check that the code point 0xFFFF (=non-character code 0xEFBFBF) is not flagged
    const QChar ch(0xFFFF);
    QString input(ch);

    QTextCodec *const codec = QTextCodec::codecForMib(106); // UTF-8
    QVERIFY(codec);

    const QByteArray asDecoded(codec->fromUnicode(input));
    QCOMPARE(asDecoded, QByteArray("\357\277\277"));

    QByteArray ffff("\357\277\277");
    QTextCodec::ConverterState state(QTextCodec::ConvertInvalidToNull);
    QVERIFY(codec->toUnicode(ffff.constData(), ffff.length(), &state) == QByteArray::fromHex("EFBFBF"));
}

void tst_QTextCodec::flagF7808080() const
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

    QTextCodec *const codec = QTextCodec::codecForMib(106); // UTF-8
    QVERIFY(codec);

    //QVERIFY(!codec->canEncode(QChar(0x1C0000)));

    QTextCodec::ConverterState state(QTextCodec::ConvertInvalidToNull);
    QCOMPARE(codec->toUnicode(input.constData(), input.length(), &state), QString(input.size(), QChar(0)));
}

void tst_QTextCodec::nonFlaggedEFBFBF() const
{
    /* Check that the codec does NOT flag EFBFBF.
     * This is a regression test; see QTBUG-33229
     */
    QByteArray validInput;
    validInput.resize(3);
    validInput[0] = char(0xEF);
    validInput[1] = char(0xBF);
    validInput[2] = char(0xBF);

    const QTextCodec *const codec = QTextCodec::codecForMib(106); // UTF-8
    QVERIFY(codec);

    {
        //QVERIFY(!codec->canEncode(QChar(0xFFFF)));
        QTextCodec::ConverterState state(QTextCodec::ConvertInvalidToNull);
        QVERIFY(codec->toUnicode(validInput.constData(), validInput.length(), &state) == QByteArray::fromHex("EFBFBF"));

        QByteArray start("<?pi ");
        start.append(validInput);
        start.append("?>");
    }

    // Check that 0xEFBFBF is correctly decoded when preceded by an arbitrary character
    {
        QByteArray start("B");
        start.append(validInput);

        QTextCodec::ConverterState state(QTextCodec::ConvertInvalidToNull);
        QVERIFY(codec->toUnicode(start.constData(), start.length(), &state) == QByteArray("B").append(QByteArray::fromHex("EFBFBF")));
    }
}

void tst_QTextCodec::decode0D() const
{
    QByteArray input;
    input.resize(3);
    input[0] = 'A';
    input[1] = '\r';
    input[2] = 'B';

    QCOMPARE(QString::fromUtf8(input.constData()).toUtf8(), input);
}

void tst_QTextCodec::aliasForUTF16() const
{
    QVERIFY(QTextCodec::codecForName("UTF-16")->aliases().isEmpty());
}

void tst_QTextCodec::mibForTSCII() const
{
    QTextCodec *codec = QTextCodec::codecForName("TSCII");
    QVERIFY(codec);
    QCOMPARE(codec->mibEnum(), 2107);
}

void tst_QTextCodec::codecForTSCII() const
{
    QTextCodec *codec = QTextCodec::codecForMib(2107);
    QVERIFY(codec);
    QCOMPARE(codec->mibEnum(), 2107);
}

void tst_QTextCodec::iso8859_16() const
{
    QTextCodec *codec = QTextCodec::codecForName("ISO8859-16");
    QVERIFY(codec);
    QCOMPARE(codec->name(), QByteArray("ISO-8859-16"));
}

static QString fromInvalidUtf8Sequence(const QByteArray &ba)
{
    return QString().fill(QChar::ReplacementCharacter, ba.size());
}

// copied from tst_QString::fromUtf8_data()
void tst_QTextCodec::utf8Codec_data()
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
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.1-1") << utf8 << str << -1;

    // 3.3.2
    utf8.clear();
    utf8 += char(0xe0);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.2-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xe0);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.2-2") << utf8 << str << -1;
    utf8 += 0x30;
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.2-3") << utf8 << str << -1;

    // 3.3.3
    utf8.clear();
    utf8 += char(0xf0);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.3") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.3-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xf0);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.3-2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.3-3") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xf0);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.3-4") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
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
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.4-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xf8);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.4-2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.4-3") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xf8);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.4-4") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.4-5") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xf8);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.4-6") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
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
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfc);
    utf8 += char(0x80);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-3") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfc);
    utf8 += char(0x80);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-4") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-5") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfc);
    utf8 += char(0x80);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-6") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-7") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfc);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-8") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.5-9") << utf8 << str << -1;

    // 3.3.6
    utf8.clear();
    utf8 += char(0xdf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.6") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.6-1") << utf8 << str << -1;

    // 3.3.7
    utf8.clear();
    utf8 += char(0xef);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.7") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.7-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xef);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.7-2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.7-3") << utf8 << str << -1;

    // 3.3.8
    utf8.clear();
    utf8 += char(0xf7);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.8") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.8-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xf7);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.8-2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.8-3") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xf7);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.8-4") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
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
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.9-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfb);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.9-2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.9-3") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfb);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.9-4") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.9-5") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfb);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.9-6") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
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
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-1") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfd);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-2") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-3") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfd);
    utf8 += char(0xbf);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-4") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-5") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfd);
    utf8 += char(0xbf);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-6") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-7") << utf8 << str << -1;

    utf8.clear();
    utf8 += char(0xfd);
    str = fromInvalidUtf8Sequence(utf8);
    QTest::newRow("http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html 3.3.10-8") << utf8 << str << -1;
    utf8 += char(0x30);
    str += 0x30;
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

void tst_QTextCodec::utf8Codec()
{
    QTextCodec *codec = QTextCodec::codecForMib(106); // UTF-8
    QVERIFY(codec != 0);

    QFETCH(QByteArray, utf8);
    QFETCH(QString, res);
    QFETCH(int, len);

    QString str = codec->toUnicode(utf8.isNull() ? 0 : utf8.constData(),
                                   len < 0 ? qstrlen(utf8.constData()) : len);
    QCOMPARE(str, res);

    str = QString::fromUtf8(utf8.isNull() ? 0 : utf8.constData(), len);
    QCOMPARE(str, res);
}

void tst_QTextCodec::utf8bom_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("result");

    QTest::newRow("nobom")
        << QByteArray("\302\240", 2)
        << QString::fromLatin1("\240");

    {
        static const ushort data[] = { 0x201d };
        QTest::newRow("nobom 2")
            << QByteArray("\342\200\235", 3)
            << QString::fromUtf16(data, sizeof(data)/sizeof(short));
    }

    {
        static const ushort data[] = { 0xf000 };
        QTest::newRow("bom1")
            << QByteArray("\357\200\200", 3)
            << QString::fromUtf16(data, sizeof(data)/sizeof(short));
    }

    {
        static const ushort data[] = { 0xfec0 };
        QTest::newRow("bom2")
            << QByteArray("\357\273\200", 3)
            << QString::fromUtf16(data, sizeof(data)/sizeof(short));
    }

    {
        QTest::newRow("normal-bom")
            << QByteArray("\357\273\277a", 4)
            << QString("a");
    }

    { // test the non-SIMD code-path
        static const ushort data[] = { 0x61, 0xfeff, 0x62 };
        QTest::newRow("middle-bom (non SIMD)")
            << QByteArray("a\357\273\277b")
            << QString::fromUtf16(data, sizeof(data)/sizeof(short));
    }

    { // test the SIMD code-path
        static const ushort data[] = { 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0xfeff, 0x6d };
        QTest::newRow("middle-bom (SIMD)")
            << QByteArray("abcdefghijkl\357\273\277m")
            << QString::fromUtf16(data, sizeof(data)/sizeof(short));
    }
}

void tst_QTextCodec::utf8bom()
{
    QFETCH(QByteArray, data);
    QFETCH(QString, result);

    QTextCodec *const codec = QTextCodec::codecForMib(106); // UTF-8
    QVERIFY(codec);

    QCOMPARE(codec->toUnicode(data.constData(), data.length(), 0), result);

    QTextCodec::ConverterState state;
    QCOMPARE(codec->toUnicode(data.constData(), data.length(), &state), result);
}

void tst_QTextCodec::utf8stateful_data()
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

    // invalid: sequence too short (the empty second buffer causes a state reset)
    QTest::newRow("1of2+empty") << QByteArray("\xc2") << QByteArray() << QString();
    QTest::newRow("1of3+empty") << QByteArray("\xe0") << QByteArray() << QString();
    QTest::newRow("2of3+empty") << QByteArray("\xe0\xa0") << QByteArray() << QString();
    QTest::newRow("1of4+empty") << QByteArray("\360") << QByteArray() << QString();
    QTest::newRow("2of4+empty") << QByteArray("\360\220") << QByteArray() << QString();
    QTest::newRow("3of4+empty") << QByteArray("\360\220\210") << QByteArray() << QString();

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

void tst_QTextCodec::utf8stateful()
{
    QFETCH(QByteArray, buffer1);
    QFETCH(QByteArray, buffer2);
    QFETCH(QString, result);

    QTextCodec *utf8codec = QTextCodec::codecForName("utf-8");
    QVERIFY(utf8codec);

    QTextCodec::ConverterState state;
    memset(&state, 0, sizeof state);

    QString decoded1 = utf8codec->toUnicode(buffer1, buffer1.size(), &state);
    if (result.isNull()) {
        // the decoder may have found an early error (invalidChars > 0):
        // if it has, remainingChars == 0;
        // if it hasn't, then it must have a state
        QVERIFY2((state.remainingChars == 0) != (state.invalidChars == 0),
                 "remainingChars = " + QByteArray::number(state.remainingChars) +
                 "; invalidChars = " + QByteArray::number(state.invalidChars));
    } else {
        QVERIFY(state.remainingChars > 0);
        QCOMPARE(state.invalidChars, 0);
    }

    QString decoded2 = utf8codec->toUnicode(buffer2, buffer2.size(), &state);
    QCOMPARE(state.remainingChars, 0);
    if (result.isNull()) {
        QVERIFY(state.invalidChars > 0);
    } else {
        QCOMPARE(decoded1 + decoded2, result);
    }
}

void tst_QTextCodec::utfHeaders_data()
{
    QTest::addColumn<QByteArray>("codecName");
    QTest::addColumn<int>("flags");
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<QString>("unicode");
    QTest::addColumn<bool>("toUnicode");

    QTest::newRow("utf8 bom")
        << QByteArray("UTF-8")
        << 0
        << QByteArray("\xef\xbb\xbfhello")
        << QString::fromLatin1("hello")
        << true;
    QTest::newRow("utf8 nobom")
        << QByteArray("UTF-8")
        << 0
        << QByteArray("hello")
        << QString::fromLatin1("hello")
        << true;
    QTest::newRow("utf8 bom ignore header")
        << QByteArray("UTF-8")
        << (int)QTextCodec::IgnoreHeader
        << QByteArray("\xef\xbb\xbfhello")
        << (QString(QChar(0xfeff)) + QString::fromLatin1("hello"))
        << true;
    QTest::newRow("utf8 nobom ignore header")
        << QByteArray("UTF-8")
        << (int)QTextCodec::IgnoreHeader
        << QByteArray("hello")
        << QString::fromLatin1("hello")
        << true;

    QTest::newRow("utf16 bom be")
        << QByteArray("UTF-16")
        << 0
        << QByteArray("\xfe\xff\0h\0e\0l", 8)
        << QString::fromLatin1("hel")
        << true;
    QTest::newRow("utf16 bom le")
        << QByteArray("UTF-16")
        << 0
        << QByteArray("\xff\xfeh\0e\0l\0", 8)
        << QString::fromLatin1("hel")
        << true;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        QTest::newRow("utf16 nobom")
            << QByteArray("UTF-16")
            << 0
            << QByteArray("\0h\0e\0l", 6)
            << QString::fromLatin1("hel")
            << true;
        QTest::newRow("utf16 bom be ignore header")
            << QByteArray("UTF-16")
            << (int)QTextCodec::IgnoreHeader
            << QByteArray("\xfe\xff\0h\0e\0l", 8)
            << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"))
            << true;
    } else {
        QTest::newRow("utf16 nobom")
            << QByteArray("UTF-16")
            << 0
            << QByteArray("h\0e\0l\0", 6)
            << QString::fromLatin1("hel")
            << true;
        QTest::newRow("utf16 bom le ignore header")
            << QByteArray("UTF-16")
            << (int)QTextCodec::IgnoreHeader
            << QByteArray("\xff\xfeh\0e\0l\0", 8)
            << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"))
            << true;
    }

    QTest::newRow("utf16-be bom be")
        << QByteArray("UTF-16BE")
        << 0
        << QByteArray("\xfe\xff\0h\0e\0l", 8)
        << QString::fromLatin1("hel")
        << true;
    QTest::newRow("utf16-be nobom")
        << QByteArray("UTF-16BE")
        << 0
        << QByteArray("\0h\0e\0l", 6)
        << QString::fromLatin1("hel")
        << true;
    QTest::newRow("utf16-be bom be ignore header")
        << QByteArray("UTF-16BE")
        << (int)QTextCodec::IgnoreHeader
        << QByteArray("\xfe\xff\0h\0e\0l", 8)
        << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"))
        << true;

    QTest::newRow("utf16-le bom le")
        << QByteArray("UTF-16LE")
        << 0
        << QByteArray("\xff\xfeh\0e\0l\0", 8)
        << QString::fromLatin1("hel")
        << true;
    QTest::newRow("utf16-le nobom")
        << QByteArray("UTF-16LE")
        << 0
        << QByteArray("h\0e\0l\0", 6)
        << QString::fromLatin1("hel")
        << true;
    QTest::newRow("utf16-le bom le ignore header")
        << QByteArray("UTF-16LE")
        << (int)QTextCodec::IgnoreHeader
        << QByteArray("\xff\xfeh\0e\0l\0", 8)
        << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"))
        << true;


    QTest::newRow("utf32 bom be")
        << QByteArray("UTF-32")
        << 0
        << QByteArray("\0\0\xfe\xff\0\0\0h\0\0\0e\0\0\0l", 16)
        << QString::fromLatin1("hel")
        << true;
    QTest::newRow("utf32 bom le")
        << QByteArray("UTF-32")
        << 0
        << QByteArray("\xff\xfe\0\0h\0\0\0e\0\0\0l\0\0\0", 16)
        << QString::fromLatin1("hel")
        << true;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        QTest::newRow("utf32 nobom")
            << QByteArray("UTF-32")
            << 0
            << QByteArray("\0\0\0h\0\0\0e\0\0\0l", 12)
            << QString::fromLatin1("hel")
            << true;
        QTest::newRow("utf32 bom be ignore header")
            << QByteArray("UTF-32")
            << (int)QTextCodec::IgnoreHeader
            << QByteArray("\0\0\xfe\xff\0\0\0h\0\0\0e\0\0\0l", 16)
            << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"))
            << true;
    } else {
        QTest::newRow("utf32 nobom")
            << QByteArray("UTF-32")
            << 0
            << QByteArray("h\0\0\0e\0\0\0l\0\0\0", 12)
            << QString::fromLatin1("hel")
            << true;
        QTest::newRow("utf32 bom le ignore header")
            << QByteArray("UTF-32")
            << (int)QTextCodec::IgnoreHeader
            << QByteArray("\xff\xfe\0\0h\0\0\0e\0\0\0l\0\0\0", 16)
            << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"))
            << true;
    }


    QTest::newRow("utf32-be bom be")
        << QByteArray("UTF-32BE")
        << 0
        << QByteArray("\0\0\xfe\xff\0\0\0h\0\0\0e\0\0\0l", 16)
        << QString::fromLatin1("hel")
        << true;
    QTest::newRow("utf32-be nobom")
        << QByteArray("UTF-32BE")
        << 0
        << QByteArray("\0\0\0h\0\0\0e\0\0\0l", 12)
        << QString::fromLatin1("hel")
        << true;
    QTest::newRow("utf32-be bom be ignore header")
        << QByteArray("UTF-32BE")
        << (int)QTextCodec::IgnoreHeader
        << QByteArray("\0\0\xfe\xff\0\0\0h\0\0\0e\0\0\0l", 16)
        << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"))
        << true;


    QTest::newRow("utf32-le bom le")
        << QByteArray("UTF-32LE")
        << 0
        << QByteArray("\xff\xfe\0\0h\0\0\0e\0\0\0l\0\0\0", 16)
        << QString::fromLatin1("hel")
        << true;
    QTest::newRow("utf32-le nobom")
        << QByteArray("UTF-32LE")
        << 0
        << QByteArray("h\0\0\0e\0\0\0l\0\0\0", 12)
        << QString::fromLatin1("hel")
        << true;
    QTest::newRow("utf32-le bom le ignore header")
        << QByteArray("UTF-32LE")
        << (int)QTextCodec::IgnoreHeader
        << QByteArray("\xff\xfe\0\0h\0\0\0e\0\0\0l\0\0\0", 16)
        << (QString(QChar(0xfeff)) + QString::fromLatin1("hel"))
        << true;
}

void tst_QTextCodec::utfHeaders()
{
    QFETCH(QByteArray, codecName);
    QTextCodec *codec = QTextCodec::codecForName(codecName);
    QVERIFY(codec != 0);

    QFETCH(int, flags);
    QTextCodec::ConversionFlags cFlags = QTextCodec::ConversionFlags(flags);
    QTextCodec::ConverterState state(cFlags);

    QFETCH(QByteArray, encoded);
    QFETCH(QString, unicode);

    QFETCH(bool, toUnicode);

    QLatin1String ignoreReverseTestOn = (QSysInfo::ByteOrder == QSysInfo::BigEndian) ? QLatin1String(" le") : QLatin1String(" be");
    QString rowName(QTest::currentDataTag());

    if (toUnicode) {
        QString result = codec->toUnicode(encoded.constData(), encoded.length(), &state);
        QCOMPARE(result.length(), unicode.length());
        QCOMPARE(result, unicode);

        if (!rowName.endsWith("nobom") && !rowName.contains(ignoreReverseTestOn)) {
            QTextCodec::ConverterState state2(cFlags);
            QByteArray reencoded = codec->fromUnicode(unicode.unicode(), unicode.length(), &state2);
            QCOMPARE(reencoded, encoded);
        }
    } else {
        QByteArray result = codec->fromUnicode(unicode.unicode(), unicode.length(), &state);
        QCOMPARE(result, encoded);
    }
}

void tst_QTextCodec::codecForHtml_data()
{
    QTest::addColumn<QByteArray>("html");
    QTest::addColumn<int>("defaultCodecMib");
    QTest::addColumn<int>("expectedMibEnum");

    int noDefault = -1;
    int fallback = 4; // latin 1
    QByteArray html = "<html><head></head><body>blah</body></html>";
    QTest::newRow("no charset, latin 1") << html << noDefault << fallback;

    QTest::newRow("no charset, default UTF-8") << html << 106 << 106;

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-15\" /></head></html>";
    QTest::newRow("latin 15, default UTF-8") << html << 106 << 111;

    html = "<html><head><meta content=\"text/html; charset=ISO-8859-15\" http-equiv=\"content-type\" /></head></html>";
    QTest::newRow("latin 15, default UTF-8 (#2)") << html << 106 << 111;

    html = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><title>Test</title></head>";
    QTest::newRow("UTF-8, no default") << html << noDefault << 106;

    html = "<!DOCTYPE html><html><head><meta charset=\"ISO_8859-1:1987\"><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><title>Test</title></head>";
    QTest::newRow("latin 1, no default") << html << noDefault << 4;

    html = "<!DOCTYPE html><html><head><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><meta charset=\"utf-8\"><title>Test</title></head>";
    QTest::newRow("UTF-8, no default (#2)") << html << noDefault << 106;

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8/></head></html>";
    QTest::newRow("UTF-8, no quotes") << html << noDefault << 106;

    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset='UTF-8'/></head></html>";
    QTest::newRow("UTF-8, single quotes") << html << noDefault << 106;

    html = "<!DOCTYPE html><html><head><meta charset=utf-8><title>Test</title></head>";
    QTest::newRow("UTF-8, > terminator") << html << noDefault << 106;

    html = "<!DOCTYPE html><html><head><meta charset= utf-8 ><title>Test</title></head>";
    QTest::newRow("UTF-8, > terminator with spaces") << html << noDefault << 106;

    html = "<!DOCTYPE html><html><head><meta charset= utf/8 ><title>Test</title></head>";
    QTest::newRow("UTF-8, > teminator with early backslash)") << html << noDefault << 106;

    // Test invalid charsets.
    html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=invalid-foo\" /></head></html>";
    QTest::newRow("invalid charset, no default") << html << noDefault << fallback;
    QTest::newRow("invalid charset, default UTF-8") << html << 106 << 106;

    html = "<!DOCTYPE html><html><head><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><meta charset=\"";
    html.prepend(QByteArray().fill(' ', 512 - html.size()));
    QTest::newRow("invalid charset (large header)") << html << noDefault << fallback;

    html = "<!DOCTYPE html><html><head><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><meta charset=\"utf-8";
    QTest::newRow("invalid charset (no closing double quote)") << html << noDefault << fallback;

    html = "<!DOCTYPE html><html><head><meta http-equiv=\"X-UA-Compatible\" content=\"IE=9,chrome=1\"><meta charset='utf-8";
    QTest::newRow("invalid charset (no closing single quote)") << html << noDefault << fallback;

    html = "<!DOCTYPE html><html><head><meta charset=utf-8 foo=bar><title>Test</title></head>";
    QTest::newRow("invalid (space terminator)") << html << noDefault << fallback;

    html = "<!DOCTYPE html><html><head><meta charset=\" utf' 8 /><title>Test</title></head>";
    QTest::newRow("invalid charset, early terminator (')") << html << noDefault << fallback;

    const char src[] = { char(0xff), char(0xfe), char(0x7a), char(0x03), 0, 0 };
    html = src;
    QTest::newRow("greek text UTF-16LE") << html << 106 << 1014;

    html = "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><span style=\"color: rgb(0, 0, 0); font-family: "
        "'Galatia SIL'; font-size: 27px; font-style: normal; font-variant: normal; font-weight: normal; letter-spacing: normal; "
        "line-height: normal; orphans: auto; text-align: start; text-indent: 0px; text-transform: none; white-space: normal; widows: "
        "auto; word-spacing: 0px; -webkit-text-size-adjust: auto; -webkit-text-stroke-width: 0px; display: inline !important; float: "
        "none;\">&#x37b</span>\000";
    QTest::newRow("greek text UTF-8") << html << 106 << 106;

    html = "<!DOCTYPE html><html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=unicode\">"
            "<head/><body><p>bla</p></body></html>"; // QTBUG-41998, ICU will return UTF-16.
    QTest::newRow("legacy unicode UTF-8") << html << 106 << 106;
}

void tst_QTextCodec::codecForHtml()
{
    QFETCH(QByteArray, html);
    QFETCH(int, defaultCodecMib);
    QFETCH(int, expectedMibEnum);

    if (defaultCodecMib != -1)
        QCOMPARE(QTextCodec::codecForHtml(html, QTextCodec::codecForMib(defaultCodecMib))->mibEnum(), expectedMibEnum);
    else // Test one parameter version when there is no default codec.
        QCOMPARE(QTextCodec::codecForHtml(html)->mibEnum(), expectedMibEnum);
}

void tst_QTextCodec::codecForUtfText_data()
{
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<bool>("detected");
    QTest::addColumn<int>("mib");


    QTest::newRow("utf8 bom")
        << QByteArray("\xef\xbb\xbfhello")
        << true
        << 106;
    QTest::newRow("utf8 nobom")
        << QByteArray("hello")
        << false
        << 0;

    QTest::newRow("utf16 bom be")
        << QByteArray("\xfe\xff\0h\0e\0l", 8)
        << true
        << 1013;
    QTest::newRow("utf16 bom le")
        << QByteArray("\xff\xfeh\0e\0l\0", 8)
        << true
        << 1014;
    QTest::newRow("utf16 nobom")
        << QByteArray("\0h\0e\0l", 6)
        << false
        << 0;

    QTest::newRow("utf32 bom be")
        << QByteArray("\0\0\xfe\xff\0\0\0h\0\0\0e\0\0\0l", 16)
        << true
        << 1018;
    QTest::newRow("utf32 bom le")
        << QByteArray("\xff\xfe\0\0h\0\0\0e\0\0\0l\0\0\0", 16)
        << true
        << 1019;
    QTest::newRow("utf32 nobom")
        << QByteArray("\0\0\0h\0\0\0e\0\0\0l", 12)
        << false
        << 0;
}

void tst_QTextCodec::codecForUtfText()
{
    QFETCH(QByteArray, encoded);
    QFETCH(bool, detected);
    QFETCH(int, mib);

    QTextCodec *codec = QTextCodec::codecForUtfText(encoded, 0);
    if (detected)
        QCOMPARE(codec->mibEnum(), mib);
    else
        QVERIFY(!codec);
}

#if defined(Q_OS_UNIX)
void tst_QTextCodec::toLocal8Bit()
{
#ifdef QT_NO_PROCESS
    QSKIP("No qprocess support", SkipAll);
#else
    QProcess process;
    process.start("echo/echo");
    QString string(QChar(0x410));
    process.write((const char*)string.utf16(), string.length()*2);

    process.closeWriteChannel();
    process.waitForFinished();
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
#endif
}
#endif

class LoadAndConvert: public QRunnable
{
public:
    LoadAndConvert(const QByteArray &source, QByteArray *destination)
        : codecName(source), target(destination)
    {}
    QByteArray codecName;
    QByteArray *target;
    void run()
    {
        QTextCodec *c = QTextCodec::codecForName(codecName);
        if (!c) {
            qWarning() << "WARNING" << codecName << "not found?";
            return;
        }
        QString str = QString::fromLatin1(codecName);
        QByteArray b = c->fromUnicode(str);
        c->toUnicode(b);
        *target = codecName;
    }
};

class LoadAndConvertMIB: public QRunnable
{
public:
    LoadAndConvertMIB(int mib, int *target)
        : mib(mib), target(target)
    {}
    int mib;
    int *target;
    void run()
    {
        QTextCodec *c = QTextCodec::codecForMib(mib);
        if (!c) {
            qWarning() << "WARNING" << mib << "not found?";
            return;
        }
        QString str = QString::number(mib);
        QByteArray b = c->fromUnicode(str);
        c->toUnicode(b);
        *target = mib;
    }
};


void tst_QTextCodec::threadSafety()
{
    QList<QByteArray> codecList = QTextCodec::availableCodecs();
    const QVector<int> mibList = QTextCodec::availableMibs().toVector();
    QThreadPool::globalInstance()->setMaxThreadCount(12);

    QVector<QByteArray> res;
    res.resize(codecList.size());
    for (int i = 0; i < codecList.size(); ++i) {
        QThreadPool::globalInstance()->start(new LoadAndConvert(codecList.at(i), &res[i]));
    }

    QVector<int> res2;
    res2.resize(mibList.size());
    for (int i = 0; i < mibList.size(); ++i) {
        QThreadPool::globalInstance()->start(new LoadAndConvertMIB(mibList.at(i), &res2[i]));
    }

    // wait for all threads to finish working
    QThreadPool::globalInstance()->waitForDone();

    QCOMPARE(res.toList(), codecList);
    QCOMPARE(res2, mibList);
}

void tst_QTextCodec::invalidNames()
{
    QVERIFY(!QTextCodec::codecForName(""));
    QVERIFY(!QTextCodec::codecForName(QByteArray()));
    QVERIFY(!QTextCodec::codecForName("-"));
    QVERIFY(!QTextCodec::codecForName("\1a\2b\3a\4d\5c\6s\7a\xffr\xec_\x9c_"));
    QVERIFY(!QTextCodec::codecForName("\n"));
    QVERIFY(!QTextCodec::codecForName("don't exist"));
    QByteArray huge = "azertyuiop^$qsdfghjklm<wxcvbn,;:=1234567890�_";
    huge = huge + huge + huge + huge + huge + huge + huge + huge;
    huge = huge + huge + huge + huge + huge + huge + huge + huge;
    huge = huge + huge + huge + huge + huge + huge + huge + huge;
    huge = huge + huge + huge + huge + huge + huge + huge + huge;
    QVERIFY(!QTextCodec::codecForName(huge));
}

void tst_QTextCodec::checkAliases_data()
{
    QTest::addColumn<QByteArray>("codecName");
    const QList<QByteArray> codecList = QTextCodec::availableCodecs();
    for (const QByteArray &a : codecList)
        QTest::newRow( a.constData() ) << a;
}

void tst_QTextCodec::checkAliases()
{
    QFETCH( QByteArray, codecName );
    QTextCodec *c = QTextCodec::codecForName(codecName);
    QVERIFY(c);
    QCOMPARE(QTextCodec::codecForName(codecName), c);
    QCOMPARE(QTextCodec::codecForName(c->name()), c);

    const auto aliases = c->aliases();
    for (const QByteArray &a : aliases) {
        QCOMPARE(QTextCodec::codecForName(a), c);
    }
}


void tst_QTextCodec::moreToFromUnicode_data() {
    QTest::addColumn<QByteArray>("codecName");
    QTest::addColumn<QByteArray>("testData");

    QTest::newRow("russian") << QByteArray("ISO-8859-5")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF\x00");

    QTest::newRow("arabic") << QByteArray("ISO-8859-6")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA4\xAC\xAD\xBB\xBF\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2");

    QTest::newRow("greek") << QByteArray("ISO-8859-7")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA6\xA7\xA8\xA9\xAB\xAC\xAD\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE");

    QTest::newRow("turkish") << QByteArray("ISO-8859-9")
        << QByteArray("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QTest::newRow("latin1") << QByteArray("ISO-8859-1")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QByteArray sms7bit_ba;
    for (int i=1; i <= 0x7f; ++i) {
        if (i!='\x1b') {
            sms7bit_ba.append(i);
        }
    }

    QTest::newRow("latin2") << QByteArray("ISO-8859-2")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QTest::newRow("latin3") << QByteArray("ISO-8859-3")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBF\xC0\xC1\xC2\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QTest::newRow("latin4") << QByteArray("ISO-8859-4")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QTest::newRow("russian 2") << QByteArray("ISO-8859-5")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QTest::newRow("arabic 2") << QByteArray("ISO-8859-6")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA4\xAC\xAD\xBB\xBF\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2");

    QTest::newRow("greek 2") << QByteArray("ISO-8859-7")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA6\xA7\xA8\xA9\xAB\xAC\xAD\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE");

    QTest::newRow("latin5") << QByteArray("ISO-8859-9")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QTest::newRow("latin6") << QByteArray("ISO-8859-10")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

#if 0
    QByteArray iso8859_11_ba;
    for (int x=0x20; x<=0x7f; ++x) {
        iso8859_11_ba.append(x);
    }
    for (int x=0xa0; x<0xff; ++x) {
        if ((x>=0xdb && x<0xdf) || x>0xfb){
            continue;
        }
        iso8859_11_ba.append(x);
    }
    QTest::newRow("latin-thai") << QByteArray("ISO-8859-11") << iso8859_11_ba;
#endif

    QTest::newRow("latin7") << QByteArray("ISO-8859-13")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QTest::newRow("celtic") << QByteArray("ISO-8859-14")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QTest::newRow("latin9") << QByteArray("ISO-8859-15")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

//    QTest::newRow("latin10") << QByteArray("ISO-8859-16")
//        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QTest::newRow("cp850") << QByteArray("CP850")
        << QByteArray("\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x7e\x7f\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff");

    QTest::newRow("cp874") << QByteArray("CP874")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x85\x91\x92\x93\x94\x95\x96\x97\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB");

    QTest::newRow("cp1250") << QByteArray("CP1250")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x82\x84\x85\x86\x87\x89\x8A\x8B\x8C\x8D\x8E\x8F\x91\x92\x93\x94\x95\x96\x97\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QTest::newRow("cp1251") << QByteArray("CP1251")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QTest::newRow("cp1252") << QByteArray("CP1252")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8E\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QTest::newRow("cp1253") << QByteArray("CP1253")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x82\x83\x84\x85\x86\x87\x89\x8B\x91\x92\x93\x94\x95\x96\x97\x99\x9B\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE");

    QTest::newRow("cp1254") << QByteArray("CP1254")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QTest::newRow("cp1255") << QByteArray("CP1255")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x82\x83\x84\x85\x86\x87\x88\x89,x8B\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9B\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFD\xFE");

    QTest::newRow("cp1256") << QByteArray("CP1256")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QTest::newRow("cp1257") << QByteArray("CP1257")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x82\x84\x85\x86\x87\x89\x8B\x8D\x8E\x8F\x91\x92\x93\x94\x95\x96\x97\x99\x9B\x9D\x9E\xA0\xA2\xA3\xA4\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QTest::newRow("cp1258") << QByteArray("CP1258")
        << QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80\x82\x83\x84\x85\x86\x87\x88\x89\x8B\x8C\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9B\x9C\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

    QByteArray koi8_r_ba;
    for (int x=0x20; x<=0xff; ++x) {
        if (x!=0x9A && x!=0xbf) {
            koi8_r_ba.append(x);
        }
    }
    QTest::newRow("KOI8-R") << QByteArray("KOI8-R") << koi8_r_ba;

    QByteArray koi8_u_ba;
    for (int x=0x20; x<=0xff; ++x) {
        koi8_u_ba.append(x);
    }
    QTest::newRow("KOI8-U") << QByteArray("KOI8-U") << koi8_u_ba;


    QByteArray big5_ba;
    for (unsigned char u=0xa1; u<=0xf9; u++) {
        if (u==0xc8) {
            continue;
        }
        for (unsigned char v=0x40; v<=0x7e; v++) {
            big5_ba.append(u);
            big5_ba.append(v);
        }
        unsigned char v_up;
        switch (u) {
            case 0xa2: v_up=0xa1; break;
            case 0xa3: v_up=0xbf; break;
            case 0xc7: v_up=0xfc; break;
            case 0xf9: v_up=0xd5; break;
            default: v_up=0xfe;
        }

        for (unsigned char v=0xa1; v<=v_up; v++) {
            if (u==0xa2 && (v==0xcc || v==0xce)) {
                continue;
            }
            big5_ba.append(u);
            big5_ba.append(v);
        }
    }

    QTest::newRow("BIG5") << QByteArray("BIG5") << big5_ba;

    QByteArray gb2312_ba;
    for (unsigned char u=0xa1; u<=0xf7; u++) {
        for (unsigned char v=0xa1; v<=0xfe; v++) {
            gb2312_ba.append(u);
            gb2312_ba.append(v);
        }
    }

    QTest::newRow("GB2312") << QByteArray("GB2312") << gb2312_ba;
}

void tst_QTextCodec::moreToFromUnicode()
{
    QFETCH( QByteArray, codecName );
    QFETCH( QByteArray, testData );

    QTextCodec *c = QTextCodec::codecForName( codecName.data() );
    QVERIFY(c);

    QString uStr = c->toUnicode(testData);
    QByteArray cStr = c->fromUnicode(uStr);
    QCOMPARE(testData, cStr);
}

void tst_QTextCodec::shiftJis()
{
    QByteArray backslashTilde("\\~");
    QTextCodec* codec = QTextCodec::codecForName("shift_jis");
    QString string = codec->toUnicode(backslashTilde);
    QCOMPARE(string.length(), 2);
    QCOMPARE(string.at(0), QChar(QLatin1Char('\\')));
    QCOMPARE(string.at(1), QChar(QLatin1Char('~')));

    QByteArray encoded = codec->fromUnicode(string);
    QCOMPARE(encoded, backslashTilde);
}

struct UserCodec : public QTextCodec
{
    // implement pure virtuals
    QByteArray name() const Q_DECL_OVERRIDE
    { return "UserCodec"; }
    QList<QByteArray> aliases() const Q_DECL_OVERRIDE
    { return QList<QByteArray>() << "usercodec" << "user-codec"; }
    int mibEnum() const Q_DECL_OVERRIDE
    { return 5000; }

    virtual QString convertToUnicode(const char *, int, ConverterState *) const Q_DECL_OVERRIDE
    { return QString(); }
    virtual QByteArray convertFromUnicode(const QChar *, int, ConverterState *) const Q_DECL_OVERRIDE
    { return QByteArray(); }
};

void tst_QTextCodec::userCodec()
{
    // check that it isn't there
    static bool executedOnce = false;
    if (executedOnce)
        QSKIP("Test already executed once");

    QVERIFY(!QTextCodec::availableCodecs().contains("UserCodec"));
    QVERIFY(!QTextCodec::codecForName("UserCodec"));

    QTextCodec *codec = new UserCodec;
    executedOnce = true;

    QList<QByteArray> availableCodecs = QTextCodec::availableCodecs();
    QVERIFY(availableCodecs.contains("UserCodec"));
    QVERIFY(availableCodecs.contains("usercodec"));
    QVERIFY(availableCodecs.contains("user-codec"));

    QTextCodec *pcodec = QTextCodec::codecForName("UserCodec");
    QCOMPARE(pcodec, codec);

    pcodec = QTextCodec::codecForName("user-codec");
    QCOMPARE(pcodec, codec);

    pcodec = QTextCodec::codecForName("User-Codec");
    QCOMPARE(pcodec, codec);

    pcodec = QTextCodec::codecForMib(5000);
    QCOMPARE(pcodec, codec);
}

struct DontCrashAtExit {
    ~DontCrashAtExit() {
        QTextCodec *c = QTextCodec::codecForName("utf8");
        if (c)
            c->toUnicode("azerty");

    }
} dontCrashAtExit;


QTEST_MAIN(tst_QTextCodec)
#include "tst_qtextcodec.moc"
