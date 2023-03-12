// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>
#include <QtSql/QtSql>

#include "../qsqldatabase/tst_databases.h"

class tst_QSqlDriver : public QObject
{
    Q_OBJECT

public:
    void recreateTestTables(QSqlDatabase);

    tst_Databases dbs;

public slots:
    void initTestCase_data();
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void record();
    void primaryIndex();
    void formatValue();
};

static bool driverSupportsDefaultValues(QSqlDriver::DbmsType dbType)
{
    switch (dbType) {
    case QSqlDriver::SQLite:
    case QSqlDriver::PostgreSQL:
    case QSqlDriver::Oracle:
        return true;
    default:
        break;
    }
    return false;
}

void tst_QSqlDriver::initTestCase_data()
{
    QVERIFY(dbs.open());
    if (dbs.fillTestTable() == 0)
        QSKIP("No database drivers are available in this Qt configuration");
}

void tst_QSqlDriver::recreateTestTables(QSqlDatabase db)
{
    QSqlQuery q(db);
    const QString tableName(qTableName("relTEST1", __FILE__, db));
    tst_Databases::safeDropTables(db, {tableName});

    QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    if (dbType == QSqlDriver::PostgreSQL)
        QVERIFY_SQL( q, exec("set client_min_messages='warning'"));

    QString doubleField;
    if (dbType == QSqlDriver::SQLite)
        doubleField = "more_data double";
    else if (dbType == QSqlDriver::Oracle)
        doubleField = "more_data number(8,7)";
    else if (dbType == QSqlDriver::PostgreSQL || dbType == QSqlDriver::MimerSQL)
        doubleField = "more_data double precision";
    else if (dbType == QSqlDriver::Interbase)
        doubleField = "more_data numeric(8,7)";
    else
        doubleField = "more_data double(8,7)";
    const QString defValue(driverSupportsDefaultValues(dbType) ? QStringLiteral("DEFAULT 'defaultVal'") : QString());
    QVERIFY_SQL( q, exec("create table " + tableName +
            " (id int not null primary key, name varchar(20) " + defValue + ", title_key int, another_title_key int, " + doubleField + QLatin1Char(')')));
    QVERIFY_SQL( q, exec("insert into " + tableName + " values(1, 'harry', 1, 2, 1.234567)"));
    QVERIFY_SQL( q, exec("insert into " + tableName + " values(2, 'trond', 2, 1, 8.901234)"));
    QVERIFY_SQL( q, exec("insert into " + tableName + " values(3, 'vohi', 1, 2, 5.678901)"));
    QVERIFY_SQL( q, exec("insert into " + tableName + " values(4, 'boris', 2, 2, 2.345678)"));
}

void tst_QSqlDriver::initTestCase()
{
    for (const QString &dbname : std::as_const(dbs.dbNames))
        recreateTestTables(QSqlDatabase::database(dbname));
}

void tst_QSqlDriver::cleanupTestCase()
{
    for (const QString &dbName : std::as_const(dbs.dbNames)) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        QStringList tables = {qTableName("relTEST1", __FILE__, db)};
        const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
        if (dbType == QSqlDriver::Oracle)
            tables.push_back(qTableName("clobTable", __FILE__, db));
        tst_Databases::safeDropTables(db, tables);
    }
    dbs.close();
}

void tst_QSqlDriver::init()
{
}

void tst_QSqlDriver::cleanup()
{
}

void tst_QSqlDriver::record()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QString tablename(qTableName("relTEST1", __FILE__, db, false));
    QStringList fields;
    fields << "id" << "name" << "title_key" << "another_title_key" << "more_data";

    //check we can get records using an unquoted mixed case table name
    QSqlRecord rec = db.driver()->record(tablename);
    QCOMPARE(rec.count(), fields.size());

    QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    // QTBUG-1363: QSqlField::length() always return -1 when using QODBC driver and QSqlDatabase::record()
    if (dbType == QSqlDriver::MSSqlServer && db.driverName().startsWith("QODBC"))
        QCOMPARE(rec.field(1).length(), 20);

    if (dbType == QSqlDriver::Interbase || dbType == QSqlDriver::Oracle || dbType == QSqlDriver::DB2)
        for(int i = 0; i < fields.size(); ++i)
            fields[i] = fields[i].toUpper();

    for (int i = 0; i < fields.size(); ++i)
        QCOMPARE(rec.fieldName(i), fields[i]);

    if (driverSupportsDefaultValues(dbType))
        QCOMPARE(rec.field(QStringLiteral("name")).defaultValue().toString(), QStringLiteral("defaultVal"));

    if (dbType == QSqlDriver::Oracle || dbType == QSqlDriver::DB2)
        tablename = tablename.toUpper();
    else if (dbType == QSqlDriver::PostgreSQL)
        tablename = tablename.toLower();

    if (dbType != QSqlDriver::PostgreSQL && !db.driverName().startsWith("QODBC")) {
        //check we can get records using a properly quoted table name
        rec = db.driver()->record(db.driver()->escapeIdentifier(tablename,QSqlDriver::TableName));
        QCOMPARE(rec.count(), 5);
    }

    for (int i = 0; i < fields.size(); ++i)
        QCOMPARE(rec.fieldName(i), fields[i]);

    if (dbType == QSqlDriver::Interbase || dbType == QSqlDriver::Oracle || dbType == QSqlDriver::DB2)
        tablename = tablename.toLower();
    else if (dbType == QSqlDriver::PostgreSQL)
        tablename = tablename.toUpper();

    //check that we can't get records using incorrect tablename casing that's been quoted
    rec = db.driver()->record(db.driver()->escapeIdentifier(tablename,QSqlDriver::TableName));
    if (dbType == QSqlDriver::MySqlServer || dbType == QSqlDriver::SQLite
        || dbType == QSqlDriver::Sybase || dbType == QSqlDriver::MSSqlServer
        || tst_Databases::isMSAccess(db) || dbType == QSqlDriver::MimerSQL)
        QCOMPARE(rec.count(), 5); //mysql, sqlite and tds will match
    else
        QCOMPARE(rec.count(), 0);

}

void tst_QSqlDriver::primaryIndex()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QString tablename(qTableName("relTEST1", __FILE__, db, false));
    //check that we can get primary index using unquoted mixed case table name
    QSqlIndex index = db.driver()->primaryIndex(tablename);
    QCOMPARE(index.count(), 1);

    QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    if (dbType == QSqlDriver::Interbase || dbType == QSqlDriver::Oracle || dbType == QSqlDriver::DB2)
        QCOMPARE(index.fieldName(0), QString::fromLatin1("ID"));
    else
        QCOMPARE(index.fieldName(0), QString::fromLatin1("id"));


    //check that we can get the primary index using a quoted tablename
    if (dbType == QSqlDriver::Oracle || dbType == QSqlDriver::DB2)
        tablename = tablename.toUpper();
    else if (dbType == QSqlDriver::PostgreSQL)
        tablename = tablename.toLower();

    if (dbType != QSqlDriver::PostgreSQL && !db.driverName().startsWith("QODBC")) {
        index = db.driver()->primaryIndex(db.driver()->escapeIdentifier(tablename, QSqlDriver::TableName));
        QCOMPARE(index.count(), 1);
    }
    if (dbType == QSqlDriver::Interbase || dbType == QSqlDriver::Oracle || dbType == QSqlDriver::DB2)
        QCOMPARE(index.fieldName(0), QString::fromLatin1("ID"));
    else
        QCOMPARE(index.fieldName(0), QString::fromLatin1("id"));



    //check that we can not get the primary index using a quoted but incorrect table name casing
    if (dbType == QSqlDriver::Interbase || dbType == QSqlDriver::Oracle || dbType == QSqlDriver::DB2)
        tablename = tablename.toLower();
    else if (dbType == QSqlDriver::PostgreSQL)
        tablename = tablename.toUpper();

    index = db.driver()->primaryIndex(db.driver()->escapeIdentifier(tablename, QSqlDriver::TableName));
    if (dbType == QSqlDriver::MySqlServer || dbType == QSqlDriver::SQLite
        || dbType == QSqlDriver::Sybase || dbType == QSqlDriver::MSSqlServer
        || tst_Databases::isMSAccess(db) || dbType == QSqlDriver::MimerSQL)
        QCOMPARE(index.count(), 1); //mysql will always find the table name regardless of casing
    else
        QCOMPARE(index.count(), 0);

    // Test getting a primary index for a table with a clob in it - QTBUG-64427
    if (dbType == QSqlDriver::Oracle) {
        TableScope ts(db, "clobTable", __FILE__);
        QSqlQuery qry(db);
        QVERIFY_SQL(qry, exec("CREATE TABLE " + ts.tableName() + " (id INTEGER, clobField CLOB)"));
        QVERIFY_SQL(qry, exec("CREATE UNIQUE INDEX " + ts.tableName() + "IDX ON " + ts.tableName() + " (id)"));
        QVERIFY_SQL(qry, exec("ALTER TABLE " + ts.tableName() + " ADD CONSTRAINT " + ts.tableName() +
                              "PK PRIMARY KEY(id)"));
        QVERIFY_SQL(qry, exec("ALTER TABLE " + ts.tableName() + " MODIFY (id NOT NULL ENABLE)"));
        const QSqlIndex primaryIndex = db.driver()->primaryIndex(ts.tableName());
        QCOMPARE(primaryIndex.count(), 1);
        QCOMPARE(primaryIndex.fieldName(0), QStringLiteral("ID"));
    }
}

void tst_QSqlDriver::formatValue()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QString tablename(qTableName("relTEST1", __FILE__, db));
    QSqlQuery qry(db);
    QVERIFY_SQL(qry, exec("SELECT * FROM " + tablename));
    qry.next();
    QSqlRecord rec = qry.record();
    QCOMPARE(db.driver()->formatValue(rec.field("id")), QString("1"));
    QCOMPARE(db.driver()->formatValue(rec.field("name")), QString("'harry'"));
    QCOMPARE(db.driver()->formatValue(rec.field("more_data")), QString("1.234567"));
}

QTEST_MAIN(tst_QSqlDriver)
#include "tst_qsqldriver.moc"
