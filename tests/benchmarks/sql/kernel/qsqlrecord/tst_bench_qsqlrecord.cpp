// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QtSql/QtSql>

#include "../../../../auto/sql/kernel/qsqldatabase/tst_databases.h"

class tst_QSqlRecord : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

private slots:
    void benchmarkRecord_data() { generic_data(); }
    void benchmarkRecord();
    void benchFieldName_data() { generic_data("QPSQL"); }
    void benchFieldName();
    void benchFieldIndex_data() { generic_data("QPSQL"); }
    void benchFieldIndex();

private:
    void generic_data(const QString &engine = QString());

    tst_Databases dbs;
};

QTEST_MAIN(tst_QSqlRecord)

void tst_QSqlRecord::initTestCase()
{
    dbs.open();
}

void tst_QSqlRecord::cleanupTestCase()
{
    dbs.close();
}

void tst_QSqlRecord::cleanup()
{
}

void tst_QSqlRecord::generic_data(const QString &engine)
{
    if (dbs.fillTestTable(engine) == 0) {
        if (engine.isEmpty())
            QSKIP("No database drivers are available in this Qt configuration");
        else
            QSKIP(QString("No database drivers of type %1 are available in this Qt configuration").arg(engine).toLocal8Bit());
    }
}

void tst_QSqlRecord::benchmarkRecord()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "record", __FILE__);
    {
        QSqlQuery qry(db);
        QVERIFY_SQL(qry, exec("create table " + ts.tableName() +
                              " (id int NOT NULL, t_varchar varchar(20), "
                              "t_char char(20), primary key(id))"));
        // Limit to 500: at 600, the set-up takes nearly 5 minutes
        for (int i = 0; i < 500; i++)
            QVERIFY_SQL(qry, exec(QString("INSERT INTO " + ts.tableName() +
                                          " VALUES (%1, 'VarChar%1', 'Char%1')").arg(i)));
        QVERIFY_SQL(qry, exec(QString("SELECT * from ") + ts.tableName()));
        QBENCHMARK {
            while (qry.next())
                qry.record();
            QVERIFY(qry.seek(0));
        }
    }
}

void tst_QSqlRecord::benchFieldName()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QCOMPARE(tst_Databases::getDatabaseType(db), QSqlDriver::PostgreSQL);
    QSqlQuery qry(db);
    QVERIFY_SQL(qry, exec("SELECT GENERATE_SERIES(1,5000) AS r"));
    QBENCHMARK {
        while (qry.next())
            qry.value("r");
        QVERIFY(qry.seek(0));
    }
}

void tst_QSqlRecord::benchFieldIndex()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QCOMPARE(tst_Databases::getDatabaseType(db), QSqlDriver::PostgreSQL);
    QSqlQuery qry(db);
    QVERIFY_SQL(qry, exec("SELECT GENERATE_SERIES(1,5000) AS r"));
    QBENCHMARK {
        while (qry.next())
            qry.value(0);
        QVERIFY(qry.seek(0));
    }
}

#include "tst_bench_qsqlrecord.moc"
