// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CXXATTRIBUTE_H
#define CXXATTRIBUTE_H

#include <QtCore/QObject>

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wunknown-attributes")
QT_WARNING_DISABLE_GCC("-Wattributes")

class [[deprecated]] CppAttribute : public QObject
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

QT_WARNING_POP

#ifdef Q_MOC_RUN
#  define TEST_COMPILER_DEPRECATION [[deprecated]]
#  define TEST_COMPILER_DEPRECATION_X(x) [[deprecated(x)]]
#else
#  define TEST_COMPILER_DEPRECATION Q_DECL_ENUMERATOR_DEPRECATED
#  define TEST_COMPILER_DEPRECATION_X(x) Q_DECL_ENUMERATOR_DEPRECATED_X(x)
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED

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

QT_WARNING_POP

#endif // CXXATTRIBUTE_H
