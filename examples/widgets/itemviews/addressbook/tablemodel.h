// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TABLEMODEL_H
#define TABLEMODEL_H

#include "contact.h"

#include <QAbstractTableModel>
#include <QList>

//! [0]
class TableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit TableModel(QObject *parent = nullptr);
    TableModel(const QList<Contact> &contacts, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool insertRows(int position, int rows, const QModelIndex &index = {}) override;
    bool removeRows(int position, int rows, const QModelIndex &index = {}) override;

    const QList<Contact> &getContacts() const;

private:
    QList<Contact> contacts;
};
//! [0]

#endif // TABLEMODEL_H
