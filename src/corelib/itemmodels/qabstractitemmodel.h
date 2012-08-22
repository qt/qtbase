/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QABSTRACTITEMMODEL_H
#define QABSTRACTITEMMODEL_H

#include <QtCore/qvariant.h>
#include <QtCore/qobject.h>
#include <QtCore/qhash.h>
#include <QtCore/qvector.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QAbstractItemModel;
class QPersistentModelIndex;

class Q_CORE_EXPORT QModelIndex
{
    friend class QAbstractItemModel;
public:
    inline QModelIndex() : r(-1), c(-1), p(0), m(0) {}
    // compiler-generated copy/move ctors/assignment operators are fine!
    inline int row() const { return r; }
    inline int column() const { return c; }
    inline void *internalPointer() const { return p; }
    inline qint64 internalId() const { return reinterpret_cast<qint64>(p); }
    inline QModelIndex parent() const;
    inline QModelIndex sibling(int row, int column) const;
    inline QModelIndex child(int row, int column) const;
    inline QVariant data(int role = Qt::DisplayRole) const;
    inline Qt::ItemFlags flags() const;
    inline const QAbstractItemModel *model() const { return m; }
    inline bool isValid() const { return (r >= 0) && (c >= 0) && (m != 0); }
    inline bool operator==(const QModelIndex &other) const
        { return (other.r == r) && (other.p == p) && (other.c == c) && (other.m == m); }
    inline bool operator!=(const QModelIndex &other) const
        { return !(*this == other); }
    inline bool operator<(const QModelIndex &other) const
        {
          if (r < other.r) return true;
          if (r == other.r) {
              if (c < other.c) return true;
              if (c == other.c) {
                  if (p < other.p) return true;
                  if (p == other.p) return m < other.m;
              }
          }
          return false; }
private:
    inline QModelIndex(int row, int column, void *ptr, const QAbstractItemModel *model);
    int r, c;
    void *p;
    const QAbstractItemModel *m;
};
Q_DECLARE_TYPEINFO(QModelIndex, Q_MOVABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QModelIndex &);
#endif

class QPersistentModelIndexData;

class Q_CORE_EXPORT QPersistentModelIndex
{
public:
    QPersistentModelIndex();
    QPersistentModelIndex(const QModelIndex &index);
    QPersistentModelIndex(const QPersistentModelIndex &other);
    ~QPersistentModelIndex();
    bool operator<(const QPersistentModelIndex &other) const;
    bool operator==(const QPersistentModelIndex &other) const;
    inline bool operator!=(const QPersistentModelIndex &other) const
    { return !operator==(other); }
    QPersistentModelIndex &operator=(const QPersistentModelIndex &other);
    inline void swap(QPersistentModelIndex &other) { qSwap(d, other.d); }
    bool operator==(const QModelIndex &other) const;
    bool operator!=(const QModelIndex &other) const;
    QPersistentModelIndex &operator=(const QModelIndex &other);
    operator const QModelIndex&() const;
    int row() const;
    int column() const;
    void *internalPointer() const;
    qint64 internalId() const;
    QModelIndex parent() const;
    QModelIndex sibling(int row, int column) const;
    QModelIndex child(int row, int column) const;
    QVariant data(int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags() const;
    const QAbstractItemModel *model() const;
    bool isValid() const;
private:
    QPersistentModelIndexData *d;
    friend uint qHash(const QPersistentModelIndex &, uint seed = 0);
#ifndef QT_NO_DEBUG_STREAM
    friend Q_CORE_EXPORT QDebug operator<<(QDebug, const QPersistentModelIndex &);
#endif
};
Q_DECLARE_SHARED(QPersistentModelIndex)

inline uint qHash(const QPersistentModelIndex &index, uint seed)
{ return qHash(index.d, seed); }


#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QPersistentModelIndex &);
#endif

template<typename T> class QList;
typedef QList<QModelIndex> QModelIndexList;

class QMimeData;
class QAbstractItemModelPrivate;
template <class Key, class T> class QMap;


class Q_CORE_EXPORT QAbstractItemModel : public QObject
{
    Q_OBJECT

    friend class QPersistentModelIndexData;
    friend class QAbstractItemViewPrivate;
    friend class QIdentityProxyModel;
public:

    explicit QAbstractItemModel(QObject *parent = 0);
    virtual ~QAbstractItemModel();

    bool hasIndex(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex index(int row, int column,
                              const QModelIndex &parent = QModelIndex()) const = 0;
    virtual QModelIndex parent(const QModelIndex &child) const = 0;

    virtual QModelIndex sibling(int row, int column, const QModelIndex &idx) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const = 0;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const = 0;
    virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const = 0;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;
    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value,
                               int role = Qt::EditRole);

    virtual QMap<int, QVariant> itemData(const QModelIndex &index) const;
    virtual bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles);

    virtual QStringList mimeTypes() const;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action,
                                 int row, int column, const QModelIndex &parent) const;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                              int row, int column, const QModelIndex &parent);
    virtual Qt::DropActions supportedDropActions() const;

    virtual Qt::DropActions supportedDragActions() const;
#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED void setSupportedDragActions(Qt::DropActions actions)
    { doSetSupportedDragActions(actions); }
#endif

    virtual bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
    virtual bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex());
    virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    virtual bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex());
    virtual bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                          const QModelIndex &destinationParent, int destinationChild);
    virtual bool moveColumns(const QModelIndex &sourceParent, int sourceColumn, int count,
                             const QModelIndex &destinationParent, int destinationChild);

    inline bool insertRow(int row, const QModelIndex &parent = QModelIndex());
    inline bool insertColumn(int column, const QModelIndex &parent = QModelIndex());
    inline bool removeRow(int row, const QModelIndex &parent = QModelIndex());
    inline bool removeColumn(int column, const QModelIndex &parent = QModelIndex());
    inline bool moveRow(const QModelIndex &sourceParent, int sourceRow,
                        const QModelIndex &destinationParent, int destinationChild);
    inline bool moveColumn(const QModelIndex &sourceParent, int sourceColumn,
                           const QModelIndex &destinationParent, int destinationChild);

    virtual void fetchMore(const QModelIndex &parent);
    virtual bool canFetchMore(const QModelIndex &parent) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
    virtual QModelIndex buddy(const QModelIndex &index) const;
    virtual QModelIndexList match(const QModelIndex &start, int role,
                                  const QVariant &value, int hits = 1,
                                  Qt::MatchFlags flags =
                                  Qt::MatchFlags(Qt::MatchStartsWith|Qt::MatchWrap)) const;
    virtual QSize span(const QModelIndex &index) const;

    virtual QHash<int,QByteArray> roleNames() const;

#ifdef Q_NO_USING_KEYWORD
    inline QObject *parent() const { return QObject::parent(); }
#else
    using QObject::parent;
#endif

Q_SIGNALS:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
    void headerDataChanged(Qt::Orientation orientation, int first, int last);
    void layoutChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>());
    void layoutAboutToBeChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>());

    void rowsAboutToBeInserted(const QModelIndex &parent, int first, int last
#if !defined(qdoc)
      , QPrivateSignal
#endif
    );
    void rowsInserted(const QModelIndex &parent, int first, int last
#if !defined(qdoc)
      , QPrivateSignal
#endif
    );

    void rowsAboutToBeRemoved(const QModelIndex &parent, int first, int last
#if !defined(qdoc)
      , QPrivateSignal
#endif
    );
    void rowsRemoved(const QModelIndex &parent, int first, int last
#if !defined(qdoc)
      , QPrivateSignal
#endif
    );

    void columnsAboutToBeInserted(const QModelIndex &parent, int first, int last
#if !defined(qdoc)
      , QPrivateSignal
#endif
    );
    void columnsInserted(const QModelIndex &parent, int first, int last
#if !defined(qdoc)
      , QPrivateSignal
#endif
    );

    void columnsAboutToBeRemoved(const QModelIndex &parent, int first, int last
#if !defined(qdoc)
      , QPrivateSignal
#endif
    );
    void columnsRemoved(const QModelIndex &parent, int first, int last
#if !defined(qdoc)
      , QPrivateSignal
#endif
    );

    void modelAboutToBeReset(
#if !defined(qdoc)
      QPrivateSignal
#endif
    );
    void modelReset(
#if !defined(qdoc)
      QPrivateSignal
#endif
    );

    void rowsAboutToBeMoved( const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow
#if !defined(qdoc)
      , QPrivateSignal
#endif
    );
    void rowsMoved( const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row
#if !defined(qdoc)
      , QPrivateSignal
#endif
    );

    void columnsAboutToBeMoved( const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationColumn
#if !defined(qdoc)
      , QPrivateSignal
#endif
    );
    void columnsMoved( const QModelIndex &parent, int start, int end, const QModelIndex &destination, int column
#if !defined(qdoc)
      , QPrivateSignal
#endif
    );

public Q_SLOTS:
    virtual bool submit();
    virtual void revert();

protected:
    QAbstractItemModel(QAbstractItemModelPrivate &dd, QObject *parent = 0);

    inline QModelIndex createIndex(int row, int column, void *data = 0) const;
    inline QModelIndex createIndex(int row, int column, int id) const;
    inline QModelIndex createIndex(int row, int column, quint32 id) const;

    void encodeData(const QModelIndexList &indexes, QDataStream &stream) const;
    bool decodeData(int row, int column, const QModelIndex &parent, QDataStream &stream);

    void beginInsertRows(const QModelIndex &parent, int first, int last);
    void endInsertRows();

    void beginRemoveRows(const QModelIndex &parent, int first, int last);
    void endRemoveRows();

    bool beginMoveRows(const QModelIndex &sourceParent, int sourceFirst, int sourceLast, const QModelIndex &destinationParent, int destinationRow);
    void endMoveRows();

    void beginInsertColumns(const QModelIndex &parent, int first, int last);
    void endInsertColumns();

    void beginRemoveColumns(const QModelIndex &parent, int first, int last);
    void endRemoveColumns();

    bool beginMoveColumns(const QModelIndex &sourceParent, int sourceFirst, int sourceLast, const QModelIndex &destinationParent, int destinationColumn);
    void endMoveColumns();


#if QT_DEPRECATED_SINCE(5,0)
    QT_DEPRECATED void reset()
    {
        beginResetModel();
        endResetModel();
    }
#endif

    void beginResetModel();
    void endResetModel();

    void changePersistentIndex(const QModelIndex &from, const QModelIndex &to);
    void changePersistentIndexList(const QModelIndexList &from, const QModelIndexList &to);
    QModelIndexList persistentIndexList() const;

#if QT_DEPRECATED_SINCE(5,0)
    QT_DEPRECATED void setRoleNames(const QHash<int,QByteArray> &theRoleNames)
    {
        doSetRoleNames(theRoleNames);
    }
#endif

private:
    void doSetRoleNames(const QHash<int,QByteArray> &roleNames);
    void doSetSupportedDragActions(Qt::DropActions actions);

    Q_DECLARE_PRIVATE(QAbstractItemModel)
    Q_DISABLE_COPY(QAbstractItemModel)
};

inline bool QAbstractItemModel::insertRow(int arow, const QModelIndex &aparent)
{ return insertRows(arow, 1, aparent); }
inline bool QAbstractItemModel::insertColumn(int acolumn, const QModelIndex &aparent)
{ return insertColumns(acolumn, 1, aparent); }
inline bool QAbstractItemModel::removeRow(int arow, const QModelIndex &aparent)
{ return removeRows(arow, 1, aparent); }
inline bool QAbstractItemModel::removeColumn(int acolumn, const QModelIndex &aparent)
{ return removeColumns(acolumn, 1, aparent); }
inline bool QAbstractItemModel::moveRow(const QModelIndex &sourceParent, int sourceRow,
                                        const QModelIndex &destinationParent, int destinationChild)
{ return moveRows(sourceParent, sourceRow, 1, destinationParent, destinationChild); }
inline bool QAbstractItemModel::moveColumn(const QModelIndex &sourceParent, int sourceColumn,
                                           const QModelIndex &destinationParent, int destinationChild)
{ return moveRows(sourceParent, sourceColumn, 1, destinationParent, destinationChild); }
inline QModelIndex QAbstractItemModel::createIndex(int arow, int acolumn, void *adata) const
{ return QModelIndex(arow, acolumn, adata, this); }
inline QModelIndex QAbstractItemModel::createIndex(int arow, int acolumn, int aid) const
#if defined(Q_CC_MSVC)
#pragma warning( push )
#pragma warning( disable : 4312 ) // avoid conversion warning on 64-bit
#endif
{ return QModelIndex(arow, acolumn, reinterpret_cast<void*>(aid), this); }
#if defined(Q_CC_MSVC)
#pragma warning( pop )
#endif
inline QModelIndex QAbstractItemModel::createIndex(int arow, int acolumn, quint32 aid) const
#if defined(Q_CC_MSVC)
#pragma warning( push )
#pragma warning( disable : 4312 ) // avoid conversion warning on 64-bit
#endif
{ return QModelIndex(arow, acolumn, reinterpret_cast<void*>(aid), this); }
#if defined(Q_CC_MSVC)
#pragma warning( pop )
#endif


class Q_CORE_EXPORT QAbstractTableModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit QAbstractTableModel(QObject *parent = 0);
    ~QAbstractTableModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent);
protected:
    QAbstractTableModel(QAbstractItemModelPrivate &dd, QObject *parent);

private:
    Q_DISABLE_COPY(QAbstractTableModel)
    QModelIndex parent(const QModelIndex &child) const;
    bool hasChildren(const QModelIndex &parent) const;
};

class Q_CORE_EXPORT QAbstractListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit QAbstractListModel(QObject *parent = 0);
    ~QAbstractListModel();

    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent);
protected:
    QAbstractListModel(QAbstractItemModelPrivate &dd, QObject *parent);

private:
    Q_DISABLE_COPY(QAbstractListModel)
    QModelIndex parent(const QModelIndex &child) const;
    int columnCount(const QModelIndex &parent) const;
    bool hasChildren(const QModelIndex &parent) const;
};

// inline implementations

inline QModelIndex::QModelIndex(int arow, int acolumn, void *adata,
                                const QAbstractItemModel *amodel)
    : r(arow), c(acolumn), p(adata), m(amodel) {}

inline QModelIndex QModelIndex::parent() const
{ return m ? m->parent(*this) : QModelIndex(); }

inline QModelIndex QModelIndex::sibling(int arow, int acolumn) const
{ return m ? (r == arow && c == acolumn) ? *this : m->sibling(arow, acolumn, *this) : QModelIndex(); }

inline QModelIndex QModelIndex::child(int arow, int acolumn) const
{ return m ? m->index(arow, acolumn, *this) : QModelIndex(); }

inline QVariant QModelIndex::data(int arole) const
{ return m ? m->data(*this, arole) : QVariant(); }

inline Qt::ItemFlags QModelIndex::flags() const
{ return m ? m->flags(*this) : Qt::ItemFlags(0); }

inline uint qHash(const QModelIndex &index)
{ return uint((index.row() << 4) + index.column() + index.internalId()); }

QT_END_NAMESPACE

QT_END_HEADER

#endif // QABSTRACTITEMMODEL_H
