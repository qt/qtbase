// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "listmodel.h"
#include "recycledlistitem.h"

ListModel::ListModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_items()
{

}

ListModel::~ListModel()
{
    qDeleteAll(m_items);
    m_items.clear();
}

int ListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.count();
}

QVariant ListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= m_items.size() || index.row() < 0)
        return QVariant();

    switch (role)
    {
        case Qt::DisplayRole:
            return QVariant::fromValue(m_items.at(index.row())->data(role));
        default:
            return QVariant();
    }
}

bool ListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // TODO implement if we like to edit list items
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);
    return false;
}

void ListModel::clear()
{
    m_items.clear();
    clear();
}

QModelIndex ListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (hasIndex(row, column, parent))
        return createIndex(row, column, m_items.at(row));

    return QModelIndex();
}

void ListModel::insert(int row, RecycledListItem *item)
{
    if (item)
        item->setModel(this);
    if (!item || m_items.contains(item) ) {
        return;
    }
    if (row < 0)
        row = 0;
    else if (row > m_items.count())
        row = m_items.count();
    beginInsertRows(QModelIndex(), row, row);
    m_items.insert(row, item);
    endInsertRows();
}

void ListModel::appendRow(RecycledListItem *item)
{
    if (!item) return;
    item->setModel(this);
    insert(rowCount(),item);
}

RecycledListItem *ListModel::item(const int row) const
{
    if (row < 0 || row > m_items.count())
         return 0;
    return m_items.at(row);
}

RecycledListItem *ListModel::takeItem(const int row)
{
    if (row < 0 || row >= m_items.count())
         return 0;

    beginRemoveRows(QModelIndex(), row, row);
    RecycledListItem *item = m_items.takeAt(row);
    endRemoveRows();

    return item;
}

