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
#include <qchar.h>
#include <qfile.h>
#include <qstringlist.h>
#include <private/qunicodetables_p.h>
#if defined(Q_OS_WINCE)
#include <qcoreapplication.h>
#endif



//TESTED_CLASS=
//TESTED_FILES=

class tst_QChar : public QObject
{
    Q_OBJECT

public:
    tst_QChar();
    ~tst_QChar();


public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void toUpper();
    void toLower();
    void toTitle();
    void toCaseFolded();
    void isPrint();
    void isUpper();
    void isLower();
    void isTitle();
    void category();
    void direction();
    void joining();
    void combiningClass();
    void digitValue();
    void decomposition();
//     void ligature();
    void lineBreakClass();
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

tst_QChar::tst_QChar()
{}

tst_QChar::~tst_QChar()
{ }

void tst_QChar::initTestCase()
{ }

void tst_QChar::cleanupTestCase()
{ }

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

    QVERIFY(QChar::toUpper((ushort)'a') == 'A');
    QVERIFY(QChar::toUpper((ushort)'A') == 'A');
    QVERIFY(QChar::toUpper((ushort)0x1c7) == 0x1c7);
    QVERIFY(QChar::toUpper((ushort)0x1c8) == 0x1c7);
    QVERIFY(QChar::toUpper((ushort)0x1c9) == 0x1c7);

    QVERIFY(QChar::toUpper((uint)'a') == 'A');
    QVERIFY(QChar::toUpper((uint)'A') == 'A');
    QVERIFY(QChar::toUpper((uint)0x1c7) == 0x1c7);
    QVERIFY(QChar::toUpper((uint)0x1c8) == 0x1c7);
    QVERIFY(QChar::toUpper((uint)0x1c9) == 0x1c7);

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

    QVERIFY(QChar::toLower((ushort)'a') == 'a');
    QVERIFY(QChar::toLower((ushort)'A') == 'a');
    QVERIFY(QChar::toLower((ushort)0x1c7) == 0x1c9);
    QVERIFY(QChar::toLower((ushort)0x1c8) == 0x1c9);
    QVERIFY(QChar::toLower((ushort)0x1c9) == 0x1c9);

    QVERIFY(QChar::toLower((uint)'a') == 'a');
    QVERIFY(QChar::toLower((uint)'A') == 'a');
    QVERIFY(QChar::toLower((uint)0x1c7) == 0x1c9);
    QVERIFY(QChar::toLower((uint)0x1c8) == 0x1c9);
    QVERIFY(QChar::toLower((uint)0x1c9) == 0x1c9);

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

    QVERIFY(QChar::toTitleCase((ushort)'a') == 'A');
    QVERIFY(QChar::toTitleCase((ushort)'A') == 'A');
    QVERIFY(QChar::toTitleCase((ushort)0x1c7) == 0x1c8);
    QVERIFY(QChar::toTitleCase((ushort)0x1c8) == 0x1c8);
    QVERIFY(QChar::toTitleCase((ushort)0x1c9) == 0x1c8);

    QVERIFY(QChar::toTitleCase((uint)'a') == 'A');
    QVERIFY(QChar::toTitleCase((uint)'A') == 'A');
    QVERIFY(QChar::toTitleCase((uint)0x1c7) == 0x1c8);
    QVERIFY(QChar::toTitleCase((uint)0x1c8) == 0x1c8);
    QVERIFY(QChar::toTitleCase((uint)0x1c9) == 0x1c8);

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

    QVERIFY(QChar::toCaseFolded((ushort)'a') == 'a');
    QVERIFY(QChar::toCaseFolded((ushort)'A') == 'a');
    QVERIFY(QChar::toCaseFolded((ushort)0x1c7) == 0x1c9);
    QVERIFY(QChar::toCaseFolded((ushort)0x1c8) == 0x1c9);
    QVERIFY(QChar::toCaseFolded((ushort)0x1c9) == 0x1c9);

    QVERIFY(QChar::toCaseFolded((uint)'a') == 'a');
    QVERIFY(QChar::toCaseFolded((uint)'A') == 'a');
    QVERIFY(QChar::toCaseFolded((uint)0x1c7) == 0x1c9);
    QVERIFY(QChar::toCaseFolded((uint)0x1c8) == 0x1c9);
    QVERIFY(QChar::toCaseFolded((uint)0x1c9) == 0x1c9);

    QVERIFY(QChar::toCaseFolded((uint)0x10400) == 0x10428);
    QVERIFY(QChar::toCaseFolded((uint)0x10428) == 0x10428);

    QVERIFY(QChar::toCaseFolded((ushort)0xb5) == 0x3bc);
}

void tst_QChar::isPrint()
{
    QVERIFY(QChar('A').isPrint());
    QVERIFY(!QChar(0x1aff).isPrint()); // General_Gategory =Cn
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

    for (uint codepoint = 0; codepoint <= UNICODE_LAST_CODEPOINT; ++codepoint) {
        if (QChar::category(codepoint) == QChar::Letter_Uppercase)
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

    for (uint codepoint = 0; codepoint <= UNICODE_LAST_CODEPOINT; ++codepoint) {
        if (QChar::category(codepoint) == QChar::Letter_Lowercase)
            QVERIFY(codepoint == QChar::toLower(codepoint));
    }
}

void tst_QChar::isTitle()
{
    for (uint codepoint = 0; codepoint <= UNICODE_LAST_CODEPOINT; ++codepoint) {
        if (QChar::category(codepoint) == QChar::Letter_Titlecase)
            QVERIFY(codepoint == QChar::toTitleCase(codepoint));
    }
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

    QVERIFY(QChar::category((uint)0x10fffdu) == QChar::Other_PrivateUse);
    QVERIFY(QChar::category((uint)0x110000u) == QChar::NoCategory);

    QVERIFY(QChar::category((uint)0x1aff) == QChar::Other_NotAssigned);
    QVERIFY(QChar::category((uint)0x10ffffu) == QChar::Other_NotAssigned);
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
}

void tst_QChar::decomposition()
{
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
        str += QChar( (0x1D157 - 0x10000) / 0x400 + 0xd800 );
        str += QChar( ((0x1D157 - 0x10000) % 0x400) + 0xdc00 );
        str += QChar( (0x1D165 - 0x10000) / 0x400 + 0xd800 );
        str += QChar( ((0x1D165 - 0x10000) % 0x400) + 0xdc00 );
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

#if 0
void tst_QChar::ligature()
{
    QVERIFY(QChar::ligature(0x0041, 0x00300) == 0xc0);
    QVERIFY(QChar::ligature(0x0049, 0x00308) == 0xcf);
    QVERIFY(QChar::ligature(0x0391, 0x00301) == 0x386);
    QVERIFY(QChar::ligature(0x0627, 0x00653) == 0x622);

    QVERIFY(QChar::ligature(0x1100, 0x1161) == 0xac00);
    QVERIFY(QChar::ligature(0xac00, 0x11a8) == 0xac01);
}
#endif

void tst_QChar::lineBreakClass()
{
    QVERIFY(QUnicodeTables::lineBreakClass(0x0041u) == QUnicodeTables::LineBreak_AL);
    QVERIFY(QUnicodeTables::lineBreakClass(0x0033u) == QUnicodeTables::LineBreak_NU);
    QVERIFY(QUnicodeTables::lineBreakClass(0xe0164u) == QUnicodeTables::LineBreak_CM);
    QVERIFY(QUnicodeTables::lineBreakClass(0x2f9a4u) == QUnicodeTables::LineBreak_ID);
    QVERIFY(QUnicodeTables::lineBreakClass(0x10000u) == QUnicodeTables::LineBreak_AL);
    QVERIFY(QUnicodeTables::lineBreakClass(0x0fffdu) == QUnicodeTables::LineBreak_AL);
}

void tst_QChar::normalization_data()
{
    QTest::addColumn<QStringList>("columns");
    QTest::addColumn<int>("part");

    int linenum = 0;
    int part = 0;

    QFile f(SRCDIR "NormalizationTest.txt");
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
                if (uc < 0x10000)
                    columns[i].append(QChar(uc));
                else {
                    // convert to utf16
                    ushort high = QChar::highSurrogate(uc);
                    ushort low = QChar::lowSurrogate(uc);
                    columns[i].append(QChar(high));
                    columns[i].append(QChar(low));
                }
            }
        }

        QString nm = QString("line #%1:").arg(linenum);
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
