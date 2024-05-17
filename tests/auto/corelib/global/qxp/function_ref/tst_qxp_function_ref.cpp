// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qxpfunctional.h>

#include <QTest>

#include <type_traits>

//  checking dependency q20::remove_cvref_t:
#define CHECK(in, out) \
    static_assert(std::is_same_v<q20::remove_cvref_t< in >, out >)
CHECK(int, int);
CHECK(const int, int);
CHECK(int &, int);
CHECK(const int &, int);
CHECK(int &&, int);
CHECK(const int &&, int);
CHECK(int *, int *);
CHECK(const int *, const int *);
CHECK(int[4], int[4]);
CHECK(const int (&)[4], int[4]);
#undef CHECK

template <typename T> constexpr inline bool
is_noexcept_function_ref_helper_v = false;
template <typename R, typename...Args> constexpr inline bool
is_noexcept_function_ref_helper_v<qxp::function_ref<R(Args...) noexcept(true)>> = true;
template <typename R, typename...Args> constexpr inline bool
is_noexcept_function_ref_helper_v<qxp::function_ref<R(Args...) const noexcept(true)>> = true;

template <typename T> constexpr inline bool
is_noexcept_function_ref_v = is_noexcept_function_ref_helper_v<q20::remove_cvref_t<T>>;

class tst_qxp_function_ref : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

private Q_SLOTS:
    void basics();
    void constOverloads();
    void constExpr();
    void voidReturning();
    void ctad();
};

void tst_qxp_function_ref::basics()
{
    static_assert(std::is_trivially_copyable_v<qxp::function_ref<int(int)>>);
    static_assert(std::is_trivially_copyable_v<qxp::function_ref<int()>>);
    static_assert(std::is_trivially_copyable_v<qxp::function_ref<void()>>);

    {
        Q_CONSTINIT static int invoked = 0;
        auto lambda = [](int i) noexcept { ++invoked; return i; };
        const qxp::function_ref<int(int)> f = lambda;
        QCOMPARE(invoked, 0);
        QCOMPARE(f(42), 42);
        QCOMPARE(invoked, 1);

        const int fourtyTwo = 42;

        const qxp::function_ref<int(int) noexcept> f2 = std::move(lambda);
        QCOMPARE(invoked, 1);
        QCOMPARE(f2(fourtyTwo), 42);
        QCOMPARE(invoked, 2);

        int (*fpr)(int) = lambda;

        const qxp::function_ref f3 = fpr;
        static_assert(!is_noexcept_function_ref_v<decltype(f3)>);
        QCOMPARE(invoked, 2);
        QCOMPARE(f3(42), 42);
        QCOMPARE(invoked, 3);

        int (*fpr2)(int) noexcept = lambda;

        const qxp::function_ref f4 = fpr2;
        static_assert(is_noexcept_function_ref_v<decltype(f4)>);
        QCOMPARE(invoked, 3);
        QCOMPARE(f4(42), 42);
        QCOMPARE(invoked, 4);
    }
    {
        Q_CONSTINIT static int invoked = 0;
        auto lambda = [] { ++invoked; return 42; };
        const qxp::function_ref<int()> f = lambda;
        QCOMPARE(invoked, 0);
        QCOMPARE(f(), 42);
        QCOMPARE(invoked, 1);

        const qxp::function_ref<int()> f2 = std::move(lambda);
        QCOMPARE(invoked, 1);
        QCOMPARE(f2(), 42);
        QCOMPARE(invoked, 2);

        int (*fpr)() = lambda;

        const qxp::function_ref f3 = fpr;
        static_assert(!is_noexcept_function_ref_v<decltype(f3)>);
        QCOMPARE(invoked, 2);
        QCOMPARE(f3(), 42);
        QCOMPARE(invoked, 3);
    }
    {
        Q_CONSTINIT static int invoked = 0;
        auto lambda = [] { ++invoked; };
        const qxp::function_ref<void()> f = lambda;
        QCOMPARE(invoked, 0);
        f();
        QCOMPARE(invoked, 1);

        const qxp::function_ref<void()> f2 = std::move(lambda);
        QCOMPARE(invoked, 1);
        f2();
        QCOMPARE(invoked, 2);

        void (*fpr)() = lambda;

        const qxp::function_ref f3 = fpr;
        QCOMPARE(invoked, 2);
        f3();
        QCOMPARE(invoked, 3);
    }
}

void tst_qxp_function_ref::constOverloads()
{
    auto func_c = [](qxp::function_ref<int() const> callable)
    {
        return callable();
    };
    auto func_m = [](qxp::function_ref<int() /*mutable*/> callable)
    {
        return callable();
    };

    struct S
    {
        int operator()() { return 1; }
        int operator()() const { return 2; }
    };
    S s;
    QCOMPARE(func_c(s), 2);
    QCOMPARE(func_m(s), 1);
    const S cs;
    QCOMPARE(func_c(cs), 2);
#if 0
    // this should not compile (and doesn't, but currently fails with an error in the impl,
    // not by failing a constructor constaint → spec issue?).
    QCOMPARE(func_m(cs), 2);
#endif
}

void tst_qxp_function_ref::constExpr()
{
    Q_CONSTINIT static int invoked = 0;
    {
        Q_CONSTINIT static auto lambda = [] (int i) { ++invoked; return i; };
        // the function object constructor is constexpr, so this should be constinit:
        Q_CONSTINIT static qxp::function_ref<int(int)> f = lambda;

        QCOMPARE(invoked, 0);
        QCOMPARE(f(15), 15);
        QCOMPARE(invoked, 1);
    }
    {
        constexpr static auto lambda = [] (int i) { ++invoked; return i; };
        // the function object constructor is constexpr, so this should be constinit:
        Q_CONSTINIT static qxp::function_ref<int(int) const> f = lambda;

        QCOMPARE(invoked, 1);
        QCOMPARE(f(51), 51);
        QCOMPARE(invoked, 2);

#if 0 // ### should this work?:
        Q_CONSTINIT static qxp::function_ref<int(int)> f2 = lambda;

        QCOMPARE(invoked, 2);
        QCOMPARE(f(150), 150);
        QCOMPARE(invoked, 3);
#endif

    }
}

int  i_f_i_nx(int i) noexcept { return i; }
void v_f_i_nx(int)   noexcept {}
int  i_f_v_nx()      noexcept { return 42; }
void v_f_v_nx()      noexcept {}

int  i_f_i_ex(int i) { return i; }
void v_f_i_ex(int)   {}
int  i_f_v_ex()      { return 42; }
void v_f_v_ex()      {}

void tst_qxp_function_ref::voidReturning()
{
    // check that "casting" int to void returns works:

    using Fi = qxp::function_ref<void(int)>;
    using Fv = qxp::function_ref<void()>;

    {
        Fi fi = i_f_i_nx;
        fi(42);
        Fv fv = i_f_v_nx;
        fv();
    }

    {
        Fi fi = i_f_i_ex;
        fi(42);
        Fv fv = i_f_v_ex;
        fv();
    }

    // now with lambdas

    bool ok = false; // prevent lambdas from decaying to function pointers
    {
        auto lambda1 = [&](int i) noexcept { return i + int(ok); };
        Fi fi = lambda1;
        fi(42);
        auto lambda2 = [&]() noexcept { return int(ok); };
        Fv fv = lambda2;
        fv();
    }

    {
        auto lambda1 = [&](int i) { return i + int(ok); };
        Fi fi = lambda1;
        fi(42);
        auto lambda2 = [&]() { return int(ok); };
        Fv fv = lambda2;
        fv();
    }
}

void tst_qxp_function_ref::ctad()
{
#define CHECK(fun, sig) \
    do { \
        qxp::function_ref f = fun; \
        static_assert(std::is_same_v<decltype(f), \
                                    qxp::function_ref<sig>>); \
        qxp::function_ref f2 = &fun; \
        static_assert(std::is_same_v<decltype(f2), \
                                    qxp::function_ref<sig>>); \
        static_assert(std::is_trivially_copyable_v<decltype(f)>); \
        static_assert(std::is_trivially_copyable_v<decltype(f2)>); \
    } while (false)

    CHECK(i_f_i_nx, int (int) noexcept);
    CHECK(v_f_i_nx, void(int) noexcept);
    CHECK(i_f_v_nx, int (   ) noexcept);
    CHECK(v_f_v_nx, void(   ) noexcept);

    CHECK(i_f_i_ex, int (int));
    CHECK(v_f_i_ex, void(int));
    CHECK(i_f_v_ex, int (   ));
    CHECK(v_f_v_ex, void(   ));

#undef CHECK

#if 0 // no deduction guides for the non-function-pointer case, so no CTAD for lambdas
    {
        auto lambda = [](int i) -> int { return i; };
        qxp::function_ref f = lambda;
        static_assert(std::is_same_v<decltype(f),
                                     qxp::function_ref<int(int)>>);
    }
#endif
}


QTEST_APPLESS_MAIN(tst_qxp_function_ref);

#include "tst_qxp_function_ref.moc"
