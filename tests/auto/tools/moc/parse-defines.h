/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef PARSE_DEFINES_H
#define PARSE_DEFINES_H

#include <qobject.h>

// this is intentionally ugly to test moc's preprocessing capabilities
#define PD_NAMESPACE PD
#define PD_BEGIN_NAMESPACE namespace PD_NAMESPACE {
#define PD_END_NAMESPACE }
#define PD_VOIDFUNCTION() voidFunction()
#define PD_CLASSNAME ParseDefine

#define PD_STRINGIFY(a) #a
#define PD_XSTRINGIFY(a) PD_STRINGIFY(a)
#define PD_SCOPED_STRING(a, b) PD_STRINGIFY(a) "::" PD_STRINGIFY(b)
#define PD_DEFINE1(a,b) a##b
#define PD_DEFINE2(a,b) a comb##b
#define PD_DEFINE3(a,b) a b##ined3()
#define PD_COMBINE(a,b) a b
#define PD_TEST_IDENTIFIER_ARG(if, while) if while

#define QString() error_type

#define PD_CLASSINFO Q_CLASSINFO

#if defined(Q_COMPILER_VARIADIC_MACROS) || defined (Q_MOC_RUN)
#define PD_VARARG(x, ...) x(__VA_ARGS__)

#if defined(Q_CC_GNU) || defined(Q_MOC_RUN)
//GCC extension for variadic macros
#define PD_VARARGEXT(x, y...) x(y)
#else
#define PD_VARARGEXT(x, ...) x(__VA_ARGS__)
#endif

#endif

#define PD_ADD_SUFFIX(x)  PD_DEFINE1(x,_SUFFIX)
#define PD_DEFINE_ITSELF PD_ADD_SUFFIX(PD_DEFINE_ITSELF)

#ifndef Q_MOC_RUN
// macro defined on the command line (in tst_moc.pro)
#define DEFINE_CMDLINE_EMPTY
#define DEFINE_CMDLINE_SIGNAL void cmdlineSignal(const QMap<int, int> &i)
#endif

#define HASH_SIGN #

PD_BEGIN_NAMESPACE

class DEFINE_CMDLINE_EMPTY PD_CLASSNAME DEFINE_CMDLINE_EMPTY
    : public DEFINE_CMDLINE_EMPTY QObject DEFINE_CMDLINE_EMPTY
{
    Q_OBJECT
    Q_CLASSINFO("TestString", PD_STRINGIFY(PD_CLASSNAME))
    Q_CLASSINFO("TestString2", PD_XSTRINGIFY(PD_CLASSNAME))
    PD_CLASSINFO("TestString3", "TestValue")
public:
    PD_CLASSNAME() {}

public slots:
    void PD_VOIDFUNCTION() {}

    QString stringMethod() { return QString::fromLatin1(""); }

    void PD_DEFINE1(comb, ined1()) {}
    PD_DEFINE2(void, ined2()) {}
    PD_DEFINE3(void, comb) {}
    PD_COMBINE(void combined4(int, int), {})

    PD_COMBINE(void combined5() {, })

    PD_TEST_IDENTIFIER_ARG(void, combined6()) {}

#if defined(Q_COMPILER_VARIADIC_MACROS) || defined (Q_MOC_RUN)
    PD_VARARG(void vararg1) {}
    PD_VARARG(void vararg2, int) {}
    PD_VARARG(void vararg3, int, int) {}

    PD_VARARGEXT(void vararg4) {}
    PD_VARARGEXT(void vararg5, int) {}
    PD_VARARGEXT(void vararg6, int, int) {}
#else
    void vararg1() {}
    void vararg2(int) {}
    void vararg3(int,int) {}
    void vararg4() {}
    void vararg5(int) {}
    void vararg6(int,int) {}
#endif

#define OUTERFUNCTION(x) x
#define INNERFUNCTION(x) OUTERFUNCTION(x)
#define INNER INNERFUNCTION

    void INNERFUNCTION(INNERFUNCTION)(int) {}
    void OUTERFUNCTION(INNERFUNCTION)(inner_expanded(int)) {}
    void expanded_method OUTERFUNCTION(INNER)((int)) {}

#undef INNERFUNCTION

#define cond1() 0x1
#define cond2() 0x2

#if !(cond1() & cond2())
    void conditionSlot() {}
#endif

    void PD_DEFINE_ITSELF(int) {}

signals:
    DEFINE_CMDLINE_SIGNAL;

};

#undef QString

PD_END_NAMESPACE

#endif
