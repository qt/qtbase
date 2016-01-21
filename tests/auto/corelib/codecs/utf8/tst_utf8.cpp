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
#include <qsharedpointer.h>

static const char utf8bom[] = "\xEF\xBB\xBF";

class tst_Utf8 : public QObject
{
    Q_OBJECT

public:
    // test data:
    QTextCodec *codec;
    QString (*from8BitPtr)(const char *, int);
    static QByteArray to8Bit(const QString &);

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
    } else {
        codec = QTextCodec::codecForMib(106);
        from8BitPtr = &QString::fromUtf8;
    }
}

QByteArray tst_Utf8::to8Bit(const QString &s)
{
    QFETCH_GLOBAL(bool, useLocale);
    if (useLocale)
        return s.toLocal8Bit();
    else
        return s.toUtf8();
}

void tst_Utf8::roundTrip_data()
{
    QTest::addColumn<QByteArray>("utf8");
    QTest::addColumn<QString>("utf16");

    QTest::newRow("empty") << QByteArray() << QString();
    QTest::newRow("nul") << QByteArray("", 1) << QString(QChar(QChar::Null));

    static const char ascii[] = "This is a standard US-ASCII message";
    QTest::newRow("ascii") << QByteArray(ascii) << QString::fromLatin1(ascii);

    static const char ascii2[] = "\1This\2is\3an\4US-ASCII\020 message interspersed with control chars";
    QTest::newRow("ascii2") << QByteArray(ascii2) << QString::fromLatin1(ascii2);

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

    QCOMPARE(to8Bit(utf16), utf8);
    QCOMPARE(from8Bit(utf8), utf16);

    QCOMPARE(to8Bit(from8Bit(utf8)), utf8);
    QCOMPARE(from8Bit(to8Bit(utf16)), utf16);

    // repeat with a longer message
    utf8.prepend("12345678901234");
    utf16.prepend(QLatin1String("12345678901234"));
    QCOMPARE(to8Bit(utf16), utf8);
    QCOMPARE(from8Bit(utf8), utf16);

    QCOMPARE(to8Bit(from8Bit(utf8)), utf8);
    QCOMPARE(from8Bit(to8Bit(utf16)), utf16);
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
            encoded = encoded.mid(int(strlen(utf8bom)));
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

    extern void loadInvalidUtf8Rows();
    loadInvalidUtf8Rows();
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

    // Unicode has a couple of "non-characters" that one can use internally
    // These characters may be used for interchange;
    // see: http://www.unicode.org/versions/corrigendum9.html
    //
    // Those are the last two entries each Unicode Plane (U+FFFE, U+FFFF,
    // U+1FFFE, U+1FFFF, etc.) as well as the entries between U+FDD0 and
    // U+FDEF (inclusive)

    // U+FDD0 through U+FDEF
    for (int i = 0; i < 32; ++i) {
        char utf8[] = { char(0357), char(0267), char(0220 + i), 0 };
        QString utf16 = QChar(0xfdd0 + i);
        QTest::newRow(qPrintable(QString::number(0xfdd0 + i, 16))) << QByteArray(utf8) << utf16;
    }

    // the last two in Planes 1 through 16
    for (uint plane = 1; plane <= 16; ++plane) {
        for (uint lower = 0xfffe; lower < 0x10000; ++lower) {
            uint ucs4 = (plane << 16) | lower;
            char utf8[] = { char(0xf0 | uchar(ucs4 >> 18)),
                            char(0x80 | (uchar(ucs4 >> 12) & 0x3f)),
                            char(0x80 | (uchar(ucs4 >> 6) & 0x3f)),
                            char(0x80 | (uchar(ucs4) & 0x3f)),
                            0 };
            ushort utf16[] = { QChar::highSurrogate(ucs4), QChar::lowSurrogate(ucs4), 0 };

            QTest::newRow(qPrintable(QString::number(ucs4, 16))) << QByteArray(utf8) << QString::fromUtf16(utf16);
        }
    }

    QTest::newRow("fffe") << QByteArray("\xEF\xBF\xBE") << QString(QChar(0xfffe));
    QTest::newRow("ffff") << QByteArray("\xEF\xBF\xBF") << QString(QChar(0xffff));

    extern void loadNonCharactersRows();
    loadNonCharactersRows();
}

void tst_Utf8::nonCharacters()
{
    QFETCH(QByteArray, utf8);
    QFETCH(QString, utf16);
    QFETCH_GLOBAL(bool, useLocale);

    QSharedPointer<QTextDecoder> decoder = QSharedPointer<QTextDecoder>(codec->makeDecoder());
    decoder->toUnicode(utf8);

    // Only enforce correctness on our UTF-8 decoder
    if (!useLocale)
        QVERIFY(!decoder->hasFailure());
    else if (decoder->hasFailure())
        qWarning("System codec reports failure when it shouldn't. Should report bug upstream.");

    QSharedPointer<QTextEncoder> encoder(codec->makeEncoder());
    encoder->fromUnicode(utf16);
    if (!useLocale)
        QVERIFY(!encoder->hasFailure());
    else if (encoder->hasFailure())
        qWarning("System codec reports failure when it shouldn't. Should report bug upstream.");
}

QTEST_MAIN(tst_Utf8)
#include "tst_utf8.moc"
