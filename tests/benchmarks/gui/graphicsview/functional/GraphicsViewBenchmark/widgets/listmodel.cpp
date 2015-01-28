/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

void ListModel::clear()
{
    m_items.clear();
    reset();
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

