// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtSql/QtSql>

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
    void benchmarkSelectPrepared_data() { generic_data(); }
    void benchmarkSelectPrepared();

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

#include "main.moc"
