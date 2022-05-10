// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MODEL_H
#define MODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include <QVariant>

class TableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    TableModel(int rows = 1, int columns = 1, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
    bool insertColumns(int position, int columns, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(int position, int columns, const QModelIndex &parent = QModelIndex()) override;

private:
    QList<QStringList> rowList;
};

#endif
