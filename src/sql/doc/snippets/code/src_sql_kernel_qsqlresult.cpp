// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlResult>
#include <QDebug>

// dummy typedef
typedef void *sqlite3_stmt;

void insertVariants()
{
//! [0]
QSqlQuery q;
q.prepare("insert into test (i1, i2, s) values (?, ?, ?)");

QVariantList col1;
QVariantList col2;
QVariantList col3;

col1 << 1 << 3;
col2 << 2 << 4;
col3 << "hello" << "world";

q.bindValue(0, col1);
q.bindValue(1, col2);
q.bindValue(2, col3);

if (!q.execBatch())
    qDebug() << q.lastError();
//! [0]
}

void querySqlite()
{
//! [1]
QSqlDatabase db = QSqlDatabase::database("sales");
QSqlQuery query("SELECT NAME, DOB FROM EMPLOYEES", db);

QVariant v = query.result()->handle();
if (v.isValid() && qstrcmp(v.typeName(), "sqlite3_stmt*") == 0) {
    // v.data() returns a pointer to the handle
    sqlite3_stmt *handle = *static_cast<sqlite3_stmt **>(v.data());
    if (handle) {
        // ...
    }
}
//! [1]
}
