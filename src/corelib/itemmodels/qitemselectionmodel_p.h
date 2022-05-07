/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QITEMSELECTIONMODEL_P_H
#define QITEMSELECTIONMODEL_P_H

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

#include "private/qobject_p.h"
#include "private/qproperty_p.h"

QT_REQUIRE_CONFIG(itemmodel);

QT_BEGIN_NAMESPACE

class QItemSelectionModelPrivate: public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QItemSelectionModel)
public:
    QItemSelectionModelPrivate()
      : currentCommand(QItemSelectionModel::NoUpdate),
        tableSelected(false), tableColCount(0), tableRowCount(0) {}

    QItemSelection expandSelection(const QItemSelection &selection,
                                   QItemSelectionModel::SelectionFlags command) const;

    void initModel(QAbstractItemModel *model);

    void _q_rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void _q_columnsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void _q_rowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void _q_columnsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void _q_layoutAboutToBeChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
    void _q_layoutChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
    void _q_modelDestroyed();

    inline void remove(QList<QItemSelectionRange> &r)
    {
        QList<QItemSelectionRange>::const_iterator it = r.constBegin();
        for (; it != r.constEnd(); ++it)
            ranges.removeAll(*it);
    }

    inline void finalize()
    {
        ranges.merge(currentSelection, currentCommand);
        if (!currentSelection.isEmpty())  // ### perhaps this should be in QList
            currentSelection.clear();
    }

    void setModel(QAbstractItemModel *mod) { q_func()->setModel(mod); }
    void modelChanged(QAbstractItemModel *mod) { q_func()->modelChanged(mod); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QItemSelectionModelPrivate, QAbstractItemModel *, model,
                                       &QItemSelectionModelPrivate::setModel,
                                       &QItemSelectionModelPrivate::modelChanged, nullptr)

    QItemSelection ranges;
    QItemSelection currentSelection;
    QPersistentModelIndex currentIndex;
    QItemSelectionModel::SelectionFlags currentCommand;
    QList<QPersistentModelIndex> savedPersistentIndexes;
    QList<QPersistentModelIndex> savedPersistentCurrentIndexes;
    QList<QPair<QPersistentModelIndex, uint>> savedPersistentRowLengths;
    QList<QPair<QPersistentModelIndex, uint>> savedPersistentCurrentRowLengths;
    // optimization when all indexes are selected
    bool tableSelected;
    QPersistentModelIndex tableParent;
    int tableColCount, tableRowCount;
};

QT_END_NAMESPACE

#endif // QITEMSELECTIONMODEL_P_H
