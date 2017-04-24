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

#include <QtCore/QUrl>
#include <QtTest/QtTest>

#include "private/qtldurl_p.h"
#include "private/qurl_p.h"

// For testsuites
#define IDNA_ACE_PREFIX "xn--"
#define IDNA_SUCCESS 1
#define STRINGPREP_NO_UNASSIGNED 1
#define STRINGPREP_CONTAINS_UNASSIGNED 2
#define STRINGPREP_CONTAINS_PROHIBITED 3
#define STRINGPREP_BIDI_BOTH_L_AND_RAL 4
#define STRINGPREP_BIDI_LEADTRAIL_NOT_RAL 5

struct ushortarray {
    ushortarray() {}
    template <size_t N>
    ushortarray(unsigned short (&array)[N])
    {
        memcpy(points, array, N*sizeof(unsigned short));
    }

    unsigned short points[100];
};

Q_DECLARE_METATYPE(ushortarray)
Q_DECLARE_METATYPE(QUrl::FormattingOptions)
Q_DECLARE_METATYPE(QUrl::ComponentFormattingOptions)

class tst_QUrlInternal : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // IDNA internals
#ifdef QT_BUILD_INTERNAL
    void idna_testsuite_data();
    void idna_testsuite();
    void nameprep_testsuite_data();
    void nameprep_testsuite();
    void nameprep_highcodes_data();
    void nameprep_highcodes();
#endif
    void ace_testsuite_data();
    void ace_testsuite();
    void std3violations_data();
    void std3violations();
    void std3deviations_data();
    void std3deviations();

    // percent-encoding internals
    void correctEncodedMistakes_data();
    void correctEncodedMistakes();
    void encodingRecode_data();
    void encodingRecode();
    void encodingRecodeInvalidUtf8_data();
    void encodingRecodeInvalidUtf8();
    void recodeByteArray_data();
    void recodeByteArray();
};
#include "tst_qurlinternal.moc"

#ifdef QT_BUILD_INTERNAL
void tst_QUrlInternal::idna_testsuite_data()
{
    QTest::addColumn<int>("numchars");
    QTest::addColumn<ushortarray>("unicode");
    QTest::addColumn<QByteArray>("punycode");
    QTest::addColumn<int>("allowunassigned");
    QTest::addColumn<int>("usestd3asciirules");
    QTest::addColumn<int>("toasciirc");
    QTest::addColumn<int>("tounicoderc");

    unsigned short d1[] = { 0x0644, 0x064A, 0x0647, 0x0645, 0x0627, 0x0628, 0x062A, 0x0643,
                            0x0644, 0x0645, 0x0648, 0x0634, 0x0639, 0x0631, 0x0628, 0x064A,
                            0x061F };
    QTest::newRow("Arabic (Egyptian)") << 17 << ushortarray(d1)
                                    << QByteArray(IDNA_ACE_PREFIX "egbpdaj6bu4bxfgehfvwxn")
                                    << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d2[] = { 0x4ED6, 0x4EEC, 0x4E3A, 0x4EC0, 0x4E48, 0x4E0D, 0x8BF4, 0x4E2D,
                            0x6587 };
    QTest::newRow("Chinese (simplified)") << 9 << ushortarray(d2)
                                       << QByteArray(IDNA_ACE_PREFIX "ihqwcrb4cv8a8dqg056pqjye")
                                       << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d3[] = { 0x4ED6, 0x5011, 0x7232, 0x4EC0, 0x9EBD, 0x4E0D, 0x8AAA, 0x4E2D,
                            0x6587 };
    QTest::newRow("Chinese (traditional)") << 9 << ushortarray(d3)
                                        << QByteArray(IDNA_ACE_PREFIX "ihqwctvzc91f659drss3x8bo0yb")
                                        << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d4[] = { 0x0050, 0x0072, 0x006F, 0x010D, 0x0070, 0x0072, 0x006F, 0x0073,
                            0x0074, 0x011B, 0x006E, 0x0065, 0x006D, 0x006C, 0x0075, 0x0076,
                            0x00ED, 0x010D, 0x0065, 0x0073, 0x006B, 0x0079 };
    QTest::newRow("Czech") << 22 << ushortarray(d4)
                        << QByteArray(IDNA_ACE_PREFIX "Proprostnemluvesky-uyb24dma41a")
                        << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d5[] = { 0x05DC, 0x05DE, 0x05D4, 0x05D4, 0x05DD, 0x05E4, 0x05E9, 0x05D5,
                            0x05D8, 0x05DC, 0x05D0, 0x05DE, 0x05D3, 0x05D1, 0x05E8, 0x05D9,
                            0x05DD, 0x05E2, 0x05D1, 0x05E8, 0x05D9, 0x05EA };
    QTest::newRow("Hebrew") << 22 << ushortarray(d5)
                         << QByteArray(IDNA_ACE_PREFIX "4dbcagdahymbxekheh6e0a7fei0b")
                         << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d6[] = { 0x092F, 0x0939, 0x0932, 0x094B, 0x0917, 0x0939, 0x093F, 0x0928,
                            0x094D, 0x0926, 0x0940, 0x0915, 0x094D, 0x092F, 0x094B, 0x0902,
                            0x0928, 0x0939, 0x0940, 0x0902, 0x092C, 0x094B, 0x0932, 0x0938,
                            0x0915, 0x0924, 0x0947, 0x0939, 0x0948, 0x0902 };
    QTest::newRow("Hindi (Devanagari)") << 30 << ushortarray(d6)
                                     << QByteArray(IDNA_ACE_PREFIX "i1baa7eci9glrd9b2ae1bj0hfcgg6iyaf8o0a1dig0cd")
                                     << 0 << 0 << IDNA_SUCCESS;

    unsigned short d7[] = { 0x306A, 0x305C, 0x307F, 0x3093, 0x306A, 0x65E5, 0x672C, 0x8A9E,
                            0x3092, 0x8A71, 0x3057, 0x3066, 0x304F, 0x308C, 0x306A, 0x3044,
                            0x306E, 0x304B };
    QTest::newRow("Japanese (kanji and hiragana)") << 18 << ushortarray(d7)
                                                << QByteArray(IDNA_ACE_PREFIX "n8jok5ay5dzabd5bym9f0cm5685rrjetr6pdxa")
                                                << 0 << 0 << IDNA_SUCCESS;

    unsigned short d8[] = { 0x043F, 0x043E, 0x0447, 0x0435, 0x043C, 0x0443, 0x0436, 0x0435,
                            0x043E, 0x043D, 0x0438, 0x043D, 0x0435, 0x0433, 0x043E, 0x0432,
                            0x043E, 0x0440, 0x044F, 0x0442, 0x043F, 0x043E, 0x0440, 0x0443,
                            0x0441, 0x0441, 0x043A, 0x0438 };
    QTest::newRow("Russian (Cyrillic)") << 28 << ushortarray(d8)
                                     << QByteArray(IDNA_ACE_PREFIX "b1abfaaepdrnnbgefbadotcwatmq2g4l")
                                     << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d9[] = { 0x0050, 0x006F, 0x0072, 0x0071, 0x0075, 0x00E9, 0x006E, 0x006F,
                            0x0070, 0x0075, 0x0065, 0x0064, 0x0065, 0x006E, 0x0073, 0x0069,
                            0x006D, 0x0070, 0x006C, 0x0065, 0x006D, 0x0065, 0x006E, 0x0074,
                            0x0065, 0x0068, 0x0061, 0x0062, 0x006C, 0x0061, 0x0072, 0x0065,
                            0x006E, 0x0045, 0x0073, 0x0070, 0x0061, 0x00F1, 0x006F, 0x006C };
    QTest::newRow("Spanish") << 40 << ushortarray(d9)
                          << QByteArray(IDNA_ACE_PREFIX "PorqunopuedensimplementehablarenEspaol-fmd56a")
                          << 0 << 0 << IDNA_SUCCESS;

    unsigned short d10[] = { 0x0054, 0x1EA1, 0x0069, 0x0073, 0x0061, 0x006F, 0x0068, 0x1ECD,
                             0x006B, 0x0068, 0x00F4, 0x006E, 0x0067, 0x0074, 0x0068, 0x1EC3,
                             0x0063, 0x0068, 0x1EC9, 0x006E, 0x00F3, 0x0069, 0x0074, 0x0069,
                             0x1EBF, 0x006E, 0x0067, 0x0056, 0x0069, 0x1EC7, 0x0074 };
    QTest::newRow("Vietnamese") << 31 << ushortarray(d10)
                             << QByteArray(IDNA_ACE_PREFIX "TisaohkhngthchnitingVit-kjcr8268qyxafd2f1b9g")
                             << 0 << 0 << IDNA_SUCCESS;

    unsigned short d11[] = { 0x0033, 0x5E74, 0x0042, 0x7D44, 0x91D1, 0x516B, 0x5148, 0x751F };
    QTest::newRow("Japanese") << 8 << ushortarray(d11)
                           << QByteArray(IDNA_ACE_PREFIX "3B-ww4c5e180e575a65lsy2b")
                           << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    // this test does NOT include nameprepping, so the capitals will remain
    unsigned short d12[] = { 0x5B89, 0x5BA4, 0x5948, 0x7F8E, 0x6075, 0x002D, 0x0077, 0x0069,
                             0x0074, 0x0068, 0x002D, 0x0053, 0x0055, 0x0050, 0x0045, 0x0052,
                             0x002D, 0x004D, 0x004F, 0x004E, 0x004B, 0x0045, 0x0059, 0x0053 };
    QTest::newRow("Japanese2") << 24 << ushortarray(d12)
                            << QByteArray(IDNA_ACE_PREFIX "-with-SUPER-MONKEYS-pc58ag80a8qai00g7n9n")
                            << 0 << 0 << IDNA_SUCCESS;

    unsigned short d13[] = { 0x0048, 0x0065, 0x006C, 0x006C, 0x006F, 0x002D, 0x0041, 0x006E,
                             0x006F, 0x0074, 0x0068, 0x0065, 0x0072, 0x002D, 0x0057, 0x0061,
                             0x0079, 0x002D, 0x305D, 0x308C, 0x305E, 0x308C, 0x306E, 0x5834,
                             0x6240 };
    QTest::newRow("Japanese3") << 25 << ushortarray(d13)
                            << QByteArray(IDNA_ACE_PREFIX "Hello-Another-Way--fc4qua05auwb3674vfr0b")
                            << 0 << 0 << IDNA_SUCCESS;

    unsigned short d14[] = { 0x3072, 0x3068, 0x3064, 0x5C4B, 0x6839, 0x306E, 0x4E0B, 0x0032 };
    QTest::newRow("Japanese4") << 8 << ushortarray(d14)
                            << QByteArray(IDNA_ACE_PREFIX "2-u9tlzr9756bt3uc0v")
                            << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d15[] = { 0x004D, 0x0061, 0x006A, 0x0069, 0x3067, 0x004B, 0x006F, 0x0069,
                             0x3059, 0x308B, 0x0035, 0x79D2, 0x524D };
    QTest::newRow("Japanese5") << 13 << ushortarray(d15)
                            << QByteArray(IDNA_ACE_PREFIX "MajiKoi5-783gue6qz075azm5e")
                            << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d16[] = { 0x30D1, 0x30D5, 0x30A3, 0x30FC, 0x0064, 0x0065, 0x30EB, 0x30F3, 0x30D0 };
    QTest::newRow("Japanese6") << 9 << ushortarray(d16)
                            << QByteArray(IDNA_ACE_PREFIX "de-jg4avhby1noc0d")
                            << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d17[] = { 0x305D, 0x306E, 0x30B9, 0x30D4, 0x30FC, 0x30C9, 0x3067 };
    QTest::newRow("Japanese7") << 7 << ushortarray(d17)
                            << QByteArray(IDNA_ACE_PREFIX "d9juau41awczczp")
                            << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d18[] = { 0x03b5, 0x03bb, 0x03bb, 0x03b7, 0x03bd, 0x03b9, 0x03ba, 0x03ac };
    QTest::newRow("Greek") << 8 << ushortarray(d18)
                        << QByteArray(IDNA_ACE_PREFIX "hxargifdar")
                        << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d19[] = { 0x0062, 0x006f, 0x006e, 0x0121, 0x0075, 0x0073, 0x0061, 0x0127,
                             0x0127, 0x0061 };
    QTest::newRow("Maltese (Malti)") << 10 << ushortarray(d19)
                                  << QByteArray(IDNA_ACE_PREFIX "bonusaa-5bb1da")
                                  << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d20[] = {0x043f, 0x043e, 0x0447, 0x0435, 0x043c, 0x0443, 0x0436, 0x0435,
                            0x043e, 0x043d, 0x0438, 0x043d, 0x0435, 0x0433, 0x043e, 0x0432,
                            0x043e, 0x0440, 0x044f, 0x0442, 0x043f, 0x043e, 0x0440, 0x0443,
                            0x0441, 0x0441, 0x043a, 0x0438 };
    QTest::newRow("Russian (Cyrillic)") << 28 << ushortarray(d20)
                                     << QByteArray(IDNA_ACE_PREFIX "b1abfaaepdrnnbgefbadotcwatmq2g4l")
                                     << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QUrlInternal::idna_testsuite()
{
    QFETCH(int, numchars);
    QFETCH(ushortarray, unicode);
    QFETCH(QByteArray, punycode);

    QString result;
    qt_punycodeEncoder((QChar*)unicode.points, numchars, &result);
    QCOMPARE(result.toLatin1(), punycode);
    QCOMPARE(qt_punycodeDecoder(result), QString::fromUtf16(unicode.points, numchars));
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QUrlInternal::nameprep_testsuite_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<QString>("profile");
    QTest::addColumn<int>("flags");
    QTest::addColumn<int>("rc");

    QTest::newRow("Map to nothing")
        << QString::fromUtf8("foo\xC2\xAD\xCD\x8F\xE1\xA0\x86\xE1\xA0\x8B"
                             "bar""\xE2\x80\x8B\xE2\x81\xA0""baz\xEF\xB8\x80\xEF\xB8\x88"
                             "\xEF\xB8\x8F\xEF\xBB\xBF")
        << QString::fromUtf8("foobarbaz")
        << QString() << 0 << 0;

    QTest::newRow("Case folding ASCII U+0043 U+0041 U+0046 U+0045")
        << QString::fromUtf8("CAFE")
        << QString::fromUtf8("cafe")
        << QString() << 0 << 0;

    QTest::newRow("Case folding 8bit U+00DF (german sharp s)")
        << QString::fromUtf8("\xC3\x9F")
        << QString("ss")
        << QString() << 0 << 0;

    QTest::newRow("Case folding U+0130 (turkish capital I with dot)")
        << QString::fromUtf8("\xC4\xB0")
        << QString::fromUtf8("i\xcc\x87")
        << QString() << 0 << 0;

    QTest::newRow("Case folding multibyte U+0143 U+037A")
        << QString::fromUtf8("\xC5\x83\xCD\xBA")
        << QString::fromUtf8("\xC5\x84 \xCE\xB9")
        << QString() << 0 << 0;

    QTest::newRow("Case folding U+2121 U+33C6 U+1D7BB")
        << QString::fromUtf8("\xE2\x84\xA1\xE3\x8F\x86\xF0\x9D\x9E\xBB")
        << QString::fromUtf8("telc\xE2\x88\x95""kg\xCF\x83")
        << QString() << 0 << 0;

    QTest::newRow("Normalization of U+006a U+030c U+00A0 U+00AA")
        << QString::fromUtf8("\x6A\xCC\x8C\xC2\xA0\xC2\xAA")
        << QString::fromUtf8("\xC7\xB0 a")
        << QString() << 0 << 0;

    QTest::newRow("Case folding U+1FB7 and normalization")
        << QString::fromUtf8("\xE1\xBE\xB7")
        << QString::fromUtf8("\xE1\xBE\xB6\xCE\xB9")
        << QString() << 0 << 0;

    QTest::newRow("Self-reverting case folding U+01F0 and normalization")
//        << QString::fromUtf8("\xC7\xF0") ### typo in the original testsuite
        << QString::fromUtf8("\xC7\xB0")
        << QString::fromUtf8("\xC7\xB0")
        << QString() << 0 << 0;

    QTest::newRow("Self-reverting case folding U+0390 and normalization")
        << QString::fromUtf8("\xCE\x90")
        << QString::fromUtf8("\xCE\x90")
        << QString() << 0 << 0;

    QTest::newRow("Self-reverting case folding U+03B0 and normalization")
        << QString::fromUtf8("\xCE\xB0")
        << QString::fromUtf8("\xCE\xB0")
        << QString() << 0 << 0;

    QTest::newRow("Self-reverting case folding U+1E96 and normalization")
        << QString::fromUtf8("\xE1\xBA\x96")
        << QString::fromUtf8("\xE1\xBA\x96")
        << QString() << 0 << 0;

    QTest::newRow("Self-reverting case folding U+1F56 and normalization")
        << QString::fromUtf8("\xE1\xBD\x96")
        << QString::fromUtf8("\xE1\xBD\x96")
        << QString() << 0 << 0;

    QTest::newRow("ASCII space character U+0020")
        << QString::fromUtf8("\x20")
        << QString::fromUtf8("\x20")
        << QString() << 0 << 0;

    QTest::newRow("Non-ASCII 8bit space character U+00A0")
        << QString::fromUtf8("\xC2\xA0")
        << QString::fromUtf8("\x20")
        << QString() << 0 << 0;

    QTest::newRow("Non-ASCII multibyte space character U+1680")
        << QString::fromUtf8("x\xE1\x9A\x80x")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Non-ASCII multibyte space character U+2000")
        << QString::fromUtf8("\xE2\x80\x80")
        << QString::fromUtf8("\x20")
        << QString() << 0 << 0;

    QTest::newRow("Zero Width Space U+200b")
        << QString::fromUtf8("\xE2\x80\x8b")
        << QString()
        << QString() << 0 << 0;

    QTest::newRow("Non-ASCII multibyte space character U+3000")
        << QString::fromUtf8("\xE3\x80\x80")
        << QString::fromUtf8("\x20")
        << QString() << 0 << 0;

    QTest::newRow("ASCII control characters U+0010 U+007F")
        << QString::fromUtf8("\x10\x7F")
        << QString::fromUtf8("\x10\x7F")
        << QString() << 0 << 0;

    QTest::newRow("Non-ASCII 8bit control character U+0080")
        << QString::fromUtf8("x\xC2\x80x")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Non-ASCII 8bit control character U+0085")
        << QString::fromUtf8("x\xC2\x85x")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Non-ASCII multibyte control character U+180E")
        << QString::fromUtf8("x\xE1\xA0\x8Ex")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Zero Width No-Break Space U+FEFF")
        << QString::fromUtf8("\xEF\xBB\xBF")
        << QString()
        << QString() << 0 << 0;

    QTest::newRow("Non-ASCII control character U+1D175")
        << QString::fromUtf8("x\xF0\x9D\x85\xB5x")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Plane 0 private use character U+F123")
        << QString::fromUtf8("x\xEF\x84\xA3x")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Plane 15 private use character U+F1234")
        << QString::fromUtf8("x\xF3\xB1\x88\xB4x")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Plane 16 private use character U+10F234")
        << QString::fromUtf8("x\xF4\x8F\x88\xB4x")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Non-character code point U+8FFFE")
        << QString::fromUtf8("x\xF2\x8F\xBF\xBEx")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Non-character code point U+10FFFF")
        << QString::fromUtf8("x\xF4\x8F\xBF\xBFx")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Surrogate code U+DF42")
        << QString::fromUtf8("x\xED\xBD\x82x")
        << QString()
        << QString("Nameprep") << 0 <<  STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Non-plain text character U+FFFD")
        << QString::fromUtf8("x\xEF\xBF\xBDx")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Ideographic description character U+2FF5")
        << QString::fromUtf8("x\xE2\xBF\xB5x")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Display property character U+0341")
        << QString::fromUtf8("\xCD\x81")
        << QString::fromUtf8("\xCC\x81")
        << QString() << 0 << 0;

    QTest::newRow("Left-to-right mark U+200E")
        << QString::fromUtf8("x\xE2\x80\x8Ex")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Deprecated U+202A")
        << QString::fromUtf8("x\xE2\x80\xAA")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Language tagging character U+E0001")
        << QString::fromUtf8("x\xF3\xA0\x80\x81x")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Language tagging character U+E0042")
        << QString::fromUtf8("x\xF3\xA0\x81\x82x")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Bidi: RandALCat character U+05BE and LCat characters")
        << QString::fromUtf8("foo\xD6\xBE""bar")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_BIDI_BOTH_L_AND_RAL;

    QTest::newRow("Bidi: RandALCat character U+FD50 and LCat characters")
        << QString::fromUtf8("foo\xEF\xB5\x90""bar")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_BIDI_BOTH_L_AND_RAL;

    QTest::newRow("Bidi: RandALCat character U+FB38 and LCat characters")
        << QString::fromUtf8("foo\xEF\xB9\xB6""bar")
        << QString::fromUtf8("foo \xd9\x8e""bar")
        << QString() << 0 << 0;

    QTest::newRow("Bidi: RandALCat without trailing RandALCat U+0627 U+0031")
        << QString::fromUtf8("\xD8\xA7\x31")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_BIDI_LEADTRAIL_NOT_RAL;

    QTest::newRow("Bidi: RandALCat character U+0627 U+0031 U+0628")
        << QString::fromUtf8("\xD8\xA7\x31\xD8\xA8")
        << QString::fromUtf8("\xD8\xA7\x31\xD8\xA8")
        << QString() << 0 << 0;

    QTest::newRow("Unassigned code point U+E0002")
        << QString::fromUtf8("\xF3\xA0\x80\x82")
        << QString()
        << QString("Nameprep") << STRINGPREP_NO_UNASSIGNED << STRINGPREP_CONTAINS_UNASSIGNED;

    QTest::newRow("Larger test (shrinking)")
        << QString::fromUtf8("X\xC2\xAD\xC3\x9F\xC4\xB0\xE2\x84\xA1\x6a\xcc\x8c\xc2\xa0\xc2"
                             "\xaa\xce\xb0\xe2\x80\x80")
        << QString::fromUtf8("xssi\xcc\x87""tel\xc7\xb0 a\xce\xb0 ")
        << QString("Nameprep") << 0 << 0;

    QTest::newRow("Larger test (expanding)")
        << QString::fromUtf8("X\xC3\x9F\xe3\x8c\x96\xC4\xB0\xE2\x84\xA1\xE2\x92\x9F\xE3\x8c\x80")
        << QString::fromUtf8("xss\xe3\x82\xad\xe3\x83\xad\xe3\x83\xa1\xe3\x83\xbc\xe3\x83\x88"
                             "\xe3\x83\xab""i\xcc\x87""tel\x28""d\x29\xe3\x82\xa2\xe3\x83\x91"
                             "\xe3\x83\xbc\xe3\x83\x88")
        << QString() << 0 << 0;
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QUrlInternal::nameprep_testsuite()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(QString, profile);

    qt_nameprep(&in, 0);
    QCOMPARE(in, out);
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QUrlInternal::nameprep_highcodes_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<QString>("profile");
    QTest::addColumn<int>("flags");
    QTest::addColumn<int>("rc");

    {
        QChar st[] = { '-', 0xd801, 0xdc1d, 'a' };
        QChar se[] = { '-', 0xd801, 0xdc45, 'a' };
        QTest::newRow("highcodes (U+1041D)")
            << QString(st, sizeof(st)/sizeof(st[0]))
            << QString(se, sizeof(se)/sizeof(se[0]))
            << QString() << 0 << 0;
    }
    {
        QChar st[] = { 0x011C, 0xd835, 0xdf6e, 0x0110 };
        QChar se[] = { 0x011D, 0x03C9, 0x0111 };
        QTest::newRow("highcodes (U+1D76E)")
            << QString(st, sizeof(st)/sizeof(st[0]))
            << QString(se, sizeof(se)/sizeof(se[0]))
            << QString() << 0 << 0;
    }
    {
        QChar st[] = { 'D', 'o', '\'', 0x2060, 'h' };
        QChar se[] = { 'd', 'o', '\'', 'h' };
        QTest::newRow("highcodes (D, o, ', U+2060, h)")
            << QString(st, sizeof(st)/sizeof(st[0]))
            << QString(se, sizeof(se)/sizeof(se[0]))
            << QString() << 0 << 0;
    }
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QUrlInternal::nameprep_highcodes()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(QString, profile);

    qt_nameprep(&in, 0);
    QCOMPARE(in, out);
}
#endif

void tst_QUrlInternal::ace_testsuite_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("toace");
    QTest::addColumn<QString>("fromace");
    QTest::addColumn<QString>("unicode");

    QTest::newRow("ascii-lower") << "fluke" << "fluke" << "fluke" << "fluke";
    QTest::newRow("ascii-mixed") << "FLuke" << "fluke" << "fluke" << "fluke";
    QTest::newRow("ascii-upper") << "FLUKE" << "fluke" << "fluke" << "fluke";

    QTest::newRow("asciifolded") << QString::fromLatin1("stra\337e") << "strasse" << "." << "strasse";
    QTest::newRow("asciifolded-dotcom") << QString::fromLatin1("stra\337e.example.com") << "strasse.example.com" << "." << "strasse.example.com";
    QTest::newRow("greek-mu") << QString::fromLatin1("\265V")
                              <<"xn--v-lmb"
                              << "."
                              << QString::fromUtf8("\316\274v");

    QTest::newRow("non-ascii-lower") << QString::fromLatin1("alqualond\353")
                                     << "xn--alqualond-34a"
                                     << "."
                                     << QString::fromLatin1("alqualond\353");
    QTest::newRow("non-ascii-mixed") << QString::fromLatin1("Alqualond\353")
                                     << "xn--alqualond-34a"
                                     << "."
                                     << QString::fromLatin1("alqualond\353");
    QTest::newRow("non-ascii-upper") << QString::fromLatin1("ALQUALOND\313")
                                     << "xn--alqualond-34a"
                                     << "."
                                     << QString::fromLatin1("alqualond\353");

    QTest::newRow("idn-lower") << "xn--alqualond-34a" << "xn--alqualond-34a"
                               << QString::fromLatin1("alqualond\353")
                               << QString::fromLatin1("alqualond\353");
    QTest::newRow("idn-mixed") << "Xn--alqualond-34a" << "xn--alqualond-34a"
                               << QString::fromLatin1("alqualond\353")
                               << QString::fromLatin1("alqualond\353");
    QTest::newRow("idn-mixed2") << "XN--alqualond-34a" << "xn--alqualond-34a"
                                << QString::fromLatin1("alqualond\353")
                                << QString::fromLatin1("alqualond\353");
    QTest::newRow("idn-mixed3") << "xn--ALQUALOND-34a" << "xn--alqualond-34a"
                                << QString::fromLatin1("alqualond\353")
                                << QString::fromLatin1("alqualond\353");
    QTest::newRow("idn-mixed4") << "xn--alqualond-34A" << "xn--alqualond-34a"
                                << QString::fromLatin1("alqualond\353")
                                << QString::fromLatin1("alqualond\353");
    QTest::newRow("idn-upper") << "XN--ALQUALOND-34A" << "xn--alqualond-34a"
                               << QString::fromLatin1("alqualond\353")
                               << QString::fromLatin1("alqualond\353");

    QTest::newRow("separator-3002") << QString::fromUtf8("example\343\200\202com")
                                    << "example.com" << "." << "example.com";

    QString egyptianIDN =
        QString::fromUtf8("\331\210\330\262\330\247\330\261\330\251\055\330\247\331\204\330"
                          "\243\330\252\330\265\330\247\331\204\330\247\330\252.\331\205"
                          "\330\265\330\261");
    QTest::newRow("egyptian-tld-ace")
        << "xn----rmckbbajlc6dj7bxne2c.xn--wgbh1c"
        << "xn----rmckbbajlc6dj7bxne2c.xn--wgbh1c"
        << "."
        << egyptianIDN;
    QTest::newRow("egyptian-tld-unicode")
        << egyptianIDN
        << "xn----rmckbbajlc6dj7bxne2c.xn--wgbh1c"
        << "."
        << egyptianIDN;
    QTest::newRow("egyptian-tld-mix1")
        << QString::fromUtf8("\331\210\330\262\330\247\330\261\330\251\055\330\247\331\204\330"
                             "\243\330\252\330\265\330\247\331\204\330\247\330\252.xn--wgbh1c")
        << "xn----rmckbbajlc6dj7bxne2c.xn--wgbh1c"
        << "."
        << egyptianIDN;
    QTest::newRow("egyptian-tld-mix2")
        << QString::fromUtf8("xn----rmckbbajlc6dj7bxne2c.\331\205\330\265\330\261")
        << "xn----rmckbbajlc6dj7bxne2c.xn--wgbh1c"
        << "."
        << egyptianIDN;

    QString russianIDN = QString::fromUtf8("\321\217\320\275\320\264\320\265\320\272\321\201.\321\200\321\204");
    QTest::newRow("russian-tld-ace")
        << "xn--d1acpjx3f.xn--p1ai"
        << "xn--d1acpjx3f.xn--p1ai"
        << "."
        << russianIDN;

    QString taiwaneseIDN = QString::fromUtf8("\345\217\260\345\214\227\346\214\211\346\221\251.\345\217\260\347\201\243");
    QTest::newRow("taiwanese-tld-ace")
        << "xn--djrptm67aikb.xn--kpry57d"
        << "xn--djrptm67aikb.xn--kpry57d"
        << "."
        << taiwaneseIDN;

    // violations / invalids
    QTest::newRow("invalid-punycode") << "xn--z" << "xn--z" << "xn--z" << "xn--z";

    // U+00A0 NO-BREAK SPACE encodes to Punycode "6a"
    // but it is prohibited and should have caused encoding failure
    QTest::newRow("invalid-nameprep-prohibited") << "xn--6a" << "xn--6a" << "xn--6a" << "xn--6a";

    // U+00AD SOFT HYPHEN between "a" and "b" encodes to Punycode "ab-5da"
    // but it should have been removed in the nameprep stage
    QTest::newRow("invalid-nameprep-maptonothing") << "xn-ab-5da" << "xn-ab-5da" << "xn-ab-5da" << "xn-ab-5da";

    // U+00C1 LATIN CAPITAL LETTER A WITH ACUTE encodes to Punycode "4ba"
    // but it should have nameprepped to lowercase first
    QTest::newRow("invalid-nameprep-uppercase") << "xn--4ba" << "xn--4ba" << "xn--4ba" << "xn--4ba";

    // U+00B5 MICRO SIGN encodes to Punycode "sba"
    // but is should have nameprepped to NFKC U+03BC GREEK SMALL LETTER MU
    QTest::newRow("invalid-nameprep-nonnfkc") << "xn--sba" << "xn--sba" << "xn--sba" << "xn--sba";

    // U+04CF CYRILLIC SMALL LETTER PALOCHKA encodes to "s5a"
    // but it's not in RFC 3454's allowed character list (Unicode 3.2)
    QTest::newRow("invalid-nameprep-unassigned") << "xn--s5a" << "xn--s5a" << "xn--s5a" << "xn--s5a";
    // same character, see QTBUG-60364
    QTest::newRow("invalid-nameprep-unassigned2") << "xn--80ak6aa92e" << "xn--80ak6aa92e" << "xn--80ak6aa92e" << "xn--80ak6aa92e";
}

void tst_QUrlInternal::ace_testsuite()
{
    static const char canonsuffix[] = ".troll.no";
    QFETCH(QString, in);
    QFETCH(QString, toace);
    QFETCH(QString, fromace);
    QFETCH(QString, unicode);

    const char *suffix = canonsuffix;
    if (toace.contains('.'))
        suffix = 0;

    QString domain = in + suffix;
    QCOMPARE(QString::fromLatin1(QUrl::toAce(domain)), toace + suffix);
    if (fromace != ".")
        QCOMPARE(QUrl::fromAce(domain.toLatin1()), fromace + suffix);
    QCOMPARE(QUrl::fromAce(QUrl::toAce(domain)), unicode + suffix);

    QUrl u;
    u.setHost(domain);
    QVERIFY(u.isValid());
    QCOMPARE(u.host(), unicode + suffix);
    QCOMPARE(u.host(QUrl::EncodeUnicode), toace + suffix);
    QCOMPARE(u.toEncoded(), "//" + toace.toLatin1() + suffix);
    QCOMPARE(u.toDisplayString(), "//" + unicode + suffix);

    domain = in + (suffix ? ".troll.No" : "");
    QCOMPARE(QString::fromLatin1(QUrl::toAce(domain)), toace + suffix);
    if (fromace != ".")
        QCOMPARE(QUrl::fromAce(domain.toLatin1()), fromace + suffix);
    QCOMPARE(QUrl::fromAce(QUrl::toAce(domain)), unicode + suffix);

    domain = in + (suffix ? ".troll.NO" : "");
    QCOMPARE(QString::fromLatin1(QUrl::toAce(domain)), toace + suffix);
    if (fromace != ".")
        QCOMPARE(QUrl::fromAce(domain.toLatin1()), fromace + suffix);
    QCOMPARE(QUrl::fromAce(QUrl::toAce(domain)), unicode + suffix);
}

void tst_QUrlInternal::std3violations_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<bool>("validUrl");

    QTest::newRow("too-long") << "this-domain-is-far-too-long-for-its-own-good-and-should-have-been-limited-to-63-chars" << false;
    QTest::newRow("dash-begin") << "-x-foo" << false;
    QTest::newRow("dash-end") << "x-foo-" << false;
    QTest::newRow("dash-begin-end") << "-foo-" << false;

    QTest::newRow("control") << "\033foo" << false;
    QTest::newRow("bang") << "foo!" << false;
    QTest::newRow("plus") << "foo+bar" << false;
    QTest::newRow("dot") << "foo.bar";
    QTest::newRow("startingdot") << ".bar" << false;
    QTest::newRow("startingdot2") << ".example.com" << false;
    QTest::newRow("slash") << "foo/bar" << true;
    QTest::newRow("colon") << "foo:80" << true;
    QTest::newRow("question") << "foo?bar" << true;
    QTest::newRow("at") << "foo@bar" << true;
    QTest::newRow("backslash") << "foo\\bar" << false;

    // these characters are transformed by NFKC to non-LDH characters
    QTest::newRow("dot-like") << QString::fromUtf8("foo\342\200\244bar") << false;  // U+2024 ONE DOT LEADER
    QTest::newRow("slash-like") << QString::fromUtf8("foo\357\274\217bar") << false;    // U+FF0F FULLWIDTH SOLIDUS

    // The following should be invalid but isn't
    // the DIVISON SLASH doesn't case-fold to a slash
    // is this a problem with RFC 3490?
    //QTest::newRow("slash-like2") << QString::fromUtf8("foo\342\210\225bar") << false; // U+2215 DIVISION SLASH
}

void tst_QUrlInternal::std3violations()
{
    QFETCH(QString, source);

#ifdef QT_BUILD_INTERNAL
    {
        QString prepped = source;
        qt_nameprep(&prepped, 0);
        QVERIFY(!qt_check_std3rules(prepped.constData(), prepped.length()));
    }
#endif

    if (source.contains('.'))
        return; // this test ends here

    QUrl url;
    url.setHost(source);
    QVERIFY(url.host().isEmpty());

    QFETCH(bool, validUrl);
    if (validUrl)
        return;  // test ends here for these cases

    url = QUrl("http://" + source + "/some/path");
    QVERIFY(!url.isValid());
}

void tst_QUrlInternal::std3deviations_data()
{
    QTest::addColumn<QString>("source");

    QTest::newRow("ending-dot") << "example.com.";
    QTest::newRow("ending-dot3002") << QString("example.com") + QChar(0x3002);
    QTest::newRow("underline") << "foo_bar";  //QTBUG-7434
}

void tst_QUrlInternal::std3deviations()
{
    QFETCH(QString, source);
    QVERIFY(!QUrl::toAce(source).isEmpty());

    QUrl url;
    url.setHost(source);
    QVERIFY(!url.host().isEmpty());
}

void tst_QUrlInternal::correctEncodedMistakes_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("empty") << "" << "";

    // these contain one invalid percent
    QTest::newRow("%") << QString("%") << QString("%25");
    QTest::newRow("3%") << QString("3%") << QString("3%25");
    QTest::newRow("13%") << QString("13%") << QString("13%25");
    QTest::newRow("13%!") << QString("13%!") << QString("13%25!");
    QTest::newRow("13%!!") << QString("13%!!") << QString("13%25!!");
    QTest::newRow("13%a") << QString("13%a") << QString("13%25a");
    QTest::newRow("13%az") << QString("13%az") << QString("13%25az");

    // two invalid percents
    QTest::newRow("13%%") << "13%%" << "13%25%25";
    QTest::newRow("13%a%a") << "13%a%a" << "13%25a%25a";
    QTest::newRow("13%az%az") << "13%az%az" << "13%25az%25az";

    // these are correct (idempotent)
    QTest::newRow("13%25") << QString("13%25")  << QString("13%25");
    QTest::newRow("13%25%25") << QString("13%25%25")  << QString("13%25%25");

    // these contain one invalid and one valid
    // the code assumes they are all invalid
    QTest::newRow("13%13..%") << "13%13..%" << "13%2513..%25";
    QTest::newRow("13%..%13") << "13%..%13" << "13%25..%2513";

    // three percents, one invalid
    QTest::newRow("%01%02%3") << "%01%02%3" << "%2501%2502%253";

    // now mix bad percents with Unicode decoding
    QTest::newRow("%C2%") << "%C2%" << "%25C2%25";
    QTest::newRow("%C2%A") << "%C2%A" << "%25C2%25A";
    QTest::newRow("%C2%Az") << "%C2%Az" << "%25C2%25Az";
    QTest::newRow("%E2%A0%") << "%E2%A0%" << "%25E2%25A0%25";
    QTest::newRow("%E2%A0%A") << "%E2%A0%A" << "%25E2%25A0%25A";
    QTest::newRow("%E2%A0%Az") << "%E2%A0%Az" << "%25E2%25A0%25Az";
    QTest::newRow("%F2%A0%A0%") << "%F2%A0%A0%" << "%25F2%25A0%25A0%25";
    QTest::newRow("%F2%A0%A0%A") << "%F2%A0%A0%A" << "%25F2%25A0%25A0%25A";
    QTest::newRow("%F2%A0%A0%Az") << "%F2%A0%A0%Az" << "%25F2%25A0%25A0%25Az";
}

void tst_QUrlInternal::correctEncodedMistakes()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    // prepend some data to be sure that it remains there
    QString dataTag = QTest::currentDataTag();
    QString output = dataTag;

    if (!qt_urlRecode(output, input.constData(), input.constData() + input.length(), 0))
        output += input;
    QCOMPARE(output, dataTag + expected);

    // now try the full decode mode
    output = dataTag;
    QString expected2 = QUrl::fromPercentEncoding(expected.toLatin1());

    if (!qt_urlRecode(output, input.constData(), input.constData() + input.length(), QUrl::FullyDecoded))
        output += input;
    QCOMPARE(output, dataTag + expected2);
}

static void addUtf8Data(const char *name, const char *data)
{
    QString encoded = QByteArray(data).toPercentEncoding();
    QString decoded = QString::fromUtf8(data);

    // this data contains invaild UTF-8 sequences, so FullyDecoded doesn't work (by design)
    // use PrettyDecoded instead
    QTest::newRow(QByteArray("decode-") + name) << encoded << QUrl::ComponentFormattingOptions(QUrl::PrettyDecoded) << decoded;
    QTest::newRow(QByteArray("encode-") + name) << decoded << QUrl::ComponentFormattingOptions(QUrl::FullyEncoded) << encoded;
}

void tst_QUrlInternal::encodingRecode_data()
{
    typedef QUrl::ComponentFormattingOptions F;
    QTest::addColumn<QString>("input");
    QTest::addColumn<F>("encodingMode");
    QTest::addColumn<QString>("expected");

    // -- idempotent tests --
    static int modes[] = { QUrl::PrettyDecoded,
                           QUrl::EncodeSpaces,
                           QUrl::EncodeSpaces | QUrl::EncodeUnicode,
                           QUrl::EncodeSpaces | QUrl::EncodeUnicode | QUrl::EncodeDelimiters,
                           QUrl::EncodeSpaces | QUrl::EncodeUnicode | QUrl::EncodeDelimiters | QUrl::EncodeReserved,
                           QUrl::EncodeSpaces | QUrl::EncodeUnicode | QUrl::EncodeDelimiters | QUrl::DecodeReserved,
                           QUrl::EncodeSpaces | QUrl::EncodeUnicode | QUrl::EncodeReserved,
                           QUrl::EncodeSpaces | QUrl::EncodeUnicode | QUrl::DecodeReserved,
                           QUrl::EncodeSpaces | QUrl::EncodeDelimiters,
                           QUrl::EncodeSpaces | QUrl::EncodeDelimiters | QUrl::EncodeReserved,
                           QUrl::EncodeSpaces | QUrl::EncodeDelimiters | QUrl::DecodeReserved,
                           QUrl::EncodeSpaces | QUrl::EncodeReserved,
                           QUrl::EncodeSpaces | QUrl::DecodeReserved,

                           QUrl::EncodeUnicode,
                           QUrl::EncodeUnicode | QUrl::EncodeDelimiters,
                           QUrl::EncodeUnicode | QUrl::EncodeDelimiters | QUrl::EncodeReserved,
                           QUrl::EncodeUnicode | QUrl::EncodeDelimiters | QUrl::DecodeReserved,
                           QUrl::EncodeUnicode | QUrl::EncodeReserved,

                           QUrl::EncodeDelimiters,
                           QUrl::EncodeDelimiters | QUrl::EncodeReserved,
                           QUrl::EncodeDelimiters | QUrl::DecodeReserved,
                           QUrl::EncodeReserved,
                           QUrl::DecodeReserved };
    for (uint i = 0; i < sizeof(modes)/sizeof(modes[0]); ++i) {
        QByteArray code = QByteArray::number(modes[i], 16);
        F mode = QUrl::ComponentFormattingOption(modes[i]);

        QTest::newRow("null-0x" + code) << QString() << mode << QString();
        QTest::newRow("empty-0x" + code) << "" << mode << "";

        //    unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
        // Unreserved characters are never encoded
        QTest::newRow("alpha-0x" + code) << "abcABCZZzz" << mode << "abcABCZZzz";
        QTest::newRow("digits-0x" + code) << "01234567890" << mode << "01234567890";
        QTest::newRow("otherunreserved-0x" + code) << "-._~" << mode << "-._~";

        // Control characters are always encoded
        // Use uppercase because the output is also uppercased
        QTest::newRow("control-nul-0x" + code) << "%00" << mode << "%00";
        QTest::newRow("control-0x" + code) << "%0D%0A%1F%1A%7F" << mode << "%0D%0A%1F%1A%7F";

        // The percent is always encoded
        QTest::newRow("percent-0x" + code) << "25%2525" << mode << "25%2525";

        // mixed control and unreserved
        QTest::newRow("control-unreserved-0x" + code) << "Foo%00Bar%0D%0Abksp%7F" << mode << "Foo%00Bar%0D%0Abksp%7F";
    }

    // however, control characters and the percent *are* decoded in FullyDecoded mode
    // this is the only exception
    QTest::newRow("control-nul-fullydecoded") << "%00" << F(QUrl::FullyDecoded) << QStringLiteral("\0");
    QTest::newRow("control-fullydecoded") << "%0D%0A%1F%1A%7F" << F(QUrl::FullyDecoded) << "\r\n\x1f\x1a\x7f";
    QTest::newRow("percent-fullydecoded") << "25%2525" << F(QUrl::FullyDecoded) << "25%25";
    QTest::newRow("control-unreserved-fullydecoded") << "Foo%00Bar%0D%0Abksp%7F" << F(QUrl::FullyDecoded)
                                                     << QStringLiteral("Foo\0Bar\r\nbksp\x7F");

    //    gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
    //    sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
    //                  / "*" / "+" / "," / ";" / "="
    // in the default operation, delimiters don't get encoded or decoded
    static const char delimiters[] =  ":/?#[]@" "!$&'()*+,;=";
    for (const char *c = delimiters; *c; ++c) {
        QByteArray code = QByteArray::number(*c, 16);
        QString encoded = QString("abc%") + code.toUpper() + "def" ;
        QString decoded = QString("abc") + *c + "def" ;
        QTest::newRow("delimiter-encoded-" + code) << encoded << F(QUrl::FullyEncoded) << encoded;
        QTest::newRow("delimiter-decoded-" + code) << decoded << F(QUrl::FullyEncoded) << decoded;
    }

    // encode control characters
    QTest::newRow("encode-control") << "\1abc\2\033esc" << F(QUrl::PrettyDecoded) << "%01abc%02%1Besc";
    QTest::newRow("encode-nul") << QString::fromLatin1("abc\0def", 7) << F(QUrl::PrettyDecoded) << "abc%00def";

    // space
    QTest::newRow("space-leave-decoded") << "Hello World " << F(QUrl::PrettyDecoded) << "Hello World ";
    QTest::newRow("space-leave-encoded") << "Hello%20World%20" << F(QUrl::FullyEncoded) << "Hello%20World%20";
    QTest::newRow("space-encode") << "Hello World " << F(QUrl::FullyEncoded) << "Hello%20World%20";
    QTest::newRow("space-decode") << "Hello%20World%20" << F(QUrl::PrettyDecoded) << "Hello World ";

    // decode unreserved
    QTest::newRow("unreserved-decode") << "%66%6f%6f%42a%72" << F(QUrl::FullyEncoded) << "fooBar";

    // mix encoding with decoding
    QTest::newRow("encode-control-decode-space") << "\1\2%200" << F(QUrl::PrettyDecoded) << "%01%02 0";
    QTest::newRow("decode-space-encode-control") << "%20\1\2" << F(QUrl::PrettyDecoded) << " %01%02";

    // decode and encode valid UTF-8 data
    // invalid is tested in encodingRecodeInvalidUtf8
    addUtf8Data("utf8-2char-1", "\xC2\x80"); // U+0080
    addUtf8Data("utf8-2char-2", "\xDF\xBF"); // U+07FF
    addUtf8Data("utf8-3char-1", "\xE0\xA0\x80"); // U+0800
    addUtf8Data("utf8-3char-2", "\xED\x9F\xBF"); // U+D7FF
    addUtf8Data("utf8-3char-3", "\xEE\x80\x80"); // U+E000
    addUtf8Data("utf8-3char-4", "\xEF\xBF\xBD"); // U+FFFD
    addUtf8Data("utf8-4char-1", "\xF0\x90\x80\x80"); // U+10000
    addUtf8Data("utf8-4char-2", "\xF4\x8F\xBF\xBD"); // U+10FFFD

    // longer UTF-8 sequences, mixed with unreserved
    addUtf8Data("utf8-string-1", "R\xc3\xa9sum\xc3\xa9");
    addUtf8Data("utf8-string-2", "\xDF\xBF\xE0\xA0\x80""A");
    addUtf8Data("utf8-string-3", "\xE0\xA0\x80\xDF\xBF...");

    QTest::newRow("encode-unicode-noncharacter") << QString(QChar(0xffff)) << F(QUrl::FullyEncoded) << "%EF%BF%BF";
    QTest::newRow("decode-unicode-noncharacter") << QString(QChar(0xffff)) << F(QUrl::PrettyDecoded) << QString::fromUtf8("\xEF\xBF\xBF");

    // special cases: stuff we can encode, but not decode
    QTest::newRow("unicode-lo-surrogate") << QString(QChar(0xD800)) << F(QUrl::FullyEncoded) << "%ED%A0%80";
    QTest::newRow("unicode-hi-surrogate") << QString(QChar(0xDC00)) << F(QUrl::FullyEncoded) << "%ED%B0%80";

    // a couple of Unicode strings with leading spaces
    QTest::newRow("space-unicode") << QString::fromUtf8(" \xc2\xa0") << F(QUrl::FullyEncoded) << "%20%C2%A0";
    QTest::newRow("space-space-unicode") << QString::fromUtf8("  \xc2\xa0") << F(QUrl::FullyEncoded) << "%20%20%C2%A0";
    QTest::newRow("space-space-space-unicode") << QString::fromUtf8("   \xc2\xa0") << F(QUrl::FullyEncoded) << "%20%20%20%C2%A0";

    // hex case testing
    QTest::newRow("FF") << "%FF" << F(QUrl::FullyEncoded) << "%FF";
    QTest::newRow("Ff") << "%Ff" << F(QUrl::FullyEncoded) << "%FF";
    QTest::newRow("fF") << "%fF" << F(QUrl::FullyEncoded) << "%FF";
    QTest::newRow("ff") << "%ff" << F(QUrl::FullyEncoded) << "%FF";

    // decode UTF-8 mixed with non-UTF-8 and unreserved
    QTest::newRow("utf8-mix-1") << "%80%C2%80" << F(QUrl::PrettyDecoded) << QString::fromUtf8("%80\xC2\x80");
    QTest::newRow("utf8-mix-2") << "%C2%C2%80" << F(QUrl::PrettyDecoded) << QString::fromUtf8("%C2\xC2\x80");
    QTest::newRow("utf8-mix-3") << "%E0%C2%80" << F(QUrl::PrettyDecoded) << QString::fromUtf8("%E0\xC2\x80");
    QTest::newRow("utf8-mix-3") << "A%C2%80" << F(QUrl::PrettyDecoded) << QString::fromUtf8("A\xC2\x80");
    QTest::newRow("utf8-mix-3") << "%C2%80A" << F(QUrl::PrettyDecoded) << QString::fromUtf8("\xC2\x80""A");
}

void tst_QUrlInternal::encodingRecode()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);
    QFETCH(QUrl::ComponentFormattingOptions, encodingMode);

    // prepend some data to be sure that it remains there
    QString output = QTest::currentDataTag();
    expected.prepend(output);

    if (!qt_urlRecode(output, input.constData(), input.constData() + input.length(), encodingMode))
        output += input;
    QCOMPARE(output, expected);
}

void tst_QUrlInternal::encodingRecodeInvalidUtf8_data()
{
    QTest::addColumn<QByteArray>("utf8");
    QTest::addColumn<QString>("utf16");

    extern void loadInvalidUtf8Rows();
    extern void loadNonCharactersRows();
    loadInvalidUtf8Rows();
    loadNonCharactersRows();

    QTest::newRow("utf8-mix-4") << QByteArray("\xE0.A2\x80");
    QTest::newRow("utf8-mix-5") << QByteArray("\xE0\xA2.80");
    QTest::newRow("utf8-mix-6") << QByteArray("\xE0\xA2\x33");
}

void tst_QUrlInternal::encodingRecodeInvalidUtf8()
{
    QFETCH(QByteArray, utf8);
    QString input = utf8.toPercentEncoding();

    // prepend some data to be sure that it remains there
    QString output = QTest::currentDataTag();

    if (!qt_urlRecode(output, input.constData(), input.constData() + input.length(), QUrl::PrettyDecoded))
        output += input;
    QCOMPARE(output, QTest::currentDataTag() + input);

    // this is just control
    output = QTest::currentDataTag();
    if (!qt_urlRecode(output, input.constData(), input.constData() + input.length(), QUrl::FullyEncoded))
        output += input;
    QCOMPARE(output, QTest::currentDataTag() + input);

    // verify for security reasons that all bad UTF-8 data got replaced by QChar::ReplacementCharacter
    output = QTest::currentDataTag();
    if (!qt_urlRecode(output, input.constData(), input.constData() + input.length(), QUrl::FullyEncoded))
        output += input;
    for (int i = int(strlen(QTest::currentDataTag())); i < output.length(); ++i) {
        QVERIFY2(output.at(i).unicode() < 0x80 || output.at(i) == QChar::ReplacementCharacter,
                 qPrintable(QString("Character at i == %1 was U+%2").arg(i).arg(output.at(i).unicode(), 4, 16, QLatin1Char('0'))));
    }
}

void tst_QUrlInternal::recodeByteArray_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("null") << QByteArray() << QString();
    QTest::newRow("empty") << QByteArray("") << QString("");
    QTest::newRow("normal") << QByteArray("Hello") << "Hello";
    QTest::newRow("valid-utf8") << QByteArray("\xc3\xa9") << "%C3%A9";
    QTest::newRow("percent-encoded") << QByteArray("%C3%A9%00%C0%80") << "%C3%A9%00%C0%80";
    QTest::newRow("invalid-utf8-1") << QByteArray("\xc3\xc3") << "%C3%C3";
    QTest::newRow("invalid-utf8-2") << QByteArray("\xc0\x80") << "%C0%80";

    // note: percent-encoding the control characters ("\0" -> "%00") would also
    // be correct, but it's unnecessary for this function
    QTest::newRow("binary") << QByteArray("\0\x1f", 2) << QString::fromLatin1("\0\x1f", 2);;
    QTest::newRow("binary+percent-encoded") << QByteArray("\0%25", 4) << QString::fromLatin1("\0%25", 4);
}

void tst_QUrlInternal::recodeByteArray()
{
    QFETCH(QByteArray, input);
    QFETCH(QString, expected);
    QString output = qt_urlRecodeByteArray(input);

    QCOMPARE(output.isNull(), input.isNull());
    QCOMPARE(output.isEmpty(), input.isEmpty());
    QCOMPARE(output, expected);
}

QTEST_APPLESS_MAIN(tst_QUrlInternal)
