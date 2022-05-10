// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlDriver>
#include <QDebug>

void openDatabase()
{
//! [0]
// WRONG
QSqlDatabase db = QSqlDatabase::database("sales");
QSqlQuery query("SELECT NAME, DOB FROM EMPLOYEES", db);
QSqlDatabase::removeDatabase("sales"); // will output a warning
// "db" is now a dangling invalid database connection,
// "query" contains an invalid result set
//! [0]
}

void removeDatabase()
{
//! [1]
{
    QSqlDatabase db = QSqlDatabase::database("sales");
    QSqlQuery query("SELECT NAME, DOB FROM EMPLOYEES", db);
}
// Both "db" and "query" are destroyed because they are out of scope
QSqlDatabase::removeDatabase("sales"); // correct
//! [1]
}

void setmyDatabase()
{
//! [3]
// ...
QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
db.setDatabaseName("DRIVER={Microsoft Access Driver (*.mdb, *.accdb)};FIL={MS Access};DBQ=myaccessfile.mdb");
if (db.open()) {
    // success!
}
// ...
//! [3]
}

// ...
// MySQL connection
void dbConnect()
{
QSqlDatabase db;
//! [4]
db.setConnectOptions("SSL_KEY=client-key.pem;SSL_CERT=client-cert.pem;SSL_CA=ca-cert.pem;CLIENT_IGNORE_SPACE=1"); // use an SSL connection to the server
if (!db.open()) {
    db.setConnectOptions(); // clears the connect option string
    // ...
}
// ...
// PostgreSQL connection
db.setConnectOptions("requiressl=1"); // enable PostgreSQL SSL connections
if (!db.open()) {
    db.setConnectOptions(); // clear options
    // ...
}
// ...
// ODBC connection
db.setConnectOptions("SQL_ATTR_ACCESS_MODE=SQL_MODE_READ_ONLY;SQL_ATTR_TRACE=SQL_OPT_TRACE_ON"); // set ODBC options
if (!db.open()) {
    db.setConnectOptions(); // don't try to set this option
    // ...
}
}
//! [4]

void dbQdebug()
{
//! [8]
QSqlDatabase db;
qDebug() << db.isValid();    // Returns false

db = QSqlDatabase::database("sales");
qDebug() << db.isValid();    // Returns \c true if "sales" connection exists

QSqlDatabase::removeDatabase("sales");
qDebug() << db.isValid();    // Returns false
//! [8]
}
