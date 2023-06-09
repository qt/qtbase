// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Ivan Komissarov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef STORAGEMODEL_H
#define STORAGEMODEL_H

#include <QAbstractTableModel>
#include <QStorageInfo>

class StorageModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_DISABLE_COPY(StorageModel)
public:
    enum Column {
        ColumnRootPath = 0,
        ColumnName,
        ColumnDevice,
        ColumnFileSystemName,
        ColumnTotal,
        ColumnFree,
        ColumnAvailable,
        ColumnIsReady,
        ColumnIsReadOnly,
        ColumnIsValid,
        ColumnCount
    };

    using QAbstractTableModel::QAbstractTableModel;

    int columnCount(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

public slots:
    void refresh();

private:
    QList<QStorageInfo> m_volumes;
};

#endif // STORAGEMODEL_H
