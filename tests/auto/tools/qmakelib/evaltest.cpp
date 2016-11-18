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

#include "tst_qmakelib.h"

#include <proitems.h>
#include <qmakevfs.h>
#include <qmakeparser.h>
#include <qmakeglobals.h>
#include <qmakeevaluator.h>

void tst_qmakelib::addAssignments()
{
    QTest::newRow("assignment")
            << "VAR = foo bar baz"
            << "VAR = foo bar baz"
            << ""
            << true;

    QTest::newRow("appending")
            << "VAR = foo bar baz\nVAR += foo gaz gaz"
            << "VAR = foo bar baz foo gaz gaz"
            << ""
            << true;

    QTest::newRow("unique appending")
            << "VAR = foo bar baz\nVAR *= foo gaz gaz"
            << "VAR = foo bar baz gaz"
            << ""
            << true;

    QTest::newRow("removing")
            << "VAR = foo bar foo baz\nVAR -= foo gaz gaz"
            << "VAR = bar baz"
            << ""
            << true;

    // Somewhat unexpectedly, the g modifier is implicit within each element.
    QTest::newRow("replacing")
            << "VAR = foo bar foo baz\nVAR ~= s,o,0,"
            << "VAR = f00 bar foo baz"
            << ""
            << true;

    // Consistently with the "there are no empty elements", what becomes empty gets zapped.
    QTest::newRow("replacing with nothing")
            << "VAR = foo bar foo baz\nVAR ~= s,foo,,"
            << "VAR = bar foo baz"
            << ""
            << true;

    QTest::newRow("replacing case-insensitively")
            << "VAR = foO bar foo baz\nVAR ~= s,o,0,i"
            << "VAR = f00 bar foo baz"
            << ""
            << true;

    // In all elements, not all within each/one/??? element.
    QTest::newRow("replacing globally")
            << "VAR = foo bar foo baz\nVAR ~= s,o,0,g"
            << "VAR = f00 bar f00 baz"
            << ""
            << true;

    // Replacing with the same string counts as no match.
    // This is rather questionable ...
    QTest::newRow("replacing with same")
            << "VAR = foo bar foo baz\nVAR ~= s,ba[rz],bar,"
            << "VAR = foo bar foo bar"
            << ""
            << true;

    QTest::newRow("replacing with auto-quote")
            << "VAR = foo [bar] foo baz\nVAR ~= s,[bar],bar,q"
            << "VAR = foo bar foo baz"
            << ""
            << true;

    QTest::newRow("replacing with expansions")
            << "VAR = foo bar foo baz\nPAT = foo\nREPL = 'yee haw'\nVAR ~= s,$$PAT,$$REPL,"
            << "VAR = 'yee haw' bar foo baz"
            << ""
            << true;

    QTest::newRow("~= with bad function")
            << "VAR ~= m/foo/"
            << ""
            << "##:1: The ~= operator can handle only the s/// function."
            << true; // rather questionable

    QTest::newRow("~= s with bad number of arguments")
            << "VAR ~= s/bla\nVAR ~= s/bla/foo//"
            << ""
            << "##:1: The s/// function expects 3 or 4 arguments.\n"
               "##:2: The s/// function expects 3 or 4 arguments."
            << true; // rather questionable
}

void tst_qmakelib::addExpansions()
{
    QTest::newRow("expand variable")
            << "V1 = foo\nVAR = $$V1"
            << "VAR = foo"
            << ""
            << true;

    QTest::newRow("expand property")
            << "VAR = $$[P1]"
            << "VAR = 'prop val'"
            << ""
            << true;

    QTest::newRow("expand environment variable")
            << "VAR = $$(E1)"
            << "VAR = 'env var'"
            << ""
            << true;

    // These test addStr/addStr.

    QTest::newRow("expand: str $$(env)")
            << "VAR = foo $$(E1)"
            << "VAR = foo 'env var'"
            << ""
            << true;

    QTest::newRow("expand: str$$(env)")
            << "VAR = foo$$(E1)"
            << "VAR = 'fooenv var'"
            << ""
            << true;

    QTest::newRow("expand: 'str $$(env)'")
            << "VAR = 'foo $$(E1)'"
            << "VAR = 'foo env var'"
            << ""
            << true;

    // These test addStr/addStrList

    QTest::newRow("expand: str $$var")
            << "V1 = foo barbaz\nVAR = str $$V1"
            << "VAR = str foo barbaz"
            << ""
            << true;

    QTest::newRow("expand: $$var str")
            << "V1 = foo barbaz\nVAR = $$V1 str"
            << "VAR = foo barbaz str"
            << ""
            << true;

    QTest::newRow("expand: str$$var")
            << "V1 = foo barbaz\nVAR = str$$V1"
            << "VAR = strfoo barbaz"
            << ""
            << true;

    QTest::newRow("expand: $${var}str")
            << "V1 = foo barbaz\nVAR = $${V1}str"
            << "VAR = foo barbazstr"
            << ""
            << true;

    QTest::newRow("expand: 'str $$var'")
            << "V1 = foo barbaz\nVAR = 'str $$V1'"
            << "VAR = 'str foo barbaz'"
            << ""
            << true;

    QTest::newRow("expand: '$$var str'")
            << "V1 = foo barbaz\nVAR = '$$V1 str'"
            << "VAR = 'foo barbaz str'"
            << ""
            << true;

    // Same again in joined context

    QTest::newRow("expand joined: str $$(env)")
            << "VAR = $$quote(foo $$(E1))"
            << "VAR = 'foo env var'"
            << ""
            << true;

    QTest::newRow("expand joined: str$$(env)")
            << "VAR = $$quote(foo$$(E1))"
            << "VAR = 'fooenv var'"
            << ""
            << true;

    QTest::newRow("expand joined: 'str $$(env)'")
            << "VAR = $$quote('foo $$(E1)')"
            << "VAR = 'foo env var'"
            << ""
            << true;

    QTest::newRow("expand joined: str $$var")
            << "V1 = foo barbaz\nVAR = $$quote(str $$V1)"
            << "VAR = 'str foo barbaz'"
            << ""
            << true;

    QTest::newRow("expand joined: $$var str")
            << "V1 = foo barbaz\nVAR = $$quote($$V1 str)"
            << "VAR = 'foo barbaz str'"
            << ""
            << true;

    QTest::newRow("expand joined: str$$var")
            << "V1 = foo barbaz\nVAR = $$quote(str$$V1)"
            << "VAR = 'strfoo barbaz'"
            << ""
            << true;

    QTest::newRow("expand joined: $${var}str")
            << "V1 = foo barbaz\nVAR = $$quote($${V1}str)"
            << "VAR = 'foo barbazstr'"
            << ""
            << true;

    QTest::newRow("expand joined: 'str $$var'")
            << "V1 = foo barbaz\nVAR = $$quote('str $$V1')"
            << "VAR = 'str foo barbaz'"
            << ""
            << true;

    QTest::newRow("expand joined: '$$var str'")
            << "V1 = foo barbaz\nVAR = $$quote('$$V1 str')"
            << "VAR = 'foo barbaz str'"
            << ""
            << true;

    // Variable expansions on LHS

    QTest::newRow("indirect assign: $$var")
            << "V = VAR\n$$V = foo"
            << "VAR = foo"
            << ""
            << true;

    QTest::newRow("indirect assign: fix$$var")
            << "V = AR\nV$$V = foo"
            << "VAR = foo"
            << ""
            << true;

    QTest::newRow("indirect assign: $${var}fix")
            << "V = VA\n$${V}R = foo"
            << "VAR = foo"
            << ""
            << true;

    QTest::newRow("indirect assign: eval")
            << "V = VAR\n$$eval(V) = foo"
            << "VAR = foo"
            << ""
            << true;

    QTest::newRow("indirect assign: multiple")
            << "V = FOO BAR\n$$V = foo"
            << ""
            << "##:2: Left hand side of assignment must expand to exactly one word."
            << true;
}

void tst_qmakelib::addControlStructs()
{
    QTest::newRow("true")
            << "true: VAR = 1"
            << "VAR = 1"
            << ""
            << true;

    QTest::newRow("false")
            << "false: VAR = 1"
            << "VAR = UNDEF"
            << ""
            << true;

    QTest::newRow("true-config")
            << "CONFIG += test\ntest: VAR = 1"
            << "VAR = 1"
            << ""
            << true;

    QTest::newRow("false-config")
            << "test: VAR = 1"
            << "VAR = UNDEF"
            << ""
            << true;

    QTest::newRow("true-wildcard")
            << "CONFIG += testing\ntest*: VAR = 1"
            << "VAR = 1"
            << ""
            << true;

    QTest::newRow("false-wildcard")
            << "test*: VAR = 1"
            << "VAR = UNDEF"
            << ""
            << true;

    QTest::newRow("true-else")
            << "true: VAR1 = 1\nelse: VAR2 = 1"
            << "VAR1 = 1\nVAR2 = UNDEF"
            << ""
            << true;

    QTest::newRow("false-else")
            << "false: VAR1 = 1\nelse: VAR2 = 1"
            << "VAR1 = UNDEF\nVAR2 = 1"
            << ""
            << true;

    QTest::newRow("true-else-true-else")
            << "true: VAR1 = 1\nelse: true: VAR2 = 1\nelse: VAR3 = 1"
            << "VAR1 = 1\nVAR2 = UNDEF\nVAR3 = UNDEF"
            << ""
            << true;

    QTest::newRow("true-else-false-else")
            << "true: VAR1 = 1\nelse: false: VAR2 = 1\nelse: VAR3 = 1"
            << "VAR1 = 1\nVAR2 = UNDEF\nVAR3 = UNDEF"
            << ""
            << true;

    QTest::newRow("false-else-true-else")
            << "false: VAR1 = 1\nelse: true: VAR2 = 1\nelse: VAR3 = 1"
            << "VAR1 = UNDEF\nVAR2 = 1\nVAR3 = UNDEF"
            << ""
            << true;

    QTest::newRow("false-else-false-else")
            << "false: VAR1 = 1\nelse: false: VAR2 = 1\nelse: VAR3 = 1"
            << "VAR1 = UNDEF\nVAR2 = UNDEF\nVAR3 = 1"
            << ""
            << true;

    QTest::newRow("true-{false-else}-else")
            << "true {\nfalse: VAR1 = 1\nelse: VAR2 = 1\n}\nelse: VAR3 = 1"
            << "VAR1 = UNDEF\nVAR2 = 1\nVAR3 = UNDEF"
            << ""
            << true;

    QTest::newRow("NOT-true")
            << "!true: VAR = 1"
            << "VAR = UNDEF"
            << ""
            << true;

    QTest::newRow("NOT-false")
            << "!false: VAR = 1"
            << "VAR = 1"
            << ""
            << true;

    QTest::newRow("true-AND-true")
            << "true:true: VAR = 1"
            << "VAR = 1"
            << ""
            << true;

    QTest::newRow("true-AND-false")
            << "true:false: VAR = 1"
            << "VAR = UNDEF"
            << ""
            << true;

    QTest::newRow("false-AND-true")
            << "false:true: VAR = 1"
            << "VAR = UNDEF"
            << ""
            << true;

    QTest::newRow("false-OR-false")
            << "false|false: VAR = 1"
            << "VAR = UNDEF"
            << ""
            << true;

    QTest::newRow("true-OR-false")
            << "true|false: VAR = 1"
            << "VAR = 1"
            << ""
            << true;

    QTest::newRow("false-OR-true")
            << "false|true: VAR = 1"
            << "VAR = 1"
            << ""
            << true;

    QTest::newRow("NOT-false-AND-true")
            << "!false:true: VAR = 1"
            << "VAR = 1"
            << ""
            << true;

    QTest::newRow("true-AND-message")
            << "true:message(hi): VAR = 1"
            << "VAR = 1"
            << "Project MESSAGE: hi"
            << true;

    QTest::newRow("false-AND-message")
            << "false:message(hi): VAR = 1"
            << "VAR = UNDEF"
            << ""
            << true;

    QTest::newRow("true-OR-message")
            << "true|message(hi): VAR = 1"
            << "VAR = 1"
            << ""
            << true;

    QTest::newRow("false-OR-message")
            << "false|message(hi): VAR = 1"
            << "VAR = 1"
            << "Project MESSAGE: hi"
            << true;

    QTest::newRow("true-OR-message-AND-false")
            << "true|message(hi):false: VAR = 1"
            << "VAR = UNDEF"
            << ""
            << true;

    QTest::newRow("false-OR-message-AND-false")
            << "false|message(hi):false: VAR = 1"
            << "VAR = UNDEF"
            << "Project MESSAGE: hi"
            << true;

    QTest::newRow("true (indirect)")
            << "TEST = true\n$$TEST: VAR = 1"
            << "VAR = 1"
            << ""
            << true;

    QTest::newRow("false (indirect)")
            << "TEST = false\n$$TEST: VAR = 1"
            << "VAR = UNDEF"
            << ""
            << true;

    // Yes, this is not supposed to work
    QTest::newRow("true|false (indirect)")
            << "TEST = true|false\n$$TEST: VAR = 1"
            << "VAR = UNDEF"
            << ""
            << true;

    QTest::newRow("for (var, var)")
            << "IN = one two three\nfor (IT, IN) { OUT += $$IT }"
            << "OUT = one two three"
            << ""
            << true;

    QTest::newRow("for (var, range)")
            << "for (IT, 1..3) { OUT += $$IT }"
            << "OUT = 1 2 3"
            << ""
            << true;

    QTest::newRow("for (var, reverse-range)")
            << "for (IT, 3..1) { OUT += $$IT }"
            << "OUT = 3 2 1"
            << ""
            << true;

    // This syntax is rather ridiculous.
    QTest::newRow("for (ever)")
            << "for (ever) {}"
            << ""
            << "##:1: Ran into infinite loop (> 1000 iterations)."
            << true;

    // This is even worse.
    QTest::newRow("for (VAR, forever)")
            << "for (VAR, forever) { OUT = $$VAR }"
            << "OUT = 999"
            << "##:1: Ran into infinite loop (> 1000 iterations)."
            << true;

    QTest::newRow("for (garbage)")
            << "for (garbage) { OUT = FAIL }"
            << "OUT = UNDEF"
            << "##:1: Invalid loop expression."
            << true;

    QTest::newRow("next()")
            << "IN = one two three\nfor (IT, IN) {\nequals(IT, two):next()\nOUT += $$IT\n}"
            << "OUT = one three"
            << ""
            << true;

    QTest::newRow("nested next()")
            << "IN = one two three\nfor (IT, IN) {\nfor (NIT, IN):next()\nOUT += $$IT\n}"
            << "OUT = one two three"
            << ""
            << true;

    QTest::newRow("break()")
            << "IN = one two three\nfor (IT, IN) {\nequals(IT, three):break()\nOUT += $$IT\n}"
            << "OUT = one two"
            << ""
            << true;

    QTest::newRow("nested break()")
            << "IN = one two three\nfor (IT, IN) {\nfor (NIT, IN):break()\nOUT += $$IT\n}"
            << "OUT = one two three"
            << ""
            << true;

    QTest::newRow("defineReplace()")
            << "defineReplace(func) { return($$1 + $$2) }\n"
               "VAR = $$func(test me, \"foo bar\")"
            << "VAR = test me + 'foo bar'"
            << ""
            << true;

    QTest::newRow("defineTest()")
            << "defineTest(func) { return($$1) }\n"
               "func(true): VAR += true\n"
               "func(false): VAR += false"
            << "VAR = true"
            << ""
            << true;

    QTest::newRow("true-AND-defineTest()")
            << "true: defineTest(func)\n"
               "defined(func): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("false-AND-defineTest()")
            << "false: defineTest(func)\n"
               "defined(func): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("true-OR-defineTest()")
            << "true| defineTest(func)\n"
               "defined(func): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("false-OR-defineTest()")
            << "false| defineTest(func)\n"
               "defined(func): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("variable scoping")
            << "defineTest(func) {\n"
                   "VAR1 = modified\n!equals(VAR1, modified): return(false)\n"
                   "VAR2 += modified\n!equals(VAR2, original modified): return(false)\n"
                   "VAR3 = new var\n!equals(VAR3, new var): return(false)\n"
                   "return(true)\n"
               "}\n"
               "VAR1 = pristine\nVAR2 = original\nfunc(): OK = 1"
            << "OK = 1\nVAR1 = pristine\nVAR2 = original\nVAR3 = UNDEF"
            << ""
            << true;

    QTest::newRow("function arguments")
            << "defineTest(func) {\n"
                   "defined(1, var) {\nd1 = 1\nexport(d1)\n}\n"
                   "defined(3, var) {\nd3 = 1\nexport(d3)\n}\n"
                   "x1 = $$1\nexport(x1)\n"
                   "2 += foo\nx2 = $$2\nexport(x2)\n"
                   "x3 = $$3\nexport(x3)\n"
                   "4 += foo\nx4 = $$4\nexport(x4)\n"
                   "x5 = $$5\nexport(x5)\n"
                   "6 += foo\nx6 = $$6\nexport(x6)\n"
               "}\n"
               "1 = first\n2 = second\n3 = third\n4 = fourth\nfunc(one, two)"
            << "1 = first\n2 = second\n3 = third\n4 = fourth\n5 = UNDEF\n6 = UNDEF\n"
               "d1 = 1\nd3 = UNDEF\nx1 = one\nx2 = two foo\nx3 =\nx4 = foo\nx5 =\nx6 = foo"
            << ""
            << true;

    QTest::newRow("ARGC and ARGS")
            << "defineTest(func) {\n"
                   "export(ARGC)\n"
                   "export(ARGS)\n"
               "}\n"
               "func(test me, \"foo bar\")"
            << "ARGC = 2\nARGS = test me 'foo bar'"
            << ""
            << true;

    QTest::newRow("recursion")
            << "defineReplace(func) {\n"
                   "RET = *$$member(1, 0)*\n"
                   "REST = $$member(1, 1, -1)\n"
                   "!isEmpty(REST): RET += $$func($$REST)\n"
                   "return($$RET)\n"
               "}\n"
               "VAR = $$func(you are ...)"
            << "VAR = *you* *are* *...*"
            << ""
            << true;

    QTest::newRow("top-level return()")
            << "VAR = good\nreturn()\nVAR = bad"
            << "VAR = good"
            << ""
            << true;

    QTest::newRow("return() from function")
            << "defineTest(func) {\nVAR = good\nexport(VAR)\nreturn()\nVAR = bad\nexport(VAR)\n}\n"
               "func()"
            << "VAR = good"
            << ""
            << true;

    QTest::newRow("return() from nested function")
            << "defineTest(inner) {\nVAR = initial\nexport(VAR)\nreturn()\nVAR = bad\nexport(VAR)\n}\n"
               "defineTest(outer) {\ninner()\nVAR = final\nexport(VAR)\n}\n"
               "outer()"
            << "VAR = final"
            << ""
            << true;

    QTest::newRow("error() from replace function (assignment)")
            << "defineReplace(func) {\nerror(error)\n}\n"
               "VAR = $$func()\n"
               "OKE = 1"
            << "VAR = UNDEF\nOKE = UNDEF"
            << "Project ERROR: error"
            << false;

    QTest::newRow("error() from replace function (replacement)")
            << "defineReplace(func) {\nerror(error)\n}\n"
               "VAR = $$func()\n"
               "OKE = 1"
            << "VAR = UNDEF\nOKE = UNDEF"
            << "Project ERROR: error"
            << false;

    QTest::newRow("error() from replace function (LHS)")
            << "defineReplace(func) {\nerror(error)\nreturn(VAR)\n}\n"
               "$$func() = 1\n"
               "OKE = 1"
            << "VAR = UNDEF\nOKE = UNDEF"
            << "Project ERROR: error"
            << false;

    QTest::newRow("error() from replace function (loop variable)")
            << "defineReplace(func) {\nerror(error)\nreturn(BLAH)\n}\n"
               "for($$func()) {\nVAR = $$BLAH\nbreak()\n}\n"
               "OKE = 1"
            << "VAR = UNDEF\nOKE = UNDEF"
            << "Project ERROR: error"
            << false;

    QTest::newRow("error() from replace function (built-in test arguments)")
            << "defineReplace(func) {\nerror(error)\n}\n"
               "message($$func()): VAR = 1\n"
               "OKE = 1"
            << "VAR = UNDEF\nOKE = UNDEF"
            << "Project ERROR: error"
            << false;

    QTest::newRow("error() from replace function (built-in replace arguments)")
            << "defineReplace(func) {\nerror(error)\n}\n"
               "VAR = $$upper($$func())\n"
               "OKE = 1"
            << "VAR = UNDEF\nOKE = UNDEF"
            << "Project ERROR: error"
            << false;

    QTest::newRow("error() from replace function (custom test arguments)")
            << "defineReplace(func) {\nerror(error)\n}\n"
               "defineTest(custom) {\n}\n"
               "custom($$func()): VAR = 1\n"
               "OKE = 1"
            << "VAR = UNDEF\nOKE = UNDEF"
            << "Project ERROR: error"
            << false;

    QTest::newRow("error() from replace function (custom replace arguments)")
            << "defineReplace(func) {\nerror(error)\nreturn(1)\n}\n"
               "defineReplace(custom) {\nreturn($$1)\n}\n"
               "VAR = $$custom($$func(1))\n"
               "OKE = 1"
            << "VAR = UNDEF\nOKE = UNDEF"
            << "Project ERROR: error"
            << false;

    QTest::newRow("REQUIRES = error()")
            << "REQUIRES = error(error)\n"
               "OKE = 1"
            << "OKE = UNDEF"
            << "Project ERROR: error"
            << false;

    QTest::newRow("requires(error())")
            << "requires(error(error))\n"
               "OKE = 1"
            << "OKE = UNDEF"
            << "Project ERROR: error"
            << false;
}

void tst_qmakelib::addReplaceFunctions(const QString &qindir)
{
    QTest::newRow("$$member(): empty")
            << "IN = \nVAR = $$member(IN)"
            << "VAR ="
            << ""
            << true;

    QTest::newRow("$$member(): too short")
            << "IN = one two three\nVAR = $$member(IN, 1, 5)"
            << "VAR ="  // this is actually kinda stupid
            << ""
            << true;

    QTest::newRow("$$member(): ok")
            << "IN = one two three four five six seven\nVAR = $$member(IN, 1, 4)"
            << "VAR = two three four five"
            << ""
            << true;

    QTest::newRow("$$member(): ok (default start)")
            << "IN = one two three\nVAR = $$member(IN)"
            << "VAR = one"
            << ""
            << true;

    QTest::newRow("$$member(): ok (default end)")
            << "IN = one two three\nVAR = $$member(IN, 2)"
            << "VAR = three"
            << ""
            << true;

    QTest::newRow("$$member(): negative")
            << "IN = one two three four five six seven\nVAR = $$member(IN, -4, -3)"
            << "VAR = four five"
            << ""
            << true;

    QTest::newRow("$$member(): inverse")
            << "IN = one two three four five six seven\nVAR = $$member(IN, 4, 1)"
            << "VAR = five four three two"
            << ""
            << true;

    QTest::newRow("$$member(): dots")
            << "IN = one two three four five six seven\nVAR = $$member(IN, 1..4)"
            << "VAR = two three four five"
            << ""
            << true;

    QTest::newRow("$$member(): bad number of arguments")
            << "VAR = $$member(1, 2, 3, 4)"
            << "VAR ="
            << "##:1: member(var, start, end) requires one to three arguments."
            << true;

    QTest::newRow("$$member(): bad args (1)")
            << "IN = one two three\nVAR = $$member(IN, foo, 4)"
            << "VAR ="
            << "##:2: member() argument 2 (start) 'foo' invalid."
            << true;

    QTest::newRow("$$member(): bad args (2)")
            << "IN = one two three\nVAR = $$member(IN, foo..4)"
            << "VAR ="
            << "##:2: member() argument 2 (start) 'foo..4' invalid."
            << true;

    QTest::newRow("$$member(): bad args (3)")
            << "IN = one two three\nVAR = $$member(IN, 4, foo)"
            << "VAR ="
            << "##:2: member() argument 3 (end) 'foo' invalid."
            << true;

    QTest::newRow("$$member(): bad args (4)")
            << "IN = one two three\nVAR = $$member(IN, 4..foo)"
            << "VAR ="
            << "##:2: member() argument 2 (start) '4..foo' invalid."
            << true;

    // The argument processing is shared with $$member(), so some tests are skipped.
    QTest::newRow("$$str_member(): empty")
            << "VAR = $$str_member()"
            << "VAR ="
            << ""
            << true;

    QTest::newRow("$$str_member(): too short")
            << "VAR = $$str_member(string_value, 7, 12)"
            << "VAR ="  // this is actually kinda stupid
            << ""
            << true;

    QTest::newRow("$$str_member(): ok")
            << "VAR = $$str_member(string_value, 7, 11)"
            << "VAR = value"
            << ""
            << true;

    QTest::newRow("$$str_member(): ok (default start)")
            << "VAR = $$str_member(string_value)"
            << "VAR = s"
            << ""
            << true;

    QTest::newRow("$$str_member(): ok (default end)")
            << "VAR = $$str_member(string_value, 7)"
            << "VAR = v"
            << ""
            << true;

    QTest::newRow("$$str_member(): negative")
            << "VAR = $$str_member(string_value, -5, -3)"
            << "VAR = val"
            << ""
            << true;

    QTest::newRow("$$str_member(): inverse")
            << "VAR = $$str_member(string_value, -2, 1)"
            << "VAR = ulav_gnirt"
            << ""
            << true;

    QTest::newRow("$$first(): empty")
            << "IN = \nVAR = $$first(IN)"
            << "VAR ="
            << ""
            << true;

    QTest::newRow("$$first(): one")
            << "IN = one\nVAR = $$first(IN)"
            << "VAR = one"
            << ""
            << true;

    QTest::newRow("$$first(): multiple")
            << "IN = one two three\nVAR = $$first(IN)"
            << "VAR = one"
            << ""
            << true;

    QTest::newRow("$$first(): bad number of arguments")
            << "VAR = $$first(1, 2)"
            << "VAR ="
            << "##:1: first(var) requires one argument."
            << true;

    QTest::newRow("$$take_first(): empty")
            << "IN = \nVAR = $$take_first(IN)"
            << "VAR =\nIN ="
            << ""
            << true;

    QTest::newRow("$$take_first(): one")
            << "IN = one\nVAR = $$take_first(IN)"
            << "VAR = one\nIN ="
            << ""
            << true;

    QTest::newRow("$$take_first(): multiple")
            << "IN = one two three\nVAR = $$take_first(IN)"
            << "VAR = one\nIN = two three"
            << ""
            << true;

    QTest::newRow("$$take_first(): bad number of arguments")
            << "VAR = $$take_first(1, 2)"
            << "VAR ="
            << "##:1: take_first(var) requires one argument."
            << true;

    QTest::newRow("$$last(): empty")
            << "IN = \nVAR = $$last(IN)"
            << "VAR ="
            << ""
            << true;

    QTest::newRow("$$last(): one")
            << "IN = one\nVAR = $$last(IN)"
            << "VAR = one"
            << ""
            << true;

    QTest::newRow("$$last(): multiple")
            << "IN = one two three\nVAR = $$last(IN)"
            << "VAR = three"
            << ""
            << true;

    QTest::newRow("$$last(): bad number of arguments")
            << "VAR = $$last(1, 2)"
            << "VAR ="
            << "##:1: last(var) requires one argument."
            << true;

    QTest::newRow("$$take_last(): empty")
            << "IN = \nVAR = $$take_last(IN)"
            << "VAR =\nIN ="
            << ""
            << true;

    QTest::newRow("$$take_last(): one")
            << "IN = one\nVAR = $$take_last(IN)"
            << "VAR = one\nIN ="
            << ""
            << true;

    QTest::newRow("$$take_last(): multiple")
            << "IN = one two three\nVAR = $$take_last(IN)"
            << "VAR = three\nIN = one two"
            << ""
            << true;

    QTest::newRow("$$take_last(): bad number of arguments")
            << "VAR = $$take_last(1, 2)"
            << "VAR ="
            << "##:1: take_last(var) requires one argument."
            << true;

    QTest::newRow("$$size()")
            << "IN = one two three\nVAR = $$size(IN)"
            << "VAR = 3"
            << ""
            << true;

    QTest::newRow("$$size(): bad number of arguments")
            << "VAR = $$size(1, 2)"
            << "VAR ="
            << "##:1: size(var) requires one argument."
            << true;

    QTest::newRow("$$str_size()")
            << "VAR = $$str_size(one two three)"
            << "VAR = 13"
            << ""
            << true;

    QTest::newRow("$$str_size(): bad number of arguments")
            << "VAR = $$str_size(1, 2)"
            << "VAR ="
            << "##:1: str_size(str) requires one argument."
            << true;

    QTest::newRow("$$fromfile(): right var")
            << "VAR = $$fromfile(" + qindir + "/fromfile/infile.prx, DEFINES)"
            << "VAR = QT_DLL"
            << ""
            << true;

    QTest::newRow("$$fromfile(): wrong var")
            << "VAR = $$fromfile(" + qindir + "/fromfile/infile.prx, INCLUDES)"
            << "VAR ="
            << ""
            << true;

    QTest::newRow("$$fromfile(): bad file")
            << "VAR = $$fromfile(" + qindir + "/fromfile/badfile.prx, DEFINES)"
            << "VAR ="
            << "Project ERROR: fail!"
            << true;

    QTest::newRow("$$fromfile(): bad number of arguments")
            << "VAR = $$fromfile(1) \\\n$$fromfile(1, 2, 3)"
            << "VAR ="
            << "##:1: fromfile(file, variable) requires two arguments.\n"
               "##:2: fromfile(file, variable) requires two arguments."
            << true;

    QTest::newRow("$$eval()")
            << "IN = one two three\nVAR = $$eval(IN)"
            << "VAR = one two three"
            << ""
            << true;

    QTest::newRow("$$eval(): bad number of arguments")
            << "VAR = $$eval(1, 2)"
            << "VAR ="
            << "##:1: eval(variable) requires one argument."
            << true;

    QTest::newRow("$$list()")
            << "VARNAME = $$list(one, two three, 'four five')\nVAR = $$eval($$VARNAME)"
            << "VAR = one two three four five"  // total nonsense ...
            << ""
            << true;

    QTest::newRow("$$sprintf()")
            << "VAR = $$sprintf(hello %1 %2, you, there)"
            << "VAR = 'hello you there'"
            << ""
            << true;

    QTest::newRow("$$format_number(): simple number format")
            << "VAR = $$format_number(13)"
            << "VAR = 13"
            << ""
            << true;

    QTest::newRow("$$format_number(): negative number format")
            << "VAR = $$format_number(-13)"
            << "VAR = -13"
            << ""
            << true;

    QTest::newRow("$$format_number(): hex input number format")
            << "VAR = $$format_number(13, ibase=16)"
            << "VAR = 19"
            << ""
            << true;

    QTest::newRow("$$format_number(): hex output number format")
            << "VAR = $$format_number(13, obase=16)"
            << "VAR = d"
            << ""
            << true;

    QTest::newRow("$$format_number(): right aligned number format")
            << "VAR = $$format_number(13, width=5)"
            << "VAR = '   13'"
            << ""
            << true;

    QTest::newRow("$$format_number(): left aligned number format")
            << "VAR = $$format_number(13, width=5 leftalign)"
            << "VAR = '13   '"
            << ""
            << true;

    QTest::newRow("$$format_number(): zero-padded number format")
            << "VAR = $$format_number(13, width=5 zeropad)"
            << "VAR = 00013"
            << ""
            << true;

    QTest::newRow("$$format_number(): always signed number format")
            << "VAR = $$format_number(13, width=5 alwayssign)"
            << "VAR = '  +13'"
            << ""
            << true;

    QTest::newRow("$$format_number(): zero-padded always signed number format")
            << "VAR = $$format_number(13, width=5 alwayssign zeropad)"
            << "VAR = +0013"
            << ""
            << true;

    QTest::newRow("$$format_number(): sign-padded number format")
            << "VAR = $$format_number(13, width=5 padsign)"
            << "VAR = '   13'"
            << ""
            << true;

    QTest::newRow("$$format_number(): zero-padded sign-padded number format")

            << "VAR = $$format_number(13, width=5 padsign zeropad)"
            << "VAR = ' 0013'"
            << ""
            << true;

    QTest::newRow("$$format_number(): bad number of arguments")
            << "VAR = $$format_number(13, 1, 2)"
            << "VAR ="
            << "##:1: format_number(number[, options...]) requires one or two arguments."
            << true;

    QTest::newRow("$$format_number(): invalid option")
            << "VAR = $$format_number(13, foo=bar)"
            << "VAR ="
            << "##:1: format_number(): invalid format option foo=bar."
            << true;

    QTest::newRow("$$num_add(): one")
            << "VAR = $$num_add(10)"
            << "VAR = 10"
            << ""
            << true;

    QTest::newRow("$$num_add(): two")
            << "VAR = $$num_add(1, 2)"
            << "VAR = 3"
            << ""
            << true;

    QTest::newRow("$$num_add(): three")
            << "VAR = $$num_add(1, 3, 5)"
            << "VAR = 9"
            << ""
            << true;

    QTest::newRow("$$num_add(): negative")
            << "VAR = $$num_add(7, -13)"
            << "VAR = -6"
            << ""
            << true;

    QTest::newRow("$$num_add(): bad number of arguments")
            << "VAR = $$num_add()"
            << "VAR = "
            << "##:1: num_add(num, ...) requires at least one argument."
            << true;

    QTest::newRow("$$num_add(): bad number: float")
            << "VAR = $$num_add(1.1)"
            << "VAR ="
            << "##:1: num_add(): floats are currently not supported."
            << true;

    QTest::newRow("$$num_add(): bad number: malformed")
            << "VAR = $$num_add(fail)"
            << "VAR ="
            << "##:1: num_add(): malformed number fail."
            << true;

    QTest::newRow("$$join(): empty")
            << "IN = \nVAR = $$join(IN, //)"
            << "VAR ="
            << ""
            << true;

    QTest::newRow("$$join(): multiple")
            << "IN = one two three\nVAR = $$join(IN, //)"
            << "VAR = one//two//three"
            << ""
            << true;

    QTest::newRow("$$join(): multiple surrounded")
            << "IN = one two three\nVAR = $$join(IN, //, <<, >>)"
            << "VAR = <<one//two//three>>"
            << ""
            << true;

    QTest::newRow("$$join(): bad number of arguments")
            << "VAR = $$join(1, 2, 3, 4, 5)"
            << "VAR ="
            << "##:1: join(var, glue, before, after) requires one to four arguments."
            << true;

    QTest::newRow("$$split(): default sep")
            << "IN = 'one/two three' 'four / five'\nVAR = $$split(IN)"
            << "VAR = one/two three four / five"
            << ""
            << true;

    QTest::newRow("$$split(): specified sep")
            << "IN = 'one/two three' 'four / five'\nVAR = $$split(IN, /)"
            << "VAR = one 'two three' 'four ' ' five'"
            << ""
            << true;

    QTest::newRow("$$split(): bad number of arguments")
            << "VAR = $$split(1, 2, 3)"
            << "VAR ="
            << "##:1: split(var, sep) requires one or two arguments."
            << true;

    QTest::newRow("$$basename(): empty")
            << "IN = \nVAR = $$basename(IN)"
            << "VAR ="
            << ""
            << true;

    QTest::newRow("$$basename(): bare")
            << "IN = file\nVAR = $$basename(IN)"
            << "VAR = file"
            << ""
            << true;

    QTest::newRow("$$basename(): relative")
            << "IN = path/file\nVAR = $$basename(IN)"
            << "VAR = file"
            << ""
            << true;

    QTest::newRow("$$basename(): absolute")
            << "IN = \\\\path\\\\file\nVAR = $$basename(IN)"
            << "VAR = file"
            << ""
            << true;

    QTest::newRow("$$basename(): bad number of arguments")
            << "VAR = $$basename(1, 2)"
            << "VAR ="
            << "##:1: basename(var) requires one argument."
            << true;

    QTest::newRow("$$dirname(): empty")
            << "IN = \nVAR = $$dirname(IN)"
            << "VAR ="
            << ""
            << true;

    QTest::newRow("$$dirname(): bare")
            << "IN = file\nVAR = $$dirname(IN)"
            << "VAR ="
            << ""
            << true;

    QTest::newRow("$$dirname(): relative")
            << "IN = path/file\nVAR = $$dirname(IN)"
            << "VAR = path"
            << ""
            << true;

    QTest::newRow("$$dirname(): absolute")
            << "IN = \\\\path\\\\file\nVAR = $$dirname(IN)"
            << "VAR = \\\\path"
            << ""
            << true;

    QTest::newRow("$$dirname(): bad number of arguments")
            << "VAR = $$dirname(1, 2)"
            << "VAR ="
            << "##:1: dirname(var) requires one argument."
            << true;

    QTest::newRow("$$section(): explicit end")
            << "IN = one~two~three~four~five~six\nVAR = $$section(IN, ~, 2, 4)"
            << "VAR = three~four~five"
            << ""
            << true;

    QTest::newRow("$$section(): implicit end")
            << "IN = one~two~three~four~five~six\nVAR = $$section(IN, ~, 3)"
            << "VAR = four~five~six"
            << ""
            << true;

    QTest::newRow("$$section(): bad number of arguments")
            << "VAR = $$section(1, 2) \\\n$$section(1, 2, 3, 4, 5)"
            << "VAR ="
            << "##:1: section(var, sep, begin, end) requires three or four arguments.\n"
               "##:2: section(var, sep, begin, end) requires three or four arguments."
            << true;

    QTest::newRow("$$find()")
            << "IN = foo bar baz blubb\nVAR = $$find(IN, ^ba)"
            << "VAR = bar baz"
            << ""
            << true;

    QTest::newRow("$$find(): bad number of arguments")
            << "VAR = $$find(1) \\\n$$find(1, 2, 3)"
            << "VAR ="
            << "##:1: find(var, str) requires two arguments.\n"
               "##:2: find(var, str) requires two arguments."
            << true;

    // FIXME: $$cat() & $$system(): There is no way to generate the newlines
    // necessary for testing "multi-line" and "blob" mode adequately.
    // Note: these functions have *different* splitting behavior.

    // This gives split_value_list() an exercise
    QTest::newRow("$$cat(): default mode")
            << "VAR = $$cat(" + qindir + "/cat/file2.txt)"
            << "VAR = foo bar baz \"\\\"Hello, \\' world.\\\"\" post \"\\'Hello, \\\" world.\\'\" post \\\\\\\" \\\\\\' \\\\\\\\ \\\\a \\\\ nix \"\\\" \\\"\""
            << ""
            << true;

    QTest::newRow("$$cat(): lines mode")
            << "VAR = $$cat(" + qindir + "/cat/file1.txt, lines)"
            << "VAR = '\"Hello, world.\"' 'foo bar baz'"
            << ""
            << true;

    QTest::newRow("$$cat(): bad number of arguments")
            << "VAR = $$cat(1, 2, 3)"
            << "VAR ="
            << "##:1: cat(file, singleline=true) requires one or two arguments."
            << true;

    QTest::newRow("$$system(): default mode")
#ifdef Q_OS_WIN
            << "VAR = $$system('echo Hello, ^\"world.&& echo foo^\" bar baz')"
#else
            << "VAR = $$system('echo Hello, \\\\\\\"world. && echo foo\\\\\\\" bar baz')"
#endif
            << "VAR = Hello, '\"world. foo\"' bar baz"
            << ""
            << true;

    QTest::newRow("$$system(): lines mode")
#ifdef Q_OS_WIN
            << "VAR = $$system('echo Hello, ^\"world.&& echo foo^\" bar baz', lines)"
#else
            << "VAR = $$system('echo Hello, \\\\\\\"world. && echo foo\\\\\\\" bar baz', lines)"
#endif
            << "VAR = 'Hello, \"world.' 'foo\" bar baz'"
            << ""
            << true;

    QTest::newRow("$$system(): bad number of arguments")
            << "VAR = $$system(1, 2, 3, 4)"
            << "VAR ="
            << "##:1: system(command, [mode], [stsvar]) requires one to three arguments."
            << true;

    QTest::newRow("$$unique()")
            << "IN = foo bar foo baz\nVAR = $$unique(IN)"
            << "VAR = foo bar baz"
            << ""
            << true;

    QTest::newRow("$$unique(): bad number of arguments")
            << "VAR = $$unique(1, 2)"
            << "VAR ="
            << "##:1: unique(var) requires one argument."
            << true;

    QTest::newRow("$$sorted()")
            << "IN = one two three\nVAR = $$sorted(IN)"
            << "VAR = one three two"
            << ""
            << true;

    QTest::newRow("$$sorted(): bad number of arguments")
            << "VAR = $$sorted(1, 2)"
            << "VAR ="
            << "##:1: sorted(var) requires one argument."
            << true;

    QTest::newRow("$$reverse()")
            << "IN = one two three\nVAR = $$reverse(IN)"
            << "VAR = three two one"
            << ""
            << true;

    QTest::newRow("$$reverse(): bad number of arguments")
            << "VAR = $$reverse(1, 2)"
            << "VAR ="
            << "##:1: reverse(var) requires one argument."
            << true;

    QTest::newRow("$$quote()")
            << "VAR = $$quote(foo bar, 'foo bar')"
            << "VAR = 'foo bar' 'foo bar'"
            << ""
            << true;

    // FIXME: \n and \r go untested, because there is no way to get them into the
    // expected result. And if there was one, this function would be unnecessary.
    // In other news, the behavior of backslash escaping makes no sense.
    QTest::newRow("$$escape_expand()")
            << "VAR = $$escape_expand(foo\\\\ttab\\\\\\\\slash\\\\invalid, verbatim)"
            << "VAR = 'foo\ttab\\\\\\\\slash\\\\invalid' verbatim"
            << ""
            << true;

    QTest::newRow("$$upper()")
            << "VAR = $$upper(kEwL, STuff)"
            << "VAR = KEWL STUFF"
            << ""
            << true;

    QTest::newRow("$$lower()")
            << "VAR = $$lower(kEwL, STuff)"
            << "VAR = kewl stuff"
            << ""
            << true;

    QTest::newRow("$$title()")
            << "VAR = $$title(kEwL, STuff)"
            << "VAR = Kewl Stuff"
            << ""
            << true;

    QTest::newRow("$$re_escape()")
            << "VAR = $$re_escape(one, hey.*you[funny]+people)"
            << "VAR = one hey\\\\.\\\\*you\\\\[funny\\\\]\\\\+people"
            << ""
            << true;

    QTest::newRow("$$val_escape()")
            << "IN = easy \"less easy\" sca$${LITERAL_HASH}ry"
                    " crazy$$escape_expand(\\\\t\\\\r\\\\n)"
                    " $$escape_expand(\\\\t)stuff \\'no\\\"way\\\\here\n"
               "VAR = $$val_escape(IN)"
            << "VAR = easy '\\\"less easy\\\"' sca\\$\\${LITERAL_HASH}ry"
                    " crazy\\$\\$escape_expand(\\\\\\\\t\\\\\\\\r\\\\\\\\n)"
                    " \\$\\$escape_expand(\\\\\\\\t)stuff \\\\\\'no\\\\\\\"way\\\\\\\\here"
            << ""
            << true;

    QTest::newRow("$$val_escape(): bad number of arguments")
            << "VAR = $$val_escape(1, 2)"
            << "VAR ="
            << "##:1: val_escape(var) requires one argument."
            << true;

    QTest::newRow("$$files(): non-recursive")
            << "VAR = $$files(" + qindir + "/files/file*.txt)"
            << "VAR = " + qindir + "/files/file1.txt "
                        + qindir + "/files/file2.txt"
            << ""
            << true;

    QTest::newRow("$$files(): recursive")
            << "VAR = $$files(" + qindir + "/files/file*.txt, true)"
            << "VAR = " + qindir + "/files/file1.txt "
                        + qindir + "/files/file2.txt "
                        + qindir + "/files/dir/file1.txt "
                        + qindir + "/files/dir/file2.txt"
            << ""
            << true;

    QTest::newRow("$$files(): bad number of arguments")
            << "VAR = $$files(1, 2, 3)"
            << "VAR ="
            << "##:1: files(pattern, recursive=false) requires one or two arguments."
            << true;

#if 0
    // FIXME: no emulated input layer
    QTest::newRow("$$prompt()")
            << "VAR = $$prompt(que)"
            << "VAR = whatever"
            << "Project PROMPT: que? "
            << true;
#endif

    QTest::newRow("$$replace()")
            << "IN = foo 'bar baz'\nVAR = $$replace(IN, \\\\bba, hello)"
            << "VAR = foo 'hellor helloz'"
            << ""
            << true;

    QTest::newRow("$$replace(): bad number of arguments")
            << "VAR = $$replace(1, 2) \\\n$$replace(1, 2, 3, 4)"
            << "VAR ="
            << "##:1: replace(var, before, after) requires three arguments.\n"
               "##:2: replace(var, before, after) requires three arguments."
            << true;

    QTest::newRow("$$sort_depends()")
            << "foo.depends = bar baz\n"
               "bar.depends = baz bak duck\n"
               "baz.depends = bak\n"
               "bak.depends = duck\n"
               "VAR = $$sort_depends($$list(baz foo duck bar))"
            << "VAR = foo bar baz duck"
            << ""
            << true;

    QTest::newRow("$$resolve_depends(): basic")
            << "foo.depends = bar baz\n"
               "bar.depends = baz bak duck\n"
               "baz.depends = bak\n"
               "bak.depends = duck\n"
               "VAR = $$resolve_depends($$list(baz foo duck bar))"
            << "VAR = foo bar baz bak duck"
            << ""
            << true;

    QTest::newRow("$$resolve_depends(): prefix and multiple suffixes")
            << "MOD.foo.dep = bar baz\n"
               "MOD.bar.dep = baz bak\n"
               "MOD.bar.priv_dep = duck\n"
               "MOD.baz.dep = bak\n"
               "MOD.bak.dep = duck\n"
               "VAR = $$resolve_depends($$list(baz foo duck bar), MOD., .dep .priv_dep)"
            << "VAR = foo bar baz bak duck"
            << ""
            << true;

    QTest::newRow("$$resolve_depends(): priorities: b first")
            << "MOD.a.depends =\n"
               "MOD.b.depends =\n"
               "MOD.b.priority = 1\n"
               "MOD.c.depends = a b\n"
               "VAR = $$resolve_depends($$list(c), MOD.)"
            << "VAR = c b a"
            << ""
            << true;

    QTest::newRow("$$resolve_depends(): priorities: a first")
            << "MOD.a.depends =\n"
               "MOD.a.priority = 1\n"
               "MOD.b.depends =\n"
               "MOD.b.priority = 0\n"
               "MOD.c.depends = a b\n"
               "VAR = $$resolve_depends($$list(c), MOD.)"
            << "VAR = c a b"
            << ""
            << true;

    QTest::newRow("$$resolve_depends(): priorities: custom suffix")
            << "MOD.a.depends =\n"
               "MOD.a.prrt = 1\n"
               "MOD.b.depends =\n"
               "MOD.b.prrt = 0\n"
               "MOD.c.depends = a b\n"
               "VAR = $$resolve_depends($$list(c), MOD., .depends, .prrt)"
            << "VAR = c a b"
            << ""
            << true;

    QTest::newRow("$$resolve_depends(): bad number of arguments")
            << "VAR = $$resolve_depends(1, 2, 3, 4, 5)"
            << "VAR ="
            << "##:1: resolve_depends(var, [prefix, [suffixes, [prio-suffix]]]) requires one to four arguments."
            << true;

    QTest::newRow("$$enumerate_vars()")
            << "V1 = foo\nV2 = bar\nVAR = $$enumerate_vars()\n"
               "count(VAR, 2, >=):contains(VAR, V1):contains(VAR, V2): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("$$shadowed(): bare")
            << "VAR = $$shadowed(test.txt)"
            << "VAR = " + QMakeEvaluator::quoteValue(ProString(m_outdir + "/test.txt"))
            << ""
            << true;

    QTest::newRow("$$shadowed(): subdir")
            << "VAR = $$shadowed(" + qindir + "/sub/test.txt)"
            << "VAR = " + QMakeEvaluator::quoteValue(ProString(m_outdir + "/sub/test.txt"))
            << ""
            << true;

    QTest::newRow("$$shadowed(): outside source dir")
            << "VAR = $$shadowed(/some/random/path)"
            << "VAR ="
            << ""
            << true;

    QTest::newRow("$$shadowed(): bad number of arguments")
            << "VAR = $$shadowed(1, 2)"
            << "VAR ="
            << "##:1: shadowed(path) requires one argument."
            << true;

    QTest::newRow("$$absolute_path(): relative file")
            << "VAR = $$absolute_path(dir/file.ext)"
            << "VAR = " + qindir + "/dir/file.ext"
            << ""
            << true;

    QTest::newRow("$$absolute_path(): file & path")
            << "VAR = $$absolute_path(dir/file.ext, /root/sub)"
            << "VAR = /root/sub/dir/file.ext"
            << ""
            << true;

    QTest::newRow("$$absolute_path(): absolute file & path")
            << "VAR = $$absolute_path(/root/sub/dir/file.ext, /other)"
            << "VAR = /root/sub/dir/file.ext"
            << ""
            << true;

    QTest::newRow("$$absolute_path(): empty file & path")
            << "VAR = $$absolute_path('', /root/sub)"
            << "VAR = /root/sub"
            << ""
            << true;

    QTest::newRow("$$absolute_path(): bad number of arguments")
            << "VAR = $$absolute_path(1, 2, 3)"
            << "VAR ="
            << "##:1: absolute_path(path[, base]) requires one or two arguments."
            << true;

    QTest::newRow("$$relative_path(): relative file")
            << "VAR = $$relative_path(dir/file.ext)"
            << "VAR = dir/file.ext"
            << ""
            << true;

    QTest::newRow("$$relative_path(): relative file to empty")
            << "VAR = $$relative_path(dir/..)"
            << "VAR = ."
            << ""
            << true;

    QTest::newRow("$$relative_path(): absolute file & path")
            << "VAR = $$relative_path(/root/sub/dir/file.ext, /root/sub)"
            << "VAR = dir/file.ext"
            << ""
            << true;

    QTest::newRow("$$relative_path(): empty file & path")
            << "VAR = $$relative_path('', /root/sub)"
            << "VAR = ."
            << ""
            << true;

    QTest::newRow("$$relative_path(): bad number of arguments")
            << "VAR = $$relative_path(1, 2, 3)"
            << "VAR ="
            << "##:1: relative_path(path[, base]) requires one or two arguments."
            << true;

    QTest::newRow("$$clean_path()")
#ifdef Q_OS_WIN  // This is actually kinda stupid.
            << "VAR = $$clean_path(foo//bar\\\\../baz/)"
#else
            << "VAR = $$clean_path(foo//bar/../baz/)"
#endif
            << "VAR = foo/baz"
            << ""
            << true;

    QTest::newRow("$$clean_path(): bad number of arguments")
            << "VAR = $$clean_path(1, 2)"
            << "VAR ="
            << "##:1: clean_path(path) requires one argument."
            << true;

    QTest::newRow("$$system_path()")
            << "VAR = $$system_path(foo/bar\\\\baz)"
#ifdef Q_OS_WIN
            << "VAR = foo\\\\bar\\\\baz"
#else
            << "VAR = foo/bar/baz"
#endif
            << ""
            << true;

    QTest::newRow("$$system_path(): bad number of arguments")
            << "VAR = $$system_path(1, 2)"
            << "VAR ="
            << "##:1: system_path(path) requires one argument."
            << true;

    // This is is effectively $$system_path() in this test, as we load no specs
    QTest::newRow("$$shell_path()")
            << "VAR = $$shell_path(foo/bar\\\\baz)"
#ifdef Q_OS_WIN
            << "VAR = foo\\\\bar\\\\baz"
#else
            << "VAR = foo/bar/baz"
#endif
            << ""
            << true;

    QTest::newRow("$$shell_path(): bad number of arguments")
            << "VAR = $$shell_path(1, 2)"
            << "VAR ="
            << "##:1: shell_path(path) requires one argument."
            << true;

    // The quoteArgs() test exercises this more thoroughly
    QTest::newRow("$$system_quote()")
            << "IN = \nVAR = $$system_quote(\"some nasty & ugly\\\" path & thing\\\\\")"
#ifdef Q_OS_WIN
            << "VAR = \"\\\"some nasty & ugly\\\\\\\" path ^& thing\\\\\\\\^\\\"\""
#else
            << "VAR = \"'some nasty & ugly\\\" path & thing\\\\'\""
#endif
            << ""
            << true;

    QTest::newRow("$$system_quote(): bad number of arguments")
            << "VAR = $$system_quote(1, 2)"
            << "VAR ="
            << "##:1: system_quote(arg) requires one argument."
            << true;

    // This is is effectively $$system_path() in this test, as we load no specs
    QTest::newRow("$$shell_quote()")
            << "IN = \nVAR = $$shell_quote(\"some nasty & ugly\\\" path & thing\\\\\")"
#ifdef Q_OS_WIN
            << "VAR = \"\\\"some nasty & ugly\\\\\\\" path ^& thing\\\\\\\\^\\\"\""
#else
            << "VAR = \"'some nasty & ugly\\\" path & thing\\\\'\""
#endif
            << ""
            << true;

    QTest::newRow("$$shell_quote(): bad number of arguments")
            << "VAR = $$shell_quote(1, 2)"
            << "VAR ="
            << "##:1: shell_quote(arg) requires one argument."
            << true;

    QTest::newRow("$$getenv()")
            << "VAR = $$getenv(E1)"
            << "VAR = 'env var'"
            << ""
            << true;

    QTest::newRow("$$getenv(): bad number of arguments")
            << "VAR = $$getenv(1, 2)"
            << "VAR ="
            << "##:1: getenv(arg) requires one argument."
            << true;
}

void tst_qmakelib::addTestFunctions(const QString &qindir)
{
    QTest::newRow("defined(): found replace")
            << "defineReplace(func) {}\ndefined(func): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("defined(): found test")
            << "defineTest(func) {}\ndefined(func): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("defined(): not found")
            << "defined(func): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("defined(replace): found")
            << "defineReplace(func) {}\ndefined(func, replace): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("defined(replace): not found")
            << "defineTest(func) {}\ndefined(func, replace): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("defined(test): found")
            << "defineTest(func) {}\ndefined(func, test): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("defined(test): not found")
            << "defineReplace(func) {}\ndefined(func, test): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("defined(var): found")
            << "VAR = 1\ndefined(VAR, var): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("defined(var): not found")
            << "defined(VAR, var): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("defined(): invalid type")
            << "defined(VAR, nope): OK = 1"
            << "OK = UNDEF"
            << "##:1: defined(function, type): unexpected type [nope]."
            << true;

    QTest::newRow("defined(): bad number of arguments")
            << "defined(1, 2, 3): OK = 1"
            << "OK = UNDEF"
            << "##:1: defined(function, [\"test\"|\"replace\"|\"var\"]) requires one or two arguments."
            << true;

    QTest::newRow("export()")
            << "defineTest(func) {\n"
                   "VAR1 += different\nexport(VAR1)\n"
                   "unset(VAR2)\nexport(VAR2): OK = 1\nexport(OK)\n"
                   "VAR3 = new var\nexport(VAR3)\n"
               "}\n"
               "VAR1 = entirely\nVAR2 = set\nfunc()"
            << "OK = 1\nVAR1 = entirely different\nVAR2 =\nVAR3 = new var"
            << ""
            << true;

    QTest::newRow("export(): bad number of arguments")
            << "export(1, 2): OK = 1"
            << "OK = UNDEF"
            << "##:1: export(variable) requires one argument."
            << true;

    QTest::newRow("infile(): found")
            << "infile(" + qindir + "/fromfile/infile.prx, DEFINES): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("infile(): not found")
            << "infile(" + qindir + "/fromfile/infile.prx, INCLUDES): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("infile(plain): found")
            << "infile(" + qindir + "/fromfile/infile.prx, DEFINES, QT_DLL): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("infile(plain): not found")
            << "infile(" + qindir + "/fromfile/infile.prx, DEFINES, NOPE): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("infile(regex): found")
            << "infile(" + qindir + "/fromfile/infile.prx, DEFINES, QT_.*): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("infile(regex): not found")
            << "infile(" + qindir + "/fromfile/infile.prx, DEFINES, NO.*): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    // The early return is debatable, esp. as it's inconsistent with $$fromfile()
    QTest::newRow("infile(): bad file")
            << "infile(" + qindir + "/fromfile/badfile.prx, DEFINES): OK = 1\nOKE = 1"
            << "OK = UNDEF\nOKE = UNDEF"
            << "Project ERROR: fail!"
            << false;

    QTest::newRow("infile(): bad number of arguments")
            << "infile(1): OK = 1\ninfile(1, 2, 3, 4): OK = 1"
            << "OK = UNDEF"
            << "##:1: infile(file, var, [values]) requires two or three arguments.\n"
               "##:2: infile(file, var, [values]) requires two or three arguments."
            << true;

    QTest::newRow("requires()")
            << "requires(true, false, isEmpty(FOO), !isEmpty(BAR), true|false, true:false)"
            << "QMAKE_FAILED_REQUIREMENTS = false !isEmpty(BAR) true:false"
            << ""
            << true;

    // The sparator semantics are *very* questionable.
    // The return value semantics are rather questionable.
    QTest::newRow("eval()")
            << "eval(FOO = one, two$$escape_expand(\\\\n)BAR = blah$$escape_expand(\\\\n)error(fail)$$escape_expand(\\\\n)BAZ = nope)"
            << "FOO = one two\nBAR = blah\nBAZ = UNDEF"
            << "Project ERROR: fail"
            << true;

    QTest::newRow("if(): true")
            << "if(false|true): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("if(): true (space)")
            << "if(false| true): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("if(): true (spaces)")
            << "if( false | true ): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("if(): false")
            << "if(false:true): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("if(): false (space)")
            << "if(false: true): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("if(): false (spaces)")
            << "if( false : true ): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("if(): bad number of arguments")
            << "if(1, 2): OK = 1"
            << "OK = UNDEF"
            << "##:1: if(condition) requires one argument."
            << true;

    QTest::newRow("CONFIG(simple): true")
            << "CONFIG = debug release\nCONFIG(debug): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("CONFIG(simple): false")
            << "CONFIG = debug release\nCONFIG(nope): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("CONFIG(alt): true")
            << "CONFIG = debug release\nCONFIG(release, debug|release): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("CONFIG(alt): false (presence)")
            << "CONFIG = not here\nCONFIG(debug, debug|release): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("CONFIG(alt): false (order)")
            << "CONFIG = debug release\nCONFIG(debug, debug|release): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("CONFIG(): bad number of arguments")
            << "CONFIG(1, 2, 3): OK = 1"
            << "OK = UNDEF"
            << "##:1: CONFIG(config) requires one or two arguments."
            << true;

    QTest::newRow("contains(simple plain): true")
            << "VAR = one two three\ncontains(VAR, two): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("contains(simple plain): false")
            << "VAR = one two three\ncontains(VAR, four): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("contains(simple regex): true")
            << "VAR = one two three\ncontains(VAR, tw.*): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("contains(simple regex): false")
            << "VAR = one two three\ncontains(VAR, fo.*): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("contains(alt plain): true")
            << "VAR = one two three\ncontains(VAR, three, two|three): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("contains(alt plain): false (presence)")
            << "VAR = one four five\ncontains(VAR, three, two|three): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("contains(alt plain): false (order)")
            << "VAR = one two three\ncontains(VAR, two, two|three): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("contains(alt regex): true")
            << "VAR = one two three\ncontains(VAR, th.*, two|three): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("contains(alt regex): false (presence)")
            << "VAR = one four five\ncontains(VAR, th.*, two|three): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("contains(alt regex): false (order)")
            << "VAR = one two three\ncontains(VAR, tw.*, two|three): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("contains(): bad number of arguments")
            << "contains(1): OK = 1\ncontains(1, 2, 3, 4): OK = 1"
            << "OK = UNDEF"
            << "##:1: contains(var, val) requires two or three arguments.\n"
               "##:2: contains(var, val) requires two or three arguments."
            << true;

    QTest::newRow("count(): true")
            << "VAR = one two three\ncount(VAR, 3): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("count(): false")
            << "VAR = one two three\ncount(VAR, 4): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("count(operators): true")
            << "VAR = one two three\n"
               "count(VAR, 3, equals): OKE1 = 1\n"
               "count(VAR, 3, isEqual): OKE2 = 1\n"
               "count(VAR, 3, =): OKE3 = 1\n"
               "count(VAR, 3, ==): OKE4 = 1\n"
               "count(VAR, 2, greaterThan): OKG1 = 1\n"
               "count(VAR, 2, >): OKG2 = 1\n"
               "count(VAR, 2, >=): OKGE = 1\n"
               "count(VAR, 4, lessThan): OKL1 = 1\n"
               "count(VAR, 4, <): OKL2 = 1\n"
               "count(VAR, 4, <=): OKLE = 1\n"
            << "OKE1 = 1\nOKE2 = 1\nOKE3 = 1\nOKE4 = 1\n"
               "OKG1 = 1\nOKG2 = 1\nOKGE = 1\n"
               "OKL1 = 1\nOKL2 = 1\nOKLE = 1"
            << ""
            << true;

    QTest::newRow("count(operators): false")
            << "VAR = one two three\n"
               "count(VAR, 4, equals): OKE1 = 1\n"
               "count(VAR, 4, isEqual): OKE2 = 1\n"
               "count(VAR, 4, =): OKE3 = 1\n"
               "count(VAR, 4, ==): OKE4 = 1\n"
               "count(VAR, 3, greaterThan): OKG1 = 1\n"
               "count(VAR, 3, >): OKG2 = 1\n"
               "count(VAR, 4, >=): OKGE = 1\n"
               "count(VAR, 3, lessThan): OKL1 = 1\n"
               "count(VAR, 3, <): OKL2 = 1\n"
               "count(VAR, 2, <=): OKLE = 1\n"
            << "OKE1 = UNDEF\nOKE2 = UNDEF\nOKE3 = UNDEF\nOKE4 = UNDEF\n"
               "OKG1 = UNDEF\nOKG2 = UNDEF\nOKGE = UNDEF\n"
               "OKL1 = UNDEF\nOKL2 = UNDEF\nOKLE = UNDEF"
            << ""
            << true;

    QTest::newRow("count(): bad operator")
            << "VAR = one two three\ncount(VAR, 2, !!!): OK = 1"
            << "OK = UNDEF"
            << "##:2: Unexpected modifier to count(!!!)."
            << true;

    QTest::newRow("count(): bad number of arguments")
            << "count(1): OK = 1\ncount(1, 2, 3, 4): OK = 1"
            << "OK = UNDEF"
            << "##:1: count(var, count, op=\"equals\") requires two or three arguments.\n"
               "##:2: count(var, count, op=\"equals\") requires two or three arguments."
            << true;

    QTest::newRow("greaterThan(int): true")
            << "VAR = 20\ngreaterThan(VAR, 3): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("greaterThan(int): false")
            << "VAR = 3\ngreaterThan(VAR, 20): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("greaterThan(string): true")
            << "VAR = foo 3\ngreaterThan(VAR, foo 20): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("greaterThan(string): false")
            << "VAR = foo 20\ngreaterThan(VAR, foo 3): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("greaterThan(): bad number of arguments")
            << "greaterThan(1): OK = 1\ngreaterThan(1, 2, 3): OK = 1"
            << "OK = UNDEF"
            << "##:1: greaterThan(variable, value) requires two arguments.\n"
               "##:2: greaterThan(variable, value) requires two arguments."
            << true;

    QTest::newRow("lessThan(int): true")
            << "VAR = 3\nlessThan(VAR, 20): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("lessThan(int): false")
            << "VAR = 20\nlessThan(VAR, 3): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("lessThan(string): true")
            << "VAR = foo 20\nlessThan(VAR, foo 3): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("lessThan(string): false")
            << "VAR = foo 3\nlessThan(VAR, foo 20): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("lessThan(): bad number of arguments")
            << "lessThan(1): OK = 1\nlessThan(1, 2, 3): OK = 1"
            << "OK = UNDEF"
            << "##:1: lessThan(variable, value) requires two arguments.\n"
               "##:2: lessThan(variable, value) requires two arguments."
            << true;

    QTest::newRow("equals(): true")
            << "VAR = foo\nequals(VAR, foo): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("equals(): false")
            << "VAR = foo\nequals(VAR, bar): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("equals(): bad number of arguments")
            << "equals(1): OK = 1\nequals(1, 2, 3): OK = 1"
            << "OK = UNDEF"
            << "##:1: equals(variable, value) requires two arguments.\n"
               "##:2: equals(variable, value) requires two arguments."
            << true;

    // That's just an alias, so don't test much.
    QTest::newRow("isEqual(): true")
            << "VAR = foo\nisEqual(VAR, foo): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("clear(): top-level")
            << "VAR = there\nclear(VAR): OK = 1"
            << "OK = 1\nVAR ="
            << ""
            << true;

    QTest::newRow("clear(): scoped")
            << "defineTest(func) {\n"
                   "clear(VAR): OK = 1\nexport(OK)\n"
                   "equals(VAR, \"\"): OKE = 1\nexport(OKE)\n"
               "}\n"
               "VAR = there\nfunc()"
            << "OK = 1\nOKE = 1"
            << ""
            << true;

    QTest::newRow("clear(): absent")
            << "clear(VAR): OK = 1"
            << "OK = UNDEF\nVAR = UNDEF"
            << ""
            << true;

    QTest::newRow("clear(): bad number of arguments")
            << "clear(1, 2): OK = 1"
            << "OK = UNDEF"
            << "##:1: clear(variable) requires one argument."
            << true;

    QTest::newRow("unset(): top-level")
            << "VAR = there\nunset(VAR): OK = 1"
            << "OK = 1\nVAR = UNDEF"
            << ""
            << true;

    QTest::newRow("unset(): scoped")
            << "defineTest(func) {\n"
                   "unset(VAR): OK = 1\nexport(OK)\n"
                   "!defined(VAR, var): OKE = 1\nexport(OKE)\n"
               "}\n"
               "VAR = there\nfunc()"
            << "OK = 1\nOKE = 1"
            << ""
            << true;

    QTest::newRow("unset(): absent")
            << "unset(VAR): OK = 1"
            << "OK = UNDEF\nVAR = UNDEF"
            << ""
            << true;

    QTest::newRow("unset(): bad number of arguments")
            << "unset(1, 2): OK = 1"
            << "OK = UNDEF"
            << "##:1: unset(variable) requires one argument."
            << true;

    // This function does not follow the established naming pattern.
    QTest::newRow("parseJson()")
            << "jsontext = \\\n"
               "    \"{\"\\\n"
               "    \"    \\\"array\\\" : [\\\"arrayItem1\\\", \\\"arrayItem2\\\", \\\"arrayItem3\\\"],\"\\\n"
               "    \"    \\\"object\\\" : { \\\"key1\\\" : \\\"objectValue1\\\", \\\"key2\\\" : \\\"objectValue2\\\" },\"\\\n"
               "    \"    \\\"string\\\" : \\\"test string\\\",\"\\\n"
               "    \"    \\\"number\\\" : 999,\"\\\n"
               "    \"    \\\"true\\\" : true,\"\\\n"
               "    \"    \\\"false\\\" :false,\"\"\\\n"
               "    \"    \\\"null\\\" : null\"\"\\\n"
               "    \"}\"\n"
               "parseJson(jsontext, json): OK = 1"
            << "OK = 1\n"
               "json._KEYS_ = array false null number object string true\n"
               // array
               "json.array._KEYS_ = 0 1 2\n"
               "json.array.0 = arrayItem1\n"
               "json.array.1 = arrayItem2\n"
               "json.array.2 = arrayItem3\n"
               // object
               "json.object._KEYS_ = key1 key2\n"
               "json.object.key1 = objectValue1\n"
               "json.object.key1 = objectValue1\n"
                // value types
               "json.string = 'test string'\n"
               "json.number = 999\n"
               "json.true = true\n"
               "json.false = false\n"
               "json.null = UNDEF"
            << ""
            << true;

    QTest::newRow("parseJson(): bad input")
            << "jsontext = not good\n"
               "parseJson(jsontext, json): OK = 1"
            << "OK = UNDEF"
            << "##:2: Error parsing JSON at 1:1: illegal value"
            << true;

    QTest::newRow("parseJson(): bad number of arguments")
            << "parseJson(1): OK = 1\nparseJson(1, 2, 3): OK = 1"
            << "OK = UNDEF"
            << "##:1: parseJson(variable, into) requires two arguments.\n"
               "##:2: parseJson(variable, into) requires two arguments."
            << true;

    QTest::newRow("include()")
            << "include(include/inc.pri): OK = 1\nfunc()"
            << "OK = 1\nVAR = val\n.VAR = nope"
            << "Project MESSAGE: say hi!"
            << true;

    QTest::newRow("include(): fail")
            << "include(include/nope.pri): OK = 1"
            << "OK = UNDEF"
            << "Cannot read " + m_indir + "/include/nope.pri: No such file or directory"
            << true;

    QTest::newRow("include(): silent fail")
            << "include(include/nope.pri, , true): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("include(into)")
            << "SUB.MISS = 1\ninclude(include/inc.pri, SUB): OK = 1"
            << "OK = 1\nSUB.VAR = val\nSUB..VAR = UNDEF\nSUB.MISS = UNDEF\n"
               // As a side effect, we test some things that need full project setup
               "SUB.MATCH = 1\nSUB.QMAKESPEC = " + qindir + "/mkspecs/fake-g++"
            << ""
            << true;

    QTest::newRow("include(): bad number of arguments")
            << "include(1, 2, 3, 4): OK = 1"
            << "OK = UNDEF"
            << "##:1: include(file, [into, [silent]]) requires one, two or three arguments."
            << true;

    QTest::newRow("load()")
            << "load(testfeat): OK = 1"
            << "OK = 1\nVAR = foo bar"
            << ""
            << true;

    QTest::newRow("load(): fail")
            << "load(no_such_feature): OK = 1"
            << "OK = UNDEF"
            << "##:1: Cannot find feature no_such_feature"
            << true;

    QTest::newRow("load(): silent fail")
            << "load(no_such_feature, true): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("load(): bad number of arguments")
            << "load(1, 2, 3): OK = 1"
            << "OK = UNDEF"
            << "##:1: load(feature) requires one or two arguments."
            << true;

    QTest::newRow("discard_from()")
            << "HERE = 1\nPLUS = one\n"
               "defineTest(tfunc) {}\ndefineReplace(rfunc) {}\n"
               "include(include/inc.pri)\n"
               "contains(QMAKE_INTERNAL_INCLUDED_FILES, .*/include/inc\\\\.pri): PRE = 1\n"
               "discard_from(include/inc.pri): OK = 1\n"
               "!contains(QMAKE_INTERNAL_INCLUDED_FILES, .*/include/inc\\\\.pri): POST = 1\n"
               "defined(tfunc, test): TDEF = 1\ndefined(rfunc, replace): RDEF = 1\n"
               "defined(func, test): DTDEF = 1\ndefined(func, replace): DRDEF = 1\n"
            << "PRE = 1\nPOST = 1\nOK = 1\nHERE = 1\nPLUS = one\nVAR = UNDEF\n"
               "TDEF = 1\nRDEF = 1\nDTDEF = UNDEF\nDRDEF = UNDEF"
            << ""
            << true;

    QTest::newRow("discard_from(): bad number of arguments")
            << "discard_from(1, 2): OK = 1"
            << "OK = UNDEF"
            << "##:1: discard_from(file) requires one argument."
            << true;

    // We don't test debug() and log(), because they print directly to stderr.

    QTest::newRow("message()")
            << "message('Hello, World!'): OK = 1\nOKE = 1"
            << "OK = 1\nOKE = 1"
            << "Project MESSAGE: Hello, World!"
            << true;

    // Don't test that for warning() and error(), as it's the same code path.
    QTest::newRow("message(): bad number of arguments")
            << "message(1, 2): OK = 1"
            << "OK = UNDEF"
            << "##:1: message(message) requires one argument."
            << true;

    QTest::newRow("warning()")
            << "warning('World, be warned!'): OK = 1\nOKE = 1"
            << "OK = 1\nOKE = 1"
            << "Project WARNING: World, be warned!"
            << true;

    QTest::newRow("error(message)")
            << "error('World, you FAIL!'): OK = 1\nOKE = 1"
            << "OK = UNDEF\nOKE = UNDEF"
            << "Project ERROR: World, you FAIL!"
            << false;

    QTest::newRow("error(empty)")
            << "error(): OK = 1\nOKE = 1"
            << "OK = UNDEF\nOKE = UNDEF"
            << ""
            << false;

    QTest::newRow("if(error())")
            << "if(error(\\'World, you FAIL!\\')): OK = 1\nOKE = 1"
            << "OK = UNDEF\nOKE = UNDEF"
            << "Project ERROR: World, you FAIL!"
            << false;

    QTest::newRow("system()")
            << "system('"
#ifdef Q_OS_WIN
               "cd"
#else
               "pwd"
#endif
               "> '" + QMakeEvaluator::quoteValue(ProString(QDir::toNativeSeparators(
                            m_outdir + "/system_out.txt"))) + "): OK = 1\n"
               "DIR = $$cat(" + QMakeEvaluator::quoteValue(ProString(
                            m_outdir + "/system_out.txt")) + ")"
            << "OK = 1\nDIR = " + QMakeEvaluator::quoteValue(ProString(QDir::toNativeSeparators(m_indir)))
            << ""
            << true;

    QTest::newRow("system(): fail")
#ifdef Q_OS_WIN
            << "system(no_such_cmd 2> NUL): OK = 1"
#else
            << "system(no_such_cmd 2> /dev/null): OK = 1"
#endif
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("system(): bad number of arguments")
            << "system(1, 2): OK = 1"
            << "OK = UNDEF"
            << "##:1: system(exec) requires one argument."
            << true;

    QTest::newRow("isEmpty(): true (empty)")
            << "VAR =\nisEmpty(VAR): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("isEmpty(): true (undef)")
            << "isEmpty(VAR): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("isEmpty(): false")
            << "VAR = val\nisEmpty(VAR): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("isEmpty(): bad number of arguments")
            << "isEmpty(1, 2): OK = 1"
            << "OK = UNDEF"
            << "##:1: isEmpty(var) requires one argument."
            << true;

    QTest::newRow("exists(plain): true")
            << "exists(files/file1.txt): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("exists(plain): false")
            << "exists(files/not_there.txt): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("exists(wildcard): true")
            << "exists(files/fil*.txt): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("exists(wildcard): false")
            << "exists(files/not_th*.txt): OK = 1"
            << "OK = UNDEF"
            << ""
            << true;

    QTest::newRow("exists(): bad number of arguments")
            << "exists(1, 2): OK = 1"
            << "OK = UNDEF"
            << "##:1: exists(file) requires one argument."
            << true;

    QString wpath = QMakeEvaluator::quoteValue(ProString(m_outdir + "/outdir/written.txt"));
    QTest::newRow("write_file(): create")
            << "VAR = 'this is text' 'yet again'\n"
               "write_file(" + wpath + ", VAR): OK = 1\n"
               "OUT = $$cat(" + wpath + ", lines)"
            << "OK = 1\nOUT = 'this is text' 'yet again'"
            << ""
            << true;

    QTest::newRow("write_file(): truncate")
            << "VAR = 'other content'\n"
               "write_file(" + wpath + ", VAR): OK = 1\n"
               "OUT = $$cat(" + wpath + ", lines)"
            << "OK = 1\nOUT = 'other content'"
            << ""
            << true;

    // FIXME: This also tests that 'exe' is accepted, but does not test whether it actually works.
    QTest::newRow("write_file(): append")
            << "VAR = 'one more line'\n"
               "write_file(" + wpath + ", VAR, append exe): OK = 1\n"
               "OUT = $$cat(" + wpath + ", lines)"
            << "OK = 1\nOUT = 'other content' 'one more line'"
            << ""
            << true;

    QString vpath = QMakeEvaluator::quoteValue(ProString(m_outdir));
    QTest::newRow("write_file(): fail")
            << "write_file(" + vpath + "): OK = 1"
            << "OK = UNDEF"
#ifdef Q_OS_WIN
            << "##:1: Cannot write file " + QDir::toNativeSeparators(m_outdir) + ": Access is denied."
#else
            << "##:1: Cannot write file " + m_outdir + ": Is a directory"
#endif
            << true;

    QTest::newRow("write_file(): bad number of arguments")
            << "write_file(1, 2, 3, 4): OK = 1"
            << "OK = UNDEF"
            << "##:1: write_file(name, [content var, [append] [exe]]) requires one to three arguments."
            << true;

    QTest::newRow("write_file(): invalid flag")
            << "write_file(file, VAR, fail): OK = 1"
            << "OK = UNDEF"
            << "##:1: write_file(): invalid flag fail."
            << true;

    // FIXME: This doesn't test whether it actually works.
    QTest::newRow("touch()")
            << "touch(" + wpath + ", files/other.txt): OK = 1"
            << "OK = 1"
            << ""
            << true;

    QTest::newRow("touch(): missing target")
            << "touch(/does/not/exist, files/other.txt): OK = 1"
            << "OK = UNDEF"
#ifdef Q_OS_WIN
            << "##:1: Cannot open /does/not/exist: The system cannot find the path specified."
#else
            << "##:1: Cannot touch /does/not/exist: No such file or directory."
#endif
            << true;

    QTest::newRow("touch(): missing reference")
            << "touch(" + wpath + ", /does/not/exist): OK = 1"
            << "OK = UNDEF"
#ifdef Q_OS_WIN
            << "##:1: Cannot open reference file /does/not/exist: The system cannot find the path specified."
#else
            << "##:1: Cannot stat() reference file /does/not/exist: No such file or directory."
#endif
            << true;

    QTest::newRow("touch(): bad number of arguments")
            << "touch(1): OK = 1\ntouch(1, 2, 3): OK = 1"
            << "OK = UNDEF"
            << "##:1: touch(file, reffile) requires two arguments.\n"
               "##:2: touch(file, reffile) requires two arguments."
            << true;

    QString apath = QMakeEvaluator::quoteValue(ProString(m_outdir + "/a/path"));
    QTest::newRow("mkpath()")
            << "mkpath(" + apath + "): OK = 1\n"
               "exists(" + apath + "): OKE = 1"
            << "OK = 1\nOKE = 1"
            << ""
            << true;

    QString bpath = QMakeEvaluator::quoteValue(ProString(m_outdir + "/fail_me"));
    QTest::newRow("mkpath(): fail")
            << "write_file(" + bpath + ")|error(FAIL)\n"
               "mkpath(" + bpath + "): OK = 1"
            << "OK = UNDEF"
            << "##:2: Cannot create directory " + QDir::toNativeSeparators(m_outdir + "/fail_me") + '.'
            << true;

    QTest::newRow("mkpath(): bad number of arguments")
            << "mkpath(1, 2): OK = 1"
            << "OK = UNDEF"
            << "##:1: mkpath(file) requires one argument."
            << true;

#if 0
    // FIXME ... insanity lies ahead
    QTest::newRow("cache()")
            << ""
            << ""
            << ""
            << true;
#endif
}

void tst_qmakelib::proEval_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<QString>("msgs");
    QTest::addColumn<bool>("ok");

    QTest::newRow("empty")
            << ""
            << "VAR = UNDEF"
            << ""
            << true;

    addAssignments();
    addExpansions(); // Variable, etc. expansions on RHS
    addControlStructs(); // Conditions, loops, custom functions

    QString qindir = QMakeEvaluator::quoteValue(ProString(m_indir));
    addReplaceFunctions(qindir); // Built-in replace functions
    addTestFunctions(qindir); // Built-in test functions

    // Some compound tests that verify compatibility with odd Qt 4 edge cases

    QTest::newRow("empty (leading)")
            << "defineTest(myMsg) { message(\"$$1\") }\n"
               "XMPL = /this/is/a/test\n"
               "message(split: $$split(XMPL, /))\n"
               "message(split joined:$$split(XMPL, /))\n"
               "message(\"split quoted: $$split(XMPL, /)\")\n"
               "myMsg(my split: $$split(XMPL, /) :post)\n"
               "myMsg(my split joined:$$split(XMPL, /):post)\n"
               "myMsg(\"my split quoted: $$split(XMPL, /) post\")\n"
               "OUT = word $$split(XMPL, /) done\n"
               "message(\"assign split separate: $$OUT\")\n"
               "OUT = word:$$split(XMPL, /):done\n"
               "message(\"assign split joined: $$OUT\")\n"
               "OUT = \"word $$split(XMPL, /) done\"\n"
               "message(\"assign split quoted: $$OUT\")\n"
            << ""
            << "Project MESSAGE: split: this is a test\n"
               "Project MESSAGE: split joined: this is a test\n"
               "Project MESSAGE: split quoted:  this is a test\n"
               "Project MESSAGE: my split: this is a test :post\n"
               "Project MESSAGE: my split joined: this is a test:post\n"
               "Project MESSAGE: my split quoted:  this is a test post\n"
               "Project MESSAGE: assign split separate: word this is a test done\n"
               "Project MESSAGE: assign split joined: word: this is a test:done\n"
               "Project MESSAGE: assign split quoted: word  this is a test done"
            << true;

    QTest::newRow("empty (multiple)")
            << "defineTest(myMsg) { message(\"$$1\") }\n"
               "XMPL = //this///is/a/////test\n"
               "message(split: $$split(XMPL, /) :post)\n"
               "message(split joined:$$split(XMPL, /):post)\n"
               "message(\"split quoted: $$split(XMPL, /) post\")\n"
               "myMsg(my split: $$split(XMPL, /) :post)\n"
               "myMsg(my split joined:$$split(XMPL, /):post)\n"
               "myMsg(\"my split quoted: $$split(XMPL, /) post\")\n"
               "OUT = word $$split(XMPL, /) done\n"
               "message(\"assign split separate: $$OUT\")\n"
               "OUT = word:$$split(XMPL, /):done\n"
               "message(\"assign split joined: $$OUT\")\n"
               "OUT = \"word $$split(XMPL, /) done\"\n"
               "message(\"assign split quoted: $$OUT\")\n"
            << ""
            << "Project MESSAGE: split:  this   is a     test :post\n"
               "Project MESSAGE: split joined:  this   is a     test:post\n"
               "Project MESSAGE: split quoted:   this   is a     test post\n"
               "Project MESSAGE: my split:  this   is a     test :post\n"
               "Project MESSAGE: my split joined:  this   is a     test:post\n"
               "Project MESSAGE: my split quoted:   this   is a     test post\n"
               "Project MESSAGE: assign split separate: word this is a test done\n"
               "Project MESSAGE: assign split joined: word: this is a test:done\n"
               "Project MESSAGE: assign split quoted: word   this   is a     test done"
            << true;

    // Raw data leak with empty file name. Verify with Valgrind or asan.
    QTest::newRow("QTBUG-54550")
            << "FULL = /there/is\n"
               "VAR = $$absolute_path(, $$FULL/nothing/here/really)"
            << "VAR = /there/is/nothing/here/really"
            << ""
            << true;
}

static QString formatValue(const ProStringList &vals)
{
    QString ret;

    foreach (const ProString &str, vals) {
        ret += QLatin1Char(' ');
        ret += QMakeEvaluator::quoteValue(str);
    }
    return ret;
}

static void skipNoise(const ushort *&tokPtr)
{
    forever {
        ushort tok = *tokPtr;
        if (tok != TokLine)
            break;
        tokPtr += 2;
    }
}

static bool compareState(QMakeEvaluator *eval, ProFile *out)
{
    bool ret = true;
    const ushort *tokPtr = out->tokPtr();
    forever {
        skipNoise(tokPtr);
        ushort tok = *tokPtr++;
        if (!tok)
            break;
        if (tok != TokHashLiteral) {
            qWarning("Expected output is malformed: not variable%s",
                     qPrintable(QMakeParser::formatProBlock(out->items())));
            return false;
        }
        const ProKey &var = out->getHashStr(tokPtr);
        tok = *tokPtr++;
        if (tok != TokAssign) {
            qWarning("Expected output is malformed: not assignment%s",
                     qPrintable(QMakeParser::formatProBlock(out->items())));
            return false;
        }
        ProStringList value;
        value.reserve(*tokPtr++);
        forever {
            skipNoise(tokPtr);
            tok = *tokPtr++;
            if (tok == TokValueTerminator)
                break;
            if (tok != (TokLiteral | TokNewStr)) {
                qWarning("Expected output is malformed: not literal%s",
                         qPrintable(QMakeParser::formatProBlock(out->items())));
                return false;
            }
            value << out->getStr(tokPtr);
        }
        ProValueMap::Iterator it;
        ProValueMap *vmap = eval->findValues(var, &it);
        if (value.length() == 1 && value.at(0) == "UNDEF") {
            if (vmap) {
                qWarning("Value of %s is incorrect.\n  Actual:%s\nExpected: <UNDEFINED>",
                         qPrintable(var.toQString()),
                         qPrintable(formatValue(*it)));
                ret = false;
            }
        } else {
            if (!vmap) {
                qWarning("Value of %s is incorrect.\n  Actual: <UNDEFINED>\nExpected:%s",
                         qPrintable(var.toQString()),
                         qPrintable(formatValue(value)));
                ret = false;
            } else if (*it != value) {
                qWarning("Value of %s is incorrect.\n  Actual:%s\nExpected:%s",
                         qPrintable(var.toQString()),
                         qPrintable(formatValue(*it)), qPrintable(formatValue(value)));
                ret = false;
            }
        }
    }
    return ret;
}

void tst_qmakelib::proEval()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(QString, msgs);
    QFETCH(bool, ok);

    QString infile = m_indir + "/test.pro";
    bool verified = true;
    QMakeTestHandler handler;
    handler.setExpectedMessages(msgs.replace("##:", infile + ':').split('\n', QString::SkipEmptyParts));
    QMakeVfs vfs;
    ProFileCache cache;
    QMakeParser parser(&cache, &vfs, &handler);
    QMakeGlobals globals;
    globals.do_cache = false;
    globals.xqmakespec = "fake-g++";
    globals.environment = m_env;
    globals.setProperties(m_prop);
    globals.setDirectories(m_indir, m_outdir);
    ProFile *outPro = parser.parsedProBlock(QStringRef(&out), "out", 1, QMakeParser::FullGrammar);
    if (!outPro->isOk()) {
        qWarning("Expected output is malformed");
        verified = false;
    }
    ProFile *pro = parser.parsedProBlock(QStringRef(&in), infile, 1, QMakeParser::FullGrammar);
    QMakeEvaluator visitor(&globals, &parser, &vfs, &handler);
    visitor.setOutputDir(m_outdir);
#ifdef Q_OS_WIN
    visitor.m_dirSep = ProString("\\");
#else
    visitor.m_dirSep = ProString("/");
#endif
    QMakeEvaluator::VisitReturn ret
            = visitor.visitProFile(pro, QMakeHandler::EvalAuxFile, QMakeEvaluator::LoadProOnly);
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
    if ((ret == QMakeEvaluator::ReturnTrue) != ok) {
        static const char * const lbl[] = { "failure", "success" };
        qWarning("Expected %s, got %s", lbl[int(ok)], lbl[1 - int(ok)]);
        verified = false;
    }
    if (!compareState(&visitor, outPro))
        verified = false;
    pro->deref();
    outPro->deref();
    QVERIFY(verified);
}
