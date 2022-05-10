// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mymodel.h"

MyModel::MyModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

//-------------------------------------------------------
int MyModel::rowCount(const QModelIndex & /*parent*/) const
{
    return 2;
}

//-------------------------------------------------------
int MyModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}

//-------------------------------------------------------
QVariant MyModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        return QString("Row%1, Column%2")
                .arg(index.row() + 1)
                .arg(index.column() +1);
    }
    return QVariant();
}

//! [quoting mymodel_c]
QVariant MyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return QString("first");
        case 1:
            return QString("second");
        case 2:
            return QString("third");
        }
    }
    return QVariant();
}
//! [quoting mymodel_c]
