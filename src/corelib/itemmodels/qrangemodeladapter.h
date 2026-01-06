// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QRANGEMODELADAPTER_H
#define QRANGEMODELADAPTER_H

#include <QtCore/qrangemodeladapter_impl.h>

QT_BEGIN_NAMESPACE

template <typename Range, typename Protocol = void, typename Model = QRangeModel>
class QT_TECH_PREVIEW_API QRangeModelAdapter
{
    using Impl = QRangeModelDetails::RangeImplementation<Range, Protocol>;
    using Storage = QRangeModelDetails::AdapterStorage<Model, Impl>;

    Storage storage;

#ifdef Q_QDOC
    using range_type = Range;
#else
    using range_type = QRangeModelDetails::wrapped_t<Range>;
#endif
    using const_row_reference = typename Impl::const_row_reference;
    using row_reference = typename Impl::row_reference;
    using range_features = typename QRangeModelDetails::range_traits<range_type>;
    using row_type = std::remove_reference_t<row_reference>;
    using row_features = QRangeModelDetails::range_traits<typename Impl::wrapped_row_type>;
    using row_ptr = typename Impl::wrapped_row_type *;
    using row_traits = typename Impl::row_traits;
    using item_type = std::remove_reference_t<typename row_traits::item_type>;
    using data_type = typename QRangeModelDetails::data_type<item_type>::type;
    using const_data_type = QRangeModelDetails::asConst_t<data_type>;
    using protocol_traits = typename Impl::protocol_traits;

    template <typename I> static constexpr bool is_list = I::protocol_traits::is_list;
    template <typename I> static constexpr bool is_table = I::protocol_traits::is_table;
    template <typename I> static constexpr bool is_tree = I::protocol_traits::is_tree;
    template <typename I> static constexpr bool canInsertColumns = I::dynamicColumns()
                                                                 && I::isMutable()
                                                                 && row_features::has_insert;
    template <typename I> static constexpr bool canRemoveColumns = I::dynamicColumns()
                                                                 && I::isMutable()
                                                                 && row_features::has_erase;

    template <typename I> using if_writable = std::enable_if_t<I::isMutable(), bool>;
    template <typename I> using if_list = std::enable_if_t<is_list<I>, bool>;
    template <typename I> using unless_list = std::enable_if_t<!is_list<I>, bool>;
    template <typename I> using if_table = std::enable_if_t<is_table<I>, bool>;
    template <typename I> using if_tree = std::enable_if_t<is_tree<I>, bool>;
    template <typename I> using unless_tree = std::enable_if_t<!is_tree<I>, bool>;
    template <typename I> using if_flat = std::enable_if_t<is_list<I> || is_table<I>, bool>;

    template <typename I>
    using if_canInsertRows = std::enable_if_t<I::canInsertRows(), bool>;
    template <typename I>
    using if_canRemoveRows = std::enable_if_t<I::canRemoveRows(), bool>;
    template <typename F>
    using if_canMoveItems = std::enable_if_t<F::has_rotate || F::has_splice, bool>;

    template <typename I>
    using if_canInsertColumns = std::enable_if_t<canInsertColumns<I>, bool>;
    template <typename I>
    using if_canRemoveColumns = std::enable_if_t<canRemoveColumns<I>, bool>;

    template <typename Row>
    static constexpr bool is_compatible_row = std::is_convertible_v<Row, const_row_reference>;
    template <typename Row>
    using if_compatible_row = std::enable_if_t<is_compatible_row<Row>, bool>;

    template <typename C>
    static constexpr bool is_compatible_row_range = is_compatible_row<
                                                          decltype(*std::begin(std::declval<C&>()))
                                                    >;
    template <typename C>
    using if_compatible_row_range = std::enable_if_t<is_compatible_row_range<C>, bool>;
    template <typename Data>
    static constexpr bool is_compatible_data = std::is_convertible_v<Data, data_type>;
    template <typename Data>
    using if_compatible_data = std::enable_if_t<is_compatible_data<Data>, bool>;
    template <typename C>
    static constexpr bool is_compatible_data_range = is_compatible_data<
                                                typename QRangeModelDetails::data_type<
                                                    typename QRangeModelDetails::row_traits<
                                                        decltype(*std::begin(std::declval<C&>()))
                                                    >::item_type
                                                >::type
                                            >;
    template <typename C>
    using if_compatible_column_data = std::enable_if_t<is_compatible_data<C>
                                                    || is_compatible_data_range<C>, bool>;
    template <typename C>
    using if_compatible_column_range = std::enable_if_t<is_compatible_data_range<C>, bool>;

    template <typename R>
    using if_assignable_range = std::enable_if_t<std::is_assignable_v<range_type, R>, bool>;

    friend class QRangeModel;
    template <typename T>
    static constexpr bool is_adapter = QRangeModelDetails::is_any_of<q20::remove_cvref_t<T>,
                                                                     QRangeModelAdapter>::value;
    template <typename T>
    using unless_adapter = std::enable_if_t<!is_adapter<T>, bool>;

    template <typename R, typename P>
    using if_compatible_model_params =
        std::enable_if_t<
           std::conjunction_v<
                std::disjunction<
                    std::is_convertible<R &&, Range>,
                    std::is_convertible<R &&, Range &&> // for C-arrays
                >,
                std::is_convertible<P, Protocol> // note, only void is expected to be convertible to void
         >, bool>;

#if !defined(Q_OS_VXWORKS) && !defined(Q_OS_INTEGRITY)
    // An adapter on a mutable range can make itself an adapter on a const
    // version of that same range. To make the constructor for a sub-range
    // accessible, befriend the mutable version. We can use more
    // generic pattern matching here, as we only use as input what asConst
    // might produce as output.
    template <typename T> static constexpr T asMutable(const T &);
    template <typename T> static constexpr T *asMutable(const T *);
    template <template <typename, typename...> typename U, typename T, typename ...Args>
    static constexpr U<T, Args...> asMutable(const U<const T, Args...> &);

    template <typename T>
    using asMutable_t = decltype(asMutable(std::declval<T>()));
    friend class QRangeModelAdapter<asMutable_t<Range>, Protocol, Model>;
#else
    template <typename R, typename P, typename M>
    friend class QRangeModelAdapter;
#endif

    explicit QRangeModelAdapter(const std::shared_ptr<QRangeModel> &model, const QModelIndex &root,
                                std::in_place_t) // disambiguate from range/protocol c'tor
        : storage{model, root}
    {}

    explicit QRangeModelAdapter(QRangeModel *model)
        : storage(model)
    {}

public:
    struct DataReference
    {
        using value_type = data_type;
        using const_value_type = const_data_type;
        using pointer = QRangeModelDetails::data_pointer_t<const_value_type>;

        explicit DataReference(const QModelIndex &index) noexcept
            : m_index(index)
        {
            Q_ASSERT_X(m_index.isValid(), "QRangeModelAdapter::at", "Index at position is invalid");
        }

        DataReference(const DataReference &other) = default;
        DataReference(DataReference &&other) = default;

        // reference (not std::reference_wrapper) semantics
        DataReference &operator=(const DataReference &other)
        {
            *this = other.get();
            return *this;
        }

        DataReference &operator=(DataReference &&other)
        {
            *this = other.get();
            return *this;
        }

        ~DataReference() = default;

        DataReference &operator=(const value_type &value)
        {
            assign(value);
            return *this;
        }

        DataReference &operator=(value_type &&value)
        {
            assign(std::move(value));
            return *this;
        }

        const_value_type get() const
        {
            Q_ASSERT_X(m_index.isValid(), "QRangeModelAdapter::at", "Index at position is invalid");
            return QRangeModelDetails::dataAtIndex<q20::remove_cvref_t<value_type>>(m_index);
        }

        operator const_value_type() const
        {
            return get();
        }

        pointer operator->() const
        {
            return {get()};
        }

        bool isValid() const { return m_index.isValid(); }

    private:
        QModelIndex m_index;

        template <typename Value>
        void assign(Value &&value)
        {
            constexpr Qt::ItemDataRole dataRole = Qt::RangeModelAdapterRole;

            if (m_index.isValid()) {
                auto model = const_cast<QAbstractItemModel *>(m_index.model());
                [[maybe_unused]] bool couldWrite = false;
                if constexpr (std::is_same_v<q20::remove_cvref_t<Value>, QVariant>) {
                    couldWrite = model->setData(m_index, value, dataRole);
                } else {
                    couldWrite = model->setData(m_index,
                                                QVariant::fromValue(std::forward<Value>(value)),
                                                dataRole);
                }
#ifndef QT_NO_DEBUG
                if (!couldWrite) {
                    qWarning() << "Writing value of type"
                               << QMetaType::fromType<q20::remove_cvref_t<Value>>().name()
                               << "to role" << dataRole << "at index" << m_index << "failed";
                }
            } else {
                qCritical("Data reference for invalid index, can't write to model");
#endif
            }
        }

        friend inline bool comparesEqual(const DataReference &lhs, const DataReference &rhs)
        {
            return lhs.m_index == rhs.m_index
                || lhs.get() == rhs.get();
        }
        Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(DataReference);

        friend inline bool comparesEqual(const DataReference &lhs, const value_type &rhs)
        {
            return lhs.get() == rhs;
        }
        Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(DataReference, value_type);

        friend inline void swap(DataReference lhs, DataReference rhs)
        {
            const value_type lhsValue = lhs.get();
            lhs = rhs;
            rhs = lhsValue; // no point in moving, we have to go through QVariant anyway
        }

#ifndef QT_NO_DEBUG_STREAM
        friend inline QDebug operator<<(QDebug dbg, const DataReference &ref)
        {
            return dbg << ref.get();
        }
#endif
#ifndef QT_NO_DATASTREAM
        friend inline QDataStream &operator<<(QDataStream &ds, const DataReference &ref)
        {
            return ds << ref.get();
        }
        friend inline QDataStream &operator>>(QDataStream &ds, DataReference &ref)
        {
            value_type value;
            ds >> value;
            ref = value;
            return ds;
        }
#endif
    };
    template <typename Iterator, typename Adapter>
    struct ColumnIteratorBase
    {
        using iterator_category = std::random_access_iterator_tag;
        using difference_type = int;

        ColumnIteratorBase() = default;
        ColumnIteratorBase(const ColumnIteratorBase &other) = default;
        ColumnIteratorBase(ColumnIteratorBase &&other) = default;
        ColumnIteratorBase &operator=(const ColumnIteratorBase &other) = default;
        ColumnIteratorBase &operator=(ColumnIteratorBase &&other) = default;

        ColumnIteratorBase(const QModelIndex &rowIndex, int column, Adapter *adapter) noexcept
            : m_rowIndex(rowIndex), m_column(column), m_adapter(adapter)
        {
        }

        void swap(ColumnIteratorBase &other) noexcept
        {
            qSwap(m_rowIndex, other.m_rowIndex);
            qSwap(m_column, other.m_column);
            q_ptr_swap(m_adapter, other.m_adapter);
        }

        friend Iterator &operator++(Iterator &that) noexcept
        {
            ++that.m_column;
            return that;
        }
        friend Iterator operator++(Iterator &that, int) noexcept
        {
            auto copy = that;
            ++that;
            return copy;
        }
        friend Iterator operator+(const Iterator &that, difference_type n) noexcept
        {
            return {that.m_rowIndex, that.m_column + n, that.m_adapter};
        }
        friend Iterator operator+(difference_type n, const Iterator &that) noexcept
        {
            return that + n;
        }
        friend Iterator &operator+=(Iterator &that, difference_type n) noexcept
        {
            that.m_column += n;
            return that;
        }

        friend Iterator &operator--(Iterator &that) noexcept
        {
            --that.m_column;
            return that;
        }
        friend Iterator operator--(Iterator &that, int) noexcept
        {
            auto copy = that;
            --that;
            return copy;
        }
        friend Iterator operator-(const Iterator &that, difference_type n) noexcept
        {
            return {that.m_rowIndex, that.m_column - n, that.m_adapter};
        }
        friend Iterator operator-(difference_type n, const Iterator &that) noexcept
        {
            return that - n;
        }
        friend Iterator &operator-=(Iterator &that, difference_type n) noexcept
        {
            that.m_column -= n;
            return that;
        }

        friend difference_type operator-(const Iterator &lhs, const Iterator &rhs) noexcept
        {
            Q_PRE(lhs.m_rowIndex == rhs.m_rowIndex);
            Q_PRE(lhs.m_adapter == rhs.m_adapter);
            return lhs.m_column - rhs.m_column;
        }

    protected:
        ~ColumnIteratorBase() = default;
        QModelIndex m_rowIndex;
        int m_column = -1;
        Adapter *m_adapter = nullptr;

    private:
        friend bool comparesEqual(const Iterator &lhs, const Iterator &rhs)
        {
            Q_ASSERT(lhs.m_rowIndex == rhs.m_rowIndex);
            return lhs.m_column == rhs.m_column;
        }
        friend Qt::strong_ordering compareThreeWay(const Iterator &lhs, const Iterator &rhs)
        {
            Q_ASSERT(lhs.m_rowIndex == rhs.m_rowIndex);
            return qCompareThreeWay(lhs.m_column, rhs.m_column);
        }

        Q_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT(Iterator)

#ifndef QT_NO_DEBUG_STREAM
        friend inline QDebug operator<<(QDebug dbg, const Iterator &it)
        {
            QDebugStateSaver saver(dbg);
            dbg.nospace();
            return dbg << "ColumnIterator(" << it.m_rowIndex.siblingAtColumn(it.m_column) << ")";
        }
#endif
    };

    struct ConstColumnIterator : ColumnIteratorBase<ConstColumnIterator, const QRangeModelAdapter>
    {
        using Base = ColumnIteratorBase<ConstColumnIterator, const QRangeModelAdapter>;
        using difference_type = typename Base::difference_type;
        using value_type = data_type;
        using reference = const_data_type;
        using pointer = QRangeModelDetails::data_pointer_t<value_type>;

        using Base::Base;
        using Base::operator=;
        ~ConstColumnIterator() = default;

        pointer operator->() const
        {
            return pointer{operator*()};
        }

        reference operator*() const
        {
            return std::as_const(this->m_adapter)->at(this->m_rowIndex.row(), this->m_column);
        }

        reference operator[](difference_type n) const
        {
            return *(*this + n);
        }
    };

    struct ColumnIterator : ColumnIteratorBase<ColumnIterator, QRangeModelAdapter>
    {
        using Base = ColumnIteratorBase<ColumnIterator, QRangeModelAdapter>;
        using difference_type = typename Base::difference_type;
        using value_type = DataReference;
        using reference = DataReference;
        using pointer = reference;

        using Base::Base;
        using Base::operator=;
        ~ColumnIterator() = default;

        operator ConstColumnIterator() const
        {
            return ConstColumnIterator{this->m_rowIndex, this->m_column, this->m_adapter};
        }

        pointer operator->() const
        {
            return operator*();
        }

        reference operator*() const
        {
            return reference{this->m_rowIndex.siblingAtColumn(this->m_column)};
        }

        reference operator[](difference_type n) const
        {
            return *(*this + n);
        }
    };

    template <typename Reference, typename const_row_type, typename = void>
    struct RowGetter
    {
        const_row_type get() const
        {
            const Reference *that = static_cast<const Reference *>(this);
            const auto *impl = that->m_adapter->storage.implementation();
            auto *childRange = impl->childRange(that->m_index.parent());
            if constexpr (std::is_convertible_v<const row_type &, const_row_type>) {
                return *std::next(QRangeModelDetails::adl_begin(childRange), that->m_index.row());
            } else {
                const auto &row = *std::next(QRangeModelDetails::adl_begin(childRange),
                                             that->m_index.row());
                return const_row_type{QRangeModelDetails::adl_begin(row),
                                      QRangeModelDetails::adl_end(row)};
            }
        }

        const_row_type operator->() const
        {
            return {get()};
        }

        operator const_row_type() const
        {
            return get();
        }
    };

    template <typename Reference, typename const_row_type>
    struct RowGetter<Reference, const_row_type,
                     std::enable_if_t<std::is_reference_v<const_row_type>>>
    {
        const_row_type get() const
        {
            const Reference *that = static_cast<const Reference *>(this);
            const auto *impl = that->m_adapter->storage.implementation();
            return *std::next(QRangeModelDetails::begin(
                                QRangeModelDetails::refTo(impl->childRange(that->m_index.parent()))),
                              that->m_index.row());
        }

        auto operator->() const
        {
            return std::addressof(get());
        }

        operator const_row_type() const
        {
            return get();
        }
    };

    template <typename Reference, typename const_row_type>
    struct RowGetter<Reference, const_row_type,
                     std::enable_if_t<std::is_pointer_v<const_row_type>>>
    {
        const_row_type get() const
        {
            const Reference *that = static_cast<const Reference *>(this);
            const auto *impl = that->m_adapter->storage.implementation();
            return *std::next(QRangeModelDetails::begin(
                                QRangeModelDetails::refTo(impl->childRange(that->m_index.parent()))),
                              that->m_index.row());
        }

        const_row_type operator->() const
        {
            return get();
        }

        operator const_row_type() const
        {
            return get();
        }
    };

    template <typename Reference, typename Adapter>
    struct RowReferenceBase : RowGetter<Reference, QRangeModelDetails::asConstRow_t<row_type>>
    {
        using const_iterator = ConstColumnIterator;
        using size_type = int;
        using difference_type = int;
        using const_row_type = QRangeModelDetails::asConstRow_t<row_type>;

        RowReferenceBase(const QModelIndex &index, Adapter *adapter) noexcept
            : m_index(index), m_adapter(adapter)
        {}

        template <typename I = Impl, if_tree<I> = true>
        bool hasChildren() const
        {
            return m_adapter->model()->hasChildren(m_index);
        }

        template <typename I = Impl, if_tree<I> = true>
        auto children() const
        {
            using ConstRange = QRangeModelDetails::asConst_t<Range>;
            return QRangeModelAdapter<ConstRange, Protocol, Model>(m_adapter->storage.m_model,
                                                                   m_index, std::in_place);
        }

        ConstColumnIterator cbegin() const
        {
            return ConstColumnIterator{m_index, 0, m_adapter};
        }
        ConstColumnIterator cend() const
        {
            return ConstColumnIterator{m_index, m_adapter->columnCount(), m_adapter};
        }

        ConstColumnIterator begin() const { return cbegin(); }
        ConstColumnIterator end() const { return cend(); }

        size_type size() const
        {
            return m_adapter->columnCount();
        }

        auto at(int column) const
        {
            Q_ASSERT(column >= 0 && column < m_adapter->columnCount());
            return *ConstColumnIterator{m_index, column, m_adapter};
        }

        auto operator[](int column) const
        {
            return at(column);
        }

    protected:
        friend struct RowGetter<Reference, const_row_type>;
        ~RowReferenceBase() = default;
        QModelIndex m_index;
        Adapter *m_adapter;

    private:
        friend bool comparesEqual(const Reference &lhs, const Reference &rhs)
        {
            Q_ASSERT(lhs.m_adapter == rhs.m_adapter);
            return lhs.m_index == rhs.m_index;
        }
        friend Qt::strong_ordering compareThreeWay(const Reference &lhs, const Reference &rhs)
        {
            Q_ASSERT(lhs.m_adapter == rhs.m_adapter);
            return qCompareThreeWay(lhs.m_index, rhs.m_index);
        }

        Q_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT(Reference)

        friend bool comparesEqual(const Reference &lhs, const row_type &rhs)
        {
            return lhs.get() == rhs;
        }
        Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(Reference, row_type)

#ifndef QT_NO_DEBUG_STREAM
        friend inline QDebug operator<<(QDebug dbg, const Reference &ref)
        {
            QDebugStateSaver saver(dbg);
            dbg.nospace();
            return dbg << "RowReference(" << ref.m_index << ")";
        }
#endif
#ifndef QT_NO_DATASTREAM
        friend inline QDataStream &operator<<(QDataStream &ds, const Reference &ref)
        {
            return ds << ref.get();
        }
#endif
    };

    struct ConstRowReference : RowReferenceBase<ConstRowReference, const QRangeModelAdapter>
    {
        using Base = RowReferenceBase<ConstRowReference, const QRangeModelAdapter>;
        using Base::Base;

        ConstRowReference() = default;
        ConstRowReference(const ConstRowReference &) = default;
        ConstRowReference(ConstRowReference &&) = default;
        ConstRowReference &operator=(const ConstRowReference &) = default;
        ConstRowReference &operator=(ConstRowReference &&) = default;
        ~ConstRowReference() = default;
    };

    struct RowReference : RowReferenceBase<RowReference, QRangeModelAdapter>
    {
        using Base = RowReferenceBase<RowReference, QRangeModelAdapter>;
        using iterator = ColumnIterator;
        using const_iterator = typename Base::const_iterator;
        using size_type = typename Base::size_type;
        using difference_type = typename Base::difference_type;
        using const_row_type = typename Base::const_row_type;

        using Base::Base;
        RowReference() = delete;
        ~RowReference() = default;
        RowReference(const RowReference &other) = default;
        RowReference(RowReference &&other) = default;

        // assignment has reference (std::reference_wrapper) semantics
        RowReference &operator=(const ConstRowReference &other)
        {
            *this = other.get();
            return *this;
        }

        RowReference &operator=(const RowReference &other)
        {
            *this = other.get();
            return *this;
        }

        RowReference &operator=(const row_type &other)
        {
            assign(other);
            return *this;
        }

        RowReference &operator=(row_type &&other)
        {
            assign(std::move(other));
            return *this;
        }

        operator ConstRowReference() const
        {
            return ConstRowReference{this->m_index, this->m_adapter};
        }

        template <typename ConstRowType = const_row_type,
                  std::enable_if_t<!std::is_same_v<ConstRowType, const row_type &>, bool> = true>
        RowReference &operator=(const ConstRowType &other)
        {
            assign(other);
            return *this;
        }

        template <typename T, typename It, typename Sentinel>
        RowReference &operator=(const QRangeModelDetails::RowView<T, It, Sentinel> &other)
        {
            *this = row_type{other.begin(), other.end()};
            return *this;
        }

        friend inline void swap(RowReference lhs, RowReference rhs)
        {
            auto lhsRow = lhs.get();
            lhs = rhs.get();
            rhs = std::move(lhsRow);
        }

        template <typename I = Impl, if_tree<I> = true>
        auto children()
        {
            return QRangeModelAdapter(this->m_adapter->storage.m_model, this->m_index,
                                      std::in_place);
        }

        using Base::begin;
        ColumnIterator begin()
        {
            return ColumnIterator{this->m_index, 0, this->m_adapter};
        }

        using Base::end;
        ColumnIterator end()
        {
            return ColumnIterator{this->m_index, this->m_adapter->columnCount(), this->m_adapter};
        }

        using Base::at;
        auto at(int column)
        {
            Q_ASSERT(column >= 0 && column < this->m_adapter->columnCount());
            return *ColumnIterator{this->m_index, column, this->m_adapter};
        }

        using Base::operator[];
        auto operator[](int column)
        {
            return at(column);
        }

    private:
        template <typename RHS>
        void verifyRows(const row_type &oldRow, const RHS &newRow)
        {
            if constexpr (QRangeModelDetails::test_size<row_type>::value) {
                // prevent that tables get populated with wrongly sized rows
                Q_ASSERT_X(Impl::size(newRow) == Impl::size(oldRow),
                           "RowReference::operator=()",
                           "The new row has the wrong size!");
            }

            if constexpr (is_tree<Impl>) {
                // we cannot hook invalid rows up to the tree hierarchy
                Q_ASSERT_X(QRangeModelDetails::isValid(newRow),
                           "RowReference::operator=()",
                           "An invalid row can not inserted into a tree!");
            }
        }

        template <typename R>
        void assign(R &&other)
        {
            auto *impl = this->m_adapter->storage.implementation();
            decltype(auto) oldRow = impl->rowData(this->m_index);

            verifyRows(oldRow, other);

            if constexpr (is_tree<Impl>) {
                auto &protocol = impl->protocol();
                auto *oldParent = protocol.parentRow(QRangeModelDetails::refTo(oldRow));

                // the old children will be removed; we don't try to overwrite
                // them with the new children, we replace them completely
                if (decltype(auto) oldChildren = protocol.childRows(QRangeModelDetails::refTo(oldRow));
                    QRangeModelDetails::isValid(oldChildren)) {
                    if (int oldChildCount = this->m_adapter->model()->rowCount(this->m_index)) {
                        impl->beginRemoveRows(this->m_index, 0, oldChildCount - 1);
                        impl->deleteRemovedRows(QRangeModelDetails::refTo(oldChildren));
                        // make sure the list is empty before we emit rowsRemoved
                        QRangeModelDetails::refTo(oldChildren) = range_type{};
                        impl->endRemoveRows();
                    }
                }

                if constexpr (protocol_traits::has_deleteRow)
                    protocol.deleteRow(oldRow);
                oldRow = std::forward<R>(other);
                if constexpr (protocol_traits::has_setParentRow) {
                    protocol.setParentRow(QRangeModelDetails::refTo(oldRow), oldParent);
                    if (decltype(auto) newChildren = protocol.childRows(QRangeModelDetails::refTo(oldRow));
                        QRangeModelDetails::isValid(newChildren)) {
                        impl->beginInsertRows(this->m_index, 0,
                                              Impl::size(QRangeModelDetails::refTo(newChildren)) - 1);
                        impl->setParentRow(QRangeModelDetails::refTo(newChildren),
                                           QRangeModelDetails::pointerTo(oldRow));
                        impl->endInsertRows();
                    }
                }
            } else {
                oldRow = std::forward<R>(other);
            }
            this->m_adapter->emitDataChanged(this->m_index,
                                             this->m_index.siblingAtColumn(this->m_adapter->columnCount() - 1));
            if constexpr (Impl::itemsAreQObjects) {
                if (this->m_adapter->model()->autoConnectPolicy() == QRangeModel::AutoConnectPolicy::Full) {
                    impl->autoConnectPropertiesInRow(oldRow, this->m_index.row(), this->m_index.parent());
                    if constexpr (is_tree<Impl>)
                        impl->autoConnectProperties(this->m_index);
                }

            }
        }

#ifndef QT_NO_DATASTREAM
        friend inline QDataStream &operator>>(QDataStream &ds, RowReference &ref)
        {
            row_type value;
            ds >> value;
            ref = value;
            return ds;
        }
#endif
    };

    template <typename Iterator, typename Adapter>
    struct RowIteratorBase : QRangeModelDetails::ParentIndex<is_tree<Impl>>
    {
        using iterator_category = std::random_access_iterator_tag;
        using difference_type = int;

        RowIteratorBase() = default;
        RowIteratorBase(const RowIteratorBase &) = default;
        RowIteratorBase(RowIteratorBase &&) = default;
        RowIteratorBase &operator=(const RowIteratorBase &) = default;
        RowIteratorBase &operator=(RowIteratorBase &&) = default;

        RowIteratorBase(int row, const QModelIndex &parent, Adapter *adapter)
            : QRangeModelDetails::ParentIndex<is_tree<Impl>>{parent}
            , m_row(row), m_adapter(adapter)
        {}

        void swap(RowIteratorBase &other) noexcept
        {
            qSwap(m_row, other.m_row);
            qSwap(this->m_rootIndex, other.m_rootIndex);
            q_ptr_swap(m_adapter, other.m_adapter);
        }

        friend Iterator &operator++(Iterator &that) noexcept
        {
            ++that.m_row;
            return that;
        }
        friend Iterator operator++(Iterator &that, int) noexcept
        {
            auto copy = that;
            ++that;
            return copy;
        }
        friend Iterator operator+(const Iterator &that, difference_type n) noexcept
        {
            return {that.m_row + n, that.root(), that.m_adapter};
        }
        friend Iterator operator+(difference_type n, const Iterator &that) noexcept
        {
            return that + n;
        }
        friend Iterator &operator+=(Iterator &that, difference_type n) noexcept
        {
            that.m_row += n;
            return that;
        }

        friend Iterator &operator--(Iterator &that) noexcept
        {
            --that.m_row;
            return that;
        }
        friend Iterator operator--(Iterator &that, int) noexcept
        {
            auto copy = that;
            --that;
            return copy;
        }
        friend Iterator operator-(const Iterator &that, difference_type n) noexcept
        {
            return {that.m_row - n, that.root(), that.m_adapter};
        }
        friend Iterator operator-(difference_type n, const Iterator &that) noexcept
        {
            return that - n;
        }
        friend Iterator &operator-=(Iterator &that, difference_type n) noexcept
        {
            that.m_row -= n;
            return that;
        }

        friend difference_type operator-(const Iterator &lhs, const Iterator &rhs) noexcept
        {
            return lhs.m_row - rhs.m_row;
        }

    protected:
        ~RowIteratorBase() = default;
        int m_row = -1;
        Adapter *m_adapter = nullptr;

    private:
        friend bool comparesEqual(const Iterator &lhs, const Iterator &rhs) noexcept
        {
            return lhs.m_row == rhs.m_row && lhs.root() == rhs.root();
        }
        friend Qt::strong_ordering compareThreeWay(const Iterator &lhs, const Iterator &rhs) noexcept
        {
            if (lhs.root() == rhs.root())
                return qCompareThreeWay(lhs.m_row, rhs.m_row);
            return qCompareThreeWay(lhs.root(), rhs.root());
        }

        Q_DECLARE_STRONGLY_ORDERED(Iterator)

#ifndef QT_NO_DEBUG_STREAM
        friend inline QDebug operator<<(QDebug dbg, const Iterator &it)
        {
            QDebugStateSaver saver(dbg);
            dbg.nospace();
            return dbg << "RowIterator(" << it.m_row << it.root() << ")";
        }
#endif
    };

public:
    struct ConstRowIterator : public RowIteratorBase<ConstRowIterator, const QRangeModelAdapter>
    {
        using Base = RowIteratorBase<ConstRowIterator, const QRangeModelAdapter>;
        using Base::Base;

        using difference_type = typename Base::difference_type;
        using value_type = std::conditional_t<is_list<Impl>,
                                              const_data_type,
                                              ConstRowReference>;
        using reference = std::conditional_t<is_list<Impl>,
                                             const_data_type,
                                             ConstRowReference>;
        using pointer = std::conditional_t<is_list<Impl>,
                                           QRangeModelDetails::data_pointer_t<const_data_type>,
                                           ConstRowReference>;

        ConstRowIterator(const ConstRowIterator &other) = default;
        ConstRowIterator(ConstRowIterator &&other) = default;
        ConstRowIterator &operator=(const ConstRowIterator &other) = default;
        ConstRowIterator &operator=(ConstRowIterator &&other) = default;
        ~ConstRowIterator() = default;

        pointer operator->() const
        {
            return pointer{operator*()};
        }

        reference operator*() const
        {
            if constexpr (is_list<Impl>) {
                return this->m_adapter->at(this->m_row);
            } else {
                const QModelIndex index = this->m_adapter->model()->index(this->m_row, 0,
                                                                          this->root());
                return ConstRowReference{index, this->m_adapter};
            }
        }

        reference operator[](difference_type n) const
        {
            return *(*this + n);
        }
    };

    struct RowIterator : public RowIteratorBase<RowIterator, QRangeModelAdapter>
    {
        using Base = RowIteratorBase<RowIterator, QRangeModelAdapter>;
        using Base::Base;

        using difference_type = typename Base::difference_type;
        using value_type = std::conditional_t<is_list<Impl>,
                                              DataReference,
                                              RowReference>;
        using reference = std::conditional_t<is_list<Impl>,
                                             DataReference,
                                             RowReference>;
        using pointer = std::conditional_t<is_list<Impl>,
                                           DataReference,
                                           RowReference>;

        RowIterator(const RowIterator &other) = default;
        RowIterator(RowIterator &&other) = default;
        RowIterator &operator=(const RowIterator &other) = default;
        RowIterator &operator=(RowIterator &&other) = default;
        ~RowIterator() = default;

        operator ConstRowIterator() const
        {
            return ConstRowIterator{this->m_row, this->root(), this->m_adapter};
        }

        pointer operator->() const
        {
            return pointer{operator*()};
        }

        reference operator*() const
        {
            const QModelIndex index = this->m_adapter->model()->index(this->m_row, 0, this->root());
            if constexpr (is_list<Impl>) {
                return reference{index};
            } else {
                return reference{index, this->m_adapter};
            }
        }

        reference operator[](difference_type n) const
        {
            return *(*this + n);
        }
    };

    using const_iterator = ConstRowIterator;
    using iterator = RowIterator;

    template <typename R,
              typename P,
              if_compatible_model_params<R, P> = true>
    explicit QRangeModelAdapter(R &&range, P &&protocol)
        : QRangeModelAdapter(new Model(QRangeModelDetails::forwardOrConvert<Range>(std::forward<R>(range)),
                                       QRangeModelDetails::forwardOrConvert<Protocol>(std::forward<P>(protocol))))
    {}

    template <typename R,
              typename P = void, // to enable the ctr for void protocols only
              if_compatible_model_params<R, P> = true,
              unless_adapter<R> = true>
    explicit QRangeModelAdapter(R &&range)
        : QRangeModelAdapter(new Model(QRangeModelDetails::forwardOrConvert<Range>(std::forward<R>(range))))
    {}

    // compiler-generated copy/move SMF are fine!

    Model *model() const
    {
        return storage.m_model.get();
    }

    const range_type &range() const
    {
        return QRangeModelDetails::refTo(storage.implementation()->childRange(storage.root()));
    }

    Q_IMPLICIT operator const range_type &() const
    {
        return range();
    }

    template <typename NewRange = range_type, if_assignable_range<NewRange> = true>
    void setRange(NewRange &&newRange)
    {
        setRangeImpl(qsizetype(Impl::size(QRangeModelDetails::refTo(newRange))) - 1,
            [&newRange](auto &oldRange) {
            oldRange = std::forward<NewRange>(newRange);
        });
    }

    template <typename NewRange = range_type, if_assignable_range<NewRange> = true,
              unless_adapter<NewRange> = true>
    QRangeModelAdapter &operator=(NewRange &&newRange)
    {
        setRange(std::forward<NewRange>(newRange));
        return *this;
    }

    template <typename Row, if_assignable_range<std::initializer_list<Row>> = true>
    void setRange(std::initializer_list<Row> newRange)
    {
        setRangeImpl(qsizetype(newRange.size() - 1), [&newRange](auto &oldRange) {
            oldRange = newRange;
        });
    }

    template <typename Row, if_assignable_range<std::initializer_list<Row>> = true>
    QRangeModelAdapter &operator=(std::initializer_list<Row> newRange)
    {
        setRange(newRange);
        return *this;
    }

    template <typename Row, if_assignable_range<std::initializer_list<Row>> = true>
    void assign(std::initializer_list<Row> newRange)
    {
        setRange(newRange);
    }

    template <typename InputIterator, typename Sentinel, typename I = Impl, if_writable<I> = true>
    void setRange(InputIterator first, Sentinel last)
    {
        setRangeImpl(qsizetype(std::distance(first, last) - 1), [first, last](auto &oldRange) {
            oldRange.assign(first, last);
        });
    }

    template <typename InputIterator, typename Sentinel, typename I = Impl, if_writable<I> = true>
    void assign(InputIterator first, Sentinel last)
    {
        setRange(first, last);
    }

    // iterator API
    ConstRowIterator cbegin() const
    {
        return ConstRowIterator{ 0, storage.root(), this };
    }
    ConstRowIterator begin() const { return cbegin(); }

    ConstRowIterator cend() const
    {
        return ConstRowIterator{ rowCount(), storage.root(), this };
    }
    ConstRowIterator end() const { return cend(); }

    template <typename I = Impl, if_writable<I> = true>
    RowIterator begin()
    {
        return RowIterator{ 0, storage.root(), this };
    }

    template <typename I = Impl, if_writable<I> = true>
    RowIterator end()
    {
        return RowIterator{ rowCount(), storage.root(), this };
    }

    int size() const
    {
        return rowCount();
    }

    template <typename I = Impl, if_list<I> = true>
    QModelIndex index(int row) const
    {
        return storage->index(row, 0, storage.root());
    }

    template <typename I = Impl, unless_list<I> = true>
    QModelIndex index(int row, int column) const
    {
        return storage->index(row, column, storage.root());
    }

    template <typename I = Impl, if_tree<I> = true>
    QModelIndex index(QSpan<const int> path, int col) const
    {
        Q_PRE(path.size());
        QModelIndex result = storage.root();
        auto count = path.size();
        for (const int r : path) {
            if (--count)
                result = storage->index(r, 0, result);
            else
                result = storage->index(r, col, result);
        }
        return result;
    }

    int columnCount() const
    {
        // all rows and tree branches have the same column count
        return storage->columnCount({});
    }

    int rowCount() const
    {
        return storage->rowCount(storage.root());
    }

    template <typename I = Impl, if_tree<I> = true>
    int rowCount(int row) const
    {
        return storage->rowCount(index(row, 0));
    }

    template <typename I = Impl, if_tree<I> = true>
    int rowCount(QSpan<const int> path) const
    {
        return storage->rowCount(index(path, 0));
    }

    template <typename I = Impl, if_tree<I> = true>
    constexpr bool hasChildren(int row) const
    {
        return storage.m_model->hasChildren(index(row, 0));
    }

    template <typename I = Impl, if_tree<I> = true>
    constexpr bool hasChildren(QSpan<const int> path) const
    {
        return storage.m_model->hasChildren(index(path, 0));
    }

    template <typename I = Impl, if_list<I> = true>
    QVariant data(int row) const
    {
        return QRangeModelDetails::dataAtIndex<QVariant>(index(row));
    }

    template <typename I = Impl, if_list<I> = true>
    QVariant data(int row, int role) const
    {
        return QRangeModelDetails::dataAtIndex<QVariant>(index(row), role);
    }

    template <typename I = Impl, if_list<I> = true, if_writable<I> = true>
    bool setData(int row, const QVariant &value, int role = Qt::EditRole)
    {
        return storage->setData(index(row), value, role);
    }

    template <typename I = Impl, unless_list<I> = true>
    QVariant data(int row, int column) const
    {
        return QRangeModelDetails::dataAtIndex<QVariant>(index(row, column));
    }

    template <typename I = Impl, unless_list<I> = true>
    QVariant data(int row, int column, int role) const
    {
        return QRangeModelDetails::dataAtIndex<QVariant>(index(row, column), role);
    }

    template <typename I = Impl, unless_list<I> = true, if_writable<I> = true>
    bool setData(int row, int column, const QVariant &value, int role = Qt::EditRole)
    {
        return storage->setData(index(row, column), value, role);
    }

    template <typename I = Impl, if_tree<I> = true>
    QVariant data(QSpan<const int> path, int column) const
    {
        return QRangeModelDetails::dataAtIndex<QVariant>(index(path, column));
    }

    template <typename I = Impl, if_tree<I> = true>
    QVariant data(QSpan<const int> path, int column, int role) const
    {
        return QRangeModelDetails::dataAtIndex<QVariant>(index(path, column), role);
    }

    template <typename I = Impl, if_tree<I> = true, if_writable<I> = true>
    bool setData(QSpan<const int> path, int column, const QVariant &value, int role = Qt::EditRole)
    {
        return storage->setData(index(path, column), value, role);
    }

    // at/operator[int] for list: returns value at row
    // if multi-role value: return the entire object
    template <typename I= Impl, if_list<I> = true>
    const_data_type at(int row) const
    {
        return QRangeModelDetails::dataAtIndex<data_type>(index(row));
    }
    template <typename I = Impl, if_list<I> = true>
    const_data_type operator[](int row) const { return at(row); }

    template <typename I= Impl, if_list<I> = true, if_writable<I> = true>
    auto at(int row) { return DataReference{this->index(row)}; }
    template <typename I = Impl, if_list<I> = true, if_writable<I> = true>
    auto operator[](int row) { return DataReference{this->index(row)}; }

    // at/operator[int] for table or tree: a reference or view of the row
    template <typename I = Impl, unless_list<I> = true>
    decltype(auto) at(int row) const
    {
        return ConstRowReference{index(row, 0), this}.get();
    }
    template <typename I = Impl, unless_list<I> = true>
    decltype(auto) operator[](int row) const { return at(row); }

    template <typename I = Impl, if_table<I> = true, if_writable<I> = true>
    auto at(int row)
    {
        return RowReference{index(row, 0), this};
    }
    template <typename I = Impl, if_table<I> = true, if_writable<I> = true>
    auto operator[](int row) { return at(row); }

    // at/operator[int, int] for table: returns value at row/column
    template <typename I = Impl, unless_list<I> = true>
    const_data_type at(int row, int column) const
    {
        return QRangeModelDetails::dataAtIndex<data_type>(index(row, column));
    }

#ifdef __cpp_multidimensional_subscript
    template <typename I = Impl, unless_list<I> = true>
    const_data_type operator[](int row, int column) const { return at(row, column); }
#endif

    template <typename I = Impl, unless_list<I> = true, if_writable<I> = true>
    auto at(int row, int column)
    {
        return DataReference{this->index(row, column)};
    }
#ifdef __cpp_multidimensional_subscript
    template <typename I = Impl, unless_list<I> = true, if_writable<I> = true>
    auto operator[](int row, int column) { return at(row, column); }
#endif

    // at/operator[int] for tree: return a wrapper that maintains reference to
    // parent.
    template <typename I = Impl, if_tree<I> = true, if_writable<I> = true>
    auto at(int row)
    {
        return RowReference{index(row, 0), this};
    }
    template <typename I = Impl, if_tree<I> = true, if_writable<I> = true>
    auto operator[](int row) { return at(row); }

    // at/operator[path] for tree: a reference or view of the row
    template <typename I = Impl, if_tree<I> = true>
    decltype(auto) at(QSpan<const int> path) const
    {
        return ConstRowReference{index(path, 0), this}.get();
    }
    template <typename I = Impl, if_tree<I> = true>
    decltype(auto) operator[](QSpan<const int> path) const { return at(path); }

    template <typename I = Impl, if_tree<I> = true, if_writable<I> = true>
    auto at(QSpan<const int> path)
    {
        return RowReference{index(path, 0), this};
    }
    template <typename I = Impl, if_tree<I> = true, if_writable<I> = true>
    auto operator[](QSpan<const int> path) { return at(path); }

    // at/operator[path, column] for tree: return value
    template <typename I = Impl, if_tree<I> = true>
    const_data_type at(QSpan<const int> path, int column) const
    {
        Q_PRE(path.size());
        return QRangeModelDetails::dataAtIndex<data_type>(index(path, column));
    }

#ifdef __cpp_multidimensional_subscript
    template <typename I = Impl, if_tree<I> = true>
    const_data_type operator[](QSpan<const int> path, int column) const { return at(path, column); }
#endif

    template <typename I = Impl, if_tree<I> = true, if_writable<I> = true>
    auto at(QSpan<const int> path, int column)
    {
        Q_PRE(path.size());
        return DataReference{this->index(path, column)};
    }
#ifdef __cpp_multidimensional_subscript
    template <typename I = Impl, if_tree<I> = true, if_writable<I> = true>
    auto operator[](QSpan<const int> path, int column) { return at(path, column); }
#endif

    template <typename I = Impl, if_canInsertRows<I> = true>
    bool insertRow(int before)
    {
        return storage.m_model->insertRow(before);
    }

    template <typename I = Impl, if_canInsertRows<I> = true, if_tree<I> = true>
    bool insertRow(QSpan<const int> before)
    {
        Q_PRE(before.size());
        return storage.m_model->insertRow(before.back(), this->index(before.first(before.size() - 1), 0));
    }

    template <typename D = row_type, typename I = Impl,
              if_canInsertRows<I> = true, if_compatible_row<D> = true>
    bool insertRow(int before, D &&data)
    {
        return insertRowImpl(before, storage.root(), std::forward<D>(data));
    }

    template <typename D = row_type, typename I = Impl,
              if_canInsertRows<I> = true, if_compatible_row<D> = true, if_tree<I> = true>
    bool insertRow(QSpan<const int> before, D &&data)
    {
        return insertRowImpl(before, storage.root(), std::forward<D>(data));
    }

    template <typename C, typename I = Impl,
              if_canInsertRows<I> = true, if_compatible_row_range<C> = true>
    bool insertRows(int before, C &&data)
    {
        return insertRowsImpl(before, storage.root(), std::forward<C>(data));
    }

    template <typename C, typename I = Impl,
              if_canInsertRows<I> = true, if_compatible_row_range<C> = true, if_tree<I> = true>
    bool insertRows(QSpan<const int> before, C &&data)
    {
        return insertRowsImpl(before.back(), this->index(before.first(before.size() - 1), 0),
                              std::forward<C>(data));
    }

    template <typename I = Impl, if_canRemoveRows<I> = true>
    bool removeRow(int row)
    {
        return removeRows(row, 1);
    }

    template <typename I = Impl, if_canRemoveRows<I> = true, if_tree<I> = true>
    bool removeRow(QSpan<const int> path)
    {
        return removeRows(path, 1);
    }

    template <typename I = Impl, if_canRemoveRows<I> = true>
    bool removeRows(int row, int count)
    {
        return storage->removeRows(row, count, storage.root());
    }

    template <typename I = Impl, if_canRemoveRows<I> = true, if_tree<I> = true>
    bool removeRows(QSpan<const int> path, int count)
    {
        return storage->removeRows(path.back(), count,
                                   this->index(path.first(path.size() - 1), 0));
    }

    template <typename F = range_features, if_canMoveItems<F> = true>
    bool moveRow(int source, int destination)
    {
        return moveRows(source, 1, destination);
    }

    template <typename F = range_features, if_canMoveItems<F> = true>
    bool moveRows(int source, int count, int destination)
    {
        return storage->moveRows(storage.root(), source, count, storage.root(), destination);
    }

    template <typename I = Impl, typename F = range_features,
              if_canMoveItems<F> = true, if_tree<I> = true>
    bool moveRow(QSpan<const int> source, QSpan<const int> destination)
    {
        return moveRows(source, 1, destination);
    }

    template <typename I = Impl, typename F = range_features,
              if_canMoveItems<F> = true, if_tree<I> = true>
    bool moveRows(QSpan<const int> source, int count, QSpan<const int> destination)
    {
        return storage->moveRows(this->index(source.first(source.size() - 1), 0),
                                 source.back(),
                                 count,
                                 this->index(destination.first(destination.size() - 1), 0),
                                 destination.back());
    }

    template <typename I = Impl, if_canInsertColumns<I> = true>
    bool insertColumn(int before)
    {
        return storage.m_model->insertColumn(before);
    }

    template <typename D, typename I = Impl,
              if_canInsertColumns<I> = true, if_compatible_column_data<D> = true>
    bool insertColumn(int before, D &&data)
    {
        return insertColumnImpl(before, storage.root(), std::forward<D>(data));
    }

    template <typename C, typename I = Impl,
              if_canInsertColumns<I> = true, if_compatible_column_range<C> = true>
    bool insertColumns(int before, C &&data)
    {
        return insertColumnsImpl(before, storage.root(), std::forward<C>(data));
    }

    template <typename I = Impl, if_canRemoveColumns<I> = true>
    bool removeColumn(int column)
    {
        return storage.m_model->removeColumn(column);
    }

    template <typename I = Impl, if_canRemoveColumns<I> = true>
    bool removeColumns(int column, int count)
    {
        return storage->removeColumns(column, count, {});
    }

    template <typename F = row_features, if_canMoveItems<F> = true>
    bool moveColumn(int from, int to)
    {
        return moveColumns(from, 1, to);
    }

    template <typename F = row_features, if_canMoveItems<F> = true>
    bool moveColumns(int from, int count, int to)
    {
        return storage->moveColumns(storage.root(), from, count, storage.root(), to);
    }

    template <typename I = Impl, typename F = row_features,
              if_canMoveItems<F> = true, if_tree<I> = true>
    bool moveColumn(QSpan<const int> source, int to)
    {
        const QModelIndex parent = this->index(source.first(source.size() - 1), 0);
        return storage->moveColumns(parent, source.back(), 1, parent, to);
    }

    template <typename I = Impl, typename F = row_features,
              if_canMoveItems<F> = true, if_tree<I> = true>
    bool moveColumns(QSpan<const int> source, int count, int destination)
    {
        const QModelIndex parent = this->index(source.first(source.size() - 1), 0);
        return storage->moveColumns(parent, source.back(), count, parent, destination);
    }

private:
    friend inline
    bool comparesEqual(const QRangeModelAdapter &lhs, const QRangeModelAdapter &rhs) noexcept
    {
        return lhs.storage.m_model == rhs.storage.m_model;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(QRangeModelAdapter)

    friend inline
    bool comparesEqual(const QRangeModelAdapter &lhs, const range_type &rhs)
    {
        return lhs.range() == rhs;
    }
    Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(QRangeModelAdapter, range_type)


    void emitDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
    {
        Q_EMIT storage.implementation()->dataChanged(topLeft, bottomRight, {});
    }

    void beginSetRangeImpl(Impl *impl, range_type *oldRange, qsizetype newLastRow)
    {
        const QModelIndex root = storage.root();
        const qsizetype oldLastRow = qsizetype(Impl::size(oldRange)) - 1;

        if (!root.isValid()) {
            impl->beginResetModel();
            impl->deleteOwnedRows();
        } else if constexpr (is_tree<Impl>) {
            if (oldLastRow > 0) {
                impl->beginRemoveRows(root, 0, model()->rowCount(root) - 1);
                impl->deleteRemovedRows(QRangeModelDetails::refTo(oldRange));
                impl->endRemoveRows();
            }
            if (newLastRow > 0)
                impl->beginInsertRows(root, 0, newLastRow);
        } else {
            Q_ASSERT_X(false, "QRangeModelAdapter::setRange",
                       "Internal error: The root index in a table or list must be invalid.");
        }
    }

    void endSetRangeImpl(Impl *impl, qsizetype newLastRow)
    {
        const QModelIndex root = storage.root();
        if (!root.isValid()) {
            impl->endResetModel();
        } else if constexpr (is_tree<Impl>) {
            if (newLastRow > 0) {
                Q_ASSERT(model()->hasChildren(root));
                // if it was moved, then newRange is now likely to be empty. Get
                // the inserted row.
                impl->setParentRow(QRangeModelDetails::refTo(impl->childRange(root)),
                                   QRangeModelDetails::pointerTo(impl->rowData(root)));
                impl->endInsertRows();
            }
        }
    }

    template <typename Assigner>
    void setRangeImpl(qsizetype newLastRow, Assigner &&assigner)
    {
        auto *impl = storage.implementation();
        auto *oldRange = impl->childRange(storage.root());
        beginSetRangeImpl(impl, oldRange, newLastRow);
        assigner(QRangeModelDetails::refTo(oldRange));
        endSetRangeImpl(impl, newLastRow);

        if constexpr (Impl::itemsAreQObjects) {
            if (model()->autoConnectPolicy() == QRangeModel::AutoConnectPolicy::Full) {
                const auto begin = QRangeModelDetails::begin(QRangeModelDetails::refTo(oldRange));
                const auto end = QRangeModelDetails::end(QRangeModelDetails::refTo(oldRange));
                int rowIndex = 0;
                for (auto it = begin; it != end; ++it, ++rowIndex)
                    impl->autoConnectPropertiesInRow(*it, rowIndex, storage.root());
            }
        }
    }

    template <typename P>
    static auto setParentRow(P protocol, row_type &newRow, row_ptr parentRow)
        -> decltype(protocol.setParentRow(std::declval<row_type&>(), std::declval<row_ptr>()))
    {
        return protocol.setParentRow(newRow, parentRow);
    }

    template <typename ...Args> static constexpr void setParentRow(Args &&...) {}

    template <typename D>
    bool insertRowImpl(int before, const QModelIndex &parent, D &&data)
    {
        return storage.implementation()->doInsertRows(before, 1, parent, [&data, this]
                                        (range_type &range, auto parentRow, int row, int count) {
            Q_UNUSED(this);
            const auto oldSize = range.size();
            auto newRow = range.emplace(QRangeModelDetails::pos(range, row), std::forward<D>(data));
            setParentRow(storage.implementation()->protocol(), *newRow, parentRow);
            return range.size() == oldSize + count;
        });
    }

    template <typename LHS>
    static auto selfInsertion(LHS *lhs, LHS *rhs) -> decltype(lhs == rhs)
    {
        if (lhs == rhs) {
#ifndef QT_NO_DEBUG
            qCritical("Inserting data into itself is not supported");
#endif
            return true;
        }
        return false;
    }
    template <typename LHS, typename RHS>
    static constexpr bool selfInsertion(LHS *, RHS *) { return false; }

    template <typename C>
    bool insertRowsImpl(int before, const QModelIndex &parent, C &&data)
    {
        bool result = false;
        result = storage->doInsertRows(before, int(std::size(data)), parent, [&data, this]
                                        (range_type &range, auto parentRow, int row, int count){
            Q_UNUSED(parentRow);
            Q_UNUSED(this);
            const auto pos = QRangeModelDetails::pos(range, row);
            const auto oldSize = range.size();

            auto dataRange = [&data]{
                if constexpr (std::is_rvalue_reference_v<C&&>) {
                    return std::make_pair(
                        std::move_iterator(std::begin(data)),
                        std::move_iterator(std::end(data))
                    );
                } else {
                    return std::make_pair(std::begin(data), std::end(data));
                }
            }();

            if constexpr (range_features::has_insert_range) {
                if (selfInsertion(&range, &data))
                    return false;
                auto start = range.insert(pos, dataRange.first, dataRange.second);
                if constexpr (protocol_traits::has_setParentRow) {
                    while (count) {
                        setParentRow(storage->protocol(), *start, parentRow);
                        ++start;
                        --count;
                    }
                } else {
                    Q_UNUSED(start);
                }
            } else {
                auto newRow = range.insert(pos, count, row_type{});
                while (dataRange.first != dataRange.second) {
                    *newRow = *dataRange.first;
                    setParentRow(storage->protocol(), *newRow, parentRow);
                    ++dataRange.first;
                    ++newRow;
                }
            }
            return range.size() == oldSize + count;
        });
        return result;
    }

    template <typename D, typename = void>
    struct DataFromList {
        static constexpr auto first(D &data) { return &data; }
        static constexpr auto next(D &, D *entry) { return entry; }
    };

    template <typename D>
    struct DataFromList<D, std::enable_if_t<QRangeModelDetails::range_traits<D>::value>>
    {
        static constexpr auto first(D &data) { return std::begin(data); }
        static constexpr auto next(D &data, typename D::iterator entry)
        {
            ++entry;
            if (entry == std::end(data))
                entry = first(data);
            return entry;
        }
    };

    template <typename D, typename = void> struct RowFromTable
    {
        static constexpr auto first(D &data) { return &data; }
        static constexpr auto next(D &, D *entry) { return entry; }
    };

    template <typename D>
    struct RowFromTable<D, std::enable_if_t<std::conjunction_v<
                                            QRangeModelDetails::range_traits<D>,
                                            QRangeModelDetails::range_traits<typename D::value_type>
                                           >>
                        > : DataFromList<D>
    {};

    template <typename D>
    bool insertColumnImpl(int before, const QModelIndex &parent, D data)
    {
        auto entry = DataFromList<D>::first(data);

        return storage->doInsertColumns(before, 1, parent, [&entry, &data]
                                        (auto &range, auto pos, int count) {
            const auto oldSize = range.size();
            range.insert(pos, *entry);
            entry = DataFromList<D>::next(data, entry);
            return range.size() == oldSize + count;
        });
    }

    template <typename C>
    bool insertColumnsImpl(int before, const QModelIndex &parent, C data)
    {
        bool result = false;
        auto entries = RowFromTable<C>::first(data);
        auto begin = std::begin(*entries);
        auto end = std::end(*entries);
        result = storage->doInsertColumns(before, int(std::size(*entries)), parent,
                            [&begin, &end, &entries, &data](auto &range, auto pos, int count) {
            const auto oldSize = range.size();
            if constexpr (row_features::has_insert_range) {
                range.insert(pos, begin, end);
            } else {
                auto start = range.insert(pos, count, {});
                std::copy(begin, end, start);
            }
            entries = RowFromTable<C>::next(data, entries);
            begin = std::begin(*entries);
            end = std::end(*entries);
            return range.size() == oldSize + count;
        });
        return result;
    }
};

template <typename Range, typename Protocol,
          QRangeModelDetails::if_can_construct<Range, Protocol> = true>
QRangeModelAdapter(Range &&, Protocol &&) -> QRangeModelAdapter<Range, Protocol>;

template <typename Range,
          QRangeModelDetails::if_can_construct<Range> = true>
QRangeModelAdapter(Range &&) -> QRangeModelAdapter<Range, void>;

QT_END_NAMESPACE

#endif // QRANGEMODELADAPTER_H
