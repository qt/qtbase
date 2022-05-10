// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CUSTOMSQLMODEL_H
#define CUSTOMSQLMODEL_H

#include <QSqlQueryModel>

//! [0]
class CustomSqlModel : public QSqlQueryModel
{
    Q_OBJECT

public:
    CustomSqlModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &item, int role) const override;
};
//! [0]

#endif
