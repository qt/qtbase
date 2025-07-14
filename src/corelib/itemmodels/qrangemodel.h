// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QRANGEMODEL_H
#define QRANGEMODEL_H

#include <QtCore/qrangemodel_impl.h>

QT_BEGIN_NAMESPACE

class QRangeModelPrivate;

class Q_CORE_EXPORT QRangeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    template <typename T>
    using SingleColumn = std::tuple<T>;

    template <typename T>
    struct MultiColumn
    {
        using type = std::remove_pointer_t<T>;
        T data{};
        template <typename X>
        using if_get_matches = std::enable_if_t<std::is_same_v<q20::remove_cvref_t<X>,
                                                               MultiColumn<T>>, bool>;
        template <typename X>
        using if_validatable = std::enable_if_t<QRangeModelDetails::is_validatable<X>::value, bool>;

        template <typename V = T, if_validatable<V> = true>
        constexpr explicit operator bool() const noexcept { return bool(data); }

        // unconstrained on size_t I, gcc internal error #3280
        template <std::size_t I, typename V, if_get_matches<V> = true>
        friend inline decltype(auto) get(V &&multiColumn)
        {
            static_assert(I < std::tuple_size_v<type>, "Index out of bounds for wrapped type");
            return get<I>(QRangeModelDetails::refTo(q23::forward_like<V>(multiColumn.data)));
        }
    };

    template <typename Range,
              QRangeModelDetails::if_table_range<Range> = true>
    explicit QRangeModel(Range &&range, QObject *parent = nullptr);

    template <typename Range,
              QRangeModelDetails::if_tree_range<Range> = true>
    explicit QRangeModel(Range &&range, QObject *parent = nullptr);

    template <typename Range, typename Protocol,
              QRangeModelDetails::if_tree_range<Range, Protocol> = true>
    explicit QRangeModel(Range &&range, Protocol &&protocol, QObject *parent = nullptr);

    ~QRangeModel() override;

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QModelIndex sibling(int row, int column, const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &data, int role = Qt::EditRole) override;
    QMap<int, QVariant> itemData(const QModelIndex &index) const override;
    bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &data) override;
    bool clearItemData(const QModelIndex &index) override;
    bool insertColumns(int column, int count, const QModelIndex &parent = {}) override;
    bool removeColumns(int column, int count, const QModelIndex &parent = {}) override;
    bool moveColumns(const QModelIndex &sourceParent, int sourceColumn, int count,
                     const QModelIndex &destParent, int destColumn) override;
    bool insertRows(int row, int count, const QModelIndex &parent = {}) override;
    bool removeRows(int row, int count, const QModelIndex &parent = {}) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                  const QModelIndex &destParent, int destRow) override;

    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex buddy(const QModelIndex &index) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                         const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                      const QModelIndex &parent) override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    QStringList mimeTypes() const override;
    QModelIndexList match(const QModelIndex &start, int role, const QVariant &value, int hits,
                          Qt::MatchFlags flags) const override;
    void multiData(const QModelIndex &index, QModelRoleDataSpan roleDataSpan) const override;
    QHash<int, QByteArray> roleNames() const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    QSize span(const QModelIndex &index) const override;
    Qt::DropActions supportedDragActions() const override;
    Qt::DropActions supportedDropActions() const override;

protected Q_SLOTS:
    void resetInternalData() override;

protected:
    bool event(QEvent *) override;
    bool eventFilter(QObject *, QEvent *) override;

private:
    Q_DISABLE_COPY_MOVE(QRangeModel)
    Q_DECLARE_PRIVATE(QRangeModel)

    explicit QRangeModel(QRangeModelImplBase *impl, QObject *parent);
    friend class QRangeModelImplBase;
};

// implementation of forwarders
QModelIndex QRangeModelImplBase::createIndex(int row, int column, const void *ptr) const
{
    return m_rangeModel->createIndex(row, column, ptr);
}
void QRangeModelImplBase::changePersistentIndexList(const QModelIndexList &from,
                                                          const QModelIndexList &to)
{
    m_rangeModel->changePersistentIndexList(from, to);
}
QHash<int, QByteArray> QRangeModelImplBase::roleNames() const
{
    return m_rangeModel->roleNames();
}
void QRangeModelImplBase::dataChanged(const QModelIndex &from, const QModelIndex &to,
                                            const QList<int> &roles)
{
    m_rangeModel->dataChanged(from, to, roles);
}
void QRangeModelImplBase::beginInsertColumns(const QModelIndex &parent, int start, int count)
{
    m_rangeModel->beginInsertColumns(parent, start, count);
}
void QRangeModelImplBase::endInsertColumns()
{
    m_rangeModel->endInsertColumns();
}
void QRangeModelImplBase::beginRemoveColumns(const QModelIndex &parent, int start, int count)
{
    m_rangeModel->beginRemoveColumns(parent, start, count);
}
void QRangeModelImplBase::endRemoveColumns()
{
    m_rangeModel->endRemoveColumns();
}
bool QRangeModelImplBase::beginMoveColumns(const QModelIndex &sourceParent, int sourceFirst,
                                                 int sourceLast, const QModelIndex &destParent,
                                                 int destColumn)
{
    return m_rangeModel->beginMoveColumns(sourceParent, sourceFirst, sourceLast,
                                         destParent, destColumn);
}
void QRangeModelImplBase::endMoveColumns()
{
    m_rangeModel->endMoveColumns();
}

void QRangeModelImplBase::beginInsertRows(const QModelIndex &parent, int start, int count)
{
    m_rangeModel->beginInsertRows(parent, start, count);
}
void QRangeModelImplBase::endInsertRows()
{
    m_rangeModel->endInsertRows();
}
void QRangeModelImplBase::beginRemoveRows(const QModelIndex &parent, int start, int count)
{
    m_rangeModel->beginRemoveRows(parent, start, count);
}
void QRangeModelImplBase::endRemoveRows()
{
    m_rangeModel->endRemoveRows();
}
bool QRangeModelImplBase::beginMoveRows(const QModelIndex &sourceParent, int sourceFirst,
                                              int sourceLast,
                                              const QModelIndex &destParent, int destRow)
{
    return m_rangeModel->beginMoveRows(sourceParent, sourceFirst, sourceLast, destParent, destRow);
}
void QRangeModelImplBase::endMoveRows()
{
    m_rangeModel->endMoveRows();
}
QAbstractItemModel &QRangeModelImplBase::itemModel()
{
    return *m_rangeModel;
}
const QAbstractItemModel &QRangeModelImplBase::itemModel() const
{
    return *m_rangeModel;
}

template <typename Range, QRangeModelDetails::if_table_range<Range>>
QRangeModel::QRangeModel(Range &&range, QObject *parent)
    : QRangeModel(new QGenericTableItemModelImpl<Range>(std::forward<Range>(range), this), parent)
{}

template <typename Range, QRangeModelDetails::if_tree_range<Range>>
QRangeModel::QRangeModel(Range &&range, QObject *parent)
    : QRangeModel(std::forward<Range>(range), QRangeModelDetails::DefaultTreeProtocol<Range>{},
                  parent)
{}

template <typename Range, typename Protocol, QRangeModelDetails::if_tree_range<Range, Protocol>>
QRangeModel::QRangeModel(Range &&range, Protocol &&protocol, QObject *parent)
    : QRangeModel(new QGenericTreeItemModelImpl<Range, Protocol>(std::forward<Range>(range),
                                                                 std::forward<Protocol>(protocol),
                                                                 this),
                  parent)
{}

QT_END_NAMESPACE

namespace std {
    template <typename T>
    struct tuple_size<QT_PREPEND_NAMESPACE(QRangeModel)::MultiColumn<T>>
        : tuple_size<typename QT_PREPEND_NAMESPACE(QRangeModel)::MultiColumn<T>::type>
    {};
    template <std::size_t I, typename T>
    struct tuple_element<I, QT_PREPEND_NAMESPACE(QRangeModel)::MultiColumn<T>>
        : tuple_element<I, typename QT_PREPEND_NAMESPACE(QRangeModel)::MultiColumn<T>::type>
    {};
}

#endif // QRANGEMODEL_H
