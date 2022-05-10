// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef LOCALEMODEL_H
#define LOCALEMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include <QVariant>

class LocaleModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    LocaleModel(QObject *parent = nullptr);

    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QModelIndex index(int row, int column,
                                const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole ) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value,
                            int role = Qt::EditRole);
private:
    QList<QVariant> m_data_list;
};

#endif // LOCALEMODEL_H
