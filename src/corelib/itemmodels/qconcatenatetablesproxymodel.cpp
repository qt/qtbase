// Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qconcatenatetablesproxymodel.h"
#include <private/qabstractitemmodel_p.h>
#include "qsize.h"
#include "qmap.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

class QConcatenateTablesProxyModelPrivate : public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QConcatenateTablesProxyModel);

public:
    QConcatenateTablesProxyModelPrivate();

    int computeRowsPrior(const QAbstractItemModel *sourceModel) const;

    struct SourceModelForRowResult
    {
        SourceModelForRowResult() : sourceModel(nullptr), sourceRow(-1) {}
        QAbstractItemModel *sourceModel;
        int sourceRow;
    };
    SourceModelForRowResult sourceModelForRow(int row) const;

    void slotRowsAboutToBeInserted(const QModelIndex &, int start, int end);
    void slotRowsInserted(const QModelIndex &, int start, int end);
    void slotRowsAboutToBeRemoved(const QModelIndex &, int start, int end);
    void slotRowsRemoved(const QModelIndex &, int start, int end);
    void slotRowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                                const QModelIndex &destinationParent, int destinationRow);
    void slotRowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                       const QModelIndex &destinationParent, int destinationRow);
    void slotColumnsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void slotColumnsInserted(const QModelIndex &parent, int, int);
    void slotColumnsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void slotColumnsRemoved(const QModelIndex &parent, int, int);
    void slotColumnsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                                   const QModelIndex &destinationParent, int destination);
    void slotColumnsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                          const QModelIndex &destinationParent, int destination);
    void slotDataChanged(const QModelIndex &from, const QModelIndex &to, const QList<int> &roles);
    void slotSourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint);
    void slotSourceLayoutChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint);
    void slotModelAboutToBeReset();
    void slotModelReset();
    int columnCountAfterChange(const QAbstractItemModel *model, int newCount) const;
    int calculatedColumnCount() const;
    void updateColumnCount();
    bool mapDropCoordinatesToSource(int row, int column, const QModelIndex &parent,
                                    int *sourceRow, int *sourceColumn, QModelIndex *sourceParent, QAbstractItemModel **sourceModel) const;

    struct ModelInfo {
        using ConnArray = std::array<QMetaObject::Connection, 17>;
        ModelInfo(QAbstractItemModel *m, ConnArray &&con)
            : model(m), connections(std::move(con)) {}
        QAbstractItemModel *model = nullptr;
        ConnArray connections;
    };
    QList<ModelInfo> m_models;
    mutable QHash<int, QByteArray> m_roleNames;

    QList<ModelInfo>::const_iterator findSourceModel(const QAbstractItemModel *m) const
    {
        auto byModelPtr = [m](const auto &modInfo) { return modInfo.model == m; };
        return std::find_if(m_models.cbegin(), m_models.cend(), byModelPtr);
    }

    bool containsSourceModel(const QAbstractItemModel *m) const
    { return findSourceModel(m) != m_models.cend(); }

    int m_rowCount; // have to maintain it here since we can't compute during model destruction
    int m_columnCount;

    // for columns{AboutToBe,}{Inserted,Removed}
    int m_newColumnCount;

    mutable uint m_roleNamesDirty : 1;
    uint m_reserved : 31;

    // for layoutAboutToBeChanged/layoutChanged
    QList<QPersistentModelIndex> layoutChangePersistentIndexes;
    QList<QModelIndex> layoutChangeProxyIndexes;
};

QConcatenateTablesProxyModelPrivate::QConcatenateTablesProxyModelPrivate()
    : m_rowCount(0),
      m_columnCount(0),
      m_newColumnCount(0),
      m_roleNamesDirty(true),
      m_reserved(0)
{
}

/*!
    \since 5.13
    \class QConcatenateTablesProxyModel
    \inmodule QtCore
    \brief The QConcatenateTablesProxyModel class proxies multiple source models, concatenating their rows.

    \ingroup model-view

    QConcatenateTablesProxyModel takes multiple source models and concatenates their rows.

    In other words, the proxy will have all rows of the first source model,
    followed by all rows of the second source model, and so on.

    If the source models don't have the same number of columns, the proxy will only
    have as many columns as the source model with the smallest number of columns.
    Additional columns in other source models will simply be ignored.

    Source models can be added and removed at runtime, and the column count is adjusted accordingly.

    This proxy does not inherit from QAbstractProxyModel because it uses multiple source
    models, rather than a single one.

    Only flat models (lists and tables) are supported, tree models are not.

    \sa QAbstractProxyModel, {Model/View Programming}, QIdentityProxyModel, QAbstractItemModel
 */


/*!
    Constructs a concatenate-rows proxy model with the given \a parent.
*/
QConcatenateTablesProxyModel::QConcatenateTablesProxyModel(QObject *parent)
    : QAbstractItemModel(*new QConcatenateTablesProxyModelPrivate, parent)
{
}

/*!
    Destroys this proxy model.
*/
QConcatenateTablesProxyModel::~QConcatenateTablesProxyModel()
{
}

/*!
    Returns the proxy index for a given \a sourceIndex, which can be from any of the source models.
*/
QModelIndex QConcatenateTablesProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    Q_D(const QConcatenateTablesProxyModel);
    if (!sourceIndex.isValid())
        return QModelIndex();
    const QAbstractItemModel *sourceModel = sourceIndex.model();
    if (!d->containsSourceModel(sourceModel)) {
        qWarning("QConcatenateTablesProxyModel: index from wrong model passed to mapFromSource");
        Q_ASSERT(!"QConcatenateTablesProxyModel: index from wrong model passed to mapFromSource");
        return QModelIndex();
    }
    if (sourceIndex.column() >= d->m_columnCount)
        return QModelIndex();
    int rowsPrior = d_func()->computeRowsPrior(sourceModel);
    return createIndex(rowsPrior + sourceIndex.row(), sourceIndex.column(), sourceIndex.internalPointer());
}

/*!
    Returns the source index for a given \a proxyIndex.
*/
QModelIndex QConcatenateTablesProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    Q_D(const QConcatenateTablesProxyModel);
    Q_ASSERT(checkIndex(proxyIndex));
    if (!proxyIndex.isValid())
        return QModelIndex();
    if (proxyIndex.model() != this) {
        qWarning("QConcatenateTablesProxyModel: index from wrong model passed to mapToSource");
        Q_ASSERT(!"QConcatenateTablesProxyModel: index from wrong model passed to mapToSource");
        return QModelIndex();
    }
    const int row = proxyIndex.row();
    const auto result = d->sourceModelForRow(row);
    if (!result.sourceModel)
        return QModelIndex();
    return result.sourceModel->index(result.sourceRow, proxyIndex.column());
}

/*!
  \reimp
*/
QVariant QConcatenateTablesProxyModel::data(const QModelIndex &index, int role) const
{
    const QModelIndex sourceIndex = mapToSource(index);
    Q_ASSERT(checkIndex(index, CheckIndexOption::IndexIsValid));
    if (!sourceIndex.isValid())
        return QVariant();
    return sourceIndex.data(role);
}

/*!
  \reimp
*/
bool QConcatenateTablesProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_ASSERT(checkIndex(index, CheckIndexOption::IndexIsValid));
    const QModelIndex sourceIndex = mapToSource(index);
    Q_ASSERT(sourceIndex.isValid());
    const auto sourceModel = const_cast<QAbstractItemModel *>(sourceIndex.model());
    return sourceModel->setData(sourceIndex, value, role);
}

/*!
  \reimp
*/
QMap<int, QVariant> QConcatenateTablesProxyModel::itemData(const QModelIndex &proxyIndex) const
{
    Q_ASSERT(checkIndex(proxyIndex));
    const QModelIndex sourceIndex = mapToSource(proxyIndex);
    Q_ASSERT(sourceIndex.isValid());
    return sourceIndex.model()->itemData(sourceIndex);
}

/*!
  \reimp
*/
bool QConcatenateTablesProxyModel::setItemData(const QModelIndex &proxyIndex, const QMap<int, QVariant> &roles)
{
    Q_ASSERT(checkIndex(proxyIndex));
    const QModelIndex sourceIndex = mapToSource(proxyIndex);
    Q_ASSERT(sourceIndex.isValid());
    const auto sourceModel = const_cast<QAbstractItemModel *>(sourceIndex.model());
    return sourceModel->setItemData(sourceIndex, roles);
}

/*!
  Returns the flags for the given index.
  If the \a index is valid, the flags come from the source model for this \a index.
  If the \a index is invalid (as used to determine if dropping onto an empty area
  in the view is allowed, for instance), the flags from the first model are returned.
*/
Qt::ItemFlags QConcatenateTablesProxyModel::flags(const QModelIndex &index) const
{
    Q_D(const QConcatenateTablesProxyModel);
    if (d->m_models.isEmpty())
        return Qt::NoItemFlags;
    Q_ASSERT(checkIndex(index));
    if (!index.isValid())
        return d->m_models.at(0).model->flags(index);
    const QModelIndex sourceIndex = mapToSource(index);
    Q_ASSERT(sourceIndex.isValid());
    return sourceIndex.model()->flags(sourceIndex);
}

/*!
    This method returns the horizontal header data for the first source model,
    and the vertical header data for the source model corresponding to each row.
    \reimp
*/
QVariant QConcatenateTablesProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_D(const QConcatenateTablesProxyModel);
    if (d->m_models.isEmpty())
        return QVariant();
    switch (orientation) {
        case Qt::Horizontal:
            return d->m_models.at(0).model->headerData(section, orientation, role);
        case Qt::Vertical: {
            const auto result = d->sourceModelForRow(section);
            Q_ASSERT(result.sourceModel);
            return result.sourceModel->headerData(result.sourceRow, orientation, role);
        }
    }
    return QVariant();
}

/*!
    This method returns the column count of the source model with the smallest number of columns.
    \reimp
*/
int QConcatenateTablesProxyModel::columnCount(const QModelIndex &parent) const
{
    Q_D(const QConcatenateTablesProxyModel);
    if (parent.isValid())
        return 0; // flat model
    return d->m_columnCount;
}

/*!
  \reimp
*/
QModelIndex QConcatenateTablesProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const QConcatenateTablesProxyModel);
    Q_ASSERT(hasIndex(row, column, parent));
    if (!hasIndex(row, column, parent))
        return QModelIndex();
    Q_ASSERT(checkIndex(parent, QAbstractItemModel::CheckIndexOption::ParentIsInvalid)); // flat model
    const auto result = d->sourceModelForRow(row);
    Q_ASSERT(result.sourceModel);
    return mapFromSource(result.sourceModel->index(result.sourceRow, column));
}

/*!
  \reimp
*/
QModelIndex QConcatenateTablesProxyModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex(); // flat model, no hierarchy
}

/*!
  \reimp
*/
int QConcatenateTablesProxyModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QConcatenateTablesProxyModel);
    if (parent.isValid())
        return 0; // flat model
    return d->m_rowCount;
}

/*!
    This method returns the mime types for the first source model.
    \reimp
*/
QStringList QConcatenateTablesProxyModel::mimeTypes() const
{
    Q_D(const QConcatenateTablesProxyModel);
    if (d->m_models.isEmpty())
        return QStringList();
    return d->m_models.at(0).model->mimeTypes();
}

/*!
  The call is forwarded to the source model of the first index in the list of \a indexes.

  Important: please note that this proxy only supports dragging a single row.
  It will assert if called with indexes from multiple rows, because dragging rows that
  might come from different source models cannot be implemented generically by this proxy model.
  Each piece of data in the QMimeData needs to be merged, which is data-type-specific.
  Reimplement this method in a subclass if you want to support dragging multiple rows.

  \reimp
*/
QMimeData *QConcatenateTablesProxyModel::mimeData(const QModelIndexList &indexes) const
{
    Q_D(const QConcatenateTablesProxyModel);
    if (indexes.isEmpty())
        return nullptr;
    const QModelIndex firstIndex = indexes.first();
    Q_ASSERT(checkIndex(firstIndex, CheckIndexOption::IndexIsValid));
    const auto result = d->sourceModelForRow(firstIndex.row());
    QModelIndexList sourceIndexes;
    sourceIndexes.reserve(indexes.size());
    for (const QModelIndex &index : indexes) {
        const QModelIndex sourceIndex = mapToSource(index);
        Q_ASSERT(sourceIndex.model() == result.sourceModel); // see documentation above
        sourceIndexes.append(sourceIndex);
    }
    return result.sourceModel->mimeData(sourceIndexes);
}


bool QConcatenateTablesProxyModelPrivate::mapDropCoordinatesToSource(int row, int column, const QModelIndex &parent,
                                                                     int *sourceRow, int *sourceColumn, QModelIndex *sourceParent, QAbstractItemModel **sourceModel) const
{
    Q_Q(const QConcatenateTablesProxyModel);
    *sourceColumn = column;
    if (!parent.isValid()) {
        // Drop after the last item
        if (row == -1 || row == m_rowCount) {
            *sourceRow = -1;
            *sourceModel = m_models.constLast().model;
            return true;
        }
        // Drop between toplevel items
        const auto result = sourceModelForRow(row);
        Q_ASSERT(result.sourceModel);
        *sourceRow = result.sourceRow;
        *sourceModel = result.sourceModel;
        return true;
    } else {
        if (row > -1)
            return false; // flat model, no dropping as new children of items
        // Drop onto item
        const int targetRow = parent.row();
        const auto result = sourceModelForRow(targetRow);
        Q_ASSERT(result.sourceModel);
        const QModelIndex sourceIndex = q->mapToSource(parent);
        *sourceRow = -1;
        *sourceParent = sourceIndex;
        *sourceModel = result.sourceModel;
        return true;
    }
}

/*!
  \reimp
*/
bool QConcatenateTablesProxyModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_D(const QConcatenateTablesProxyModel);
    if (d->m_models.isEmpty())
        return false;

    int sourceRow, sourceColumn;
    QModelIndex sourceParent;
    QAbstractItemModel *sourceModel;
    if (!d->mapDropCoordinatesToSource(row, column, parent, &sourceRow, &sourceColumn, &sourceParent, &sourceModel))
        return false;
    return sourceModel->canDropMimeData(data, action, sourceRow, sourceColumn, sourceParent);
}

/*!
  QConcatenateTablesProxyModel handles dropping onto an item, between items, and after the last item.
  In all cases the call is forwarded to the underlying source model.
  When dropping onto an item, the source model for this item is called.
  When dropping between items, the source model immediately below the drop position is called.
  When dropping after the last item, the last source model is called.

  \reimp
*/
bool QConcatenateTablesProxyModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_D(const QConcatenateTablesProxyModel);
    if (d->m_models.isEmpty())
        return false;
    int sourceRow, sourceColumn;
    QModelIndex sourceParent;
    QAbstractItemModel *sourceModel;
    if (!d->mapDropCoordinatesToSource(row, column, parent, &sourceRow, &sourceColumn, &sourceParent, &sourceModel))
        return false;

    return sourceModel->dropMimeData(data, action, sourceRow, sourceColumn, sourceParent);
}

/*!
    \reimp
*/
QSize QConcatenateTablesProxyModel::span(const QModelIndex &index) const
{
    Q_D(const QConcatenateTablesProxyModel);
    Q_ASSERT(checkIndex(index));
    if (d->m_models.isEmpty() || !index.isValid())
        return QSize();
    const QModelIndex sourceIndex = mapToSource(index);
    Q_ASSERT(sourceIndex.isValid());
    return sourceIndex.model()->span(sourceIndex);
}

/*!
    Returns a list of models that were added as source models for this proxy model.

    \since 5.15
*/
QList<QAbstractItemModel *> QConcatenateTablesProxyModel::sourceModels() const
{
    Q_D(const QConcatenateTablesProxyModel);
    QList<QAbstractItemModel *> ret;
    ret.reserve(d->m_models.size());
    for (const auto &info : d->m_models)
        ret.push_back(info.model);
    return ret;
}

/*!
    Adds a source model \a sourceModel, below all previously added source models.

    The ownership of \a sourceModel is not affected by this.

    The same source model cannot be added more than once.
 */
void QConcatenateTablesProxyModel::addSourceModel(QAbstractItemModel *sourceModel)
{
    Q_D(QConcatenateTablesProxyModel);
    Q_ASSERT(sourceModel);
    Q_ASSERT(!d->containsSourceModel(sourceModel));

    const int newRows = sourceModel->rowCount();
    if (newRows > 0)
        beginInsertRows(QModelIndex(), d->m_rowCount, d->m_rowCount + newRows - 1);
    d->m_rowCount += newRows;
    d->m_models.emplace_back(sourceModel, std::array{
        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::dataChanged,
                                d, &QConcatenateTablesProxyModelPrivate::slotDataChanged),
        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::rowsInserted,
                                d, &QConcatenateTablesProxyModelPrivate::slotRowsInserted),
        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::rowsRemoved,
                                d, &QConcatenateTablesProxyModelPrivate::slotRowsRemoved),
        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::rowsAboutToBeInserted,
                                d, &QConcatenateTablesProxyModelPrivate::slotRowsAboutToBeInserted),
        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved,
                                d, &QConcatenateTablesProxyModelPrivate::slotRowsAboutToBeRemoved),
        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::rowsMoved,
                                d, &QConcatenateTablesProxyModelPrivate::slotRowsMoved),
        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::rowsAboutToBeMoved,
                                d, &QConcatenateTablesProxyModelPrivate::slotRowsAboutToBeMoved),

        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::columnsInserted,
                                d, &QConcatenateTablesProxyModelPrivate::slotColumnsInserted),
        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::columnsRemoved,
                                d, &QConcatenateTablesProxyModelPrivate::slotColumnsRemoved),
        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::columnsAboutToBeInserted,
                                d, &QConcatenateTablesProxyModelPrivate::slotColumnsAboutToBeInserted),
        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::columnsAboutToBeRemoved,
                                d, &QConcatenateTablesProxyModelPrivate::slotColumnsAboutToBeRemoved),
        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::columnsMoved,
                                d, &QConcatenateTablesProxyModelPrivate::slotColumnsMoved),
        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::columnsAboutToBeMoved,
                                d, &QConcatenateTablesProxyModelPrivate::slotColumnsAboutToBeMoved),

        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::layoutAboutToBeChanged,
                                d, &QConcatenateTablesProxyModelPrivate::slotSourceLayoutAboutToBeChanged),
        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::layoutChanged,
                                d, &QConcatenateTablesProxyModelPrivate::slotSourceLayoutChanged),
        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::modelAboutToBeReset,
                                d, &QConcatenateTablesProxyModelPrivate::slotModelAboutToBeReset),
        QObjectPrivate::connect(sourceModel, &QAbstractItemModel::modelReset,
                                d, &QConcatenateTablesProxyModelPrivate::slotModelReset),
    });
    if (!d->m_roleNamesDirty) {
        // do update immediately, since append() is a simple update:
        const auto newRoleNames = sourceModel->roleNames();
        for (const auto &[k, v] : newRoleNames.asKeyValueRange())
            d->m_roleNames.insert_or_assign(k, v);
    }
    if (newRows > 0)
        endInsertRows();

    d->updateColumnCount();
}

/*!
    Removes the source model \a sourceModel, which was previously added to this proxy.

    The ownership of \a sourceModel is not affected by this.
*/
void QConcatenateTablesProxyModel::removeSourceModel(QAbstractItemModel *sourceModel)
{
    Q_D(QConcatenateTablesProxyModel);

    auto it = d->findSourceModel(sourceModel);
    Q_ASSERT(it != d->m_models.cend());
    for (auto &c : it->connections)
        disconnect(c);

    const int rowsRemoved = sourceModel->rowCount();
    const int rowsPrior = d->computeRowsPrior(sourceModel);   // location of removed section

    if (rowsRemoved > 0)
        beginRemoveRows(QModelIndex(), rowsPrior, rowsPrior + rowsRemoved - 1);
    d->m_models.erase(it);
    d->m_roleNamesDirty = true;
    d->m_rowCount -= rowsRemoved;
    if (rowsRemoved > 0)
        endRemoveRows();

    d->updateColumnCount();
}

/*!
    \since 6.9.0
    \reimp
    Returns the union of the roleNames() of the underlying models.

    In case source models associate different names to the same role,
    the name used in last source model overrides the name used in earlier models.
*/
QHash<int, QByteArray> QConcatenateTablesProxyModel::roleNames() const
{
    Q_D(const QConcatenateTablesProxyModel);
    if (d->m_roleNamesDirty) {
        d->m_roleNames = QAbstractItemModelPrivate::defaultRoleNames();
        for (const auto &[model, _] : d->m_models)
            d->m_roleNames.insert(model->roleNames());
        d->m_roleNamesDirty = false;
    }
    return d->m_roleNames;
}

void QConcatenateTablesProxyModelPrivate::slotRowsAboutToBeInserted(const QModelIndex &parent,
                                                                    int start, int end)
{
    Q_Q(QConcatenateTablesProxyModel);
    if (parent.isValid()) // not supported, the proxy is a flat model
        return;
    const QAbstractItemModel * const model = static_cast<QAbstractItemModel *>(q->sender());
    const int rowsPrior = computeRowsPrior(model);
    q->beginInsertRows(QModelIndex(), rowsPrior + start, rowsPrior + end);
}

void QConcatenateTablesProxyModelPrivate::slotRowsInserted(const QModelIndex &parent, int start,
                                                           int end)
{
    Q_Q(QConcatenateTablesProxyModel);
    if (parent.isValid()) // flat model
        return;
    m_rowCount += end - start + 1;
    q->endInsertRows();
}

void QConcatenateTablesProxyModelPrivate::slotRowsAboutToBeRemoved(const QModelIndex &parent,
                                                                   int start, int end)
{
    Q_Q(QConcatenateTablesProxyModel);
    if (parent.isValid()) // flat model
        return;
    const QAbstractItemModel * const model = static_cast<QAbstractItemModel *>(q->sender());
    const int rowsPrior = computeRowsPrior(model);
    q->beginRemoveRows(QModelIndex(), rowsPrior + start, rowsPrior + end);
}

void QConcatenateTablesProxyModelPrivate::slotRowsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_Q(QConcatenateTablesProxyModel);
    if (parent.isValid()) // flat model
        return;
    m_rowCount -= end - start + 1;
    q->endRemoveRows();
}

void QConcatenateTablesProxyModelPrivate::slotRowsAboutToBeMoved(
        const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
        const QModelIndex &destinationParent, int destinationRow)
{
    Q_Q(QConcatenateTablesProxyModel);
    if (sourceParent.isValid() || destinationParent.isValid())
        return;
    const QAbstractItemModel *const model = static_cast<QAbstractItemModel *>(q->sender());
    const int rowsPrior = computeRowsPrior(model);
    q->beginMoveRows(sourceParent, rowsPrior + sourceStart, rowsPrior + sourceEnd,
                     destinationParent, rowsPrior + destinationRow);
}

void QConcatenateTablesProxyModelPrivate::slotRowsMoved(const QModelIndex &sourceParent,
                                                        int sourceStart, int sourceEnd,
                                                        const QModelIndex &destinationParent,
                                                        int destinationRow)
{
    Q_Q(QConcatenateTablesProxyModel);
    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destinationRow)
    if (sourceParent.isValid() || destinationParent.isValid())
        return;
    q->endMoveRows();
}

void QConcatenateTablesProxyModelPrivate::slotColumnsAboutToBeInserted(const QModelIndex &parent,
                                                                       int start, int end)
{
    Q_Q(QConcatenateTablesProxyModel);
    if (parent.isValid()) // flat model
        return;
    const QAbstractItemModel * const model = static_cast<QAbstractItemModel *>(q->sender());
    const int oldColCount = model->columnCount();
    const int newColCount = columnCountAfterChange(model, oldColCount + end - start + 1);
    Q_ASSERT(newColCount >= oldColCount);
    if (newColCount > oldColCount)
        // If the underlying models have a different number of columns (example: 2 and 3), inserting 2 columns in
        // the first model leads to inserting only one column in the proxy, since qMin(2+2,3) == 3.
        q->beginInsertColumns(QModelIndex(), start, qMin(end, start + newColCount - oldColCount - 1));
    m_newColumnCount = newColCount;
}

void QConcatenateTablesProxyModelPrivate::slotColumnsInserted(const QModelIndex &parent, int start,
                                                              int end)
{
    Q_UNUSED(start);
    Q_UNUSED(end);
    Q_Q(QConcatenateTablesProxyModel);
    if (parent.isValid()) // flat model
        return;
    if (m_newColumnCount != m_columnCount) {
        m_columnCount = m_newColumnCount;
        q->endInsertColumns();
    }
}

void QConcatenateTablesProxyModelPrivate::slotColumnsAboutToBeRemoved(const QModelIndex &parent,
                                                                      int start, int end)
{
    Q_Q(QConcatenateTablesProxyModel);
    if (parent.isValid()) // flat model
        return;
    const QAbstractItemModel * const model = static_cast<QAbstractItemModel *>(q->sender());
    const int oldColCount = model->columnCount();
    const int newColCount = columnCountAfterChange(model, oldColCount - (end - start + 1));
    Q_ASSERT(newColCount <= oldColCount);
    if (newColCount < oldColCount)
        q->beginRemoveColumns(QModelIndex(), start, qMax(end, start + oldColCount - newColCount - 1));
    m_newColumnCount = newColCount;
}

void QConcatenateTablesProxyModelPrivate::slotColumnsRemoved(const QModelIndex &parent, int start,
                                                             int end)
{
    Q_Q(QConcatenateTablesProxyModel);
    Q_UNUSED(start);
    Q_UNUSED(end);
    if (parent.isValid()) // flat model
        return;
    if (m_newColumnCount != m_columnCount) {
        m_columnCount = m_newColumnCount;
        q->endRemoveColumns();
    }
}

void QConcatenateTablesProxyModelPrivate::slotColumnsAboutToBeMoved(
        const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
        const QModelIndex &destinationParent, int destination)
{
    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destination)
    if (sourceParent.isValid() || destinationParent.isValid())
        return;
    slotSourceLayoutAboutToBeChanged({}, QAbstractItemModel::HorizontalSortHint);
}

void QConcatenateTablesProxyModelPrivate::slotColumnsMoved(const QModelIndex &sourceParent,
                                                           int sourceStart, int sourceEnd,
                                                           const QModelIndex &destinationParent,
                                                           int destination)
{
    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destination)
    if (sourceParent.isValid() || destinationParent.isValid())
        return;
    slotSourceLayoutChanged({}, QAbstractItemModel::HorizontalSortHint);
}

void QConcatenateTablesProxyModelPrivate::slotDataChanged(const QModelIndex &from,
                                                          const QModelIndex &to,
                                                          const QList<int> &roles)
{
    Q_Q(QConcatenateTablesProxyModel);
    Q_ASSERT(from.isValid());
    Q_ASSERT(to.isValid());
    if (from.column() >= m_columnCount)
        return;
    QModelIndex adjustedTo = to;
    if (to.column() >= m_columnCount)
        adjustedTo = to.siblingAtColumn(m_columnCount - 1);
    const QModelIndex myFrom = q->mapFromSource(from);
    Q_ASSERT(q->checkIndex(myFrom, QAbstractItemModel::CheckIndexOption::IndexIsValid));
    const QModelIndex myTo = q->mapFromSource(adjustedTo);
    Q_ASSERT(q->checkIndex(myTo, QAbstractItemModel::CheckIndexOption::IndexIsValid));
    emit q->dataChanged(myFrom, myTo, roles);
}

void QConcatenateTablesProxyModelPrivate::slotSourceLayoutAboutToBeChanged(
    const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_Q(QConcatenateTablesProxyModel);

    if (!sourceParents.isEmpty() && !sourceParents.contains(QModelIndex()))
        return;

    emit q->layoutAboutToBeChanged({}, hint);

    const QModelIndexList persistentIndexList = q->persistentIndexList();
    layoutChangePersistentIndexes.reserve(persistentIndexList.size());
    layoutChangeProxyIndexes.reserve(persistentIndexList.size());

    for (const QModelIndex &proxyPersistentIndex : persistentIndexList) {
        layoutChangeProxyIndexes.append(proxyPersistentIndex);
        Q_ASSERT(proxyPersistentIndex.isValid());
        const QPersistentModelIndex srcPersistentIndex = q->mapToSource(proxyPersistentIndex);
        Q_ASSERT(srcPersistentIndex.isValid());
        layoutChangePersistentIndexes << srcPersistentIndex;
    }
}

void QConcatenateTablesProxyModelPrivate::slotSourceLayoutChanged(
    const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_Q(QConcatenateTablesProxyModel);
    if (!sourceParents.isEmpty() && !sourceParents.contains(QModelIndex()))
        return;
    for (int i = 0; i < layoutChangeProxyIndexes.size(); ++i) {
        const QModelIndex proxyIdx = layoutChangeProxyIndexes.at(i);
        const QModelIndex newProxyIdx = q->mapFromSource(layoutChangePersistentIndexes.at(i));
        q->changePersistentIndex(proxyIdx, newProxyIdx);
    }

    layoutChangePersistentIndexes.clear();
    layoutChangeProxyIndexes.clear();

    emit q->layoutChanged({}, hint);
}

void QConcatenateTablesProxyModelPrivate::slotModelAboutToBeReset()
{
    Q_Q(QConcatenateTablesProxyModel);
    Q_ASSERT(containsSourceModel(static_cast<QAbstractItemModel *>(q->sender())));
    q->beginResetModel();
    // A reset might reduce both rowCount and columnCount, and we can't notify of both at the same time,
    // and notifying of one after the other leaves an intermediary invalid situation.
    // So the only safe choice is to forward it as a full reset.
}

void QConcatenateTablesProxyModelPrivate::slotModelReset()
{
    Q_Q(QConcatenateTablesProxyModel);
    Q_ASSERT(containsSourceModel(static_cast<QAbstractItemModel *>(q->sender())));
    m_columnCount = calculatedColumnCount();
    m_rowCount = computeRowsPrior(nullptr);
    q->endResetModel();
}

int QConcatenateTablesProxyModelPrivate::calculatedColumnCount() const
{
    if (m_models.isEmpty())
        return 0;

    auto byColumnCount = [](const auto &a, const auto &b) {
        return a.model->columnCount() < b.model->columnCount();
    };
    const auto it = std::min_element(m_models.begin(), m_models.end(), byColumnCount);
    return it->model->columnCount();
}

void QConcatenateTablesProxyModelPrivate::updateColumnCount()
{
    Q_Q(QConcatenateTablesProxyModel);
    const int newColumnCount = calculatedColumnCount();
    const int columnDiff = newColumnCount - m_columnCount;
    if (columnDiff > 0) {
        q->beginInsertColumns(QModelIndex(), m_columnCount, m_columnCount + columnDiff - 1);
        m_columnCount = newColumnCount;
        q->endInsertColumns();
    } else if (columnDiff < 0) {
        const int lastColumn = m_columnCount - 1;
        q->beginRemoveColumns(QModelIndex(), lastColumn + columnDiff + 1, lastColumn);
        m_columnCount = newColumnCount;
        q->endRemoveColumns();
    }
}

int QConcatenateTablesProxyModelPrivate::columnCountAfterChange(const QAbstractItemModel *model, int newCount) const
{
    int newColumnCount = 0;
    for (qsizetype i = 0; i < m_models.size(); ++i) {
        const QAbstractItemModel *mod = m_models.at(i).model;
        const int colCount = mod == model ? newCount : mod->columnCount();
        if (i == 0)
            newColumnCount = colCount;
        else
            newColumnCount = qMin(colCount, newColumnCount);
    }
    return newColumnCount;
}

int QConcatenateTablesProxyModelPrivate::computeRowsPrior(const QAbstractItemModel *sourceModel) const
{
    int rowsPrior = 0;
    for (const auto &[model, _] : m_models) {
        if (model == sourceModel)
            break;
        rowsPrior += model->rowCount();
    }
    return rowsPrior;
}

QConcatenateTablesProxyModelPrivate::SourceModelForRowResult QConcatenateTablesProxyModelPrivate::sourceModelForRow(int row) const
{
    QConcatenateTablesProxyModelPrivate::SourceModelForRowResult result;
    int rowCount = 0;
    for (const auto &[model, _] : m_models) {
        const int subRowCount = model->rowCount();
        if (rowCount + subRowCount > row) {
            result.sourceModel = model;
            break;
        }
        rowCount += subRowCount;
    }
    result.sourceRow = row - rowCount;
    return result;
}

QT_END_NAMESPACE

#include "moc_qconcatenatetablesproxymodel.cpp"
