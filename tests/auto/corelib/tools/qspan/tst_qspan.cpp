// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qspan_p.h>

#include <QList>
#include <QTest>

#include <algorithm>
#include <array>
#include <vector>

namespace {

struct NotNothrowMovable {
    NotNothrowMovable(NotNothrowMovable &&) noexcept(false) {};
    NotNothrowMovable &operator=(NotNothrowMovable &&) noexcept(false) { return *this; };
};
static_assert(!std::is_nothrow_move_constructible_v<NotNothrowMovable>);
static_assert(!std::is_nothrow_move_assignable_v<NotNothrowMovable>);

} // unnamed namespace

//
// QSpan is nothrow movable even if the payload type is not:
//
static_assert(std::is_nothrow_move_constructible_v<QSpan<NotNothrowMovable>>);
static_assert(std::is_nothrow_move_constructible_v<QSpan<NotNothrowMovable, 42>>);
static_assert(std::is_nothrow_move_constructible_v<QSpan<NotNothrowMovable, 0>>);

static_assert(std::is_nothrow_move_assignable_v<QSpan<NotNothrowMovable>>);
static_assert(std::is_nothrow_move_assignable_v<QSpan<NotNothrowMovable, 42>>);
static_assert(std::is_nothrow_move_assignable_v<QSpan<NotNothrowMovable, 0>>);

//
// All QSpans are trivially destructible and trivially copyable:
//
static_assert(std::is_trivially_copyable_v<QSpan<NotNothrowMovable>>);
static_assert(std::is_trivially_copyable_v<QSpan<NotNothrowMovable, 42>>);
static_assert(std::is_trivially_copyable_v<QSpan<NotNothrowMovable, 0>>);

static_assert(std::is_trivially_destructible_v<QSpan<NotNothrowMovable>>);
static_assert(std::is_trivially_destructible_v<QSpan<NotNothrowMovable, 42>>);
static_assert(std::is_trivially_destructible_v<QSpan<NotNothrowMovable, 0>>);

//
// Fixed-size QSpans implicitly convert to variable-sized ones:
//
static_assert(std::is_convertible_v<QSpan<int, 42>, QSpan<int>>);
static_assert(std::is_convertible_v<QSpan<int, 0>, QSpan<int>>);

//
// Mutable spans implicitly convert to read-only ones, but not vice versa:
//
static_assert(std::is_convertible_v<QSpan<int>, QSpan<const int>>);
static_assert(std::is_convertible_v<QSpan<int, 42>, QSpan<const int, 42>>);
static_assert(std::is_convertible_v<QSpan<int, 0>, QSpan<const int, 0>>);

static_assert(!std::is_convertible_v<QSpan<const int>, QSpan<int>>);
static_assert(!std::is_convertible_v<QSpan<const int, 42>, QSpan<int, 42>>);
static_assert(!std::is_convertible_v<QSpan<const int, 0>, QSpan<int, 0>>);

class tst_QSpan : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

private Q_SLOTS:
    void onlyZeroExtentSpansHaveDefaultCtors() const;
    void zeroExtentSpansMaintainADataPointer() const;
    void fromArray() const;
    void fromStdArray() const;
    void fromZeroSizeStdArray() const;
    void fromStdVector() const;
    void fromQList() const;

private:
    template <typename T, std::size_t N>
    void check_nonempty_span(QSpan<T, N>, qsizetype expectedSize) const;
    template <typename T, std::size_t N>
    void check_empty_span_incl_subspans(QSpan<T, N>) const;
    template <typename T, std::size_t N>
    void check_empty_span(QSpan<T, N>) const;
    template <typename T, std::size_t N>
    void check_null_span(QSpan<T, N>) const;

    template <std::size_t ExpectedExtent, typename C>
    void from_container_impl(C &&c) const;
    template <typename C>
    void from_variable_size_container_impl(C &&c) const;
};

#define RETURN_IF_FAILED() \
    do { if (QTest::currentTestFailed()) return; } while (false)

void tst_QSpan::onlyZeroExtentSpansHaveDefaultCtors() const
{
    static_assert(std::is_nothrow_default_constructible_v<QSpan<int, 0>>);
    static_assert(std::is_nothrow_default_constructible_v<QSpan<const int, 0>>);
    static_assert(std::is_nothrow_default_constructible_v<QSpan<int>>);
    static_assert(std::is_nothrow_default_constructible_v<QSpan<const int, 0>>);

    QSpan<int, 0> si;
    check_null_span(si);
    RETURN_IF_FAILED();

    QSpan<const int, 0> sci;
    check_null_span(sci);
    RETURN_IF_FAILED();

    QSpan<int> sdi;
    check_null_span(sdi);
    RETURN_IF_FAILED();

    QSpan<const int> sdci;
    check_null_span(sdci);
    RETURN_IF_FAILED();

    static_assert(!std::is_default_constructible_v<QSpan<int, 1>>);
    static_assert(!std::is_default_constructible_v<QSpan<const int, 42>>);
}

void tst_QSpan::zeroExtentSpansMaintainADataPointer() const
{
    int i;
    QSpan<int, 0> si{&i, 0};
    QCOMPARE(si.data(), &i);
    check_empty_span_incl_subspans(si);
    RETURN_IF_FAILED();

    QSpan<const int, 0> sci{&i, 0};
    QCOMPARE(sci.data(), &i);
    check_empty_span_incl_subspans(sci);
    RETURN_IF_FAILED();

    QSpan<int, 0> sdi{&i, 0};
    QCOMPARE(sdi.data(), &i);
    check_empty_span_incl_subspans(sdi);
    RETURN_IF_FAILED();

    QSpan<const int, 0> sdci{&i, 0};
    QCOMPARE(sdci.data(), &i);
    check_empty_span_incl_subspans(sdci);
    RETURN_IF_FAILED();
}

template <typename T, std::size_t N>
void tst_QSpan::check_nonempty_span(QSpan<T, N> s, qsizetype expectedSize) const
{
    static_assert(N > 0);
    QCOMPARE_GT(expectedSize, 0); // otherwise, use check_empty_span!

    QVERIFY(!s.empty());
    QVERIFY(!s.isEmpty());

    QCOMPARE_EQ(s.size(), expectedSize);
    QCOMPARE_NE(s.data(), nullptr);

    QCOMPARE_NE(s.begin(), s.end());
    QCOMPARE_NE(s.rbegin(), s.rend());
    QCOMPARE_NE(s.cbegin(), s.cend());
    QCOMPARE_NE(s.crbegin(), s.crend());

    QCOMPARE_EQ(s.end() - s.begin(), s.size());
    QCOMPARE_EQ(s.cend() - s.cbegin(), s.size());
    QCOMPARE_EQ(s.rend() - s.rbegin(), s.size());
    QCOMPARE_EQ(s.crend() - s.crbegin(), s.size());

    QCOMPARE_EQ(std::addressof(s.front()), std::addressof(*s.begin()));
    QCOMPARE_EQ(std::addressof(s.front()), std::addressof(*s.cbegin()));
    QCOMPARE_EQ(std::addressof(s.front()), std::addressof(s[0]));
    QCOMPARE_EQ(std::addressof(s.back()), std::addressof(*s.rbegin()));
    QCOMPARE_EQ(std::addressof(s.back()), std::addressof(*s.crbegin()));
    QCOMPARE_EQ(std::addressof(s.back()), std::addressof(s[s.size() - 1]));

    // ### more?

    if (expectedSize == 1) {
        // don't run into Mandates: Offset >= Extent
        if constexpr (N > 0) { // incl. N == std::dynamic_extent
            check_empty_span_incl_subspans(s.template subspan<1>());
            RETURN_IF_FAILED();
        }
        check_empty_span_incl_subspans(s.subspan(1));
        RETURN_IF_FAILED();
    } else {
        // don't run into Mandates: Offset >= Extent
        if constexpr (N > 1) { // incl. N == std::dynamic_extent
            check_nonempty_span(s.template subspan<1>(), expectedSize - 1);
            RETURN_IF_FAILED();
        }
        check_nonempty_span(s.subspan(1), expectedSize - 1);
        RETURN_IF_FAILED();
    }
}

template <typename T, std::size_t N>
void tst_QSpan::check_empty_span(QSpan<T, N> s) const
{
    QVERIFY(s.empty());
    QVERIFY(s.isEmpty());

    QCOMPARE_EQ(s.size(), 0);

    QCOMPARE_EQ(s.begin(), s.end());
    QCOMPARE_EQ(s.cbegin(), s.cend());
    QCOMPARE_EQ(s.rbegin(), s.rend());
    QCOMPARE_EQ(s.crbegin(), s.crend());
}

template <typename T, std::size_t N>
void tst_QSpan::check_empty_span_incl_subspans(QSpan<T, N> s) const
{
    check_empty_span(s);
    RETURN_IF_FAILED();

    {
        const auto fi = s.template first<0>();
        check_empty_span(fi);
        RETURN_IF_FAILED();
        QCOMPARE_EQ(fi.data(), s.data());
    }
    {
        const auto la = s.template last<0>();
        check_empty_span(la);
        RETURN_IF_FAILED();
        QCOMPARE_EQ(la.data(), s.data());
    }
    {
        const auto ss = s.template subspan<0>();
        check_empty_span(ss);
        RETURN_IF_FAILED();
        QCOMPARE_EQ(ss.data(), s.data());
    }
    {
        const auto ss = s.template subspan<0, 0>();
        check_empty_span(ss);
        RETURN_IF_FAILED();
        QCOMPARE_EQ(ss.data(), s.data());
    }

    {
        const auto fi = s.first(0);
        check_empty_span(fi);
        RETURN_IF_FAILED();
        QCOMPARE_EQ(fi.data(), s.data());
    }
    {
        const auto la = s.last(0);
        check_empty_span(la);
        RETURN_IF_FAILED();
        QCOMPARE_EQ(la.data(), s.data());
    }
    {
        const auto ss = s.subspan(0);
        check_empty_span(ss);
        RETURN_IF_FAILED();
        QCOMPARE_EQ(ss.data(), s.data());
    }
    {
        const auto ss = s.subspan(0, 0);
        check_empty_span(ss);
        RETURN_IF_FAILED();
        QCOMPARE_EQ(ss.data(), s.data());
    }
}


template<typename T, std::size_t N>
void tst_QSpan::check_null_span(QSpan<T, N> s) const
{
    QCOMPARE_EQ(s.data(), nullptr);
    QCOMPARE_EQ(s.begin(), nullptr);
    QCOMPARE_EQ(s.cbegin(), nullptr);
    QCOMPARE_EQ(s.end(), nullptr);
    check_empty_span_incl_subspans(s);
}

template <std::size_t ExpectedExtent, typename C>
void tst_QSpan::from_container_impl(C &&c) const
{
    const auto c_size = qsizetype(QSpanPrivate::adl_size(c));
    const auto c_data = QSpanPrivate::adl_data(c);
    {
        QSpan si = c; // CTAD
        static_assert(std::is_same_v<decltype(si), QSpan<int, ExpectedExtent>>);

        QCOMPARE_EQ(si.size(), c_size);
        QCOMPARE_EQ(si.data(), c_data);

        check_nonempty_span(si, c_size);
        RETURN_IF_FAILED();

        QSpan<const int> sci = c;

        QCOMPARE_EQ(sci.size(), c_size);
        QCOMPARE_EQ(sci.data(), c_data);

        check_nonempty_span(sci, c_size);
        RETURN_IF_FAILED();
    }
    {
        QSpan sci = std::as_const(c); // CTAD
        static_assert(std::is_same_v<decltype(sci), QSpan<const int, ExpectedExtent>>);

        QCOMPARE_EQ(sci.size(), c_size);
        QCOMPARE_EQ(sci.data(), c_data);

        check_nonempty_span(sci, c_size);
        RETURN_IF_FAILED();
    }
}

template <typename C>
void tst_QSpan::from_variable_size_container_impl(C &&c) const
{
    constexpr auto E = q20::dynamic_extent;
    from_container_impl<E>(std::forward<C>(c));
}

void tst_QSpan::fromArray() const
{
    int ai[] = {42, 84, 168, 336};
    from_container_impl<4>(ai);
}

void tst_QSpan::fromStdArray() const
{
    std::array<int, 4> ai = {42, 84, 168, 336};
    from_container_impl<4>(ai);
}

void tst_QSpan::fromZeroSizeStdArray() const
{
    std::array<int, 0> ai = {};
    QSpan si = ai; // CTAD
    static_assert(std::is_same_v<decltype(si), QSpan<int, 0>>);
    QCOMPARE_EQ(si.data(), ai.data());

    const std::array<int, 0> cai = {};
    QSpan csi = cai; // CTAD
    static_assert(std::is_same_v<decltype(csi), QSpan<const int, 0>>);
    QCOMPARE_EQ(csi.data(), cai.data());

    std::array<const int, 0> aci = {};
    QSpan sci = aci; // CTAD
    static_assert(std::is_same_v<decltype(sci), QSpan<const int, 0>>);
    QCOMPARE_EQ(sci.data(), aci.data());

    std::array<const int, 0> caci = {};
    QSpan csci = caci; // CTAD
    static_assert(std::is_same_v<decltype(csci), QSpan<const int, 0>>);
    QCOMPARE_EQ(csci.data(), caci.data());
}

void tst_QSpan::fromStdVector() const
{
    std::vector<int> vi = {42, 84, 168, 336};
    from_variable_size_container_impl(vi);
}

void tst_QSpan::fromQList() const
{
    QList<int> li = {42, 84, 168, 336};
    from_variable_size_container_impl(li);
}

#undef RETURN_IF_FAILED

QTEST_APPLESS_MAIN(tst_QSpan);
#include "tst_qspan.moc"
