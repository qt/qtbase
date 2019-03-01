/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef CXXATTRIBUTE_H
#define CXXATTRIBUTE_H

#include <QtCore/QObject>

class CppAttribute : public QObject
{
    Q_OBJECT
signals:
    [[deprecated]] void deprecatedSignal();

public slots:
    [[deprecated]] void deprecatedSlot() {}
    [[deprecated]] [[tst_moc::maybe_unused]] int deprecatedSlot2() { return 42; }
    [[deprecated("reason")]] void deprecatedReason() {}
    [[deprecated("reason[")]] void deprecatedReasonWithLBRACK() {}
    [[deprecated("reason[[")]] void deprecatedReasonWith2LBRACK() {}
    [[deprecated("reason]")]] void deprecatedReasonWithRBRACK() {}
    [[deprecated("reason]]")]] void deprecatedReasonWith2RBRACK() {}
    void slotWithArguments([[tst_moc::maybe_unused]] int) {}
#if !defined(_MSC_VER) || _MSC_VER >= 1912
    // On MSVC it causes:
    // moc_cxx-attributes.cpp(133): fatal error C1001: An internal error has occurred in the compiler.
    Q_INVOKABLE [[tst_moc::noreturn]] void noreturnSlot() { throw "unused"; }
    [[tst_moc::noreturn]] Q_SCRIPTABLE void noreturnSlot2() { throw "unused"; }
    [[deprecated]] int returnInt() { return 0; }
    Q_SLOT [[tst_moc::noreturn]] [[deprecated]] void noreturnDeprecatedSlot() { throw "unused"; }
    Q_INVOKABLE void noreturnSlot3() [[tst_moc::noreturn]] { throw "unused"; }
#endif
};

#ifdef Q_MOC_RUN
#  define TEST_COMPILER_DEPRECATION [[deprecated]]
#  define TEST_COMPILER_DEPRECATION_X(x) [[deprecated(x)]]
#else
#  define TEST_COMPILER_DEPRECATION Q_DECL_ENUMERATOR_DEPRECATED
#  define TEST_COMPILER_DEPRECATION_X(x) Q_DECL_ENUMERATOR_DEPRECATED_X(x)
#endif

namespace TestQNamespaceDeprecated {
    Q_NAMESPACE
    enum class TestEnum1 {
        Key1 = 11,
        Key2 TEST_COMPILER_DEPRECATION,
        Key3 TEST_COMPILER_DEPRECATION_X("reason"),
        Key4 TEST_COMPILER_DEPRECATION_X("reason["),
        Key5 TEST_COMPILER_DEPRECATION_X("reason[["),
        Key6 TEST_COMPILER_DEPRECATION_X("reason]"),
        Key7 TEST_COMPILER_DEPRECATION_X("reason]]"),
    };
    Q_ENUM_NS(TestEnum1)

    // try to dizzy moc by adding a struct in between
    struct TestGadget {
        Q_GADGET
    public:
        enum class TestGEnum1 {
            Key1 = 13,
            Key2 TEST_COMPILER_DEPRECATION,
            Key3 TEST_COMPILER_DEPRECATION_X("reason")
        };
        Q_ENUM(TestGEnum1)
    };

    enum class TestFlag1 {
        None = 0,
        Flag1 = 1,
        Flag2 TEST_COMPILER_DEPRECATION = 2,
        Flag3 TEST_COMPILER_DEPRECATION_X("reason") = 3,
        Any = Flag1 | Flag2 | Flag3
    };
    Q_FLAG_NS(TestFlag1)
}

#endif // CXXATTRIBUTE_H
