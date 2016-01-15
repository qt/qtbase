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

void loadInvalidUtf8Rows()
{
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
    QTest::newRow("non-unicode-4") << QByteArray("\xFD\xBF\xBF\xBF\xBF\xBF");

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
    QTest::newRow("overlong-2-5") << QByteArray("\xF8\x80\x80\x82\x80");
    // overlong 6: xxxx:xxz0 xz00:0000 xz00:0000 xz00:0000 xz00:0010 xz00:0000
    QTest::newRow("overlong-2-6") << QByteArray("\xFC\x80\x80\x80\x82\x80");

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

void loadNonCharactersRows()
{
    // Unicode has a couple of "non-characters" that one can use internally
    // These characters are allowed for text-interchange (see http://www.unicode.org/versions/corrigendum9.html)
    //
    // Those are the last two entries each Unicode Plane (U+FFFE, U+FFFF,
    // U+1FFFE, U+1FFFF, etc.) as well as the entries between U+FDD0 and
    // U+FDEF (inclusive)

    // U+FDD0 through U+FDEF
    for (int i = 0; i < 16; ++i) {
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
}
