// Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIDENTITYPROXYMODEL_P_H
#define QIDENTITYPROXYMODEL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QAbstractItemModel*.  This header file may change from version
// to version without notice, or even be removed.
//
// We mean it.
//
//

#include <QtCore/private/qabstractproxymodel_p.h>
#include <QtCore/qidentityproxymodel.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QIdentityProxyModelPrivate : public QAbstractProxyModelPrivate
{
    Q_DECLARE_PUBLIC(QIdentityProxyModel)

public:
    QIdentityProxyModelPrivate()
    {
    }

    QList<QPersistentModelIndex> layoutChangePersistentIndexes;
    QModelIndexList proxyIndexes;

    void _q_sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void _q_sourceRowsInserted(const QModelIndex &parent, int start, int end);
    void _q_sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void _q_sourceRowsRemoved(const QModelIndex &parent, int start, int end);
    void _q_sourceRowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest);
    void _q_sourceRowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest);

    void _q_sourceColumnsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void _q_sourceColumnsInserted(const QModelIndex &parent, int start, int end);
    void _q_sourceColumnsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void _q_sourceColumnsRemoved(const QModelIndex &parent, int start, int end);
    void _q_sourceColumnsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest);
    void _q_sourceColumnsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest);

    void _q_sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);
    void _q_sourceHeaderDataChanged(Qt::Orientation orientation, int first, int last);

    void _q_sourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint);
    void _q_sourceLayoutChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint);
    void _q_sourceModelAboutToBeReset();
    void _q_sourceModelReset();

};

QT_END_NAMESPACE

#endif // QIDENTITYPROXYMODEL_P_H
