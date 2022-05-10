// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MYSORTFILTERPROXYMODEL_H
#define MYSORTFILTERPROXYMODEL_H

#include <QDate>
#include <QSortFilterProxyModel>

//! [0]
class MySortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    MySortFilterProxyModel(QObject *parent = nullptr);

    QDate filterMinimumDate() const { return minDate; }
    void setFilterMinimumDate(QDate date);

    QDate filterMaximumDate() const { return maxDate; }
    void setFilterMaximumDate(QDate date);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    bool dateInRange(QDate date) const;

    QDate minDate;
    QDate maxDate;
};
//! [0]

#endif // MYSORTFILTERPROXYMODEL_H
