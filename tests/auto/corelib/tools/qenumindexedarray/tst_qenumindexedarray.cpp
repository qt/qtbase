// Copyright (C) 2026 The Qt Company Ltd.
// Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "private/qenumindexedarray_p.h"

#include <QTest>

#include <memory>
#include <type_traits>

enum A {
    One, Two, Three,
    MAX
};

enum class B {
    Uno, Dos, Tres,
    MAX
};

namespace BasicPremise {
namespace test {
template <typename EnumIndexedArray, typename Arg>
using at = decltype(
        std::declval<EnumIndexedArray        &>().at(std::declval<Arg>()),
        std::declval<EnumIndexedArray const  &>().at(std::declval<Arg>()),
        std::declval<EnumIndexedArray       &&>().at(std::declval<Arg>()),
        std::declval<EnumIndexedArray const &&>().at(std::declval<Arg>()),
     0
    );
template <typename EnumIndexedArray, typename Arg>
using op = decltype(
        std::declval<EnumIndexedArray        &>()[std::declval<Arg>()],
        std::declval<EnumIndexedArray const  &>()[std::declval<Arg>()],
        std::declval<EnumIndexedArray       &&>()[std::declval<Arg>()],
        std::declval<EnumIndexedArray const &&>()[std::declval<Arg>()],
        0
    );
}

static_assert( qxp::is_detected_v<test::at, QEnumIndexedArrayFor<int, A::MAX>, A>);   // OK:  at(A)
static_assert(!qxp::is_detected_v<test::at, QEnumIndexedArrayFor<int, A::MAX>, int>); // NAK: at(int)
static_assert(!qxp::is_detected_v<test::at, QEnumIndexedArrayFor<int, A::MAX>, B>);   // NAK: at(B)

static_assert( qxp::is_detected_v<test::op, QEnumIndexedArrayFor<int, A::MAX>, A>);   // OK:  op[A]
static_assert(!qxp::is_detected_v<test::op, QEnumIndexedArrayFor<int, A::MAX>, int>); // NAK: op[int]
static_assert(!qxp::is_detected_v<test::op, QEnumIndexedArrayFor<int, A::MAX>, B>);   // NAK: op[B]

static_assert( qxp::is_detected_v<test::at, QEnumIndexedArrayFor<int, B::MAX>, B>);   // OK:  at(B)
static_assert(!qxp::is_detected_v<test::at, QEnumIndexedArrayFor<int, B::MAX>, int>); // NAK: at(int)
static_assert(!qxp::is_detected_v<test::at, QEnumIndexedArrayFor<int, B::MAX>, A>);   // NAK: at(A)

static_assert( qxp::is_detected_v<test::op, QEnumIndexedArrayFor<int, B::MAX>, B>);   // OK:  op[B]
static_assert(!qxp::is_detected_v<test::op, QEnumIndexedArrayFor<int, B::MAX>, int>); // NAK: op[int]
static_assert(!qxp::is_detected_v<test::op, QEnumIndexedArrayFor<int, B::MAX>, A>);   // NAK: op[A]

} // namespace BasicPremise

class tst_QEnumIndexedArray : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

private Q_SLOTS:
    void basics_enum() { basics_impl(A::One, A::Two, A::Three); }
    void basics_enum_class() { basics_impl(B::Uno, B::Dos, B::Tres); }

    void cursorExample();
    void constExpr();

private:
    template <typename E>
    void basics_impl(E one, E two, E three);
};


template <typename E>
void tst_QEnumIndexedArray::basics_impl(E one, E two, E three)
{
    {
        QEnumIndexedArrayFor<int, E::MAX> array = {{{
            {one, -1},
            {two, 0},
            {three, 42},
        }}};
        static_assert(std::is_same_v<typename decltype(array)::index_type, E>);

        QCOMPARE_EQ(array[one], -1);
        QCOMPARE_EQ(array[two], 0);
        QCOMPARE_EQ(array[three], 42);

        QVERIFY_THROWS_EXCEPTION(std::out_of_range, array.at(E::MAX));

        QEnumIndexedArrayFor<int, E::MAX> copied = array;
        QEnumIndexedArrayFor<int, E::MAX> moved = std::move(array);
        QCOMPARE_EQ(copied, moved);

        QEnumIndexedArrayFor<int, E::MAX> zeros = {{0, 0, 0}};
        QEnumIndexedArrayFor<int, E::MAX> ones  = {{1, 1, 1}};
        swap(zeros, ones);
        QCOMPARE_EQ(zeros[one], 1);
        QCOMPARE_EQ(ones[one], 0);

        // Tuple Protocol

        const auto [x1, x2, x3] = array;
        QCOMPARE_EQ(x1, -1);
        QCOMPARE_EQ(x2, 0);
        QCOMPARE_EQ(x3, 42);
    }
}

void tst_QEnumIndexedArray::cursorExample()
{
    // this is the example from the header docs
    struct CursorData {
        // ~~~~
    };
    constexpr auto NumQtCursorShapes = Qt::CursorShape(Qt::CustomCursor + 1);
    using CursorIndexedArray = QEnumIndexedArrayFor<
            std::unique_ptr<CursorData>,
            NumQtCursorShapes
        >;
    CursorIndexedArray cursorData;

    //~~~~
    auto& cross = cursorData[Qt::CrossCursor]; // OK
    // auto& ibeam = cursorData[4]; // Qt::IBeamCursor; ERROR: no op[] taking int

    QCOMPARE(cross, nullptr);
    QCOMPARE(cursorData.size(), qToUnderlying(NumQtCursorShapes));
}

void tst_QEnumIndexedArray::constExpr()
{
    constexpr QEnumIndexedArrayFor<int, B::MAX> a = {{{
        {B::Uno,    42},
        {B::Dos,   -42},
        {B::Tres, 0x42},
    }}};

    // value()
    {
        constexpr auto a0 = a.value(0);
        static_assert(a0 == 42);
        constexpr auto a1 = a.value(1);
        static_assert(a1 == -42);
        constexpr auto a2 = a.value(2);
        static_assert(a2 == 0x42);
    }

    // at()
    {
        constexpr auto a0 = a.at(B::Uno);
        static_assert(a0 == 42);
        constexpr auto a1 = a.at(B::Dos);
        static_assert(a1 == -42);
        constexpr auto a2 = a.at(B::Tres);
        static_assert(a2 == 0x42);
    }

    // op[]
    {
        constexpr auto a0 = a[B::Uno];
        static_assert(a0 == 42);
        constexpr auto a1 = a[B::Dos];
        static_assert(a1 == -42);
        constexpr auto a2 = a[B::Tres];
        static_assert(a2 == 0x42);
    }

    // front()/back()
    {
        constexpr auto a0 = a.front();
        static_assert(a0 == 42);
        constexpr auto a2 = a.back();
        static_assert(a2 == 0x42);
    }

    // data()
    {
        constexpr auto a0 = a.data()[0];
        static_assert(a0 == 42);
        constexpr auto a1 = a.data()[1];
        static_assert(a1 == -42);
        constexpr auto a2 = a.data()[2];
        static_assert(a2 == 0x42);
    }


    // begin()/end()
    {
        constexpr auto a0 = a.begin()[0];
        static_assert(a0 == 42);
        constexpr auto a1 = a.begin()[1];
        static_assert(a1 == -42);
        constexpr auto a2 = a.end()[-1];
        static_assert(a2 == 0x42);
    }

    // cbegin()/cend()
    {
        constexpr auto a0 = a.cbegin()[0];
        static_assert(a0 == 42);
        constexpr auto a1 = a.cbegin()[1];
        static_assert(a1 == -42);
        constexpr auto a2 = a.cend()[-1];
        static_assert(a2 == 0x42);
    }

    // rbegin()/rend()
    {
        constexpr auto a0 = a.rend()[-1];
        static_assert(a0 == 42);
        constexpr auto a1 = a.rbegin()[1];
        static_assert(a1 == -42);
        constexpr auto a2 = a.rbegin()[0];
        static_assert(a2 == 0x42);
    }

    // crbegin()/crend()
    {
        constexpr auto a0 = a.crend()[-1];
        static_assert(a0 == 42);
        constexpr auto a1 = a.crbegin()[1];
        static_assert(a1 == -42);
        constexpr auto a2 = a.crbegin()[0];
        static_assert(a2 == 0x42);
    }

    // empty()/size()/max_size()
    static_assert(!a.empty());
    static_assert(a.size() == 3);
    static_assert(a.max_size() == 3);

    // Tuple Protocol
    {
        using std::get;
        constexpr auto a0 = get<0>(a);
        static_assert(a0 == 42);
        constexpr auto a1 = get<1>(a);
        static_assert(a1 == -42);
        constexpr auto a2 = get<2>(a);
        static_assert(a2 == 0x42);

    }

// https://cppreference.com/cpp/feature_test says all these are covered by:
#if __cpp_lib_array_constexpr >= 201811L
    // fill()
    {
        constexpr auto b = [&] {
            auto copy = a;
            copy.fill(17);
            return copy;
        }();
        static_assert(b.value(0) == 17);
        static_assert(b.value(1) == 17);
        static_assert(b.value(2) == 17);
    }

    // swap()
    {
        constexpr auto b = [&] {
            auto b = a;
            decltype(b) null = {};
            // Round and round she goes, where she stops, nobody knows.
            b.swap(null);
            b.swap(null);
            swap(b, null);
            return b;
        }();
        static_assert(b.value(0) == 0);
        static_assert(b.value(1) == 0);
        static_assert(b.value(2) == 0);
    }

    // equal containers
    {
        constexpr auto copy = a;
        static_assert(copy == a);
        static_assert(!(copy != a));
        static_assert(copy <= a);
        static_assert(!(copy < a));
        static_assert(copy >= a);
        static_assert(!(copy > a));
#ifdef __cpp_lib_three_way_comparison
        static_assert((copy <=> a) == std::strong_ordering::equal);
#endif
    }

    // unequal containers
    {
        constexpr decltype(a) b = {{{
            {B::Uno,    17},
            {B::Dos,   -17},
            {B::Tres, 0x17},
        }}};
        static_assert(b != a);
        static_assert(!(b == a));
        static_assert(b <= a);
        static_assert(b < a);
        static_assert(!(b >= a));
        static_assert(!(b > a));
#ifdef __cpp_lib_three_way_comparison
        static_assert((b <=> a) == std::strong_ordering::less);
#endif
    }
#endif // __cpp_lib_array_constexpr >= 201811L
}


QTEST_APPLESS_MAIN(tst_QEnumIndexedArray)

#include "tst_qenumindexedarray.moc"
