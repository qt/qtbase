/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "tst_qmakelib.h"

#include <proitems.h>
#include <qmakevfs.h>
#include <qmakeparser.h>

class TokenStream
{
public:
    TokenStream() {}
    QString toString() const { return ts; }

    TokenStream &operator<<(ushort n) { ts += QChar(n); return *this; }
    TokenStream &operator<<(uint n) { ts += QChar(n & 0xffff); ts += QChar(n >> 16); return *this; }
    TokenStream &operator<<(const QStringRef &s) { ts += s; return *this; }
    TokenStream &operator<<(const ProString &s) { return *this << ushort(s.length()) << s.toQStringRef(); }
    TokenStream &operator<<(const ProKey &s) { return *this << s.hash() << s.toString(); }

private:
    QString ts;
};

#define TS(s) (TokenStream() s).toString()
#define H(n) ushort(n)
#define I(n) uint(n)
#define S(s) ProString(QString::fromWCharArray(s))
#define HS(s) ProKey(QString::fromWCharArray(s))

QT_WARNING_PUSH
QT_WARNING_DISABLE_MSVC(4003)  // "not enough actual parameters for macro TS()"

void tst_qmakelib::addParseOperators()
{
    QTest::newRow("assign none")
            << "VAR ="
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"VAR")
    /*     9 */ << H(TokAssign) << H(0)
    /*    11 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("append none")
            << "VAR +="
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"VAR")
    /*     9 */ << H(TokAppend) << H(0)
    /*    11 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("unique append none")
            << "VAR *="
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"VAR")
    /*     9 */ << H(TokAppendUnique) << H(0)
    /*    11 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("remove none")
            << "VAR -="
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"VAR")
    /*     9 */ << H(TokRemove) << H(0)
    /*    11 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("replace empty")
            << "VAR ~="
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"VAR")
    /*     9 */ << H(TokReplace) << H(0)
    /*    11 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("assignment without variable")
            << "="
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokAssign) << H(0)
    /*     4 */ << H(TokValueTerminator))
            << "in:1: Assignment needs exactly one word on the left hand side."
            << false;

    QTest::newRow("assignment with multiple variables")
            << "VAR VAR ="
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokAssign) << H(0)
    /*     4 */ << H(TokValueTerminator))
            << "in:1: Assignment needs exactly one word on the left hand side."
            << false;
}

void tst_qmakelib::addParseValues()
{
#define ASSIGN_VAR(h) \
        H(TokLine) << H(1) \
        << H(TokHashLiteral) << HS(L"VAR") \
        << H(TokAssign) << H(h)

    QTest::newRow("one literal")
            << "VAR = val"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"val")
    /*    16 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("one literal (squeezed)")
            << "VAR=val"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"val")
    /*    16 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("many literals")
            << "VAR = foo barbaz bak hello"
            << TS(
    /*     0 */ << ASSIGN_VAR(4)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"foo")
    /*    16 */ << H(TokLiteral | TokNewStr) << S(L"barbaz")
    /*    24 */ << H(TokLiteral | TokNewStr) << S(L"bak")
    /*    29 */ << H(TokLiteral | TokNewStr) << S(L"hello")
    /*    36 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("many literals (tab-separated")
            << "VAR\t=\tfoo\tbarbaz\tbak\thello"
            << TS(
    /*     0 */ << ASSIGN_VAR(4)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"foo")
    /*    16 */ << H(TokLiteral | TokNewStr) << S(L"barbaz")
    /*    24 */ << H(TokLiteral | TokNewStr) << S(L"bak")
    /*    29 */ << H(TokLiteral | TokNewStr) << S(L"hello")
    /*    36 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("one quoted literal")
            << "VAR = \"val ue\""
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"val ue")
    /*    19 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("quoted literal with missing quote")
            << "VAR = val \"ue"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"VAR")
    /*     9 */ << H(TokAssign) << H(0)
    /*    11 */ << H(TokValueTerminator))
            << "in:1: Missing closing \" quote"
            << false;

    QTest::newRow("many quoted literals")
            << "VAR = \"foo\" barbaz    'bak hello' \"\""
            << TS(
    /*     0 */ << ASSIGN_VAR(3)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"foo")
    /*    16 */ << H(TokLiteral | TokNewStr) << S(L"barbaz")
    /*    24 */ << H(TokLiteral | TokNewStr) << S(L"bak hello")
    /*    35 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("many quoted literals (with tabs)")
            << "VAR\t=\t\"foo\"\tbarbaz\t'bak\thello'"
            << TS(
    /*     0 */ << ASSIGN_VAR(3)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"foo")
    /*    16 */ << H(TokLiteral | TokNewStr) << S(L"barbaz")
    /*    24 */ << H(TokLiteral | TokNewStr) << S(L"bak\thello")
    /*    35 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("quoted and unquoted spaces")
            << "  VAR = \"val ue   \"   "
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"val ue   ")
    /*    22 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("funny literals")
            << "VAR = foo:bar|!baz(blam!, ${foo})"
            << TS(
    /*     0 */ << ASSIGN_VAR(2)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"foo:bar|!baz(blam!,")
    /*    32 */ << H(TokLiteral | TokNewStr) << S(L"${foo})")
    /*    41 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("literals with escapes")
            << "VAR = \\{hi\\} \\[ho\\] \\)uh\\( \"\\\\oh\\$\"\\' \\$\\${FOO}"
            << TS(
    /*     0 */ << ASSIGN_VAR(5)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"{hi}")
    /*    17 */ << H(TokLiteral | TokNewStr) << S(L"[ho]")
    /*    23 */ << H(TokLiteral | TokNewStr) << S(L")uh(")
    /*    29 */ << H(TokLiteral | TokNewStr) << S(L"\\oh$'")
    /*    36 */ << H(TokLiteral | TokNewStr) << S(L"$${FOO}")
    /*    45 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("magic variables")
            << "VAR = $$LITERAL_HASH $$LITERAL_DOLLAR $$LITERAL_WHITESPACE $$_FILE_ $$_LINE_"
            << TS(
    /*     0 */ << ASSIGN_VAR(5)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"#")
    /*    14 */ << H(TokLiteral | TokNewStr) << S(L"$")
    /*    17 */ << H(TokLiteral | TokNewStr) << S(L"\t")
    /*    20 */ << H(TokLiteral | TokNewStr) << S(L"in")
    /*    24 */ << H(TokLiteral | TokNewStr) << S(L"1")
    /*    27 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("continuations and comments")
            << "VAR = foo \\\n  bar\n  \n"
               "GAR = foo \\ # comment\n  bar \\\n   # comment\n baz \\\n"
                    "\"quoted \\ #comment\n    escape\" \\\n    right\\\n      after \\\n    gorilla!\n \n\n"
               "MOO = \\\n  kuh # comment\nLOO =\n\n"
               "FOO = bar \\\n# comment\n   baz \\\n    \n# comment\n"
               "GAZ="
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"VAR")
    /*     9 */ << H(TokAssign) << H(2)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"foo")
    /*    16 */ << H(TokLiteral | TokNewStr) << S(L"bar")
    /*    21 */ << H(TokValueTerminator)
    /*    22 */ << H(TokLine) << H(4)
    /*    24 */ << H(TokHashLiteral) << HS(L"GAR")
    /*    31 */ << H(TokAssign) << H(7)
    /*    33 */ << H(TokLiteral | TokNewStr) << S(L"foo")
    /*    38 */ << H(TokLiteral | TokNewStr) << S(L"bar")
    /*    43 */ << H(TokLiteral | TokNewStr) << S(L"baz")
    /*    48 */ << H(TokLiteral | TokNewStr) << S(L"quoted  escape")
    /*    64 */ << H(TokLiteral | TokNewStr) << S(L"right")
    /*    71 */ << H(TokLiteral | TokNewStr) << S(L"after")
    /*    78 */ << H(TokLiteral | TokNewStr) << S(L"gorilla!")
    /*    88 */ << H(TokValueTerminator)
    /*    89 */ << H(TokLine) << H(15)
    /*    91 */ << H(TokHashLiteral) << HS(L"MOO")
    /*    98 */ << H(TokAssign) << H(0)
    /*   100 */ << H(TokLiteral | TokNewStr) << S(L"kuh")
    /*   105 */ << H(TokValueTerminator)
    /*   106 */ << H(TokLine) << H(17)
    /*   108 */ << H(TokHashLiteral) << HS(L"LOO")
    /*   115 */ << H(TokAssign) << H(0)
    /*   117 */ << H(TokValueTerminator)
    /*   118 */ << H(TokLine) << H(19)
    /*   120 */ << H(TokHashLiteral) << HS(L"FOO")
    /*   127 */ << H(TokAssign) << H(2)
    /*   129 */ << H(TokLiteral | TokNewStr) << S(L"bar")
    /*   134 */ << H(TokLiteral | TokNewStr) << S(L"baz")
    /*   139 */ << H(TokValueTerminator)
    /*   140 */ << H(TokLine) << H(24)
    /*   142 */ << H(TokHashLiteral) << HS(L"GAZ")
    /*   149 */ << H(TokAssign) << H(0)
    /*   151 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("accidental continuation")
            << "VAR0 = \\\n    this \\\n    is \\\n    ok\n"
               "VAR1 = \\\n    this \\\n    is=still \\\n    ok\n"
               "VAR2 = \\\n    this \\\n    is \\\n"
               "VAR3 = \\\n    not ok\n"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"VAR0")
    /*    10 */ << H(TokAssign) << H(3)
    /*    12 */ << H(TokLiteral | TokNewStr) << S(L"this")
    /*    18 */ << H(TokLiteral | TokNewStr) << S(L"is")
    /*    22 */ << H(TokLiteral | TokNewStr) << S(L"ok")
    /*    26 */ << H(TokValueTerminator)
    /*    27 */ << H(TokLine) << H(5)
    /*    29 */ << H(TokHashLiteral) << HS(L"VAR1")
    /*    37 */ << H(TokAssign) << H(3)
    /*    39 */ << H(TokLiteral | TokNewStr) << S(L"this")
    /*    45 */ << H(TokLiteral | TokNewStr) << S(L"is=still")
    /*    55 */ << H(TokLiteral | TokNewStr) << S(L"ok")
    /*    59 */ << H(TokValueTerminator)
    /*    60 */ << H(TokLine) << H(9)
    /*    62 */ << H(TokHashLiteral) << HS(L"VAR2")
    /*    70 */ << H(TokAssign) << H(6)
    /*    72 */ << H(TokLiteral | TokNewStr) << S(L"this")
    /*    78 */ << H(TokLiteral | TokNewStr) << S(L"is")
    /*    82 */ << H(TokLiteral | TokNewStr) << S(L"VAR3")
    /*    88 */ << H(TokLiteral | TokNewStr) << S(L"=")
    /*    91 */ << H(TokLiteral | TokNewStr) << S(L"not")
    /*    96 */ << H(TokLiteral | TokNewStr) << S(L"ok")
    /*   100 */ << H(TokValueTerminator))
            << "WARNING: in:12: Possible accidental line continuation"
            << true;

    QTest::newRow("plain variable expansion")
            << "VAR = $$bar"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokVariable | TokNewStr) << HS(L"bar")
    /*    18 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("braced variable expansion")
            << "VAR = $${foo/bar}"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokVariable | TokNewStr) << HS(L"foo/bar")
    /*    22 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("bogus variable expansion")
            << "VAR = $$  "
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokVariable | TokNewStr) << HS(L"")
    /*    15 */ << H(TokValueTerminator))
            << "WARNING: in:1: Missing name in expansion"
            << true;

    QTest::newRow("bogus braced variable expansion")
            << "VAR = $${}"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokVariable | TokNewStr) << HS(L"")
    /*    15 */ << H(TokValueTerminator))
            << "WARNING: in:1: Missing name in expansion"
            << true;

    QTest::newRow("unterminated braced variable expansion")
            << "VAR = $${FOO"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"VAR")
    /*     9 */ << H(TokAssign) << H(0)
    /*    11 */ << H(TokVariable | TokNewStr) << HS(L"FOO")
    /*    18 */ << H(TokValueTerminator))
            << "in:1: Missing } terminator [found end-of-line]"
            << false;

    QTest::newRow("invalid identifier in braced variable expansion")
            << "VAR = $${FOO/BAR+BAZ}"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"VAR")
    /*     9 */ << H(TokAssign) << H(0)
    /*    11 */ << H(TokVariable | TokNewStr) << HS(L"FOO/BAR")
    /*    22 */ << H(TokLiteral) << S(L"+BAZ")
    /*    28 */ << H(TokValueTerminator))
            << "in:1: Missing } terminator [found +]"
            << false;

    QTest::newRow("property expansion")
            << "VAR = $$[bar]"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokProperty | TokNewStr) << HS(L"bar")
    /*    18 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("environment expansion")
            << "VAR = $$(bar)"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokEnvVar | TokNewStr) << S(L"bar")
    /*    16 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("plain function call")
            << "VAR = $$bar()"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokFuncName | TokNewStr) << HS(L"bar")
    /*    18 */     << H(TokFuncTerminator)
    /*    19 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("braced function call")
            << "VAR = $${bar()}"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokFuncName | TokNewStr) << HS(L"bar")
    /*    18 */     << H(TokFuncTerminator)
    /*    19 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("function call with one argument")
            << "VAR = $$bar(blubb)"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokFuncName | TokNewStr) << HS(L"bar")
    /*    18 */     << H(TokLiteral | TokNewStr) << S(L"blubb")
    /*    25 */     << H(TokFuncTerminator)
    /*    26 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("function call with multiple arguments")
            << "VAR = $$bar(  blubb blubb, hey  ,$$you)"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokFuncName | TokNewStr) << HS(L"bar")
    /*    18 */     << H(TokLiteral | TokNewStr) << S(L"blubb")
    /*    25 */     << H(TokLiteral | TokNewStr) << S(L"blubb")
    /*    32 */     << H(TokArgSeparator)
    /*    33 */     << H(TokLiteral | TokNewStr) << S(L"hey")
    /*    38 */     << H(TokArgSeparator)
    /*    39 */     << H(TokVariable | TokNewStr) << HS(L"you")
    /*    46 */     << H(TokFuncTerminator)
    /*    47 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("nested function call")
            << "VAR = $$foo(yo, $$bar(blubb))"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokFuncName | TokNewStr) << HS(L"foo")
    /*    18 */     << H(TokLiteral | TokNewStr) << S(L"yo")
    /*    22 */     << H(TokArgSeparator)
    /*    23 */     << H(TokFuncName | TokNewStr) << HS(L"bar")
    /*    30 */         << H(TokLiteral | TokNewStr) << S(L"blubb")
    /*    37 */         << H(TokFuncTerminator)
    /*    38 */     << H(TokFuncTerminator)
    /*    39 */ << H(TokValueTerminator))
            << ""
            << true;

    // This is a rather questionable "feature"
    QTest::newRow("function call with parenthesized argument")
            << "VAR = $$bar(blubb (yo, man) blabb, nope)"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokFuncName | TokNewStr) << HS(L"bar")
    /*    18 */     << H(TokLiteral | TokNewStr) << S(L"blubb")
    /*    25 */     << H(TokLiteral | TokNewStr) << S(L"(yo,")
    /*    31 */     << H(TokLiteral | TokNewStr) << S(L"man)")
    /*    37 */     << H(TokLiteral | TokNewStr) << S(L"blabb")
    /*    44 */     << H(TokArgSeparator)
    /*    45 */     << H(TokLiteral | TokNewStr) << S(L"nope")
    /*    51 */     << H(TokFuncTerminator)
    /*    52 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("separate literal and expansion")
            << "VAR = foo $$bar"
            << TS(
    /*     0 */ << ASSIGN_VAR(2)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"foo")
    /*    16 */ << H(TokVariable | TokNewStr) << HS(L"bar")
    /*    23 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("separate expansion and literal")
            << "VAR = $$bar foo"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokVariable | TokNewStr) << HS(L"bar")
    /*    18 */ << H(TokLiteral | TokNewStr) << S(L"foo")
    /*    23 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("joined literal and expansion")
            << "VAR = foo$$bar"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"foo")
    /*    16 */ << H(TokVariable) << HS(L"bar")
    /*    23 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("joined expansion and literal")
            << "VAR = $${bar}foo"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokVariable | TokNewStr) << HS(L"bar")
    /*    18 */ << H(TokLiteral) << S(L"foo")
    /*    23 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("plain variable expansion with funny name and literal")
            << "VAR = $$az_AZ_09.dot/nix"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokVariable | TokNewStr) << HS(L"az_AZ_09.dot")
    /*    27 */ << H(TokLiteral) << S(L"/nix")
    /*    33 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("braced variable expansion with funny name")
            << "VAR = $${az_AZ_09.dot/nix}"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokVariable | TokNewStr) << HS(L"az_AZ_09.dot/nix")
    /*    31 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("quoted joined literal and expansion")
            << "VAR = 'foo$$bar'"
            << TS(
    /*     0 */ << ASSIGN_VAR(0)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"foo")
    /*    16 */ << H(TokVariable | TokQuoted) << HS(L"bar")
    /*    23 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("assignment with expansion in variable name")
            << "VAR$$EXTRA ="
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"VAR")
    /*     9 */ << H(TokVariable) << HS(L"EXTRA")
    /*    18 */ << H(TokAssign) << H(0)
    /*    20 */ << H(TokValueTerminator))
            << ""
            << true;
}

void tst_qmakelib::addParseConditions()
{
    QTest::newRow("one test")
            << "foo"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition))
            << ""
            << true;

    QTest::newRow("wildcard-test")
            << "foo-*"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo-*")
    /*    11 */ << H(TokCondition))
            << ""
            << true;

    // This is a rather questionable "feature"
    QTest::newRow("one quoted test")
            << "\"foo\""
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition))
            << ""
            << true;

    QTest::newRow("two tests")
            << "foo\nbar"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition)
    /*    10 */ << H(TokLine) << H(2)
    /*    12 */ << H(TokHashLiteral) << HS(L"bar")
    /*    19 */ << H(TokCondition))
            << ""
            << true;

    QTest::newRow("bogus two tests")
            << "foo bar\nbaz"
            << TS()
            << "in:1: Extra characters after test expression."
            << false;

    QTest::newRow("test-AND-test")
            << "foo:bar"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition)
    /*    10 */ << H(TokAnd)
    /*    11 */ << H(TokHashLiteral) << HS(L"bar")
    /*    18 */ << H(TokCondition))
            << ""
            << true;

    QTest::newRow("test-OR-test")
            << "  foo  | bar "
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition)
    /*    10 */ << H(TokOr)
    /*    11 */ << H(TokHashLiteral) << HS(L"bar")
    /*    18 */ << H(TokCondition))
            << ""
            << true;

    QTest::newRow("NOT-test")
            << "!foo"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokNot)
    /*     3 */ << H(TokHashLiteral) << HS(L"foo")
    /*    10 */ << H(TokCondition))
            << ""
            << true;

    QTest::newRow("NOT-NOT-test")
            << "!!foo"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition))
            << ""
            << true;

    // This is a rather questionable "feature"
    QTest::newRow("quoted-NOT-test")
            << "\"!foo\""
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokNot)
    /*     3 */ << H(TokHashLiteral) << HS(L"foo")
    /*    10 */ << H(TokCondition))
            << ""
            << true;

    // This is a rather questionable "feature"
    QTest::newRow("NOT-quoted-test")
            << "!\"foo\""
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokNot)
    /*     3 */ << H(TokHashLiteral) << HS(L"foo")
    /*    10 */ << H(TokCondition))
            << ""
            << true;

    QTest::newRow("test-AND-NOT-test")
            << "foo:!bar"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition)
    /*    10 */ << H(TokAnd)
    /*    11 */ << H(TokNot)
    /*    12 */ << H(TokHashLiteral) << HS(L"bar")
    /*    19 */ << H(TokCondition))
            << ""
            << true;

    QTest::newRow("test-assignment")
            << "foo\nVAR="
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition)
    /*    10 */ << H(TokLine) << H(2)
    /*    12 */ << H(TokHashLiteral) << HS(L"VAR")
    /*    19 */ << H(TokAssign) << H(0)
    /*    21 */ << H(TokValueTerminator))
            << ""
            << true;

    QTest::newRow("test-AND-assignment")
            << "foo: VAR ="
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition)
    /*    10 */ << H(TokBranch)
    /*    11 */ /* then branch */ << I(11)
    /*    13 */     << H(TokHashLiteral) << HS(L"VAR")
    /*    20 */     << H(TokAssign) << H(0)
    /*    22 */     << H(TokValueTerminator)
    /*    23 */     << H(TokTerminator)
    /*    24 */ /* else branch */ << I(0))
            << ""
            << true;

    QTest::newRow("test-else-test")
            << "foo\nelse: bar"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition)
    /*    10 */ << H(TokBranch)
    /*    11 */ /* then branch */ << I(0)
    /*    13 */ /* else branch */ << I(11)
    /*    15 */     << H(TokLine) << H(2)
    /*    17 */     << H(TokHashLiteral) << HS(L"bar")
    /*    24 */     << H(TokCondition)
    /*    25 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("function-else-test")
            << "foo()\nelse: bar"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokTestCall)
    /*    10 */     << H(TokFuncTerminator)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(0)
    /*    14 */ /* else branch */ << I(11)
    /*    16 */     << H(TokLine) << H(2)
    /*    18 */     << H(TokHashLiteral) << HS(L"bar")
    /*    25 */     << H(TokCondition)
    /*    26 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("test-AND-test-else-test")
            << "foo:bar\nelse: baz"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition)
    /*    10 */ << H(TokAnd)
    /*    11 */ << H(TokHashLiteral) << HS(L"bar")
    /*    18 */ << H(TokCondition)
    /*    19 */ << H(TokBranch)
    /*    20 */ /* then branch */ << I(0)
    /*    22 */ /* else branch */ << I(11)
    /*    24 */     << H(TokLine) << H(2)
    /*    26 */     << H(TokHashLiteral) << HS(L"baz")
    /*    33 */     << H(TokCondition)
    /*    34 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("test-AND-test-else-test-else-test-function")
            << "foo:bar\nelse: baz\nelse: bak\nbuzz()"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition)
    /*    10 */ << H(TokAnd)
    /*    11 */ << H(TokHashLiteral) << HS(L"bar")
    /*    18 */ << H(TokCondition)
    /*    19 */ << H(TokBranch)
    /*    20 */ /* then branch */ << I(0)
    /*    22 */ /* else branch */ << I(27)
    /*    24 */     << H(TokLine) << H(2)
    /*    26 */     << H(TokHashLiteral) << HS(L"baz")
    /*    33 */     << H(TokCondition)
    /*    34 */     << H(TokBranch)
    /*    35 */     /* then branch */ << I(0)
    /*    37 */     /* else branch */ << I(11)
    /*    39 */         << H(TokLine) << H(3)
    /*    41 */         << H(TokHashLiteral) << HS(L"bak")
    /*    48 */         << H(TokCondition)
    /*    49 */         << H(TokTerminator)
    /*    50 */     << H(TokTerminator)
    /*    51 */ << H(TokLine) << H(4)
    /*    53 */ << H(TokHashLiteral) << HS(L"buzz")
    /*    61 */ << H(TokTestCall)
    /*    62 */     << H(TokFuncTerminator))
            << ""
            << true;

    QTest::newRow("test-assignment-else-assignment")
            << "foo: VAR =\nelse: VAR="
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition)
    /*    10 */ << H(TokBranch)
    /*    11 */ /* then branch */ << I(11)
    /*    13 */     << H(TokHashLiteral) << HS(L"VAR")
    /*    20 */     << H(TokAssign) << H(0)
    /*    22 */     << H(TokValueTerminator)
    /*    23 */     << H(TokTerminator)
    /*    24 */ /* else branch */ << I(13)
    /*    26 */     << H(TokLine) << H(2)
    /*    28 */     << H(TokHashLiteral) << HS(L"VAR")
    /*    35 */     << H(TokAssign) << H(0)
    /*    37 */     << H(TokValueTerminator)
    /*    38 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("test-else-test-assignment")
            << "foo\nelse: bar: VAR ="
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition)
    /*    10 */ << H(TokBranch)
    /*    11 */ /* then branch */ << I(0)
    /*    13 */ /* else branch */ << I(27)
    /*    15 */     << H(TokLine) << H(2)
    /*    17 */     << H(TokHashLiteral) << HS(L"bar")
    /*    24 */     << H(TokCondition)
    /*    25 */     << H(TokBranch)
    /*    26 */     /* then branch */ << I(11)
    /*    28 */         << H(TokHashLiteral) << HS(L"VAR")
    /*    35 */         << H(TokAssign) << H(0)
    /*    37 */         << H(TokValueTerminator)
    /*    38 */         << H(TokTerminator)
    /*    39 */     /* else branch */ << I(0)
    /*    41 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("one function")
            << "foo()"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokTestCall)
    /*    10 */     << H(TokFuncTerminator))
            << ""
            << true;

    QTest::newRow("one function (with spaces)")
            << " foo(  ) "
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokTestCall)
    /*    10 */     << H(TokFuncTerminator))
            << ""
            << true;

    QTest::newRow("unterminated function call")
            << "foo(\nfoo"
            << TS()
            << "in:1: Missing closing parenthesis in function call"
            << false;

    QTest::newRow("function with arguments")
            << "foo(blah, hi ho)"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokTestCall)
    /*    10 */     << H(TokLiteral | TokNewStr) << S(L"blah")
    /*    16 */     << H(TokArgSeparator)
    /*    17 */     << H(TokLiteral | TokNewStr) << S(L"hi")
    /*    21 */     << H(TokLiteral | TokNewStr) << S(L"ho")
    /*    25 */     << H(TokFuncTerminator))
            << ""
            << true;

    QTest::newRow("function with empty arguments")
            << "foo(,)"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokTestCall)
    /*    10 */     << H(TokArgSeparator)
    /*    11 */     << H(TokFuncTerminator))
            << ""
            << true;

    QTest::newRow("function with funny arguments")
            << "foo(blah\\, \"hi ,  \\ho\" ,uh\\  ,\\oh  ,,   )"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokTestCall)
    /*    10 */     << H(TokLiteral | TokNewStr) << S(L"blah\\")
    /*    17 */     << H(TokArgSeparator)
    /*    18 */     << H(TokLiteral | TokNewStr) << S(L"hi ,  \\ho")
    /*    29 */     << H(TokArgSeparator)
    /*    30 */     << H(TokLiteral | TokNewStr) << S(L"uh\\")
    /*    35 */     << H(TokArgSeparator)
    /*    36 */     << H(TokLiteral | TokNewStr) << S(L"\\oh")
    /*    41 */     << H(TokArgSeparator)
    /*    42 */     << H(TokArgSeparator)
    /*    43 */     << H(TokFuncTerminator))
            << "WARNING: in:1: Unescaped backslashes are deprecated\n"
               "WARNING: in:1: Unescaped backslashes are deprecated\n"
               "WARNING: in:1: Unescaped backslashes are deprecated\n"
               "WARNING: in:1: Unescaped backslashes are deprecated"
            << true;

    QTest::newRow("function with nested call")
            << "foo($$blah(hi ho))"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokTestCall)
    /*    10 */     << H(TokFuncName | TokNewStr) << HS(L"blah")
    /*    18 */         << H(TokLiteral | TokNewStr) << S(L"hi")
    /*    22 */         << H(TokLiteral | TokNewStr) << S(L"ho")
    /*    26 */         << H(TokFuncTerminator)
    /*    27 */     << H(TokFuncTerminator))
            << ""
            << true;

    QTest::newRow("stand-alone parentheses")
            << "()"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokTestCall)
    /*     3 */     << H(TokFuncTerminator))
            << "in:1: Opening parenthesis without prior test name."
            << false;

    QTest::newRow("bogus test and function")
            << "foo bar()"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokTestCall)
    /*     3 */     << H(TokFuncTerminator))
            << "in:1: Extra characters after test expression."
            << false;

    // This is a rather questionable "feature"
    QTest::newRow("two functions")
            << "foo() bar()"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokTestCall)
    /*    10 */     << H(TokFuncTerminator)
    /*    11 */ << H(TokHashLiteral) << HS(L"bar")
    /*    18 */ << H(TokTestCall)
    /*    19 */     << H(TokFuncTerminator))
            << ""
            << true;

    QTest::newRow("function-AND-test")
            << "foo():bar"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokTestCall)
    /*    10 */     << H(TokFuncTerminator)
    /*    11 */ << H(TokAnd)
    /*    12 */ << H(TokHashLiteral) << HS(L"bar")
    /*    19 */ << H(TokCondition))
            << ""
            << true;

    QTest::newRow("test-AND-function")
            << "foo:bar()"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition)
    /*    10 */ << H(TokAnd)
    /*    11 */ << H(TokHashLiteral) << HS(L"bar")
    /*    18 */ << H(TokTestCall)
    /*    19 */     << H(TokFuncTerminator))
            << ""
            << true;

    QTest::newRow("NOT-function-AND-test")
            << "!foo():bar"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokNot)
    /*     3 */ << H(TokHashLiteral) << HS(L"foo")
    /*    10 */ << H(TokTestCall)
    /*    11 */     << H(TokFuncTerminator)
    /*    12 */ << H(TokAnd)
    /*    13 */ << H(TokHashLiteral) << HS(L"bar")
    /*    20 */ << H(TokCondition))
            << ""
            << true;

    QTest::newRow("test-AND-NOT-function")
            << "foo:!bar()"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"foo")
    /*     9 */ << H(TokCondition)
    /*    10 */ << H(TokAnd)
    /*    11 */ << H(TokNot)
    /*    12 */ << H(TokHashLiteral) << HS(L"bar")
    /*    19 */ << H(TokTestCall)
    /*    20 */     << H(TokFuncTerminator))
            << ""
            << true;
}

void tst_qmakelib::addParseControlStatements()
{
    QTest::newRow("for(VAR, LIST) loop")
            << "for(VAR, LIST)"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokForLoop) << HS(L"VAR")
    /*     9 */ /* iterator */ << I(7)
    /*    11 */     << H(TokLiteral | TokNewStr) << S(L"LIST")
    /*    17 */     << H(TokValueTerminator)
    /*    18 */ /* body */ << I(1)
    /*    20 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("for(ever) loop")
            << "for(ever)"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokForLoop) << HS(L"")
    /*     6 */ /* iterator */ << I(9)
    /*     8 */     << H(TokHashLiteral) << HS(L"ever")
    /*    16 */     << H(TokValueTerminator)
    /*    17 */ /* body */ << I(1)
    /*    19 */     << H(TokTerminator))
            << ""
            << true;

    // This is a rather questionable "feature"
    QTest::newRow("for($$blub) loop")
            << "for($$blub)"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokForLoop) << HS(L"")
    /*     6 */ /* iterator */ << I(9)
    /*     8 */     << H(TokVariable | TokNewStr) << HS(L"blub")
    /*    16 */     << H(TokValueTerminator)
    /*    17 */ /* body */ << I(1)
    /*    19 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("test-for-test-else-test")
            << "true:for(VAR, LIST): true\nelse: true"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(31)
    /*    14 */     << H(TokForLoop) << HS(L"VAR")
    /*    21 */     /* iterator */ << I(7)
    /*    23 */         << H(TokLiteral | TokNewStr) << S(L"LIST")
    /*    29 */         << H(TokValueTerminator)
    /*    30 */     /* body */ << I(12)
    /*    32 */         << H(TokLine) << H(1)
    /*    34 */         << H(TokHashLiteral) << HS(L"true")
    /*    42 */         << H(TokCondition)
    /*    43 */         << H(TokTerminator)
    /*    44 */     << H(TokTerminator)
    /*    45 */ /* else branch */ << I(12)
    /*    47 */     << H(TokLine) << H(2)
    /*    49 */     << H(TokHashLiteral) << HS(L"true")
    /*    57 */     << H(TokCondition)
    /*    58 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("next()")
            << "for(ever): next()"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokForLoop) << HS(L"")
    /*     6 */ /* iterator */ << I(9)
    /*     8 */     << H(TokHashLiteral) << HS(L"ever")
    /*    16 */     << H(TokValueTerminator)
    /*    17 */ /* body */ << I(4)
    /*    19 */     << H(TokLine) << H(1)
    /*    21 */     << H(TokNext)
    /*    22 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("break()")
            << "for(ever): break()"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokForLoop) << HS(L"")
    /*     6 */ /* iterator */ << I(9)
    /*     8 */     << H(TokHashLiteral) << HS(L"ever")
    /*    16 */     << H(TokValueTerminator)
    /*    17 */ /* body */ << I(4)
    /*    19 */     << H(TokLine) << H(1)
    /*    21 */     << H(TokBreak)
    /*    22 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("top-level return()")
            << "return()"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokReturn))
            << ""
            << true;

    QTest::newRow("else")
            << "else"
            << TS()
            << "in:1: Unexpected 'else'."
            << false;

    QTest::newRow("test-{else}")
            << "test { else }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"test")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(1)
    /*    14 */     << H(TokTerminator)
    /*    15 */ /* else branch */ << I(0))
            << "in:1: Unexpected 'else'."
            << false;

    QTest::newRow("defineTest-{else}")
            << "defineTest(fn) { else }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokTestDef) << HS(L"fn")
    /*     8 */ /* body */ << I(1)
    /*    10 */     << H(TokTerminator))
            << "in:1: Unexpected 'else'."
            << false;

    QTest::newRow("for-else")
            << "for(ever) { else }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokForLoop) << HS(L"")
    /*     6 */ /* iterator */ << I(9)
    /*     8 */     << H(TokHashLiteral) << HS(L"ever")
    /*    16 */     << H(TokValueTerminator)
    /*    17 */ /* body */ << I(1)
    /*    19 */     << H(TokTerminator))
            << "in:1: Unexpected 'else'."
            << false;

    QTest::newRow("double-test-else")
            << "foo bar\nelse"
            << TS(
    /*     0 */ << H(TokBranch)
    /*     1 */ /* then branch */ << I(0)
    /*     3 */ /* else branch */ << I(1)  // This seems weird
    /*     5 */     << H(TokTerminator))
            << "in:1: Extra characters after test expression."
            << false;

    QTest::newRow("test-function-else")
            << "foo bar()\nelse"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokTestCall)  // This seems pointless
    /*     3 */     << H(TokFuncTerminator)
    /*     4 */ << H(TokBranch)
    /*     5 */ /* then branch */ << I(0)
    /*     7 */ /* else branch */ << I(1)  // This seems weird
    /*     9 */     << H(TokTerminator))
            << "in:1: Extra characters after test expression."
            << false;
}

void tst_qmakelib::addParseBraces()
{
    QTest::newRow("{}")
            << "{ }"
            << TS()
            << ""
            << true;

    QTest::newRow("{}-newlines")
            << "\n\n{ }\n\n"
            << TS()
            << ""
            << true;

    QTest::newRow("{")
            << "{"
            << TS()
            << "in:2: Missing closing brace(s)."
            << false;

    QTest::newRow("test {")
            << "test {"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"test")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(1)
    /*    14 */     << H(TokTerminator)
    /*    15 */ /* else branch */ << I(0))
            << "in:2: Missing closing brace(s)."
            << false;

    QTest::newRow("}")
            << "}"
            << TS()
            << "in:1: Excess closing brace."
            << false;

    QTest::newRow("{test}")
            << "{ true }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition))
            << ""
            << true;

    QTest::newRow("{test-newlines}")
            << "{\ntrue\n}"
            << TS(
    /*     0 */ << H(TokLine) << H(2)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition))
            << ""
            << true;

    QTest::newRow("{assignment-test}-test")
            << "{ VAR = { foo } bar } true"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"VAR")
    /*     9 */ << H(TokAssign) << H(4)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"{")
    /*    14 */ << H(TokLiteral | TokNewStr) << S(L"foo")
    /*    19 */ << H(TokLiteral | TokNewStr) << S(L"}")
    /*    22 */ << H(TokLiteral | TokNewStr) << S(L"bar")
    /*    27 */ << H(TokValueTerminator)
    /*    28 */ << H(TokHashLiteral) << HS(L"true")
    /*    36 */ << H(TokCondition))
            << ""
            << true;

    QTest::newRow("assignment with excess opening brace")
            << "VAR = { { foo }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"VAR")
    /*     9 */ << H(TokAssign) << H(4)
    /*    11 */ << H(TokLiteral | TokNewStr) << S(L"{")
    /*    14 */ << H(TokLiteral | TokNewStr) << S(L"{")
    /*    17 */ << H(TokLiteral | TokNewStr) << S(L"foo")
    /*    22 */ << H(TokLiteral | TokNewStr) << S(L"}")
    /*    25 */ << H(TokValueTerminator))
            << "WARNING: in:1: Possible braces mismatch"
            << true;

    QTest::newRow("test-{}")
            << "true {}"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(1)
    /*    14 */     << H(TokTerminator)
    /*    15 */ /* else branch */ << I(0))
            << ""
            << true;

    QTest::newRow("test-{newlines}")
            << "true {\n}"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(1)
    /*    14 */     << H(TokTerminator)
    /*    15 */ /* else branch */ << I(0))
            << ""
            << true;

    QTest::newRow("test-{test}")
            << "true { true }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(10)
    /*    14 */     << H(TokHashLiteral) << HS(L"true")
    /*    22 */     << H(TokCondition)
    /*    23 */     << H(TokTerminator)
    /*    24 */ /* else branch */ << I(0))
            << ""
            << true;

    QTest::newRow("test:-{test}")
            << "true: { true }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(10)
    /*    14 */     << H(TokHashLiteral) << HS(L"true")
    /*    22 */     << H(TokCondition)
    /*    23 */     << H(TokTerminator)
    /*    24 */ /* else branch */ << I(0))
            << "WARNING: in:1: Excess colon in front of opening brace."
            << true;

    QTest::newRow("test-{test-newlines}")
            << "true {\ntrue\n}"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(12)
    /*    14 */     << H(TokLine) << H(2)
    /*    16 */     << H(TokHashLiteral) << HS(L"true")
    /*    24 */     << H(TokCondition)
    /*    25 */     << H(TokTerminator)
    /*    26 */ /* else branch */ << I(0))
            << ""
            << true;

    QTest::newRow("test:-{test-newlines}")
            << "true: {\ntrue\n}"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(12)
    /*    14 */     << H(TokLine) << H(2)
    /*    16 */     << H(TokHashLiteral) << HS(L"true")
    /*    24 */     << H(TokCondition)
    /*    25 */     << H(TokTerminator)
    /*    26 */ /* else branch */ << I(0))
            << "WARNING: in:1: Excess colon in front of opening brace."
            << true;

    QTest::newRow("test-{assignment}")
            << "true { VAR = {foo} }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(18)
    /*    14 */     << H(TokHashLiteral) << HS(L"VAR")
    /*    21 */     << H(TokAssign) << H(0)
    /*    23 */     << H(TokLiteral | TokNewStr) << S(L"{foo}")
    /*    30 */     << H(TokValueTerminator)
    /*    31 */     << H(TokTerminator)
    /*    32 */ /* else branch */ << I(0))
            << ""
            << true;

    QTest::newRow("test-{test-assignment}")
            << "true { true: VAR = {foo} }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(33)
    /*    14 */     << H(TokHashLiteral) << HS(L"true")
    /*    22 */     << H(TokCondition)
    /*    23 */     << H(TokBranch)
    /*    24 */     /* then branch */ << I(18)
    /*    26 */         << H(TokHashLiteral) << HS(L"VAR")
    /*    33 */         << H(TokAssign) << H(0)
    /*    35 */         << H(TokLiteral | TokNewStr) << S(L"{foo}")
    /*    42 */         << H(TokValueTerminator)
    /*    43 */         << H(TokTerminator)
    /*    44 */     /* else branch */ << I(0)
    /*    46 */     << H(TokTerminator)
    /*    47 */ /* else branch */ << I(0))
            << ""
            << true;

    QTest::newRow("test-{assignment-newlines}")
            << "true {\nVAR = {foo}\n}"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(20)
    /*    14 */     << H(TokLine) << H(2)
    /*    16 */     << H(TokHashLiteral) << HS(L"VAR")
    /*    23 */     << H(TokAssign) << H(0)
    /*    25 */     << H(TokLiteral | TokNewStr) << S(L"{foo}")
    /*    32 */     << H(TokValueTerminator)
    /*    33 */     << H(TokTerminator)
    /*    34 */ /* else branch */ << I(0))
            << ""
            << true;

    QTest::newRow("test-{}-else-test-{}")
            << "true {} else: true {}"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(1)
    /*    14 */     << H(TokTerminator)
    /*    15 */ /* else branch */ << I(18)
    /*    17 */     << H(TokLine) << H(1)
    /*    19 */     << H(TokHashLiteral) << HS(L"true")
    /*    27 */     << H(TokCondition)
    /*    28 */     << H(TokBranch)
    /*    29 */     /* then branch */ << I(1)
    /*    31 */         << H(TokTerminator)
    /*    32 */     /* else branch */ << I(0)
    /*    34 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("test-{}-else-test-{}-newlines")
            << "true {\n}\nelse: true {\n}"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(1)
    /*    14 */     << H(TokTerminator)
    /*    15 */ /* else branch */ << I(18)
    /*    17 */     << H(TokLine) << H(3)
    /*    19 */     << H(TokHashLiteral) << HS(L"true")
    /*    27 */     << H(TokCondition)
    /*    28 */     << H(TokBranch)
    /*    29 */     /* then branch */ << I(1)
    /*    31 */         << H(TokTerminator)
    /*    32 */     /* else branch */ << I(0)
    /*    34 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("test-{test}-else-test-{}-newlines")
            << "true {\ntrue\n}\nelse: true {\n}"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(12)
    /*    14 */     << H(TokLine) << H(2)
    /*    16 */     << H(TokHashLiteral) << HS(L"true")
    /*    24 */     << H(TokCondition)
    /*    25 */     << H(TokTerminator)
    /*    26 */ /* else branch */ << I(18)
    /*    28 */     << H(TokLine) << H(4)
    /*    30 */     << H(TokHashLiteral) << HS(L"true")
    /*    38 */     << H(TokCondition)
    /*    39 */     << H(TokBranch)
    /*    40 */     /* then branch */ << I(1)
    /*    42 */         << H(TokTerminator)
    /*    43 */     /* else branch */ << I(0)
    /*    45 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("for-{next}")
            << "for(ever) { next() }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokForLoop) << HS(L"")
    /*     6 */ /* iterator */ << I(9)
    /*     8 */     << H(TokHashLiteral) << HS(L"ever")
    /*    16 */     << H(TokValueTerminator)
    /*    17 */ /* body */ << I(4)
    /*    19 */     << H(TokLine) << H(1)
    /*    21 */     << H(TokNext)
    /*    22 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("for:-{next}")
            << "for(ever): { next() }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokForLoop) << HS(L"")
    /*     6 */ /* iterator */ << I(9)
    /*     8 */     << H(TokHashLiteral) << HS(L"ever")
    /*    16 */     << H(TokValueTerminator)
    /*    17 */ /* body */ << I(4)
    /*    19 */     << H(TokLine) << H(1)
    /*    21 */     << H(TokNext)
    /*    22 */     << H(TokTerminator))
            << "WARNING: in:1: Excess colon in front of opening brace."
            << true;

    QTest::newRow("test-for-{test-else-test-newlines}")
            << "true:for(VAR, LIST) {\ntrue\nelse: true\n}"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(48)
    /*    14 */     << H(TokForLoop) << HS(L"VAR")
    /*    21 */     /* iterator */ << I(7)
    /*    23 */         << H(TokLiteral | TokNewStr) << S(L"LIST")
    /*    29 */         << H(TokValueTerminator)
    /*    30 */     /* body */ << I(29)
    /*    32 */         << H(TokLine) << H(2)
    /*    34 */         << H(TokHashLiteral) << HS(L"true")
    /*    42 */         << H(TokCondition)
    /*    43 */         << H(TokBranch)
    /*    44 */         /* then branch */ << I(0)
    /*    46 */         /* else branch */ << I(12)
    /*    48 */             << H(TokLine) << H(3)
    /*    50 */             << H(TokHashLiteral) << HS(L"true")
    /*    58 */             << H(TokCondition)
    /*    59 */             << H(TokTerminator)
    /*    60 */         << H(TokTerminator)
    /*    61 */     << H(TokTerminator)
    /*    62 */ /* else branch */ << I(0))
            << ""
            << true;

    QTest::newRow("test-for-{test-else-test}")
            << "true:for(VAR, LIST) { true\nelse: true }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"true")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(48)
    /*    14 */     << H(TokForLoop) << HS(L"VAR")
    /*    21 */     /* iterator */ << I(7)
    /*    23 */         << H(TokLiteral | TokNewStr) << S(L"LIST")
    /*    29 */         << H(TokValueTerminator)
    /*    30 */     /* body */ << I(29)
    /*    32 */         << H(TokLine) << H(1)
    /*    34 */         << H(TokHashLiteral) << HS(L"true")
    /*    42 */         << H(TokCondition)
    /*    43 */         << H(TokBranch)
    /*    44 */         /* then branch */ << I(0)
    /*    46 */         /* else branch */ << I(12)
    /*    48 */             << H(TokLine) << H(2)
    /*    50 */             << H(TokHashLiteral) << HS(L"true")
    /*    58 */             << H(TokCondition)
    /*    59 */             << H(TokTerminator)
    /*    60 */         << H(TokTerminator)
    /*    61 */     << H(TokTerminator)
    /*    62 */ /* else branch */ << I(0))
            << ""
            << true;
}

void tst_qmakelib::addParseCustomFunctions()
{
    QTest::newRow("defineTest-{newlines}")
            << "defineTest(test) {\n}"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokTestDef) << HS(L"test")
    /*    10 */ /* body */ << I(1)
    /*    12 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("defineTest:-test")
            << "defineTest(test): test"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokTestDef) << HS(L"test")
    /*    10 */ /* body */ << I(12)
    /*    12 */     << H(TokLine) << H(1)
    /*    14 */     << H(TokHashLiteral) << HS(L"test")
    /*    22 */     << H(TokCondition)
    /*    23 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("defineTest-{test}")
            << "defineTest(test) { test }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokTestDef) << HS(L"test")
    /*    10 */ /* body */ << I(12)
    /*    12 */     << H(TokLine) << H(1)
    /*    14 */     << H(TokHashLiteral) << HS(L"test")
    /*    22 */     << H(TokCondition)
    /*    23 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("defineTest-{return}")
            << "defineTest(test) { return() }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokTestDef) << HS(L"test")
    /*    10 */ /* body */ << I(4)
    /*    12 */     << H(TokLine) << H(1)
    /*    14 */     << H(TokReturn)
    /*    15 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("defineReplace-{return-stuff}")
            << "defineReplace(stuff) { return(foo bar) }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokReplaceDef) << HS(L"stuff")
    /*    11 */ /* body */ << I(14)
    /*    13 */     << H(TokLine) << H(1)
    /*    15 */     << H(TokLiteral | TokNewStr) << S(L"foo")
    /*    20 */     << H(TokLiteral | TokNewStr) << S(L"bar")
    /*    25 */     << H(TokReturn)
    /*    26 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("test-AND-defineTest-{}")
            << "test: defineTest(test) {}"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"test")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokAnd)
    /*    12 */ << H(TokTestDef) << HS(L"test")
    /*    20 */ /* body */ << I(1)
    /*    22 */     << H(TokTerminator))
            << ""
            << true;

    QTest::newRow("test-OR-defineTest-{}")
            << "test| defineTest(test) {}"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"test")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokOr)
    /*    12 */ << H(TokTestDef) << HS(L"test")
    /*    20 */ /* body */ << I(1)
    /*    22 */     << H(TokTerminator))
            << ""
            << true;
}

void tst_qmakelib::addParseAbuse()
{
    QTest::newRow("!")
            << ""
            << TS()
            << ""
            << true;

    QTest::newRow("|")
            << ""
            << TS()
            << ""
            << true;

    QTest::newRow(":")
            << ""
            << TS()
            << ""
            << true;

    QTest::newRow("NOT-assignment")
            << "!VAR ="
            << TS()
            << "in:1: Unexpected NOT operator in front of assignment."
            << false;

    QTest::newRow("NOT-{}")
            << "!{}"
            << TS()
            << "in:1: Unexpected NOT operator in front of opening brace."
            << false;

    QTest::newRow("NOT-else")
            << "test\n!else {}"
            << TS()
            << "in:2: Unexpected NOT operator in front of else."
            << false;

    QTest::newRow("NOT-for-{}")
            << "!for(ever) {}"
            << TS()
            << "in:1: Unexpected NOT operator in front of for()."
            << false;

    QTest::newRow("NOT-defineTest-{}")
            << "!defineTest(test) {}"
            << TS()
            << "in:1: Unexpected NOT operator in front of function definition."
            << false;

    QTest::newRow("AND-test")
            << ":test"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"test")
    /*    10 */ << H(TokCondition))
            << "in:1: AND operator without prior condition."
            << false;

    QTest::newRow("test-AND-else")
            << "test:else"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"test")
    /*    10 */ << H(TokCondition))
            << "in:1: Unexpected AND operator in front of else."
            << false;

    QTest::newRow("test-AND-AND-test")
            << "test::test"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"test")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokAnd)
    /*    12 */ << H(TokHashLiteral) << HS(L"test")
    /*    20 */ << H(TokCondition))
            << "WARNING: in:1: Stray AND operator in front of AND operator."
            << true;

    QTest::newRow("test-AND-OR-test")
            << "test:|test"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"test")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokOr)
    /*    12 */ << H(TokHashLiteral) << HS(L"test")
    /*    20 */ << H(TokCondition))
            << "WARNING: in:1: Stray AND operator in front of OR operator."
            << true;

    QTest::newRow("test-{AND-test}")
            << "test { :test }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"test")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(10)
    /*    14 */     << H(TokHashLiteral) << HS(L"test")
    /*    22 */     << H(TokCondition)
    /*    23 */     << H(TokTerminator)
    /*    24 */ /* else branch */ << I(0))
            << "in:1: AND operator without prior condition."
            << false;

    QTest::newRow("test-OR-assignment")
            << "foo| VAR ="
            << TS()
            << "in:1: Unexpected OR operator in front of assignment."
            << false;

    QTest::newRow("test-OR-{}")
            << "foo|{}"
            << TS()
            << "in:1: Unexpected OR operator in front of opening brace."
            << false;

    QTest::newRow("test-OR-for")
            << "foo|for(ever) {}"
            << TS()
            << "in:1: Unexpected OR operator in front of for()."
            << false;

    QTest::newRow("OR-test")
            << "|test"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"test")
    /*    10 */ << H(TokCondition))
            << "in:1: OR operator without prior condition."
            << false;

    QTest::newRow("test-OR-else")
            << "test|else"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"test")
    /*    10 */ << H(TokCondition))
            << "in:1: Unexpected OR operator in front of else."
            << false;

    QTest::newRow("test-OR-OR-test")
            << "test||test"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"test")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokOr)
    /*    12 */ << H(TokHashLiteral) << HS(L"test")
    /*    20 */ << H(TokCondition))
            << "WARNING: in:1: Stray OR operator in front of OR operator."
            << true;

    QTest::newRow("test-OR-AND-test")
            << "test|:test"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"test")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokAnd)
    /*    12 */ << H(TokHashLiteral) << HS(L"test")
    /*    20 */ << H(TokCondition))
            << "WARNING: in:1: Stray OR operator in front of AND operator."
            << true;

    QTest::newRow("test-{OR-test}")
            << "test { |test }"
            << TS(
    /*     0 */ << H(TokLine) << H(1)
    /*     2 */ << H(TokHashLiteral) << HS(L"test")
    /*    10 */ << H(TokCondition)
    /*    11 */ << H(TokBranch)
    /*    12 */ /* then branch */ << I(10)
    /*    14 */     << H(TokHashLiteral) << HS(L"test")
    /*    22 */     << H(TokCondition)
    /*    23 */     << H(TokTerminator)
    /*    24 */ /* else branch */ << I(0))
            << "in:1: OR operator without prior condition."
            << false;
}

void tst_qmakelib::proParser_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<QString>("msgs");
    QTest::addColumn<bool>("ok");

    QTest::newRow("empty")
            << ""
            << TS()
            << ""
            << true;

    QTest::newRow("empty (whitespace)")
            << "  \t   \t"
            << TS()
            << ""
            << true;

    addParseOperators();  // Variable operators

    addParseValues();

    addParseConditions();  // "Tests"

    addParseControlStatements();

    addParseBraces();

    addParseCustomFunctions();

    addParseAbuse();  // Mostly operator abuse

    // option() (these produce no tokens)

    QTest::newRow("option(host_build)")
            << "option(host_build)"
            << TS()
            << ""
            << true;

    QTest::newRow("option()")
            << "option()"
            << TS()
            << "in:1: option() requires one literal argument."
            << false;

    QTest::newRow("option(host_build magic)")
            << "option(host_build magic)"
            << TS()
            << "in:1: option() requires one literal argument."
            << false;

    QTest::newRow("option(host_build, magic)")
            << "option(host_build, magic)"
            << TS()
            << "in:1: option() requires one literal argument."
            << false;

    QTest::newRow("option($$OPTION)")
            << "option($$OPTION)"
            << TS()
            << "in:1: option() requires one literal argument."
            << false;

    QTest::newRow("{option(host_build)}")
            << "{option(host_build)}"
            << TS()
            << "in:1: option() must appear outside any control structures."
            << false;
}

QT_WARNING_POP

void tst_qmakelib::proParser()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(QString, msgs);
    QFETCH(bool, ok);

    bool verified = true;
    QMakeTestHandler handler;
    handler.setExpectedMessages(msgs.split('\n', QString::SkipEmptyParts));
    QMakeVfs vfs;
    QMakeParser parser(0, &vfs, &handler);
    ProFile *pro = parser.parsedProBlock(in, "in", 1, QMakeParser::FullGrammar);
    if (handler.printedMessages()) {
        qWarning("Got unexpected message(s)");
        verified = false;
    }
    QStringList missingMsgs = handler.expectedMessages();
    if (!missingMsgs.isEmpty()) {
        foreach (const QString &msg, missingMsgs)
            qWarning("Missing message: %s", qPrintable(msg));
        verified = false;
    }
    if (pro->isOk() != ok) {
        static const char * const lbl[] = { "failure", "success" };
        qWarning("Expected %s, got %s", lbl[int(ok)], lbl[1 - int(ok)]);
        verified = false;
    }
    if (pro->items() != out && (ok || !out.isEmpty())) {
        qWarning("Bytecode mismatch.\nActual:%s\nExpected:%s",
                 qPrintable(QMakeParser::formatProBlock(pro->items())),
                 qPrintable(QMakeParser::formatProBlock(out)));
        verified = false;
    }
    pro->deref();
    QVERIFY(verified);
}
