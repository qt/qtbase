// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtSql/QtSql>
#include <QtCore/qtimezone.h>

#include "../../../../auto/sql/kernel/qsqldatabase/tst_databases.h"

class tst_QSqlQuery : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void benchmark_data() { generic_data(); }
    void benchmark();
    void benchmarkMetadata_data() { generic_data(); }
    void benchmarkMetadata();
    void benchmarkSelectPrepared_data() { generic_data(); }
    void benchmarkSelectPrepared();
    void benchmarkSelectTimeZone_data() { generic_data(); }
    void benchmarkSelectTimeZone();

private:
    // returns all database connections
    void generic_data(const QString &engine = QString());

    tst_Databases dbs;
};

QTEST_MAIN(tst_QSqlQuery)

void tst_QSqlQuery::initTestCase()
{
    dbs.open();
}

void tst_QSqlQuery::cleanupTestCase()
{
    dbs.close();
}

void tst_QSqlQuery::init()
{
}

void tst_QSqlQuery::cleanup()
{
}

void tst_QSqlQuery::generic_data(const QString &engine)
{
    if (dbs.fillTestTable(engine) == 0) {
        if (engine.isEmpty())
           QSKIP( "No database drivers are available in this Qt configuration");
        else
           QSKIP( (QString("No database drivers of type %1 are available in this Qt configuration").arg(engine)).toLocal8Bit());
    }
}

void tst_QSqlQuery::benchmark()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QSqlQuery q(db);
    TableScope ts(db, "benchmark", __FILE__);

    QVERIFY_SQL(q, exec("CREATE TABLE " + ts.tableName() + "(\n"
                        "MainKey INT NOT NULL,\n"
                        "OtherTextCol VARCHAR(45) NOT NULL,\n"
                        "PRIMARY KEY(MainKey))"));

    int i=1;

    QBENCHMARK {
        const QString num = QString::number(i);
        QVERIFY_SQL(q, exec("INSERT INTO " + ts.tableName() + " VALUES(" + num + ", 'Value" + num + "')"));
        i++;
    }
}

// Looks up record() and primaryIndex() for every table of a schema large
// enough to tell a scan of the system tables from an index lookup.
void tst_QSqlQuery::benchmarkMetadata()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    const int NUM_TABLES = 150;
    const int NUM_COLUMNS = 40;

    QStringList tables;
    for (int i = 0; i < NUM_TABLES; ++i)
        tables << qTableName("metadata" + QString::number(i), __FILE__, db);
    tst_Databases::safeDropTables(db, tables);
    const auto dropTables = qScopeGuard([&] { tst_Databases::safeDropTables(db, tables); });

    QString columns = "id INT NOT NULL PRIMARY KEY";
    for (int i = 1; i < NUM_COLUMNS; ++i)
        columns += ", c" + QString::number(i) + " INT";
    QSqlQuery q(db);
    for (const QString &table : std::as_const(tables))
        QVERIFY_SQL(q, exec("CREATE TABLE " + table + " (" + columns + ')'));

    QBENCHMARK {
        int fields = 0;
        int keyFields = 0;

        for (const QString &table : std::as_const(tables)) {
            fields += db.record(table).count();
            keyFields += db.primaryIndex(table).count();
        }

        QCOMPARE(fields, NUM_TABLES * NUM_COLUMNS);
        QCOMPARE(keyFields, NUM_TABLES);
    }
}

void tst_QSqlQuery::benchmarkSelectPrepared()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QSqlQuery q(db);
    TableScope ts(db, "benchmark", __FILE__);

    QVERIFY_SQL(q, exec("CREATE TABLE " + ts.tableName() + "(id INT NOT NULL)"));

    const int NUM_ROWS = 1000;
    int expectedSum = 0;
    QString fillQuery = "INSERT INTO " + ts.tableName() + " VALUES (0)";
    for (int i = 1; i < NUM_ROWS; ++i) {
        fillQuery += ", (" + QString::number(i) + QLatin1Char(')');
        expectedSum += i;
    }
    QVERIFY_SQL(q, exec(fillQuery));

    QVERIFY_SQL(q, prepare("SELECT id FROM " + ts.tableName()));
    QBENCHMARK {
        QVERIFY_SQL(q, exec());
        int sum = 0;

        while (q.next())
            sum += q.value(0).toInt();

        QCOMPARE(sum, expectedSum);
    }
}

// Reads a TIMESTAMP WITH TIME ZONE column whose values all carry the same
// named zone, so each one needs a QTimeZone when it is decoded.
void tst_QSqlQuery::benchmarkSelectTimeZone()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    if (dbType != QSqlDriver::Interbase && dbType != QSqlDriver::FirebirdSQL)
        QSKIP("Implemented only for the Firebird/Interbase family");
    if (tst_Databases::getIbaseEngineVersion(db).majorVersion() < 4)
        QSKIP("Time zone support only implemented for firebird engine version 4 and greater");

    QSqlQuery q(db);
    TableScope ts(db, "benchmark", __FILE__);

    QVERIFY_SQL(q, exec("CREATE TABLE " + ts.tableName()
                        + "(id INT NOT NULL, ts TIMESTAMP WITH TIME ZONE)"));

    // One row per minute, starting at noon in Berlin.
    const int NUM_ROWS = 5000;
    QVERIFY_SQL(q, exec("EXECUTE BLOCK AS DECLARE i INT = 0; BEGIN "
                        "WHILE (i < " + QString::number(NUM_ROWS) + ") DO BEGIN "
                        "INSERT INTO " + ts.tableName() + " VALUES (:i, DATEADD(:i MINUTE TO "
                        "TIMESTAMP '2024-01-15 12:00:00 Europe/Berlin')); "
                        "i = i + 1; END END"));

    const qint64 firstMSecs = QDateTime(QDate(2024, 1, 15), QTime(12, 0),
                                        QTimeZone("Europe/Berlin")).toMSecsSinceEpoch();
    const qint64 n = NUM_ROWS;
    const qint64 expectedSum = n * firstMSecs + 60000 * (n * (n - 1) / 2);

    q.setForwardOnly(true);
    QVERIFY_SQL(q, prepare("SELECT ts FROM " + ts.tableName() + " ORDER BY id"));
    QBENCHMARK {
        QVERIFY_SQL(q, exec());
        qint64 sum = 0;

        while (q.next())
            sum += q.value(0).toDateTime().toMSecsSinceEpoch();

        QCOMPARE(sum, expectedSum);
    }
}

#include "main.moc"
