// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
    model.cpp

    Provides a table model for use in various examples.
*/

#include <QtGui>

#include "model.h"

/*!
    Constructs a table model with at least one row and one column.
*/

TableModel::TableModel(int rows, int columns, QObject *parent)
    : QAbstractTableModel(parent)
{
    QStringList newList;

    for (int column = 0; column < qMax(1, columns); ++column) {
        newList.append(QString());
    }

    for (int row = 0; row < qMax(1, rows); ++row) {
        rowList.append(newList);
    }
}


/*!
    Returns the number of items in the row list as the number of rows
    in the model.
*/

int TableModel::rowCount(const QModelIndex &/*parent*/) const
{
    return rowList.size();
}

/*!
    Returns the number of items in the first list item as the number of
    columns in the model. All rows should have the same number of columns.
*/

int TableModel::columnCount(const QModelIndex &/*parent*/) const
{
    return rowList[0].size();
}

/*!
    Returns an appropriate value for the requested data.
    If the view requests an invalid index, an invalid variant is returned.
    Any valid index that corresponds to a string in the list causes that
    string to be returned for the display role; otherwise an invalid variant
    is returned.
*/

QVariant TableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole)
        return rowList[index.row()][index.column()];
    else
        return QVariant();
}

/*!
    Returns the appropriate header string depending on the orientation of
    the header and the section. If anything other than the display role is
    requested, we return an invalid variant.
*/

QVariant TableModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
        return QStringLiteral("Column %1").arg(section);
    else
        return QStringLiteral("Row %1").arg(section);
}

/*!
    Returns an appropriate value for the item's flags. Valid items are
    enabled, selectable, and editable.
*/

Qt::ItemFlags TableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

/*!
    Changes an item in the model, but only if the following conditions
    are met:

    * The index supplied is valid.
    * The role associated with editing text is specified.

    The dataChanged() signal is emitted if the item is changed.
*/

bool TableModel::setData(const QModelIndex &index,
                         const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    rowList[index.row()][index.column()] = value.toString();
    emit dataChanged(index, index, {role});
    return true;
}

/*!
    Inserts a number of rows into the model at the specified position.
*/

bool TableModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    int columns = columnCount();
    beginInsertRows(parent, position, position + rows - 1);

    for (int row = 0; row < rows; ++row) {
        QStringList items;
        for (int column = 0; column < columns; ++column)
            items.append(QString());
        rowList.insert(position, items);
    }

    endInsertRows();
    return true;
}

/*!
    Inserts a number of columns into the model at the specified position.
    Each entry in the list is extended in turn with the required number of
    empty strings.
*/

bool TableModel::insertColumns(int position, int columns,
                               const QModelIndex &parent)
{
    int rows = rowCount();
    beginInsertColumns(parent, position, position + columns - 1);

    for (int row = 0; row < rows; ++row) {
        for (int column = position; column < columns; ++column) {
            rowList[row].insert(position, QString());
        }
    }

    endInsertColumns();
    return true;
}

/*!
    Removes a number of rows from the model at the specified position.
*/

bool TableModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    beginRemoveRows(parent, position, position + rows - 1);

    for (int row = 0; row < rows; ++row) {
        rowList.removeAt(position);
    }

    endRemoveRows();
    return true;
}

/*!
    Removes a number of columns from the model at the specified position.
    Each row is shortened by the number of columns specified.
*/

bool TableModel::removeColumns(int position, int columns,
                               const QModelIndex &parent)
{
    int rows = rowCount();
    beginRemoveColumns(parent, position, position + columns - 1);

    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            rowList[row].removeAt(position);
        }
    }

    endRemoveColumns();
    return true;
}
