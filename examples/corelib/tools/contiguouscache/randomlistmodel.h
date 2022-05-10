// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#ifndef RANDOMLISTMODEL_H
#define RANDOMLISTMODEL_H

#include <QContiguousCache>
#include <QAbstractListModel>

QT_FORWARD_DECLARE_CLASS(QTimer)

class RandomListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    RandomListModel(QObject *parent = nullptr);
    ~RandomListModel();

    int rowCount(const QModelIndex & = QModelIndex()) const override;
    QVariant data(const QModelIndex &, int) const override;

private:
    void cacheRows(int, int) const;
    QString fetchRow(int) const;

    mutable QContiguousCache<QString> m_rows;
    const int m_count;
};

#endif
