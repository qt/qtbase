// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QDebug>

void checkSqlQueryModel()
{
//! [0]
QSqlQueryModel model;
model.setQuery("select * from myTable");
if (model.lastError().isValid())
    qDebug() << model.lastError();
//! [0]
}
