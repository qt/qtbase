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

class tst_QFlags: public QObject
{
    Q_OBJECT
private slots:
    void boolCasts() const;
    void operators() const;
    void testFlag() const;
    void testFlagZeroFlag() const;
    void testFlagMultiBits() const;
    void constExpr();
    void signedness();
    void classEnum();
    void initializerLists();
    void testSetFlags();
    void adl();
};

void tst_QFlags::boolCasts() const
{
    // This tests that the operator overloading is sufficient so that common
    // idioms involving flags -> bool casts work as expected:

    const Qt::Alignment nonNull = Qt::AlignCenter;
    const Qt::Alignment null = {};

    // basic premiss:
    QVERIFY(bool(nonNull));
    QVERIFY(!bool(null));

    // The rest is just checking that stuff compiles:

    // QVERIFY should compile:
    QVERIFY(nonNull);
    QVERIFY(!null);

    // ifs should compile:
    if (null) QFAIL("Can't contextually convert QFlags to bool!");
    if (!nonNull) QFAIL("Missing operator! on QFlags (shouldn't be necessary).");

    // ternary should compile:
    QVERIFY(nonNull ? true : false);
    QVERIFY(!null ? true : false);

    // logical operators should compile:
    QVERIFY(nonNull && true);
    QVERIFY(nonNull || false);
    QVERIFY(!null && true);
    QVERIFY(!null || false);

    // ... in both directions:
    QVERIFY(true && nonNull);
    QVERIFY(false || nonNull);
    QVERIFY(true && !null);
    QVERIFY(false || !null);

    // ... and mixed:
    QVERIFY(null || nonNull);
    QVERIFY(!(null && nonNull));
}

void tst_QFlags::operators() const
{
#define CHECK(op, LHS, RHS, RES) \
    do { \
        using LFlags = QFlags<decltype(LHS)>; \
        using RFlags = QFlags<decltype(RHS)>; \
        QCOMPARE((LHS op RHS), (RES)); \
        QCOMPARE((LFlags(LHS) op RHS), (RES)); \
        QCOMPARE((LHS op RFlags(RHS)), (RES)); \
        QCOMPARE((LFlags(LHS) op RFlags(RHS)), (RES)); \
        QCOMPARE((LFlags(LHS) op ## = RHS), (RES)); \
        QCOMPARE((LFlags(LHS) op ## = RFlags(RHS)), (RES)); \
    } while (false)

    CHECK(|, Qt::AlignHCenter, Qt::AlignVCenter, Qt::AlignCenter);
    CHECK(|, Qt::AlignHCenter, Qt::AlignHCenter, Qt::AlignHCenter);
    CHECK(&, Qt::AlignHCenter, Qt::AlignVCenter, Qt::Alignment());
    CHECK(&, Qt::AlignHCenter, Qt::AlignHCenter, Qt::AlignHCenter);
    CHECK(^, Qt::AlignHCenter, Qt::AlignVCenter, Qt::AlignCenter);
    CHECK(^, Qt::AlignHCenter, Qt::AlignHCenter, Qt::Alignment());

#undef CHECK
}

void tst_QFlags::testFlag() const
{
    Qt::MouseButtons btn = Qt::LeftButton | Qt::RightButton;

    QVERIFY(btn.testFlag(Qt::LeftButton));
    QVERIFY(!btn.testFlag(Qt::MiddleButton));

    btn = { };
    QVERIFY(!btn.testFlag(Qt::LeftButton));
}

void tst_QFlags::testFlagZeroFlag() const
{
    {
        Qt::MouseButtons btn = Qt::LeftButton | Qt::RightButton;
        /* Qt::NoButton has the value 0. */

        QVERIFY(!btn.testFlag(Qt::NoButton));
    }

    {
        /* A zero enum set should test true with zero. */
        QVERIFY(Qt::MouseButtons().testFlag(Qt::NoButton));
    }

    {
        Qt::MouseButtons btn = Qt::NoButton;
        QVERIFY(btn.testFlag(Qt::NoButton));
    }
}

void tst_QFlags::testFlagMultiBits() const
{
    /* Qt::Window is 0x00000001
     * Qt::Dialog is 0x00000002 | Window
     */
    {
        const Qt::WindowFlags onlyWindow(Qt::Window);
        QVERIFY(!onlyWindow.testFlag(Qt::Dialog));
    }

    {
        const Qt::WindowFlags hasDialog(Qt::Dialog);
        QVERIFY(hasDialog.testFlag(Qt::Dialog));
    }
}

template <unsigned int N, typename T> bool verifyConstExpr(T n) { return n == N; }

Q_DECL_RELAXED_CONSTEXPR Qt::MouseButtons testRelaxedConstExpr()
{
    Qt::MouseButtons value;
    value = Qt::LeftButton | Qt::RightButton;
    value |= Qt::MiddleButton;
    value &= ~Qt::LeftButton;
    value ^= Qt::RightButton;
    return value;
}

void tst_QFlags::constExpr()
{
#ifdef Q_COMPILER_CONSTEXPR
    Qt::MouseButtons btn = Qt::LeftButton | Qt::RightButton;
    switch (btn) {
        case Qt::LeftButton: QVERIFY(false); break;
        case Qt::RightButton: QVERIFY(false); break;
        case int(Qt::LeftButton | Qt::RightButton): QVERIFY(true); break;
        default: QVERIFY(false);
    }

    QVERIFY(verifyConstExpr<uint((Qt::LeftButton | Qt::RightButton) & Qt::LeftButton)>(Qt::LeftButton));
    QVERIFY(verifyConstExpr<uint((Qt::LeftButton | Qt::RightButton) & Qt::MiddleButton)>(0));
    QVERIFY(verifyConstExpr<uint((Qt::LeftButton | Qt::RightButton) | Qt::MiddleButton)>(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton));
    QVERIFY(verifyConstExpr<uint(~(Qt::LeftButton | Qt::RightButton))>(~(Qt::LeftButton | Qt::RightButton)));
    QVERIFY(verifyConstExpr<uint(Qt::MouseButtons(Qt::LeftButton) ^ Qt::RightButton)>(Qt::LeftButton ^ Qt::RightButton));
    QVERIFY(verifyConstExpr<uint(Qt::MouseButtons(0))>(0));
    QVERIFY(verifyConstExpr<uint(Qt::MouseButtons(Qt::RightButton) & 0xff)>(Qt::RightButton));
    QVERIFY(verifyConstExpr<uint(Qt::MouseButtons(Qt::RightButton) | 0xff)>(0xff));

    QVERIFY(!verifyConstExpr<Qt::RightButton>(~Qt::MouseButtons(Qt::LeftButton)));

#if defined(__cpp_constexpr) &&  __cpp_constexpr-0 >= 201304
    QVERIFY(verifyConstExpr<uint(testRelaxedConstExpr())>(Qt::MiddleButton));
#endif
#endif
}

void tst_QFlags::signedness()
{
    // these are all 'true' on GCC, but since the std says the
    // underlying type is implementation-defined, we need to allow for
    // a different signedness, so we only check that the relative
    // signedness of the types matches:
    Q_STATIC_ASSERT((std::is_unsigned<typename std::underlying_type<Qt::MouseButton>::type>::value ==
                     std::is_unsigned<Qt::MouseButtons::Int>::value));

    Q_STATIC_ASSERT((std::is_signed<typename std::underlying_type<Qt::AlignmentFlag>::type>::value ==
                     std::is_signed<Qt::Alignment::Int>::value));
}

enum class MyStrictEnum { StrictZero, StrictOne, StrictTwo, StrictFour=4 };
Q_DECLARE_FLAGS( MyStrictFlags, MyStrictEnum )
Q_DECLARE_OPERATORS_FOR_FLAGS( MyStrictFlags )

enum class MyStrictNoOpEnum { StrictZero, StrictOne, StrictTwo, StrictFour=4 };
Q_DECLARE_FLAGS( MyStrictNoOpFlags, MyStrictNoOpEnum )

Q_STATIC_ASSERT( !QTypeInfo<MyStrictFlags>::isComplex );
Q_STATIC_ASSERT( !QTypeInfo<MyStrictFlags>::isStatic );
Q_STATIC_ASSERT( !QTypeInfo<MyStrictFlags>::isLarge );
Q_STATIC_ASSERT( !QTypeInfo<MyStrictFlags>::isPointer );

void tst_QFlags::classEnum()
{
    // The main aim of the test is making sure it compiles
    // The QCOMPARE are there as an extra
    MyStrictEnum e1 = MyStrictEnum::StrictOne;
    MyStrictEnum e2 = MyStrictEnum::StrictTwo;

    MyStrictFlags f1(MyStrictEnum::StrictOne);
    QCOMPARE(f1, 1);

    MyStrictFlags f2(e2);
    QCOMPARE(f2, 2);

    MyStrictFlags f0;
    QCOMPARE(f0, 0);

    MyStrictFlags f3(e2 | e1);
    QCOMPARE(f3, 3);

    QVERIFY(f3.testFlag(MyStrictEnum::StrictOne));
    QVERIFY(!f1.testFlag(MyStrictEnum::StrictTwo));

    QVERIFY(!f0);

    QCOMPARE(f3 & int(1), 1);
    QCOMPARE(f3 & uint(1), 1);
    QCOMPARE(f3 & MyStrictEnum::StrictOne, 1);

    MyStrictFlags aux;
    aux = f3;
    aux &= int(1);
    QCOMPARE(aux, 1);

    aux = f3;
    aux &= uint(1);
    QCOMPARE(aux, 1);

    aux = f3;
    aux &= MyStrictEnum::StrictOne;
    QCOMPARE(aux, 1);

    aux = f3;
    aux &= f1;
    QCOMPARE(aux, 1);

    aux = f3 ^ f3;
    QCOMPARE(aux, 0);

    aux = f3 ^ f1;
    QCOMPARE(aux, 2);

    aux = f3 ^ f0;
    QCOMPARE(aux, 3);

    aux = f3 ^ MyStrictEnum::StrictOne;
    QCOMPARE(aux, 2);

    aux = f3 ^ MyStrictEnum::StrictZero;
    QCOMPARE(aux, 3);

    aux = f3;
    aux ^= f3;
    QCOMPARE(aux, 0);

    aux = f3;
    aux ^= f1;
    QCOMPARE(aux, 2);

    aux = f3;
    aux ^= f0;
    QCOMPARE(aux, 3);

    aux = f3;
    aux ^= MyStrictEnum::StrictOne;
    QCOMPARE(aux, 2);

    aux = f3;
    aux ^= MyStrictEnum::StrictZero;
    QCOMPARE(aux, 3);

    aux = f1 | f2;
    QCOMPARE(aux, 3);

    aux = MyStrictEnum::StrictOne | MyStrictEnum::StrictTwo;
    QCOMPARE(aux, 3);

    aux = f1;
    aux |= f2;
    QCOMPARE(aux, 3);

    aux = MyStrictEnum::StrictOne;
    aux |= MyStrictEnum::StrictTwo;
    QCOMPARE(aux, 3);

    aux = ~f1;
    QCOMPARE(aux, -2);

    // Just to make sure it compiles
    if (false)
        qDebug() << f3;
}

void tst_QFlags::initializerLists()
{
    Qt::MouseButtons bts = { Qt::LeftButton, Qt::RightButton };
    QVERIFY(bts.testFlag(Qt::LeftButton));
    QVERIFY(bts.testFlag(Qt::RightButton));
    QVERIFY(!bts.testFlag(Qt::MiddleButton));

    MyStrictNoOpFlags flags = { MyStrictNoOpEnum::StrictOne, MyStrictNoOpEnum::StrictFour };
    QVERIFY(flags.testFlag(MyStrictNoOpEnum::StrictOne));
    QVERIFY(flags.testFlag(MyStrictNoOpEnum::StrictFour));
    QVERIFY(!flags.testFlag(MyStrictNoOpEnum::StrictTwo));
}

void tst_QFlags::testSetFlags()
{
    Qt::MouseButtons btn = Qt::NoButton;

    btn.setFlag(Qt::LeftButton);
    QVERIFY(btn.testFlag(Qt::LeftButton));
    QVERIFY(!btn.testFlag(Qt::MiddleButton));

    btn.setFlag(Qt::LeftButton, false);
    QVERIFY(!btn.testFlag(Qt::LeftButton));
    QVERIFY(!btn.testFlag(Qt::MiddleButton));

    MyStrictFlags flags;
    flags.setFlag(MyStrictEnum::StrictOne);
    flags.setFlag(MyStrictEnum::StrictTwo, true);
    QVERIFY(flags.testFlag(MyStrictEnum::StrictOne));
    QVERIFY(flags.testFlag(MyStrictEnum::StrictTwo));
    QVERIFY(!flags.testFlag(MyStrictEnum::StrictFour));

    flags.setFlag(MyStrictEnum::StrictTwo, false);
    QVERIFY(flags.testFlag(MyStrictEnum::StrictOne));
    QVERIFY(!flags.testFlag(MyStrictEnum::StrictTwo));
    QVERIFY(!flags.testFlag(MyStrictEnum::StrictFour));
}

namespace SomeNS {
enum Foo { Foo_A = 1 << 0, Foo_B = 1 << 1, Foo_C = 1 << 2 };

Q_DECLARE_FLAGS(Foos, Foo)
Q_DECLARE_OPERATORS_FOR_FLAGS(Foos);

Qt::Alignment alignment()
{
    // Checks that the operator| works, despite there is another operator| in this namespace.
    return Qt::AlignLeft | Qt::AlignTop;
}
}

void tst_QFlags::adl()
{
    SomeNS::Foos fl = SomeNS::Foo_B | SomeNS::Foo_C;
    QVERIFY(fl & SomeNS::Foo_B);
    QVERIFY(!(fl & SomeNS::Foo_A));
    QCOMPARE(SomeNS::alignment(), Qt::AlignLeft | Qt::AlignTop);
}

// (statically) check QTypeInfo for QFlags instantiations:
enum MyEnum { Zero, One, Two, Four=4 };
Q_DECLARE_FLAGS( MyFlags, MyEnum )
Q_DECLARE_OPERATORS_FOR_FLAGS( MyFlags )

Q_STATIC_ASSERT( !QTypeInfo<MyFlags>::isComplex );
Q_STATIC_ASSERT( !QTypeInfo<MyFlags>::isStatic );
Q_STATIC_ASSERT( !QTypeInfo<MyFlags>::isLarge );
Q_STATIC_ASSERT( !QTypeInfo<MyFlags>::isPointer );

QTEST_MAIN(tst_QFlags)
#include "tst_qflags.moc"
