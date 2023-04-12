// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlDriver>
#include <QSqlResult>
#include <QVariant>
#include <QDebug>

void testProc()
{
//! [2]
QSqlQuery q;
q.exec("call qtestproc (@outval1, @outval2)");
q.exec("select @outval1, @outval2");
if (q.next())
    qDebug() << q.value(0) << q.value(1); // outputs "42" and "43"
//! [2]
}

void callStoredProc()
{
//! [10]
// STORED_PROC uses the return statement or returns multiple result sets
QSqlQuery query;
query.setForwardOnly(true);
query.exec("{call STORED_PROC}");
//! [10]
}

void setHost()
{
//! [24]
QSqlDatabase db;
db.setHostName("MyServer");
db.setDatabaseName("C:\\test.gdb");
//! [24]
}

void exProc()
{
//! [26]
QSqlQuery q;
q.exec("execute procedure my_procedure");
if (q.next())
    qDebug() << q.value(0); // outputs the first RETURN/OUT value
//! [26]

qDebug( \
"QSqlDatabase: QMYSQL driver not loaded \
QSqlDatabase: available drivers: QMYSQL" \
);

/* Commented because the following line is not compilable
//! [34]
column.contains(QRegularExpression("pattern"));
//! [34]
*/
}



void updTable2()
{
QSqlDatabase db;
//! [37]
int value;
QSqlQuery query1;
query1.setForwardOnly(true);
query1.exec("select * FROM table1");
while (query1.next()) {
    value = query1.value(0).toInt();
    if (value == 1) {
        QSqlQuery query2;
        query2.exec("update table2 set col=2");  // WRONG: This will discard all results of
    }                                            // query1, and cause the loop to quit
}
//! [37]
}

void callOutProc()
{
//! [40]
    QSqlDatabase db;
    QSqlQuery query;
    int i1 = 10, i2 = 0;
    query.prepare("call qtestproc(?, ?)");
    query.bindValue(0, i1, QSql::InOut);
    query.bindValue(1, i2, QSql::Out);
    query.exec();
//! [40]
}
