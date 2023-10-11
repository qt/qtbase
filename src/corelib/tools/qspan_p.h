// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSPAN_P_H
#define QSPAN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qtypes.h>

#include <array>
#include <cstddef>
#include <cassert>
#include <QtCore/q20iterator.h>
#include <QtCore/q20memory.h>
#ifdef __cpp_lib_span
#include <span>
#endif
#include <QtCore/q20type_traits.h>

QT_BEGIN_NAMESPACE

// like std::dynamic_extent
namespace q20 {
    inline constexpr std::size_t dynamic_extent = -1;
} // namespace q20

template <typename T, std::size_t E = q20::dynamic_extent> class QSpan;

namespace QSpanPrivate {

template <typename T, std::size_t E> class QSpanBase;

template <typename T>
struct is_qspan_helper : std::false_type {};
template <typename T, std::size_t E>
struct is_qspan_helper<QSpan<T, E>> : std::true_type {};
template <typename T, std::size_t E>
struct is_qspan_helper<QSpanBase<T, E>> : std::true_type {};
template <typename T>
using is_qspan = is_qspan_helper<q20::remove_cvref_t<T>>;

template <typename T>
struct is_std_array_helper : std::false_type {};
template <typename T, std::size_t N>
struct is_std_array_helper<std::array<T, N>> : std::true_type {};
template <typename T>
using is_std_array = is_std_array_helper<q20::remove_cvref_t<T>>;

template <typename From, typename To>
using is_qualification_conversion =
    std::is_convertible<From(*)[], To(*)[]>; // https://eel.is/c++draft/span.cons#note-1
template <typename From, typename To>
constexpr inline bool is_qualification_conversion_v = is_qualification_conversion<From, To>::value;

// Replacements for std::ranges::XXX(), but only bringing in ADL XXX()s,
// not doing the extra work C++20 requires
template <typename Range>
decltype(auto) adl_begin(Range &&r) { using std::begin; return begin(r); }
template <typename Range>
decltype(auto) adl_data(Range &&r)  { using std::data; return data(r); }
template <typename Range>
decltype(auto) adl_size(Range &&r)  { using std::size; return size(r); }

// Replacement for std::ranges::iterator_t (which depends on C++20 std::ranges::begin)
// This one uses adl_begin() instead.
template <typename Range>
using iterator_t = decltype(QSpanPrivate::adl_begin(std::declval<Range&>()));
template <typename Range>
using range_reference_t = q20::iter_reference_t<QSpanPrivate::iterator_t<Range>>;

template <typename T>
class QSpanCommon {
protected:
    template <typename Iterator>
    using is_compatible_iterator = std::conjunction<
            std::is_base_of<
                std::random_access_iterator_tag,
                typename std::iterator_traits<Iterator>::iterator_category
            >,
            is_qualification_conversion<
                std::remove_reference_t<q20::iter_reference_t<Iterator>>,
                T
            >
        >;
    template <typename Iterator, typename End>
    using is_compatible_iterator_and_sentinel = std::conjunction<
            is_compatible_iterator<Iterator>,
            std::negation<std::is_convertible<End, std::size_t>>
        >;
    template <typename Range, typename = void> // wrap use of SFINAE-unfriendly iterator_t:
    struct is_compatible_range_helper : std::false_type {};
    template <typename Range>
    struct is_compatible_range_helper<Range, std::void_t<QSpanPrivate::iterator_t<Range>>>
        : is_compatible_iterator<QSpanPrivate::iterator_t<Range>> {};
    template <typename Range>
    using is_compatible_range = std::conjunction<
            // ### this needs more work, esp. extension to C++20 contiguous iterators
            std::negation<is_qspan<Range>>,
            std::negation<is_std_array<Range>>,
            std::negation<std::is_array<q20::remove_cvref_t<Range>>>,
            is_compatible_range_helper<Range>
        >;

    // constraints
    template <typename Iterator>
    using if_compatible_iterator = std::enable_if_t<
                is_compatible_iterator<Iterator>::value
            , bool>;
    template <typename Iterator, typename End>
    using if_compatible_iterator_and_sentinel = std::enable_if_t<
                is_compatible_iterator_and_sentinel<Iterator, End>::value
            , bool>;
    template <typename Range>
    using if_compatible_range = std::enable_if_t<is_compatible_range<Range>::value, bool>;
}; // class QSpanCommon

template <typename T, std::size_t E>
class QSpanBase : protected QSpanCommon<T>
{
    static_assert(E < size_t{(std::numeric_limits<qsizetype>::max)()},
                  "QSpan only supports extents that fit into the signed size type (qsizetype).");

    struct Enabled_t { explicit Enabled_t() = default; };
    static inline constexpr Enabled_t Enable{};

    template <typename S, std::size_t N>
    using if_compatible_array = std::enable_if_t<
            N == E && is_qualification_conversion_v<S, T>
        , bool>;

    template <typename S>
    using if_qualification_conversion = std::enable_if_t<
            is_qualification_conversion_v<S, T>
        , bool>;
protected:
    using Base = QSpanCommon<T>;

    // data members:
    T *m_data;
    static constexpr qsizetype m_size = qsizetype(E);

    // types and constants:
    // (in QSpan only)

    // constructors (need to be public d/t the way ctor inheriting works):
public:
    template <std::size_t E2 = E, std::enable_if_t<E2 == 0, bool> = true>
    Q_IMPLICIT constexpr QSpanBase() noexcept : m_data{nullptr} {}

    template <typename It, typename Base::template if_compatible_iterator<It> = true>
    explicit constexpr QSpanBase(It first, qsizetype count)
        : m_data{q20::to_address(first)}
    {
        Q_ASSERT(count == m_size);
    }

    template <typename It, typename End, typename Base::template if_compatible_iterator_and_sentinel<It, End> = true>
    explicit constexpr QSpanBase(It first, End last)
        : QSpanBase(first, last - first) {}

    template <size_t N, std::enable_if_t<N == E, bool> = true>
    Q_IMPLICIT constexpr QSpanBase(q20::type_identity_t<T> (&arr)[N]) noexcept
        : QSpanBase(arr, N) {}

    template <typename S, size_t N, if_compatible_array<S, N> = true>
    Q_IMPLICIT constexpr QSpanBase(std::array<S, N> &arr) noexcept
        : QSpanBase(arr.data(), N) {}

    template <typename S, size_t N, if_compatible_array<S, N> = true>
    Q_IMPLICIT constexpr QSpanBase(const std::array<S, N> &arr) noexcept
        : QSpanBase(arr.data(), N) {}

    template <typename Range, typename Base::template if_compatible_range<Range> = true>
    Q_IMPLICIT constexpr QSpanBase(Range &&r)
        : QSpanBase(QSpanPrivate::adl_data(r),  // no forward<>() here (std doesn't have it, either)
                    qsizetype(QSpanPrivate::adl_size(r))) // ditto, no forward<>()
    {}

    template <typename S, if_qualification_conversion<S> = true>
    Q_IMPLICIT constexpr QSpanBase(QSpan<S, E> other) noexcept
        : QSpanBase(other.data(), other.size())
    {}

    template <typename S, if_qualification_conversion<S> = true>
    Q_IMPLICIT constexpr QSpanBase(QSpan<S> other)
        : QSpanBase(other.data(), other.size())
    {}

}; // class QSpanBase (fixed extent)

template <typename T>
class QSpanBase<T, q20::dynamic_extent> : protected QSpanCommon<T>
{
    template <typename S>
    using if_qualification_conversion = std::enable_if_t<
            is_qualification_conversion_v<S, T>
        , bool>;
protected:
    using Base = QSpanCommon<T>;

    // data members:
    T *m_data;
    qsizetype m_size;

    // constructors (need to be public d/t the way ctor inheriting works):
public:
    Q_IMPLICIT constexpr QSpanBase() noexcept : m_data{nullptr}, m_size{0} {}

    template <typename It, typename Base::template if_compatible_iterator<It> = true>
    Q_IMPLICIT constexpr QSpanBase(It first, qsizetype count)
        : m_data{q20::to_address(first)}, m_size{count} {}

    template <typename It, typename End, typename Base::template if_compatible_iterator_and_sentinel<It, End> = true>
    Q_IMPLICIT constexpr QSpanBase(It first, End last)
        : QSpanBase(first, last - first) {}

    template <size_t N>
    Q_IMPLICIT constexpr QSpanBase(q20::type_identity_t<T> (&arr)[N]) noexcept
        : QSpanBase(arr, N) {}

    template <typename S, size_t N, if_qualification_conversion<S> = true>
    Q_IMPLICIT constexpr QSpanBase(std::array<S, N> &arr) noexcept
        : QSpanBase(arr.data(), N) {}

    template <typename S, size_t N, if_qualification_conversion<S> = true>
    Q_IMPLICIT constexpr QSpanBase(const std::array<S, N> &arr) noexcept
        : QSpanBase(arr.data(), N) {}

    template <typename Range, typename Base::template if_compatible_range<Range> = true>
    Q_IMPLICIT constexpr QSpanBase(Range &&r)
        : QSpanBase(QSpanPrivate::adl_data(r),  // no forward<>() here (std doesn't have it, either)
                    qsizetype(QSpanPrivate::adl_size(r))) // ditto, no forward<>()
    {}

    template <typename S, size_t N, if_qualification_conversion<S> = true>
    Q_IMPLICIT constexpr QSpanBase(QSpan<S, N> other) noexcept
        : QSpanBase(other.data(), other.size())
    {}
}; // class QSpanBase (dynamic extent)

} // namespace QSpanPrivate

template <typename T, std::size_t E>
class QSpan : private QSpanPrivate::QSpanBase<T, E>
{
    using Base = QSpanPrivate::QSpanBase<T, E>;
    Q_ALWAYS_INLINE constexpr void verify([[maybe_unused]] qsizetype pos = 0,
                                          [[maybe_unused]] qsizetype n = 1) const
    {
        Q_ASSERT(pos >= 0);
        Q_ASSERT(pos <= size());
        Q_ASSERT(n >= 0);
        Q_ASSERT(n <= size() - pos);
    }

    template <std::size_t N>
    static constexpr bool subspan_always_succeeds_v = N <= E && E != q20::dynamic_extent;
public:
    // constants and types
    using element_type = T;
    using value_type = std::remove_cv_t<element_type>;
    using size_type = qsizetype;               // difference to std::span
    using difference_type = qptrdiff;          // difference to std::span
    using pointer = element_type*;
    using const_pointer = const element_type*;
    using reference = element_type&;
    using const_reference = const element_type&;
    using iterator = pointer;                  // implementation-defined choice
    using const_iterator = const_pointer;      // implementation-defined choice
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    static constexpr size_type extent = E;

    // [span.cons], constructors, copy, and assignment
    using Base::Base;

    // [span.obs]
    [[nodiscard]] constexpr size_type size() const noexcept { return this->m_size; }
    [[nodiscard]] constexpr size_type size_bytes() const noexcept { return size() * sizeof(T); }
    [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

    // [span.elem]
    [[nodiscard]] constexpr reference operator[](size_type idx) const
    { verify(idx); return data()[idx]; }
    [[nodiscard]] constexpr reference front() const { verify(); return *data(); }
    [[nodiscard]] constexpr reference back() const  { verify(); return data()[size() - 1]; }
    [[nodiscard]] constexpr pointer data() const noexcept { return this->m_data; }

    // [span.iterators]
    [[nodiscard]] constexpr auto begin() const noexcept { return data(); }
    [[nodiscard]] constexpr auto end() const noexcept { return data() + size(); }
    [[nodiscard]] constexpr auto cbegin() const noexcept { return const_iterator{begin()}; }
    [[nodiscard]] constexpr auto cend() const noexcept { return const_iterator{end()}; }
    [[nodiscard]] constexpr auto rbegin() const noexcept { return reverse_iterator{end()}; }
    [[nodiscard]] constexpr auto rend() const noexcept { return reverse_iterator{begin()}; }
    [[nodiscard]] constexpr auto crbegin() const noexcept { return const_reverse_iterator{end()}; }
    [[nodiscard]] constexpr auto crend() const noexcept { return const_reverse_iterator{begin()}; }

    // [span.sub]
    template <std::size_t Count>
    [[nodiscard]] constexpr QSpan<T, Count> first() const
        noexcept(subspan_always_succeeds_v<Count>)
    {
        static_assert(Count <= E,
                      "Count cannot be larger than the span's extent.");
        verify(0, Count);
        return QSpan<T, Count>{data(), Count};
    }

    template <std::size_t Count>
    [[nodiscard]] constexpr QSpan<T, Count> last() const
        noexcept(subspan_always_succeeds_v<Count>)
    {
        static_assert(Count <= E,
                      "Count cannot be larger than the span's extent.");
        verify(0, Count);
        return QSpan<T, Count>{data() + (size() - Count), Count};
    }

    template <std::size_t Offset>
    [[nodiscard]] constexpr auto subspan() const
        noexcept(subspan_always_succeeds_v<Offset>)
    {
        static_assert(Offset <= E,
                      "Offset cannot be larger than the span's extent.");
        verify(Offset, 0);
        if constexpr (E == q20::dynamic_extent)
            return QSpan<T>{data() + Offset, qsizetype(size() - Offset)};
        else
            return QSpan<T, E - Offset>{data() + Offset, qsizetype(E - Offset)};
    }

    template <std::size_t Offset, std::size_t Count>
    [[nodiscard]] constexpr auto subspan() const
        noexcept(subspan_always_succeeds_v<Offset + Count>)
    { return subspan<Offset>().template first<Count>(); }

    [[nodiscard]] constexpr QSpan<T> first(size_type n) const { verify(0, n); return {data(), n}; }
    [[nodiscard]] constexpr QSpan<T> last(size_type n)  const { verify(0, n); return {data() + (size() - n), n}; }
    [[nodiscard]] constexpr QSpan<T> subspan(size_type pos) const { verify(pos, 0); return {data() + pos, size() - pos}; }
    [[nodiscard]] constexpr QSpan<T> subspan(size_type pos, size_type n) const { return subspan(pos).first(n); }

    // Qt-compatibility API:
    [[nodiscard]] bool isEmpty() const noexcept { return empty(); }
    // nullary first()/last() clash with first<>() and last<>(), so they're not provided for QSpan
    [[nodiscard]] constexpr QSpan<T> sliced(size_type pos) const { return subspan(pos); }
    [[nodiscard]] constexpr QSpan<T> sliced(size_type pos, size_type n) const { return subspan(pos, n); }

}; // class QSpan

// [span.deduct]
template <class It, class EndOrSize>
QSpan(It, EndOrSize) -> QSpan<std::remove_reference_t<q20::iter_reference_t<It>>>;
template <class T, std::size_t N>
QSpan(T (&)[N]) -> QSpan<T, N>;
template <class T, std::size_t N>
QSpan(std::array<T, N> &) -> QSpan<T, N>;
template <class T, std::size_t N>
QSpan(const std::array<T, N> &) -> QSpan<const T, N>;
template <class R>
QSpan(R&&) -> QSpan<std::remove_reference_t<QSpanPrivate::range_reference_t<R>>>;

QT_END_NAMESPACE

#endif // QSPAN_P_H
