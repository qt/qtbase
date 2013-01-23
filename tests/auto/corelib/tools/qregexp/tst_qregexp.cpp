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
#include <qregexp.h>

const int N = 1;

class tst_QRegExp : public QObject
{
    Q_OBJECT
private slots:
    void getSetCheck();
    void indexIn_data();
    void indexIn_addMoreRows(const QByteArray &stri);
    void indexIn();
    void lastIndexIn_data();
    void lastIndexIn();
    void matchedLength();
    void wildcard_data();
    void wildcard();
    void testEscapingWildcard_data();
    void testEscapingWildcard();
    void testInvalidWildcard_data();
    void testInvalidWildcard();
    void caretAnchoredOptimization();
    void isEmpty();
    void prepareEngineOptimization();
    void swap();
    void operator_eq();

    void exactMatch();
    void capturedTexts();
    void staticRegExp();
    void rainersSlowRegExpCopyBug();
    void nonExistingBackReferenceBug();

    void reentrancy();
    void threadsafeEngineCache();

    void posAndCapConsistency_data();
    void posAndCapConsistency();
    void interval();
    void validityCheck_data();
    void validityCheck();
    void escapeSequences();
};

// Testing get/set functions
void tst_QRegExp::getSetCheck()
{
    QRegExp obj1;
    // PatternSyntax QRegExp::patternSyntax()
    // void QRegExp::setPatternSyntax(PatternSyntax)
    obj1.setPatternSyntax(QRegExp::PatternSyntax(QRegExp::RegExp));
    QCOMPARE(QRegExp::PatternSyntax(QRegExp::RegExp), obj1.patternSyntax());
    obj1.setPatternSyntax(QRegExp::PatternSyntax(QRegExp::Wildcard));
    QCOMPARE(QRegExp::PatternSyntax(QRegExp::Wildcard), obj1.patternSyntax());
    obj1.setPatternSyntax(QRegExp::PatternSyntax(QRegExp::FixedString));
    QCOMPARE(QRegExp::PatternSyntax(QRegExp::FixedString), obj1.patternSyntax());
}

extern const char email[];

void tst_QRegExp::lastIndexIn_data()
{
    indexIn_data();
}

void tst_QRegExp::indexIn_data()
{
    QTest::addColumn<QString>("regexpStr");
    QTest::addColumn<QString>("target");
    QTest::addColumn<int>("pos");
    QTest::addColumn<int>("len");
    QTest::addColumn<QStringList>("caps");

    for (int i = 0; i < N; ++i) {
        QByteArray stri;
        if (i > 0)
            stri.setNum(i);

        // anchors
        QTest::newRow(qPrintable(stri + "anc00")) << QString("a(?=)z") << QString("az") << 0 << 2 << QStringList();
        QTest::newRow(qPrintable(stri + "anc01")) << QString("a(?!)z") << QString("az") << -1 << -1 << QStringList();
        QTest::newRow(qPrintable(stri + "anc02")) << QString("a(?:(?=)|(?=))z") << QString("az") << 0 << 2
                               << QStringList();
        QTest::newRow(qPrintable(stri + "anc03")) << QString("a(?:(?=)|(?!))z") << QString("az") << 0 << 2
                               << QStringList();
        QTest::newRow(qPrintable(stri + "anc04")) << QString("a(?:(?!)|(?=))z") << QString("az") << 0 << 2
                               << QStringList();
        QTest::newRow(qPrintable(stri + "anc05")) << QString("a(?:(?!)|(?!))z") << QString("az") << -1 << -1
                               << QStringList();
        QTest::newRow(qPrintable(stri + "anc06")) << QString("a(?:(?=)|b)z") << QString("az") << 0 << 2
                               << QStringList();
        QTest::newRow(qPrintable(stri + "anc07")) << QString("a(?:(?=)|b)z") << QString("abz") << 0 << 3
                               << QStringList();
        QTest::newRow(qPrintable(stri + "anc08")) << QString("a(?:(?!)|b)z") << QString("az") << -1 << -1
                               << QStringList();
        QTest::newRow(qPrintable(stri + "anc09")) << QString("a(?:(?!)|b)z") << QString("abz") << 0 << 3
                               << QStringList();
        QTest::newRow(qPrintable(stri + "anc10")) << QString("a?(?=^b$)") << QString("ab") << -1 << -1
                               << QStringList();
        QTest::newRow(qPrintable(stri + "anc11")) << QString("a?(?=^b$)") << QString("b") << 0 << 0
                               << QStringList();

        // back-references
        QTest::newRow(qPrintable(stri + "bref00")) << QString("(a*)(\\1)") << QString("aaaaa") << 0 << 4
                               << QStringList( QStringList() << "aa" << "aa" );
        QTest::newRow(qPrintable(stri + "bref01")) << QString("<(\\w*)>.+</\\1>") << QString("<b>blabla</b>bla</>")
                               << 0 << 13 << QStringList( QStringList() << "b" );
        QTest::newRow(qPrintable(stri + "bref02")) << QString("<(\\w*)>.+</\\1>") << QString("<>blabla</b>bla</>")
                               << 0 << 18 << QStringList( QStringList() << "" );
        QTest::newRow(qPrintable(stri + "bref03")) << QString("((a*\\2)\\2)") << QString("aaaa") << 0 << 4
                               << QStringList( QStringList() << QString("aaaa") << "aa" );
        QTest::newRow(qPrintable(stri + "bref04")) << QString("^(aa+)\\1+$") << QString("aaaaaa") << 0 << 6
                               << QStringList( QStringList() << QString("aa") );
        QTest::newRow(qPrintable(stri + "bref05")) << QString("^(1)(2)(3)(4)(5)(6)(7)(8)(9)(10)(11)(12)(13)(14)"
                                          "\\14\\13\\12\\11\\10\\9\\8\\7\\6\\5\\4\\3\\2\\1")
                               << QString("12345678910111213141413121110987654321") << 0 << 38
                               << QStringList( QStringList() << "1" << "2" << "3" << "4" << "5" << "6"
                                                             << "7" << "8" << "9" << "10" << "11"
                                                             << "12" << "13" << "14");

        // captures
        QTest::newRow(qPrintable(stri + "cap00")) << QString("(a*)") << QString("") << 0 << 0
                               << QStringList( QStringList() << QString("") );
        QTest::newRow(qPrintable(stri + "cap01")) << QString("(a*)") << QString("aaa") << 0 << 3
                               << QStringList( QStringList() << "aaa" );
        QTest::newRow(qPrintable(stri + "cap02")) << QString("(a*)") << QString("baaa") << 0 << 0
                               << QStringList( QStringList() << QString("") );
        QTest::newRow(qPrintable(stri + "cap03")) << QString("(a*)(a*)") << QString("aaa") << 0 << 3
                               << QStringList( QStringList() << QString("aaa") << QString("") );
        QTest::newRow(qPrintable(stri + "cap04")) << QString("(a*)(b*)") << QString("aaabbb") << 0 << 6
                               << QStringList( QStringList() << QString("aaa") << QString("bbb") );
        QTest::newRow(qPrintable(stri + "cap06")) << QString("(a*)a*") << QString("aaa") << 0 << 3
                               << QStringList( QStringList() << QString("aaa") );
        QTest::newRow(qPrintable(stri + "cap07")) << QString("((a*a*)*)") << QString("aaa") << 0 << 3
                               << QStringList( QStringList() << "aaa" << QString("aaa") );
        QTest::newRow(qPrintable(stri + "cap08")) << QString("(((a)*(b)*)*)") << QString("ababa") << 0 << 5
                               << QStringList( QStringList() << QString("ababa") << QString("a") << QString("a")
                               << "" );
        QTest::newRow(qPrintable(stri + "cap09")) << QString("(((a)*(b)*)c)*") << QString("") << 0 << 0
                               << QStringList( QStringList() << QString("") << QString("") << QString("") << QString("") );
        QTest::newRow(qPrintable(stri + "cap10")) << QString("(((a)*(b)*)c)*") << QString("abc") << 0 << 3
                               << QStringList( QStringList() << "abc" << "ab" << "a"
                               << "b" );
        QTest::newRow(qPrintable(stri + "cap11")) << QString("(((a)*(b)*)c)*") << QString("abcc") << 0 << 4
                               << QStringList( QStringList() << "c" << "" << "" << "" );
        QTest::newRow(qPrintable(stri + "cap12")) << QString("(((a)*(b)*)c)*") << QString("abcac") << 0 << 5
                               << QStringList( QStringList() << "ac" << "a" << "a" << "" );
        QTest::newRow(qPrintable(stri + "cap13")) << QString("(to|top)?(o|polo)?(gical|o?logical)")
                               << QString("topological") << 0 << 11
                               << QStringList( QStringList() << "top" << "o"
                                                  << "logical" );
        QTest::newRow(qPrintable(stri + "cap14")) << QString("(a)+") << QString("aaaa") << 0 << 4
                               << QStringList( QStringList() << "a" );

        // concatenation
        QTest::newRow(qPrintable(stri + "cat00")) << QString("") << QString("") << 0 << 0 << QStringList();
        QTest::newRow(qPrintable(stri + "cat01")) << QString("") << QString("a") << 0 << 0 << QStringList();
        QTest::newRow(qPrintable(stri + "cat02")) << QString("a") << QString("") << -1 << -1 << QStringList();
        QTest::newRow(qPrintable(stri + "cat03")) << QString("a") << QString("a") << 0 << 1 << QStringList();
        QTest::newRow(qPrintable(stri + "cat04")) << QString("a") << QString("b") << -1 << -1 << QStringList();
        QTest::newRow(qPrintable(stri + "cat05")) << QString("b") << QString("a") << -1 << -1 << QStringList();
        QTest::newRow(qPrintable(stri + "cat06")) << QString("ab") << QString("ab") << 0 << 2 << QStringList();
        QTest::newRow(qPrintable(stri + "cat07")) << QString("ab") << QString("ba") << -1 << -1 << QStringList();
        QTest::newRow(qPrintable(stri + "cat08")) << QString("abab") << QString("abbaababab") << 4 << 4 << QStringList();

        indexIn_addMoreRows(stri);
    }
}

void tst_QRegExp::indexIn_addMoreRows(const QByteArray &stri)
{
        // from Perl Cookbook
        QTest::newRow(qPrintable(stri + "cook00")) << QString("^(m*)(d?c{0,3}|c[dm])(1?x{0,3}|x[lc])(v?i{0,3}|i[vx])$")
                                << QString("mmxl") << 0 << 4
                                << QStringList( QStringList() << "mm" << "" << "xl"
                                << "" );
        QTest::newRow(qPrintable(stri + "cook01")) << QString("(\\S+)(\\s+)(\\S+)") << QString(" a   b") << 1 << 5
                                << QStringList( QStringList() << "a" << "   " << "b" );
        QTest::newRow(qPrintable(stri + "cook02")) << QString("(\\w+)\\s*=\\s*(.*)\\s*$") << QString(" PATH=. ") << 1
                                << 7 << QStringList( QStringList() << "PATH" << ". " );
        QTest::newRow(qPrintable(stri + "cook03")) << QString(".{80,}")
                                << QString("0000000011111111222222223333333344444444555"
                                   "5555566666666777777778888888899999999000000"
                                   "00aaaaaaaa")
                                << 0 << 96 << QStringList();
        QTest::newRow(qPrintable(stri + "cook04")) << QString("(\\d+)/(\\d+)/(\\d+) (\\d+):(\\d+):(\\d+)")
                                << QString("1978/05/24 07:30:00") << 0 << 19
                                << QStringList( QStringList() << "1978" << "05" << "24"
                                << "07" << "30" << "00" );
        QTest::newRow(qPrintable(stri + "cook05")) << QString("/usr/bin") << QString("/usr/local/bin:/usr/bin")
                                << 15 << 8 << QStringList();
        QTest::newRow(qPrintable(stri + "cook06")) << QString("%([0-9A-Fa-f]{2})") << QString("http://%7f") << 7 << 3
                                << QStringList( QStringList() << "7f" );
        QTest::newRow(qPrintable(stri + "cook07")) << QString("/\\*.*\\*/") << QString("i++; /* increment i */") << 5
                                << 17 << QStringList();
        QTest::newRow(qPrintable(stri + "cook08")) << QString("^\\s+") << QString("   aaa   ") <<  0 << 3
                                << QStringList();
        QTest::newRow(qPrintable(stri + "cook09")) << QString("\\s+$") << QString("   aaa   ") <<  6 << 3
                                << QStringList();
        QTest::newRow(qPrintable(stri + "cook10")) << QString("^.*::") << QString("Box::cat") << 0 << 5
                                << QStringList();
        QTest::newRow(qPrintable(stri + "cook11")) << QString("^([01]?\\d\\d|2[0-4]\\d|25[0-5])\\.([01]?\\"
                                   "d\\d|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d|2[0-"
                                   "4]\\d|25[0-5])\\.([01]?\\d\\d|2[0-4]\\d|25["
                                   "0-5])$")
                                << QString("255.00.40.30") << 0 << 12
                                << QStringList( QStringList() << "255" << "00" << "40"
                                << "30" );
        QTest::newRow(qPrintable(stri + "cook12")) << QString("^.*/") << QString(" /usr/local/bin/moc") << 0 << 16
                                << QStringList();
        QTest::newRow(qPrintable(stri + "cook13")) << QString(":co#(\\d+):") << QString("bla:co#55:") << 3 << 7
                                << QStringList( QStringList() << "55" );
        QTest::newRow(qPrintable(stri + "cook14")) << QString("linux") << QString("alphalinuxinunix") << 5 << 5
                                << QStringList();
        QTest::newRow(qPrintable(stri + "cook15")) << QString("(\\d+\\.?\\d*|\\.\\d+)") << QString("0.0.5") << 0 << 3
                                << QStringList( QStringList() << "0.0" );

        // mathematical trivia
        QTest::newRow(qPrintable(stri + "math00")) << QString("^(a\\1*)$") << QString("a") << 0 << 1
                                << QStringList( QStringList() << "a" );
        QTest::newRow(qPrintable(stri + "math01")) << QString("^(a\\1*)$") << QString("aa") << 0 << 2
                                << QStringList( QStringList() << "aa" );
        QTest::newRow(qPrintable(stri + "math02")) << QString("^(a\\1*)$") << QString("aaa") << -1 << -1
                                << QStringList( QStringList() << QString() );
        QTest::newRow(qPrintable(stri + "math03")) << QString("^(a\\1*)$") << QString("aaaa") << 0 << 4
                                << QStringList( QStringList() << "aaaa" );
        QTest::newRow(qPrintable(stri + "math04")) << QString("^(a\\1*)$") << QString("aaaaa") << -1 << -1
                                << QStringList( QStringList() << QString() );
        QTest::newRow(qPrintable(stri + "math05")) << QString("^(a\\1*)$") << QString("aaaaaa") << -1 << -1
                                << QStringList( QStringList() << QString() );
        QTest::newRow(qPrintable(stri + "math06")) << QString("^(a\\1*)$") << QString("aaaaaaa") << -1 << -1
                                << QStringList( QStringList() << QString() );
        QTest::newRow(qPrintable(stri + "math07")) << QString("^(a\\1*)$") << QString("aaaaaaaa") << 0 << 8
                                << QStringList( QStringList() << "aaaaaaaa" );
        QTest::newRow(qPrintable(stri + "math08")) << QString("^(a\\1*)$") << QString("aaaaaaaaa") << -1 << -1
                                << QStringList( QStringList() << QString() );
        QTest::newRow(qPrintable(stri + "math09")) << QString("^a(?:a(\\1a))*$") << QString("a") << 0 << 1
                                << QStringList( QStringList() << "" );
        QTest::newRow(qPrintable(stri + "math10")) << QString("^a(?:a(\\1a))*$") << QString("aaa") << 0 << 3
                                << QStringList( QStringList() << "a" );

        QTest::newRow(qPrintable(stri + "math13")) << QString("^(?:((?:^a)?\\2\\3)(\\3\\1|(?=a$))(\\1\\2|("
                                   "?=a$)))*a$")
                                << QString("aaa") << 0 << 3
                                << QStringList( QStringList() << "a" << "a" << "" );
        QTest::newRow(qPrintable(stri + "math14")) << QString("^(?:((?:^a)?\\2\\3)(\\3\\1|(?=a$))(\\1\\2|("
                                   "?=a$)))*a$")
                                << QString("aaaaa") << 0 << 5
                                << QStringList( QStringList() << "a" << "a" << "aa" );
        QTest::newRow(qPrintable(stri + "math17")) << QString("^(?:(a(?:(\\1\\3)(\\1\\2))*(?:\\1\\3)?)|((?"
                                   ":(\\4(?:^a)?\\6)(\\4\\5))*(?:\\4\\6)?))$")
                                << QString("aaa") << 0 << 3
                                << QStringList( QStringList() << "" << "" << "" << "aaa" << "a" << "aa" );
        QTest::newRow(qPrintable(stri + "math18")) << QString("^(?:(a(?:(\\1\\3)(\\1\\2))*(?:\\1\\3)?)|((?"
                                   ":(\\4(?:^a)?\\6)(\\4\\5))*(?:\\4\\6)?))$")
                                << QString("aaaaa") << 0 << 5
                                << QStringList( QStringList() << "aaaaa" << "a" << "aaa" << "" << "" << "" );
        QTest::newRow(qPrintable(stri + "math19")) << QString("^(?:(a(?:(\\1\\3)(\\1\\2))*(?:\\1\\3)?)|((?"
                                   ":(\\4(?:^a)?\\6)(\\4\\5))*(?:\\4\\6)?))$")
                                << QString("aaaaaaaa") << 0 << 8
                                << QStringList( QStringList() << "" << "" << "" << "aaaaaaaa" << "a" << "aa" );
        QTest::newRow(qPrintable(stri + "math20")) << QString("^(?:(a(?:(\\1\\3)(\\1\\2))*(?:\\1\\3)?)|((?"
                                   ":(\\4(?:^a)?\\6)(\\4\\5))*(?:\\4\\6)?))$")
                                << QString("aaaaaaaaa") << -1 << -1
                                << QStringList( QStringList() << QString()
                                   << QString()
                                   << QString()
                                   << QString()
                                   << QString()
                                   << QString() );
        QTest::newRow(qPrintable(stri + "math21")) << QString("^(aa+)\\1+$") << QString("aaaaaaaaaaaa") << 0 << 12
                                << QStringList( QStringList() << "aa" );

        static const char * const squareRegExp[] = {
            "^a(?:(\\1aa)a)*$",
            "^(\\2(\\1a))+$",
            "^((\\2a)*)\\1\\2a$",
            0
                };

        int ii = 0;

        while ( squareRegExp[ii] != 0 ) {
            for ( int j = 0; j < 100; j++ ) {
                QString name;
                name.sprintf( "square%.1d%.2d", ii, j );

                QString target = "";
                target.fill( 'a', j );

                int pos = -1;
                int len = -1;

                for ( int k = 1; k * k <= j; k++ ) {
                    if ( k * k == j ) {
                        pos = 0;
                        len = j;
                        break;
                    }
                }

                QTest::newRow( name.toLatin1() ) << QString( squareRegExp[ii] ) << target
                                << pos << len << QStringList( "IGNORE ME" );
            }
            ii++;
        }

        // miscellaneous
        QTest::newRow(qPrintable(stri + "misc00")) << QString(email)
                                << QString("email123@example.com") << 0 << 20
                                << QStringList();
        QTest::newRow(qPrintable(stri + "misc01")) << QString("[0-9]*\\.[0-9]+") << QString("pi = 3.14") << 5 << 4
                                << QStringList();

        // or operator
        QTest::newRow(qPrintable(stri + "or00")) << QString("(?:|b)") << QString("xxx") << 0 << 0 << QStringList();
        QTest::newRow(qPrintable(stri + "or01")) << QString("(?:|b)") << QString("b") << 0 << 1 << QStringList();
        QTest::newRow(qPrintable(stri + "or02")) << QString("(?:b|)") << QString("") << 0 << 0 << QStringList();
        QTest::newRow(qPrintable(stri + "or03")) << QString("(?:b|)") << QString("b") << 0 << 1 << QStringList();
        QTest::newRow(qPrintable(stri + "or04")) << QString("(?:||b||)") << QString("") << 0 << 0 << QStringList();
        QTest::newRow(qPrintable(stri + "or05")) << QString("(?:||b||)") << QString("b") << 0 << 1 << QStringList();
        QTest::newRow(qPrintable(stri + "or06")) << QString("(?:a|b)") << QString("") << -1 << -1 << QStringList();
        QTest::newRow(qPrintable(stri + "or07")) << QString("(?:a|b)") << QString("cc") << -1 << -1 << QStringList();
        QTest::newRow(qPrintable(stri + "or08")) << QString("(?:a|b)") << QString("abc") << 0 << 1 << QStringList();
        QTest::newRow(qPrintable(stri + "or09")) << QString("(?:a|b)") << QString("cba") << 1 << 1 << QStringList();
        QTest::newRow(qPrintable(stri + "or10")) << QString("(?:ab|ba)") << QString("aba") << 0 << 2
                              << QStringList();
        QTest::newRow(qPrintable(stri + "or11")) << QString("(?:ab|ba)") << QString("bab") << 0 << 2
                              << QStringList();
        QTest::newRow(qPrintable(stri + "or12")) << QString("(?:ab|ba)") << QString("caba") << 1 << 2
                              << QStringList();
        QTest::newRow(qPrintable(stri + "or13")) << QString("(?:ab|ba)") << QString("cbab") << 1 << 2
                              << QStringList();

        // quantifiers
        QTest::newRow(qPrintable(stri + "qua00")) << QString("((([a-j])){0,0})") << QString("") << 0 << 0
                               << QStringList( QStringList() << "" << "" << "" );
        QTest::newRow(qPrintable(stri + "qua01")) << QString("((([a-j])){0,0})") << QString("a") << 0 << 0
                               << QStringList( QStringList() << "" << "" << "" );
        QTest::newRow(qPrintable(stri + "qua02")) << QString("((([a-j])){0,0})") << QString("xyz") << 0 << 0
                               << QStringList( QStringList() << "" << "" << "" );
        QTest::newRow(qPrintable(stri + "qua03")) << QString("((([a-j]))?)") << QString("") << 0 << 0
                               << QStringList( QStringList() << "" << "" << "" );
        QTest::newRow(qPrintable(stri + "qua04")) << QString("((([a-j]))?)") << QString("a") << 0 << 1
                               << QStringList( QStringList() << "a" << "a" << "a" );
        QTest::newRow(qPrintable(stri + "qua05")) << QString("((([a-j]))?)") << QString("x") << 0 << 0
                               << QStringList( QStringList() << "" << "" << "" );
        QTest::newRow(qPrintable(stri + "qua06")) << QString("((([a-j]))?)") << QString("ab") << 0 << 1
                               << QStringList( QStringList() << "a" << "a" << "a" );
        QTest::newRow(qPrintable(stri + "qua07")) << QString("((([a-j]))?)") << QString("xa") << 0 << 0
                               << QStringList( QStringList() << "" << "" << "" );
        QTest::newRow(qPrintable(stri + "qua08")) << QString("((([a-j])){0,3})") << QString("") << 0 << 0
                               << QStringList( QStringList() << "" << "" << "" );
        QTest::newRow(qPrintable(stri + "qua09")) << QString("((([a-j])){0,3})") << QString("a") << 0 << 1
                               << QStringList( QStringList() << "a" << "a" << "a" );
        QTest::newRow(qPrintable(stri + "qua10")) << QString("((([a-j])){0,3})") << QString("abcd") << 0 << 3
                               << QStringList( QStringList() << "abc" << "c" << "c" );
        QTest::newRow(qPrintable(stri + "qua11")) << QString("((([a-j])){0,3})") << QString("abcde") << 0 << 3
                               << QStringList( QStringList() << "abc" << "c" << "c" );
        QTest::newRow(qPrintable(stri + "qua12")) << QString("((([a-j])){2,4})") << QString("a") << -1 << -1
                               << QStringList( QStringList() << QString()
                                                  << QString()
                                                  << QString() );
        QTest::newRow(qPrintable(stri + "qua13")) << QString("((([a-j])){2,4})") << QString("ab") << 0 << 2
                               << QStringList( QStringList() << "ab" << "b" << "b" );
        QTest::newRow(qPrintable(stri + "qua14")) << QString("((([a-j])){2,4})") << QString("abcd") << 0 << 4
                               << QStringList( QStringList() << "abcd" << "d" << "d" );
        QTest::newRow(qPrintable(stri + "qua15")) << QString("((([a-j])){2,4})") << QString("abcdef") << 0 << 4
                               << QStringList( QStringList() << "abcd" << "d" << "d" );
        QTest::newRow(qPrintable(stri + "qua16")) << QString("((([a-j])){2,4})") << QString("xaybcd") << 3 << 3
                               << QStringList( QStringList() << "bcd" << "d" << "d" );
        QTest::newRow(qPrintable(stri + "qua17")) << QString("((([a-j])){0,})") << QString("abcdefgh") << 0 << 8
                               << QStringList( QStringList() << "abcdefgh" << "h" << "h" );
        QTest::newRow(qPrintable(stri + "qua18")) << QString("((([a-j])){,0})") << QString("abcdefgh") << 0 << 0
                               << QStringList( QStringList() << "" << "" << "" );
        QTest::newRow(qPrintable(stri + "qua19")) << QString("(1(2(3){3,4}){2,3}){1,2}") << QString("123332333") << 0
                               << 9
                               << QStringList( QStringList() << "123332333" << "2333"
                                                  << "3" );
        QTest::newRow(qPrintable(stri + "qua20")) << QString("(1(2(3){3,4}){2,3}){1,2}")
                               << QString("12333323333233331233332333323333") << 0 << 32
                               << QStringList( QStringList() << "1233332333323333"
                                                  << "23333" << "3" );
        QTest::newRow(qPrintable(stri + "qua21")) << QString("(1(2(3){3,4}){2,3}){1,2}") << QString("") << -1 << -1
                               << QStringList( QStringList() << QString()
                                                  << QString()
                                                  << QString() );
        QTest::newRow(qPrintable(stri + "qua22")) << QString("(1(2(3){3,4}){2,3}){1,2}") << QString("12333") << -1
                               << -1
                               << QStringList( QStringList() << QString()
                                                  << QString()
                                                  << QString() );
        QTest::newRow(qPrintable(stri + "qua23")) << QString("(1(2(3){3,4}){2,3}){1,2}") << QString("12333233") << -1
                               << -1
                               << QStringList( QStringList() << QString()
                                                  << QString()
                                                  << QString() );
        QTest::newRow(qPrintable(stri + "qua24")) << QString("(1(2(3){3,4}){2,3}){1,2}") << QString("122333") << -1
                               << -1
                               << QStringList( QStringList() << QString()
                                                  << QString()
                                                  << QString() );

        // star operator
        QTest::newRow(qPrintable(stri + "star00")) << QString("(?:)*") << QString("") << 0 << 0 << QStringList();
        QTest::newRow(qPrintable(stri + "star01")) << QString("(?:)*") << QString("abc") << 0 << 0 << QStringList();
        QTest::newRow(qPrintable(stri + "star02")) << QString("(?:a)*") << QString("") << 0 << 0 << QStringList();
        QTest::newRow(qPrintable(stri + "star03")) << QString("(?:a)*") << QString("a") << 0 << 1 << QStringList();
        QTest::newRow(qPrintable(stri + "star04")) << QString("(?:a)*") << QString("aaa") << 0 << 3 << QStringList();
        QTest::newRow(qPrintable(stri + "star05")) << QString("(?:a)*") << QString("bbbbaaa") << 0 << 0
                                << QStringList();
        QTest::newRow(qPrintable(stri + "star06")) << QString("(?:a)*") << QString("bbbbaaabbaaaaa") << 0 << 0
                                << QStringList();
        QTest::newRow(qPrintable(stri + "star07")) << QString("(?:b)*(?:a)*") << QString("") << 0 << 0
                                << QStringList();
        QTest::newRow(qPrintable(stri + "star08")) << QString("(?:b)*(?:a)*") << QString("a") << 0 << 1
                                << QStringList();
        QTest::newRow(qPrintable(stri + "star09")) << QString("(?:b)*(?:a)*") << QString("aaa") << 0 << 3
                                << QStringList();
        QTest::newRow(qPrintable(stri + "star10")) << QString("(?:b)*(?:a)*") << QString("bbbbaaa") << 0 << 7
                                << QStringList();
        QTest::newRow(qPrintable(stri + "star11")) << QString("(?:b)*(?:a)*") << QString("bbbbaaabbaaaaa") << 0 << 7
                                << QStringList();
        QTest::newRow(qPrintable(stri + "star12")) << QString("(?:a|b)*") << QString("c") << 0 << 0 << QStringList();
        QTest::newRow(qPrintable(stri + "star13")) << QString("(?:a|b)*") << QString("abac") << 0 << 3
                                << QStringList();
        QTest::newRow(qPrintable(stri + "star14")) << QString("(?:a|b|)*") << QString("c") << 0 << 0
                                << QStringList();
        QTest::newRow(qPrintable(stri + "star15")) << QString("(?:a|b|)*") << QString("abac") << 0 << 3
                                << QStringList();
        QTest::newRow(qPrintable(stri + "star16")) << QString("(?:ab|ba|b)*") << QString("abbbababbbaaab") << 0 << 11
                                << QStringList();
}

void tst_QRegExp::exactMatch()
{
    QRegExp rx_d( "\\d" );
    QRegExp rx_s( "\\s" );
    QRegExp rx_w( "\\w" );
    QRegExp rx_D( "\\D" );
    QRegExp rx_S( "\\S" );
    QRegExp rx_W( "\\W" );

    for ( int i = 0; i < 65536; i++ ) {
        QChar ch( i );
        bool is_d = ( ch.category() == QChar::Number_DecimalDigit );
        bool is_s = ch.isSpace();
        bool is_w = ( ch.isLetterOrNumber()
        || ch.isMark()
        || ch == '_' );

        QVERIFY( rx_d.exactMatch(QString(ch)) == is_d );
        QVERIFY( rx_s.exactMatch(QString(ch)) == is_s );
        QVERIFY( rx_w.exactMatch(QString(ch)) == is_w );
        QVERIFY( rx_D.exactMatch(QString(ch)) != is_d );
        QVERIFY( rx_S.exactMatch(QString(ch)) != is_s );
        QVERIFY( rx_W.exactMatch(QString(ch)) != is_w );
    }
}

void tst_QRegExp::capturedTexts()
{
    QRegExp rx1("a*(a*)", Qt::CaseSensitive, QRegExp::RegExp);
    rx1.exactMatch("aaa");
    QCOMPARE(rx1.matchedLength(), 3);
    QCOMPARE(rx1.cap(0), QString("aaa"));
    QCOMPARE(rx1.cap(1), QString("aaa"));

    QRegExp rx2("a*(a*)", Qt::CaseSensitive, QRegExp::RegExp2);
    rx2.exactMatch("aaa");
    QCOMPARE(rx2.matchedLength(), 3);
    QCOMPARE(rx2.cap(0), QString("aaa"));
    QCOMPARE(rx2.cap(1), QString(""));

    QRegExp rx3("(?:a|aa)(a*)", Qt::CaseSensitive, QRegExp::RegExp);
    rx3.exactMatch("aaa");
    QCOMPARE(rx3.matchedLength(), 3);
    QCOMPARE(rx3.cap(0), QString("aaa"));
    QCOMPARE(rx3.cap(1), QString("aa"));

    QRegExp rx4("(?:a|aa)(a*)", Qt::CaseSensitive, QRegExp::RegExp2);
    rx4.exactMatch("aaa");
    QCOMPARE(rx4.matchedLength(), 3);
    QCOMPARE(rx4.cap(0), QString("aaa"));
    QCOMPARE(rx4.cap(1), QString("a"));

    QRegExp rx5("(a)*(a*)", Qt::CaseSensitive, QRegExp::RegExp);
    rx5.exactMatch("aaa");
    QCOMPARE(rx5.matchedLength(), 3);
    QCOMPARE(rx5.cap(0), QString("aaa"));
    QCOMPARE(rx5.cap(1), QString("a"));
    QCOMPARE(rx5.cap(2), QString("aa"));

    QRegExp rx6("(a)*(a*)", Qt::CaseSensitive, QRegExp::RegExp2);
    rx6.exactMatch("aaa");
    QCOMPARE(rx6.matchedLength(), 3);
    QCOMPARE(rx6.cap(0), QString("aaa"));
    QCOMPARE(rx6.cap(1), QString("a"));
    QCOMPARE(rx6.cap(2), QString(""));

    QRegExp rx7("([A-Za-z_])([A-Za-z_0-9]*)");
    rx7.setCaseSensitivity(Qt::CaseSensitive);
    rx7.setPatternSyntax(QRegExp::RegExp);
    QCOMPARE(rx7.captureCount(), 2);

    int pos = rx7.indexIn("(10 + delta4) * 32");
    QCOMPARE(pos, 6);
    QCOMPARE(rx7.matchedLength(), 6);
    QCOMPARE(rx7.cap(0), QString("delta4"));
    QCOMPARE(rx7.cap(1), QString("d"));
    QCOMPARE(rx7.cap(2), QString("elta4"));
}

void tst_QRegExp::indexIn()
{
    QFETCH( QString, regexpStr );
    QFETCH( QString, target );
    QFETCH( int, pos );
    QFETCH( int, len );
    QFETCH( QStringList, caps );

    caps.prepend( "dummy cap(0)" );

    {
        QRegExp rx( regexpStr );
        QVERIFY( rx.isValid() );

        int mypos = rx.indexIn( target );
        int mylen = rx.matchedLength();
        QStringList mycaps = rx.capturedTexts();

        QCOMPARE( mypos, pos );
        QCOMPARE( mylen, len );
        if ( caps.size() > 1 && caps[1] != "IGNORE ME" ) {
            QCOMPARE( mycaps.count(), caps.count() );
            for ( int i = 1; i < (int) mycaps.count(); i++ )
                QCOMPARE( mycaps[i], caps[i] );
        }
    }

    // same as above, but with RegExp2
    {
        QRegExp rx( regexpStr, Qt::CaseSensitive, QRegExp::RegExp2 );
        QVERIFY( rx.isValid() );

        int mypos = rx.indexIn( target );
        int mylen = rx.matchedLength();
        QStringList mycaps = rx.capturedTexts();

        QCOMPARE( mypos, pos );
        QCOMPARE( mylen, len );
        if ( caps.size() > 1 && caps[1] != "IGNORE ME" ) {
            QCOMPARE( mycaps.count(), caps.count() );
            for ( int i = 1; i < (int) mycaps.count(); i++ )
                QCOMPARE( mycaps[i], caps[i] );
        }
    }
}

void tst_QRegExp::lastIndexIn()
{
    QFETCH( QString, regexpStr );
    QFETCH( QString, target );
    QFETCH( int, pos );
    QFETCH( int, len );
    QFETCH( QStringList, caps );

    caps.prepend( "dummy" );

    /*
      The test data was really designed for indexIn(), not
      lastIndexIn(), but it turns out that we can reuse much of that
      for lastIndexIn().
    */

    {
        QRegExp rx( regexpStr );
        QVERIFY( rx.isValid() );

        int mypos = rx.lastIndexIn( target, target.length() );
        int mylen = rx.matchedLength();
        QStringList mycaps = rx.capturedTexts();

        if ( mypos <= pos || pos == -1 ) {
            QCOMPARE( mypos, pos );
            QCOMPARE( mylen, len );

            if (caps.size() > 1 && caps[1] != "IGNORE ME") {
                QCOMPARE( mycaps.count(), caps.count() );
                for ( int i = 1; i < (int) mycaps.count(); i++ )
                    QCOMPARE( mycaps[i], caps[i] );
            }
        }
    }

    {
        QRegExp rx( regexpStr, Qt::CaseSensitive, QRegExp::RegExp2 );
        QVERIFY( rx.isValid() );

        int mypos = rx.lastIndexIn( target, target.length() );
        int mylen = rx.matchedLength();
        QStringList mycaps = rx.capturedTexts();

        if ( mypos <= pos || pos == -1 ) {
            QCOMPARE( mypos, pos );
            QCOMPARE( mylen, len );

            if (caps.size() > 1 && caps[1] != "IGNORE ME") {
                QCOMPARE( mycaps.count(), caps.count() );
                for ( int i = 1; i < (int) mycaps.count(); i++ )
                    QCOMPARE( mycaps[i], caps[i] );
            }
        }
    }
}

void tst_QRegExp::matchedLength()
{
    QRegExp r1( "a+" );
    r1.exactMatch( "aaaba" );
    QCOMPARE( r1.matchedLength(), 3 );
}

const char email[] =
    "^[\\040\\t]*(?:\\([^\\\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff"
    "]|\\([^\\\\\\x80-\\xff\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\x"
    "ff\\n\\015()]*)*\\))[^\\\\\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*(?:"
    "(?:[^(\\040)<>@,;:\".\\\\\\[\\]\\000-\\037\\x80-\\xff]+(?![^(\\040)<>@"
    ",;:\".\\\\\\[\\]\\000-\\037\\x80-\\xff])|\"[^\\\\\\x80-\\xff\\n\\015\""
    "]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015\"]*)*\")[\\040\\t]*(?"
    ":\\([^\\\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\\\x"
    "80-\\xff\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015()]*"
    ")*\\))[^\\\\\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*(?:\\.[\\040\\t]*"
    "(?:\\([^\\\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\"
    "\\x80-\\xff\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015("
    ")]*)*\\))[^\\\\\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*(?:[^(\\040)<>"
    "@,;:\".\\\\\\[\\]\\000-\\037\\x80-\\xff]+(?![^(\\040)<>@,;:\".\\\\\\["
    "\\]\\000-\\037\\x80-\\xff])|\"[^\\\\\\x80-\\xff\\n\\015\"]*(?:\\\\[^\\"
    "x80-\\xff][^\\\\\\x80-\\xff\\n\\015\"]*)*\")[\\040\\t]*(?:\\([^\\\\\\x"
    "80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\\\x80-\\xff\\n\\"
    "015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015()]*)*\\))[^\\\\"
    "\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*)*@[\\040\\t]*(?:\\([^\\\\\\x"
    "80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\\\x80-\\xff\\n\\"
    "015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015()]*)*\\))[^\\\\"
    "\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*(?:[^(\\040)<>@,;:\".\\\\\\["
    "\\]\\000-\\037\\x80-\\xff]+(?![^(\\040)<>@,;:\".\\\\\\[\\]\\000-\\037"
    "\\x80-\\xff])|\\[(?:[^\\\\\\x80-\\xff\\n\\015\\[\\]]|\\\\[^\\x80-\\xff"
    "])*\\])[\\040\\t]*(?:\\([^\\\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80"
    "-\\xff]|\\([^\\\\\\x80-\\xff\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x"
    "80-\\xff\\n\\015()]*)*\\))[^\\\\\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]"
    "*)*(?:\\.[\\040\\t]*(?:\\([^\\\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x"
    "80-\\xff]|\\([^\\\\\\x80-\\xff\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\"
    "\\x80-\\xff\\n\\015()]*)*\\))[^\\\\\\x80-\\xff\\n\\015()]*)*\\)[\\040"
    "\\t]*)*(?:[^(\\040)<>@,;:\".\\\\\\[\\]\\000-\\037\\x80-\\xff]+(?![^(\\"
    "040)<>@,;:\".\\\\\\[\\]\\000-\\037\\x80-\\xff])|\\[(?:[^\\\\\\x80-\\xf"
    "f\\n\\015\\[\\]]|\\\\[^\\x80-\\xff])*\\])[\\040\\t]*(?:\\([^\\\\\\x80-"
    "\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\\\x80-\\xff\\n\\015"
    "()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015()]*)*\\))[^\\\\\\x8"
    "0-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*)*|(?:[^(\\040)<>@,;:\".\\\\\\[\\"
    "]\\000-\\037\\x80-\\xff]+(?![^(\\040)<>@,;:\".\\\\\\[\\]\\000-\\037\\x"
    "80-\\xff])|\"[^\\\\\\x80-\\xff\\n\\015\"]*(?:\\\\[^\\x80-\\xff][^\\\\"
    "\\x80-\\xff\\n\\015\"]*)*\")[^()<>@,;:\".\\\\\\[\\]\\x80-\\xff\\000-\\"
    "010\\012-\\037]*(?:(?:\\([^\\\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x8"
    "0-\\xff]|\\([^\\\\\\x80-\\xff\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\"
    "x80-\\xff\\n\\015()]*)*\\))[^\\\\\\x80-\\xff\\n\\015()]*)*\\)|\"[^\\\\"
    "\\x80-\\xff\\n\\015\"]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015"
    "\"]*)*\")[^()<>@,;:\".\\\\\\[\\]\\x80-\\xff\\000-\\010\\012-\\037]*)*<"
    "[\\040\\t]*(?:\\([^\\\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]"
    "|\\([^\\\\\\x80-\\xff\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xf"
    "f\\n\\015()]*)*\\))[^\\\\\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*(?:@"
    "[\\040\\t]*(?:\\([^\\\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]"
    "|\\([^\\\\\\x80-\\xff\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xf"
    "f\\n\\015()]*)*\\))[^\\\\\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*(?:["
    "^(\\040)<>@,;:\".\\\\\\[\\]\\000-\\037\\x80-\\xff]+(?![^(\\040)<>@,;:"
    "\".\\\\\\[\\]\\000-\\037\\x80-\\xff])|\\[(?:[^\\\\\\x80-\\xff\\n\\015"
    "\\[\\]]|\\\\[^\\x80-\\xff])*\\])[\\040\\t]*(?:\\([^\\\\\\x80-\\xff\\n"
    "\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\\\x80-\\xff\\n\\015()]*(?:"
    "\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015()]*)*\\))[^\\\\\\x80-\\xff"
    "\\n\\015()]*)*\\)[\\040\\t]*)*(?:\\.[\\040\\t]*(?:\\([^\\\\\\x80-\\xff"
    "\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\\\x80-\\xff\\n\\015()]*("
    "?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015()]*)*\\))[^\\\\\\x80-\\x"
    "ff\\n\\015()]*)*\\)[\\040\\t]*)*(?:[^(\\040)<>@,;:\".\\\\\\[\\]\\000-"
    "\\037\\x80-\\xff]+(?![^(\\040)<>@,;:\".\\\\\\[\\]\\000-\\037\\x80-\\xf"
    "f])|\\[(?:[^\\\\\\x80-\\xff\\n\\015\\[\\]]|\\\\[^\\x80-\\xff])*\\])[\\"
    "040\\t]*(?:\\([^\\\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\"
    "([^\\\\\\x80-\\xff\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\"
    "n\\015()]*)*\\))[^\\\\\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*)*(?:,["
    "\\040\\t]*(?:\\([^\\\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]|"
    "\\([^\\\\\\x80-\\xff\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff"
    "\\n\\015()]*)*\\))[^\\\\\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*@[\\0"
    "40\\t]*(?:\\([^\\\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\("
    "[^\\\\\\x80-\\xff\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n"
    "\\015()]*)*\\))[^\\\\\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*(?:[^(\\"
    "040)<>@,;:\".\\\\\\[\\]\\000-\\037\\x80-\\xff]+(?![^(\\040)<>@,;:\".\\"
    "\\\\[\\]\\000-\\037\\x80-\\xff])|\\[(?:[^\\\\\\x80-\\xff\\n\\015\\[\\]"
    "]|\\\\[^\\x80-\\xff])*\\])[\\040\\t]*(?:\\([^\\\\\\x80-\\xff\\n\\015()"
    "]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\\\x80-\\xff\\n\\015()]*(?:\\\\[^\\"
    "x80-\\xff][^\\\\\\x80-\\xff\\n\\015()]*)*\\))[^\\\\\\x80-\\xff\\n\\015"
    "()]*)*\\)[\\040\\t]*)*(?:\\.[\\040\\t]*(?:\\([^\\\\\\x80-\\xff\\n\\015"
    "()]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\\\x80-\\xff\\n\\015()]*(?:\\\\[^"
    "\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015()]*)*\\))[^\\\\\\x80-\\xff\\n\\0"
    "15()]*)*\\)[\\040\\t]*)*(?:[^(\\040)<>@,;:\".\\\\\\[\\]\\000-\\037\\x8"
    "0-\\xff]+(?![^(\\040)<>@,;:\".\\\\\\[\\]\\000-\\037\\x80-\\xff])|\\[(?"
    ":[^\\\\\\x80-\\xff\\n\\015\\[\\]]|\\\\[^\\x80-\\xff])*\\])[\\040\\t]*("
    "?:\\([^\\\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\\\"
    "x80-\\xff\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015()]"
    "*)*\\))[^\\\\\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*)*)*:[\\040\\t]*"
    "(?:\\([^\\\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\"
    "\\x80-\\xff\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015("
    ")]*)*\\))[^\\\\\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*)?(?:[^(\\040)"
    "<>@,;:\".\\\\\\[\\]\\000-\\037\\x80-\\xff]+(?![^(\\040)<>@,;:\".\\\\\\"
    "[\\]\\000-\\037\\x80-\\xff])|\"[^\\\\\\x80-\\xff\\n\\015\"]*(?:\\\\[^"
    "\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015\"]*)*\")[\\040\\t]*(?:\\([^\\\\"
    "\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\\\x80-\\xff\\"
    "n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015()]*)*\\))[^\\"
    "\\\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*(?:\\.[\\040\\t]*(?:\\([^\\"
    "\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\\\x80-\\xff"
    "\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015()]*)*\\))[^"
    "\\\\\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*(?:[^(\\040)<>@,;:\".\\\\"
    "\\[\\]\\000-\\037\\x80-\\xff]+(?![^(\\040)<>@,;:\".\\\\\\[\\]\\000-\\0"
    "37\\x80-\\xff])|\"[^\\\\\\x80-\\xff\\n\\015\"]*(?:\\\\[^\\x80-\\xff][^"
    "\\\\\\x80-\\xff\\n\\015\"]*)*\")[\\040\\t]*(?:\\([^\\\\\\x80-\\xff\\n"
    "\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\\\x80-\\xff\\n\\015()]*(?:"
    "\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015()]*)*\\))[^\\\\\\x80-\\xff"
    "\\n\\015()]*)*\\)[\\040\\t]*)*)*@[\\040\\t]*(?:\\([^\\\\\\x80-\\xff\\n"
    "\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\\\x80-\\xff\\n\\015()]*(?:"
    "\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\015()]*)*\\))[^\\\\\\x80-\\xff"
    "\\n\\015()]*)*\\)[\\040\\t]*)*(?:[^(\\040)<>@,;:\".\\\\\\[\\]\\000-\\0"
    "37\\x80-\\xff]+(?![^(\\040)<>@,;:\".\\\\\\[\\]\\000-\\037\\x80-\\xff])"
    "|\\[(?:[^\\\\\\x80-\\xff\\n\\015\\[\\]]|\\\\[^\\x80-\\xff])*\\])[\\040"
    "\\t]*(?:\\([^\\\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\([^"
    "\\\\\\x80-\\xff\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n\\"
    "015()]*)*\\))[^\\\\\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*(?:\\.[\\0"
    "40\\t]*(?:\\([^\\\\\\x80-\\xff\\n\\015()]*(?:(?:\\\\[^\\x80-\\xff]|\\("
    "[^\\\\\\x80-\\xff\\n\\015()]*(?:\\\\[^\\x80-\\xff][^\\\\\\x80-\\xff\\n"
    "\\015()]*)*\\))[^\\\\\\x80-\\xff\\n\\015()]*)*\\)[\\040\\t]*)*(?:[^(\\"
    "040)<>@,;:\".\\\\\\[\\]\\000-\\037\\x80-\\xff]+(?![^(\\040)<>@,;:\".\\"
    "\\\\[\\]\\000-\\037\\x80-\\xff])|\\[(?:[^\\\\\\x80-\\xff\\n\\015\\[\\]"
    "]|\\\\[^\\x80-\\xff])*\\])[\\040\\t]*(?:\\([^\\\\\\x80-\\xff\\n\\015()"
    "]*(?:(?:\\\\[^\\x80-\\xff]|\\([^\\\\\\x80-\\xff\\n\\015()]*(?:\\\\[^\\"
    "x80-\\xff][^\\\\\\x80-\\xff\\n\\015()]*)*\\))[^\\\\\\x80-\\xff\\n\\015"
    "()]*)*\\)[\\040\\t]*)*)*>)$";

void tst_QRegExp::wildcard_data()
{
    QTest::addColumn<QString>("rxp");
    QTest::addColumn<QString>("string");
    QTest::addColumn<int>("foundIndex");

    QTest::newRow( "data0" ) << QString("*.html") << QString("test.html") << 0;
    QTest::newRow( "data1" ) << QString("*.html") << QString("test.htm") << -1;
    QTest::newRow( "data2" ) << QString("bar*") << QString("foobarbaz") << 3;
    QTest::newRow( "data3" ) << QString("*") << QString("Qt Rocks!") << 0;
    QTest::newRow( "data4" ) << QString(".html") << QString("test.html") << 4;
    QTest::newRow( "data5" ) << QString(".h") << QString("test.cpp") << -1;
    QTest::newRow( "data6" ) << QString(".???l") << QString("test.html") << 4;
    QTest::newRow( "data7" ) << QString("?") << QString("test.html") << 0;
    QTest::newRow( "data8" ) << QString("?m") << QString("test.html") << 6;
    QTest::newRow( "data9" ) << QString(".h[a-z]ml") << QString("test.html") << 4;
    QTest::newRow( "data10" ) << QString(".h[A-Z]ml") << QString("test.html") << -1;
    QTest::newRow( "data11" ) << QString(".h[A-Z]ml") << QString("test.hTml") << 4;
}

void tst_QRegExp::wildcard()
{
    QFETCH( QString, rxp );
    QFETCH( QString, string );
    QFETCH( int, foundIndex );

    QRegExp r( rxp );
    r.setPatternSyntax(QRegExp::WildcardUnix);
    QCOMPARE( r.indexIn( string ), foundIndex );
}

void tst_QRegExp::testEscapingWildcard_data(){
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("teststring");
    QTest::addColumn<bool>("isMatching");

    QTest::newRow("[ Not escaped") << "[Qt;" <<  "[Qt;" << false;
    QTest::newRow("[ Escaped") << "\\[Qt;" <<  "[Qt;" << true;

    QTest::newRow("] Not escaped") << "]Ik;" <<  "]Ik;" << false;
    QTest::newRow("] Escaped") << "\\]Ip;" <<  "]Ip;" << true;

    QTest::newRow("? Not escaped valid") << "?Ou:" <<  ".Ou:" << true;
    QTest::newRow("? Not escaped invalid") << "?Tr;" <<  "Tr;" << false;
    QTest::newRow("? Escaped") << "\\?O;" <<  "?O;" << true;

    QTest::newRow("[] not escaped") << "[lL]" <<  "l" << true;
    QTest::newRow("[] escaped") << "\\[\\]" <<  "[]" << true;

    QTest::newRow("case [[]") << "[[abc]" <<  "[" << true;
    QTest::newRow("case []abc] match ]") << "[]abc]" <<  "]" << true;
    QTest::newRow("case []abc] match a") << "[]abc]" <<  "a" << true;
    QTest::newRow("case [abc] match a") << "[abc]" <<  "a" << true;
    QTest::newRow("case []] don't match [") << "[]abc]" <<  "[" << false;
    QTest::newRow("case [^]abc] match d") << "[^]abc]" <<  "d" << true;
    QTest::newRow("case [^]abc] don't match ]") << "[^]abc]" <<  "]" << false;

    QTest::newRow("* Not escaped with char") << "*Te;" <<  "12345Te;" << true;
    QTest::newRow("* Not escaped without char") << "*Ch;" <<  "Ch;" << true;
    QTest::newRow("* Not escaped invalid") << "*Ro;" <<  "o;" << false;
    QTest::newRow("* Escaped") << "\\[Cks;" <<  "[Cks;" << true;

    QTest::newRow("a true '\\' in input") << "\\Qt;" <<  "\\Qt;" << true;
    QTest::newRow("two true '\\' in input") << "\\\\Qt;" <<  "\\\\Qt;" << true;
    QTest::newRow("a '\\' at the end") << "\\\\Qt;\\" <<  "\\\\Qt;\\" << true;
}

void tst_QRegExp::testEscapingWildcard(){
    QFETCH(QString, pattern);

    QRegExp re(pattern);
    re.setPatternSyntax(QRegExp::WildcardUnix);

    QFETCH(QString, teststring);
    QFETCH(bool, isMatching);
    QCOMPARE(re.exactMatch(teststring), isMatching);
}

void tst_QRegExp::testInvalidWildcard_data(){
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<bool>("isValid");

    QTest::newRow("valid []") << "[abc]" << true;
    QTest::newRow("invalid [") << "[abc" << false;
    QTest::newRow("ending [") << "abc[" << false;
    QTest::newRow("ending ]") << "abc]" << false;
    QTest::newRow("ending [^") << "abc[^" << false;
    QTest::newRow("ending [\\") << "abc[\\" << false;
    QTest::newRow("ending []") << "abc[]" << false;
    QTest::newRow("ending [[") << "abc[[" << false;
}

void tst_QRegExp::testInvalidWildcard(){
    QFETCH(QString, pattern);

    QRegExp re(pattern);
    re.setPatternSyntax(QRegExp::Wildcard);

    QFETCH(bool, isValid);
    QCOMPARE(re.isValid(), isValid);
}

void tst_QRegExp::caretAnchoredOptimization()
{
    QString s = "---babnana----";
    s.replace( QRegExp("^-*|(-*)$"), "" );
    QVERIFY(s == "babnana");

    s = "---babnana----";
    s.replace( QRegExp("^-*|(-{0,})$"), "" );
    QVERIFY(s == "babnana");

    s = "---babnana----";
    s.replace( QRegExp("^-*|(-{1,})$"), "" );
    QVERIFY(s == "babnana");

    s = "---babnana----";
    s.replace( QRegExp("^-*|(-+)$"), "" );
    QVERIFY(s == "babnana");
}

void tst_QRegExp::isEmpty()
{
    QRegExp rx1;
    QVERIFY(rx1.isEmpty());

    QRegExp rx2 = rx1;
    QVERIFY(rx2.isEmpty());

    rx2.setPattern("");
    QVERIFY(rx2.isEmpty());

    rx2.setPattern("foo");
    QVERIFY(!rx2.isEmpty());

    rx2.setPattern(")(");
    QVERIFY(!rx2.isEmpty());

    rx2.setPattern("");
    QVERIFY(rx2.isEmpty());

    rx2.setPatternSyntax(QRegExp::Wildcard);
    rx2.setPattern("");
    QVERIFY(rx2.isEmpty());
}

static QRegExp re("foo.*bar");

void tst_QRegExp::staticRegExp()
{
    QVERIFY(re.exactMatch("fooHARRYbar"));
    // the actual test is that a static regexp should not crash
}

void tst_QRegExp::rainersSlowRegExpCopyBug()
{
    // this test should take an extreme amount of time if QRegExp is broken
    QRegExp original(email);
#if defined(Q_OS_WINCE)
    for (int i = 0; i < 100; ++i) {
#else
    for (int i = 0; i < 100000; ++i) {
#endif
        QRegExp copy = original;
        (void)copy.exactMatch("~");
        QRegExp copy2 = original;
    }
}

void tst_QRegExp::nonExistingBackReferenceBug()
{
    {
        QRegExp rx("<\\5>");
        QVERIFY(rx.isValid());
        QCOMPARE(rx.indexIn("<>"), 0);
        QCOMPARE(rx.capturedTexts(), QStringList("<>"));
    }

    {
        QRegExp rx("<\\1>");
        QVERIFY(rx.isValid());
        QCOMPARE(rx.indexIn("<>"), 0);
        QCOMPARE(rx.capturedTexts(), QStringList("<>"));
    }

    {
        QRegExp rx("(?:<\\1>)\\1\\5\\4");
        QVERIFY(rx.isValid());
        QCOMPARE(rx.indexIn("<>"), 0);
        QCOMPARE(rx.capturedTexts(), QStringList("<>"));
    }
}

class Thread : public QThread
{
public:
    Thread(const QRegExp &rx) : rx(rx) {}

    void run();

    QRegExp rx;
};

void Thread::run()
{
    QString str = "abc";
    for (int i = 0; i < 10; ++i)
        str += str;
    str += "abbbdekcz";
    int x;

#if defined(Q_OS_WINCE)
    for (int j = 0; j < 100; ++j) {
#else
    for (int j = 0; j < 10000; ++j) {
#endif
        x = rx.indexIn(str);
    }
    QCOMPARE(x, 3072);
}

void tst_QRegExp::reentrancy()
{
    QRegExp rx("(ab{2,}d?e?f?[g-z]?)c");
    Thread *threads[10];

    for (int i = 0; i < int(sizeof(threads) / sizeof(threads[0])); ++i) {
        threads[i] = new Thread(rx);
        threads[i]->start();
    }

    for (int i = 0; i < int(sizeof(threads) / sizeof(threads[0])); ++i)
        threads[i]->wait();

    for (int i = 0; i < int(sizeof(threads) / sizeof(threads[0])); ++i)
        delete threads[i];
}

class Thread2 : public QThread
{
public:
    void run();
};

void Thread2::run()
{
    QRegExp rx("(ab{2,}d?e?f?[g-z]?)c");
    QString str = "abc";
    for (int i = 0; i < 10; ++i)
        str += str;
    str += "abbbdekcz";
    int x;

#if defined(Q_OS_WINCE)
    for (int j = 0; j < 100; ++j) {
#else
    for (int j = 0; j < 10000; ++j) {
#endif
        x = rx.indexIn(str);
    }
    QCOMPARE(x, 3072);
}

// Test that multiple threads can construct equal QRegExps.
// (In the current QRegExp design each engine instatance will share
// the same cache key, so the threads will race for the cache entry
// in the global cache.)
void tst_QRegExp::threadsafeEngineCache()
{
    Thread2 *threads[10];

    for (int i = 0; i < int(sizeof(threads) / sizeof(threads[0])); ++i) {
        threads[i] = new Thread2();
        threads[i]->start();
    }

    for (int i = 0; i < int(sizeof(threads) / sizeof(threads[0])); ++i)
        threads[i]->wait();

    for (int i = 0; i < int(sizeof(threads) / sizeof(threads[0])); ++i)
        delete threads[i];
}


void tst_QRegExp::prepareEngineOptimization()
{
    QRegExp rx0("(f?)(?:(o?)(o?))?");

    QRegExp rx1(rx0);

    QCOMPARE(rx1.capturedTexts(), QStringList() << "" << "" << "" << "");
    QCOMPARE(rx1.matchedLength(), -1);
    QCOMPARE(rx1.matchedLength(), -1);
    QCOMPARE(rx1.captureCount(), 3);

    QCOMPARE(rx1.exactMatch("foo"), true);
    QCOMPARE(rx1.matchedLength(), 3);
    QCOMPARE(rx1.capturedTexts(), QStringList() << "foo" << "f" << "o" << "o");
    QCOMPARE(rx1.captureCount(), 3);
    QCOMPARE(rx1.matchedLength(), 3);
    QCOMPARE(rx1.capturedTexts(), QStringList() << "foo" << "f" << "o" << "o");
    QCOMPARE(rx1.pos(3), 2);

    QCOMPARE(rx1.exactMatch("foo"), true);
    QCOMPARE(rx1.captureCount(), 3);
    QCOMPARE(rx1.matchedLength(), 3);
    QCOMPARE(rx1.capturedTexts(), QStringList() << "foo" << "f" << "o" << "o");
    QCOMPARE(rx1.pos(3), 2);

    QRegExp rx2 = rx1;

    QCOMPARE(rx1.captureCount(), 3);
    QCOMPARE(rx1.matchedLength(), 3);
    QCOMPARE(rx1.capturedTexts(), QStringList() << "foo" << "f" << "o" << "o");
    QCOMPARE(rx1.pos(3), 2);

    QCOMPARE(rx2.captureCount(), 3);
    QCOMPARE(rx2.matchedLength(), 3);
    QCOMPARE(rx2.capturedTexts(), QStringList() << "foo" << "f" << "o" << "o");
    QCOMPARE(rx2.pos(3), 2);

    QCOMPARE(rx1.exactMatch("fo"), true);
    QCOMPARE(rx1.captureCount(), 3);
    QCOMPARE(rx1.matchedLength(), 2);
    QCOMPARE(rx1.capturedTexts(), QStringList() << "fo" << "f" << "o" << "");
    QCOMPARE(rx1.pos(2), 1);

    QRegExp rx3;
    QVERIFY(rx3.isValid());

    QRegExp rx4("foo", Qt::CaseInsensitive, QRegExp::RegExp);
    QVERIFY(rx4.isValid());

    QRegExp rx5("foo", Qt::CaseInsensitive, QRegExp::RegExp2);
    QVERIFY(rx5.isValid());

    QRegExp rx6("foo", Qt::CaseInsensitive, QRegExp::FixedString);
    QVERIFY(rx6.isValid());

    QRegExp rx7("foo", Qt::CaseInsensitive, QRegExp::Wildcard);
    QVERIFY(rx7.isValid());

    QRegExp rx8("][", Qt::CaseInsensitive, QRegExp::RegExp);
    QVERIFY(!rx8.isValid());

    QRegExp rx9("][", Qt::CaseInsensitive, QRegExp::RegExp2);
    QVERIFY(!rx9.isValid());

    QRegExp rx10("][", Qt::CaseInsensitive, QRegExp::Wildcard);
    QVERIFY(!rx10.isValid());

    QRegExp rx11("][", Qt::CaseInsensitive, QRegExp::FixedString);
    QVERIFY(rx11.isValid());
    QVERIFY(rx11.exactMatch("]["));
    QCOMPARE(rx11.matchedLength(), 2);

    rx11.setPatternSyntax(QRegExp::Wildcard);
    QVERIFY(!rx11.isValid());
    QCOMPARE(rx11.captureCount(), 0);
    QCOMPARE(rx11.matchedLength(), -1);

    rx11.setPatternSyntax(QRegExp::RegExp);
    QVERIFY(!rx11.isValid());
    QCOMPARE(rx11.captureCount(), 0);
    QCOMPARE(rx11.matchedLength(), -1);

    rx11.setPattern("(foo)");
    QVERIFY(rx11.isValid());
    QCOMPARE(rx11.captureCount(), 1);
    QCOMPARE(rx11.matchedLength(), -1);

    QCOMPARE(rx11.indexIn("ofoo"), 1);
    QCOMPARE(rx11.captureCount(), 1);
    QCOMPARE(rx11.matchedLength(), 3);

    rx11.setPatternSyntax(QRegExp::RegExp);
    QCOMPARE(rx11.captureCount(), 1);
    QCOMPARE(rx11.matchedLength(), 3);

    /*
        This behavior isn't entirely consistent with setPatter(),
        setPatternSyntax(), and setCaseSensitivity(), but I'm testing
        it here to ensure that it doesn't change subtly in future
        releases.
    */
    rx11.setMinimal(true);
    QCOMPARE(rx11.matchedLength(), 3);
    rx11.setMinimal(false);
    QCOMPARE(rx11.matchedLength(), 3);

    rx11.setPatternSyntax(QRegExp::Wildcard);
    QCOMPARE(rx11.captureCount(), 0);
    QCOMPARE(rx11.matchedLength(), -1);

    rx11.setPatternSyntax(QRegExp::RegExp);
    QCOMPARE(rx11.captureCount(), 1);
    QCOMPARE(rx11.matchedLength(), -1);
}

void tst_QRegExp::swap()
{
    QRegExp r1(QLatin1String(".*")), r2(QLatin1String("a*"));
    r1.swap(r2);
    QCOMPARE(r1.pattern(),QLatin1String("a*"));
    QCOMPARE(r2.pattern(),QLatin1String(".*"));
}

void tst_QRegExp::operator_eq()
{
    const int I = 2;
    const int J = 4;
    const int K = 2;
    const int ELL = 2;
    QRegExp rxtable[I * J * K * ELL];
    int n;

    n = 0;
    for (int i = 0; i < I; ++i) {
        for (int j = 0; j < J; ++j) {
            for (int k = 0; k < K; ++k) {
                for (int ell = 0; ell < ELL; ++ell) {
                    Qt::CaseSensitivity cs = i == 0 ? Qt::CaseSensitive : Qt::CaseInsensitive;
                    QRegExp::PatternSyntax syntax = QRegExp::PatternSyntax(j);
                    bool minimal = k == 0;

                    if (ell == 0) {
                        QRegExp rx("foo", cs, syntax);
                        rx.setMinimal(minimal);
                        rxtable[n++] = rx;
                    } else {
                        QRegExp rx;
                        rx.setPattern("bar");
                        rx.setMinimal(true);
                        rx.exactMatch("bar");
                        rx.setCaseSensitivity(cs);
                        rx.setMinimal(minimal);
                        rx.setPattern("foo");
                        rx.setPatternSyntax(syntax);
                        rx.exactMatch("foo");
                        rxtable[n++] = rx;
                    }
                }
            }
        }
    }

    for (int i = 0; i < I * J * K * ELL; ++i) {
        for (int j = 0; j < I * J * K * ELL; ++j) {
            QCOMPARE(rxtable[i] == rxtable[j], i / ELL == j / ELL);
            QCOMPARE(rxtable[i] != rxtable[j], i / ELL != j / ELL);
        }
    }
}

// This test aims to ensure that the values returned by pos() and cap()
// are consistent.
void tst_QRegExp::posAndCapConsistency_data()
{
    QTest::addColumn<QString>("reStr");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("matchIndex");

    QTest::addColumn<int>("pos0");
    QTest::addColumn<int>("pos1");
    QTest::addColumn<int>("pos2");

    QTest::addColumn<QString>("cap0");
    QTest::addColumn<QString>("cap1");
    QTest::addColumn<QString>("cap2");

    QTest::newRow("no match")
        << QString("(a) (b)") << QString("b a") << -1
        << -1 << -1 << -1 << QString() << QString() << QString();

    QTest::newRow("both captures match")
        << QString("(a) (b)") << QString("a b") << 0
        << 0 << 0 << 2 << QString("a b") << QString("a") << QString("b");

    QTest::newRow("first capture matches @0")
        << QString("(a*)|(b*)") << QString("axx") << 0
        << 0 << 0 << -1 << QString("a") << QString("a") << QString();
    QTest::newRow("second capture matches @0")
        << QString("(a*)|(b*)") << QString("bxx") << 0
        << 0 << -1 << 0 << QString("b") << QString() << QString("b");
    QTest::newRow("first capture empty match @0")
        << QString("(a*)|(b*)") << QString("xx") << 0
        << 0 << -1 << -1 << QString("") << QString() << QString();
    QTest::newRow("second capture empty match @0")
        << QString("(a)|(b*)") << QString("xx") << 0
        << 0 << -1 << -1 << QString("") << QString() << QString();

    QTest::newRow("first capture matches @1")
        << QString("x(?:(a*)|(b*))") << QString("-xa") << 1
        << 1 << 2 << -1 << QString("xa") << QString("a") << QString();
    QTest::newRow("second capture matches @1")
        << QString("x(?:(a*)|(b*))") << QString("-xb") << 1
        << 1 << -1 << 2 << QString("xb") << QString() << QString("b");
    QTest::newRow("first capture empty match @1")
        << QString("x(?:(a*)|(b*))") << QString("-xx") << 1
        << 1 << -1 << -1 << QString("x") << QString() << QString();
    QTest::newRow("second capture empty match @1")
        << QString("x(?:(a)|(b*))") << QString("-xx") << 1
        << 1 << -1 << -1 << QString("x") << QString() << QString();

    QTest::newRow("first capture matches @2")
        << QString("(a)|(b)") << QString("xxa") << 2
        << 2 << 2 << -1 << QString("a") << QString("a") << QString();
    QTest::newRow("second capture matches @2")
        << QString("(a)|(b)") << QString("xxb") << 2
        << 2 << -1 << 2 << QString("b") << QString() << QString("b");
    QTest::newRow("no match - with options")
        << QString("(a)|(b)") << QString("xx") << -1
        << -1 << -1 << -1 << QString() << QString() << QString();

}

void tst_QRegExp::posAndCapConsistency()
{
    QFETCH( QString, reStr );
    QFETCH( QString, text );
    QFETCH( int, matchIndex );
    QFETCH( int, pos0 );
    QFETCH( int, pos1 );
    QFETCH( int, pos2 );
    QFETCH( QString, cap0 );
    QFETCH( QString, cap1 );
    QFETCH( QString, cap2 );

    QRegExp re(reStr);
    QCOMPARE(re.captureCount(), 2);
    QCOMPARE(re.capturedTexts().size(), 3);

    QCOMPARE(re.indexIn(text), matchIndex);

    QCOMPARE( re.pos(0), pos0 );
    QCOMPARE( re.pos(1), pos1 );
    QCOMPARE( re.pos(2), pos2 );

    QCOMPARE( re.cap(0).isNull(), cap0.isNull() );
    QCOMPARE( re.cap(0), cap0 );
    QCOMPARE( re.cap(1).isNull(), cap1.isNull() );
    QCOMPARE( re.cap(1), cap1 );
    QCOMPARE( re.cap(2).isNull(), cap2.isNull() );
    QCOMPARE( re.cap(2), cap2 );
}

void tst_QRegExp::interval()
{
    {
        QRegExp exp("a{0,1}");
        QVERIFY(exp.isValid());
    }
    {
        QRegExp exp("a{1,1}");
        QVERIFY(exp.isValid());
    }
    {
        QRegExp exp("a{1,0}");
        QVERIFY(!exp.isValid());
    }
}

void tst_QRegExp::validityCheck_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<bool>("validity");
    QTest::newRow("validity01") << QString() << true;
    QTest::newRow("validity02") << QString("abc.*abc") << true;
    QTest::newRow("validity03") << QString("[a-z") << false;
    QTest::newRow("validity04") << QString("a(b") << false;
}

void tst_QRegExp::validityCheck()
{
    QFETCH(QString, pattern);

    QRegExp rx(pattern);
    QTEST(rx.isValid(), "validity");
    QCOMPARE(rx.matchedLength(), -1);
    QCOMPARE(rx.pos(), -1);
    QCOMPARE(rx.cap(), QString(""));

    QRegExp rx2(rx);
    QTEST(rx2.isValid(), "validity");
    QCOMPARE(rx2.matchedLength(), -1);
    QCOMPARE(rx2.pos(), -1);
    QCOMPARE(rx2.cap(), QString(""));
}

void tst_QRegExp::escapeSequences()
{
    QString perlSyntaxSpecialChars("0123456789afnrtvbBdDwWsSx\\|[]{}()^$?+*");
    QString w3cXmlSchema11SyntaxSpecialChars("cCiIpP"); // as well as the perl ones
    for (int i = ' '; i <= 127; ++i) {
        QLatin1Char c(i);
        if (perlSyntaxSpecialChars.indexOf(c) == -1) {
            QRegExp rx(QString("\\%1").arg(c), Qt::CaseSensitive, QRegExp::RegExp);
            // we'll never have c == 'a' since it's a special character
            QString s = QString("aaa%1aaa").arg(c);
            QCOMPARE(rx.indexIn(s), 3);

            rx.setPatternSyntax(QRegExp::RegExp2);
            QCOMPARE(rx.indexIn(s), 3);

            if (w3cXmlSchema11SyntaxSpecialChars.indexOf(c) == -1) {
                rx.setPatternSyntax(QRegExp::W3CXmlSchema11);
                QCOMPARE(rx.indexIn(s), 3);
            }
        }
    }
}


QTEST_APPLESS_MAIN(tst_QRegExp)
#include "tst_qregexp.moc"
