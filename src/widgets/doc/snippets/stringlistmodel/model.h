// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MODEL_H
#define MODEL_H

#include <QAbstractListModel>
#include <QObject>
#include <QStringList>

//! [0]
class StringListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    StringListModel(const QStringList &strings, QObject *parent = nullptr)
        : QAbstractListModel(parent), stringList(strings) {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
//! [0] //! [1]
                        int role = Qt::DisplayRole) const override;
//! [1]

//! [2]
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
//! [2] //! [3]
                 int role = Qt::EditRole) override;
//! [3]

//! [4]
    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
//! [4]

//! [5]
private:
    QStringList stringList;
};
//! [5]

#endif
