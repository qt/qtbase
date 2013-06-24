/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <qchar.h>
#include <qfile.h>
#include <qstringlist.h>
#include <private/qunicodetables_p.h>
#if defined(Q_OS_WINCE)
#include <qcoreapplication.h>
#endif

class tst_QChar : public QObject
{
    Q_OBJECT
public slots:
    void init();
    void cleanup();
private slots:
    void toUpper();
    void toLower();
    void toTitle();
    void toCaseFolded();
    void isDigit_data();
    void isDigit();
    void isLetter_data();
    void isLetter();
    void isLetterOrNumber_data();
    void isLetterOrNumber();
    void isPrint();
    void isUpper();
    void isLower();
    void isTitleCase();
    void isSpace_data();
    void isSpace();
    void isSpaceSpecial();
    void category();
    void direction();
    void joining();
    void combiningClass();
    void digitValue();
    void mirroredChar();
    void decomposition();
    void lineBreakClass();
    void script();
    void normalization_data();
    void normalization();
    void normalization_manual();
    void normalizationCorrections();
    void unicodeVersion();
#if defined(Q_OS_WINCE)
private:
    QCoreApplication* app;
#endif
};

void tst_QChar::init()
{
#if defined(Q_OS_WINCE)
    int argc = 0;
    app = new QCoreApplication(argc, NULL);
#endif
}

void tst_QChar::cleanup()
{
#if defined(Q_OS_WINCE)
    delete app;
#endif
}

void tst_QChar::toUpper()
{
    QVERIFY(QChar('a').toUpper() == 'A');
    QVERIFY(QChar('A').toUpper() == 'A');
    QVERIFY(QChar((ushort)0x1c7).toUpper().unicode() == 0x1c7);
    QVERIFY(QChar((ushort)0x1c8).toUpper().unicode() == 0x1c7);
    QVERIFY(QChar((ushort)0x1c9).toUpper().unicode() == 0x1c7);
    QVERIFY(QChar((ushort)0x1d79).toUpper().unicode() == 0xa77d);
    QVERIFY(QChar((ushort)0x0265).toUpper().unicode() == 0xa78d);

    QVERIFY(QChar::toUpper((ushort)'a') == 'A');
    QVERIFY(QChar::toUpper((ushort)'A') == 'A');
    QVERIFY(QChar::toUpper((ushort)0x1c7) == 0x1c7);
    QVERIFY(QChar::toUpper((ushort)0x1c8) == 0x1c7);
    QVERIFY(QChar::toUpper((ushort)0x1c9) == 0x1c7);
    QVERIFY(QChar::toUpper((ushort)0x1d79) == 0xa77d);
    QVERIFY(QChar::toUpper((ushort)0x0265) == 0xa78d);

    QVERIFY(QChar::toUpper((uint)'a') == 'A');
    QVERIFY(QChar::toUpper((uint)'A') == 'A');
    QVERIFY(QChar::toUpper((uint)0xdf) == 0xdf); // german sharp s
    QVERIFY(QChar::toUpper((uint)0x1c7) == 0x1c7);
    QVERIFY(QChar::toUpper((uint)0x1c8) == 0x1c7);
    QVERIFY(QChar::toUpper((uint)0x1c9) == 0x1c7);
    QVERIFY(QChar::toUpper((uint)0x1d79) == 0xa77d);
    QVERIFY(QChar::toUpper((uint)0x0265) == 0xa78d);

    QVERIFY(QChar::toUpper((uint)0x10400) == 0x10400);
    QVERIFY(QChar::toUpper((uint)0x10428) == 0x10400);
}

void tst_QChar::toLower()
{
    QVERIFY(QChar('A').toLower() == 'a');
    QVERIFY(QChar('a').toLower() == 'a');
    QVERIFY(QChar((ushort)0x1c7).toLower().unicode() == 0x1c9);
    QVERIFY(QChar((ushort)0x1c8).toLower().unicode() == 0x1c9);
    QVERIFY(QChar((ushort)0x1c9).toLower().unicode() == 0x1c9);
    QVERIFY(QChar((ushort)0xa77d).toLower().unicode() == 0x1d79);
    QVERIFY(QChar((ushort)0xa78d).toLower().unicode() == 0x0265);

    QVERIFY(QChar::toLower((ushort)'a') == 'a');
    QVERIFY(QChar::toLower((ushort)'A') == 'a');
    QVERIFY(QChar::toLower((ushort)0x1c7) == 0x1c9);
    QVERIFY(QChar::toLower((ushort)0x1c8) == 0x1c9);
    QVERIFY(QChar::toLower((ushort)0x1c9) == 0x1c9);
    QVERIFY(QChar::toLower((ushort)0xa77d) == 0x1d79);
    QVERIFY(QChar::toLower((ushort)0xa78d) == 0x0265);

    QVERIFY(QChar::toLower((uint)'a') == 'a');
    QVERIFY(QChar::toLower((uint)'A') == 'a');
    QVERIFY(QChar::toLower((uint)0x1c7) == 0x1c9);
    QVERIFY(QChar::toLower((uint)0x1c8) == 0x1c9);
    QVERIFY(QChar::toLower((uint)0x1c9) == 0x1c9);
    QVERIFY(QChar::toLower((uint)0xa77d) == 0x1d79);
    QVERIFY(QChar::toLower((uint)0xa78d) == 0x0265);

    QVERIFY(QChar::toLower((uint)0x10400) == 0x10428);
    QVERIFY(QChar::toLower((uint)0x10428) == 0x10428);
}

void tst_QChar::toTitle()
{
    QVERIFY(QChar('a').toTitleCase() == 'A');
    QVERIFY(QChar('A').toTitleCase() == 'A');
    QVERIFY(QChar((ushort)0x1c7).toTitleCase().unicode() == 0x1c8);
    QVERIFY(QChar((ushort)0x1c8).toTitleCase().unicode() == 0x1c8);
    QVERIFY(QChar((ushort)0x1c9).toTitleCase().unicode() == 0x1c8);
    QVERIFY(QChar((ushort)0x1d79).toTitleCase().unicode() == 0xa77d);
    QVERIFY(QChar((ushort)0x0265).toTitleCase().unicode() == 0xa78d);

    QVERIFY(QChar::toTitleCase((ushort)'a') == 'A');
    QVERIFY(QChar::toTitleCase((ushort)'A') == 'A');
    QVERIFY(QChar::toTitleCase((ushort)0x1c7) == 0x1c8);
    QVERIFY(QChar::toTitleCase((ushort)0x1c8) == 0x1c8);
    QVERIFY(QChar::toTitleCase((ushort)0x1c9) == 0x1c8);
    QVERIFY(QChar::toTitleCase((ushort)0x1d79) == 0xa77d);
    QVERIFY(QChar::toTitleCase((ushort)0x0265) == 0xa78d);

    QVERIFY(QChar::toTitleCase((uint)'a') == 'A');
    QVERIFY(QChar::toTitleCase((uint)'A') == 'A');
    QVERIFY(QChar::toTitleCase((uint)0xdf) == 0xdf); // german sharp s
    QVERIFY(QChar::toTitleCase((uint)0x1c7) == 0x1c8);
    QVERIFY(QChar::toTitleCase((uint)0x1c8) == 0x1c8);
    QVERIFY(QChar::toTitleCase((uint)0x1c9) == 0x1c8);
    QVERIFY(QChar::toTitleCase((uint)0x1d79) == 0xa77d);
    QVERIFY(QChar::toTitleCase((uint)0x0265) == 0xa78d);

    QVERIFY(QChar::toTitleCase((uint)0x10400) == 0x10400);
    QVERIFY(QChar::toTitleCase((uint)0x10428) == 0x10400);
}

void tst_QChar::toCaseFolded()
{
    QVERIFY(QChar('a').toCaseFolded() == 'a');
    QVERIFY(QChar('A').toCaseFolded() == 'a');
    QVERIFY(QChar((ushort)0x1c7).toCaseFolded().unicode() == 0x1c9);
    QVERIFY(QChar((ushort)0x1c8).toCaseFolded().unicode() == 0x1c9);
    QVERIFY(QChar((ushort)0x1c9).toCaseFolded().unicode() == 0x1c9);
    QVERIFY(QChar((ushort)0xa77d).toCaseFolded().unicode() == 0x1d79);
    QVERIFY(QChar((ushort)0xa78d).toCaseFolded().unicode() == 0x0265);

    QVERIFY(QChar::toCaseFolded((ushort)'a') == 'a');
    QVERIFY(QChar::toCaseFolded((ushort)'A') == 'a');
    QVERIFY(QChar::toCaseFolded((ushort)0x1c7) == 0x1c9);
    QVERIFY(QChar::toCaseFolded((ushort)0x1c8) == 0x1c9);
    QVERIFY(QChar::toCaseFolded((ushort)0x1c9) == 0x1c9);
    QVERIFY(QChar::toCaseFolded((ushort)0xa77d) == 0x1d79);
    QVERIFY(QChar::toCaseFolded((ushort)0xa78d) == 0x0265);

    QVERIFY(QChar::toCaseFolded((uint)'a') == 'a');
    QVERIFY(QChar::toCaseFolded((uint)'A') == 'a');
    QVERIFY(QChar::toCaseFolded((uint)0x1c7) == 0x1c9);
    QVERIFY(QChar::toCaseFolded((uint)0x1c8) == 0x1c9);
    QVERIFY(QChar::toCaseFolded((uint)0x1c9) == 0x1c9);
    QVERIFY(QChar::toCaseFolded((uint)0xa77d) == 0x1d79);
    QVERIFY(QChar::toCaseFolded((uint)0xa78d) == 0x0265);

    QVERIFY(QChar::toCaseFolded((uint)0x10400) == 0x10428);
    QVERIFY(QChar::toCaseFolded((uint)0x10428) == 0x10428);

    QVERIFY(QChar::toCaseFolded((ushort)0xb5) == 0x3bc);
}

void tst_QChar::isDigit_data()
{
    QTest::addColumn<ushort>("ucs");
    QTest::addColumn<bool>("expected");

    for (ushort ucs = 0; ucs < 256; ++ucs) {
        bool isDigit = (ucs <= '9' && ucs >= '0');
        QString tag = QString::fromLatin1("0x%0").arg(QString::number(ucs, 16));
        QTest::newRow(tag.toLatin1()) << ucs << isDigit;
    }
}

void tst_QChar::isDigit()
{
    QFETCH(ushort, ucs);
    QFETCH(bool, expected);
    QCOMPARE(QChar(ucs).isDigit(), expected);
}

static bool isExpectedLetter(ushort ucs)
{
    return (ucs >= 'a' && ucs <= 'z') || (ucs >= 'A' && ucs <= 'Z')
            || ucs == 0xAA || ucs == 0xB5 || ucs == 0xBA
            || (ucs >= 0xC0 && ucs <= 0xD6)
            || (ucs >= 0xD8 && ucs <= 0xF6)
            || (ucs >= 0xF8 && ucs <= 0xFF);
}

void tst_QChar::isLetter_data()
{
    QTest::addColumn<ushort>("ucs");
    QTest::addColumn<bool>("expected");

    for (ushort ucs = 0; ucs < 256; ++ucs) {
        QString tag = QString::fromLatin1("0x%0").arg(QString::number(ucs, 16));
        QTest::newRow(tag.toLatin1()) << ucs << isExpectedLetter(ucs);
    }
}

void tst_QChar::isLetter()
{
    QFETCH(ushort, ucs);
    QFETCH(bool, expected);
    QCOMPARE(QChar(ucs).isLetter(), expected);
}

void tst_QChar::isLetterOrNumber_data()
{
    QTest::addColumn<ushort>("ucs");
    QTest::addColumn<bool>("expected");

    for (ushort ucs = 0; ucs < 256; ++ucs) {
        bool isLetterOrNumber = isExpectedLetter(ucs)
                || (ucs >= '0' && ucs <= '9')
                || ucs == 0xB2 || ucs == 0xB3 || ucs == 0xB9
                || (ucs >= 0xBC && ucs <= 0xBE);
        QString tag = QString::fromLatin1("0x%0").arg(QString::number(ucs, 16));
        QTest::newRow(tag.toLatin1()) << ucs << isLetterOrNumber;
    }
}

void tst_QChar::isLetterOrNumber()
{
    QFETCH(ushort, ucs);
    QFETCH(bool, expected);
    QCOMPARE(QChar(ucs).isLetterOrNumber(), expected);
}

void tst_QChar::isPrint()
{
    // noncharacters, reserved (General_Gategory =Cn)
    QVERIFY(!QChar(0x2064).isPrint());
    QVERIFY(!QChar(0x2069).isPrint());
    QVERIFY(!QChar(0xfdd0).isPrint());
    QVERIFY(!QChar(0xfdef).isPrint());
    QVERIFY(!QChar(0xfff0).isPrint());
    QVERIFY(!QChar(0xfff8).isPrint());
    QVERIFY(!QChar(0xfffe).isPrint());
    QVERIFY(!QChar(0xffff).isPrint());
    QVERIFY(!QChar::isPrint(0xe0000u));
    QVERIFY(!QChar::isPrint(0xe0002u));
    QVERIFY(!QChar::isPrint(0xe001fu));
    QVERIFY(!QChar::isPrint(0xe0080u));
    QVERIFY(!QChar::isPrint(0xe00ffu));

    // Other_Default_Ignorable_Code_Point, Variation_Selector
    QVERIFY(QChar(0x034f).isPrint());
    QVERIFY(QChar(0x115f).isPrint());
    QVERIFY(QChar(0x180b).isPrint());
    QVERIFY(QChar(0x180d).isPrint());
    QVERIFY(QChar(0x3164).isPrint());
    QVERIFY(QChar(0xfe00).isPrint());
    QVERIFY(QChar(0xfe0f).isPrint());
    QVERIFY(QChar(0xffa0).isPrint());
    QVERIFY(QChar::isPrint(0xe0100u));
    QVERIFY(QChar::isPrint(0xe01efu));

    // Cf, Cs, Cc, White_Space, Annotation Characters
    QVERIFY(!QChar(0x0008).isPrint());
    QVERIFY(!QChar(0x000a).isPrint());
    QVERIFY(QChar(0x0020).isPrint());
    QVERIFY(QChar(0x00a0).isPrint());
    QVERIFY(!QChar(0x00ad).isPrint());
    QVERIFY(!QChar(0x0085).isPrint());
    QVERIFY(!QChar(0xd800).isPrint());
    QVERIFY(!QChar(0xdc00).isPrint());
    QVERIFY(!QChar(0xfeff).isPrint());
    QVERIFY(!QChar::isPrint(0x1d173u));

    QVERIFY(QChar('0').isPrint());
    QVERIFY(QChar('A').isPrint());
    QVERIFY(QChar('a').isPrint());

    QVERIFY(QChar(0x0370).isPrint()); // assigned in 5.1
    QVERIFY(QChar(0x0524).isPrint()); // assigned in 5.2
    QVERIFY(QChar(0x0526).isPrint()); // assigned in 6.0
    QVERIFY(QChar(0x08a0).isPrint()); // assigned in 6.1
    QVERIFY(!QChar(0x1aff).isPrint()); // not assigned
    QVERIFY(QChar(0x1e9e).isPrint()); // assigned in 5.1
    QVERIFY(QChar::isPrint(0x1b000u)); // assigned in 6.0
    QVERIFY(QChar::isPrint(0x110d0u)); // assigned in 5.1
}

void tst_QChar::isUpper()
{
    QVERIFY(QChar('A').isUpper());
    QVERIFY(QChar('Z').isUpper());
    QVERIFY(!QChar('a').isUpper());
    QVERIFY(!QChar('z').isUpper());
    QVERIFY(!QChar('?').isUpper());
    QVERIFY(QChar(0xC2).isUpper());   // A with ^
    QVERIFY(!QChar(0xE2).isUpper());  // a with ^

    for (uint codepoint = 0; codepoint <= QChar::LastValidCodePoint; ++codepoint) {
        if (QChar::isUpper(codepoint))
            QVERIFY(codepoint == QChar::toUpper(codepoint));
    }
}

void tst_QChar::isLower()
{
    QVERIFY(!QChar('A').isLower());
    QVERIFY(!QChar('Z').isLower());
    QVERIFY(QChar('a').isLower());
    QVERIFY(QChar('z').isLower());
    QVERIFY(!QChar('?').isLower());
    QVERIFY(!QChar(0xC2).isLower());   // A with ^
    QVERIFY(QChar(0xE2).isLower());  // a with ^

    for (uint codepoint = 0; codepoint <= QChar::LastValidCodePoint; ++codepoint) {
        if (QChar::isLower(codepoint))
            QVERIFY(codepoint == QChar::toLower(codepoint));
    }
}

void tst_QChar::isTitleCase()
{
    for (uint codepoint = 0; codepoint <= QChar::LastValidCodePoint; ++codepoint) {
        if (QChar::isTitleCase(codepoint))
            QVERIFY(codepoint == QChar::toTitleCase(codepoint));
    }
}

void tst_QChar::isSpace_data()
{
    QTest::addColumn<ushort>("ucs");
    QTest::addColumn<bool>("expected");

    for (ushort ucs = 0; ucs < 256; ++ucs) {
        bool isSpace = (ucs <= 0x0D && ucs >= 0x09) || ucs == 0x20 || ucs == 0xA0 || ucs == 0x85;
        QString tag = QString::fromLatin1("0x%0").arg(QString::number(ucs, 16));
        QTest::newRow(tag.toLatin1()) << ucs << isSpace;
    }
}

void tst_QChar::isSpace()
{
    QFETCH(ushort, ucs);
    QFETCH(bool, expected);
    QCOMPARE(QChar(ucs).isSpace(), expected);
}

void tst_QChar::isSpaceSpecial()
{
    QVERIFY(!QChar(QChar::Null).isSpace());
    QVERIFY(QChar(QChar::Nbsp).isSpace());
    QVERIFY(QChar(QChar::ParagraphSeparator).isSpace());
    QVERIFY(QChar(QChar::LineSeparator).isSpace());
    QVERIFY(QChar(0x1680).isSpace());
}

void tst_QChar::category()
{
    QVERIFY(QChar('a').category() == QChar::Letter_Lowercase);
    QVERIFY(QChar('A').category() == QChar::Letter_Uppercase);

    QVERIFY(QChar::category((ushort)'a') == QChar::Letter_Lowercase);
    QVERIFY(QChar::category((ushort)'A') == QChar::Letter_Uppercase);

    QVERIFY(QChar::category((uint)'a') == QChar::Letter_Lowercase);
    QVERIFY(QChar::category((uint)'A') == QChar::Letter_Uppercase);

    QVERIFY(QChar::category(0xe0100u) == QChar::Mark_NonSpacing);
    QVERIFY(QChar::category(0xeffffu) != QChar::Other_PrivateUse);
    QVERIFY(QChar::category(0xf0000u) == QChar::Other_PrivateUse);
    QVERIFY(QChar::category(0xf0001u) == QChar::Other_PrivateUse);

    QVERIFY(QChar::category(0xd900u) == QChar::Other_Surrogate);
    QVERIFY(QChar::category(0xdc00u) == QChar::Other_Surrogate);
    QVERIFY(QChar::category(0xdc01u) == QChar::Other_Surrogate);

    QVERIFY(QChar::category((uint)0x1aff) == QChar::Other_NotAssigned);
    QVERIFY(QChar::category((uint)0x10fffdu) == QChar::Other_PrivateUse);
    QVERIFY(QChar::category((uint)0x10ffffu) == QChar::Other_NotAssigned);
    QVERIFY(QChar::category((uint)0x110000u) == QChar::Other_NotAssigned);
}

void tst_QChar::direction()
{
    QVERIFY(QChar('a').direction() == QChar::DirL);
    QVERIFY(QChar('0').direction() == QChar::DirEN);
    QVERIFY(QChar((ushort)0x627).direction() == QChar::DirAL);
    QVERIFY(QChar((ushort)0x5d0).direction() == QChar::DirR);

    QVERIFY(QChar::direction((ushort)'a') == QChar::DirL);
    QVERIFY(QChar::direction((ushort)'0') == QChar::DirEN);
    QVERIFY(QChar::direction((ushort)0x627) == QChar::DirAL);
    QVERIFY(QChar::direction((ushort)0x5d0) == QChar::DirR);

    QVERIFY(QChar::direction((uint)'a') == QChar::DirL);
    QVERIFY(QChar::direction((uint)'0') == QChar::DirEN);
    QVERIFY(QChar::direction((uint)0x627) == QChar::DirAL);
    QVERIFY(QChar::direction((uint)0x5d0) == QChar::DirR);

    QVERIFY(QChar::direction(0xE01DAu) == QChar::DirNSM);
    QVERIFY(QChar::direction(0xf0000u) == QChar::DirL);
    QVERIFY(QChar::direction(0xE0030u) == QChar::DirBN);
    QVERIFY(QChar::direction(0x2FA17u) == QChar::DirL);
}

void tst_QChar::joining()
{
    QVERIFY(QChar('a').joining() == QChar::OtherJoining);
    QVERIFY(QChar('0').joining() == QChar::OtherJoining);
    QVERIFY(QChar((ushort)0x627).joining() == QChar::Right);
    QVERIFY(QChar((ushort)0x5d0).joining() == QChar::OtherJoining);

    QVERIFY(QChar::joining((ushort)'a') == QChar::OtherJoining);
    QVERIFY(QChar::joining((ushort)'0') == QChar::OtherJoining);
    QVERIFY(QChar::joining((ushort)0x627) == QChar::Right);
    QVERIFY(QChar::joining((ushort)0x5d0) == QChar::OtherJoining);

    QVERIFY(QChar::joining((uint)'a') == QChar::OtherJoining);
    QVERIFY(QChar::joining((uint)'0') == QChar::OtherJoining);
    QVERIFY(QChar::joining((uint)0x627) == QChar::Right);
    QVERIFY(QChar::joining((uint)0x5d0) == QChar::OtherJoining);

    QVERIFY(QChar::joining(0xE01DAu) == QChar::OtherJoining);
    QVERIFY(QChar::joining(0xf0000u) == QChar::OtherJoining);
    QVERIFY(QChar::joining(0xE0030u) == QChar::OtherJoining);
    QVERIFY(QChar::joining(0x2FA17u) == QChar::OtherJoining);
}

void tst_QChar::combiningClass()
{
    QVERIFY(QChar('a').combiningClass() == 0);
    QVERIFY(QChar('0').combiningClass() == 0);
    QVERIFY(QChar((ushort)0x627).combiningClass() == 0);
    QVERIFY(QChar((ushort)0x5d0).combiningClass() == 0);

    QVERIFY(QChar::combiningClass((ushort)'a') == 0);
    QVERIFY(QChar::combiningClass((ushort)'0') == 0);
    QVERIFY(QChar::combiningClass((ushort)0x627) == 0);
    QVERIFY(QChar::combiningClass((ushort)0x5d0) == 0);

    QVERIFY(QChar::combiningClass((uint)'a') == 0);
    QVERIFY(QChar::combiningClass((uint)'0') == 0);
    QVERIFY(QChar::combiningClass((uint)0x627) == 0);
    QVERIFY(QChar::combiningClass((uint)0x5d0) == 0);

    QVERIFY(QChar::combiningClass(0xE01DAu) == 0);
    QVERIFY(QChar::combiningClass(0xf0000u) == 0);
    QVERIFY(QChar::combiningClass(0xE0030u) == 0);
    QVERIFY(QChar::combiningClass(0x2FA17u) == 0);

    QVERIFY(QChar::combiningClass((ushort)0x300) == 230);
    QVERIFY(QChar::combiningClass((uint)0x300) == 230);

    QVERIFY(QChar::combiningClass((uint)0x1d244) == 230);

}

void tst_QChar::unicodeVersion()
{
    QVERIFY(QChar('a').unicodeVersion() == QChar::Unicode_1_1);
    QVERIFY(QChar('0').unicodeVersion() == QChar::Unicode_1_1);
    QVERIFY(QChar((ushort)0x627).unicodeVersion() == QChar::Unicode_1_1);
    QVERIFY(QChar((ushort)0x5d0).unicodeVersion() == QChar::Unicode_1_1);

    QVERIFY(QChar::unicodeVersion((ushort)'a') == QChar::Unicode_1_1);
    QVERIFY(QChar::unicodeVersion((ushort)'0') == QChar::Unicode_1_1);
    QVERIFY(QChar::unicodeVersion((ushort)0x627) == QChar::Unicode_1_1);
    QVERIFY(QChar::unicodeVersion((ushort)0x5d0) == QChar::Unicode_1_1);

    QVERIFY(QChar::unicodeVersion((uint)'a') == QChar::Unicode_1_1);
    QVERIFY(QChar::unicodeVersion((uint)'0') == QChar::Unicode_1_1);
    QVERIFY(QChar::unicodeVersion((uint)0x627) == QChar::Unicode_1_1);
    QVERIFY(QChar::unicodeVersion((uint)0x5d0) == QChar::Unicode_1_1);

    QVERIFY(QChar(0x0591).unicodeVersion() == QChar::Unicode_2_0);
    QVERIFY(QChar::unicodeVersion((ushort)0x0591) == QChar::Unicode_2_0);
    QVERIFY(QChar::unicodeVersion((uint)0x0591) == QChar::Unicode_2_0);

    QVERIFY(QChar(0x20AC).unicodeVersion() == QChar::Unicode_2_1_2);
    QVERIFY(QChar::unicodeVersion((ushort)0x020AC) == QChar::Unicode_2_1_2);
    QVERIFY(QChar::unicodeVersion((uint)0x20AC) == QChar::Unicode_2_1_2);
    QVERIFY(QChar(0xfffc).unicodeVersion() == QChar::Unicode_2_1_2);
    QVERIFY(QChar::unicodeVersion((ushort)0x0fffc) == QChar::Unicode_2_1_2);
    QVERIFY(QChar::unicodeVersion((uint)0xfffc) == QChar::Unicode_2_1_2);

    QVERIFY(QChar(0x01f6).unicodeVersion() == QChar::Unicode_3_0);
    QVERIFY(QChar::unicodeVersion((ushort)0x01f6) == QChar::Unicode_3_0);
    QVERIFY(QChar::unicodeVersion((uint)0x01f6) == QChar::Unicode_3_0);

    QVERIFY(QChar(0x03F4).unicodeVersion() == QChar::Unicode_3_1);
    QVERIFY(QChar::unicodeVersion((ushort)0x03F4) == QChar::Unicode_3_1);
    QVERIFY(QChar::unicodeVersion((uint)0x03F4) == QChar::Unicode_3_1);
    QVERIFY(QChar::unicodeVersion((uint)0x10300) == QChar::Unicode_3_1);

    QVERIFY(QChar(0x0220).unicodeVersion() == QChar::Unicode_3_2);
    QVERIFY(QChar::unicodeVersion((ushort)0x0220) == QChar::Unicode_3_2);
    QVERIFY(QChar::unicodeVersion((uint)0x0220) == QChar::Unicode_3_2);
    QVERIFY(QChar::unicodeVersion((uint)0xFF5F) == QChar::Unicode_3_2);

    QVERIFY(QChar(0x0221).unicodeVersion() == QChar::Unicode_4_0);
    QVERIFY(QChar::unicodeVersion((ushort)0x0221) == QChar::Unicode_4_0);
    QVERIFY(QChar::unicodeVersion((uint)0x0221) == QChar::Unicode_4_0);
    QVERIFY(QChar::unicodeVersion((uint)0x10000) == QChar::Unicode_4_0);

    QVERIFY(QChar(0x0237).unicodeVersion() == QChar::Unicode_4_1);
    QVERIFY(QChar::unicodeVersion((ushort)0x0237) == QChar::Unicode_4_1);
    QVERIFY(QChar::unicodeVersion((uint)0x0237) == QChar::Unicode_4_1);
    QVERIFY(QChar::unicodeVersion((uint)0x10140) == QChar::Unicode_4_1);

    QVERIFY(QChar(0x0242).unicodeVersion() == QChar::Unicode_5_0);
    QVERIFY(QChar::unicodeVersion((ushort)0x0242) == QChar::Unicode_5_0);
    QVERIFY(QChar::unicodeVersion((uint)0x0242) == QChar::Unicode_5_0);
    QVERIFY(QChar::unicodeVersion((uint)0x12000) == QChar::Unicode_5_0);

    QVERIFY(QChar(0x0370).unicodeVersion() == QChar::Unicode_5_1);
    QVERIFY(QChar::unicodeVersion((ushort)0x0370) == QChar::Unicode_5_1);
    QVERIFY(QChar::unicodeVersion((uint)0x0370) == QChar::Unicode_5_1);
    QVERIFY(QChar::unicodeVersion((uint)0x1f093) == QChar::Unicode_5_1);

    QVERIFY(QChar(0x0524).unicodeVersion() == QChar::Unicode_5_2);
    QVERIFY(QChar::unicodeVersion((ushort)0x0524) == QChar::Unicode_5_2);
    QVERIFY(QChar::unicodeVersion((uint)0x0524) == QChar::Unicode_5_2);
    QVERIFY(QChar::unicodeVersion((uint)0x2b734) == QChar::Unicode_5_2);

    QVERIFY(QChar(0x26ce).unicodeVersion() == QChar::Unicode_6_0);
    QVERIFY(QChar::unicodeVersion((ushort)0x26ce) == QChar::Unicode_6_0);
    QVERIFY(QChar::unicodeVersion((uint)0x26ce) == QChar::Unicode_6_0);
    QVERIFY(QChar::unicodeVersion((uint)0x1f618) == QChar::Unicode_6_0);

    QVERIFY(QChar(0xa69f).unicodeVersion() == QChar::Unicode_6_1);
    QVERIFY(QChar::unicodeVersion((ushort)0xa69f) == QChar::Unicode_6_1);
    QVERIFY(QChar::unicodeVersion((uint)0xa69f) == QChar::Unicode_6_1);
    QVERIFY(QChar::unicodeVersion((uint)0x1f600) == QChar::Unicode_6_1);

    QVERIFY(QChar(0x20ba).unicodeVersion() == QChar::Unicode_6_2);
    QVERIFY(QChar::unicodeVersion((ushort)0x20ba) == QChar::Unicode_6_2);
    QVERIFY(QChar::unicodeVersion((uint)0x20ba) == QChar::Unicode_6_2);
    QVERIFY(QChar::unicodeVersion((uint)0x20ba) == QChar::Unicode_6_2);

    QVERIFY(QChar(0x09ff).unicodeVersion() == QChar::Unicode_Unassigned);
    QVERIFY(QChar::unicodeVersion((ushort)0x09ff) == QChar::Unicode_Unassigned);
    QVERIFY(QChar::unicodeVersion((uint)0x09ff) == QChar::Unicode_Unassigned);
    QVERIFY(QChar::unicodeVersion((uint)0x110000) == QChar::Unicode_Unassigned);
}

void tst_QChar::digitValue()
{
    QVERIFY(QChar('9').digitValue() == 9);
    QVERIFY(QChar('0').digitValue() == 0);
    QVERIFY(QChar('a').digitValue() == -1);

    QVERIFY(QChar::digitValue((ushort)'9') == 9);
    QVERIFY(QChar::digitValue((ushort)'0') == 0);
    QVERIFY(QChar::digitValue((uint)'9') == 9);
    QVERIFY(QChar::digitValue((uint)'0') == 0);

    QVERIFY(QChar::digitValue((ushort)0x1049) == 9);
    QVERIFY(QChar::digitValue((ushort)0x1040) == 0);
    QVERIFY(QChar::digitValue((uint)0x1049) == 9);
    QVERIFY(QChar::digitValue((uint)0x1040) == 0);

    QVERIFY(QChar::digitValue((ushort)0xd800) == -1);
    QVERIFY(QChar::digitValue((uint)0x110000u) == -1);
}

void tst_QChar::mirroredChar()
{
    QVERIFY(QChar(0x169B).hasMirrored());
    QVERIFY(QChar(0x169B).mirroredChar() == QChar(0x169C));
    QVERIFY(QChar(0x169C).hasMirrored());
    QVERIFY(QChar(0x169C).mirroredChar() == QChar(0x169B));

    QVERIFY(QChar(0x301A).hasMirrored());
    QVERIFY(QChar(0x301A).mirroredChar() == QChar(0x301B));
    QVERIFY(QChar(0x301B).hasMirrored());
    QVERIFY(QChar(0x301B).mirroredChar() == QChar(0x301A));
}

void tst_QChar::decomposition()
{
    // Hangul syllables
    for (uint ucs = 0xac00; ucs <= 0xd7af; ++ucs) {
        QChar::Decomposition expected = QChar::unicodeVersion(ucs) > QChar::Unicode_Unassigned ? QChar::Canonical : QChar::NoDecomposition;
        QString desc = QString::fromLatin1("ucs = 0x%1, tag = %2, expected = %3")
                .arg(QString::number(ucs, 16)).arg(QChar::decompositionTag(ucs)).arg(expected);
        QVERIFY2(QChar::decompositionTag(ucs) == expected, desc.toLatin1());
    }

    QVERIFY(QChar((ushort)0xa0).decompositionTag() == QChar::NoBreak);
    QVERIFY(QChar((ushort)0xa8).decompositionTag() == QChar::Compat);
    QVERIFY(QChar((ushort)0x41).decompositionTag() == QChar::NoDecomposition);

    QVERIFY(QChar::decompositionTag(0xa0) == QChar::NoBreak);
    QVERIFY(QChar::decompositionTag(0xa8) == QChar::Compat);
    QVERIFY(QChar::decompositionTag(0x41) == QChar::NoDecomposition);

    QVERIFY(QChar::decomposition(0xa0) == QString(QChar(0x20)));
    QVERIFY(QChar::decomposition(0xc0) == (QString(QChar(0x41)) + QString(QChar(0x300))));

    {
        QString str;
        str += QChar(QChar::highSurrogate(0x1D157));
        str += QChar(QChar::lowSurrogate(0x1D157));
        str += QChar(QChar::highSurrogate(0x1D165));
        str += QChar(QChar::lowSurrogate(0x1D165));
        QVERIFY(QChar::decomposition(0x1D15e) == str);
    }

    {
        QString str;
        str += QChar(0x1100);
        str += QChar(0x1161);
        QVERIFY(QChar::decomposition(0xac00) == str);
    }
    {
        QString str;
        str += QChar(0x110c);
        str += QChar(0x1165);
        str += QChar(0x11b7);
        QVERIFY(QChar::decomposition(0xc810) == str);
    }
}

void tst_QChar::lineBreakClass()
{
    QVERIFY(QUnicodeTables::lineBreakClass(0x0029u) == QUnicodeTables::LineBreak_CP);
    QVERIFY(QUnicodeTables::lineBreakClass(0x0041u) == QUnicodeTables::LineBreak_AL);
    QVERIFY(QUnicodeTables::lineBreakClass(0x0033u) == QUnicodeTables::LineBreak_NU);
    QVERIFY(QUnicodeTables::lineBreakClass(0x00adu) == QUnicodeTables::LineBreak_BA);
    QVERIFY(QUnicodeTables::lineBreakClass(0x05d0u) == QUnicodeTables::LineBreak_HL);
    QVERIFY(QUnicodeTables::lineBreakClass(0xfffcu) == QUnicodeTables::LineBreak_CB);
    QVERIFY(QUnicodeTables::lineBreakClass(0xe0164u) == QUnicodeTables::LineBreak_CM);
    QVERIFY(QUnicodeTables::lineBreakClass(0x2f9a4u) == QUnicodeTables::LineBreak_ID);
    QVERIFY(QUnicodeTables::lineBreakClass(0x10000u) == QUnicodeTables::LineBreak_AL);
    QVERIFY(QUnicodeTables::lineBreakClass(0x1f1e6u) == QUnicodeTables::LineBreak_RI);

    // mapped to AL:
    QVERIFY(QUnicodeTables::lineBreakClass(0xfffdu) == QUnicodeTables::LineBreak_AL); // AI -> AL
    QVERIFY(QUnicodeTables::lineBreakClass(0x100000u) == QUnicodeTables::LineBreak_AL); // XX -> AL
}

void tst_QChar::script()
{
    QVERIFY(QChar::script(0x0020u) == QChar::Script_Common);
    QVERIFY(QChar::script(0x0041u) == QChar::Script_Latin);
    QVERIFY(QChar::script(0x0375u) == QChar::Script_Greek);
    QVERIFY(QChar::script(0x0400u) == QChar::Script_Cyrillic);
    QVERIFY(QChar::script(0x0531u) == QChar::Script_Armenian);
    QVERIFY(QChar::script(0x0591u) == QChar::Script_Hebrew);
    QVERIFY(QChar::script(0x0600u) == QChar::Script_Arabic);
    QVERIFY(QChar::script(0x0700u) == QChar::Script_Syriac);
    QVERIFY(QChar::script(0x0780u) == QChar::Script_Thaana);
    QVERIFY(QChar::script(0x07c0u) == QChar::Script_Nko);
    QVERIFY(QChar::script(0x0900u) == QChar::Script_Devanagari);
    QVERIFY(QChar::script(0x0981u) == QChar::Script_Bengali);
    QVERIFY(QChar::script(0x0a01u) == QChar::Script_Gurmukhi);
    QVERIFY(QChar::script(0x0a81u) == QChar::Script_Gujarati);
    QVERIFY(QChar::script(0x0b01u) == QChar::Script_Oriya);
    QVERIFY(QChar::script(0x0b82u) == QChar::Script_Tamil);
    QVERIFY(QChar::script(0x0c01u) == QChar::Script_Telugu);
    QVERIFY(QChar::script(0x0c82u) == QChar::Script_Kannada);
    QVERIFY(QChar::script(0x0d02u) == QChar::Script_Malayalam);
    QVERIFY(QChar::script(0x0d82u) == QChar::Script_Sinhala);
    QVERIFY(QChar::script(0x0e01u) == QChar::Script_Thai);
    QVERIFY(QChar::script(0x0e81u) == QChar::Script_Lao);
    QVERIFY(QChar::script(0x0f00u) == QChar::Script_Tibetan);
    QVERIFY(QChar::script(0x1000u) == QChar::Script_Myanmar);
    QVERIFY(QChar::script(0x10a0u) == QChar::Script_Georgian);
    QVERIFY(QChar::script(0x1100u) == QChar::Script_Hangul);
    QVERIFY(QChar::script(0x1680u) == QChar::Script_Ogham);
    QVERIFY(QChar::script(0x16a0u) == QChar::Script_Runic);
    QVERIFY(QChar::script(0x1780u) == QChar::Script_Khmer);
    QVERIFY(QChar::script(0x200cu) == QChar::Script_Inherited);
    QVERIFY(QChar::script(0x200du) == QChar::Script_Inherited);
    QVERIFY(QChar::script(0x1018au) == QChar::Script_Greek);
    QVERIFY(QChar::script(0x1f130u) == QChar::Script_Common);
    QVERIFY(QChar::script(0xe0100u) == QChar::Script_Inherited);
}

void tst_QChar::normalization_data()
{
    QTest::addColumn<QStringList>("columns");
    QTest::addColumn<int>("part");

    int linenum = 0;
    int part = 0;

    QString testFile = QFINDTESTDATA("data/NormalizationTest.txt");
    QVERIFY2(!testFile.isEmpty(), "data/NormalizationTest.txt not found!");
    QFile f(testFile);
    QVERIFY(f.exists());

    f.open(QIODevice::ReadOnly);

    while (!f.atEnd()) {
        linenum++;

        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);

        if (line.startsWith("@")) {
            if (line.startsWith("@Part") && line.size() > 5 && QChar(line.at(5)).isDigit())
                part = QChar(line.at(5)).digitValue();
            continue;
        }

        if (line.isEmpty())
            continue;

        line = line.trimmed();
        if (line.endsWith(';'))
            line.truncate(line.length()-1);

        QList<QByteArray> l = line.split(';');

        QCOMPARE(l.size(), 5);

        QStringList columns;
        for (int i = 0; i < 5; ++i) {
            columns.append(QString());

            QList<QByteArray> c = l.at(i).split(' ');
            QVERIFY(!c.isEmpty());

            for (int j = 0; j < c.size(); ++j) {
                bool ok;
                uint uc = c.at(j).toInt(&ok, 16);
                if (!QChar::requiresSurrogates(uc)) {
                    columns[i].append(QChar(uc));
                } else {
                    // convert to utf16
                    columns[i].append(QChar(QChar::highSurrogate(uc)));
                    columns[i].append(QChar(QChar::lowSurrogate(uc)));
                }
            }
        }

        QString nm = QString("line #%1 (part %2").arg(linenum).arg(part);
        QTest::newRow(nm.toLatin1()) << columns << part;
    }
}

void tst_QChar::normalization()
{
    QFETCH(QStringList, columns);
    QFETCH(int, part);

    Q_UNUSED(part)

        // CONFORMANCE:
        // 1. The following invariants must be true for all conformant implementations
        //
        //    NFC
        //      c2 ==  NFC(c1) ==  NFC(c2) ==  NFC(c3)
        //      c4 ==  NFC(c4) ==  NFC(c5)

        QVERIFY(columns[1] == columns[0].normalized(QString::NormalizationForm_C));
        QVERIFY(columns[1] == columns[1].normalized(QString::NormalizationForm_C));
        QVERIFY(columns[1] == columns[2].normalized(QString::NormalizationForm_C));
        QVERIFY(columns[3] == columns[3].normalized(QString::NormalizationForm_C));
        QVERIFY(columns[3] == columns[4].normalized(QString::NormalizationForm_C));

        //    NFD
        //      c3 ==  NFD(c1) ==  NFD(c2) ==  NFD(c3)
        //      c5 ==  NFD(c4) ==  NFD(c5)

        QVERIFY(columns[2] == columns[0].normalized(QString::NormalizationForm_D));
        QVERIFY(columns[2] == columns[1].normalized(QString::NormalizationForm_D));
        QVERIFY(columns[2] == columns[2].normalized(QString::NormalizationForm_D));
        QVERIFY(columns[4] == columns[3].normalized(QString::NormalizationForm_D));
        QVERIFY(columns[4] == columns[4].normalized(QString::NormalizationForm_D));

        //    NFKC
        //      c4 == NFKC(c1) == NFKC(c2) == NFKC(c3) == NFKC(c4) == NFKC(c5)

        QVERIFY(columns[3] == columns[0].normalized(QString::NormalizationForm_KC));
        QVERIFY(columns[3] == columns[1].normalized(QString::NormalizationForm_KC));
        QVERIFY(columns[3] == columns[2].normalized(QString::NormalizationForm_KC));
        QVERIFY(columns[3] == columns[3].normalized(QString::NormalizationForm_KC));
        QVERIFY(columns[3] == columns[4].normalized(QString::NormalizationForm_KC));

        //    NFKD
        //      c5 == NFKD(c1) == NFKD(c2) == NFKD(c3) == NFKD(c4) == NFKD(c5)

        QVERIFY(columns[4] == columns[0].normalized(QString::NormalizationForm_KD));
        QVERIFY(columns[4] == columns[1].normalized(QString::NormalizationForm_KD));
        QVERIFY(columns[4] == columns[2].normalized(QString::NormalizationForm_KD));
        QVERIFY(columns[4] == columns[3].normalized(QString::NormalizationForm_KD));
        QVERIFY(columns[4] == columns[4].normalized(QString::NormalizationForm_KD));

        // 2. For every code point X assigned in this version of Unicode that is not specifically
        //    listed in Part 1, the following invariants must be true for all conformant
        //    implementations:
        //
        //      X == NFC(X) == NFD(X) == NFKC(X) == NFKD(X)

        // #################

}

void tst_QChar::normalization_manual()
{
    {
        QString decomposed;
        decomposed += QChar(0x41);
        decomposed += QChar(0x0221); // assigned in 4.0
        decomposed += QChar(0x300);

        QVERIFY(decomposed.normalized(QString::NormalizationForm_C, QChar::Unicode_3_2) == decomposed);

        decomposed[1] = QChar(0x037f); // unassigned in 6.1

        QVERIFY(decomposed.normalized(QString::NormalizationForm_C) == decomposed);
    }
    {
        QString composed;
        composed += QChar(0xc0);
        QString decomposed;
        decomposed += QChar(0x41);
        decomposed += QChar(0x300);

        QVERIFY(composed.normalized(QString::NormalizationForm_D) == decomposed);
        QVERIFY(composed.normalized(QString::NormalizationForm_C) == composed);
        QVERIFY(composed.normalized(QString::NormalizationForm_KD) == decomposed);
        QVERIFY(composed.normalized(QString::NormalizationForm_KC) == composed);
    }
    {
        QString composed;
        composed += QChar(0xa0);
        QString decomposed;
        decomposed += QChar(0x20);

        QVERIFY(composed.normalized(QString::NormalizationForm_D) == composed);
        QVERIFY(composed.normalized(QString::NormalizationForm_C) == composed);
        QVERIFY(composed.normalized(QString::NormalizationForm_KD) == decomposed);
        QVERIFY(composed.normalized(QString::NormalizationForm_KC) == decomposed);
    }
    {
        QString composed;
        composed += QChar(0x0061);
        composed += QChar(0x00f2);
        QString decomposed;
        decomposed += QChar(0x0061);
        decomposed += QChar(0x006f);
        decomposed += QChar(0x0300);

        QVERIFY(decomposed.normalized(QString::NormalizationForm_D) == decomposed);
        QVERIFY(decomposed.normalized(QString::NormalizationForm_C) == composed);
        QVERIFY(decomposed.normalized(QString::NormalizationForm_KD) == decomposed);
        QVERIFY(decomposed.normalized(QString::NormalizationForm_KC) == composed);
    }
    {   // hangul
        QString composed;
        composed += QChar(0xc154);
        composed += QChar(0x11f0);
        QString decomposed;
        decomposed += QChar(0x1109);
        decomposed += QChar(0x1167);
        decomposed += QChar(0x11f0);

        QVERIFY(composed.normalized(QString::NormalizationForm_D) == decomposed);
        QVERIFY(composed.normalized(QString::NormalizationForm_C) == composed);
        QVERIFY(composed.normalized(QString::NormalizationForm_KD) == decomposed);
        QVERIFY(composed.normalized(QString::NormalizationForm_KC) == composed);

        QVERIFY(decomposed.normalized(QString::NormalizationForm_D) == decomposed);
        QVERIFY(decomposed.normalized(QString::NormalizationForm_C) == composed);
        QVERIFY(decomposed.normalized(QString::NormalizationForm_KD) == decomposed);
        QVERIFY(decomposed.normalized(QString::NormalizationForm_KC) == composed);
    }
}

void tst_QChar::normalizationCorrections()
{
    QString s;
    s.append(QChar(0xf951));

    QString n = s.normalized(QString::NormalizationForm_D);
    QString res;
    res.append(QChar(0x964b));
    QCOMPARE(n, res);

    n = s.normalized(QString::NormalizationForm_D, QChar::Unicode_3_1);
    res.clear();
    res.append(QChar(0x96fb));
    QCOMPARE(n, res);

    s.clear();
    s += QChar(QChar::highSurrogate(0x2f868));
    s += QChar(QChar::lowSurrogate(0x2f868));

    n = s.normalized(QString::NormalizationForm_C);
    res.clear();
    res += QChar(0x36fc);
    QCOMPARE(n, res);

    n = s.normalized(QString::NormalizationForm_C, QChar::Unicode_3_1);
    res.clear();
    res += QChar(0xd844);
    res += QChar(0xdf6a);
    QCOMPARE(n, res);

    n = s.normalized(QString::NormalizationForm_C, QChar::Unicode_3_2);
    QCOMPARE(n, res);
}


QTEST_APPLESS_MAIN(tst_QChar)
#include "tst_qchar.moc"
