// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <qsqldatabase.h>
#include <qstandardpaths.h>

#include "../qsqldatabase/tst_databases.h"

using namespace Qt::StringLiterals;

class tst_QVfsSql : public QObject
{
    Q_OBJECT
private slots:
    void testRoDb();
    void testRwDb();
};

void tst_QVfsSql::testRoDb()
{
    QVERIFY(QSqlDatabase::drivers().contains("QSQLITE"_L1));
    QSqlDatabase::addDatabase("QSQLITE"_L1, "ro_db"_L1);
    QSqlDatabase db = QSqlDatabase::database("ro_db"_L1, false);
    QVERIFY_SQL(db, isValid());
    db.setDatabaseName(":/ro/sample.db"_L1);

    db.setConnectOptions("QSQLITE_USE_QT_VFS"_L1);
    QVERIFY(!db.open()); // can not open as the QSQLITE_OPEN_READONLY attribute is missing

    db.setConnectOptions("QSQLITE_USE_QT_VFS;QSQLITE_OPEN_READONLY"_L1);
    QVERIFY_SQL(db, open());

    QStringList tables = db.tables();
    QSqlQuery q{db};
    for (auto table : {"reltest1"_L1, "reltest2"_L1, "reltest3"_L1, "reltest4"_L1, "reltest5"_L1}) {
        QVERIFY(tables.contains(table));
        QVERIFY_SQL(q, exec("select * from " + table));
        QVERIFY(q.next());
    }
    QVERIFY_SQL(q, exec("select * from reltest1 where id = 4"_L1));
    QVERIFY_SQL(q, first());
    QVERIFY(q.value(0).toInt() == 4);
    QVERIFY(q.value(1).toString() == "boris"_L1);
    QVERIFY(q.value(2).toInt() == 2);
    QVERIFY(q.value(3).toInt() == 2);
}

void tst_QVfsSql::testRwDb()
{
    QSqlDatabase::addDatabase("QSQLITE"_L1, "rw_db"_L1);
    QSqlDatabase db = QSqlDatabase::database("rw_db"_L1, false);
    QVERIFY_SQL(db, isValid());
    const auto dbPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/test_qt_vfs.db"_L1;
    db.setDatabaseName(dbPath);
    QFile::remove(dbPath);

    db.setConnectOptions("QSQLITE_USE_QT_VFS;QSQLITE_OPEN_READONLY"_L1);
    QVERIFY(!db.open()); // can not open as the QSQLITE_OPEN_READONLY attribute is set and the file is missing

    db.setConnectOptions("QSQLITE_USE_QT_VFS"_L1);
    QVERIFY_SQL(db, open());

    QVERIFY(db.tables().isEmpty());
    QSqlQuery q{db};
    QVERIFY_SQL(q, exec("CREATE TABLE test (id INTEGER PRIMARY KEY AUTOINCREMENT, val INTEGER)"_L1));
    QVERIFY_SQL(q, exec("BEGIN"_L1));
    for (int i = 0; i < 1000; ++i) {
        q.prepare("INSERT INTO test (val) VALUES (:val)"_L1);
        q.bindValue(":val"_L1, i);
        QVERIFY_SQL(q, exec());
    }
    QVERIFY_SQL(q, exec("COMMIT"_L1));
    QVERIFY_SQL(q, exec("SELECT val FROM test ORDER BY val"_L1));
    for (int i = 0; i < 1000; ++i) {
        QVERIFY_SQL(q, next());
        QCOMPARE(q.value(0).toInt() , i);
    }
    QVERIFY_SQL(q, exec("DELETE FROM test WHERE val < 500"_L1));
    auto fileSize = QFileInfo{dbPath}.size();
    QVERIFY_SQL(q, exec("VACUUM"_L1));
    QVERIFY(QFileInfo{dbPath}.size() < fileSize); // TEST xTruncate VFS
    QVERIFY_SQL(q, exec("SELECT val FROM test ORDER BY val"_L1));
    for (int i = 500; i < 1000; ++i) {
        QVERIFY_SQL(q, next());
        QCOMPARE(q.value(0).toInt() , i);
    }
    db.close();
    QFile::remove(dbPath);
}

QTEST_MAIN(tst_QVfsSql)
#include "tst_qvfssql.moc"
