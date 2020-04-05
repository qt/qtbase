/****************************************************************************
**
** Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qconcatenatetablesproxymodel.h"
#include <private/qabstractitemmodel_p.h>
#include "qsize.h"
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

    void _q_slotRowsAboutToBeInserted(const QModelIndex &, int start, int end);
    void _q_slotRowsInserted(const QModelIndex &, int start, int end);
    void _q_slotRowsAboutToBeRemoved(const QModelIndex &, int start, int end);
    void _q_slotRowsRemoved(const QModelIndex &, int start, int end);
    void _q_slotColumnsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void _q_slotColumnsInserted(const QModelIndex &parent, int, int);
    void _q_slotColumnsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void _q_slotColumnsRemoved(const QModelIndex &parent, int, int);
    void _q_slotDataChanged(const QModelIndex &from, const QModelIndex &to, const QVector<int> &roles);
    void _q_slotSourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint);
    void _q_slotSourceLayoutChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint);
    void _q_slotModelAboutToBeReset();
    void _q_slotModelReset();
    int columnCountAfterChange(const QAbstractItemModel *model, int newCount) const;
    int calculatedColumnCount() const;
    void updateColumnCount();
    bool mapDropCoordinatesToSource(int row, int column, const QModelIndex &parent,
                                    int *sourceRow, int *sourceColumn, QModelIndex *sourceParent, QAbstractItemModel **sourceModel) const;

    QVector<QAbstractItemModel *> m_models;
    int m_rowCount; // have to maintain it here since we can't compute during model destruction
    int m_columnCount;

    // for columns{AboutToBe,}{Inserted,Removed}
    int m_newColumnCount;

    // for layoutAboutToBeChanged/layoutChanged
    QVector<QPersistentModelIndex> layoutChangePersistentIndexes;
    QVector<QModelIndex> layoutChangeProxyIndexes;
};

QConcatenateTablesProxyModelPrivate::QConcatenateTablesProxyModelPrivate()
    : m_rowCount(0),
      m_columnCount(0),
      m_newColumnCount(0)
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
    if (!d->m_models.contains(const_cast<QAbstractItemModel *>(sourceModel))) {
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
        return d->m_models.at(0)->flags(index);
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
            return d->m_models.at(0)->headerData(section, orientation, role);
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
    Q_ASSERT(checkIndex(parent, QAbstractItemModel::CheckIndexOption::ParentIsInvalid)); // flat model
    Q_UNUSED(parent);
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
    return d->m_models.at(0)->mimeTypes();
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
    sourceIndexes.reserve(indexes.count());
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
            *sourceModel = m_models.constLast();
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
    return d->m_models.toList();
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
    Q_ASSERT(!d->m_models.contains(sourceModel));
    connect(sourceModel, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(_q_slotDataChanged(QModelIndex,QModelIndex,QVector<int>)));
    connect(sourceModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(_q_slotRowsInserted(QModelIndex,int,int)));
    connect(sourceModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(_q_slotRowsRemoved(QModelIndex,int,int)));
    connect(sourceModel, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)), this, SLOT(_q_slotRowsAboutToBeInserted(QModelIndex,int,int)));
    connect(sourceModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(_q_slotRowsAboutToBeRemoved(QModelIndex,int,int)));

    connect(sourceModel, SIGNAL(columnsInserted(QModelIndex,int,int)), this, SLOT(_q_slotColumnsInserted(QModelIndex,int,int)));
    connect(sourceModel, SIGNAL(columnsRemoved(QModelIndex,int,int)), this, SLOT(_q_slotColumnsRemoved(QModelIndex,int,int)));
    connect(sourceModel, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)), this, SLOT(_q_slotColumnsAboutToBeInserted(QModelIndex,int,int)));
    connect(sourceModel, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(_q_slotColumnsAboutToBeRemoved(QModelIndex,int,int)));

    connect(sourceModel, SIGNAL(layoutAboutToBeChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)),
            this, SLOT(_q_slotSourceLayoutAboutToBeChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)));
    connect(sourceModel, SIGNAL(layoutChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)),
            this, SLOT(_q_slotSourceLayoutChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)));
    connect(sourceModel, SIGNAL(modelAboutToBeReset()), this, SLOT(_q_slotModelAboutToBeReset()));
    connect(sourceModel, SIGNAL(modelReset()), this, SLOT(_q_slotModelReset()));

    const int newRows = sourceModel->rowCount();
    if (newRows > 0)
        beginInsertRows(QModelIndex(), d->m_rowCount, d->m_rowCount + newRows - 1);
    d->m_rowCount += newRows;
    d->m_models.append(sourceModel);
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
    Q_ASSERT(d->m_models.contains(sourceModel));
    disconnect(sourceModel, nullptr, this, nullptr);

    const int rowsRemoved = sourceModel->rowCount();
    const int rowsPrior = d->computeRowsPrior(sourceModel);   // location of removed section

    if (rowsRemoved > 0)
        beginRemoveRows(QModelIndex(), rowsPrior, rowsPrior + rowsRemoved - 1);
    d->m_models.removeOne(sourceModel);
    d->m_rowCount -= rowsRemoved;
    if (rowsRemoved > 0)
        endRemoveRows();

    d->updateColumnCount();
}

void QConcatenateTablesProxyModelPrivate::_q_slotRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    Q_Q(QConcatenateTablesProxyModel);
    if (parent.isValid()) // not supported, the proxy is a flat model
        return;
    const QAbstractItemModel * const model = static_cast<QAbstractItemModel *>(q->sender());
    const int rowsPrior = computeRowsPrior(model);
    q->beginInsertRows(QModelIndex(), rowsPrior + start, rowsPrior + end);
}

void QConcatenateTablesProxyModelPrivate::_q_slotRowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_Q(QConcatenateTablesProxyModel);
    if (parent.isValid()) // flat model
        return;
    m_rowCount += end - start + 1;
    q->endInsertRows();
}

void QConcatenateTablesProxyModelPrivate::_q_slotRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Q_Q(QConcatenateTablesProxyModel);
    if (parent.isValid()) // flat model
        return;
    const QAbstractItemModel * const model = static_cast<QAbstractItemModel *>(q->sender());
    const int rowsPrior = computeRowsPrior(model);
    q->beginRemoveRows(QModelIndex(), rowsPrior + start, rowsPrior + end);
}

void QConcatenateTablesProxyModelPrivate::_q_slotRowsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_Q(QConcatenateTablesProxyModel);
    if (parent.isValid()) // flat model
        return;
    m_rowCount -= end - start + 1;
    q->endRemoveRows();
}

void QConcatenateTablesProxyModelPrivate::_q_slotColumnsAboutToBeInserted(const QModelIndex &parent, int start, int end)
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

void QConcatenateTablesProxyModelPrivate::_q_slotColumnsInserted(const QModelIndex &parent, int start, int end)
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

void QConcatenateTablesProxyModelPrivate::_q_slotColumnsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
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

void QConcatenateTablesProxyModelPrivate::_q_slotColumnsRemoved(const QModelIndex &parent, int start, int end)
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

void QConcatenateTablesProxyModelPrivate::_q_slotDataChanged(const QModelIndex &from, const QModelIndex &to, const QVector<int> &roles)
{
    Q_Q(QConcatenateTablesProxyModel);
    Q_ASSERT(from.isValid());
    Q_ASSERT(to.isValid());
    const QModelIndex myFrom = q->mapFromSource(from);
    Q_ASSERT(q->checkIndex(myFrom, QAbstractItemModel::CheckIndexOption::IndexIsValid));
    const QModelIndex myTo = q->mapFromSource(to);
    Q_ASSERT(q->checkIndex(myTo, QAbstractItemModel::CheckIndexOption::IndexIsValid));
    emit q->dataChanged(myFrom, myTo, roles);
}

void QConcatenateTablesProxyModelPrivate::_q_slotSourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint)
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

void QConcatenateTablesProxyModelPrivate::_q_slotSourceLayoutChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint)
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

void QConcatenateTablesProxyModelPrivate::_q_slotModelAboutToBeReset()
{
    Q_Q(QConcatenateTablesProxyModel);
    Q_ASSERT(m_models.contains(const_cast<QAbstractItemModel *>(static_cast<const QAbstractItemModel *>(q->sender()))));
    q->beginResetModel();
    // A reset might reduce both rowCount and columnCount, and we can't notify of both at the same time,
    // and notifying of one after the other leaves an intermediary invalid situation.
    // So the only safe choice is to forward it as a full reset.
}

void QConcatenateTablesProxyModelPrivate::_q_slotModelReset()
{
    Q_Q(QConcatenateTablesProxyModel);
    Q_ASSERT(m_models.contains(const_cast<QAbstractItemModel *>(static_cast<const QAbstractItemModel *>(q->sender()))));
    m_columnCount = calculatedColumnCount();
    m_rowCount = computeRowsPrior(nullptr);
    q->endResetModel();
}

int QConcatenateTablesProxyModelPrivate::calculatedColumnCount() const
{
    if (m_models.isEmpty())
        return 0;

    const auto it = std::min_element(m_models.begin(), m_models.end(), [](const QAbstractItemModel* model1, const QAbstractItemModel* model2) {
                        return model1->columnCount() < model2->columnCount();
                    });
    return (*it)->columnCount();
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
    for (int i = 0; i < m_models.count(); ++i) {
        const QAbstractItemModel *mod = m_models.at(i);
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
    for (const QAbstractItemModel *model : m_models) {
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
    for (QAbstractItemModel *model : m_models) {
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
