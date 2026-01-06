// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QRANGEMODELADAPTER_IMPL_H
#define QRANGEMODELADAPTER_IMPL_H

#ifndef Q_QDOC

#ifndef QRANGEMODELADAPTER_H
#error Do not include qrangemodeladapter_impl.h directly
#endif

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#include <QtCore/qrangemodel.h>
#include <QtCore/qspan.h>
#include <set>
#include <memory>

QT_BEGIN_NAMESPACE

namespace QRangeModelDetails
{
template <typename Range, typename Protocol>
using RangeImplementation = std::conditional_t<std::is_void_v<Protocol>,
                                std::conditional_t<is_tree_range<Range>::value,
                                    QGenericTreeItemModelImpl<Range, DefaultTreeProtocol<Range>>,
                                    QGenericTableItemModelImpl<Range>
                                >,
                                QGenericTreeItemModelImpl<Range, Protocol>
                            >;

template <typename ...Args>
using construct_rangeModel_test = decltype(QRangeModel(std::declval<Args &&>()...));

template <typename ...Args>
static constexpr bool can_construct_rangeModel = qxp::is_detected_v<construct_rangeModel_test,
                                                                    Args...>;

template <typename ...Args>
using if_can_construct = std::enable_if_t<can_construct_rangeModel<Args...>, bool>;

// we can't use wrapped_t, we only want to unpack smart pointers, and maintain
// the pointer nature of the type.
template <typename T, typename = void>
struct data_type { using type = T; };
template <>
struct data_type<void> { using type = QVariant; };

// pointer types of iterators use QtPrivate::ArrowProxy if the type does not
// provide operator->() (or is a pointer).
template <typename T, typename = void> struct test_pointerAccess : std::false_type {};
template <typename T> struct test_pointerAccess<T *> : std::true_type {};
template <typename T>
struct test_pointerAccess<T, std::void_t<decltype(std::declval<T>().operator->())>>
    : std::true_type
{};

template <typename T>
using data_pointer_t = std::conditional_t<test_pointerAccess<T>::value,
                                          T, QtPrivate::ArrowProxy<T>>;

// Helpers to make a type const "in depth", taking into account raw pointers
// and wrapping types, like smart pointers and std::reference_wrapper.

// We need to return data by value, not by reference, as we might only have
// temporary values (i.e. a QVariant returned by QAIM::data).
template <typename T, typename = void> struct AsConstData { using type = T; };
template <typename T> struct AsConstData<const T &> { using type = T; };
template <typename T> struct AsConstData<T *> { using type = const T *; };
template <template <typename> typename U, typename T>
struct AsConstData<U<T>, std::enable_if_t<is_any_shared_ptr<U<T>>::value>>
{ using type = U<const T>; };
template <typename T> struct AsConstData<std::reference_wrapper<T>>
{ using type = std::reference_wrapper<const T>; };

template <typename T> using asConst_t = typename AsConstData<T>::type;

// Rows get wrapped into a "view", as a begin/end iterator/sentinel pair.
// The iterator dereferences to the const version of the value returned by
// the underlying iterator.
// Could be replaced with std::views::sub_range in C++ 20.
template <typename const_row_type, typename Iterator, typename Sentinel>
struct RowView
{
    // this is similar to C++23's std::basic_const_iterator, but we don't want
    // to convert to the underlying const_iterator.
    struct iterator
    {
        using value_type = asConst_t<typename Iterator::value_type>;
        using difference_type = typename Iterator::difference_type;
        using pointer = QRangeModelDetails::data_pointer_t<value_type>;
        using reference = value_type;
        using const_reference = value_type;
        using iterator_category = typename std::iterator_traits<Iterator>::iterator_category;

        template <typename I, typename Category>
        static constexpr bool is_atLeast = std::is_base_of_v<Category,
                                               typename std::iterator_traits<I>::iterator_category>;
        template <typename I, typename Category>
        using if_atLeast = std::enable_if_t<is_atLeast<I, Category>, bool>;

        reference operator*() const { return *m_it; }
        pointer operator->() const { return operator*(); }

        // QRM requires at least forward_iterator, so we provide both post- and
        // prefix increment unconditionally
        friend constexpr iterator &operator++(iterator &it)
            noexcept(noexcept(++std::declval<Iterator&>()))
        {
            ++it.m_it;
            return it;
        }
        friend constexpr iterator operator++(iterator &it, int)
            noexcept(noexcept(std::declval<Iterator&>()++))
        {
            iterator copy = it;
            ++copy.m_it;
            return copy;
        }

        template <typename I = Iterator, if_atLeast<I, std::bidirectional_iterator_tag> = true>
        friend constexpr iterator &operator--(iterator &it)
            noexcept(noexcept(--std::declval<I&>()))
        {
            --it.m_it;
            return it;
        }
        template <typename I = Iterator, if_atLeast<I, std::bidirectional_iterator_tag> = true>
        friend constexpr iterator operator--(iterator &it, int)
            noexcept(noexcept(std::declval<I&>()--))
        {
            iterator copy = it;
            --it.m_it;
            return copy;
        }

        template <typename I = Iterator, if_atLeast<I, std::random_access_iterator_tag> = true>
        friend constexpr iterator &operator+=(iterator &it, difference_type n)
            noexcept(noexcept(std::declval<I&>() += 1))
        {
            it.m_it += n;
            return it;
        }
        template <typename I = Iterator, if_atLeast<I, std::random_access_iterator_tag> = true>
        friend constexpr iterator &operator-=(iterator &it, difference_type n)
            noexcept(noexcept(std::declval<I&>() -= 1))
        {
            it.m_it -= n;
            return it;
        }

        template <typename I = Iterator, if_atLeast<I, std::random_access_iterator_tag> = true>
        friend constexpr iterator operator+(const iterator &it, difference_type n)
            noexcept(noexcept(std::declval<I&>() + 1))
        {
            iterator copy = it;
            copy.m_it += n;
            return copy;
        }
        template <typename I = Iterator, if_atLeast<I, std::random_access_iterator_tag> = true>
        friend constexpr iterator operator+(difference_type n, const iterator &it)
            noexcept(noexcept(1 + std::declval<I&>()))
        {
            return it + n;
        }
        template <typename I = Iterator, if_atLeast<I, std::random_access_iterator_tag> = true>
        friend constexpr iterator operator-(const iterator &it, difference_type n)
            noexcept(noexcept(std::declval<I&>() - 1))
        {
            iterator copy = it;
            copy.m_it = it.m_it - n;
            return copy;
        }

        template <typename I = Iterator, if_atLeast<I, std::random_access_iterator_tag> = true>
        constexpr reference operator[](difference_type n) const
            noexcept(noexcept(I::operator[]()))
        {
            return m_it[n];
        }

        template <typename I = Iterator, if_atLeast<I, std::random_access_iterator_tag> = true>
        friend constexpr difference_type operator-(const iterator &lhs, const iterator &rhs)
            noexcept(noexcept(std::declval<I&>() - std::declval<I&>()))
        {
            return lhs.m_it - rhs.m_it;
        }

        template <typename I = Iterator, if_atLeast<I, std::random_access_iterator_tag> = true>
        friend constexpr bool operator<(const iterator &lhs, const iterator &rhs)
            noexcept(noexcept(std::declval<I&>() < std::declval<I&>()))
        {
            return lhs.m_it < rhs.m_it;
        }
        template <typename I = Iterator, if_atLeast<I, std::random_access_iterator_tag> = true>
        friend constexpr bool operator<=(const iterator &lhs, const iterator &rhs)
            noexcept(noexcept(std::declval<I&>() <= std::declval<I&>()))
        {
            return lhs.m_it <= rhs.m_it;
        }

        template <typename I = Iterator, if_atLeast<I, std::random_access_iterator_tag> = true>
        friend constexpr bool operator>(const iterator &lhs, const iterator &rhs)
            noexcept(noexcept(std::declval<I&>() > std::declval<I&>()))
        {
            return lhs.m_it > rhs.m_it;
        }
        template <typename I = Iterator, if_atLeast<I, std::random_access_iterator_tag> = true>
        friend constexpr bool operator>=(const iterator &lhs, const iterator &rhs)
            noexcept(noexcept(std::declval<I&>() >= std::declval<I&>()))
        {
            return lhs.m_it >= rhs.m_it;
        }

        // This would implement the P2836R1 fix from std::basic_const_iterator,
        // but a const_iterator on a range<pointer> would again allow us to
        // mutate the pointed-to object, which is exactly what we want to
        // prevent.
        /*
        template <typename CI, std::enable_if_t<std::is_convertible_v<const Iterator &, CI>, bool> = true>
        operator CI() const
        {
            return CI{m_it};
        }

        template <typename CI, std::enable_if_t<std::is_convertible_v<Iterator, CI>, bool> = true>
        operator CI() &&
        {
            return CI{std::move(m_it)};
        }
        */

        friend bool comparesEqual(const iterator &lhs, const iterator &rhs) noexcept
        {
            return lhs.m_it == rhs.m_it;
        }
        Q_DECLARE_EQUALITY_COMPARABLE(iterator)

        Iterator m_it;
    };

    using value_type = typename iterator::value_type;
    using difference_type = typename iterator::difference_type;

    friend bool comparesEqual(const RowView &lhs, const RowView &rhs) noexcept
    {
        return lhs.m_begin == rhs.m_begin;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(RowView)

    template <typename RHS>
    bool operator==(const RHS &rhs) const noexcept
    {
        return m_begin == QRangeModelDetails::begin(rhs);
    }
    template <typename RHS>
    bool operator!=(const RHS &rhs) const noexcept
    {
        return !operator==(rhs);
    }

    value_type at(difference_type n) const { return *std::next(m_begin, n); }

    iterator begin() const { return iterator{m_begin}; }
    iterator end() const { return iterator{m_end}; }

    Iterator m_begin;
    Sentinel m_end;
};

// Const-in-depth mapping for row types. We do store row types, and they might
// be move-only, so we return them by const reference.
template <typename T, typename = void> struct AsConstRow { using type = const T &; };
// Otherwise the mapping for basic row types is the same as for data.
template <typename T> struct AsConstRow<T *> : AsConstData<T *> {};
template <template <typename> typename U, typename T>
struct AsConstRow<U<T>, std::enable_if_t<is_any_shared_ptr<U<T>>::value>> : AsConstData<U<T>> {};
template <typename T> struct AsConstRow<std::reference_wrapper<T>>
    : AsConstData<std::reference_wrapper<T>> {};

template <typename T> using if_range = std::enable_if_t<is_range_v<T>, bool>;
// If the row type is a range, then we assume that the first type is the
// element type.
template <template <typename, typename ...> typename R, typename T, typename ...Args>
struct AsConstRow<R<T, Args...>, if_range<R<T, Args...>>>
{
    using type = const R<T, Args...> &;
};

// specialize for range of pointers and smart pointers
template <template <typename, typename ...> typename R, typename T, typename ...Args>
struct AsConstRow<R<T *, Args...>, if_range<R<T *, Args...>>>
{
    using row_type = R<T, Args...>;
    using const_iterator = typename row_type::const_iterator;
    using const_row_type = R<asConst_t<T>>;
    using type = RowView<const_row_type, const_iterator, const_iterator>;
};

template <template <typename, typename ...> typename R, typename T, typename ...Args>
struct AsConstRow<R<T, Args...>,
    std::enable_if_t<std::conjunction_v<is_range<R<T, Args...>>, is_any_shared_ptr<T>>>
>
{
    using row_type = R<T, Args...>;
    using const_iterator = typename row_type::const_iterator;
    using const_row_type = R<asConst_t<T>>;
    using type = RowView<const_row_type, const_iterator, const_iterator>;
};

template <typename T>
using asConstRow_t = typename AsConstRow<T>::type;

Q_CORE_EXPORT QVariant qVariantAtIndex(const QModelIndex &index);

template <typename Type>
static inline Type dataAtIndex(const QModelIndex &index)
{
    Q_ASSERT_X(index.isValid(), "QRangeModelAdapter::dataAtIndex", "Index at position is invalid");
    QVariant variant = qVariantAtIndex(index);

    if constexpr (std::is_same_v<QVariant, Type>)
        return variant;
    else
        return variant.value<Type>();
}

template <typename Type>
static inline Type dataAtIndex(const QModelIndex &index, int role)
{
    Q_ASSERT_X(index.isValid(), "QRangeModelAdapter::dataAtIndex", "Index at position is invalid");
    QVariant variant = index.data(role);

    if constexpr (std::is_same_v<QVariant, Type>)
        return variant;
    else
        return variant.value<Type>();
}

template <bool isTree = false>
struct ParentIndex
{
    ParentIndex(const QModelIndex &dummy = {}) { Q_ASSERT(!dummy.isValid()); }
    QModelIndex root() const { return {}; }
};

template <>
struct ParentIndex<true>
{
    const QModelIndex m_rootIndex;
    QModelIndex root() const { return m_rootIndex; }
};

template <typename Model, typename Impl>
struct AdapterStorage : ParentIndex<Impl::protocol_traits::is_tree>
{
    // If it is, then we can shortcut the model and operate on the container.
    // Otherwise we have to go through the model's vtable. For now, this is always
    // the case.
    static constexpr bool isRangeModel = std::is_same_v<Model, QRangeModel>;
    static_assert(isRangeModel, "The model must be a QRangeModel (not a subclass).");
    std::shared_ptr<QRangeModel> m_model;

    template <typename I = Impl, std::enable_if_t<I::protocol_traits::is_tree, bool> = true>
    explicit AdapterStorage(const std::shared_ptr<QRangeModel> &model, const QModelIndex &root)
        : ParentIndex<Impl::protocol_traits::is_tree>{root}, m_model(model)
    {
    }

    explicit AdapterStorage(Model *model)
        : m_model{model}
    {}

    const Impl *implementation() const
    {
        return static_cast<const Impl *>(QRangeModelImplBase::getImplementation(m_model.get()));
    }

    Impl *implementation()
    {
        return static_cast<Impl *>(QRangeModelImplBase::getImplementation(m_model.get()));
    }

    auto *operator->()
    {
        if constexpr (isRangeModel)
            return implementation();
        else
            return m_model.get();
    }

    const auto *operator->() const
    {
        if constexpr (isRangeModel)
            return implementation();
        else
            return m_model.get();
    }
};

} // QRangeModelDetails

QT_END_NAMESPACE

#endif // Q_QDOC

#endif // QRANGEMODELADAPTER_IMPL_H
