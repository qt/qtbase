// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QRANGEMODEL_IMPL_H
#define QRANGEMODEL_IMPL_H

#ifndef Q_QDOC

#ifndef QRANGEMODEL_H
#error Do not include qrangemodel_impl.h directly
#endif

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qvariant.h>
#include <QtCore/qmap.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <type_traits>
#include <QtCore/q20type_traits.h>
#include <tuple>
#include <QtCore/q23utility.h>

QT_BEGIN_NAMESPACE

namespace QRangeModelDetails
{
    template <typename T, template <typename...> typename... Templates>
    struct is_any_of_impl : std::false_type {};

    template <template <typename...> typename Template,
              typename... Params,
              template <typename...> typename... Templates>
    struct is_any_of_impl<Template<Params...>, Template, Templates...> : std::true_type {};

    template <typename T,
              template <typename...> typename Template,
              template <typename...> typename... Templates>
    struct is_any_of_impl<T, Template, Templates...> : is_any_of_impl<T, Templates...> {};

    template <typename T, template <typename...> typename... Templates>
    using is_any_of = is_any_of_impl<std::remove_cv_t<T>, Templates...>;

    template <typename T>
    using is_validatable = std::is_constructible<bool, T>;

    template <typename T, typename = void>
    struct is_smart_ptr : std::false_type {};

    template <typename T>
    struct is_smart_ptr<T,
        std::enable_if_t<std::conjunction_v<
                std::is_pointer<decltype(std::declval<T&>().get())>,
                std::is_same<decltype(*std::declval<T&>().get()), decltype(*std::declval<T&>())>,
                is_validatable<T>
            >>>
        : std::true_type
    {};

    // TODO: shouldn't we check is_smart_ptr && !is_copy_constructible && !is_copy_assignable
    //       to support users-specific ptrs?
    template <typename T>
    using is_any_unique_ptr = is_any_of<T,
#ifndef QT_NO_SCOPED_POINTER
            QScopedPointer,
#endif
            std::unique_ptr
        >;

    template <typename T>
    using is_any_shared_ptr = is_any_of<T, std::shared_ptr, QSharedPointer,
                                           QExplicitlySharedDataPointer, QSharedDataPointer>;

    template <typename T>
    using is_owning_or_raw_pointer = std::disjunction<is_any_shared_ptr<T>, is_any_unique_ptr<T>,
                                                      std::is_pointer<T>>;

    template <typename T>
    static auto pointerTo(T&& t) {
        using Type = q20::remove_cvref_t<T>;
        if constexpr (is_any_of<Type, std::optional>())
            return t ? std::addressof(*std::forward<T>(t)) : nullptr;
        else if constexpr (std::is_pointer<Type>())
            return t;
        else if constexpr (is_smart_ptr<Type>())
            return t.get();
        else if constexpr (is_any_of<Type, std::reference_wrapper>())
            return std::addressof(t.get());
        else
            return std::addressof(std::forward<T>(t));
    }

    template <typename T>
    using wrapped_t = std::remove_pointer_t<decltype(pointerTo(std::declval<T&>()))>;

    template <typename T>
    using is_wrapped = std::negation<std::is_same<wrapped_t<T>, std::remove_reference_t<T>>>;

    template <typename T, typename = void>
    struct tuple_like : std::false_type {};
    template <typename T>
    struct tuple_like<T, std::void_t<std::tuple_element_t<0, wrapped_t<T>>>> : std::true_type {};
    template <typename T>
    [[maybe_unused]] static constexpr bool tuple_like_v = tuple_like<T>::value;

    template <typename T, typename = void>
    struct has_metaobject : std::false_type {};
    template <typename T>
    struct has_metaobject<T, std::void_t<decltype(wrapped_t<T>::staticMetaObject)>>
        : std::true_type {};
    template <typename T>
    [[maybe_unused]] static constexpr bool has_metaobject_v = has_metaobject<T>::value;

    template <typename T>
    static constexpr bool isValid(const T &t) noexcept
    {
        if constexpr (std::is_array_v<T>)
            return true;
        else if constexpr (is_validatable<T>())
            return bool(t);
        else
            return true;
    }

    template <typename T>
    static decltype(auto) refTo(T&& t) {
        Q_ASSERT(isValid(t));
        // it's allowed to move only if the object holds unique ownership of the wrapped data
        using Type = q20::remove_cvref_t<T>;
        if constexpr (is_any_of<T, std::optional>())
            return *std::forward<T>(t); // let std::optional resolve dereferencing
        if constexpr (!is_wrapped<Type>() || is_any_unique_ptr<Type>())
            return q23::forward_like<T>(*pointerTo(t));
        else
            return *pointerTo(t);
    }

    template <typename It>
    auto key(It&& it) -> decltype(it.key()) { return std::forward<It>(it).key(); }
    template <typename It>
    auto key(It&& it) -> decltype((it->first)) { return std::forward<It>(it)->first; }

    template <typename It>
    auto value(It&& it) -> decltype(it.value()) { return std::forward<It>(it).value(); }
    template <typename It>
    auto value(It&& it) -> decltype((it->second)) { return std::forward<It>(it)->second; }

    // use our own version of begin/end so that we can overload for pointers
    template <typename C>
    static auto begin(C &&c) -> decltype(std::begin(refTo(std::forward<C>(c))))
    { return std::begin(refTo(std::forward<C>(c))); }
    template <typename C>
    static auto end(C &&c) -> decltype(std::end(refTo(std::forward<C>(c))))
    { return std::end(refTo(std::forward<C>(c))); }
    template <typename C>
    static auto cbegin(C &&c) -> decltype(std::cbegin(refTo(std::forward<C>(c))))
    { return std::cbegin(refTo(std::forward<C>(c))); }
    template <typename C>
    static auto cend(C &&c) -> decltype(std::cend(refTo(std::forward<C>(c))))
    { return std::cend(refTo(std::forward<C>(c))); }
    template <typename C>
    static auto pos(C &&c, int i)
    { return std::next(QRangeModelDetails::begin(std::forward<C>(c)), i); }
    template <typename C>
    static auto cpos(C &&c, int i)
    { return std::next(QRangeModelDetails::cbegin(std::forward<C>(c)), i); }

    // Test if a type is a range, and whether we can modify it using the
    // standard C++ container member functions insert, erase, and resize.
    // For the sake of QAIM, we cannot modify a range if it holds const data
    // even if the range itself is not const; we'd need to initialize new rows
    // and columns, and move old row and column data.
    template <typename C, typename = void>
    struct test_insert : std::false_type {};

    template <typename C>
    struct test_insert<C, std::void_t<decltype(std::declval<C>().insert(
        std::declval<typename C::const_iterator>(),
        std::declval<typename C::size_type>(),
        std::declval<typename C::value_type>()
    ))>>
        : std::true_type
    {};

    // Can we insert from another (identical) range? Required to support
    // move-only types
    template <typename C, typename = void>
    struct test_insert_range : std::false_type {};

    template <typename C>
    struct test_insert_range<C, std::void_t<decltype(std::declval<C&>().insert(
      std::declval<typename C::const_iterator&>(),
      std::declval<std::move_iterator<typename C::iterator>&>(),
      std::declval<std::move_iterator<typename C::iterator>&>()
    ))>>
        : std::true_type
    {};

    template <typename C, typename = void>
    struct test_erase : std::false_type {};

    template <typename C>
    struct test_erase<C, std::void_t<decltype(std::declval<C>().erase(
        std::declval<typename C::const_iterator>(),
        std::declval<typename C::const_iterator>()
    ))>>
        : std::true_type
    {};

    template <typename C, typename = void>
    struct test_resize : std::false_type {};

    template <typename C>
    struct test_resize<C, std::void_t<decltype(std::declval<C>().resize(
        std::declval<typename C::size_type>(),
        std::declval<typename C::value_type>()
    ))>>
        : std::true_type
    {};

    // Test if a type is an associative container that we can use for multi-role
    // data, i.e. has a key_type and a mapped_type typedef, and maps from int,
    // Qt::ItemDataRole, or QString to QVariant. This excludes std::set (and
    // unordered_set), which are not useful for us anyway even though they are
    // considered associative containers.
    template <typename C, typename = void> struct is_multi_role : std::false_type
    {
        static constexpr bool int_key = false;
    };
    template <typename C> // Qt::ItemDataRole -> QVariant, or QString -> QVariant, int -> QVariant
    struct is_multi_role<C, std::void_t<typename C::key_type, typename C::mapped_type>>
        : std::conjunction<std::disjunction<std::is_same<typename C::key_type, int>,
                                            std::is_same<typename C::key_type, Qt::ItemDataRole>,
                                            std::is_same<typename C::key_type, QString>>,
                           std::is_same<typename C::mapped_type, QVariant>>
    {
        static constexpr bool int_key = !std::is_same_v<typename C::key_type, QString>;
    };
    template <typename C>
    [[maybe_unused]]
    static constexpr bool is_multi_role_v = is_multi_role<C>::value;

    template <typename C, typename = void>
    struct test_size : std::false_type {};
    template <typename C>
    struct test_size<C, std::void_t<decltype(std::size(std::declval<C&>()))>> : std::true_type {};

    template <typename C, typename = void>
    struct range_traits : std::false_type {
        static constexpr bool is_mutable = !std::is_const_v<C>;
        static constexpr bool has_insert = false;
        static constexpr bool has_insert_range = false;
        static constexpr bool has_erase = false;
        static constexpr bool has_resize = false;
    };
    template <typename C>
    struct range_traits<C, std::void_t<decltype(cbegin(std::declval<C&>())),
                                       decltype(cend(std::declval<C&>())),
                                       std::enable_if_t<!is_multi_role_v<C>>
                                      >> : std::true_type
    {
        using iterator = decltype(begin(std::declval<C&>()));
        using value_type = std::remove_reference_t<decltype(*std::declval<iterator&>())>;
        static constexpr bool is_mutable = !std::is_const_v<C> && !std::is_const_v<value_type>;
        static constexpr bool has_insert = test_insert<C>();
        static constexpr bool has_insert_range = test_insert_range<C>();
        static constexpr bool has_erase = test_erase<C>();
        static constexpr bool has_resize = test_resize<C>();
    };

    // Specializations for types that look like ranges, but should be
    // treated as values.
    enum class Mutable { Yes, No };
    template <Mutable IsMutable>
    struct iterable_value : std::false_type {
        static constexpr bool is_mutable = IsMutable == Mutable::Yes;
        static constexpr bool has_insert = false;
        static constexpr bool has_erase = false;
        static constexpr bool has_resize = false;
    };
    template <> struct range_traits<QByteArray> : iterable_value<Mutable::Yes> {};
    template <> struct range_traits<QString> : iterable_value<Mutable::Yes> {};
    template <class CharT, class Traits, class Allocator>
    struct range_traits<std::basic_string<CharT, Traits, Allocator>> : iterable_value<Mutable::Yes>
    {};

    // const T * and views are read-only
    template <typename T> struct range_traits<const T *> : iterable_value<Mutable::No> {};
    template <> struct range_traits<QLatin1StringView> : iterable_value<Mutable::No> {};

    template <typename C>
    using is_range = range_traits<C>;
    template <typename C>
    [[maybe_unused]] static constexpr bool is_range_v = is_range<C>();

    // Detect which options are set to override default heuristics. Since
    // QRangeModel is not yet defined we need to delay the evaluation.
    template <typename T> struct QRangeModelRowOptions;

    template <typename T, typename = void>
    struct row_category : std::false_type
    {
        static constexpr bool isMultiRole = false;
    };

    template <typename T>
    struct row_category<T, std::void_t<decltype(QRangeModelRowOptions<T>::rowCategory)>>
        : std::true_type
    {
        static constexpr auto rowCategory = QRangeModelRowOptions<T>::rowCategory;
        using RowCategory = decltype(rowCategory);
        static constexpr bool isMultiRole = rowCategory == RowCategory::MultiRoleItem;
    };

    // Find out how many fixed elements can be retrieved from a row element.
    // main template for simple values and ranges. Specializing for ranges
    // is ambiguous with arrays, as they are also ranges
    template <typename T, typename = void>
    struct row_traits {
        static constexpr bool is_range = is_range_v<q20::remove_cvref_t<T>>;
        // A static size of -1 indicates dynamically sized range
        // A static size of 0 indicates that the specified type doesn't
        // represent static or dynamic range.
        static constexpr int static_size = is_range ? -1 : 0;
        using item_type = std::conditional_t<is_range, typename range_traits<T>::value_type, void>;
        static constexpr int fixed_size() { return 1; }
        static constexpr bool hasMetaObject = false;
    };

    // Specialization for tuples, using std::tuple_size
    template <typename T>
    struct tuple_row_traits {
        static constexpr std::size_t size64 = std::tuple_size_v<T>;
        static_assert(q20::in_range<int>(size64));
        static constexpr int static_size = int(size64);

        // are the types in a tuple all the same
        template <std::size_t ...I>
        static constexpr bool allSameTypes(std::index_sequence<I...>)
        {
            return (std::is_same_v<std::tuple_element_t<0, T>,
                                   std::tuple_element_t<I, T>> && ...);
        }

        using item_type = std::conditional_t<allSameTypes(std::make_index_sequence<size64>{}),
                                             std::tuple_element_t<0, T>, void>;
        static constexpr int fixed_size() { return 0; }
        static constexpr bool hasMetaObject = false;
    };

    template <typename T>
    struct row_traits<T, std::enable_if_t<tuple_like_v<T> && !has_metaobject_v<T>>>
        : tuple_row_traits<T> {};

    // Specialization for C arrays
    template <typename T, std::size_t N>
    struct row_traits<T[N]>
    {
        static_assert(q20::in_range<int>(N));
        static constexpr int static_size = int(N);
        using item_type = T;
        static constexpr int fixed_size() { return 0; }
        static constexpr bool hasMetaObject = false;
    };

    template <typename T>
    struct metaobject_row_traits
    {
        static constexpr int static_size = 0;
        using item_type = std::conditional_t<row_category<T>::isMultiRole, T, void>;
        static int fixed_size() {
            if constexpr (row_category<T>::isMultiRole) {
                return 1;
            } else {
                // Interpret a gadget in a list as a multi-column row item. To make
                // a list of multi-role items, wrap it into SingleColumn.
                static const int columnCount = []{
                    const QMetaObject &mo = T::staticMetaObject;
                    return mo.propertyCount() - mo.propertyOffset();
                }();
                return columnCount;
            }
        }
        static constexpr bool hasMetaObject = true;
    };

    template <typename T>
    struct row_traits<T, std::enable_if_t<has_metaobject_v<T> && !tuple_like_v<T>>>
        : metaobject_row_traits<T>
    {};

    // types that are both tuple-like, and have a metaobject, default to tuple
    // protocol.
    template <typename T>
    struct row_traits<T, std::enable_if_t<tuple_like_v<T> && has_metaobject_v<T>>>
        : tuple_row_traits<T> {};

    template <typename T>
    [[maybe_unused]] static constexpr int static_size_v =
                            row_traits<std::remove_cv_t<wrapped_t<T>>>::static_size;

    template <typename Range>
    struct ListProtocol
    {
        using row_type = typename range_traits<wrapped_t<Range>>::value_type;

        template <typename R = row_type>
        auto newRow() -> decltype(R{}) { return R{}; }
    };

    template <typename Range>
    struct TableProtocol
    {
        using row_type = typename range_traits<wrapped_t<Range>>::value_type;

        template <typename R = row_type,
                  std::enable_if_t<std::conjunction_v<std::is_destructible<wrapped_t<R>>,
                                                      is_owning_or_raw_pointer<R>>, bool> = true>
        auto newRow() -> decltype(R(new wrapped_t<R>)) {
            if constexpr (is_any_of<R, std::shared_ptr>())
                return std::make_shared<wrapped_t<R>>();
            else
                return R(new wrapped_t<R>);
        }

        template <typename R = row_type,
                  std::enable_if_t<!is_owning_or_raw_pointer<R>::value, bool> = true>
        auto newRow() -> decltype(R{}) { return R{}; }

        template <typename R = row_type,
                  std::enable_if_t<std::is_pointer_v<std::remove_reference_t<R>>, bool> = true>
        auto deleteRow(R&& row) -> decltype(delete row) { delete row; }
    };

    template <typename Range, typename R = typename range_traits<wrapped_t<Range>>::value_type>
    using table_protocol_t = std::conditional_t<static_size_v<R> == 0 && !has_metaobject_v<R>,
                                                ListProtocol<Range>, TableProtocol<Range>>;

    // Default tree traversal protocol implementation for row types that have
    // the respective member functions. The trailing return type implicitly
    // removes those functions that are not available.
    template <typename Range>
    struct DefaultTreeProtocol : TableProtocol<Range>
    {
        template <typename R /*wrapped_row_type*/>
        auto parentRow(const R& row) const -> decltype(row.parentRow())
        {
            return row.parentRow();
        }

        template <typename R /* = wrapped_row_type*/>
        auto setParentRow(R &row, R* parent) -> decltype(row.setParentRow(parent))
        {
            row.setParentRow(parent);
        }

        template <typename R /* = wrapped_row_type*/>
        auto childRows(const R &row) const -> decltype(row.childRows())
        {
            return row.childRows();
        }

        template <typename R /* = wrapped_row_type*/>
        auto childRows(R &row) -> decltype(row.childRows())
        {
            return row.childRows();
        }
    };

    template <typename P, typename R, typename = void>
    struct protocol_parentRow : std::false_type {};
    template <typename P, typename R>
    struct protocol_parentRow<P, R,
            std::void_t<decltype(std::declval<P&>().parentRow(std::declval<wrapped_t<R>&>()))>>
        : std::true_type {};

    template <typename P, typename R, typename = void>
    struct protocol_childRows : std::false_type {};
    template <typename P, typename R>
    struct protocol_childRows<P, R,
            std::void_t<decltype(std::declval<P&>().childRows(std::declval<wrapped_t<R>&>()))>>
        : std::true_type {};

    template <typename P, typename R, typename = void>
    struct protocol_setParentRow : std::false_type {};
    template <typename P, typename R>
    struct protocol_setParentRow<P, R,
            std::void_t<decltype(std::declval<P&>().setParentRow(std::declval<wrapped_t<R>&>(),
                                                                 std::declval<wrapped_t<R>*>()))>>
        : std::true_type {};

    template <typename P, typename R, typename = void>
    struct protocol_mutable_childRows : std::false_type {};
    template <typename P, typename R>
    struct protocol_mutable_childRows<P, R,
            std::void_t<decltype(refTo(std::declval<P&>().childRows(std::declval<wrapped_t<R>&>()))
                                                                                            = {}) >>
        : std::true_type {};

    template <typename P, typename = void>
    struct protocol_newRow : std::false_type {};
    template <typename P>
    struct protocol_newRow<P, std::void_t<decltype(std::declval<P&>().newRow())>>
        : std::true_type {};

    template <typename P, typename R, typename = void>
    struct protocol_deleteRow : std::false_type {};
    template <typename P, typename R>
    struct protocol_deleteRow<P, R,
            std::void_t<decltype(std::declval<P&>().deleteRow(std::declval<R&&>()))>>
        : std::true_type {};

    template <typename Range,
              typename Protocol = DefaultTreeProtocol<Range>,
              typename R = typename range_traits<Range>::value_type,
              typename = void>
    struct is_tree_range : std::false_type {};

    template <typename Range, typename Protocol, typename R>
    struct is_tree_range<Range, Protocol, R,
                         std::enable_if_t<std::conjunction_v<
                            protocol_parentRow<Protocol, R>, protocol_childRows<Protocol, R>>>
            > : std::true_type {};

    template <typename Range>
    using if_table_range = std::enable_if_t<std::conjunction_v<is_range<wrapped_t<Range>>,
                                                    std::negation<is_tree_range<wrapped_t<Range>>>>,
                                            bool>;

    template <typename Range, typename Protocol = DefaultTreeProtocol<Range>>
    using if_tree_range = std::enable_if_t<std::conjunction_v<is_range<wrapped_t<Range>>,
                                              is_tree_range<wrapped_t<Range>, wrapped_t<Protocol>>>,
                                           bool>;

    template <typename Range, typename Protocol>
    struct protocol_traits
    {
        using protocol = wrapped_t<Protocol>;
        using row = typename range_traits<wrapped_t<Range>>::value_type;

        static constexpr bool has_newRow = protocol_newRow<protocol>();
        static constexpr bool has_deleteRow = protocol_deleteRow<protocol, row>();
        static constexpr bool has_setParentRow = protocol_setParentRow<protocol, row>();
        static constexpr bool has_mutable_childRows = protocol_mutable_childRows<protocol, row>();

        static constexpr bool is_default = is_any_of<protocol, ListProtocol, TableProtocol, DefaultTreeProtocol>();
    };

    template <bool cacheProperties>
    struct PropertyData {
        static constexpr bool cachesProperties = false;

        void invalidateCaches() {}
    };

    template <>
    struct PropertyData<true>
    {
        static constexpr bool cachesProperties = true;
        mutable QHash<int, QMetaProperty> properties;

        void invalidateCaches()
        {
            properties.clear();
        }
    };

    // The storage of the model data. We might store it as a pointer, or as a
    // (copied- or moved-into) value (or smart pointer). But we always return a
    // raw pointer.
    template <typename ModelStorage, typename ItemType>
    struct ModelData : PropertyData<has_metaobject_v<ItemType>>
    {
        auto model() { return pointerTo(m_model); }
        auto model() const { return pointerTo(m_model); }

        template <typename Model = ModelStorage>
        ModelData(Model &&model)
            : m_model(std::forward<Model>(model))
        {}
        ModelStorage m_model;
    };
} // namespace QRangeModelDetails

class QRangeModel;

class QRangeModelImplBase
{
    Q_DISABLE_COPY_MOVE(QRangeModelImplBase)
protected:
    // Helpers for calling a lambda with the tuple element at a runtime index.
    template <typename Tuple, typename F, size_t ...Is>
    static void call_at(Tuple &&tuple, size_t idx, std::index_sequence<Is...>, F &&function)
    {
        if (QRangeModelDetails::isValid(tuple))
            ((Is == idx ? static_cast<void>(function(get<Is>(
                            QRangeModelDetails::refTo(std::forward<Tuple>(tuple)))))
                        : static_cast<void>(0)), ...);
    }

    template <typename T, typename F>
    static auto for_element_at(T &&tuple, size_t idx, F &&function)
    {
        using type = QRangeModelDetails::wrapped_t<T>;
        constexpr size_t size = std::tuple_size_v<type>;
        Q_ASSERT(idx < size);
        return call_at(std::forward<T>(tuple), idx, std::make_index_sequence<size>{},
                       std::forward<F>(function));
    }

    // Get the QMetaType for a tuple-element at a runtime index.
    // Used in the headerData implementation.
    template <typename Tuple, std::size_t ...I>
    static constexpr std::array<QMetaType, sizeof...(I)> makeMetaTypes(std::index_sequence<I...>)
    {
        return {{QMetaType::fromType<q20::remove_cvref_t<std::tuple_element_t<I, Tuple>>>()...}};
    }
    template <typename T>
    static constexpr QMetaType meta_type_at(size_t idx)
    {
        using type = QRangeModelDetails::wrapped_t<T>;
        constexpr auto size = std::tuple_size_v<type>;
        Q_ASSERT(idx < size);
        return makeMetaTypes<type>(std::make_index_sequence<size>{}).at(idx);
    }

    // Helpers to call a given member function with the correct arguments.
    template <typename Class, typename T, typename F, size_t...I>
    static auto apply(std::integer_sequence<size_t, I...>, Class* obj, F&& fn, T&& tuple)
    {
        return std::invoke(fn, obj, std::get<I>(tuple)...);
    }
    template <typename Ret, typename Class, typename ...Args>
    static void makeCall(QRangeModelImplBase *obj, Ret(Class::* &&fn)(Args...),
                              void *ret, const void *args)
    {
        const auto &tuple = *static_cast<const std::tuple<Args&...> *>(args);
        *static_cast<Ret *>(ret) = apply(std::make_index_sequence<sizeof...(Args)>{},
                                         static_cast<Class *>(obj), fn, tuple);
    }
    template <typename Ret, typename Class, typename ...Args>
    static void makeCall(const QRangeModelImplBase *obj, Ret(Class::* &&fn)(Args...) const,
                         void *ret, const void *args)
    {
        const auto &tuple = *static_cast<const std::tuple<Args&...> *>(args);
        *static_cast<Ret *>(ret) = apply(std::make_index_sequence<sizeof...(Args)>{},
                                         static_cast<const Class *>(obj), fn, tuple);
    }

public:
    enum ConstOp {
        Index,
        Parent,
        Sibling,
        RowCount,
        ColumnCount,
        Flags,
        HeaderData,
        Data,
        ItemData,
        RoleNames,
    };

    enum Op {
        Destroy,
        InvalidateCaches,
        SetHeaderData,
        SetData,
        SetItemData,
        ClearItemData,
        InsertColumns,
        RemoveColumns,
        MoveColumns,
        InsertRows,
        RemoveRows,
        MoveRows,
    };

    void destroy()
    {
        call_fn(Destroy, this, nullptr, nullptr);
    }

    void invalidateCaches()
    {
        call_fn(InvalidateCaches, this, nullptr, nullptr);
    }

private:
    // prototypes
    static void callConst(ConstOp, const QRangeModelImplBase *, void *, const void *);
    static void call(Op, QRangeModelImplBase *, void *, const void *);

    using CallConstFN = decltype(callConst);
    using CallTupleFN = decltype(call);

    friend class QRangeModelPrivate;
    CallConstFN *callConst_fn;
    CallTupleFN *call_fn;
    QRangeModel *m_rangeModel;

protected:
    template <typename Impl> // type deduction
    explicit QRangeModelImplBase(QRangeModel *itemModel, const Impl *)
        : callConst_fn(&Impl::callConst), call_fn(&Impl::call), m_rangeModel(itemModel)
    {}
    ~QRangeModelImplBase() = default;

    inline QModelIndex createIndex(int row, int column, const void *ptr = nullptr) const;
    inline void changePersistentIndexList(const QModelIndexList &from, const QModelIndexList &to);
    inline QHash<int, QByteArray> roleNames() const;
    inline void dataChanged(const QModelIndex &from, const QModelIndex &to,
                            const QList<int> &roles);
    inline void beginInsertColumns(const QModelIndex &parent, int start, int count);
    inline void endInsertColumns();
    inline void beginRemoveColumns(const QModelIndex &parent, int start, int count);
    inline void endRemoveColumns();
    inline bool beginMoveColumns(const QModelIndex &sourceParent, int sourceFirst, int sourceLast,
                                 const QModelIndex &destParent, int destRow);
    inline void endMoveColumns();
    inline void beginInsertRows(const QModelIndex &parent, int start, int count);
    inline void endInsertRows();
    inline void beginRemoveRows(const QModelIndex &parent, int start, int count);
    inline void endRemoveRows();
    inline bool beginMoveRows(const QModelIndex &sourceParent, int sourceFirst, int sourceLast,
                              const QModelIndex &destParent, int destRow);
    inline void endMoveRows();
    inline QAbstractItemModel &itemModel();
    inline const QAbstractItemModel &itemModel() const;

    // implemented in qrangemodel.cpp
    Q_CORE_EXPORT QHash<int, QByteArray> roleNamesForMetaObject(const QMetaObject &metaObject) const;
};

template <typename Structure, typename Range,
          typename Protocol = QRangeModelDetails::table_protocol_t<Range>>
class QRangeModelImpl : public QRangeModelImplBase
{
    Q_DISABLE_COPY_MOVE(QRangeModelImpl)
public:
    using range_type = QRangeModelDetails::wrapped_t<Range>;
    using row_reference = decltype(*QRangeModelDetails::begin(std::declval<range_type&>()));
    using const_row_reference = decltype(*QRangeModelDetails::cbegin(std::declval<range_type&>()));
    using row_type = std::remove_reference_t<row_reference>;
    using protocol_type = QRangeModelDetails::wrapped_t<Protocol>;

    static_assert(!QRangeModelDetails::is_any_of<range_type, std::optional>() &&
                  !QRangeModelDetails::is_any_of<row_type, std::optional>(),
                  "Currently, std::optional is not supported for ranges and rows, as "
                  "it has range semantics in c++26. Once the required behavior is clarified, "
                  "std::optional for ranges and rows will be supported.");

protected:
    using Self = QRangeModelImpl<Structure, Range, Protocol>;
    Structure& that() { return static_cast<Structure &>(*this); }
    const Structure& that() const { return static_cast<const Structure &>(*this); }

    template <typename C>
    static constexpr int size(const C &c)
    {
        if (!QRangeModelDetails::isValid(c))
            return 0;

        if constexpr (QRangeModelDetails::test_size<C>()) {
            return int(std::size(c));
        } else {
#if defined(__cpp_lib_ranges)
            return int(std::ranges::distance(QRangeModelDetails::begin(c),
                                             QRangeModelDetails::end(c)));
#else
            return int(std::distance(QRangeModelDetails::begin(c),
                                     QRangeModelDetails::end(c)));
#endif
        }
    }

    using range_features = QRangeModelDetails::range_traits<range_type>;
    using wrapped_row_type = QRangeModelDetails::wrapped_t<row_type>;
    using row_features = QRangeModelDetails::range_traits<wrapped_row_type>;
    using row_traits = QRangeModelDetails::row_traits<std::remove_cv_t<wrapped_row_type>>;
    using protocol_traits = QRangeModelDetails::protocol_traits<Range, protocol_type>;

    static constexpr bool isMutable()
    {
        return range_features::is_mutable && row_features::is_mutable
            && std::is_reference_v<row_reference>
            && Structure::is_mutable_impl;
    }

    static constexpr int static_row_count = QRangeModelDetails::static_size_v<range_type>;
    static constexpr bool rows_are_raw_pointers = std::is_pointer_v<row_type>;
    static constexpr bool rows_are_owning_or_raw_pointers =
            QRangeModelDetails::is_owning_or_raw_pointer<row_type>();
    static constexpr int static_column_count = QRangeModelDetails::static_size_v<row_type>;
    static constexpr bool one_dimensional_range = static_column_count == 0;

    static constexpr bool dynamicRows() { return isMutable() && static_row_count < 0; }
    static constexpr bool dynamicColumns() { return static_column_count < 0; }

    // A row might be a value (or range of values), or a pointer.
    // row_ptr is always a pointer, and const_row_ptr is a pointer to const.
    using row_ptr = wrapped_row_type *;
    using const_row_ptr = const wrapped_row_type *;

    template <typename T>
    static constexpr bool has_metaobject = QRangeModelDetails::has_metaobject_v<
                                                std::remove_pointer_t<std::remove_reference_t<T>>>;

    using ModelData = QRangeModelDetails::ModelData<std::conditional_t<
                                                        std::is_pointer_v<Range>,
                                                        Range, std::remove_reference_t<Range>
                                                    >,
                                                    typename row_traits::item_type
                                                >;

    // A iterator type to use as the input iterator with the
    // range_type::insert(pos, start, end) overload if available (it is in
    // std::vector, but not in QList). Generates a prvalue when dereferenced,
    // which then gets moved into the newly constructed row, which allows us to
    // implement insertRows() for move-only row types.
    struct EmptyRowGenerator
    {
        using value_type = row_type;
        using reference = value_type;
        using pointer = value_type *;
        using iterator_category = std::input_iterator_tag;
        using difference_type = int;

        value_type operator*() { return impl->makeEmptyRow(*parent); }
        EmptyRowGenerator &operator++() { ++n; return *this; }
        friend bool operator==(const EmptyRowGenerator &lhs, const EmptyRowGenerator &rhs) noexcept
        { return lhs.n == rhs.n; }
        friend bool operator!=(const EmptyRowGenerator &lhs, const EmptyRowGenerator &rhs) noexcept
        { return !(lhs == rhs); }

        difference_type n = 0;
        Structure *impl = nullptr;
        const QModelIndex* parent = nullptr;
    };

    // If we have a move-only row_type and can add/remove rows, then the range
    // must have an insert-from-range overload.
    static_assert(static_row_count || range_features::has_insert_range
                                   || std::is_copy_constructible_v<row_type>,
                  "The range holding a move-only row-type must support insert(pos, start, end)");

public:
    explicit QRangeModelImpl(Range &&model, Protocol&& protocol, QRangeModel *itemModel)
        : QRangeModelImplBase(itemModel, static_cast<const Self*>(nullptr))
        , m_data{std::forward<Range>(model)}
        , m_protocol(std::forward<Protocol>(protocol))
    {
    }

    // static interface, called by QRangeModelImplBase
    static void callConst(ConstOp op, const QRangeModelImplBase *that, void *r, const void *args)
    {
        switch (op) {
        case Index: makeCall(that, &Self::index, r, args);
            break;
        case Parent: makeCall(that, &Structure::parent, r, args);
            break;
        case Sibling: makeCall(that, &Self::sibling, r, args);
            break;
        case RowCount: makeCall(that, &Structure::rowCount, r, args);
            break;
        case ColumnCount: makeCall(that, &Structure::columnCount, r, args);
            break;
        case Flags: makeCall(that, &Self::flags, r, args);
            break;
        case HeaderData: makeCall(that, &Self::headerData, r, args);
            break;
        case Data: makeCall(that, &Self::data, r, args);
            break;
        case ItemData: makeCall(that, &Self::itemData, r, args);
            break;
        case RoleNames: makeCall(that, &Self::roleNames, r, args);
            break;
        }
    }

    static void call(Op op, QRangeModelImplBase *that, void *r, const void *args)
    {
        switch (op) {
        case Destroy: delete static_cast<Structure *>(that);
            break;
        case InvalidateCaches: static_cast<Self *>(that)->m_data.invalidateCaches();
            break;
        case SetHeaderData:
            // not implemented
            break;
        case SetData: makeCall(that, &Self::setData, r, args);
            break;
        case SetItemData: makeCall(that, &Self::setItemData, r, args);
            break;
        case ClearItemData: makeCall(that, &Self::clearItemData, r, args);
            break;
        case InsertColumns: makeCall(that, &Self::insertColumns, r, args);
            break;
        case RemoveColumns: makeCall(that, &Self::removeColumns, r, args);
            break;
        case MoveColumns: makeCall(that, &Self::moveColumns, r, args);
            break;
        case InsertRows: makeCall(that, &Self::insertRows, r, args);
            break;
        case RemoveRows: makeCall(that, &Self::removeRows, r, args);
            break;
        case MoveRows: makeCall(that, &Self::moveRows, r, args);
            break;
        }
    }

    // actual implementations
    QModelIndex index(int row, int column, const QModelIndex &parent) const
    {
        if (row < 0 || column < 0 || column >= that().columnCount(parent)
                                  || row >= that().rowCount(parent)) {
            return {};
        }

        return that().indexImpl(row, column, parent);
    }

    QModelIndex sibling(int row, int column, const QModelIndex &index) const
    {
        if (row == index.row() && column == index.column())
            return index;

        if (column < 0 || column >= itemModel().columnCount())
            return {};

        if (row == index.row())
            return createIndex(row, column, index.constInternalPointer());

        const_row_ptr parentRow = static_cast<const_row_ptr>(index.constInternalPointer());
        const auto siblingCount = size(that().childrenOf(parentRow));
        if (row < 0 || row >= int(siblingCount))
            return {};
        return createIndex(row, column, parentRow);
    }

    Qt::ItemFlags flags(const QModelIndex &index) const
    {
        if (!index.isValid())
            return Qt::NoItemFlags;

        Qt::ItemFlags f = Structure::defaultFlags();

        if constexpr (row_traits::hasMetaObject) {
            if (index.column() < row_traits::fixed_size()) {
                const QMetaObject mo = wrapped_row_type::staticMetaObject;
                const QMetaProperty prop = mo.property(index.column() + mo.propertyOffset());
                if (prop.isWritable())
                    f |= Qt::ItemIsEditable;
            }
        } else if constexpr (static_column_count <= 0) {
            if constexpr (isMutable())
                f |= Qt::ItemIsEditable;
        } else if constexpr (std::is_reference_v<row_reference> && !std::is_const_v<row_reference>) {
            // we want to know if the elements in the tuple are const; they'd always be, if
            // we didn't remove the const of the range first.
            const_row_reference row = rowData(index);
            row_reference mutableRow = const_cast<row_reference>(row);
            if (QRangeModelDetails::isValid(mutableRow)) {
                for_element_at(mutableRow, index.column(), [&f](auto &&ref){
                    using target_type = decltype(ref);
                    if constexpr (std::is_const_v<std::remove_reference_t<target_type>>)
                        f &= ~Qt::ItemIsEditable;
                    else if constexpr (std::is_lvalue_reference_v<target_type>)
                        f |= Qt::ItemIsEditable;
                });
            } else {
                // If there's no usable value stored in the row, then we can't
                // do anything with this item.
                f &= ~Qt::ItemIsEditable;
            }
        }
        return f;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const
    {
        QVariant result;
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal
         || section < 0 || section >= that().columnCount({})) {
            return itemModel().QAbstractItemModel::headerData(section, orientation, role);
        }

        if constexpr (row_traits::hasMetaObject) {
            if (row_traits::fixed_size() == 1) {
                const QMetaType metaType = QMetaType::fromType<wrapped_row_type>();
                result = QString::fromUtf8(metaType.name());
            } else if (section <= row_traits::fixed_size()) {
                const QMetaProperty prop = wrapped_row_type::staticMetaObject.property(
                                    section + wrapped_row_type::staticMetaObject.propertyOffset());
                result = QString::fromUtf8(prop.name());
            }
        } else if constexpr (static_column_count >= 1) {
            const QMetaType metaType = meta_type_at<row_type>(section);
            if (metaType.isValid())
                result = QString::fromUtf8(metaType.name());
        }
        if (!result.isValid())
            result = itemModel().QAbstractItemModel::headerData(section, orientation, role);
        return result;
    }

    QVariant data(const QModelIndex &index, int role) const
    {
        QVariant result;
        const auto readData = [this, column = index.column(), &result, role](const auto &value) {
            Q_UNUSED(this);
            using value_type = q20::remove_cvref_t<decltype(value)>;
            using multi_role = QRangeModelDetails::is_multi_role<value_type>;
            if constexpr (has_metaobject<value_type>) {
                if (row_traits::fixed_size() <= 1) {
                    result = readRole(role, QRangeModelDetails::pointerTo(value));
                } else if (column <= row_traits::fixed_size()
                        && (role == Qt::DisplayRole || role == Qt::EditRole)) {
                    result = readProperty(column, QRangeModelDetails::pointerTo(value));
                }
            } else if constexpr (multi_role::value) {
                const auto it = [this, &value, role]{
                    Q_UNUSED(this);
                    if constexpr (multi_role::int_key)
                        return std::as_const(value).find(Qt::ItemDataRole(role));
                    else
                        return std::as_const(value).find(itemModel().roleNames().value(role));
                }();
                if (it != value.cend()) {
                    result = QRangeModelDetails::value(it);
                }
            } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
                result = read(value);
            }
        };

        if (index.isValid())
            readAt(index, readData);

        return result;
    }

    QMap<int, QVariant> itemData(const QModelIndex &index) const
    {
        QMap<int, QVariant> result;
        bool tried = false;
        const auto readItemData = [this, &result, &tried](auto &&value){
            Q_UNUSED(this);
            using value_type = q20::remove_cvref_t<decltype(value)>;
            using multi_role = QRangeModelDetails::is_multi_role<value_type>;
            if constexpr (multi_role()) {
                tried = true;
                if constexpr (std::is_convertible_v<value_type, decltype(result)>) {
                    result = value;
                } else {
                    const auto roleNames = [this]() -> QHash<int, QByteArray> {
                        Q_UNUSED(this);
                        if constexpr (!multi_role::int_key)
                            return itemModel().roleNames();
                        else
                            return {};
                    }();
                    for (auto it = std::cbegin(value); it != std::cend(value); ++it) {
                        const int role = [&roleNames, key = QRangeModelDetails::key(it)]() {
                            Q_UNUSED(roleNames);
                            if constexpr (multi_role::int_key)
                                return int(key);
                            else
                                return roleNames.key(key.toUtf8(), -1);
                        }();

                        if (role != -1)
                            result.insert(role, QRangeModelDetails::value(it));
                    }
                }
            } else if constexpr (has_metaobject<value_type>) {
                if (row_traits::fixed_size() <= 1) {
                    tried = true;
                    using wrapped_type = QRangeModelDetails::wrapped_t<value_type>;
                    for (auto &&[role, roleName] : itemModel().roleNames().asKeyValueRange()) {
                        QVariant data;
                        if constexpr (std::is_base_of_v<QObject, wrapped_type>) {
                            if (value)
                                data = value->property(roleName);
                        } else {
                            const QMetaProperty prop = this->roleProperty<wrapped_type>(roleName);
                            if (prop.isValid())
                                data = prop.readOnGadget(QRangeModelDetails::pointerTo(value));
                        }
                        if (data.isValid())
                            result[role] = std::move(data);
                    }
                }
            }
        };

        if (index.isValid()) {
            readAt(index, readItemData);

            if (!tried) // no multi-role item found
                result = itemModel().QAbstractItemModel::itemData(index);
        }
        return result;
    }

    bool setData(const QModelIndex &index, const QVariant &data, int role)
    {
        if (!index.isValid())
            return false;

        bool success = false;
        if constexpr (isMutable()) {
            auto emitDataChanged = qScopeGuard([&success, this, &index, &role]{
                if (success) {
                    Q_EMIT dataChanged(index, index, role == Qt::EditRole
                                                     ? QList<int>{} : QList{role});
                }
            });

            const auto writeData = [this, column = index.column(), &data, role](auto &&target) -> bool {
                using value_type = q20::remove_cvref_t<decltype(target)>;
                using multi_role = QRangeModelDetails::is_multi_role<value_type>;
                if constexpr (has_metaobject<value_type>) {
                    if (QMetaType::fromType<value_type>() == data.metaType()) {
                        if constexpr (std::is_copy_assignable_v<value_type>) {
                            target = data.value<value_type>();
                            return true;
                        } else {
                            qCritical("Cannot assign %s", QMetaType::fromType<value_type>().name());
                            return false;
                        }
                    } else if (row_traits::fixed_size() <= 1) {
                        return writeRole(role, QRangeModelDetails::pointerTo(target), data);
                    } else if (column <= row_traits::fixed_size()
                            && (role == Qt::DisplayRole || role == Qt::EditRole)) {
                        return writeProperty(column, QRangeModelDetails::pointerTo(target), data);
                    }
                } else if constexpr (multi_role::value) {
                    Qt::ItemDataRole roleToSet = Qt::ItemDataRole(role);
                    // If there is an entry for EditRole, overwrite that; otherwise,
                    // set the entry for DisplayRole.
                    const auto roleNames = [this]() -> QHash<int, QByteArray> {
                        Q_UNUSED(this);
                        if constexpr (!multi_role::int_key)
                            return itemModel().roleNames();
                        else
                            return {};
                    }();
                    if (role == Qt::EditRole) {
                        if constexpr (multi_role::int_key) {
                            if (target.find(roleToSet) == target.end())
                                roleToSet = Qt::DisplayRole;
                        } else {
                            if (target.find(roleNames.value(roleToSet)) == target.end())
                                roleToSet = Qt::DisplayRole;
                        }
                    }
                    if constexpr (multi_role::int_key)
                        return write(target[roleToSet], data);
                    else
                        return write(target[roleNames.value(roleToSet)], data);
                } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
                    return write(target, data);
                }
                return false;
            };

            success = writeAt(index, writeData);
        }
        return success;
    }

    bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &data)
    {
        if (!index.isValid() || data.isEmpty())
            return false;

        bool success = false;
        if constexpr (isMutable()) {
            auto emitDataChanged = qScopeGuard([&success, this, &index, &data]{
                if (success)
                    Q_EMIT dataChanged(index, index, data.keys());
            });

            bool tried = false;
            auto writeItemData = [this, &tried, &data](auto &target) -> bool {
                Q_UNUSED(this);
                using value_type = q20::remove_cvref_t<decltype(target)>;
                using multi_role = QRangeModelDetails::is_multi_role<value_type>;
                if constexpr (multi_role()) {
                    using key_type = typename value_type::key_type;
                    tried = true;
                    const auto roleName = [map = itemModel().roleNames()](int role) {
                        return map.value(role);
                    };

                    // transactional: only update target if all values from data
                    // can be stored. Storing never fails with int-keys.
                    if constexpr (!multi_role::int_key)
                    {
                        auto invalid = std::find_if(data.keyBegin(), data.keyEnd(),
                            [&roleName](int role) { return roleName(role).isEmpty(); }
                        );

                        if (invalid != data.keyEnd()) {
                            qWarning("No role name set for %d", *invalid);
                            return false;
                        }
                    }

                    for (auto &&[role, value] : data.asKeyValueRange()) {
                        if constexpr (multi_role::int_key)
                            target[static_cast<key_type>(role)] = value;
                        else
                            target[QString::fromUtf8(roleName(role))] = value;
                    }
                    return true;
                } else if constexpr (has_metaobject<value_type>) {
                    if (row_traits::fixed_size() <= 1) {
                        tried = true;
                        using wrapped_type = QRangeModelDetails::wrapped_t<value_type>;
                        // transactional: if possible, modify a copy and only
                        // update target if all values from data could be stored.
                        auto targetCopy = [](auto &&origin) {
                            if constexpr (!std::is_copy_assignable_v<wrapped_type>)
                                return QRangeModelDetails::pointerTo(origin); // no transaction support
                            else if constexpr (std::is_pointer_v<decltype(target)>)
                                return *origin;
                            else if constexpr (std::is_copy_assignable_v<value_type>)
                                return origin;
                            else
                                return QRangeModelDetails::pointerTo(origin);
                        }(target);
                        const auto roleNames = itemModel().roleNames();
                        for (auto &&[role, value] : data.asKeyValueRange()) {
                            const QByteArray roleName = roleNames.value(role);
                            bool written = false;
                            if constexpr (std::is_base_of_v<QObject, wrapped_type>) {
                                if (targetCopy)
                                    written = targetCopy->setProperty(roleName, value);
                            } else {
                                const QMetaProperty prop = this->roleProperty<wrapped_type>(roleName);
                                if (prop.isValid())
                                    written = prop.writeOnGadget(QRangeModelDetails::pointerTo(targetCopy), value);
                            }
                            if (!written) {
                                qWarning("Failed to write value '%s' to role '%s'",
                                         qPrintable(QDebug::toString(value)), roleName.data());
                                return false;
                            }
                        }
                        if constexpr (std::is_pointer_v<decltype(targetCopy)>)
                            ; // couldn't copy
                        else if constexpr (std::is_pointer_v<decltype(target)>)
                            qSwap(*target, targetCopy);
                        else
                            qSwap(target, targetCopy);
                        return true;
                    }
                }
                return false;
            };

            success = writeAt(index, writeItemData);

            if (!tried) {
                // setItemData will emit the dataChanged signal
                Q_ASSERT(!success);
                emitDataChanged.dismiss();
                success = itemModel().QAbstractItemModel::setItemData(index, data);
            }
        }
        return success;
    }

    bool clearItemData(const QModelIndex &index)
    {
        if (!index.isValid())
            return false;

        bool success = false;
        if constexpr (isMutable()) {
            auto emitDataChanged = qScopeGuard([&success, this, &index]{
                if (success)
                    Q_EMIT dataChanged(index, index, {});
            });

            auto clearData = [column = index.column()](auto &&target) {
                if constexpr (row_traits::hasMetaObject) {
                    if (row_traits::fixed_size() <= 1) {
                        // multi-role object/gadget: reset all properties
                        return resetProperty(-1, QRangeModelDetails::pointerTo(target));
                    } else if (column <= row_traits::fixed_size()) {
                        return resetProperty(column, QRangeModelDetails::pointerTo(target));
                    }
                } else { // normal structs, values, associative containers
                    target = {};
                    return true;
                }
                return false;
            };

            success = writeAt(index, clearData);
        }
        return success;
    }

    QHash<int, QByteArray> roleNames() const
    {
        // will be 'void' if columns don't all have the same type
        using item_type = typename row_traits::item_type;
        if constexpr (QRangeModelDetails::has_metaobject_v<item_type>) {
            return roleNamesForMetaObject(QRangeModelDetails::wrapped_t<item_type>::staticMetaObject);
        }

        return itemModel().QAbstractItemModel::roleNames();
    }

    bool insertColumns(int column, int count, const QModelIndex &parent)
    {
        if constexpr (dynamicColumns() && isMutable() && row_features::has_insert) {
            if (count == 0)
                return false;
            range_type * const children = childRange(parent);
            if (!children)
                return false;

            beginInsertColumns(parent, column, column + count - 1);
            for (auto &child : *children) {
                auto it = QRangeModelDetails::pos(child, column);
                QRangeModelDetails::refTo(child).insert(it, count, {});
            }
            endInsertColumns();
            return true;
        }
        return false;
    }

    bool removeColumns(int column, int count, const QModelIndex &parent)
    {
        if constexpr (dynamicColumns() && isMutable() && row_features::has_erase) {
            if (column < 0 || column + count > that().columnCount(parent))
                return false;

            range_type * const children = childRange(parent);
            if (!children)
                return false;

            beginRemoveColumns(parent, column, column + count - 1);
            for (auto &child : *children) {
                const auto start = QRangeModelDetails::pos(child, column);
                QRangeModelDetails::refTo(child).erase(start, std::next(start, count));
            }
            endRemoveColumns();
            return true;
        }
        return false;
    }

    bool moveColumns(const QModelIndex &sourceParent, int sourceColumn, int count,
                     const QModelIndex &destParent, int destColumn)
    {
        // we only support moving columns within the same parent
        if (sourceParent != destParent)
            return false;
        if constexpr (isMutable()) {
            if (!Structure::canMoveColumns(sourceParent, destParent))
                return false;

            if constexpr (dynamicColumns()) {
                // we only support ranges as columns, as other types might
                // not have the same data type across all columns
                range_type * const children = childRange(sourceParent);
                if (!children)
                    return false;

                if (!beginMoveColumns(sourceParent, sourceColumn, sourceColumn + count - 1,
                                      destParent, destColumn)) {
                    return false;
                }

                for (auto &child : *children) {
                    const auto first = QRangeModelDetails::pos(child, sourceColumn);
                    const auto middle = std::next(first, count);
                    const auto last = QRangeModelDetails::pos(child, destColumn);

                    if (sourceColumn < destColumn) // moving right
                        std::rotate(first, middle, last);
                    else // moving left
                        std::rotate(last, first, middle);
                }

                endMoveColumns();
                return true;
            }
        }
        return false;
    }

    bool insertRows(int row, int count, const QModelIndex &parent)
    {
        if constexpr (canInsertRows()) {
            range_type *children = childRange(parent);
            if (!children)
                return false;

            EmptyRowGenerator generator{0, &that(), &parent};

            beginInsertRows(parent, row, row + count - 1);

            const auto pos = QRangeModelDetails::pos(children, row);
            if constexpr (range_features::has_insert_range) {
                children->insert(pos, generator, EmptyRowGenerator{count});
            } else if constexpr (rows_are_owning_or_raw_pointers) {
                auto start = children->insert(pos, count, row_type{});
                std::copy(generator, EmptyRowGenerator{count}, start);
            } else {
                children->insert(pos, count, *generator);
            }

            // fix the parent in all children of the modified row, as the
            // references back to the parent might have become invalid.
            that().resetParentInChildren(children);

            endInsertRows();
            return true;
        } else {
            return false;
        }
    }

    bool removeRows(int row, int count, const QModelIndex &parent = {})
    {
        if constexpr (canRemoveRows()) {
            const int prevRowCount = that().rowCount(parent);
            if (row < 0 || row + count > prevRowCount)
                return false;

            range_type *children = childRange(parent);
            if (!children)
                return false;

            beginRemoveRows(parent, row, row + count - 1);
            [[maybe_unused]] bool callEndRemoveColumns = false;
            if constexpr (dynamicColumns()) {
                // if we remove the last row in a dynamic model, then we no longer
                // know how many columns we should have, so they will be reported as 0.
                if (prevRowCount == count) {
                    if (const int columns = that().columnCount(parent)) {
                        callEndRemoveColumns = true;
                        beginRemoveColumns(parent, 0, columns - 1);
                    }
                }
            }
            { // erase invalidates iterators
                const auto begin = QRangeModelDetails::pos(children, row);
                const auto end = std::next(begin, count);
                that().deleteRemovedRows(begin, end);
                children->erase(begin, end);
            }
            // fix the parent in all children of the modified row, as the
            // references back to the parent might have become invalid.
            that().resetParentInChildren(children);

            if constexpr (dynamicColumns()) {
                if (callEndRemoveColumns) {
                    Q_ASSERT(that().columnCount(parent) == 0);
                    endRemoveColumns();
                }
            }
            endRemoveRows();
            return true;
        } else {
            return false;
        }
    }

    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                  const QModelIndex &destParent, int destRow)
    {
        if constexpr (isMutable()) {
            if (!Structure::canMoveRows(sourceParent, destParent))
                return false;

            if (sourceParent != destParent) {
                return that().moveRowsAcross(sourceParent, sourceRow, count,
                                             destParent, destRow);
            }

            if (sourceRow == destRow || sourceRow == destRow - 1 || count <= 0
             || sourceRow < 0 || sourceRow + count - 1 >= itemModel().rowCount(sourceParent)
             || destRow < 0 || destRow > itemModel().rowCount(destParent)) {
                return false;
            }

            range_type *source = childRange(sourceParent);
            // moving within the same range
            if (!beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destParent, destRow))
                return false;

            const auto first = QRangeModelDetails::pos(source, sourceRow);
            const auto middle = std::next(first, count);
            const auto last = QRangeModelDetails::pos(source, destRow);

            if (sourceRow < destRow) // moving down
                std::rotate(first, middle, last);
            else // moving up
                std::rotate(last, first, middle);

            that().resetParentInChildren(source);

            endMoveRows();
            return true;
        } else {
            return false;
        }
    }

protected:
    ~QRangeModelImpl()
    {
        // We delete row objects if we are not operating on a reference or pointer
        // to a range, as in that case, the owner of the referenced/pointed to
        // range also owns the row entries.
        // ### Problem: if we get a copy of a range (no matter if shared or not),
        // then adding rows will create row objects in the model's copy, and the
        // client can never delete those. But copied rows will be the same pointer,
        // which we must not delete (as we didn't create them).
        if constexpr (protocol_traits::has_deleteRow && !std::is_pointer_v<Range>
                   && !QRangeModelDetails::is_any_of<Range, std::reference_wrapper>()) {
            const auto begin = QRangeModelDetails::begin(*m_data.model());
            const auto end = QRangeModelDetails::end(*m_data.model());
            that().deleteRemovedRows(begin, end);
        }
    }

    static constexpr bool canInsertRows()
    {
        if constexpr (dynamicColumns() && !row_features::has_resize) {
            // If we operate on dynamic columns and cannot resize a newly
            // constructed row, then we cannot insert.
            return false;
        } else if constexpr (!protocol_traits::has_newRow) {
            // We also cannot insert if we cannot create a new row element
            return false;
        } else if constexpr (!range_features::has_insert_range
                          && !std::is_copy_constructible_v<row_type>) {
            // And if the row is a move-only type, then the range needs to be
            // backed by a container that can move-insert default-constructed
            // row elements.
            return false;
        } else {
            return Structure::canInsertRowsImpl();
        }
    }

    static constexpr bool canRemoveRows()
    {
        return Structure::canRemoveRowsImpl();
    }

    template <typename F>
    bool writeAt(const QModelIndex &index, F&& writer)
    {
        bool result = false;
        row_reference row = rowData(index);

        if constexpr (one_dimensional_range) {
            result = writer(row);
        } else if (QRangeModelDetails::isValid(row)) {
            if constexpr (dynamicColumns()) {
                result = writer(*QRangeModelDetails::pos(row, index.column()));
            } else {
                for_element_at(row, index.column(), [&writer, &result](auto &&target) {
                    using target_type = decltype(target);
                    // we can only assign to an lvalue reference
                    if constexpr (std::is_lvalue_reference_v<target_type>
                              && !std::is_const_v<std::remove_reference_t<target_type>>) {
                        result = writer(std::forward<target_type>(target));
                    }
                });
            }
        }

        return result;
    }

    template <typename F>
    void readAt(const QModelIndex &index, F&& reader) const {
        const_row_reference row = rowData(index);
        if constexpr (one_dimensional_range) {
            return reader(row);
        } else if (QRangeModelDetails::isValid(row)) {
            if constexpr (dynamicColumns())
                reader(*QRangeModelDetails::cpos(row, index.column()));
            else
                for_element_at(row, index.column(), std::forward<F>(reader));
        }
    }

    template <typename Value>
    static QVariant read(const Value &value)
    {
        if constexpr (std::is_constructible_v<QVariant, Value>)
            return QVariant(value);
        else
            return QVariant::fromValue(value);
    }
    template <typename Value>
    static QVariant read(Value *value)
    {
        if (value) {
            if constexpr (std::is_constructible_v<QVariant, Value *>)
                return QVariant(value);
            else
                return read(*value);
        }
        return {};
    }

    template <typename Target>
    static bool write(Target &target, const QVariant &value)
    {
        using Type = std::remove_reference_t<Target>;
        if constexpr (std::is_constructible_v<Target, QVariant>) {
            target = value;
            return true;
        } else if (value.canConvert<Type>()) {
            target = value.value<Type>();
            return true;
        }
        return false;
    }
    template <typename Target>
    static bool write(Target *target, const QVariant &value)
    {
        if (target)
            return write(*target, value);
        return false;
    }

    template <typename ItemType>
    QMetaProperty roleProperty(const QByteArray &roleName) const
    {
        const QMetaObject *mo = &ItemType::staticMetaObject;
        if (const int index = mo->indexOfProperty(roleName.data()); index >= 0)
            return mo->property(index);
        return {};
    }

    template <typename ItemType>
    QMetaProperty roleProperty(int role) const
    {
        struct {
            operator QMetaProperty() const {
                return that.roleProperty<ItemType>(that.itemModel().roleNames().value(role));
            }
            const QRangeModelImpl &that;
            const int role;
        } findProperty{*this, role};

        if constexpr (ModelData::cachesProperties)
            return *m_data.properties.tryEmplace(role, findProperty).iterator;
        else
            return findProperty;
    }

    template <typename ItemType>
    QVariant readRole(int role, ItemType *gadget) const
    {
        using item_type = std::remove_pointer_t<ItemType>;
        QVariant result;
        QMetaProperty prop = roleProperty<item_type>(role);
        if (!prop.isValid() && role == Qt::EditRole)
            prop = roleProperty<item_type>(Qt::DisplayRole);

        if (prop.isValid())
            result = readProperty(prop, gadget);
        return result;
    }

    template <typename ItemType>
    QVariant readRole(int role, const ItemType &gadget) const
    {
        return readRole(role, &gadget);
    }

    template <typename ItemType>
    static QVariant readProperty(const QMetaProperty &prop, ItemType *gadget)
    {
        if constexpr (std::is_base_of_v<QObject, ItemType>)
            return prop.read(gadget);
        else
            return prop.readOnGadget(gadget);
    }
    template <typename ItemType>
    static QVariant readProperty(int property, ItemType *gadget)
    {
        using item_type = std::remove_pointer_t<ItemType>;
        const QMetaObject &mo = item_type::staticMetaObject;
        const QMetaProperty prop = mo.property(property + mo.propertyOffset());
        return readProperty(prop, gadget);
    }

    template <typename ItemType>
    static QVariant readProperty(int property, const ItemType &gadget)
    {
        return readProperty(property, &gadget);
    }

    template <typename ItemType>
    bool writeRole(int role, ItemType *gadget, const QVariant &data)
    {
        using item_type = std::remove_pointer_t<ItemType>;
        auto prop = roleProperty<item_type>(role);
        if (!prop.isValid() && role == Qt::EditRole)
            prop = roleProperty<item_type>(Qt::DisplayRole);

        return writeProperty(prop, gadget, data);
    }

    template <typename ItemType>
    bool writeRole(int role, ItemType &&gadget, const QVariant &data)
    {
        return writeRole(role, &gadget, data);
    }

    template <typename ItemType>
    static bool writeProperty(const QMetaProperty &prop, ItemType *gadget, const QVariant &data)
    {
        if constexpr (std::is_base_of_v<QObject, ItemType>)
            return prop.write(gadget, data);
        else
            return prop.writeOnGadget(gadget, data);
    }
    template <typename ItemType>
    static bool writeProperty(int property, ItemType *gadget, const QVariant &data)
    {
        using item_type = std::remove_pointer_t<ItemType>;
        const QMetaObject &mo = item_type::staticMetaObject;
        return writeProperty(mo.property(property + mo.propertyOffset()), gadget, data);
    }

    template <typename ItemType>
    static bool writeProperty(int property, ItemType &&gadget, const QVariant &data)
    {
        return writeProperty(property, &gadget, data);
    }

    template <typename ItemType>
    static bool resetProperty(int property, ItemType *object)
    {
        using item_type = std::remove_pointer_t<ItemType>;
        const QMetaObject &mo = item_type::staticMetaObject;
        bool success = true;
        if (property == -1) {
            // reset all properties
            if constexpr (std::is_base_of_v<QObject, item_type>) {
                for (int p = mo.propertyOffset(); p < mo.propertyCount(); ++p)
                    success = writeProperty(mo.property(p), object, {}) && success;
            } else { // reset a gadget by assigning a default-constructed
                *object = {};
            }
        } else {
            success = writeProperty(mo.property(property + mo.propertyOffset()), object, {});
        }
        return success;
    }

    template <typename ItemType>
    static bool resetProperty(int property, ItemType &&object)
    {
        return resetProperty(property, &object);
    }

    // helpers
    const_row_reference rowData(const QModelIndex &index) const
    {
        Q_ASSERT(index.isValid());
        return that().rowDataImpl(index);
    }

    row_reference rowData(const QModelIndex &index)
    {
        Q_ASSERT(index.isValid());
        return that().rowDataImpl(index);
    }

    const range_type *childRange(const QModelIndex &index) const
    {
        if (!index.isValid())
            return m_data.model();
        if (index.column()) // only items at column 0 can have children
            return nullptr;
        return that().childRangeImpl(index);
    }

    range_type *childRange(const QModelIndex &index)
    {
        if (!index.isValid())
            return m_data.model();
        if (index.column()) // only items at column 0 can have children
            return nullptr;
        return that().childRangeImpl(index);
    }


    const protocol_type& protocol() const { return QRangeModelDetails::refTo(m_protocol); }
    protocol_type& protocol() { return QRangeModelDetails::refTo(m_protocol); }

    ModelData m_data;
    Protocol m_protocol;
};

// Implementations that depends on the model structure (flat vs tree) that will
// be specialized based on a protocol type. The main template implements tree
// support through a protocol type.
template <typename Range, typename Protocol>
class QGenericTreeItemModelImpl
    : public QRangeModelImpl<QGenericTreeItemModelImpl<Range, Protocol>, Range, Protocol>
{
    using Base = QRangeModelImpl<QGenericTreeItemModelImpl<Range, Protocol>, Range, Protocol>;
    friend class QRangeModelImpl<QGenericTreeItemModelImpl<Range, Protocol>, Range, Protocol>;

    using range_type = typename Base::range_type;
    using range_features = typename Base::range_features;
    using row_type = typename Base::row_type;
    using row_ptr = typename Base::row_ptr;
    using const_row_ptr = typename Base::const_row_ptr;

    using tree_traits = typename Base::protocol_traits;
    static constexpr bool is_mutable_impl = tree_traits::has_mutable_childRows;

    static constexpr bool rows_are_any_refs_or_pointers = Base::rows_are_raw_pointers ||
                                 QRangeModelDetails::is_smart_ptr<row_type>() ||
                                 QRangeModelDetails::is_any_of<row_type, std::reference_wrapper>();
    static_assert(!Base::dynamicColumns(), "A tree must have a static number of columns!");

public:
    QGenericTreeItemModelImpl(Range &&model, Protocol &&p, QRangeModel *itemModel)
        : Base(std::forward<Range>(model), std::forward<Protocol>(p), itemModel)
    {};

protected:
    QModelIndex indexImpl(int row, int column, const QModelIndex &parent) const
    {
        if (!parent.isValid())
            return this->createIndex(row, column);
        // only items at column 0 can have children
        if (parent.column())
            return QModelIndex();

        const_row_ptr grandParent = static_cast<const_row_ptr>(parent.constInternalPointer());
        const auto &parentSiblings = childrenOf(grandParent);
        const auto it = QRangeModelDetails::cpos(parentSiblings, parent.row());
        return this->createIndex(row, column, QRangeModelDetails::pointerTo(*it));
    }

    QModelIndex parent(const QModelIndex &child) const
    {
        if (!child.isValid())
            return {};

        // no pointer to parent row - no parent
        const_row_ptr parentRow = static_cast<const_row_ptr>(child.constInternalPointer());
        if (!parentRow)
            return {};

        // get the siblings of the parent via the grand parent
        auto &&grandParent = this->protocol().parentRow(QRangeModelDetails::refTo(parentRow));
        const range_type &parentSiblings = childrenOf(QRangeModelDetails::pointerTo(grandParent));
        // find the index of parentRow
        const auto begin = QRangeModelDetails::cbegin(parentSiblings);
        const auto end = QRangeModelDetails::cend(parentSiblings);
        const auto it = std::find_if(begin, end, [parentRow](auto &&s){
            return QRangeModelDetails::pointerTo(std::forward<decltype(s)>(s)) == parentRow;
        });
        if (it != end)
            return this->createIndex(std::distance(begin, it), 0,
                                     QRangeModelDetails::pointerTo(grandParent));
        return {};
    }

    int rowCount(const QModelIndex &parent) const
    {
        return Base::size(this->childRange(parent));
    }

    int columnCount(const QModelIndex &) const
    {
        // all levels of a tree have to have the same, static, column count
        if constexpr (Base::one_dimensional_range)
            return 1;
        else
            return Base::static_column_count; // if static_column_count is -1, static assert fires
    }

    static constexpr Qt::ItemFlags defaultFlags()
    {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    static constexpr bool canInsertRowsImpl()
    {
        // We must not insert rows if we cannot adjust the parents of the
        // children of the following rows. We don't have to do that if the
        // range operates on pointers.
        return (rows_are_any_refs_or_pointers || tree_traits::has_setParentRow)
             && Base::dynamicRows() && range_features::has_insert;
    }

    static constexpr bool canRemoveRowsImpl()
    {
        // We must not remove rows if we cannot adjust the parents of the
        // children of the following rows. We don't have to do that if the
        // range operates on pointers.
        return (rows_are_any_refs_or_pointers || tree_traits::has_setParentRow)
             && Base::dynamicRows() && range_features::has_erase;
    }

    static constexpr bool canMoveColumns(const QModelIndex &, const QModelIndex &)
    {
        return true;
    }

    static constexpr bool canMoveRows(const QModelIndex &, const QModelIndex &)
    {
        return true;
    }

    bool moveRowsAcross(const QModelIndex &sourceParent, int sourceRow, int count,
                        const QModelIndex &destParent, int destRow)
    {
        // If rows are pointers, then reference to the parent row don't
        // change, so we can move them around freely. Otherwise we need to
        // be able to explicitly update the parent pointer.
        if constexpr (!rows_are_any_refs_or_pointers && !tree_traits::has_setParentRow) {
            return false;
        } else if constexpr (!(range_features::has_insert && range_features::has_erase)) {
            return false;
        } else if (!this->beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1,
                                        destParent, destRow)) {
            return false;
        }

        range_type *source = this->childRange(sourceParent);
        range_type *destination = this->childRange(destParent);

        // If we can insert data from another range into, then
        // use that to move the old data over.
        const auto destStart = QRangeModelDetails::pos(destination, destRow);
        if constexpr (range_features::has_insert_range) {
            const auto sourceStart = QRangeModelDetails::pos(*source, sourceRow);
            const auto sourceEnd = std::next(sourceStart, count);

            destination->insert(destStart, std::move_iterator(sourceStart),
                                           std::move_iterator(sourceEnd));
        } else if constexpr (std::is_copy_constructible_v<row_type>) {
            // otherwise we have to make space first, and copy later.
            destination->insert(destStart, count, row_type{});
        }

        row_ptr parentRow = destParent.isValid()
                          ? QRangeModelDetails::pointerTo(this->rowData(destParent))
                          : nullptr;

        // if the source's parent was already inside the new parent row,
        // then the source row might have become invalid, so reset it.
        if (parentRow == static_cast<row_ptr>(sourceParent.internalPointer())) {
            if (sourceParent.row() < destRow) {
                source = this->childRange(sourceParent);
            } else {
                // the source parent moved down within destination
                source = this->childRange(this->createIndex(sourceParent.row() + count, 0,
                                                            sourceParent.internalPointer()));
            }
        }

        // move the data over and update the parent pointer
        {
            const auto writeStart = QRangeModelDetails::pos(destination, destRow);
            const auto writeEnd = std::next(writeStart, count);
            const auto sourceStart = QRangeModelDetails::pos(source, sourceRow);
            const auto sourceEnd = std::next(sourceStart, count);

            for (auto write = writeStart, read = sourceStart; write != writeEnd; ++write, ++read) {
                // move data over if not already done, otherwise
                // only fix the parent pointer
                if constexpr (!range_features::has_insert_range)
                    *write = std::move(*read);
                this->protocol().setParentRow(QRangeModelDetails::refTo(*write), parentRow);
            }
            // remove the old rows from the source parent
            source->erase(sourceStart, sourceEnd);
        }

        // Fix the parent pointers in children of both source and destination
        // ranges, as the references to the entries might have become invalid.
        // We don't have to do that if the rows are pointers, as in that case
        // the references to the entries are stable.
        resetParentInChildren(destination);
        resetParentInChildren(source);

        this->endMoveRows();
        return true;
    }

    auto makeEmptyRow(const QModelIndex &parent)
    {
        // tree traversal protocol: if we are here, then it must be possible
        // to change the parent of a row.
        static_assert(tree_traits::has_setParentRow);
        row_type empty_row = this->protocol().newRow();
        if (QRangeModelDetails::isValid(empty_row) && parent.isValid()) {
            this->protocol().setParentRow(QRangeModelDetails::refTo(empty_row),
                                        QRangeModelDetails::pointerTo(this->rowData(parent)));
        }
        return empty_row;
    }

    template <typename It, typename Sentinel>
    void deleteRemovedRows(It &&begin, Sentinel &&end)
    {
        if constexpr (tree_traits::has_deleteRow) {
            for (auto it = begin; it != end; ++it) {
                if constexpr (is_mutable_impl) {
                    decltype(auto) children = this->protocol().childRows(QRangeModelDetails::refTo(*it));
                    if (QRangeModelDetails::isValid(children)) {
                        deleteRemovedRows(QRangeModelDetails::begin(children),
                                          QRangeModelDetails::end(children));
                        QRangeModelDetails::refTo(children) = range_type{ };
                    }
                }

                this->protocol().deleteRow(std::move(*it));
            }
        }
    }

    void resetParentInChildren(range_type *children)
    {
        if constexpr (tree_traits::has_setParentRow && !rows_are_any_refs_or_pointers) {
            const auto begin = QRangeModelDetails::begin(*children);
            const auto end = QRangeModelDetails::end(*children);
            for (auto it = begin; it != end; ++it) {
                decltype(auto) maybeChildren = this->protocol().childRows(*it);
                if (QRangeModelDetails::isValid(maybeChildren)) {
                    auto &childrenRef = QRangeModelDetails::refTo(maybeChildren);
                    QModelIndexList fromIndexes;
                    QModelIndexList toIndexes;
                    fromIndexes.reserve(Base::size(childrenRef));
                    toIndexes.reserve(Base::size(childrenRef));
                    auto *parentRow = QRangeModelDetails::pointerTo(*it);

                    int row = 0;
                    for (auto &child : childrenRef) {
                        const_row_ptr oldParent = this->protocol().parentRow(child);
                        if (oldParent != parentRow) {
                            fromIndexes.append(this->createIndex(row, 0, oldParent));
                            toIndexes.append(this->createIndex(row, 0, parentRow));
                            this->protocol().setParentRow(child, parentRow);
                        }
                        ++row;
                    }
                    this->changePersistentIndexList(fromIndexes, toIndexes);
                    resetParentInChildren(&childrenRef);
                }
            }
        }
    }

    decltype(auto) rowDataImpl(const QModelIndex &index) const
    {
        const_row_ptr parentRow = static_cast<const_row_ptr>(index.constInternalPointer());
        const range_type &siblings = childrenOf(parentRow);
        Q_ASSERT(index.row() < int(Base::size(siblings)));
        return *QRangeModelDetails::cpos(siblings, index.row());
    }

    decltype(auto) rowDataImpl(const QModelIndex &index)
    {
        row_ptr parentRow = static_cast<row_ptr>(index.internalPointer());
        range_type &siblings = childrenOf(parentRow);
        Q_ASSERT(index.row() < int(Base::size(siblings)));
        return *QRangeModelDetails::pos(siblings, index.row());
    }

    const range_type *childRangeImpl(const QModelIndex &index) const
    {
        const auto &row = this->rowData(index);
        if (!QRangeModelDetails::isValid(row))
            return static_cast<const range_type *>(nullptr);

        decltype(auto) children = this->protocol().childRows(QRangeModelDetails::refTo(row));
        return QRangeModelDetails::pointerTo(std::forward<decltype(children)>(children));
    }

    range_type *childRangeImpl(const QModelIndex &index)
    {
        auto &row = this->rowData(index);
        if (!QRangeModelDetails::isValid(row))
            return static_cast<range_type *>(nullptr);

        decltype(auto) children = this->protocol().childRows(QRangeModelDetails::refTo(row));
        using Children = std::remove_reference_t<decltype(children)>;

        if constexpr (QRangeModelDetails::is_any_of<Children, std::optional>())
            if constexpr (std::is_default_constructible<typename Children::value_type>()) {
                if (!children)
                    children.emplace(range_type{});
            }

        return QRangeModelDetails::pointerTo(std::forward<decltype(children)>(children));
    }

    const range_type &childrenOf(const_row_ptr row) const
    {
        return row ? QRangeModelDetails::refTo(this->protocol().childRows(*row))
                   : *this->m_data.model();
    }

private:
    range_type &childrenOf(row_ptr row)
    {
        return row ? QRangeModelDetails::refTo(this->protocol().childRows(*row))
                   : *this->m_data.model();
    }
};

// specialization for flat models without protocol
template <typename Range>
class QGenericTableItemModelImpl
    : public QRangeModelImpl<QGenericTableItemModelImpl<Range>, Range>
{
    using Base = QRangeModelImpl<QGenericTableItemModelImpl<Range>, Range>;
    friend class QRangeModelImpl<QGenericTableItemModelImpl<Range>, Range>;

    using range_type = typename Base::range_type;
    using range_features = typename Base::range_features;
    using row_type = typename Base::row_type;
    using const_row_ptr = typename Base::const_row_ptr;
    using row_traits = typename Base::row_traits;
    using row_features = typename Base::row_features;

    static constexpr bool is_mutable_impl = true;

public:
    explicit QGenericTableItemModelImpl(Range &&model, QRangeModel *itemModel)
        : Base(std::forward<Range>(model), {}, itemModel)
    {}

protected:
    QModelIndex indexImpl(int row, int column, const QModelIndex &) const
    {
        if constexpr (Base::dynamicColumns()) {
            if (column < int(Base::size(*QRangeModelDetails::cpos(*this->m_data.model(), row))))
                return this->createIndex(row, column);
            // if we got here, then column < columnCount(), but this row is to short
            qCritical("QRangeModel: Column-range at row %d is not large enough!", row);
            return {};
        } else {
            return this->createIndex(row, column);
        }
    }

    QModelIndex parent(const QModelIndex &) const
    {
        return {};
    }

    int rowCount(const QModelIndex &parent) const
    {
        if (parent.isValid())
            return 0;
        return int(Base::size(*this->m_data.model()));
    }

    int columnCount(const QModelIndex &parent) const
    {
        if (parent.isValid())
            return 0;

        // in a table, all rows have the same number of columns (as the first row)
        if constexpr (Base::dynamicColumns()) {
            return int(Base::size(*this->m_data.model()) == 0
                       ? 0
                       : Base::size(*QRangeModelDetails::cbegin(*this->m_data.model())));
        } else if constexpr (Base::one_dimensional_range) {
            return row_traits::fixed_size();
        } else {
            return Base::static_column_count;
        }
    }

    static constexpr Qt::ItemFlags defaultFlags()
    {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
    }

    static constexpr bool canInsertRowsImpl()
    {
        return Base::dynamicRows() && range_features::has_insert;
    }

    static constexpr bool canRemoveRowsImpl()
    {
        return Base::dynamicRows() && range_features::has_erase;
    }

    static constexpr bool canMoveColumns(const QModelIndex &source, const QModelIndex &destination)
    {
        return !source.isValid() && !destination.isValid();
    }

    static constexpr bool canMoveRows(const QModelIndex &source, const QModelIndex &destination)
    {
        return !source.isValid() && !destination.isValid();
    }

    constexpr bool moveRowsAcross(const QModelIndex &, int , int,
                                  const QModelIndex &, int) noexcept
    {
        // table/flat model: can't move rows between different parents
        return false;
    }

    auto makeEmptyRow(const QModelIndex &)
    {
        row_type empty_row = this->protocol().newRow();

        // dynamically sized rows all have to have the same column count
        if constexpr (Base::dynamicColumns() && row_features::has_resize) {
            if (QRangeModelDetails::isValid(empty_row))
                QRangeModelDetails::refTo(empty_row).resize(this->itemModel().columnCount());
        }

        return empty_row;
    }

    template <typename It, typename Sentinel>
    void deleteRemovedRows(It &&begin, Sentinel &&end)
    {
        if constexpr (Base::protocol_traits::has_deleteRow) {
            for (auto it = begin; it != end; ++it)
                this->protocol().deleteRow(std::move(*it));
        }
    }

    decltype(auto) rowDataImpl(const QModelIndex &index) const
    {
        Q_ASSERT(q20::cmp_less(index.row(), Base::size(*this->m_data.model())));
        return *QRangeModelDetails::cpos(*this->m_data.model(), index.row());
    }

    decltype(auto) rowDataImpl(const QModelIndex &index)
    {
        Q_ASSERT(q20::cmp_less(index.row(), Base::size(*this->m_data.model())));
        return *QRangeModelDetails::pos(*this->m_data.model(), index.row());
    }

    const range_type *childRangeImpl(const QModelIndex &) const
    {
        return nullptr;
    }

    range_type *childRangeImpl(const QModelIndex &)
    {
        return nullptr;
    }

    const range_type &childrenOf(const_row_ptr row) const
    {
        Q_ASSERT(!row);
        return *this->m_data.model();
    }

    void resetParentInChildren(range_type *)
    {
    }
};

QT_END_NAMESPACE

#endif // Q_QDOC

#endif // QRANGEMODEL_IMPL_H
