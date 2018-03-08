/****************************************************************************
 **
 ** Copyright (C) 2018 The Qt Company Ltd.
 ** Contact: https://www.qt.io/licensing/
 **
 ** This file is part of the test suite of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:GPL-EXCEPT$
 ** Commercial License Usage
 ** Licensees holding valid commercial Qt licenses may use this file in
 ** accordance with the commercial license agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and The Qt Company. For licensing terms
 ** and conditions see https://www.qt.io/terms-conditions. For further
 ** information use the contact form at https://www.qt.io/contact-us.
 **
 ** GNU General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU
 ** General Public License version 3 as published by the Free Software
 ** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
 ** included in the packaging of this file. Please review the following
 ** information to ensure the GNU General Public License requirements will
 ** be met: https://www.gnu.org/licenses/gpl-3.0.html.
 **
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/

#include <QtTest/QtTest>
#include <QtSql/QtSql>

#include "../../../../auto/sql/kernel/qsqldatabase/tst_databases.h"

const QString qtest(qTableName("qtest", __FILE__, QSqlDatabase()));

class tst_QSqlRecord : public QObject
{
    Q_OBJECT

public:
    tst_QSqlRecord();
    virtual ~tst_QSqlRecord();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void benchmarkRecord_data() { generic_data(); }
    void benchmarkRecord();
    void benchFieldName_data() { generic_data(); }
    void benchFieldName();
    void benchFieldIndex_data() { generic_data(); }
    void benchFieldIndex();

private:
    void generic_data(const QString &engine = QString());
    void dropTestTables(QSqlDatabase db);
    void createTestTables(QSqlDatabase db);
    void populateTestTables(QSqlDatabase db);

    tst_Databases dbs;
};

QTEST_MAIN(tst_QSqlRecord)

tst_QSqlRecord::tst_QSqlRecord()
{
}

tst_QSqlRecord::~tst_QSqlRecord()
{
}

void tst_QSqlRecord::initTestCase()
{
    dbs.open();
    for (const auto &dbName : qAsConst(dbs.dbNames)) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        CHECK_DATABASE(db);
        dropTestTables(db); // In case of leftovers
        createTestTables(db);
        populateTestTables(db);
    }
}

void tst_QSqlRecord::cleanupTestCase()
{
    for (const auto &dbName : qAsConst(dbs.dbNames)) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        CHECK_DATABASE(db);
        dropTestTables(db);
    }
    dbs.close();
}

void tst_QSqlRecord::init()
{
}

void tst_QSqlRecord::cleanup()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);

    if (QTest::currentTestFailed() && (dbType == QSqlDriver::Oracle ||
                                       db.driverName().startsWith("QODBC"))) {
        // Since Oracle ODBC has a problem when encountering an error, we init again
        db.close();
        db.open();
    }
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

void tst_QSqlRecord::dropTestTables(QSqlDatabase db)
{
    QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    QStringList tablenames;
    // drop all the tables in case a testcase failed
    tablenames << qtest
               << qTableName("record", __FILE__, db);
    tst_Databases::safeDropTables(db, tablenames);

    if (dbType == QSqlDriver::Oracle) {
        QSqlQuery q(db);
        q.exec("DROP PACKAGE " + qTableName("pkg", __FILE__, db));
    }
}

void tst_QSqlRecord::createTestTables(QSqlDatabase db)
{
    QSqlQuery q(db);
    switch (tst_Databases::getDatabaseType(db)) {
        case QSqlDriver::PostgreSQL:
            QVERIFY_SQL(q, exec("set client_min_messages='warning'"));
            QVERIFY_SQL(q, exec("create table " + qtest + " (id serial NOT NULL, t_varchar varchar(20), "
                                "t_char char(20), primary key(id)) WITH OIDS"));
            break;
        case QSqlDriver::MySqlServer:
            QVERIFY_SQL(q, exec("set table_type=innodb"));
            Q_FALLTHROUGH();
        default:
            QVERIFY_SQL(q, exec("create table " + qtest + " (id int " + tst_Databases::autoFieldName(db) +
                                " NOT NULL, t_varchar varchar(20), t_char char(20), primary key(id))"));
            break;
    }
}

void tst_QSqlRecord::populateTestTables(QSqlDatabase db)
{
    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("delete from " + qtest));
    QVERIFY_SQL(q, exec("insert into " + qtest + " values (1, 'VarChar1', 'Char1')"));
    QVERIFY_SQL(q, exec("insert into " + qtest + " values (2, 'VarChar2', 'Char2')"));
    QVERIFY_SQL(q, exec("insert into " + qtest + " values (3, 'VarChar3', 'Char3')"));
    QVERIFY_SQL(q, exec("insert into " + qtest + " values (4, 'VarChar4', 'Char4')"));
    QVERIFY_SQL(q, exec("insert into " + qtest + " values (5, 'VarChar5', 'Char5')"));
}

void tst_QSqlRecord::benchmarkRecord()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const auto tableName = qTableName("record", __FILE__, db);
    {
        QSqlQuery qry(db);
        QVERIFY_SQL(qry, exec("create table " + tableName + " (id int NOT NULL, t_varchar varchar(20), "
                              "t_char char(20), primary key(id))"));
        for (int i = 0; i < 1000; i++)
            QVERIFY_SQL(qry, exec(QString("INSERT INTO " + tableName +
                                          " VALUES (%1, 'VarChar%1', 'Char%1')").arg(i)));
        QVERIFY_SQL(qry, exec(QString("SELECT * from ") + tableName));
        QBENCHMARK {
            while (qry.next())
                qry.record();
        }
    }
    tst_Databases::safeDropTables(db, QStringList() << tableName);
}

void tst_QSqlRecord::benchFieldName()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    if (tst_Databases::getDatabaseType(db) == QSqlDriver::PostgreSQL) {
        QSqlQuery qry(db);
        QVERIFY_SQL(qry, exec("SELECT GENERATE_SERIES(1,5000) AS r"));
        QBENCHMARK {
            while (qry.next())
                qry.value("r");
        }
    }
}

void tst_QSqlRecord::benchFieldIndex()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    if (tst_Databases::getDatabaseType(db) == QSqlDriver::PostgreSQL) {
        QSqlQuery qry(db);
        QVERIFY_SQL(qry, exec("SELECT GENERATE_SERIES(1,5000) AS r"));
        qry = db.exec("SELECT GENERATE_SERIES(1,5000) AS r");
        QBENCHMARK {
            while (qry.next())
                qry.value(0);
        }
    }
}

#include "tst_qsqlrecord.moc"
