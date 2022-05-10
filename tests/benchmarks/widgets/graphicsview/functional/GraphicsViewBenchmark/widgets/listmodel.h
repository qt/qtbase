// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef LISTMODEL_H
#define LISTMODEL_H

#include <QAbstractListModel>

class RecycledListItem;
class ListItemCache;

class ListModel : public QAbstractListModel
{
    Q_OBJECT

public:

    ListModel(QObject *parent = nullptr);
    ~ListModel();

public:

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole ) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    void insert(const int row, RecycledListItem *item);
    void appendRow(RecycledListItem *item);

    void clear();

    RecycledListItem *item(const int row) const;

    RecycledListItem *takeItem(const int row);

private:
    Q_DISABLE_COPY(ListModel)
    QList<RecycledListItem *> m_items;
};

#endif // LISTMODEL_H
