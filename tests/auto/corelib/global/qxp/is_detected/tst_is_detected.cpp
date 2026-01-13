// Copyright (C) 2025 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qxptype_traits.h>

#include <QTest>

class tst_qxp_is_detected : public QObject
{
    Q_OBJECT
};

// ALl major compilers have bugs regarding handling accessibility
// from SFINAE contexts; skip tests relying on it.

namespace InnerTypedefTest {

template <typename T>
using HasInnerFooTypedefTest = typename T::Foo;

struct A {};
struct B { using Foo = void; };
class C { using Foo = void; }; // inaccessible
struct D { int Foo; };
struct E { int Foo() const; };
struct F { static void Foo(); };

#define FAIL(Type) \
    static_assert(!qxp::is_detected_v<HasInnerFooTypedefTest, Type>); \
    static_assert(std::is_same_v<qxp::detected_or_t<int, HasInnerFooTypedefTest, Type>, int>); \
    static_assert(std::is_same_v<qxp::detected_t<HasInnerFooTypedefTest, Type>, qxp::nonesuch>)
FAIL(A);
static_assert( qxp::is_detected_v<HasInnerFooTypedefTest, B>);
static_assert(std::is_same_v<qxp::detected_or_t<int, HasInnerFooTypedefTest, B>, void>);
static_assert(std::is_same_v<qxp::detected_t<HasInnerFooTypedefTest, B>, void>);
// FAIL(C); // see above
FAIL(D);
FAIL(E);
FAIL(F);
#undef FAIL

} // InnerTypedefTest

namespace ReflectionTest {

template <typename T>
using HasPublicConstFooFunctionTest = decltype(std::declval<const T &>().foo());

struct A {};
struct B { void foo(); };
struct C { void foo() const; };
struct D { void foo(int) const; };
struct E { void foo(int = 42) const; };
struct F { void foo() const &&; };
struct G { int foo; };
class H { void foo(); };
class I { void foo() const; };

#define FAIL(Type) \
    static_assert(!qxp::is_detected_v<HasPublicConstFooFunctionTest, Type>); \
    static_assert(std::is_same_v<qxp::detected_or_t<long, HasPublicConstFooFunctionTest, Type>, long>); \
    static_assert(std::is_same_v<qxp::detected_t<HasPublicConstFooFunctionTest, Type>, qxp::nonesuch>)
FAIL(A);
FAIL(B);
static_assert( qxp::is_detected_v<HasPublicConstFooFunctionTest, C>);
static_assert(std::is_same_v<qxp::detected_or_t<long, HasPublicConstFooFunctionTest, C>, void>);
static_assert(std::is_same_v<qxp::detected_t<HasPublicConstFooFunctionTest, C>, void>);
FAIL(D);
static_assert( qxp::is_detected_v<HasPublicConstFooFunctionTest, E>);
static_assert(std::is_same_v<qxp::detected_or_t<long, HasPublicConstFooFunctionTest, E>, void>);
static_assert(std::is_same_v<qxp::detected_t<HasPublicConstFooFunctionTest, E>, void>);
FAIL(F);
FAIL(G);
// FAIL(H); // see above
// FAIL(I); // see above
#undef FAIL

} // ReflectionTest

namespace InnerTypedefTestFriend
{

struct Helper
{
    template <typename T>
    using HasInnerFooTypedefTest = typename T::Foo;
};

struct A {};
struct B { using Foo = void; };
class C { using Foo = void; }; // inaccessible
class D { friend struct Helper; using Foo = void; };

static_assert(!qxp::is_detected_v<Helper::HasInnerFooTypedefTest, A>);
static_assert(std::is_same_v<qxp::detected_or_t<int, Helper::HasInnerFooTypedefTest, A>, int>);
static_assert(std::is_same_v<qxp::detected_t<Helper::HasInnerFooTypedefTest, A>, qxp::nonesuch>);
static_assert( qxp::is_detected_v<Helper::HasInnerFooTypedefTest, B>);
static_assert(std::is_same_v<qxp::detected_or_t<int, Helper::HasInnerFooTypedefTest, B>, void>);
static_assert(std::is_same_v<qxp::detected_t<Helper::HasInnerFooTypedefTest, B>, void>);
// static_assert(!qxp::is_detected_v<Helper::HasInnerFooTypedefTest, C>); // see above
// static_assert(!qxp::is_detected_v<Helper::HasInnerFooTypedefTest, D>); // see above
}

QTEST_APPLESS_MAIN(tst_qxp_is_detected);

#include "tst_is_detected.moc"
