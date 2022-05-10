// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlDriver>
#include <QVariant>

void checkHandle()
{
//dummy definitions
typedef void sqlite3;
typedef void PGconn;
typedef void MYSQL;
//! [0]
QSqlDatabase db = QSqlDatabase::database();
QVariant v = db.driver()->handle();
if (v.isValid() && (qstrcmp(v.typeName(), "sqlite3*") == 0)) {
    // v.data() returns a pointer to the handle
    sqlite3 *handle = *static_cast<sqlite3 **>(v.data());
    if (handle) {
        // ...
    }
}
//! [0]

//! [1]
if (qstrcmp(v.typeName(), "PGconn*") == 0) {
    PGconn *handle = *static_cast<PGconn **>(v.data());
    if (handle) {
        // ...
    }
}

if (qstrcmp(v.typeName(), "MYSQL*") == 0) {
    MYSQL *handle = *static_cast<MYSQL **>(v.data());
    if (handle) {
        // ...
    }
}
//! [1]
}
