// Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qidentityproxymodel.h"
#include "qidentityproxymodel_p.h"
#include "qitemselectionmodel.h"
#include <private/qabstractproxymodel_p.h>

QT_BEGIN_NAMESPACE

QIdentityProxyModelPrivate::~QIdentityProxyModelPrivate()
    = default;

/*!
    \since 4.8
    \class QIdentityProxyModel
    \inmodule QtCore
    \brief The QIdentityProxyModel class proxies its source model unmodified.

    \ingroup model-view

    QIdentityProxyModel can be used to forward the structure of a source model exactly, with no sorting, filtering or other transformation.
    This is similar in concept to an identity matrix where A.I = A.

    Because it does no sorting or filtering, this class is most suitable to proxy models which transform the data() of the source model.
    For example, a proxy model could be created to define the font used, or the background colour, or the tooltip etc. This removes the
    need to implement all data handling in the same class that creates the structure of the model, and can also be used to create
    re-usable components.

    This also provides a way to change the data in the case where a source model is supplied by a third party which cannot be modified.

    \snippet code/src_gui_itemviews_qidentityproxymodel.cpp 0

    \sa QAbstractProxyModel, {Model/View Programming}, QAbstractItemModel

*/

/*!
    Constructs an identity model with the given \a parent.
*/
QIdentityProxyModel::QIdentityProxyModel(QObject* parent)
  : QAbstractProxyModel(*new QIdentityProxyModelPrivate, parent)
{

}

/*!
    \internal
 */
QIdentityProxyModel::QIdentityProxyModel(QIdentityProxyModelPrivate &dd, QObject* parent)
  : QAbstractProxyModel(dd, parent)
{

}

/*!
    Destroys this identity model.
*/
QIdentityProxyModel::~QIdentityProxyModel()
{
}

/*!
    \reimp
 */
int QIdentityProxyModel::columnCount(const QModelIndex& parent) const
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(const QIdentityProxyModel);
    return d->model->columnCount(mapToSource(parent));
}

/*!
    \reimp
 */
bool QIdentityProxyModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->dropMimeData(data, action, row, column, mapToSource(parent));
}

/*!
    \reimp
 */
QModelIndex QIdentityProxyModel::index(int row, int column, const QModelIndex& parent) const
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(const QIdentityProxyModel);
    const QModelIndex sourceParent = mapToSource(parent);
    const QModelIndex sourceIndex = d->model->index(row, column, sourceParent);
    return mapFromSource(sourceIndex);
}

/*!
    \reimp
 */
QModelIndex QIdentityProxyModel::sibling(int row, int column, const QModelIndex &idx) const
{
    Q_D(const QIdentityProxyModel);
    return mapFromSource(d->model->sibling(row, column, mapToSource(idx)));
}

/*!
    \reimp
 */
bool QIdentityProxyModel::insertColumns(int column, int count, const QModelIndex& parent)
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->insertColumns(column, count, mapToSource(parent));
}

/*!
    \reimp
 */
bool QIdentityProxyModel::insertRows(int row, int count, const QModelIndex& parent)
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->insertRows(row, count, mapToSource(parent));
}

/*!
    \reimp
 */
QModelIndex QIdentityProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    Q_D(const QIdentityProxyModel);
    if (!d->model || !sourceIndex.isValid())
        return QModelIndex();

    Q_ASSERT(sourceIndex.model() == d->model);
    return createIndex(sourceIndex.row(), sourceIndex.column(), sourceIndex.internalPointer());
}

/*!
    \reimp
 */
QItemSelection QIdentityProxyModel::mapSelectionFromSource(const QItemSelection& selection) const
{
    Q_D(const QIdentityProxyModel);
    QItemSelection proxySelection;

    if (!d->model)
        return proxySelection;

    QItemSelection::const_iterator it = selection.constBegin();
    const QItemSelection::const_iterator end = selection.constEnd();
    proxySelection.reserve(selection.size());
    for ( ; it != end; ++it) {
        Q_ASSERT(it->model() == d->model);
        const QItemSelectionRange range(mapFromSource(it->topLeft()), mapFromSource(it->bottomRight()));
        proxySelection.append(range);
    }

    return proxySelection;
}

/*!
    \reimp
 */
QItemSelection QIdentityProxyModel::mapSelectionToSource(const QItemSelection& selection) const
{
    Q_D(const QIdentityProxyModel);
    QItemSelection sourceSelection;

    if (!d->model)
        return sourceSelection;

    QItemSelection::const_iterator it = selection.constBegin();
    const QItemSelection::const_iterator end = selection.constEnd();
    sourceSelection.reserve(selection.size());
    for ( ; it != end; ++it) {
        Q_ASSERT(it->model() == this);
        const QItemSelectionRange range(mapToSource(it->topLeft()), mapToSource(it->bottomRight()));
        sourceSelection.append(range);
    }

    return sourceSelection;
}

/*!
    \reimp
 */
QModelIndex QIdentityProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
    Q_D(const QIdentityProxyModel);
    if (!d->model || !proxyIndex.isValid())
        return QModelIndex();
    Q_ASSERT(proxyIndex.model() == this);
    return createSourceIndex(proxyIndex.row(), proxyIndex.column(), proxyIndex.internalPointer());
}

/*!
    \reimp
 */
QModelIndexList QIdentityProxyModel::match(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags) const
{
    return QAbstractProxyModel::match(start, role, value, hits, flags);
}

/*!
    \reimp
 */
QModelIndex QIdentityProxyModel::parent(const QModelIndex& child) const
{
    Q_ASSERT(child.isValid() ? child.model() == this : true);
    const QModelIndex sourceIndex = mapToSource(child);
    const QModelIndex sourceParent = sourceIndex.parent();
    return mapFromSource(sourceParent);
}

/*!
    \reimp
 */
bool QIdentityProxyModel::removeColumns(int column, int count, const QModelIndex& parent)
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->removeColumns(column, count, mapToSource(parent));
}

/*!
    \reimp
 */
bool QIdentityProxyModel::removeRows(int row, int count, const QModelIndex& parent)
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->removeRows(row, count, mapToSource(parent));
}

/*!
    \reimp
    \since 5.15
 */
bool QIdentityProxyModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == this : true);
    Q_ASSERT(destinationParent.isValid() ? destinationParent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->moveRows(mapToSource(sourceParent), sourceRow, count, mapToSource(destinationParent), destinationChild);
}

/*!
    \reimp
    \since 5.15
 */
bool QIdentityProxyModel::moveColumns(const QModelIndex &sourceParent, int sourceColumn, int count, const QModelIndex &destinationParent, int destinationChild)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == this : true);
    Q_ASSERT(destinationParent.isValid() ? destinationParent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->moveColumns(mapToSource(sourceParent), sourceColumn, count, mapToSource(destinationParent), destinationChild);
}

/*!
    \reimp
 */
int QIdentityProxyModel::rowCount(const QModelIndex& parent) const
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(const QIdentityProxyModel);
    return d->model->rowCount(mapToSource(parent));
}

/*!
    \reimp
 */
QVariant QIdentityProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_D(const QIdentityProxyModel);
    return d->model->headerData(section, orientation, role);
}

/*!
    \reimp
 */
void QIdentityProxyModel::setSourceModel(QAbstractItemModel* newSourceModel)
{
    Q_D(QIdentityProxyModel);

    if (newSourceModel == d->model)
        return;

    beginResetModel();

    // Call QObject::disconnect() unconditionally, if there is an existing source
    // model, it's disconnected, and if there isn't, then calling disconnect() on
    // a default-constructed Connection does nothing
    for (const auto &c : d->m_sourceModelConnections)
        QObject::disconnect(c);

    QAbstractProxyModel::setSourceModel(newSourceModel);

    if (sourceModel()) {
        auto *m = sourceModel();
        d->m_sourceModelConnections = {
            QObjectPrivate::connect(m, &QAbstractItemModel::rowsAboutToBeInserted, d,
                                    &QIdentityProxyModelPrivate::sourceRowsAboutToBeInserted),
            QObjectPrivate::connect(m, &QAbstractItemModel::rowsInserted, d,
                                    &QIdentityProxyModelPrivate::sourceRowsInserted),
            QObjectPrivate::connect(m, &QAbstractItemModel::rowsAboutToBeRemoved, d,
                                    &QIdentityProxyModelPrivate::sourceRowsAboutToBeRemoved),
            QObjectPrivate::connect(m, &QAbstractItemModel::rowsRemoved, d,
                                    &QIdentityProxyModelPrivate::sourceRowsRemoved),
            QObjectPrivate::connect(m, &QAbstractItemModel::rowsAboutToBeMoved, d,
                                    &QIdentityProxyModelPrivate::sourceRowsAboutToBeMoved),
            QObjectPrivate::connect(m, &QAbstractItemModel::rowsMoved, d,
                                    &QIdentityProxyModelPrivate::sourceRowsMoved),
            QObjectPrivate::connect(m, &QAbstractItemModel::columnsAboutToBeInserted, d,
                                    &QIdentityProxyModelPrivate::sourceColumnsAboutToBeInserted),
            QObjectPrivate::connect(m, &QAbstractItemModel::columnsInserted, d,
                                    &QIdentityProxyModelPrivate::sourceColumnsInserted),
            QObjectPrivate::connect(m, &QAbstractItemModel::columnsAboutToBeRemoved, d,
                                    &QIdentityProxyModelPrivate::sourceColumnsAboutToBeRemoved),
            QObjectPrivate::connect(m, &QAbstractItemModel::columnsRemoved, d,
                                    &QIdentityProxyModelPrivate::sourceColumnsRemoved),
            QObjectPrivate::connect(m, &QAbstractItemModel::columnsAboutToBeMoved, d,
                                    &QIdentityProxyModelPrivate::sourceColumnsAboutToBeMoved),
            QObjectPrivate::connect(m, &QAbstractItemModel::columnsMoved, d,
                                    &QIdentityProxyModelPrivate::sourceColumnsMoved),
            QObjectPrivate::connect(m, &QAbstractItemModel::modelAboutToBeReset, d,
                                    &QIdentityProxyModelPrivate::sourceModelAboutToBeReset),
            QObjectPrivate::connect(m, &QAbstractItemModel::modelReset, d,
                                    &QIdentityProxyModelPrivate::sourceModelReset),
            QObjectPrivate::connect(m, &QAbstractItemModel::headerDataChanged, d,
                                    &QIdentityProxyModelPrivate::sourceHeaderDataChanged),
        };

        if (d->m_handleDataChanges) {
            d->m_sourceModelConnections.emplace_back(
                QObjectPrivate::connect(m, &QAbstractItemModel::dataChanged, d,
                                        &QIdentityProxyModelPrivate::sourceDataChanged));
        }
        if (d->m_handleLayoutChanges) {
            d->m_sourceModelConnections.emplace_back(
                QObjectPrivate::connect(m, &QAbstractItemModel::layoutAboutToBeChanged, d,
                                        &QIdentityProxyModelPrivate::sourceLayoutAboutToBeChanged));
            d->m_sourceModelConnections.emplace_back(
                QObjectPrivate::connect(m, &QAbstractItemModel::layoutChanged, d,
                                        &QIdentityProxyModelPrivate::sourceLayoutChanged));
        }
    }

    endResetModel();
}

/*!
    \since 6.8

    If \a b is \c true, this proxy model will handle the source model layout
    changes (by connecting to \c QAbstractItemModel::layoutAboutToBeChanged
    and \c QAbstractItemModel::layoutChanged signals).

    The default is for this proxy model to handle the source model layout
    changes.

    In sub-classes of QIdentityProxyModel, it may be useful to set this to
    \c false if you need to specially handle the source model layout changes.

    \note Calling this method will only have an effect after calling setSourceModel().
*/
void QIdentityProxyModel::setHandleSourceLayoutChanges(bool b)
{
    d_func()->m_handleLayoutChanges = b;
}

/*!
    \since 6.8

    Returns \c true if this proxy model handles the source model layout
    changes, otherwise returns \c false.
*/
bool QIdentityProxyModel::handleSourceLayoutChanges() const
{
    return d_func()->m_handleLayoutChanges;
}

/*!
    \since 6.8

    If \a b is \c true, this proxy model will handle the source model data
    changes (by connecting to \c QAbstractItemModel::dataChanged signal).

    The default is for this proxy model to handle the source model data
    changes.

    In sub-classes of QIdentityProxyModel, it may be useful to set this to
    \c false if you need to specially handle the source model data changes.

    \note Calling this method will only have an effect after calling setSourceModel().
*/
void QIdentityProxyModel::setHandleSourceDataChanges(bool b)
{
    d_func()->m_handleDataChanges = b;
}

/*!
    \since 6.8

    Returns \c true if this proxy model handles the source model data
    changes, otherwise returns \c false.
*/
bool QIdentityProxyModel::handleSourceDataChanges() const
{
    return d_func()->m_handleDataChanges;
}

void QIdentityProxyModelPrivate::sourceColumnsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginInsertColumns(q->mapFromSource(parent), start, end);
}

void QIdentityProxyModelPrivate::sourceColumnsAboutToBeMoved(const QModelIndex &sourceParent,
                                                             int sourceStart, int sourceEnd,
                                                             const QModelIndex &destParent,
                                                             int dest)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == model : true);
    Q_ASSERT(destParent.isValid() ? destParent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginMoveColumns(q->mapFromSource(sourceParent), sourceStart, sourceEnd, q->mapFromSource(destParent), dest);
}

void QIdentityProxyModelPrivate::sourceColumnsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginRemoveColumns(q->mapFromSource(parent), start, end);
}

void QIdentityProxyModelPrivate::sourceColumnsInserted(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    q->endInsertColumns();
}

void QIdentityProxyModelPrivate::sourceColumnsMoved(const QModelIndex &sourceParent,
                                                    int sourceStart, int sourceEnd,
                                                    const QModelIndex &destParent, int dest)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == model : true);
    Q_ASSERT(destParent.isValid() ? destParent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(sourceParent);
    Q_UNUSED(sourceStart);
    Q_UNUSED(sourceEnd);
    Q_UNUSED(destParent);
    Q_UNUSED(dest);
    q->endMoveColumns();
}

void QIdentityProxyModelPrivate::sourceColumnsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    q->endRemoveColumns();
}

void QIdentityProxyModelPrivate::sourceDataChanged(const QModelIndex &topLeft,
                                                   const QModelIndex &bottomRight,
                                                   const QList<int> &roles)
{
    Q_ASSERT(topLeft.isValid() ? topLeft.model() == model : true);
    Q_ASSERT(bottomRight.isValid() ? bottomRight.model() == model : true);
    Q_Q(QIdentityProxyModel);
    emit q->dataChanged(q->mapFromSource(topLeft), q->mapFromSource(bottomRight), roles);
}

void QIdentityProxyModelPrivate::sourceHeaderDataChanged(Qt::Orientation orientation, int first,
                                                         int last)
{
    Q_Q(QIdentityProxyModel);
    emit q->headerDataChanged(orientation, first, last);
}

void QIdentityProxyModelPrivate::sourceLayoutAboutToBeChanged(
    const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_Q(QIdentityProxyModel);

    QList<QPersistentModelIndex> parents;
    parents.reserve(sourceParents.size());
    for (const QPersistentModelIndex &parent : sourceParents) {
        if (!parent.isValid()) {
            parents << QPersistentModelIndex();
            continue;
        }
        const QModelIndex mappedParent = q->mapFromSource(parent);
        Q_ASSERT(mappedParent.isValid());
        parents << mappedParent;
    }

    emit q->layoutAboutToBeChanged(parents, hint);

    const auto proxyPersistentIndexes = q->persistentIndexList();
    for (const QModelIndex &proxyPersistentIndex : proxyPersistentIndexes) {
        proxyIndexes << proxyPersistentIndex;
        Q_ASSERT(proxyPersistentIndex.isValid());
        const QPersistentModelIndex srcPersistentIndex = q->mapToSource(proxyPersistentIndex);
        Q_ASSERT(srcPersistentIndex.isValid());
        layoutChangePersistentIndexes << srcPersistentIndex;
    }
}

void QIdentityProxyModelPrivate::sourceLayoutChanged(
    const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_Q(QIdentityProxyModel);

    for (int i = 0; i < proxyIndexes.size(); ++i) {
        q->changePersistentIndex(proxyIndexes.at(i), q->mapFromSource(layoutChangePersistentIndexes.at(i)));
    }

    layoutChangePersistentIndexes.clear();
    proxyIndexes.clear();

    QList<QPersistentModelIndex> parents;
    parents.reserve(sourceParents.size());
    for (const QPersistentModelIndex &parent : sourceParents) {
        if (!parent.isValid()) {
            parents << QPersistentModelIndex();
            continue;
        }
        const QModelIndex mappedParent = q->mapFromSource(parent);
        Q_ASSERT(mappedParent.isValid());
        parents << mappedParent;
    }

    emit q->layoutChanged(parents, hint);
}

void QIdentityProxyModelPrivate::sourceModelAboutToBeReset()
{
    Q_Q(QIdentityProxyModel);
    q->beginResetModel();
}

void QIdentityProxyModelPrivate::sourceModelReset()
{
    Q_Q(QIdentityProxyModel);
    q->endResetModel();
}

void QIdentityProxyModelPrivate::sourceRowsAboutToBeInserted(const QModelIndex &parent, int start,
                                                             int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginInsertRows(q->mapFromSource(parent), start, end);
}

void QIdentityProxyModelPrivate::sourceRowsAboutToBeMoved(const QModelIndex &sourceParent,
                                                          int sourceStart, int sourceEnd,
                                                          const QModelIndex &destParent, int dest)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == model : true);
    Q_ASSERT(destParent.isValid() ? destParent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginMoveRows(q->mapFromSource(sourceParent), sourceStart, sourceEnd, q->mapFromSource(destParent), dest);
}

void QIdentityProxyModelPrivate::sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start,
                                                            int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginRemoveRows(q->mapFromSource(parent), start, end);
}

void QIdentityProxyModelPrivate::sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    q->endInsertRows();
}

void QIdentityProxyModelPrivate::sourceRowsMoved(const QModelIndex &sourceParent, int sourceStart,
                                                 int sourceEnd, const QModelIndex &destParent,
                                                 int dest)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == model : true);
    Q_ASSERT(destParent.isValid() ? destParent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(sourceParent);
    Q_UNUSED(sourceStart);
    Q_UNUSED(sourceEnd);
    Q_UNUSED(destParent);
    Q_UNUSED(dest);
    q->endMoveRows();
}

void QIdentityProxyModelPrivate::sourceRowsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    q->endRemoveRows();
}

QT_END_NAMESPACE

#include "moc_qidentityproxymodel.cpp"
