/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QABSTRACTITEMMODEL_P_H
#define QABSTRACTITEMMODEL_P_H

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

#include "QtCore/qabstractitemmodel.h"
#include "QtCore/private/qobject_p.h"
#include "QtCore/qstack.h"
#include "QtCore/qset.h"
#include "QtCore/qhash.h"

QT_BEGIN_NAMESPACE

QT_REQUIRE_CONFIG(itemmodel);

class QPersistentModelIndexData
{
public:
    QPersistentModelIndexData() {}
    QPersistentModelIndexData(const QModelIndex &idx) : index(idx) {}
    QModelIndex index;
    QAtomicInt ref;
    static QPersistentModelIndexData *create(const QModelIndex &index);
    static void destroy(QPersistentModelIndexData *data);
};

class Q_CORE_EXPORT QAbstractItemModelPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAbstractItemModel)

public:
    QAbstractItemModelPrivate();
    ~QAbstractItemModelPrivate();

    void removePersistentIndexData(QPersistentModelIndexData *data);
    void movePersistentIndexes(const QVector<QPersistentModelIndexData *> &indexes, int change, const QModelIndex &parent, Qt::Orientation orientation);
    void rowsAboutToBeInserted(const QModelIndex &parent, int first, int last);
    void rowsInserted(const QModelIndex &parent, int first, int last);
    void rowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void rowsRemoved(const QModelIndex &parent, int first, int last);
    void columnsAboutToBeInserted(const QModelIndex &parent, int first, int last);
    void columnsInserted(const QModelIndex &parent, int first, int last);
    void columnsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void columnsRemoved(const QModelIndex &parent, int first, int last);
    static QAbstractItemModel *staticEmptyModel();
    static bool variantLessThan(const QVariant &v1, const QVariant &v2);

    void itemsAboutToBeMoved(const QModelIndex &srcParent, int srcFirst, int srcLast, const QModelIndex &destinationParent, int destinationChild, Qt::Orientation);
    void itemsMoved(const QModelIndex &srcParent, int srcFirst, int srcLast, const QModelIndex &destinationParent, int destinationChild, Qt::Orientation orientation);
    bool allowMove(const QModelIndex &srcParent, int srcFirst, int srcLast, const QModelIndex &destinationParent, int destinationChild, Qt::Orientation orientation);

    inline QModelIndex createIndex(int row, int column, void *data = nullptr) const {
        return q_func()->createIndex(row, column, data);
    }

    inline QModelIndex createIndex(int row, int column, int id) const {
        return q_func()->createIndex(row, column, id);
    }

    inline bool indexValid(const QModelIndex &index) const {
         return (index.row() >= 0) && (index.column() >= 0) && (index.model() == q_func());
    }

    void invalidatePersistentIndexes();
    void invalidatePersistentIndex(const QModelIndex &index);

    struct Change {
        Q_DECL_CONSTEXPR Change() : parent(), first(-1), last(-1), needsAdjust(false) {}
        Q_DECL_CONSTEXPR Change(const QModelIndex &p, int f, int l) : parent(p), first(f), last(l), needsAdjust(false) {}

        QModelIndex parent;
        int first, last;


        // In cases such as this:
        // - A
        // - B
        // - C
        // - - D
        // - - E
        // - - F
        //
        // If B is moved to above E, C is the source parent in the signal and its row is 2. When the move is
        // completed however, C is at row 1 and there is no row 2 at the same level in the model at all.
        // The QModelIndex is adjusted to correct that in those cases before reporting it though the
        // rowsMoved signal.
        bool needsAdjust;

        Q_DECL_CONSTEXPR bool isValid() const { return first >= 0 && last >= 0; }
    };
    QStack<Change> changes;

    struct Persistent {
        Persistent() {}
        QMultiHash<QModelIndex, QPersistentModelIndexData *> indexes;
        QStack<QVector<QPersistentModelIndexData *> > moved;
        QStack<QVector<QPersistentModelIndexData *> > invalidated;
        void insertMultiAtEnd(const QModelIndex& key, QPersistentModelIndexData *data);
    } persistent;

    Qt::DropActions supportedDragActions;

    QHash<int,QByteArray> roleNames;
    static const QHash<int,QByteArray> &defaultRoleNames();
    static bool isVariantLessThan(const QVariant &left, const QVariant &right,
                                  Qt::CaseSensitivity cs = Qt::CaseSensitive, bool isLocaleAware = false);
};
Q_DECLARE_TYPEINFO(QAbstractItemModelPrivate::Change, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

#endif // QABSTRACTITEMMODEL_P_H
