// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifdef QFLAGS_TEST_NO_TYPESAFE_FLAGS
# ifdef QT_TYPESAFE_FLAGS
#  undef QT_TYPESAFE_FLAGS
# endif
# define tst_QFlags tst_QFlagsNotTypesafe
#else
# ifndef QT_TYPESAFE_FLAGS
#  define QT_TYPESAFE_FLAGS
# endif
#endif

#include <QTest>

#if defined(__cpp_concepts) && __has_include(<concepts>)
#  include <concepts>
#endif

#ifdef QFLAGS_TEST_64
# define tst_QFlags tst_QFlags64
using IntegerSize = QIntegerForSize<8>;
#else
using IntegerSize = QIntegerForSize<4>;
#endif

class tst_QFlags: public QObject
{
    Q_OBJECT
private slots:
    void construction() const;
    void boolCasts() const;
    void operators() const;
    void compareToZero() const;
    void mixingDifferentEnums() const;
    void testFlag() const;
    void testFlagZeroFlag() const;
    void testFlagMultiBits() const;
    void testFlags();
    void testAnyFlag();
    void constExpr();
    void signedness();
    void size();
    void castToFromQFlag();
    void classEnum();
    void initializerLists();
    void testSetFlags();
    void adl();
};

enum SignedFlag : IntegerSize::Signed {
    NoSignedFlag        = 0x00000000,
    LeftSignedFlag      = 0x00000001,
    RightSignedFlag     = 0x00000002,
    MiddleSignedFlag    = 0x00000004,
    SignedFlagMask      = IntegerSize::Signed(-1)
};
Q_DECLARE_FLAGS(SignedFlags, SignedFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(SignedFlags)

enum UnsignedFlag : IntegerSize::Unsigned {
    UnsignedFlag01 = 0x0001,
    UnsignedFlag02 = 0x0002,
    UnsignedFlag04 = 0x0004,

    UnsignedFlag10 = 0x0010,
    UnsignedFlag20 = 0x0020,
    UnsignedFlag30 = UnsignedFlag10 | UnsignedFlag20
};
Q_DECLARE_FLAGS(UnsignedFlags, UnsignedFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(UnsignedFlags)

enum MixingFlag {
    MixingFlag100 = 0x0100,
};
Q_DECLARE_MIXED_ENUM_OPERATORS_SYMMETRIC(IntegerSize::Signed, UnsignedFlag, MixingFlag)

void tst_QFlags::construction() const
{
    UnsignedFlags def;
    UnsignedFlags copied(def);
    UnsignedFlags moved(std::move(copied)); Q_UNUSED(moved);
    UnsignedFlags fromEnum(UnsignedFlag01); Q_UNUSED(fromEnum);
    UnsignedFlags inPlace(std::in_place, 0xffff); Q_UNUSED(inPlace);
    UnsignedFlags fromInt = UnsignedFlags::fromInt(0xffff); Q_UNUSED(fromInt);
    // initializer_list tested in initializerLists()
}

void tst_QFlags::boolCasts() const
{
    // This tests that the operator overloading is sufficient so that common
    // idioms involving flags -> bool casts work as expected:

    const UnsignedFlags nonNull = UnsignedFlag30;
    const UnsignedFlags null = {};

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

    CHECK(|, UnsignedFlag20, UnsignedFlag10, UnsignedFlag30);
    CHECK(|, UnsignedFlag20, UnsignedFlag20, UnsignedFlag20);
    CHECK(&, UnsignedFlag20, UnsignedFlag10, UnsignedFlags());
    CHECK(&, UnsignedFlag20, UnsignedFlag20, UnsignedFlag20);
    CHECK(^, UnsignedFlag20, UnsignedFlag10, UnsignedFlag30);
    CHECK(^, UnsignedFlag20, UnsignedFlag20, UnsignedFlags());
#undef CHECK
}

void tst_QFlags::compareToZero() const
{
    SignedFlags sf;
    UnsignedFlags uf;
    // Don't change these to QT_TEST_EQUALITY_OPS
    QVERIFY(sf == 0);
    QVERIFY(0 == sf);
    QVERIFY(!(sf != 0));
    QVERIFY(!(0 != sf));
    QVERIFY(uf == 0);
    QVERIFY(0 == uf);
    QVERIFY(!(uf != 0));
    QVERIFY(!(0 != uf));
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
        CHECK(|, UnsignedFlag30, MixingFlag100, 0x0130);
        CHECK(&, UnsignedFlag30, MixingFlag100, 0x0000);
        CHECK(^, UnsignedFlag30, MixingFlag100, 0x0130);
    }
    // QFlags<AlignmentFlags> <-> TextFlags
    {
#ifndef QT_TYPESAFE_FLAGS // QTBUG-101344
        UnsignedFlags flag30 = UnsignedFlag30; // convert enum to QFlags
        CHECK(|, flag30, MixingFlag100, 0x0130U); // yes, unsigned!
        CHECK(&, flag30, MixingFlag100, 0x0000U); // yes, unsigned!
        CHECK(^, flag30, MixingFlag100, 0x0130U); // yes, unsigned!
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
    SignedFlags btn = LeftSignedFlag | RightSignedFlag;

    QVERIFY(btn.testFlag(LeftSignedFlag));
    QVERIFY(!btn.testFlag(MiddleSignedFlag));

    btn = { };
    QVERIFY(!btn.testFlag(LeftSignedFlag));
}

void tst_QFlags::testFlagZeroFlag() const
{
    {
        SignedFlags btn = LeftSignedFlag | RightSignedFlag;
        /* NoSignedFlag has the value 0. */

        QVERIFY(!btn.testFlag(NoSignedFlag));
    }

    {
        /* A zero enum set should test true with zero. */
        QVERIFY(SignedFlags().testFlag(NoSignedFlag));
    }

    {
        SignedFlags btn = NoSignedFlag;
        QVERIFY(btn.testFlag(NoSignedFlag));
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

constexpr SignedFlags testRelaxedConstExpr()
{
    SignedFlags value;
    value = LeftSignedFlag | RightSignedFlag;
    value |= MiddleSignedFlag;
    value &= ~LeftSignedFlag;
    value ^= RightSignedFlag;
    return value;
}

void tst_QFlags::constExpr()
{
    SignedFlags btn = LeftSignedFlag | RightSignedFlag;
    switch (btn.toInt()) {
    case LeftSignedFlag: QVERIFY(false); break;
    case RightSignedFlag: QVERIFY(false); break;
    case (LeftSignedFlag | RightSignedFlag).toInt(): QVERIFY(true); break;
    default: QFAIL(qPrintable(QStringLiteral("Unexpected SignedFlag: %1").arg(btn.toInt())));
    }

#define VERIFY_CONSTEXPR(expression, expected) \
    do { constexpr auto result = (expression); QCOMPARE(result, expected); } while (0)

    VERIFY_CONSTEXPR((LeftSignedFlag | RightSignedFlag) & LeftSignedFlag, LeftSignedFlag);
    VERIFY_CONSTEXPR((LeftSignedFlag | RightSignedFlag) & MiddleSignedFlag, 0);
    VERIFY_CONSTEXPR((LeftSignedFlag | RightSignedFlag) | MiddleSignedFlag, LeftSignedFlag | RightSignedFlag | MiddleSignedFlag);
    VERIFY_CONSTEXPR(~(LeftSignedFlag | RightSignedFlag), ~(LeftSignedFlag | RightSignedFlag));
    VERIFY_CONSTEXPR(SignedFlags(LeftSignedFlag) ^ RightSignedFlag, LeftSignedFlag ^ RightSignedFlag);
    VERIFY_CONSTEXPR(SignedFlags{}, 0);
#ifndef tst_QFlags
    // only works with QFlag help
    VERIFY_CONSTEXPR(SignedFlags(0), 0);
#endif
#ifndef QT_TYPESAFE_FLAGS
    VERIFY_CONSTEXPR(SignedFlags(RightSignedFlag) & 0xff, RightSignedFlag);
    VERIFY_CONSTEXPR(SignedFlags(RightSignedFlag) | 0xff, 0xff);
#endif

    VERIFY_CONSTEXPR(testRelaxedConstExpr(), MiddleSignedFlag);

#undef VERIFY_CONSTEXPR
}

void tst_QFlags::signedness()
{
    // these are all 'true' on GCC, but since the std says the
    // underlying type is implementation-defined, we need to allow for
    // a different signedness, so we only check that the relative
    // signedness of the types matches:
    static_assert(std::is_unsigned_v<SignedFlags::Int> ==
            std::is_unsigned_v<std::underlying_type_t<SignedFlag>>);

#ifndef Q_CC_MSVC
    static_assert(std::is_unsigned_v<UnsignedFlags::Int> ==
            std::is_unsigned_v<std::underlying_type_t<UnsignedFlag>>);
#endif
}

enum class MyStrictEnum : IntegerSize::Unsigned
{ StrictZero, StrictOne, StrictTwo, StrictFour=4 };
Q_DECLARE_FLAGS( MyStrictFlags, MyStrictEnum )
Q_DECLARE_OPERATORS_FOR_FLAGS( MyStrictFlags )

enum class MyStrictNoOpEnum : IntegerSize::Unsigned
{ StrictZero, StrictOne, StrictTwo, StrictFour=4 };
Q_DECLARE_FLAGS( MyStrictNoOpFlags, MyStrictNoOpEnum )

static_assert( !QTypeInfo<MyStrictFlags>::isComplex );
static_assert( QTypeInfo<MyStrictFlags>::isRelocatable );
static_assert( !std::is_pointer_v<MyStrictFlags> );

void tst_QFlags::size()
{
    static_assert(sizeof(UnsignedFlags) >= sizeof(IntegerSize::Unsigned));
    static_assert(sizeof(MyStrictFlags) == sizeof(IntegerSize::Unsigned));
}

template <typename Flags, typename FlagType> void castToFromQFlag_template()
{
    // Verify that 32-bit QFlags works with QFlag and, through it,
    // can be constructed from integer types.
    auto testType = [](auto initialValue) {
        using T = decltype(+initialValue);
        FlagType flag(initialValue);    // can construct QFlag from this type
        T v1 = flag;                    // can cast QFlag to this type
        Q_UNUSED(v1);

        Flags flags(initialValue);      // can construct QFlags through QFlag from this type
        T v2 = QFlag(flags);            // can cast QFlags to this type through QFlag
        Q_UNUSED(v2);
    };
    testType(qint8(-1));
    testType(char(1));
    testType(uchar(2));
    testType(short(3));
    testType(ushort(4));
    testType(int(5));
    testType(uint(6));
#ifdef Q_CC_MSVC
    // QFlag has a constructor for uint for all other compilers, which make
    // the construction from long or ulong ambiguous.
    testType(long(7));
    testType(ulong(8));
#endif

    FlagType flag(1);
    IntegerSize::Signed i = flag;       // must cast to integers
    IntegerSize::Unsigned u = flag;     // must cast to integers
    QCOMPARE(i, 1);
    QCOMPARE(u, 1U);

    // QFlags has a constructor on QFlag
    SignedFlags f = flag;
    QCOMPARE(f, LeftSignedFlag);
    UnsignedFlags uf = flag;
    QCOMPARE(uf, UnsignedFlag01);
    MyStrictFlags sf = flag;
    QCOMPARE(sf, MyStrictEnum::StrictOne);

#ifndef Q_CC_MSVC
    // QFlags has a cast operator to QFlag
    // ### this used to work but began failing with MSVC after QFlagsStorage
    //     was introduced
    flag = FlagType(f);
    QCOMPARE(IntegerSize::Signed(flag), 1);
    flag = FlagType(uf);
    QCOMPARE(IntegerSize::Signed(flag), 1);
    flag = FlagType(sf);
    QCOMPARE(IntegerSize::Signed(flag), 1);
#endif

    // and thus this should compile
    QCOMPARE(f, 1);
    QCOMPARE(uf, 1);
    QCOMPARE(sf, 1);
}

template <typename Flags> void noCastToFromQFlag_template()
{
    // Verify that non-32-bit QFlags doesn't have QFlag support
    static_assert(!std::is_constructible_v<Flags, char>);
    static_assert(!std::is_constructible_v<Flags, uchar>);
    static_assert(!std::is_constructible_v<Flags, signed char>);
    static_assert(!std::is_constructible_v<Flags, char16_t>);
    static_assert(!std::is_constructible_v<Flags, char32_t>);
    static_assert(!std::is_constructible_v<Flags, short>);
    static_assert(!std::is_constructible_v<Flags, ushort>);
    static_assert(!std::is_constructible_v<Flags, int>);
    static_assert(!std::is_constructible_v<Flags, uint>);
    static_assert(!std::is_constructible_v<Flags, long>);
    static_assert(!std::is_constructible_v<Flags, ulong>);
    static_assert(!std::is_constructible_v<Flags, qlonglong>);
    static_assert(!std::is_constructible_v<Flags, qulonglong>);
    static_assert(!std::is_constructible_v<Flags, QFlag>);
    static_assert(!std::is_constructible_v<QFlag, Flags>);

#if defined(__cpp_concepts) && __has_include(<concepts>)
    static_assert(!std::equality_comparable_with<QFlag, Flags>);
    static_assert(!std::equality_comparable_with<Flags, int>);
#endif
}

void tst_QFlags::castToFromQFlag()
{
    if constexpr (sizeof(IntegerSize::Signed) == sizeof(int)) {
        castToFromQFlag_template<MyStrictFlags, QFlag>();
    } else {
        noCastToFromQFlag_template<MyStrictFlags>();
    }
}

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
    SignedFlags bts = { LeftSignedFlag, RightSignedFlag };
    QVERIFY(bts.testFlag(LeftSignedFlag));
    QVERIFY(bts.testFlag(RightSignedFlag));
    QVERIFY(!bts.testFlag(MiddleSignedFlag));

    MyStrictNoOpFlags flags = { MyStrictNoOpEnum::StrictOne, MyStrictNoOpEnum::StrictFour };
    QVERIFY(flags.testFlag(MyStrictNoOpEnum::StrictOne));
    QVERIFY(flags.testFlag(MyStrictNoOpEnum::StrictFour));
    QVERIFY(!flags.testFlag(MyStrictNoOpEnum::StrictTwo));
}

void tst_QFlags::testSetFlags()
{
    SignedFlags btn = NoSignedFlag;

    btn.setFlag(LeftSignedFlag);
    QVERIFY(btn.testFlag(LeftSignedFlag));
    QVERIFY(!btn.testFlag(MiddleSignedFlag));

    btn.setFlag(LeftSignedFlag, false);
    QVERIFY(!btn.testFlag(LeftSignedFlag));
    QVERIFY(!btn.testFlag(MiddleSignedFlag));

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
enum Foo : IntegerSize::Unsigned { Foo_A = 1 << 0, Foo_B = 1 << 1, Foo_C = 1 << 2 };

Q_DECLARE_FLAGS(Foos, Foo)
Q_DECLARE_OPERATORS_FOR_FLAGS(Foos);

UnsignedFlags unsignedFlags()
{
    // Checks that the operator| works, despite there is another operator| in this namespace.
    return UnsignedFlag01 | UnsignedFlag04;
}
}

void tst_QFlags::adl()
{
    SomeNS::Foos fl = SomeNS::Foo_B | SomeNS::Foo_C;
    QVERIFY(fl & SomeNS::Foo_B);
    QVERIFY(!(fl & SomeNS::Foo_A));
    QCOMPARE(SomeNS::unsignedFlags(), UnsignedFlag01 | UnsignedFlag04);
}

// (statically) check QTypeInfo for QFlags instantiations:
enum MyEnum : IntegerSize::Unsigned { Zero, One, Two, Four=4 };
Q_DECLARE_FLAGS( MyFlags, MyEnum )
Q_DECLARE_OPERATORS_FOR_FLAGS( MyFlags )

static_assert( !QTypeInfo<MyFlags>::isComplex );
static_assert( QTypeInfo<MyFlags>::isRelocatable );
static_assert( !std::is_pointer_v<MyFlags> );

QTEST_MAIN(tst_QFlags)
#include "tst_qflags.moc"
