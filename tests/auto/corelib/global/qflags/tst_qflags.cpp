// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef QFLAGS_TEST_NO_TYPESAFE_FLAGS
# ifdef QT_TYPESAFE_FLAGS
#  undef QT_TYPESAFE_FLAGS
# endif
#else
# ifndef QT_TYPESAFE_FLAGS
#  define QT_TYPESAFE_FLAGS
# endif
#endif

#include <QTest>

class tst_QFlags: public QObject
{
    Q_OBJECT
private slots:
    void boolCasts() const;
    void operators() const;
    void mixingDifferentEnums() const;
    void testFlag() const;
    void testFlagZeroFlag() const;
    void testFlagMultiBits() const;
    void testFlags();
    void testAnyFlag();
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
        QCOMPARE((LHS op RHS), (RES)); \
        QCOMPARE(( /*CTAD*/ QFlags(LHS) op RHS), (RES)); \
        QCOMPARE((LHS op QFlags(RHS)), (RES)); \
        QCOMPARE((QFlags(LHS) op QFlags(RHS)), (RES)); \
        QCOMPARE((QFlags(LHS) op ## = RHS), (RES)); \
        QCOMPARE((QFlags(LHS) op ## = QFlags(RHS)), (RES)); \
    } while (false)

    CHECK(|, Qt::AlignHCenter, Qt::AlignVCenter, Qt::AlignCenter);
    CHECK(|, Qt::AlignHCenter, Qt::AlignHCenter, Qt::AlignHCenter);
    CHECK(&, Qt::AlignHCenter, Qt::AlignVCenter, Qt::Alignment());
    CHECK(&, Qt::AlignHCenter, Qt::AlignHCenter, Qt::AlignHCenter);
    CHECK(^, Qt::AlignHCenter, Qt::AlignVCenter, Qt::AlignCenter);
    CHECK(^, Qt::AlignHCenter, Qt::AlignHCenter, Qt::Alignment());
#undef CHECK
}

void tst_QFlags::mixingDifferentEnums() const
{
#define CHECK(op, LHS, RHS, RES) \
    /* LHS must be QFlags'able */ \
    do { \
        QCOMPARE((LHS op RHS), (RES)); \
        QCOMPARE((RHS op LHS), (RES)); \
        /*QCOMPARE(( / *CTAD* / QFlags(LHS) op RHS), (RES));*/ \
        /*QCOMPARE((QFlags(LHS) op ## = RHS), (RES));*/ \
    } while (false)

    // AlignmentFlags <-> TextFlags
    {
        CHECK(|, Qt::AlignCenter, Qt::TextSingleLine, 0x0184);
        CHECK(&, Qt::AlignCenter, Qt::TextSingleLine, 0x0000);
        CHECK(^, Qt::AlignCenter, Qt::TextSingleLine, 0x0184);
    }
    // QFlags<AlignmentFlags> <-> TextFlags
    {
#ifndef QT_TYPESAFE_FLAGS // QTBUG-101344
        Qt::Alignment MyAlignCenter = Qt::AlignCenter; // convert enum to QFlags
        CHECK(|, MyAlignCenter, Qt::TextSingleLine, 0x0184U); // yes, unsigned!
        CHECK(&, MyAlignCenter, Qt::TextSingleLine, 0x0000U); // yes, unsigned!
        CHECK(^, MyAlignCenter, Qt::TextSingleLine, 0x0184U); // yes, unsigned!
#endif
    }
    // TextElideMode <-> TextFlags
    {
        CHECK(|, Qt::ElideNone, Qt::TextSingleLine, 0x0103);
        CHECK(&, Qt::ElideNone, Qt::TextSingleLine, 0x0000);
        CHECK(^, Qt::ElideNone, Qt::TextSingleLine, 0x0103);
    }
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

void tst_QFlags::testFlags()
{
    using Int = Qt::TextInteractionFlags::Int;
    constexpr Int Zero(0);

    Qt::TextInteractionFlags flags;
    QCOMPARE(flags.toInt(), Zero);
    QVERIFY(flags.testFlags(flags));
    QVERIFY(Qt::TextInteractionFlags::fromInt(Zero).testFlags(flags));
    QVERIFY(!flags.testFlags(Qt::TextSelectableByMouse));
    QVERIFY(!flags.testFlags(Qt::TextSelectableByKeyboard));
    QVERIFY(!flags.testFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
    QVERIFY(flags.testFlags(Qt::TextInteractionFlags::fromInt(Zero)));
    QVERIFY(flags.testFlags(Qt::TextInteractionFlags(Qt::TextSelectableByMouse) & ~Qt::TextSelectableByMouse));

    flags = Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard;
    QVERIFY(flags.toInt() != Zero);
    QVERIFY(flags.testFlags(flags));
    QVERIFY(flags.testFlags(Qt::TextSelectableByMouse));
    QVERIFY(flags.testFlags(Qt::TextSelectableByKeyboard));
    QVERIFY(flags.testFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse));
    QVERIFY(!flags.testFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse | Qt::TextEditable));
    QVERIFY(!flags.testFlags(Qt::TextInteractionFlags()));
    QVERIFY(!flags.testFlags(Qt::TextInteractionFlags::fromInt(Zero)));
    QVERIFY(!flags.testFlags(Qt::TextEditable));
    QVERIFY(!flags.testFlags(Qt::TextSelectableByMouse | Qt::TextEditable));
}

void tst_QFlags::testAnyFlag()
{
    Qt::TextInteractionFlags flags;
    QVERIFY(!flags.testAnyFlags(Qt::NoTextInteraction));
    QVERIFY(!flags.testAnyFlags(Qt::TextSelectableByMouse));
    QVERIFY(!flags.testAnyFlags(Qt::TextSelectableByKeyboard));
    QVERIFY(!flags.testAnyFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
    QVERIFY(!flags.testAnyFlag(Qt::TextEditorInteraction));
    QVERIFY(!flags.testAnyFlag(Qt::TextBrowserInteraction));

    flags = Qt::TextSelectableByMouse;
    QVERIFY(!flags.testAnyFlags(Qt::NoTextInteraction));
    QVERIFY(flags.testAnyFlags(Qt::TextSelectableByMouse));
    QVERIFY(!flags.testAnyFlags(Qt::TextSelectableByKeyboard));
    QVERIFY(flags.testAnyFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
    QVERIFY(flags.testAnyFlag(Qt::TextEditorInteraction));
    QVERIFY(flags.testAnyFlag(Qt::TextBrowserInteraction));

    flags = Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard;
    QVERIFY(!flags.testAnyFlags(Qt::NoTextInteraction));
    QVERIFY(flags.testAnyFlags(Qt::TextSelectableByMouse));
    QVERIFY(flags.testAnyFlags(Qt::TextSelectableByKeyboard));
    QVERIFY(flags.testAnyFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
    QVERIFY(flags.testAnyFlag(Qt::TextEditorInteraction));
    QVERIFY(flags.testAnyFlag(Qt::TextEditorInteraction));
    QVERIFY(flags.testAnyFlag(Qt::TextBrowserInteraction));
}

template <unsigned int N, typename T> bool verifyConstExpr(T n) { return n == N; }
template <unsigned int N, typename T> bool verifyConstExpr(QFlags<T> n) { return n.toInt() == N; }

constexpr Qt::MouseButtons testRelaxedConstExpr()
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
    Qt::MouseButtons btn = Qt::LeftButton | Qt::RightButton;
    switch (btn.toInt()) {
    case Qt::LeftButton: QVERIFY(false); break;
    case Qt::RightButton: QVERIFY(false); break;
    case (Qt::LeftButton | Qt::RightButton).toInt(): QVERIFY(true); break;
    default: QFAIL(qPrintable(QStringLiteral("Unexpected button: %1").arg(btn.toInt())));
    }

#define VERIFY_CONSTEXPR(expression, expected) \
    QVERIFY(verifyConstExpr<(expression).toInt()>(expected))

    VERIFY_CONSTEXPR((Qt::LeftButton | Qt::RightButton) & Qt::LeftButton, Qt::LeftButton);
    VERIFY_CONSTEXPR((Qt::LeftButton | Qt::RightButton) & Qt::MiddleButton, 0);
    VERIFY_CONSTEXPR((Qt::LeftButton | Qt::RightButton) | Qt::MiddleButton, Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
    VERIFY_CONSTEXPR(~(Qt::LeftButton | Qt::RightButton), ~(Qt::LeftButton | Qt::RightButton));
    VERIFY_CONSTEXPR(Qt::MouseButtons(Qt::LeftButton) ^ Qt::RightButton, Qt::LeftButton ^ Qt::RightButton);
    VERIFY_CONSTEXPR(Qt::MouseButtons(0), 0);
#ifndef QT_TYPESAFE_FLAGS
    QVERIFY(verifyConstExpr<(Qt::MouseButtons(Qt::RightButton) & 0xff)>(Qt::RightButton));
    QVERIFY(verifyConstExpr<(Qt::MouseButtons(Qt::RightButton) | 0xff)>(0xff));
#endif

    QVERIFY(!verifyConstExpr<Qt::RightButton>(~Qt::MouseButtons(Qt::LeftButton)));

    VERIFY_CONSTEXPR(testRelaxedConstExpr(), Qt::MiddleButton);

#undef VERIFY_CONSTEXPR
}

void tst_QFlags::signedness()
{
    // these are all 'true' on GCC, but since the std says the
    // underlying type is implementation-defined, we need to allow for
    // a different signedness, so we only check that the relative
    // signedness of the types matches:
    static_assert((std::is_unsigned<typename std::underlying_type<Qt::MouseButton>::type>::value ==
                     std::is_unsigned<Qt::MouseButtons::Int>::value));

    static_assert((std::is_signed<typename std::underlying_type<Qt::AlignmentFlag>::type>::value ==
                     std::is_signed<Qt::Alignment::Int>::value));
}

enum class MyStrictEnum { StrictZero, StrictOne, StrictTwo, StrictFour=4 };
Q_DECLARE_FLAGS( MyStrictFlags, MyStrictEnum )
Q_DECLARE_OPERATORS_FOR_FLAGS( MyStrictFlags )

enum class MyStrictNoOpEnum { StrictZero, StrictOne, StrictTwo, StrictFour=4 };
Q_DECLARE_FLAGS( MyStrictNoOpFlags, MyStrictNoOpEnum )

static_assert( !QTypeInfo<MyStrictFlags>::isComplex );
static_assert( QTypeInfo<MyStrictFlags>::isRelocatable );
static_assert( !std::is_pointer_v<MyStrictFlags> );

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

#ifndef QT_TYPESAFE_FLAGS
    QCOMPARE(f3 & int(1), 1);
    QCOMPARE(f3 & uint(1), 1);
#endif
    QCOMPARE(f3 & MyStrictEnum::StrictOne, 1);

    MyStrictFlags aux;
#ifndef QT_TYPESAFE_FLAGS
    aux = f3;
    aux &= int(1);
    QCOMPARE(aux, 1);

    aux = f3;
    aux &= uint(1);
    QCOMPARE(aux, 1);
#endif

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

static_assert( !QTypeInfo<MyFlags>::isComplex );
static_assert( QTypeInfo<MyFlags>::isRelocatable );
static_assert( !std::is_pointer_v<MyFlags> );

QTEST_MAIN(tst_QFlags)
#include "tst_qflags.moc"
