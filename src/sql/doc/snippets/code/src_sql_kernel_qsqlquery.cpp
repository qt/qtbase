// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlDriver>
#include <QDebug>

void selectEmployees()
{
//! [1]
QSqlQuery q("select * from employees");
QSqlRecord rec = q.record();

qDebug() << "Number of columns: " << rec.count();

int nameCol = rec.indexOf("name"); // index of the field "name"
while (q.next())
    qDebug() << q.value(nameCol).toString(); // output all names
//! [1]
//! [2]
QSqlQuery q;
q.prepare("insert into myTable values (?, ?)");

QVariantList ints;
ints << 1 << 2 << 3 << 4;
q.addBindValue(ints);

QVariantList names;
names << "Harald" << "Boris" << "Trond" << QVariant(QMetaType::fromType<QString>());
q.addBindValue(names);

if (!q.execBatch())
    qDebug() << q.lastError();
//! [2]
}
